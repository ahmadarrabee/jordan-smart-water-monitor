# Installation and calibration notes

This repository does not yet include a validated wiring schematic or enclosure design.

- Mount the ultrasonic head above the full water surface, outside its specified blind zone. Measure that offset and the usable water depth and enter them in firmware. Validate readings at known water levels before relying on alarms.
- Confirm the exact sensor variant's electrical specifications and use appropriate level shifting for the ESP32 GPIO. Do not assume a 5 V sensor output can connect directly.
- Fit the flow sensor on the outlet in its marked flow direction and follow its manufacturer's installation requirements. Calibrate pulse count against a measured volume. Outlet flow does not measure municipal inflow.
- Keep the sensor face clear of obstructions. A stilling pipe must be validated for the particular sensor; a narrow pipe can introduce echoes.
- Select an enclosure and cable glands appropriate for outdoor use, with downward cable entries and strain relief. Installation must preserve their ingress protection ratings.
- Validate power regulation, battery protection, and charging separately. The proposal's charger/battery line item is not a complete backup supply circuit.
