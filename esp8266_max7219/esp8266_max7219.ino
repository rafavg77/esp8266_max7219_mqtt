#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>

#define PRINT_CALLBACK  0
#define DEBUG 1
#define LED_HEARTBEAT 0

#if DEBUG
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }
#define PRINTS(s)   { Serial.print(F(s)); }
#else
#define PRINT(s, v)
#define PRINTS(s)
#endif

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define COL_SIZE 8

// GPIO pins for ESP8266
#define CLK_PIN   14  // D5 - SCK
#define DATA_PIN  13  // D7 - MOSI
#define CS_PIN    15  // D8 - SS

#define MODE_DURATION 10000  // Duration of each mode (10 seconds)
#define WEATHER_UPDATE_INTERVAL 300000  // Update weather every 5 minutes (300000ms)

// OpenWeatherMap API Configuration
const char* openWeatherMapApiKey = "";  // Replace with your OpenWeatherMap API key
const char* city = "";  // Replace with your city
const char* countryCode = "";  // Replace with your country code

// Operation modes
enum OperationMode {
  MODE_DATETIME,
  MODE_TEMPERATURE,
  MODE_COUNT
};

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// WiFi login parameters - network name and password
const char ssid[] = "";
const char password[] = "";

// NTP Objects
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -21600, 60000);  // GMT-6 for America/Monterrey

// Global message buffer
const uint8_t MESG_SIZE = 255;
const uint8_t CHAR_SPACING = 1;
const uint8_t SCROLL_DELAY = 75;
char curMessage[MESG_SIZE];

// Variables for mode control and temperature
OperationMode currentMode = MODE_DATETIME;
unsigned long lastModeChange = 0;
unsigned long lastTimeUpdate = 0;
unsigned long lastTempUpdate = 0;
float currentTemp = 0.0;
bool hasValidTemp = false;

// Global variables for message control
bool messageCompleted = true;
char currentDisplayMessage[MESG_SIZE];

// Callback function for data being scrolled off the display
void scrollDataSink(uint8_t dev, MD_MAX72XX::transformType_t t, uint8_t col) {
  // No implementation needed for this example
}

// Callback function for data required for scrolling into the display
uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t) {
  static enum { S_IDLE, S_NEXT_CHAR, S_SHOW_CHAR, S_SHOW_SPACE } state = S_IDLE;
  static char *p;
  static uint16_t curLen, showLen;
  static uint8_t  cBuf[8];
  static bool newMessageStarted = false;
  uint8_t colData = 0;

  // finite state machine to control what we do on the callback
  switch (state) {
    case S_IDLE: // reset the message pointer and check for new message to load
      PRINTS("\nS_IDLE");
      p = curMessage;      // reset the pointer to start of message
      state = S_NEXT_CHAR;
      newMessageStarted = true;
      messageCompleted = false;
      strcpy(currentDisplayMessage, curMessage);
      break;

    case S_NEXT_CHAR: // Load the next character from the font table
      PRINT("\nS_NEXT_CHAR ", *p);
      if (*p == '\0') {
        if (newMessageStarted) {
          newMessageStarted = false;
          messageCompleted = true;
        }
        state = S_IDLE;
      } else {
        showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state = S_SHOW_CHAR;
      }
      break;

    case S_SHOW_CHAR: // display the next part of the character
      PRINTS("\nS_SHOW_CHAR");
      colData = cBuf[curLen++];
      if (curLen < showLen)
        break;

      // set up the inter character spacing
      showLen = (*p != '\0' ? CHAR_SPACING : (MAX_DEVICES*COL_SIZE)/2);
      curLen = 0;
      state = S_SHOW_SPACE;
      // fall through

    case S_SHOW_SPACE:  // display inter-character spacing (blank column)
      PRINT("\nS_SHOW_SPACE: ", curLen);
      PRINT("/", showLen);
      curLen++;
      if (curLen == showLen)
        state = S_NEXT_CHAR;
      break;

    default:
      state = S_IDLE;
  }

  return(colData);
}

void scrollText(void) {
  static uint32_t	prevTime = 0;

  // Is it time to scroll the text?
  if (millis() - prevTime >= SCROLL_DELAY)
  {
    mx.transform(MD_MAX72XX::TSL);  // scroll along - the callback will load all the data
    prevTime = millis();            // starting point for next time
  }
}

