#include "aliensight_runtime.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace aliensight
{

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthCircumferenceMeters = 40075000.0;

std::string trim(const std::string& value)
{
    const std::string whitespace = " \t\n\r\f\v";

    const std::size_t start = value.find_first_not_of(whitespace);

    if (start == std::string::npos)
    {
        return "";
    }

    const std::size_t end = value.find_last_not_of(whitespace);

    return value.substr(start, end - start + 1);
}

std::vector<std::string> split_csv_line(const std::string& line)
{
    std::vector<std::string> tokens;
    std::stringstream stream(line);
    std::string token;

    while (std::getline(stream, token, ','))
    {
        tokens.push_back(trim(token));
    }

    return tokens;
}

bool parse_bool(const std::string& value)
{
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return lowered == "1" || lowered == "true" || lowered == "yes";
}

double parse_double_or_zero(const std::string& value)
{
    if (value.empty())
    {
        return 0.0;
    }

    return std::stod(value);
}

double deg_to_rad(double degrees)
{
    return degrees * kPi / 180.0;
}

GeolocationEstimate estimate_target_position(
    const FrameObservation& frame,
    const RuntimeConfig& config
)
{
    const double image_center_x = static_cast<double>(config.image_width) / 2.0;
    const double image_center_y = static_cast<double>(config.image_height) / 2.0;

    const double pixel_dx = frame.bbox_cx - image_center_x;
    const double pixel_dy = -(frame.bbox_cy - image_center_y);

    const double scale_x =
        (frame.altitude_m * std::tan(deg_to_rad(config.fov_horizontal_deg) / 2.0)) /
        image_center_x;

    const double scale_y =
        (frame.altitude_m * std::tan(deg_to_rad(config.fov_vertical_deg) / 2.0)) /
        image_center_y;

    double camera_dx_m = pixel_dx * scale_x;
    double camera_dy_m = pixel_dy * scale_y;

    // Simplified oblique-view compensation.
    // Convention: 0 deg = nadir, negative values represent forward oblique viewing.
    if (frame.gimbal_pitch_deg < 0.0)
    {
        camera_dy_m += frame.altitude_m * std::tan(std::abs(deg_to_rad(frame.gimbal_pitch_deg)));
    }

    const double heading_rad = deg_to_rad(frame.heading_deg);

    const double east_m =
        std::cos(heading_rad) * camera_dx_m - std::sin(heading_rad) * camera_dy_m;

    const double north_m =
        std::sin(heading_rad) * camera_dx_m + std::cos(heading_rad) * camera_dy_m;

    const double delta_lat = (north_m / kEarthCircumferenceMeters) * 360.0;
    const double delta_lon =
        (east_m / (kEarthCircumferenceMeters * std::cos(deg_to_rad(frame.drone_lat)))) * 360.0;

    GeolocationEstimate estimate;
    estimate.target_lat = frame.drone_lat + delta_lat;
    estimate.target_lon = frame.drone_lon + delta_lon;
    estimate.offset_east_m = east_m;
    estimate.offset_north_m = north_m;

    return estimate;
}

}  // namespace

AlienSightRuntime::AlienSightRuntime(RuntimeConfig config)
    : config_(config)
{
}

RuntimeDecision AlienSightRuntime::process(const FrameObservation& frame)
{
    RuntimeDecision decision;
    decision.frame_id = frame.frame_id;
    decision.input = frame;

    // The detection stage is always active in this simplified runtime skeleton.
    decision.detection_stage_active = true;

    const bool valid_detection =
        frame.person_detected && frame.confidence >= config_.detection_threshold;

    decision.geolocation_stage_active = valid_detection;

    if (!frame.person_detected)
    {
        decision.system_state = SystemState::NO_DETECTION;
        return decision;
    }

    if (!valid_detection)
    {
        decision.system_state = SystemState::DETECTION_LOW_CONFIDENCE;
        return decision;
    }

    decision.geolocation = estimate_target_position(frame, config_);

    if (!frame.alert_enabled)
    {
        decision.system_state = SystemState::ALERT_SUPPRESSED;
        return decision;
    }

    decision.alert_stage_active = true;
    decision.alert_already_sent = alert_sent_;

    if (!alert_sent_)
    {
        decision.alert_sent_this_frame = true;
        decision.system_state = SystemState::ALERT_SENT;
        alert_sent_ = true;
    }
    else
    {
        decision.system_state = SystemState::TARGET_GEOLOCATED;
    }

    return decision;
}

