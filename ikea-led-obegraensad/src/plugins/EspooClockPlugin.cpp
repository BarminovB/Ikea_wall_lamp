#include "plugins/EspooClockPlugin.h"
#include "config.h"
#include <ArduinoJson.h>

#ifdef ESP32
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#endif

// WMO weather codes → our weatherIcons[] index
// 0=cloudy 1=thunderstorm 2=sun 3=partly cloudy 4=rain 5=snow 6=fog 7=moon
int EspooClockPlugin::mapWmoCode(int code, bool isNight)
{
  switch (code)
  {
  case 0:
  case 1:
    return isNight ? 7 : 2; // clear / mainly clear
  case 2:
    return 3; // partly cloudy
  case 3:
    return 0; // overcast
  case 45:
  case 48:
    return 6; // fog
  case 51: case 53: case 55: // drizzle
  case 56: case 57:          // freezing drizzle
  case 61: case 63: case 65: // rain
  case 66: case 67:          // freezing rain
  case 80: case 81: case 82: // rain showers
    return 4;
  case 71: case 73: case 75: case 77: // snow
  case 85: case 86:                    // snow showers
    return 5;
  case 95: case 96: case 99: // thunderstorm
    return 1;
  default:
    return isNight ? 7 : 2;
  }
}

void EspooClockPlugin::setup()
{
  Screen.clear();
  displayMode = 0;
  modeStartTime = millis();
  colonVisible = true;
  secondTimer.forceReady();

  // Do NOT reset weather cache here!
  // Plugin objects persist across activations (created once via new).
  // Resetting lastWeatherUpdate=0 forces an immediate HTTPS fetch (~40KB SSL alloc).
  // With scheduler cycling through 20+ plugins, repeated alloc/free fragments heap → crash.
  // Weather refreshes naturally every 10 minutes via the timer in loop().

  if (!hasWeatherData)
  {
    // Show loading dots only on first activation (before any weather data)
    Screen.setPixel(4, 7, 1);
    Screen.setPixel(5, 7, 1);
    Screen.setPixel(7, 7, 1);
    Screen.setPixel(8, 7, 1);
    Screen.setPixel(10, 7, 1);
    Screen.setPixel(11, 7, 1);
  }
}

void EspooClockPlugin::fetchWeather()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf("[EspooClock] WiFi not connected (status=%d)\n", WiFi.status());
    lastHttpError = -100;
    return;
  }

#ifdef ESP32
  Serial.printf("[EspooClock] Heap free=%u maxAlloc=%u RSSI=%d\n",
                ESP.getFreeHeap(), ESP.getMaxAllocHeap(), WiFi.RSSI());
#endif

  // Open-Meteo API: Espoo coordinates (60.20°N, 24.66°E)
  const char *url = "https://api.open-meteo.com/v1/forecast?latitude=60.20&longitude=24.66&current=temperature_2m,weather_code,is_day";
  Serial.printf("[EspooClock] GET %s\n", url);

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
  lastHttpError = code;

  if (code == HTTP_CODE_OK)
  {
    String payload = http.getString();
    Serial.printf("[EspooClock] Payload: %s\n", payload.c_str());

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err)
    {
      float temp = doc["current"]["temperature_2m"] | -99.0f;
      int wmoCode = doc["current"]["weather_code"] | 0;
      int isDay = doc["current"]["is_day"] | 1;

      cachedTemperature = (int)roundf(temp);
      hasWeatherData = true;
      weatherIcon = mapWmoCode(wmoCode, isDay == 0);

      Serial.printf("[EspooClock] Weather OK: %d°C wmo=%d icon=%d\n",
                    cachedTemperature, wmoCode, weatherIcon);
    }
    else
    {
      Serial.printf("[EspooClock] JSON parse error: %s\n", err.c_str());
    }
  }
  else
  {
    Serial.printf("[EspooClock] HTTP FAILED: %d (%s)\n", code, http.errorToString(code).c_str());
  }

  http.end();
  if (code == HTTP_CODE_OK)
  {
    lastWeatherUpdate = millis();
  }
  else
  {
    lastWeatherUpdate = millis() - 540000UL;
  }
}

void EspooClockPlugin::drawClock()
{
  if (!getLocalTime(&timeinfo, 10) || timeinfo.tm_year < (2020 - 1900))
  {
    Screen.drawLine(0, 0, 15, 15, 1, 128);
    Screen.drawLine(15, 0, 0, 15, 1, 128);
    return;
  }

  Screen.clear();

  // Hours: big at top (row 0-5)
  std::vector<int> hh = {
      (timeinfo.tm_hour - timeinfo.tm_hour % 10) / 10,
      timeinfo.tm_hour % 10};
  Screen.drawNumbers(3, 0, hh);

  // Colon between hours and minutes - blink every second
  if (colonVisible)
  {
    Screen.setPixel(7, 6, 1, 180);
    Screen.setPixel(8, 6, 1, 180);
  }

  // Minutes (row 7-12)
  std::vector<int> mm = {
      (timeinfo.tm_min - timeinfo.tm_min % 10) / 10,
      timeinfo.tm_min % 10};
  Screen.drawNumbers(3, 7, mm);
}

