#include "plugins/CityClockPlugin.h"
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

void CityClockPlugin::loadConfig()
{
#ifdef ENABLE_STORAGE
  Preferences prefs;
  prefs.begin("cityclock", true);
  currentCityIndex = prefs.getInt("cityIdx", 0);
  prefs.end();

  // Validate saved index
  if (currentCityIndex < 0 || currentCityIndex >= cityCount)
  {
    currentCityIndex = 0;
  }
#else
  currentCityIndex = 0;
#endif
}

String CityClockPlugin::getCurrentCityName()
{
  if (currentCityIndex >= 0 && currentCityIndex < cityCount)
  {
    return String(cities[currentCityIndex].name);
  }
  return "Unknown";
}

void CityClockPlugin::switchToCity(int index)
{
  if (index < 0 || index >= cityCount)
    return;

  currentCityIndex = index;

  // Apply timezone
  const char *tz = cities[currentCityIndex].timezone;
  setenv("TZ", tz, 1);
  tzset();

  Serial.print("[CityClockPlugin] Changed to city: ");
  Serial.println(cities[currentCityIndex].name);

  // Save configuration
#ifdef ENABLE_STORAGE
  Preferences prefs;
  prefs.begin("cityclock", false);
  prefs.putInt("cityIdx", currentCityIndex);
  prefs.end();
#endif

  // Reset weather
  hasWeatherData = false;
  lastWeatherUpdate = 0;
  weatherIcon = -1;
  cachedTemperature = -99;

  // Reset display
  displayMode = 0;
  modeStartTime = millis();
  colonVisible = true;
  secondTimer.forceReady();

  // Don't fetch weather here - let loop() handle it via lastWeatherUpdate == 0
  // Avoids blocking HTTPS request during plugin switch (5-10 sec)
}

int CityClockPlugin::mapWmoCode(int code, bool isNight)
{
  switch (code)
  {
  case 0:
  case 1:
    return isNight ? 7 : 2;
  case 2:
    return 3;
  case 3:
    return 0;
  case 45:
  case 48:
    return 6;
  case 51: case 53: case 55:
  case 56: case 57:
  case 61: case 63: case 65:
  case 66: case 67:
  case 80: case 81: case 82:
    return 4;
  case 71: case 73: case 75: case 77:
  case 85: case 86:
    return 5;
  case 95: case 96: case 99:
    return 1;
  default:
    return isNight ? 7 : 2;
  }
}

void CityClockPlugin::setup()
{
  Screen.clear();
  displayMode = 0;
  modeStartTime = millis();
  colonVisible = true;
  secondTimer.forceReady();

  loadConfig();

  // Save current timezone
  const char *envTz = getenv("TZ");
  if (envTz)
  {
    strncpy(savedTz, envTz, sizeof(savedTz) - 1);
    savedTz[sizeof(savedTz) - 1] = '\0';
  }
  else
  {
    savedTz[0] = '\0';
  }

  Serial.printf("[CityClockPlugin] Setup: city %d (%s)\n",
                currentCityIndex, cities[currentCityIndex].name);

  // Apply timezone directly - do NOT call switchToCity() which resets weather cache.
  // Plugin objects persist across activations (created once via new).
  // Resetting lastWeatherUpdate=0 forces an immediate HTTPS fetch (~40KB SSL alloc).
  // With scheduler cycling through 20+ plugins, repeated alloc/free fragments heap → crash.
  // Weather refreshes naturally every 10 minutes via the timer in loop().
  const char *tz = cities[currentCityIndex].timezone;
  setenv("TZ", tz, 1);
  tzset();

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

void CityClockPlugin::fetchWeather()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf("[CityClock] WiFi not connected (status=%d)\n", WiFi.status());
    return;
  }
  if (currentCityIndex < 0 || currentCityIndex >= cityCount)
  {
    Serial.printf("[CityClock] Invalid city index: %d\n", currentCityIndex);
    return;
  }

#ifdef ESP32
  Serial.printf("[CityClock] Heap free=%u maxAlloc=%u RSSI=%d\n",
                ESP.getFreeHeap(), ESP.getMaxAllocHeap(), WiFi.RSSI());