const RuntimeConfig& AlienSightRuntime::config() const
{
    return config_;
}

void AlienSightRuntime::reset_alert_session()
{
    alert_sent_ = false;
}

std::string to_string(SystemState state)
{
    switch (state)
    {
        case SystemState::NO_DETECTION:
            return "NO_DETECTION";
        case SystemState::DETECTION_LOW_CONFIDENCE:
            return "DETECTION_LOW_CONFIDENCE";
        case SystemState::TARGET_GEOLOCATED:
            return "TARGET_GEOLOCATED";
        case SystemState::ALERT_SENT:
            return "ALERT_SENT";
        case SystemState::ALERT_SUPPRESSED:
            return "ALERT_SUPPRESSED";
        default:
            return "UNKNOWN";
    }
}

std::vector<FrameObservation> load_scenario_csv(const std::string& csv_path)
{
    std::ifstream file(csv_path);

    if (!file.is_open())
    {
        throw std::runtime_error("Could not open CSV file: " + csv_path);
    }

    std::vector<FrameObservation> scenario;
    std::string line;

    // Skip header.
    if (!std::getline(file, line))
    {
        throw std::runtime_error("CSV file is empty: " + csv_path);
    }

    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        const auto tokens = split_csv_line(line);

        if (tokens.size() < 11)
        {
            throw std::runtime_error(
                "Invalid CSV row. Expected 11 fields but got " +
                std::to_string(tokens.size()) + ": " + line
            );
        }

        FrameObservation frame;

        frame.frame_id = std::stoi(tokens[0]);
        frame.person_detected = parse_bool(tokens[1]);
        frame.confidence = parse_double_or_zero(tokens[2]);
        frame.bbox_cx = parse_double_or_zero(tokens[3]);
        frame.bbox_cy = parse_double_or_zero(tokens[4]);
        frame.drone_lat = parse_double_or_zero(tokens[5]);
        frame.drone_lon = parse_double_or_zero(tokens[6]);
        frame.altitude_m = parse_double_or_zero(tokens[7]);
        frame.heading_deg = parse_double_or_zero(tokens[8]);
        frame.gimbal_pitch_deg = parse_double_or_zero(tokens[9]);
        frame.alert_enabled = parse_bool(tokens[10]);

        scenario.push_back(frame);
    }

    return scenario;
}

void print_decision(const RuntimeDecision& decision)
{
    const auto& input = decision.input;

    std::cout << "[Frame "
              << std::setw(2) << std::setfill('0') << decision.frame_id
              << "] System State: " << to_string(decision.system_state)
              << std::setfill(' ') << "\n";

    if (decision.detection_stage_active && input.person_detected)
    {
        std::cout << "  Detection: ACTIVE  | person detected"
                  << " | confidence=" << std::fixed << std::setprecision(2)
                  << input.confidence
                  << " | bbox_center=(" << input.bbox_cx << "," << input.bbox_cy << ")\n";
    }
    else
    {
        std::cout << "  Detection: ACTIVE  | no person detected\n";
    }

    if (decision.geolocation_stage_active && decision.geolocation.has_value())
    {
        const auto& geo = decision.geolocation.value();

        std::cout << "  Geolocation: ACTIVE  | target_lat="
                  << std::fixed << std::setprecision(6) << geo.target_lat
                  << " | target_lon=" << geo.target_lon
                  << " | east=" << std::setprecision(2) << geo.offset_east_m << " m"
                  << " | north=" << geo.offset_north_m << " m\n";
    }
    else if (input.person_detected)
    {
        std::cout << "  Geolocation: INACTIVE | detection below confidence threshold\n";
    }
    else
    {
        std::cout << "  Geolocation: INACTIVE | waiting for valid detection\n";
    }

    if (decision.alert_stage_active)
    {
        std::cout << "  Alert: ACTIVE  | "
                  << (decision.alert_sent_this_frame ? "alert sent" : "target already reported")
                  << "\n";
    }
    else if (decision.system_state == SystemState::ALERT_SUPPRESSED)
    {
        std::cout << "  Alert: INACTIVE | alert disabled for this frame\n";
    }
    else
    {
        std::cout << "  Alert: INACTIVE | waiting for geolocated target\n";
    }

    std::cout << "\n";
}

}  // namespace aliensight
