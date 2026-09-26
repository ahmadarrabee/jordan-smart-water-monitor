# Low-Cost Smart Water Conservation System for Rural Northern Jordan

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Target Cost](https://img.shields.io/badge/BOM--Cost-<27_JOD-green)
![Hardware](https://img.shields.io/badge/MCU-ESP32-red)

## Overview
Municipal water in northern Jordan is supplied intermittently (typically 1–2 times per week). Rural households rely on rooftop plastic tanks ($1\text{m}^3 - 2\text{m}^3$) and cisterns to store water. Undetected leaks, stuck float valves, and overflows lead to major water loss between supply cycles.

This project implements an ultra-low-cost (<27 JOD / ~$38 USD), outdoor-ruggedized IoT edge node designed for offline resilience, real-time leak/overflow detection, and behavioral anomaly modeling using machine learning.

## System Architecture
* **Dual-Sensor Array:** Non-contact waterproof ultrasonic depth tracking (`JSN-SR04T`) paired with inline pulse flow measurement (`YF-S201`).
* **Noise-Filtered Edge Firmware:** 10-sample median filtering running on the ESP32 to eliminate false level drops caused by wave sloshing during municipal refills.
* **Offline SPIFFS Storage:** Automatically buffers timestamped readings locally during network dropouts and syncs upon reconnection.
* **ML Anomaly Detection:** Python `Isolation Forest` pipeline trained to flag slow continuous leaks without generating false alarms on high-use days.

## Bill of Materials (BOM)

| Component | Model | Qty | Unit Cost (JOD) | Total Cost (JOD) |
| :--- | :--- | :--- | :--- | :--- |
| Microcontroller | ESP32-WROOM-32 | 1 | 4.50 | 4.50 |
| Waterproof Ultrasonic | JSN-SR04T V3.0 | 1 | 6.50 | 6.50 |
| Flow Sensor | YF-S201 (1/2") | 1 | 3.50 | 3.50 |
| Enclosure | IP65 Weatherproof Box | 1 | 3.00 | 3.00 |
| Battery Module | TP4056 + 18650 Li-ion | 1 | 2.50 | 2.50 |
| Power Supply | 5V 2A Wall Adapter | 1 | 2.50 | 2.50 |
| Cabling & Glands | PVC, Glands, Headers | - | 4.50 | 4.50 |
| **Total** | | | | **27.00 JOD** |

## Getting Started

### Firmware Compilation
1. Open VS Code with **PlatformIO**.
2. Open the `firmware/` directory.
3. Configure Wi-Fi details in `firmware/src/main.cpp`.
4. Click **Build & Upload**.

### Running Analytics Engine
```bash
pip install pandas numpy scikit-learn
python analytics/anomaly_detector.py

```

## License

Distributed under the MIT License. Author: Ahmad Arrabee.
