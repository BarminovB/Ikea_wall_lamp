#include "plugins/DropletPlugin.h"
#include <math.h>

void DropletPlugin::setup()
{
  Screen.clear();
  for (int i = 0; i < MAX_DROPLETS; i++)
  {
    droplets[i].active = false;
  }
}

void DropletPlugin::loop()
{
  // Spawn new droplets periodically
  if (spawnTimer.isReady(800))
  {
    for (int i = 0; i < MAX_DROPLETS; i++)
    {
      if (!droplets[i].active)
      {
        droplets[i].x = random(2, 14);
        droplets[i].y = random(2, 14);
        droplets[i].radius = 0.0f;
        droplets[i].maxRadius = random(4, 9);
        droplets[i].active = true;
        break;
      }
    }
  }

  if (!frameTimer.isReady(60))
    return;

  Screen.clear();

  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < 16; x++)
    {
      float totalBrightness = 0;

      for (int i = 0; i < MAX_DROPLETS; i++)
      {
        if (!droplets[i].active)
          continue;

        float dx = x - droplets[i].x;
        float dy = y - droplets[i].y;
        float dist = sqrtf(dx * dx + dy * dy);
        float r = droplets[i].radius;

        // Ring with width ~1.5
        float ringDist = fabsf(dist - r);
        if (ringDist < 1.5f)
        {
          float fade = 1.0f - (droplets[i].radius / droplets[i].maxRadius);
          float intensity = (1.0f - ringDist / 1.5f) * fade;
          totalBrightness += intensity * 255.0f;
        }
      }

      if (totalBrightness > 0)
      {
        uint8_t b = (uint8_t)min(255.0f, totalBrightness);
        Screen.setPixel(x, y, 1, b);
      }
    }
  }

  // Expand and deactivate droplets
  for (int i = 0; i < MAX_DROPLETS; i++)
  {
    if (droplets[i].active)
    {
      droplets[i].radius += 0.4f;
      if (droplets[i].radius >= droplets[i].maxRadius)
      {
        droplets[i].active = false;
      }
    }
  }
}

const char *DropletPlugin::getName() const
{
  return "Droplets";
}
