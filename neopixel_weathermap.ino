#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Timezone.h>

#define LED_PIN 18
#define LED_COUNT 64

const char* ssid = "";
const char* password = "";
const char* OPENWEATHERMAP_API_KEY = "a3cc6725e576479ce11822a5242604e9";
const char* OPENWEATHERMAP_LOCATION_ID = "2787663";

const int WEATHER_UPDATE_INTERVAL = 60000;

const char* ntpServer = "pool.ntp.org";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.telenet.be", 7200);

std::map<String, uint8_t> day_conditions = {
  {"Drizzle",        0x66aaff},   // Cornflower Blue
  {"Rain",           0x0066ff},   // Blue
  {"Snow",           0xffffff},   // White
  {"Mist",           0xcccccc},   // Light Gray
  {"Smoke",          0xbbbbbb},   // Silver
  {"Haze",           0xdddddd},   // Lighter Gray
  {"Fog",            0xaaaaaa},   // Medium Gray
  {"Sand",           0xffe4a7},   // Sandy Brown
  {"Dust",           0xcca27a},   // Burlywood
  {"Ash",            0x808080},   // Gray
  {"Squall",         0x2b6bff},   // Dodger Blue
  {"Tornado",        0x333333},   // Dark Gray
  {"Clear",          0xffcc33},   // Saffron
  {"Clouds",         0x6699cc},   // Steel Blue
  {"Thunderstorm",   0x800000}    // Maroon
};

std::map<String, uint32_t> night_conditions = {
  {"Drizzle",        0x3355aa},   // Dark Cornflower Blue
  {"Rain",           0x003399},   // Dark Blue
  {"Snow",           0xffffff},   // White
  {"Mist",           0x999999},   // Medium Gray
  {"Smoke",          0x999999},   // Medium Gray
  {"Haze",           0xaaaaaa},   // Light Gray
  {"Fog",            0x777777},   // Dark Gray
  {"Sand",           0xffd4a3},   // Light Sandy Brown
  {"Dust",           0xa6907a},   // Tan
  {"Ash",            0x808080},   // Gray
  {"Squall",         0x3366ff},   // Medium Dodger Blue
  {"Tornado",        0x333333},   // Dark Gray
  {"Clear",          0xff9933},   // Orange
  {"Clouds",         0x6688aa},   // Dark Steel Blue
  {"Thunderstorm",   0x800000}    // Maroon
};

Adafruit_NeoPixel matrix = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Initialize LED matrix
  matrix.begin();
  matrix.clear();
  matrix.show();

  // Fetch weather data initially
  updateWeather();
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateTime >= WEATHER_UPDATE_INTERVAL) {
    updateWeather();
    lastUpdateTime = currentTime;
  }
}

void updateWeather() {
  displayColor(0);

  // Make sure Wi-Fi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to Wi-Fi");
    return;
  }

  // Create an HTTPClient instance
  HTTPClient http;

  String requestUrl = "http://api.openweathermap.org/data/2.5/weather?id=" + String(OPENWEATHERMAP_LOCATION_ID) + "&appid=" + String(OPENWEATHERMAP_API_KEY);

  http.begin(requestUrl);
  int httpResponseCode = http.GET();
  if (httpResponseCode != HTTP_CODE_OK) {
    Serial.println("Failed to connect to OpenWeatherMap API");
    http.end();
    return;
  }

  String response = http.getString();
  http.end();

  DynamicJsonDocument json(1024);
  deserializeJson(json, response);

  String weatherCondition = json["weather"][0]["main"].as<String>();

  unsigned long sunrise = json["sys"]["sunrise"];
  unsigned long sunset = json["sys"]["sunset"];
  
  Serial.println(weatherCondition);
  if (is_night(sunrise, sunset)) {
    displayColor(night_conditions[weatherCondition]);
  } else {
    displayColor(day_conditions[weatherCondition]);
  }
  Serial.println("Weather condition: " + weatherCondition);
}

void displayColor(uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) {
    matrix.setPixelColor(i, color);
  }
  matrix.show();
}

bool is_night(unsigned long sunrise, unsigned long sunset) {
  TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
  TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};
  Timezone timezone(CEST, CET);
  time_t currentTime = timezone.toLocal(now());
  time_t localSunrise = timezone.toLocal(sunrise);
  time_t localSunset = timezone.toLocal(sunset);
  if (currentTime >= localSunset || currentTime < localSunrise) {
    Serial.println("is night");
    return true;

  } else {
    Serial.println("is day");
    return false;
  }
}
