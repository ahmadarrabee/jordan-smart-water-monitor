# Low-Cost Smart Water Conservation System for Rural Northern Jordan

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
![Target Cost: <27 JOD](https://img.shields.io/badge/Target%20Cost-%3C27%20JOD-green)
![MCU: ESP32](https://img.shields.io/badge/MCU-ESP32-red)

An ESP32 water monitor that measures tank levels and outlet flow, sends readings over MQTT, and saves failed transmissions locally. A Python script checks recorded data for unusual water use.

## Why I built this

In Northern Jordan, municipal water usually arrives once or twice a week. Households, farms, and businesses store it in tanks and cisterns until the next supply cycle. Hidden leaks, stuck float valves, and overflowing tanks can drain that reserve.

I saw these problems at my uncle's commercial farms and gas station. My uncle, who is married to my dad's sister, had trouble tracking exact tank levels and finding hidden leaks across his sites. I designed this monitor to help him keep track of the stored water and spot problems earlier, with a target hardware cost below 27 JOD, or about $38 USD.

I gave him the design, and he successfully deployed it at his farms and fuel station facilities. We plan to expand to other businesses and households with similar needs.

This repository contains the firmware, sample data, analytics script, and installation notes.

## How it works

The ESP32 reads two sensors:

- A **JSN-SR04T waterproof ultrasonic sensor** measures the distance to the water surface. The firmware uses the tank height and sensor mounting offset to calculate how full the tank is.
- A **YF-S201 flow sensor** counts pulses as water passes through the outlet pipe. A calibration factor converts those pulses to liters per minute.

Readings are sent over Wi-Fi using MQTT. Failed transmissions go into a local SPIFFS log when storage is available. The Python analytics script runs separately on recorded data.

### Level readings

Each measurement cycle takes 10 ultrasonic samples and calculates the median of the valid readings. This helps reduce sudden changes caused by water moving around during a refill.

The firmware needs at least six valid samples. If it gets fewer, it marks the level as invalid. It also sets a high-level flag when the tank is above 95%. The outlet sensor cannot measure incoming water, so that flag does not confirm an overflow or a stuck inlet valve.

### Local storage

When a reading cannot be sent, the firmware writes it to a JSONL file on SPIFFS. The file has a 128 KiB limit. Once it is full, the firmware keeps the existing records and reports any new readings it cannot save over serial.

Live MQTT publishing resumes when the connection returns. Automatic upload of saved records is still planned. The current code has no tool for retrieving or clearing the log.

Readings include UTC timestamps after network time synchronization. Until then, the timestamp is null. Device uptime is also recorded.

### Checking for unusual water use

The analytics script uses Isolation Forest with three inputs: tank level, flow rate, and whether the reading was taken at night. The aim is to help find slow, continuous leaks without flagging normal periods of heavy use.

The current script fits and scores the supplied data with a 5% contamination setting. The sample shows how the analysis runs. Measuring leak-detection accuracy and false alarms still requires field data and testing.

## Bill of materials

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

The target is below 27 JOD. The parts listed here currently add up to exactly 27 JOD, and prices may vary by supplier. The battery module alone is not a complete backup power supply. Regulation, protection, and any required level shifting may add to the installation cost.

## Setup

### Firmware

1. Install Visual Studio Code and the PlatformIO IDE extension.
2. Clone this repository and open `firmware/` as the PlatformIO project.
3. Enter your Wi-Fi details in `firmware/src/main.cpp`:

   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

4. Set the MQTT broker. Adjust `TANK_HEIGHT_CM`, `SENSOR_FULL_DISTANCE_CM`, and `FLOW_K_FACTOR` for your tank and sensors. Keep the full water surface outside the ultrasonic sensor's blind zone.
5. Connect the ESP32 by USB. Click **Build**, then **Upload** in PlatformIO.
6. Open the serial monitor at **115200 baud** to check readings and error messages.

Follow the [mounting and calibration notes](hardware/enclosure/mounting_guide.md) before installing the hardware. Set up the SPIFFS filesystem before using local logging. The firmware does not format it automatically because doing so could erase saved readings.

The default MQTT broker is public and the connection is unencrypted. Use authentication and encrypted transport for private deployment data. Do not commit real Wi-Fi credentials.

### Analytics

Use Python 3.12. Run these commands from the repository root:

```bash
pip install pandas numpy scikit-learn
python analytics/anomaly_detector.py
```

To install the specific dependency versions used by the project, use this instead of the first command:

```bash
python -m pip install -r analytics/requirements.txt
```

The script uses `analytics/data/sample_telemetry.csv` by default. To supply your own file:

```bash
python analytics/anomaly_detector.py path/to/telemetry.csv
```

The CSV needs these columns:

| Column | Value |
| :--- | :--- |
| `timestamp` | Date and time of the reading |
| `tank_level_pct` | Tank level from 0 to 100 percent |
| `flow_rate_lpm` | Flow rate in liters per minute, zero or greater |

The script prints the readings it flags as unusual. It rejects missing timestamps, out-of-range tank levels, and non-finite measurements. Convert firmware JSONL records to CSV before using them, leaving out readings with invalid levels or unknown timestamps.

The night indicator covers 01:00 through 05:59 in the input timestamp's timezone. Convert UTC readings to local time first if you want to analyze local nighttime use.

### Tests

```bash
python -m unittest discover -s analytics -p 'test_*.py'
```

GitHub Actions builds the firmware and runs the analytics script and tests. Check sensor calibration and readings on the installed hardware as well.

## Files

| Path | Contents |
| :--- | :--- |
| `firmware/src/main.cpp` | Sensor readings, MQTT publishing, and local logging |
| `firmware/platformio.ini` | ESP32 build configuration |
| `analytics/anomaly_detector.py` | Isolation Forest analysis |
| `analytics/data/sample_telemetry.csv` | Sample readings |
| `analytics/requirements.txt` | Pinned Python dependencies |
| `analytics/test_anomaly_detector.py` | Analytics tests |
| `hardware/enclosure/mounting_guide.md` | Installation and calibration notes |
| `.github/workflows/ci.yml` | Automated build and analytics checks |

## Author and license

Created by **Ahmad Arrabee**. Released under the [MIT License](LICENSE).

Copyright © 2026 Ahmad Arrabee.