void updateDateTime() {
  timeClient.update();
  
  if(WiFi.status() == WL_CONNECTED) {
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = localtime((time_t *)&epochTime);
    
    if (ptm != NULL) {
      sprintf(curMessage, "%02d/%02d/%d %02d:%02d",
              ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900,
              ptm->tm_hour, ptm->tm_min);
      PRINT("\nTime updated: ", curMessage);
    } else {
      strcpy(curMessage, "Error Time");
    }
  } else {
    strcpy(curMessage, "No WiFi");
  }
}

void updateTemperature() {
  unsigned long currentTime = millis();
  
  // Update temperature every 5 minutes
  if (!hasValidTemp || currentTime - lastTempUpdate >= WEATHER_UPDATE_INTERVAL) {
    WiFiClient client;
    HTTPClient http;
    
    // Create the API URL
    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += city;
    url += ",";
    url += countryCode;
    url += "&units=metric&appid=";
    url += openWeatherMapApiKey;
    
    http.begin(client, url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        currentTemp = doc["main"]["temp"].as<float>();
        hasValidTemp = true;
        lastTempUpdate = currentTime;
      }
    }
    
    http.end();
  }
  
  if (hasValidTemp) {
    sprintf(curMessage, "%s: %.1fC", city, currentTemp);
  } else {
    strcpy(curMessage, "Temp Error");
  }
}

void handleModeChange() {
  unsigned long currentTime = millis();
  static bool waitingForMessageComplete = false;
  
  // If we are waiting for the message to complete and it hasn't finished, do nothing
  if (waitingForMessageComplete && !messageCompleted) {
    return;
  }
  
  // If we were waiting and the message finished, change mode
  if (waitingForMessageComplete && messageCompleted) {
    waitingForMessageComplete = false;
    currentMode = static_cast<OperationMode>((currentMode + 1) % MODE_COUNT);
    lastModeChange = currentTime;
    
    // Update the message according to the new mode
    switch (currentMode) {
      case MODE_DATETIME:
        updateDateTime();
        break;
        
      case MODE_TEMPERATURE:
        updateTemperature();
        break;
    }
    return;
  }
  
  // Check if it's time to change mode
  if (currentTime - lastModeChange >= MODE_DURATION) {
    // If the current message hasn't finished, mark that we are waiting
    if (!messageCompleted) {
      waitingForMessageComplete = true;
      return;
    }
    
    // If the message finished, change mode immediately
    currentMode = static_cast<OperationMode>((currentMode + 1) % MODE_COUNT);
    lastModeChange = currentTime;
    
    // Update the message according to the new mode
    switch (currentMode) {
      case MODE_DATETIME:
        updateDateTime();
        break;
        
      case MODE_TEMPERATURE:
        updateTemperature();
        break;
    }
  }
  
  // Periodic updates according to the current mode
  switch (currentMode) {
    case MODE_DATETIME:
      if (currentTime - lastTimeUpdate >= 1000 && messageCompleted) {  // Update every second
        updateDateTime();
        lastTimeUpdate = currentTime;
      }
      break;
      
    case MODE_TEMPERATURE:
      if (messageCompleted) {
        updateTemperature();  // The function already handles the update interval
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  PRINTS("\n[MD_MAX72XX Dual-Mode Display]\n");

  // Display initialization
  mx.begin();
  mx.setShiftDataInCallback(scrollDataSource);
  mx.setShiftDataOutCallback(scrollDataSink);
  mx.control(MD_MAX72XX::INTENSITY, 5);
  mx.clear();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  strcpy(curMessage, "Connecting WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    PRINTS(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    PRINTS("\nWiFi connected");
    
    // Configure NTP
    timeClient.begin();
    timeClient.setUpdateInterval(60000); // Update every minute
    
    // Force first update
    int ntpAttempts = 0;
    while (!timeClient.update() && ntpAttempts < 5) {
      PRINTS("\nSynchronizing NTP...");
      timeClient.forceUpdate();
      delay(500);
      ntpAttempts++;
    }
    
    if (timeClient.isTimeSet()) {
      updateDateTime();
    } else {
      strcpy(curMessage, "NTP Sync Failed");
    }
  } else {
    strcpy(curMessage, "WiFi Failed");
  }

  // Get initial temperature
  if (WiFi.status() == WL_CONNECTED) {
    updateTemperature();
  }
}

void loop() {
  // Check WiFi connection and reconnect if necessary
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 30000) { // Check every 30 seconds
    if (WiFi.status() != WL_CONNECTED) {
      PRINTS("\nReconnecting WiFi...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      
      if (WiFi.status() == WL_CONNECTED) {
        timeClient.begin();
        timeClient.forceUpdate();
      }
    }
    lastWifiCheck = millis();
  }

  handleModeChange();  // Handle mode changes
  scrollText();        // Update LED matrix
}

