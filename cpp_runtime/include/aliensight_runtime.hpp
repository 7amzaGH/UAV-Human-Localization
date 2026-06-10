#ifndef ALIENSIGHT_RUNTIME_HPP
#define ALIENSIGHT_RUNTIME_HPP

#include <optional>
#include <string>
#include <vector>

namespace aliensight
{

enum class SystemState
{
    NO_DETECTION,
    DETECTION_LOW_CONFIDENCE,
    TARGET_GEOLOCATED,
    ALERT_SENT,
    ALERT_SUPPRESSED
};

struct FrameObservation
{
    int frame_id = 0;

    bool person_detected = false;
    double confidence = 0.0;

    double bbox_cx = 0.0;
    double bbox_cy = 0.0;

    double drone_lat = 0.0;
    double drone_lon = 0.0;
    double altitude_m = 0.0;
    double heading_deg = 0.0;
    double gimbal_pitch_deg = 0.0;

    bool alert_enabled = true;
};

struct RuntimeConfig
{
    double detection_threshold = 0.50;

    int image_width = 1920;
    int image_height = 1080;

    double fov_horizontal_deg = 76.0;
    double fov_vertical_deg = 49.0;
};

struct GeolocationEstimate
{
    double target_lat = 0.0;
    double target_lon = 0.0;
    double offset_east_m = 0.0;
    double offset_north_m = 0.0;
};

struct RuntimeDecision
{
    int frame_id = 0;

    bool detection_stage_active = true;
    bool geolocation_stage_active = false;
    bool alert_stage_active = false;

    bool alert_sent_this_frame = false;
    bool alert_already_sent = false;

    SystemState system_state = SystemState::NO_DETECTION;

    FrameObservation input;
    std::optional<GeolocationEstimate> geolocation;
};

class AlienSightRuntime
{
public:
    explicit AlienSightRuntime(RuntimeConfig config = RuntimeConfig{});

    RuntimeDecision process(const FrameObservation& frame);

    const RuntimeConfig& config() const;

    void reset_alert_session();

private:
    RuntimeConfig config_;
    bool alert_sent_ = false;
};

std::string to_string(SystemState state);

std::vector<FrameObservation> load_scenario_csv(const std::string& csv_path);

void print_decision(const RuntimeDecision& decision);

}  // namespace aliensight

#endif  // ALIENSIGHT_RUNTIME_HPP
