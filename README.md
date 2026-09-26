# Low-Cost Smart Water Conservation System for Rural Northern Jordan

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
![Target Cost: <27 JOD](https://img.shields.io/badge/Target%20Cost-%3C27%20JOD-green)
![MCU: ESP32](https://img.shields.io/badge/MCU-ESP32-red)

An affordable water-monitoring system designed to help households, farms, and businesses understand their stored water supply and identify potential water loss between municipal delivery cycles.

## Overview and Motivation

In Northern Jordan, municipal water is supplied intermittently, typically **1–2 times per week**. Households, farms, and businesses therefore depend on rooftop tanks and cisterns to store water between supply cycles. An undetected leak, a stuck float valve, or an overflowing tank can waste water that must last until the next delivery.

Knowing how much water remains—and recognizing unusual consumption early—can help owners respond before a small fault becomes a serious shortage.

### Origin Story

I am **Ahmad Arrabee**, the inventor and designer of this project. The idea grew from observing water-management problems at commercial farms and a gas station owned by my uncle, my paternal aunt's husband. Across these facilities, it was difficult to track exact tank water levels and identify hidden leaks.

I designed an ultra-low-cost IoT edge node with a target hardware cost of **less than 27 JOD (approximately $38 USD)** to address these practical problems. By combining water-level sensing, flow measurement, and local data processing, the design aims to make water monitoring accessible to smaller businesses and households.

### Deployment and Expansion

I provided the design to my uncle, who successfully deployed it across his agricultural farms and fuel-station facilities. Plans for further expansion include additional sites operated by other businesses and households facing similar water-storage challenges.

This repository contains the ESP32 firmware, a Python analytics demonstration, sample telemetry, and installation notes. The implementation details and current limitations below describe the software included here.

## System Architecture and Features

The system combines two sensors with an ESP32 edge node. The node processes readings locally, publishes telemetry over Wi-Fi using MQTT, and stores failed transmissions locally when storage is available. A separate Python analytics pipeline examines recorded measurements for unusual patterns.

```text
JSN-SR04T ultrasonic sensor ── Tank level ──┐
                                          ├── ESP32 ── Wi-Fi / MQTT ── Telemetry
YF-S201 inline flow sensor ─── Water flow ──┘      │
                                                 └── SPIFFS local fallback log

Recorded telemetry ── CSV preparation ── Python Isolation Forest ── Anomaly flags
```

### Dual-Sensor Array

- **JSN-SR04T waterproof ultrasonic sensor:** Measures the distance to the water surface without contacting the water. Calibrated tank dimensions and mounting offset convert distance into a tank-level percentage.
- **YF-S201 inline pulse flow sensor:** Measures water flow using pulse counts and a calibrated conversion factor. The current installation notes place the meter on the tank outlet.

### Noise-Filtered Edge Firmware

The ESP32 takes **10 ultrasonic samples per measurement cycle** and calculates the median of the valid readings. This filtering is intended to reduce false level drops caused by wave sloshing during municipal refills.

At least six valid samples are required. If too few readings are usable, the firmware reports an invalid level instead of treating a sensor timeout as an empty tank. Filtering reduces transient noise; it does not guarantee the elimination of all measurement errors.

The firmware also reports a high-level flag above 95% capacity. Because the flow sensor measures outlet flow, this flag alone does not confirm an overflow or a stuck inlet valve.

### Offline Resilience

Failed network transmissions are automatically written to a **SPIFFS local JSONL log**, provided the filesystem is mounted and space is available. The log is bounded at **128 KiB**; once full, existing entries are preserved and new readings that cannot be stored are reported over serial.

**Automatic synchronization of stored readings upon reconnection is a planned capability.** The current firmware resumes live MQTT publishing when connectivity returns, but does not replay the local log. Stored readings currently require a separate retrieval and clearing procedure.

Timestamps use UTC after network time synchronization. Before synchronization, the timestamp is null and device uptime is included instead.

### Machine Learning Anomaly Detection

The Python **Isolation Forest** pipeline analyzes tank-level percentage, flow rate, and a nighttime indicator to identify unusual readings. The intended application is to flag patterns consistent with slow, continuous leaks while distinguishing them from legitimate high-use periods.

The included pipeline fits and scores the supplied dataset with an assumed 5% contamination rate. It is an exploratory demonstration: reliable slow-leak detection and low false-alarm rates on high-use days still require representative field data, tuning, and validation. The repository does not establish a zero-false-alarm guarantee.

## Bill of Materials (BOM)

| Component | Model / Specification | Quantity | Unit Cost (JOD) | Total Cost (JOD) |
| :--- | :--- | :---: | ---: | ---: |
| Microcontroller | ESP32-WROOM-32 | 1 | 4.50 | 4.50 |
| Waterproof ultrasonic sensor | JSN-SR04T V3.0 | 1 | 6.50 | 6.50 |
| Inline flow sensor | YF-S201, 1/2-inch | 1 | 3.50 | 3.50 |
| Weatherproof enclosure | IP65 | 1 | 3.00 | 3.00 |
| Battery module | TP4056 + 18650 Li-ion | 1 | 2.50 | 2.50 |
| Power supply | 5 V, 2 A wall adapter | 1 | 2.50 | 2.50 |
| Cabling, glands, and headers | Installation components | 1 set | 4.50 | 4.50 |
| **Total** | | | | **27.00 JOD (~$38 USD)** |

The badge represents the **target of less than 27 JOD**; the listed BOM currently totals **exactly 27.00 JOD**. These are indicative project estimates, and actual sourcing and installation costs may vary. The battery-module entry is not a complete validated backup-power circuit; regulation, protection, and any required level shifting must be accounted for in the final installation.

## Repository Structure

```text
.github/workflows/ci.yml             Firmware build and analytics checks
firmware/platformio.ini             PlatformIO environment and dependencies
firmware/src/main.cpp               ESP32 sensing, MQTT, and local logging
analytics/anomaly_detector.py       Isolation Forest analytics pipeline
analytics/requirements.txt          Pinned analytics dependencies
analytics/test_anomaly_detector.py  Analytics regression tests
analytics/data/sample_telemetry.csv Example input dataset
hardware/enclosure/mounting_guide.md Installation and calibration notes
LICENSE                             MIT License
```

## Setup and Usage

### Firmware Compilation with VS Code and PlatformIO

1. Install **Visual Studio Code** and the **PlatformIO IDE** extension.
2. Clone this repository and open the **`firmware/`** directory as the PlatformIO project.
3. Update the Wi-Fi settings in **`firmware/src/main.cpp`**:

   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

4. Set the MQTT broker and calibrate `TANK_HEIGHT_CM`, `SENSOR_FULL_DISTANCE_CM`, and `FLOW_K_FACTOR` for the actual installation. Keep the full water surface outside the ultrasonic sensor's specified blind zone.
5. Connect the ESP32 by USB, select **Build**, and then **Upload** in PlatformIO.
6. Open the serial monitor at **115200 baud** to inspect readings and connection or storage messages.

Read the [installation and calibration notes](hardware/enclosure/mounting_guide.md) before wiring and mounting the hardware. Provision the SPIFFS filesystem before relying on offline storage; the firmware deliberately avoids automatic formatting to protect existing logs.

The supplied MQTT broker is public and unencrypted and is intended for demonstration. Configure appropriate broker authentication and encrypted transport before using private deployment data, and keep real Wi-Fi credentials out of commits.

### Running the Analytics Engine

Use **Python 3.12**. From the repository root, install the dependencies and run the sample analysis:

```bash
pip install pandas numpy scikit-learn
python analytics/anomaly_detector.py
```

For the pinned dependency versions used by this project:

```bash
python -m pip install -r analytics/requirements.txt
python analytics/anomaly_detector.py
```

To analyze another CSV file:

```bash
python analytics/anomaly_detector.py path/to/telemetry.csv
```

The CSV must contain these columns:

| Column | Meaning |
| :--- | :--- |
| `timestamp` | Date and time of the reading |
| `tank_level_pct` | Tank level from 0 to 100 percent |
| `flow_rate_lpm` | Nonnegative flow rate in liters per minute |

The script prints readings flagged as potential anomalies. Missing timestamps, invalid tank levels, and non-finite measurements are rejected. Firmware JSONL records must be converted to CSV before analysis; exclude records with invalid sensor measurements or unknown timestamps.

The nighttime feature covers **01:00–05:59 in the input timestamp's timezone**. Convert UTC telemetry to the intended local timezone before analysis when local nighttime behavior is relevant.

### Validation

Run the analytics regression tests from the repository root:

```bash
python -m unittest discover -s analytics -p 'test_*.py'
```

GitHub Actions also builds the ESP32 firmware and runs the analytics demonstration and tests. These checks verify software execution; sensor calibration and field performance require validation on the installed hardware.

## Author and License

**Author:** Ahmad Arrabee

**License:** [MIT License](LICENSE)

Copyright © 2026 Ahmad Arrabee.
