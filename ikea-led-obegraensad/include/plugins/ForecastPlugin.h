#pragma once

#include "PluginManager.h"
#include "timing.h"

struct ForecastCity
{
  const char *name;
  float latitude;
  float longitude;
};

class ForecastPlugin : public Plugin
{
private:
  static constexpr ForecastCity cities[] = {
      {"Espoo", 60.21f, 24.66f},
      {"Helsinki", 60.17f, 24.94f},
      {"St.Petersburg", 59.93f, 30.32f},
      {"Berlin", 52.52f, 13.41f}};
  static constexpr int cityCount = 4;

  NonBlockingDelay displayTimer;

  int currentCityIndex = 0;
  int maxTemp = -99;
  int minTemp = -99;
  int weatherIcon = -1;
  bool hasData = false;
  unsigned long lastFetch = 0;

  int displayMode = 0;
  unsigned long modeStart = 0;
  int scrollX = -16;

  void loadConfig();
  void fetchForecast();
  void drawIcon();
  void drawTemps();
  void drawTempValue(int temp, int y);
  int mapWmoCode(int code);

public:
  void setup() override;
  void loop() override;
  void websocketHook(JsonDocument &request) override;
  const char *getName() const override;
};
