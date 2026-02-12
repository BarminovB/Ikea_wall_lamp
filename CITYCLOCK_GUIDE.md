# 🕐 City Clock Plugin - User Guide

## Overview
The **City Clock Plugin** displays the current time and weather for any city in the world on your LED matrix.

### Features:
✅ Real-time clock display (HH:MM format)  
✅ Weather information from wttr.in API  
✅ Automatic timezone support  
✅ Scrolling city name  
✅ Temperature display with weather icons  
✅ Non-blocking weather fetching  

---

## How to Use

### 1. Access the Web Interface
Open your browser and navigate to:
```
http://{DEVICE_IP}/cityclock
```

**Example:** `http://192.168.1.100/cityclock`

### 2. Configure City and Timezone

**Fields:**
- **City Name**: Enter the name of the city (e.g., London, Tokyo, Paris)
  - The plugin uses wttr.in API, which supports nearly all cities worldwide
  - Examples: London, Tokyo, Sydney, New York, Moscow, Seoul, Amsterdam

- **Timezone (POSIX)**: Enter the POSIX timezone string for the city
  - This determines how the time is displayed on your LED matrix

#### Common Timezone Examples:

| City/Region | Timezone String |
|-------------|-----------------|
| London (GMT) | `GMT0` |
| Central Europe (CET) | `CET-1CEST,M3.5.0,M10.5.0/3` |
| New York (EST) | `EST5EDT,M3.2.0,M11.1.0` |
| Tokyo (JST) | `JST-9` |
| Sydney (AEDT/AEST) | `AEST-10AEDT,M10.1.0,M4.1.0/3` |
| São Paulo (BRT) | `BRT3BRST,M10.3.0/0,M2.3.0/0` |

### 3. Submit Configuration
Click **"📍 Set City"** button
- The device will switch to City Clock plugin
- Weather data will start fetching automatically
- Display will show:
  1. Current time with blinking colon (20 seconds)
  2. City name scrolling (variable duration)
  3. Weather icons and temperature

---

## Display Modes

The clock cycles through three display modes:

### Mode 1: Clock Display 🕐
```
HH:MM
```
- Shows hours and minutes
- Colon blinks every 500ms
- Duration: 20 seconds

### Mode 2: City Name 📍
```
[CITY scrolls from right to left]
```
- City name scrolls across the display
- Duration: Variable (depends on city name length)

### Mode 3: Weather & Temperature 🌡️
```
[WEATHER ICON] [TEMPERATURE]
```
- Displays current temperature
- Shows weather icon based on conditions:
  - ⛈️ Thunderstorm
  - 🌤️ Partly Cloudy
  - ☀️ Clear/Sunny
  - 🌧️ Rainy
  - ❄️ Snowy
  - 🌫️ Foggy
  - 🌙 Clear Night

---

## Features in Detail

### Weather Icons
| Weather | Icon ID | Condition |
|---------|---------|-----------|
| Thunder | 1 | Thunderstorm, Lightning |
| Clear | 2 | Sunny, Clear skies (day) |
| Partly Cloudy | 3 | Partly cloudy |
| Cloudy | 0 | Overcast, Cloudy |
| Rainy | 4 | Rain, Drizzle, Showers |
| Snowy | 5 | Snow, Sleet, Ice, Blizzard |
| Foggy | 6 | Fog, Mist, Haze |
| Night | 7 | Clear night (no sun) |

### Data Sources
- **Time**: Device's internal RTC synchronized with NTP
- **Weather**: [wttr.in API](https://wttr.in/) - Free weather data
- **Timezone**: Configured locally via POSIX timezone string

### Update Frequency
- Weather data is fetched every **10 minutes** (600 seconds)
- Time is updated every half second for colon blinking

---

## Troubleshooting

### Weather Shows Dots (⋅⋅⋅)
This means weather data hasn't been fetched yet. 
- **Solution**: Wait 10 seconds, the device will fetch weather automatically
- Ensure your device has internet connection

### City Not Found
If the city name isn't recognized:
- **Solution**: Try a spelling variation or use a more unique city name
- **Examples**: Instead of "Prague", try "Praha"; Instead of "Rome", try "Roma"

### Time Shows Crosses (✕) Pattern
This indicates no valid time has been set:
- **Solution**: Ensure the device is connected to WiFi and NTP is configured
- Check `/config` page for NTP Server settings

### Temperature Shows "?" Symbol
- Weather data couldn't be parsed
- **Solution**: Re-enter the city name and try again

---

## API Endpoints

For advanced users who want to control City Clock programmatically:

### GET Current Configuration
```
GET /api/cityclock
```
**Response:**
```json
{
  "city": "London",
  "tz": "GMT0"
}
```

### WebSocket Event
Send via WebSocket on port 81:
```json
{
  "event": "cityclock",
  "city": "Tokyo",
  "tz": "JST-9",
  "plugin": 7
}
```

---

## POSIX Timezone Format

POSIX timezone strings follow this format:
```
stdoffset[dst[offset],start[/time],end[/time]]
```

**Common Examples:**
- `GMT0` - London (no DST)
- `CET-1CEST,M3.5.0,M10.5.0/3` - Central Europe (with DST)
- `EST5EDT,M3.2.0,M11.1.0` - Eastern US (with DST)
- `JST-9` - Japan (no DST)
- `UTC0` - UTC (no DST)

**Understanding the Components:**
- `CET` = Standard timezone name
- `-1` = UTC offset (CET is UTC+1, hence -1)
- `CEST` = Daylight saving time name
- `M3.5.0` = March, 5th week, Sunday (DST starts)
- `M10.5.0/3` = October, 5th week, Sunday at 3:00 AM (DST ends)

---

## Example Configurations

### London
```
City: London
Timezone: GMT0
```

### Tokyo  
```
City: Tokyo
Timezone: JST-9
```

### New York
```
City: New York
Timezone: EST5EDT,M3.2.0,M11.1.0
```

### Berlin
```
City: Berlin
Timezone: CET-1CEST,M3.5.0,M10.5.0/3
```

### Sydney
```
City: Sydney
Timezone: AEST-10AEDT,M10.1.0,M4.1.0/3
```

---

## Notes

- The device must be **connected to WiFi** to fetch weather data
- Time synchronization requires **NTP server access** (configured in `/config`)
- Weather data updates every **10 minutes** to minimize network traffic
- City name is case-insensitive
- Configuration is **saved to device storage** and persists after reboot

---

## Support

If you encounter issues:
1. Check WiFi connection status
2. Verify city name is spelled correctly
3. Try a different city to isolate the issue
4. Check device logs for error messages