void EspooClockPlugin::drawTemperature()
{
  Screen.clear();

  if (!hasWeatherData)
  {
    if (lastHttpError == 0)
    {
      // Initial state - not yet attempted
      Screen.setPixel(5, 7, 1);
      Screen.setPixel(7, 7, 1);
      Screen.setPixel(9, 7, 1);
    }
    else
    {
      // Show HTTP error code on display so user can report it
      // -1=CONN_REFUSED -4=NOT_CONNECTED -8=LOW_RAM -11=TIMEOUT
      // -100=NO_WIFI -101=BAD_LOCATION
      int err = lastHttpError < 0 ? -lastHttpError : lastHttpError;
      if (err >= 100)
      {
        Screen.drawNumbers(1, 5, {err / 100, (err / 10) % 10, err % 10});
      }
      else if (err >= 10)
      {
        Screen.drawNumbers(3, 5, {err / 10, err % 10});
      }
      else
      {
        Screen.drawNumbers(6, 5, {err});
      }
    }
    return;
  }

  int temp = cachedTemperature;

  // Draw temperature at top (y=0, rows 0-5)
  // Use manual 2x2 degree symbol (setPixel) to avoid drawCharacter zero-overwrite bug
  if (temp >= 10)
  {
    // "15°" - two digits + degree
    Screen.drawNumbers(2, 0, {temp / 10, temp % 10});
    Screen.setPixel(13, 0, 1, 120);
    Screen.setPixel(14, 0, 1, 120);
    Screen.setPixel(13, 1, 1, 120);
    Screen.setPixel(14, 1, 1, 120);
  }
  else if (temp <= -10)
  {
    // "-15°" - 2px minus + two digits + degree (centered)
    int t = -temp;
    Screen.setPixel(0, 2, 1, 120);
    Screen.setPixel(1, 2, 1, 120);
    Screen.drawNumbers(3, 0, {t / 10, t % 10});
    Screen.setPixel(13, 0, 1, 120);
    Screen.setPixel(14, 0, 1, 120);
    Screen.setPixel(13, 1, 1, 120);
    Screen.setPixel(14, 1, 1, 120);
  }
  else if (temp >= 0)
  {
    // "5°" - single digit + degree
    Screen.drawNumbers(4, 0, {temp});
    Screen.setPixel(9, 0, 1, 120);
    Screen.setPixel(10, 0, 1, 120);
    Screen.setPixel(9, 1, 1, 120);
    Screen.setPixel(10, 1, 1, 120);
  }
  else
  {
    // "-5°" - 2px minus + single digit + degree (centered)
    Screen.setPixel(3, 2, 1, 120);
    Screen.setPixel(4, 2, 1, 120);
    Screen.drawNumbers(6, 0, {-temp});
    Screen.setPixel(11, 0, 1, 120);
    Screen.setPixel(12, 0, 1, 120);
    Screen.setPixel(11, 1, 1, 120);
    Screen.setPixel(12, 1, 1, 120);
  }

  // Draw weather condition icon below temperature
  if (weatherIcon >= 0 && weatherIcon < (int)weatherIcons.size())
  {
    Screen.drawWeather(0, 7, weatherIcon);
  }
}

void EspooClockPlugin::loop()
{
  if (lastWeatherUpdate == 0 || millis() - lastWeatherUpdate > 600000UL)
  {
    fetchWeather();
  }

  unsigned long elapsed = millis() - modeStartTime;

  switch (displayMode)
  {
  case 0: // Clock mode
    if (secondTimer.isReady(500))
    {
      colonVisible = !colonVisible;
      drawClock();
    }
    if (elapsed > 20000)
    {
      displayMode = 1;
      cityScrollX = -16;
      modeStartTime = millis();
    }
    break;

  case 1: // Scroll city name (non-blocking)
  {
    if (cityScrollX == -16)
    {
      secondTimer.forceReady();
    }
    if (secondTimer.isReady(40))
    {
      Screen.clear();
      String city = config.getWeatherLocation();
      city.toUpperCase();
      int fw = fonts[0].sizeX + 1; // char width + spacing
      int textWidth = city.length() * fw;

      for (unsigned int c = 0; c < city.length(); c++)
      {
        int xPos = c * fw - cityScrollX;
        if (xPos > -8 && xPos < 16)
        {
          int fontIdx = city.charAt(c) - fonts[0].offset;
          if (fontIdx >= 0 && fontIdx < (int)fonts[0].data.size())
          {
            Screen.drawCharacter(xPos, 4,
                               Screen.readBytes(fonts[0].data[fontIdx]), 8, 180);
          }
        }
      }
      cityScrollX++;
      if (cityScrollX >= textWidth)
      {
        cityScrollX = -16;
        displayMode = 2;
        modeStartTime = millis();
        secondTimer.forceReady();
      }
    }
    break;
  }

  case 2: // Temperature + weather icon
    if (secondTimer.isReady(1000))
    {
      drawTemperature();
    }
    // Show weather for 5s if data available, 2s if no data
    if (elapsed > (hasWeatherData ? 5000UL : 2000UL))
    {
      displayMode = 0;
      modeStartTime = millis();
      colonVisible = true;
      secondTimer.forceReady();
    }
    break;
  }
}

void EspooClockPlugin::teardown()
{
  // Nothing to clean up - HTTPClient is local to fetchWeather()
}

const char *EspooClockPlugin::getName() const
{
  return "Espoo Clock";
}
