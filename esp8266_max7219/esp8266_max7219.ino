#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "config.h"

// ============================================================================
// 🧠 Variables globales
// ============================================================================
enum OperationMode { MODE_DATETIME, MODE_TEMPERATURE, MODE_COUNT };

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", GMT_OFFSET_SECONDS, NTP_UPDATE_INTERVAL_MS);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Estado global
OperationMode currentMode = MODE_DATETIME;
unsigned long lastModeChange = 0;
unsigned long lastTimeUpdate = 0;
unsigned long lastTempUpdate = 0;
float currentTemp = 0.0;
bool hasValidTemp = false;
bool messageCompleted = true;
bool showingMQTT = false;

char curMessage[MESSAGE_BUFFER_SIZE];
String incomingMQTT = "";

// ============================================================================
// 💡 Funciones de display
// ============================================================================
uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t) {
  static enum { S_IDLE, S_NEXT_CHAR, S_SHOW_CHAR, S_SHOW_SPACE } state = S_IDLE;
  static char *p;
  static uint16_t curLen, showLen;
  static uint8_t cBuf[8];
  uint8_t colData = 0;

  switch (state) {
    case S_IDLE:
      p = curMessage;
      state = S_NEXT_CHAR;
      messageCompleted = false;
      break;
    case S_NEXT_CHAR:
      if (*p == '\0') {
        messageCompleted = true;
        state = S_IDLE;
      } else {
        showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state = S_SHOW_CHAR;
      }
      break;
    case S_SHOW_CHAR:
      colData = cBuf[curLen++];
      if (curLen < showLen) break;
      showLen = (*p != '\0' ? CHAR_SPACING : (MAX_DEVICES * 8) / 2);
      curLen = 0;
      state = S_SHOW_SPACE;
      break;
    case S_SHOW_SPACE:
      colData = 0;
      if (++curLen == showLen) state = S_NEXT_CHAR;
      break;
  }
  return colData;
}

void scrollText() {
  static uint32_t prevTime = 0;
  if (millis() - prevTime >= DISPLAY_SCROLL_SPEED) {
    mx.transform(MD_MAX72XX::TSL);
    prevTime = millis();
  }
}

// ============================================================================
// 🕒 Tiempo y clima
// ============================================================================
void updateDateTime() {
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = localtime(&epochTime);
  if (ptm != NULL)
    sprintf(curMessage, "%02d/%02d/%d %02d:%02d",
            ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900,
            ptm->tm_hour, ptm->tm_min);
  else
    strcpy(curMessage, "Error Time");
}

void updateTemperature() {
  unsigned long currentTime = millis();
  if (!hasValidTemp || currentTime - lastTempUpdate >= WEATHER_UPDATE_INTERVAL) {
    WiFiClient client;
    HTTPClient http;

    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += WEATHER_CITY;
    url += ",";
    url += WEATHER_COUNTRY_CODE;
    url += "&units=metric&appid=";
    url += WEATHER_API_KEY;

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      if (deserializeJson(doc, payload) == DeserializationError::Ok) {
        currentTemp = doc["main"]["temp"].as<float>();
        hasValidTemp = true;
        lastTempUpdate = currentTime;
      }
    }
    http.end();
  }

  if (hasValidTemp)
    sprintf(curMessage, "%s: %.1fC", WEATHER_CITY, currentTemp);
  else
    strcpy(curMessage, "Temp Error");
}

// ============================================================================
// 📡 MQTT
// ============================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  incomingMQTT = "";
  for (unsigned int i = 0; i < length; i++)
    incomingMQTT += (char)payload[i];

  Serial.printf("\n📩 MQTT (%s): %s\n", topic, incomingMQTT.c_str());
  showingMQTT = true;

  strncpy(curMessage, incomingMQTT.c_str(), MESSAGE_BUFFER_SIZE - 1);
  curMessage[MESSAGE_BUFFER_SIZE - 1] = '\0';
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.printf("Intentando conexión MQTT (%s)...", MQTT_CLIENT_ID);
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("✅ Conectado al broker");
      mqttClient.subscribe(MQTT_TOPIC);
    } else {
      Serial.print("❌ Error MQTT rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Reintentando en 5s...");
      delay(5000);
    }
  }
}

// ============================================================================
// 🔁 Cambio de modos
// ============================================================================
void handleModeChange() {
  static bool waitingForMessageComplete = false;
  unsigned long currentTime = millis();

  if (showingMQTT) {
    if (messageCompleted) {
      showingMQTT = false;
      lastModeChange = currentTime;
      currentMode = MODE_DATETIME;
      updateDateTime();
    }
    return;
  }

  if (waitingForMessageComplete && !messageCompleted) return;
  if (waitingForMessageComplete && messageCompleted) {
    waitingForMessageComplete = false;
    currentMode = static_cast<OperationMode>((currentMode + 1) % MODE_COUNT);
    lastModeChange = currentTime;
    switch (currentMode) {
      case MODE_DATETIME: updateDateTime(); break;
      case MODE_TEMPERATURE: updateTemperature(); break;
    }
    return;
  }

  if (currentTime - lastModeChange >= MODE_DURATION) {
    if (!messageCompleted) {
      waitingForMessageComplete = true;
      return;
    }
    currentMode = static_cast<OperationMode>((currentMode + 1) % MODE_COUNT);
    lastModeChange = currentTime;
    switch (currentMode) {
      case MODE_DATETIME: updateDateTime(); break;
      case MODE_TEMPERATURE: updateTemperature(); break;
    }
  }

  switch (currentMode) {
    case MODE_DATETIME:
      if (currentTime - lastTimeUpdate >= 1000 && messageCompleted) {
        updateDateTime();
        lastTimeUpdate = currentTime;
      }
      break;
    case MODE_TEMPERATURE:
      if (messageCompleted) updateTemperature();
      break;
  }
}

// ============================================================================
// 🚀 SETUP Y LOOP
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n[VertiDisplay — MQTT + Clock + Weather]");

  mx.begin();
  mx.setShiftDataInCallback(scrollDataSource);
  mx.control(MD_MAX72XX::INTENSITY, DISPLAY_INTENSITY);
  mx.clear();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ✅ WiFi conectado");

  timeClient.begin();
  timeClient.update();
  updateDateTime();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttReconnect();

  updateTemperature();
}

void loop() {
  if (!mqttClient.connected()) mqttReconnect();
  mqttClient.loop();

  handleModeChange();
  scrollText();
}
