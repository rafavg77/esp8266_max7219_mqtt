# ESP8266 with LED Matrix MAX7219 - Time and Temperature Display

This project implements an information display using an ESP8266 and a MAX7219 LED matrix, alternately showing the current time and city temperature.

## Features

- Displays current date and time (synchronized with NTP)
- Displays current city temperature (using OpenWeatherMap API)
- Automatically alternates between modes every 10 seconds
- Automatic WiFi reconnection
- Waits for messages to finish scrolling before changing modes
- Automatic local timezone adjustment (configured for America/Monterrey GMT-6)

## Hardware Requirements

- ESP8266 (NodeMCU or similar)
- LED Matrix MAX7219 (4 cascaded modules)
- Connection cables

## Connections

### MAX7219 Matrix to ESP8266
| MAX7219 | ESP8266 |
|---------|---------|
| VCC     | 5V      |
| GND     | GND     |
| DIN     | GPIO13 (D7) |
| CS      | GPIO15 (D8) |
| CLK     | GPIO14 (D5) |

## Software Requirements

### Required Arduino Libraries
- ESP8266WiFi
- MD_MAX72xx
- NTPClient
- WiFiUdp
- ESP8266HTTPClient
- ArduinoJson

### APIs and Services
- OpenWeatherMap API (requires a free API key)
- NTP Server (pool.ntp.org)

## Configuration

### Setting up config.h

The project uses a `config.h` file to store sensitive information and configuration settings. This file is not included in the repository for security reasons. Follow these steps to set it up:

1. Create a new file named `config.h` in the `esp8266_max7219` directory
2. Copy and paste the following template:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
const char* WIFI_SSID = "YOUR_SSID";          // Your WiFi network name
const char* WIFI_PASSWORD = "YOUR_PASSWORD";   // Your WiFi password

// OpenWeatherMap Configuration
const char* WEATHER_API_KEY = "YOUR_API_KEY";  // Your OpenWeatherMap API key
const char* WEATHER_CITY = "YOUR_CITY";        // Your city name (e.g., "London")
const char* WEATHER_COUNTRY_CODE = "YOUR_COUNTRY_CODE";  // Your country code (e.g., "UK")

// Time Configuration
const long GMT_OFFSET_SECONDS = -21600;        // Your timezone offset in seconds (-21600 = GMT-6)
const int NTP_UPDATE_INTERVAL_MS = 60000;      // NTP update interval (60000 = 1 minute)

#endif
```

3. Replace the placeholder values with your actual configuration:
   - Replace `YOUR_SSID` with your WiFi network name
   - Replace `YOUR_PASSWORD` with your WiFi password
   - Get an API key from [OpenWeatherMap](https://openweathermap.org/api) and replace `YOUR_API_KEY`
   - Set your city name and country code
   - Adjust the timezone offset if needed:
     * GMT-6 (America/Monterrey) = -21600
     * GMT-5 = -18000
     * GMT-4 = -14400
     * GMT-3 = -10800
     * GMT+0 = 0
     * GMT+1 = 3600
     * GMT+2 = 7200

> ⚠️ Important: The `config.h` file contains sensitive information and should never be committed to version control.

1. Install all required libraries in Arduino IDE
2. Configure WiFi credentials:
   ```cpp
   const char ssid[] = "YOUR_SSID";
   const char password[] = "YOUR_PASSWORD";
   ```

3. Configure OpenWeatherMap:
   ```cpp
   const char* openWeatherMapApiKey = "YOUR_API_KEY";
   const char* city = "YOUR_CITY";
   const char* countryCode = "YOUR_COUNTRY_CODE";
   ```

4. Adjust timezone if needed:
   ```cpp
   NTPClient timeClient(ntpUDP, "pool.ntp.org", -21600, 60000);  // -21600 = GMT-6
   ```

## Operation

The device operates in two modes that alternate every 10 seconds:

1. **Date/Time Mode**
   - Shows current date and time
   - Updates every second
   - Format: DD/MM/YYYY HH:MM

2. **Temperature Mode**
   - Shows current city temperature
   - Updates every 5 minutes
   - Format: CITY: XX.XC

## Technical Specifications

- Text scrolling speed: 75ms per column
- Matrix brightness: 5/15 (adjustable)
- Temperature update interval: 5 minutes
- WiFi check interval: 30 seconds
- WiFi reconnection timeout: 10 seconds

## Troubleshooting

### Error Messages

- "No WiFi": Device lost WiFi connection
- "Error Time": Problem with NTP synchronization
- "Temp Error": Failed to get temperature
- "NTP Sync Failed": Initial NTP synchronization failed
- "WiFi Failed": Could not connect to WiFi during startup

### Common Issues

1. **Not showing correct time**
   - Check WiFi connection
   - Verify timezone offset is correct
   - Ensure NTP port (123) is not blocked

2. **Not showing temperature**
   - Verify OpenWeatherMap API key
   - Check WiFi connection
   - Confirm city name and country code are correct

## Customization

- `MODE_DURATION`: Adjust mode duration (default: 10000ms)
- `SCROLL_DELAY`: Adjust scrolling speed (default: 75ms)
- `MAX_DEVICES`: Adjust number of cascaded MAX7219 modules
- LED Intensity: Modify value in `mx.control(MD_MAX72XX::INTENSITY, 5)`

## Development Notes

- Code implements a state machine for text scrolling
- Includes automatic WiFi reconnection handling
- Implements a waiting system for complete messages before mode changes
- Uses callbacks to optimize text scrolling

## Contributing

To contribute to the project:
1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is available under the MIT License.

## Author

rafavg77

## Acknowledgments

- MD_MAX72XX library by MajicDesigns
- OpenWeatherMap for providing the weather API
- Arduino/ESP8266 Community