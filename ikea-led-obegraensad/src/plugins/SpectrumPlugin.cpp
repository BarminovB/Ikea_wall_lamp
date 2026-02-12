#include "plugins/SpectrumPlugin.h"

void SpectrumPlugin::setup()
{
  Screen.clear();
  for (int i = 0; i < 16; i++)
  {
    levels[i] = 0;
    targets[i] = random(2, 14);
    peaks[i] = 0;
  }
}

void SpectrumPlugin::loop()
{
  // Generate new random targets periodically
  if (targetTimer.isReady(300))
  {
    for (int i = 0; i < 16; i++)
    {
      // Bass frequencies (left) tend to be higher
      int maxH = (i < 4) ? 15 : (i < 8) ? 13 : (i < 12) ? 10 : 8;
      targets[i] = random(1, maxH);
    }
  }

  if (!frameTimer.isReady(40))
    return;

  Screen.clear();

  for (int i = 0; i < 16; i++)
  {
    // Smooth approach to target
    if (levels[i] < targets[i])
      levels[i] += (targets[i] - levels[i]) * 0.3f;
    else
      levels[i] += (targets[i] - levels[i]) * 0.15f;

    // Update peak (falling dot)
    if (levels[i] > peaks[i])
      peaks[i] = levels[i];
    else
      peaks[i] -= 0.2f;

    if (peaks[i] < 0)
      peaks[i] = 0;

    int barHeight = (int)(levels[i] + 0.5f);

    // Draw bar from bottom up
    for (int y = 0; y < barHeight; y++)
    {
      int screenY = 15 - y;
      // Gradient: brighter at top
      uint8_t brightness = 80 + (uint8_t)((175.0f * y) / 15.0f);
      Screen.setPixel(i, screenY, 1, brightness);
    }

    // Draw peak dot
    int peakY = 15 - (int)(peaks[i] + 0.5f);
    if (peakY >= 0 && peakY < 16)
    {
      Screen.setPixel(i, peakY, 1, 255);
    }
  }
}

const char *SpectrumPlugin::getName() const
{
  return "Spectrum";
}