#endif

  // Open-Meteo API with city coordinates
  char url[200];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f&current=temperature_2m,weather_code,is_day",
           cities[currentCityIndex].latitude, cities[currentCityIndex].longitude);
  Serial.printf("[CityClock] GET %s\n", url);

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
    Serial.printf("[CityClock] Payload: %s\n", payload.c_str());

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

      Serial.printf("[CityClock] Weather OK: %d°C wmo=%d icon=%d\n",
                    cachedTemperature, wmoCode, weatherIcon);
    }
    else
    {
      Serial.printf("[CityClock] JSON parse error: %s\n", err.c_str());
    }
  }
  else
  {
    Serial.printf("[CityClock] HTTP FAILED: %d (%s)\n", code, http.errorToString(code).c_str());
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

void CityClockPlugin::drawClock()
{
  if (!getLocalTime(&timeinfo, 10) || timeinfo.tm_year < (2020 - 1900))
  {
    Screen.drawLine(0, 0, 15, 15, 1, 128);
    Screen.drawLine(15, 0, 0, 15, 1, 128);
    return;
  }

  Screen.clear();

  std::vector<int> hh = {
      (timeinfo.tm_hour - timeinfo.tm_hour % 10) / 10,
      timeinfo.tm_hour % 10};
  Screen.drawNumbers(3, 0, hh);

  if (colonVisible)
  {
    Screen.setPixel(7, 6, 1, 180);
    Screen.setPixel(8, 6, 1, 180);
  }

  std::vector<int> mm = {
      (timeinfo.tm_min - timeinfo.tm_min % 10) / 10,
      timeinfo.tm_min % 10};
  Screen.drawNumbers(3, 7, mm);
}

void CityClockPlugin::drawTemperature()
{
  Screen.clear();

  if (!hasWeatherData)
  {
    Screen.setPixel(5, 7, 1);
    Screen.setPixel(7, 7, 1);
    Screen.setPixel(9, 7, 1);
    return;
  }

  int temp = cachedTemperature;

  if (temp >= 10)
  {
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

  if (weatherIcon >= 0 && weatherIcon < (int)weatherIcons.size())
  {
    Screen.drawWeather(0, 7, weatherIcon);
  }
}

void CityClockPlugin::loop()
{
  // Fetch weather every 10 minutes (600000 ms)
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

  case 1: // Scroll city name
  {
    if (cityScrollX == -16)
    {
      secondTimer.forceReady();
    }
    if (secondTimer.isReady(40))
    {
      Screen.clear();
      String cityStr = getCurrentCityName();
      cityStr.toUpperCase();
      int fw = fonts[0].sizeX + 1;
      int textWidth = cityStr.length() * fw;

      for (unsigned int c = 0; c < cityStr.length(); c++)
      {
        int xPos = c * fw - cityScrollX;
        if (xPos > -8 && xPos < 16)
        {
          int fontIdx = cityStr.charAt(c) - fonts[0].offset;
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

void CityClockPlugin::teardown()
{
  // Restore previous timezone
  if (savedTz[0] != '\0')
  {
    setenv("TZ", savedTz, 1);
  }
  else
  {
    setenv("TZ", config.getTzInfo().c_str(), 1);
  }
  tzset();

  Serial.println("[CityClockPlugin] Teardown complete");
}

void CityClockPlugin::websocketHook(JsonDocument &request)
{
  if (request["event"] == "cityclock")
  {
    if (request["cityIndex"].is<int>())
    {
      int newCityIndex = request["cityIndex"].as<int>();
      Serial.print("[CityClockPlugin] Received city index: ");
      Serial.println(newCityIndex);

      switchToCity(newCityIndex);
    }
    else if (request["city"].is<String>())
    {
      // Support city name as well
      String cityName = request["city"].as<String>();
      for (int i = 0; i < cityCount; i++)
      {
        if (String(cities[i].name).equalsIgnoreCase(cityName))
        {
          Serial.print("[CityClockPlugin] Found city index: ");
          Serial.println(i);
          switchToCity(i);
          return;
        }
      }
      Serial.print("[CityClockPlugin] City not found: ");
      Serial.println(cityName.c_str());
    }
    else
    {
      Serial.println("[CityClockPlugin] WARNING: cityIndex is not an int or city is not a string!");
    }
  }
}

const char *CityClockPlugin::getName() const
{
  return "City Clock";
}
