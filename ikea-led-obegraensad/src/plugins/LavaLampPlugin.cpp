#include "plugins/LavaLampPlugin.h"
#include <math.h>

void LavaLampPlugin::setup()
{
  Screen.clear();
  for (int i = 0; i < NUM_BALLS; i++)
  {
    balls[i].x = random(20, 140) / 10.0f;
    balls[i].y = random(20, 140) / 10.0f;
    balls[i].vx = (random(-8, 9)) / 20.0f;
    balls[i].vy = (random(-8, 9)) / 20.0f;
    balls[i].radius = 2.5f + random(0, 15) / 10.0f;
  }
}

void LavaLampPlugin::loop()
{
  if (!frameTimer.isReady(50))
    return;

  // Move balls
  for (int i = 0; i < NUM_BALLS; i++)
  {
    balls[i].x += balls[i].vx;
    balls[i].y += balls[i].vy;

    // Bounce off edges
    if (balls[i].x < 1.0f || balls[i].x > 14.0f)
      balls[i].vx = -balls[i].vx;
    if (balls[i].y < 1.0f || balls[i].y > 14.0f)
      balls[i].vy = -balls[i].vy;

    balls[i].x = constrain(balls[i].x, 0.5f, 15.5f);
    balls[i].y = constrain(balls[i].y, 0.5f, 15.5f);
  }

  // Render metaballs
  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < 16; x++)
    {
      float sum = 0;
      for (int i = 0; i < NUM_BALLS; i++)
      {
        float dx = x - balls[i].x;
        float dy = y - balls[i].y;
        float distSq = dx * dx + dy * dy;
        if (distSq < 0.1f)
          distSq = 0.1f;
        sum += (balls[i].radius * balls[i].radius) / distSq;
      }

      // Threshold and brightness mapping
      if (sum > 0.8f)
      {
        uint8_t brightness;
        if (sum > 2.0f)
          brightness = 255;
        else
          brightness = (uint8_t)((sum - 0.8f) / 1.2f * 255.0f);
        Screen.setPixel(x, y, 1, brightness);
      }
      else
      {
        Screen.setPixel(x, y, 0, 0);
      }
    }
  }
}

const char *LavaLampPlugin::getName() const
{
  return "Lava Lamp";
}
