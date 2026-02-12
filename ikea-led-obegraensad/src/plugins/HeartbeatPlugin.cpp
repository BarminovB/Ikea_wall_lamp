#include "plugins/HeartbeatPlugin.h"
#include <math.h>

// 12x10 pixel heart shape — symmetric, 3px bumps, 2px center dip
static const uint8_t heartShape[10][12] = {
    {0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
};

void HeartbeatPlugin::drawHeart(int offsetX, int offsetY, uint8_t brightness)
{
  for (int y = 0; y < 10; y++)
  {
    for (int x = 0; x < 12; x++)
    {
      if (heartShape[y][x])
      {
        int px = offsetX + x;
        int py = offsetY + y;
        if (px >= 0 && px < 16 && py >= 0 && py < 16)
        {
          Screen.setPixel(px, py, 1, brightness);
        }
      }
    }
  }
}

void HeartbeatPlugin::setup()
{
  Screen.clear();
  beatStart = millis();
}

void HeartbeatPlugin::loop()
{
  if (!frameTimer.isReady(30))
    return;

  unsigned long elapsed = millis() - beatStart;
  float phase = fmodf((float)elapsed / 1000.0f, 1.2f); // 1.2s per beat cycle

  // Double-pulse heartbeat: lub at 0.0-0.15, dub at 0.2-0.35, rest 0.35-1.2
  float scale;
  uint8_t brightness;

  if (phase < 0.15f)
  {
    // First beat (lub) - scale up
    float t = phase / 0.15f;
    scale = 1.0f + 0.2f * sinf(t * 3.14159f);
    brightness = 180 + (uint8_t)(75.0f * sinf(t * 3.14159f));
  }
  else if (phase < 0.2f)
  {
    // Brief pause
    scale = 1.0f;
    brightness = 180;
  }
  else if (phase < 0.35f)
  {
    // Second beat (dub) - bigger
    float t = (phase - 0.2f) / 0.15f;
    scale = 1.0f + 0.3f * sinf(t * 3.14159f);
    brightness = 200 + (uint8_t)(55.0f * sinf(t * 3.14159f));
  }
  else
  {
    // Rest
    float t = (phase - 0.35f) / 0.85f;
    scale = 1.0f;
    brightness = 180 - (uint8_t)(80.0f * t);
    if (brightness < 100)
      brightness = 100;
  }

  Screen.clear();

  // Heart at offset (2, 3), 12x10 — center exactly at display center
  int offX = 2, offY = 3;
  float cx = offX + (12 - 1) / 2.0f; // 2 + 5.5 = 7.5
  float cy = offY + (10 - 1) / 2.0f; // 3 + 4.5 = 7.5

  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < 16; x++)
    {
      // Map screen pixel back to heart space
      float hx = (x - cx) / scale + cx - offX;
      float hy = (y - cy) / scale + cy - offY;

      int ix = (int)floorf(hx + 0.5f);
      int iy = (int)floorf(hy + 0.5f);

      if (ix >= 0 && ix < 12 && iy >= 0 && iy < 10 && heartShape[iy][ix])
      {
        Screen.setPixel(x, y, 1, brightness);
      }
    }
  }
}

const char *HeartbeatPlugin::getName() const
{
  return "Heartbeat";
}
