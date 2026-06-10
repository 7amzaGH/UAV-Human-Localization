# AlienSight C++ Embedded Runtime Skeleton

This folder contains a lightweight C++ runtime skeleton for the **AlienSight** UAV-SAR framework.

The goal is to demonstrate how the AlienSight detection, geolocation, and alerting workflow can be expressed in a deployment-oriented C++ structure.

This component does **not** perform neural network inference, UAV flight control, onboard autonomy, camera streaming, email transmission, or full embedded deployment.

It is intentionally limited to the **runtime decision logic** connecting the main AlienSight perception stages.

---

## Purpose

The C++ runtime skeleton reproduces the high-level AlienSight workflow:

```text
Detection stage is always active
        ↓
If a person is detected above the confidence threshold, geolocation becomes active
        ↓
The target GPS position is estimated from the bounding-box center and UAV telemetry
        ↓
If alerting is enabled, the system sends one alert for the detection session
        ↓
The runtime reports the current system-level state
```

This is useful for showing how the framework logic could later be integrated into embedded software, robotic middleware, or onboard UAV perception systems.

---

## Runtime States

| State | Meaning |
|---|---|
| `NO_DETECTION` | No person is detected in the current frame |
| `DETECTION_LOW_CONFIDENCE` | A person-like detection exists, but confidence is below the runtime threshold |
| `TARGET_GEOLOCATED` | A valid target is detected and geolocated, but an alert was already sent earlier in the session |
| `ALERT_SENT` | A valid target is detected, geolocated, and reported for the first time |
| `ALERT_SUPPRESSED` | A valid target is detected and geolocated, but alerting is disabled for this frame |

---

## Folder Structure

```text
cpp_runtime/
│
├── README.md
├── CMakeLists.txt
│
├── include/
│   └── aliensight_runtime.hpp
│
├── src/
│   ├── aliensight_runtime.cpp
│   └── main.cpp
│
└── examples/
    ├── scenario_01_no_detection.csv
    ├── scenario_02_person_detected_nadir.csv
    ├── scenario_03_person_detected_oblique.csv
    └── scenario_04_alert_triggered.csv
```

---

## Build

From inside `cpp_runtime/`:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

---

## Run

Run the alert-triggered scenario:

```bash
./aliensight_runtime ../examples/scenario_04_alert_triggered.csv
```

Run with a custom detection threshold:

```bash
./aliensight_runtime ../examples/scenario_04_alert_triggered.csv 0.60
```

On Windows with a Visual Studio generator, the executable may be located in `Debug/`:

```bash
./Debug/aliensight_runtime.exe ../examples/scenario_04_alert_triggered.csv
```

---

## Input Format

The runtime reads simplified CSV scenario files.

Each row represents one UAV video frame or one sampled perception step.

```csv
frame,person_detected,confidence,bbox_cx,bbox_cy,drone_lat,drone_lon,altitude_m,heading_deg,gimbal_pitch_deg,alert_enabled
```

The fields simulate outputs and telemetry inputs used by the AlienSight pipeline:

| CSV Field | Meaning |
|---|---|
| `person_detected` | Human detection result from the detector |
| `confidence` | Detection confidence score |
| `bbox_cx`, `bbox_cy` | Bounding-box center in image coordinates |
| `drone_lat`, `drone_lon` | UAV GPS position |
| `altitude_m` | UAV altitude above ground in metres |
| `heading_deg` | UAV heading angle in degrees |
| `gimbal_pitch_deg` | Simplified camera angle convention: `0` = nadir, negative values = oblique |
| `alert_enabled` | Whether the runtime is allowed to issue an alert for this frame |

The example coordinates are synthetic placeholders and are not intended to represent a real rescue location.

---

## Example Output

```text
[Frame 04] System State: ALERT_SENT
  Detection: ACTIVE  | person detected | confidence=0.92 | bbox_center=(970.00,540.00)
  Geolocation: ACTIVE  | target_lat=36.123402 | target_lon=1.234503 | east=0.25 m | north=0.18 m
  Alert: ACTIVE  | alert sent
```

---

## Scope Note

This C++ runtime is a **deployment-oriented skeleton**, not a complete embedded implementation.

It does not claim:

- real-time onboard UAV deployment,
- closed-loop UAV control,
- autonomous flight,
- real neural network inference,
- camera driver integration,
- real email or network communication,
- emergency-response validation.

Its purpose is to show how the AlienSight perception and alerting logic can be written as a clean C++ runtime prototype.

---

## Relation to the Python Pipeline

The Python implementation is used for detection, geolocation evaluation, visualization, and system demonstration.

The C++ runtime skeleton is used to demonstrate a lightweight embedded-style implementation of the high-level runtime logic.

```text
Python pipeline
        ↓
Research implementation, evaluation, and visualization

C++ runtime skeleton
        ↓
Embedded-style runtime logic prototype
```

Both components support the same AlienSight framework idea while remaining scientifically honest about the validated scope.
