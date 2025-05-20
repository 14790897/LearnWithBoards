# ESP-01S WiFi Relay Control with OTA Updates

This project turns an ESP-01S module into a WiFi-controlled relay switch with Over-The-Air (OTA) update capability.

## Features

- WiFi connectivity
- Web interface for controlling the relay
- OTA (Over-The-Air) firmware updates
- Simple and responsive web UI

## Hardware Requirements

- ESP-01S module
- ESP-01S relay module (or custom circuit with relay)
- USB to TTL adapter for initial programming

## Wiring

For the ESP-01S relay module:
- VCC to 3.3V
- GND to GND
- GPIO2 connected to relay control (this is configurable in the code)

## Initial Setup

1. Open `src/main.cpp` and update the WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

2. Update the OTA password if desired:
   ```cpp
   const char* otaPassword = "esp01s-relay";
   ```

3. Verify the relay pin configuration:
   ```cpp
   const int RELAY_PIN = 2;  // GPIO2
   ```

4. Upload the firmware to your ESP-01S using a USB to TTL adapter.

## Using OTA Updates

After the initial upload, you can update the firmware wirelessly:

1. Make sure your computer is on the same network as the ESP-01S
2. Note the IP address shown in the serial monitor or on the web interface
3. Uncomment and update the OTA settings in `platformio.ini`:
   ```ini
   upload_protocol = espota
   upload_port = 192.168.1.x  ; Replace with your ESP's IP address
   upload_flags =
       --auth=your_ota_password
   ```
4. Upload as normal through PlatformIO

## Web Interface

The web interface is accessible by navigating to the ESP's IP address in a web browser. It provides:

- Current relay status
- Button to toggle the relay on/off
- Device information

## API Endpoints

- `/` - Web interface
- `/relay?state=1` - Turn relay ON
- `/relay?state=0` - Turn relay OFF
- `/relay?state=toggle` - Toggle relay state

## Troubleshooting

- If the relay operates in reverse (ON when it should be OFF), modify the logic in the `setRelay()` function.
- For ESP-01S modules with limited flash, you may need to reduce the size of the HTML interface.
- If you're having trouble with OTA updates, ensure you're on the same network and the firewall isn't blocking the connection.

## License

This project is open source and available under the MIT License.
