#include "plugins/ForecastPlugin.h"
#include "config.h"
#include <ArduinoJson.h>

#ifdef ESP32
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#endif
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#endif

void ForecastPlugin::loadConfig()
{
#ifdef ENABLE_STORAGE
  Preferences prefs;
  prefs.begin("forecast", true);
  currentCityIndex = prefs.getInt("cityIdx", 0);
  prefs.end();
  if (currentCityIndex < 0 || currentCityIndex >= cityCount)
    currentCityIndex = 0;
#else
  currentCityIndex = 0;
#endif
}

int ForecastPlugin::mapWmoCode(int code)
{
  // weatherIcons[]: 0=cloudy, 1=thunderstorm, 2=clear/sun, 3=partly cloudy,
  //                 4=rain, 5=snow, 6=fog
  switch (code)
  {
  case 0:
  case 1:
    return 2; // Clear
  case 2:
    return 3; // Partly cloudy
  case 3:
    return 0; // Overcast
  case 45:
  case 48:
    return 6; // Fog
  case 51: case 53: case 55:
  case 56: case 57:
  case 61: case 63: case 65:
  case 66: case 67:
  case 80: case 81: case 82:
    return 4; // Rain
  case 71: case 73: case 75: case 77:
  case 85: case 86:
    return 5; // Snow
  case 95: case 96: case 99:
    return 1; // Thunderstorm
  default:
    return 2;
  }
}

void ForecastPlugin::fetchForecast()
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  if (currentCityIndex < 0 || currentCityIndex >= cityCount)
    return;

#ifdef ESP32
  Serial.printf("[Forecast] Heap free=%u maxAlloc=%u\n",
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
#endif

  char url[256];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f"
           "&daily=temperature_2m_max,temperature_2m_min,weather_code"
           "&timezone=auto&forecast_days=2",
           cities[currentCityIndex].latitude, cities[currentCityIndex].longitude);
  Serial.printf("[Forecast] GET %s\n", url);

#ifdef ESP32
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, url);
#endif
#ifdef ESP8266
  WiFiClient wifiClient;
  HTTPClient http;
  http.begin(wifiClient, url);
#endif

  http.setConnectTimeout(5000);
  http.setTimeout(10000);
  int code = http.GET();

  if (code == HTTP_CODE_OK)
  {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err)
    {
      // Index 1 = tomorrow
      float maxT = doc["daily"]["temperature_2m_max"][1] | -99.0f;
      float minT = doc["daily"]["temperature_2m_min"][1] | -99.0f;
      int wmo = doc["daily"]["weather_code"][1] | 0;

      maxTemp = (int)roundf(maxT);
      minTemp = (int)roundf(minT);
      weatherIcon = mapWmoCode(wmo);
      hasData = true;

      Serial.printf("[Forecast] Tomorrow: %d/%d C wmo=%d icon=%d\n",
                    maxTemp, minTemp, wmo, weatherIcon);
    }
    else
    {
      Serial.printf("[Forecast] JSON error: %s\n", err.c_str());
    }
  }
  else
  {
    Serial.printf("[Forecast] HTTP error: %d\n", code);
  }

  http.end();

  if (code == HTTP_CODE_OK)
  {
    lastFetch = millis();
  }
  else
  {
    // Retry in 60 seconds on failure
    lastFetch = millis() - 1740000UL;
  }
}

void ForecastPlugin::setup()
{
  Screen.clear();
  displayMode = 0;
  modeStart = millis();
  scrollX = -16;
  displayTimer.forceReady();
  loadConfig();

  if (!hasData)
  {
    // Loading dots
    Screen.setPixel(4, 7, 1);
    Screen.setPixel(7, 7, 1);
    Screen.setPixel(10, 7, 1);
  }
}

void ForecastPlugin::drawIcon()
{
  Screen.clear();
  if (!hasData || weatherIcon < 0)
  {
    Screen.setPixel(5, 7, 1);
    Screen.setPixel(7, 7, 1);
    Screen.setPixel(9, 7, 1);
    return;
  }
  // Center weather icon (icons are 16px wide, x=0 fills screen)
  Screen.drawWeather(0, 5, weatherIcon);
}

void ForecastPlugin::drawTempValue(int temp, int y)
{
  // Arrow occupies x=0-2; sign at x=4-6; digit(s) after sign; degree 2x2
  if (temp >= 10)
  {
    // Two digits, no sign needed (obviously positive)
    Screen.drawNumbers(4, y, {temp / 10, temp % 10});
    // Degree symbol 2x2
    Screen.setPixel(14, y, 1, 120);
    Screen.setPixel(15, y, 1, 120);
    Screen.setPixel(14, y + 1, 1, 120);
    Screen.setPixel(15, y + 1, 1, 120);
  }
  else if (temp >= 0)
  {
    // Plus sign (3x3 cross centered at x=5)
    Screen.setPixel(5, y + 1, 1, 150);
    Screen.setPixel(4, y + 2, 1, 150);
    Screen.setPixel(5, y + 2, 1, 150);
    Screen.setPixel(6, y + 2, 1, 150);
    Screen.setPixel(5, y + 3, 1, 150);
    Screen.drawNumbers(8, y, {temp});
    // Degree symbol 2x2
    Screen.setPixel(13, y, 1, 120);
    Screen.setPixel(14, y, 1, 120);
    Screen.setPixel(13, y + 1, 1, 120);
    Screen.setPixel(14, y + 1, 1, 120);
  }
  else if (temp > -10)
  {
    // Minus sign (3px wide centered at x=5)
    Screen.setPixel(4, y + 2, 1, 150);
    Screen.setPixel(5, y + 2, 1, 150);
    Screen.setPixel(6, y + 2, 1, 150);
    Screen.drawNumbers(8, y, {-temp});
    // Degree symbol 2x2
    Screen.setPixel(13, y, 1, 120);
    Screen.setPixel(14, y, 1, 120);
    Screen.setPixel(13, y + 1, 1, 120);
    Screen.setPixel(14, y + 1, 1, 120);
  }
  else
  {
    // "-15" — minus + two digits (tight fit, shifted right by 1)
    int t = -temp;
    Screen.setPixel(4, y + 2, 1, 150);
    Screen.setPixel(5, y + 2, 1, 150);
    Screen.drawNumbers(7, y, {t / 10, t % 10});
  }
}

