#pragma once

#include "PluginManager.h"
#include "timing.h"

struct CityInfo
{
  const char *name;
  const char *timezone;
  float latitude;
  float longitude;
};

class CityClockPlugin : public Plugin
{
private:
  // Cities configuration
  static constexpr CityInfo cities[] = {
      {"Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4", 60.17f, 24.94f},
      {"Omsk", "OMST-6", 54.99f, 73.37f},
      {"Berlin", "CET-1CEST,M3.5.0/2,M10.5.0/3", 52.52f, 13.41f},
      {"St.Petersburg", "MSK-3", 59.93f, 30.32f}};
  static constexpr int cityCount = 4;

  struct tm timeinfo;
  NonBlockingDelay secondTimer;

  int currentCityIndex = 0;
  char savedTz[100] = {0};

  int cachedTemperature = -99;
  int weatherIcon = -1;
  bool hasWeatherData = false;
  unsigned long lastWeatherUpdate = 0;

  int displayMode = 0;
  unsigned long modeStartTime = 0;
  bool colonVisible = true;
  int cityScrollX = -16;

  void loadConfig();
  void fetchWeather();
  void drawClock();
  void drawTemperature();
  int mapWmoCode(int code, bool isNight);
  void switchToCity(int index);
  String getCurrentCityName();

public:
  void setup() override;
  void loop() override;
  void teardown() override;
  void websocketHook(JsonDocument &request) override;
  const char *getName() const override;
};
