#pragma once

#include "PluginManager.h"
#include "timing.h"

class EspooClockPlugin : public Plugin
{
private:
  struct tm timeinfo;
  NonBlockingDelay secondTimer;

  int cachedTemperature = -99;
  int weatherIcon = -1; // -1 = no data, 0-7 = weatherIcons index
  bool hasWeatherData = false;
  unsigned long lastWeatherUpdate = 0;
  int lastHttpError = 0; // 0=not tried, 200=ok, negative=HTTP error

  // Display modes: 0=clock, 1=scrolling city name, 2=temperature+icon
  int displayMode = 0;
  unsigned long modeStartTime = 0;

  bool colonVisible = true;
  int cityScrollX = -16;

  void fetchWeather();
  void drawClock();
  void drawTemperature();
  int mapWmoCode(int code, bool isNight);

public:
  void setup() override;
  void loop() override;
  void teardown() override;
  const char *getName() const override;
};
