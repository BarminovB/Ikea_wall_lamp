#include "plugins/PlasmaPlugin.h"
#include <math.h>

void PlasmaPlugin::setup()
{
  Screen.clear();
  time_ = 0.0f;
  frameTimer.forceReady();
}

void PlasmaPlugin::loop()
{
  if (!frameTimer.isReady(40))
    return;

  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < 16; x++)
    {
      float fx = (float)x / 16.0f;
      float fy = (float)y / 16.0f;

      float v = sinf(fx * 10.0f + time_);
      v += sinf(fy * 8.0f + time_ * 0.7f);
      v += sinf((fx + fy) * 6.0f + time_ * 1.3f);
      v += sinf(sqrtf(fx * fx + fy * fy) * 8.0f + time_ * 0.5f);

      // Normalize from [-4,4] to [0,255]
      uint8_t brightness = (uint8_t)((v + 4.0f) * 31.875f);

      Screen.setPixel(x, y, 1, brightness);
    }
  }

  time_ += 0.08f;
  if (time_ > 628.0f)
    time_ = 0.0f;
}

const char *PlasmaPlugin::getName() const
{
  return "Plasma";
}
