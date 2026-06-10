#include "aliensight_runtime.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " <scenario_csv> [detection_threshold]\n\n"
                  << "Example:\n"
                  << "  " << argv[0] << " ../examples/scenario_04_alert_triggered.csv\n"
                  << "  " << argv[0] << " ../examples/scenario_04_alert_triggered.csv 0.60\n";

        return EXIT_FAILURE;
    }

    const std::string scenario_path = argv[1];

    aliensight::RuntimeConfig config;

    if (argc >= 3)
    {
        config.detection_threshold = std::stod(argv[2]);
    }

    try
    {
        const auto scenario = aliensight::load_scenario_csv(scenario_path);

        aliensight::AlienSightRuntime runtime(config);

        std::cout << "AlienSight C++ Embedded Runtime Skeleton\n";
        std::cout << "Scenario file: " << scenario_path << "\n";
        std::cout << "Detection threshold: "
                  << runtime.config().detection_threshold << "\n\n";

        for (const auto& frame : scenario)
        {
            const auto decision = runtime.process(frame);
            aliensight::print_decision(decision);
        }

        std::cout << "Simulation completed.\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "Runtime error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
