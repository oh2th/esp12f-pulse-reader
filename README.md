# ESP12-F Pulse Reader

This project reads pulses from a water meter (or similar sensor) on GPIO pin D6 of an ESP12-F (ESP8266) and publishes water usage data via MQTT. It features a WiFi setup portal for easy configuration of WiFi and MQTT settings, as well as the initial meter value.

## Features

- **Pulse Counting:** Reads pulses on D6 (to GND), each pulse = 1 liter.
- **Water Measurement:**
  - `measure_water`: Liters per minute (integer, resets every minute).
  - `meter_water`: Total water consumed in m³ (float, 3 decimals, persistent in EEPROM).
- **MQTT Integration:**
  - Publishes `measure_water` and `meter_water` as JSON to `<base_topic>/state`.
  - Allows remote setting of `meter_water` via `<base_topic>/set`.
  - **Configurable MQTT Base Topic:** You can set the MQTT base topic (e.g., `homey/water-meter`) in the WiFiManager portal. All MQTT topics will be published under this base.
- **WiFiManager Portal:**
  - On first boot or WiFi failure, creates an AP (`ESP12F-Setup`) for configuration.
  - Portal allows setting WiFi, MQTT broker/port, MQTT base topic, and initial meter value.

## Project Structure

```text
esp12f-pulse-reader
├── src
│   └── main.cpp          # Main source code for pulse reading and MQTT
├── include
│   └── README.md         # Documentation for additional header files
├── platformio.ini        # PlatformIO configuration file
└── README.md             # Project documentation
```

## Setup Instructions

1. **Clone the Repository**:
   ```
   git clone https://github.com/yourusername/esp12f-pulse-reader.git
   ```

2. **Install PlatformIO**:
   Make sure PlatformIO is installed in your IDE (e.g., VS Code).

3. **Open the Project**:
   Open the folder in PlatformIO.

4. **Build and Upload**:
   Connect your ESP12-F and use PlatformIO to upload the code.

5. **Configure via Portal**:
   On first boot, connect to the `ESP12F-Setup` WiFi AP. Use the captive portal to enter your WiFi credentials, MQTT broker/port, MQTT base topic (e.g., `homey/water-meter`), and the initial meter value (from your physical water meter).

## Usage

- The device will connect to your WiFi and MQTT broker.
- Every minute, it publishes:
  ```json
  {"measure_water": 12, "meter_water": 1.234}
  ```
  to the `<base_topic>/state` MQTT topic (e.g., `homey/water-meter/state`).
- To set the meter value remotely, publish a float (e.g., `1.234`) to the `<base_topic>/set` topic (e.g., `homey/water-meter/set`).

## License

This project is licensed under the MIT License - see the LICENSE file for details.