void ForecastPlugin::drawTemps()
{
  Screen.clear();
  if (!hasData)
  {
    Screen.setPixel(5, 7, 1);
    Screen.setPixel(7, 7, 1);
    Screen.setPixel(9, 7, 1);
    return;
  }

  // Top half (rows 0-6): up arrow + max temp
  // Up arrow at x=0: tip(y=0), head(y=1), shaft(y=2,y=3)
  Screen.setPixel(1, 0, 1, 200);
  Screen.setPixel(0, 1, 1, 200);
  Screen.setPixel(1, 1, 1, 200);
  Screen.setPixel(2, 1, 1, 200);
  Screen.setPixel(1, 2, 1, 200);
  Screen.setPixel(1, 3, 1, 200);
  drawTempValue(maxTemp, 0);

  // Dotted separator at row 7
  Screen.setPixel(1, 7, 1, 40);
  Screen.setPixel(5, 7, 1, 40);
  Screen.setPixel(9, 7, 1, 40);
  Screen.setPixel(13, 7, 1, 40);

  // Bottom half (rows 8-14): down arrow + min temp
  // Down arrow at x=0: shaft(y=8,y=9), head(y=10), tip(y=11)
  Screen.setPixel(1, 8, 1, 200);
  Screen.setPixel(1, 9, 1, 200);
  Screen.setPixel(0, 10, 1, 200);
  Screen.setPixel(1, 10, 1, 200);
  Screen.setPixel(2, 10, 1, 200);
  Screen.setPixel(1, 11, 1, 200);
  drawTempValue(minTemp, 8);
}

void ForecastPlugin::loop()
{
  // Fetch forecast every 30 minutes
  if (lastFetch == 0 || millis() - lastFetch > 1800000UL)
  {
    fetchForecast();
  }

  unsigned long elapsed = millis() - modeStart;

  switch (displayMode)
  {
  case 0: // Scroll city name
  {
    if (scrollX == -16)
      displayTimer.forceReady();

    if (displayTimer.isReady(40))
    {
      Screen.clear();
      String name = String(cities[currentCityIndex].name);
      name.toUpperCase();
      int fw = fonts[0].sizeX + 1;
      int textWidth = name.length() * fw;

      for (unsigned int c = 0; c < name.length(); c++)
      {
        int xPos = c * fw - scrollX;
        if (xPos > -8 && xPos < 16)
        {
          int fontIdx = name.charAt(c) - fonts[0].offset;
          if (fontIdx >= 0 && fontIdx < (int)fonts[0].data.size())
          {
            Screen.drawCharacter(xPos, 4,
                                 Screen.readBytes(fonts[0].data[fontIdx]), 8, 180);
          }
        }
      }
      scrollX++;
      if (scrollX >= textWidth)
      {
        scrollX = -16;
        displayMode = 1;
        modeStart = millis();
        displayTimer.forceReady();
      }
    }
    break;
  }

  case 1: // Weather icon (5s)
    if (displayTimer.isReady(1000))
    {
      drawIcon();
    }
    if (elapsed > 5000)
    {
      displayMode = 2;
      modeStart = millis();
      displayTimer.forceReady();
    }
    break;

  case 2: // Temperatures max/min (5s)
    if (displayTimer.isReady(1000))
    {
      drawTemps();
    }
    if (elapsed > 5000)
    {
      displayMode = 0;
      modeStart = millis();
      scrollX = -16;
      displayTimer.forceReady();
    }
    break;
  }
}

void ForecastPlugin::websocketHook(JsonDocument &request)
{
  if (request["event"] == "forecast")
  {
    if (request["cityIndex"].is<int>())
    {
      int idx = request["cityIndex"].as<int>();
      if (idx >= 0 && idx < cityCount)
      {
        currentCityIndex = idx;
#ifdef ENABLE_STORAGE
        Preferences prefs;
        prefs.begin("forecast", false);
        prefs.putInt("cityIdx", idx);
        prefs.end();
#endif
        // Reset weather data to force re-fetch for new city
        hasData = false;
        lastFetch = 0;
        weatherIcon = -1;
        maxTemp = -99;
        minTemp = -99;
        displayMode = 0;
        modeStart = millis();
        scrollX = -16;
        Serial.printf("[Forecast] City changed to %d (%s)\n", idx, cities[idx].name);
      }
    }
  }
}

const char *ForecastPlugin::getName() const
{
  return "Forecast";
}
