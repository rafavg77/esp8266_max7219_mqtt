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

// ============================================================================
// 🛜 WiFi Configuration
// ============================================================================
const char* WIFI_SSID = "YOUR_SSID";          
const char* WIFI_PASSWORD = "YOUR_PASSWORD";   

// ============================================================================
// 🌦️ OpenWeatherMap Configuration
// ============================================================================
const char* WEATHER_API_KEY = "YOUR_API_KEY";  
const char* WEATHER_CITY = "YOUR_CITY";        
const char* WEATHER_COUNTRY_CODE = "YOUR_COUNTRY_CODE";  

// ============================================================================
// ⏰ Time Configuration
// ============================================================================
const long GMT_OFFSET_SECONDS = -21600;        // Ejemplo: -21600 = GMT-6
const int NTP_UPDATE_INTERVAL_MS = 60000;      // Intervalo de actualización NTP (ms)

// ============================================================================
// 📡 MQTT Configuration
// ============================================================================
const char* MQTT_SERVER   = "192.168.1.100";   // Dirección del broker
const int   MQTT_PORT     = 1883;              // Puerto MQTT (default: 1883)
const char* MQTT_USER     = "mqtt_user";       // Usuario del broker
const char* MQTT_PASS     = "mqtt_password";   // Contraseña del broker
const char* MQTT_TOPIC    = "esp8266/display"; // Tópico suscrito
const char* MQTT_CLIENT_ID = "VertiDisplay01"; // Identificador del cliente MQTT

// ============================================================================
// 💡 Display Configuration
// ============================================================================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW      // Tipo de módulo MAX7219
#define MAX_DEVICES 4                          // Número de módulos en cadena
#define CLK_PIN   14                           // D5
#define DATA_PIN  13                           // D7
#define CS_PIN    15                           // D8

#define DISPLAY_INTENSITY 5                    // Brillo del display (0-15)
#define DISPLAY_SCROLL_SPEED 75                // Velocidad de scroll (ms entre pasos)
#define CHAR_SPACING 1                         // Espacio entre caracteres
#define MESSAGE_BUFFER_SIZE 255                // Tamaño máximo del mensaje

// ============================================================================
// ⚙️ Operación general
// ============================================================================
#define MODE_DURATION 10000                    // Duración de cada modo (ms)
#define WEATHER_UPDATE_INTERVAL 300000         // Actualizar clima cada 5 minutos

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
