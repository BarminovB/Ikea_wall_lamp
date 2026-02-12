#include "plugins/DNAHelixPlugin.h"
#include <math.h>

void DNAHelixPlugin::setup()
{
  Screen.clear();
  offset = 0.0f;
}

void DNAHelixPlugin::loop()
{
  if (!frameTimer.isReady(60))
    return;

  Screen.clear();

  for (int y = 0; y < 16; y++)
  {
    float phase = (float)y * 0.45f + offset;

    // Two strands of the helix
    float x1 = 7.5f + 5.0f * sinf(phase);
    float x2 = 7.5f + 5.0f * sinf(phase + 3.14159f);

    // Depth for brightness (cosine gives depth)
    float depth1 = cosf(phase);
    float depth2 = cosf(phase + 3.14159f);

    uint8_t b1 = (uint8_t)(128 + 127 * depth1);
    uint8_t b2 = (uint8_t)(128 + 127 * depth2);

    int px1 = (int)(x1 + 0.5f);
    int px2 = (int)(x2 + 0.5f);

    // Draw strand points
    if (px1 >= 0 && px1 < 16)
      Screen.setPixel(px1, y, 1, b1);
    if (px2 >= 0 && px2 < 16)
      Screen.setPixel(px2, y, 1, b2);

    // Draw connecting rungs every 3 rows
    if (y % 3 == 0)
    {
      int minX = min(px1, px2);
      int maxX = max(px1, px2);
      // Only draw rung when strands are on same depth plane (crossing)
      float depthDiff = fabsf(depth1 - depth2);
      if (depthDiff < 1.5f)
      {
        uint8_t rungB = (uint8_t)((b1 + b2) / 2 * 0.5f);
        for (int x = minX + 1; x < maxX; x++)
        {
          if (x >= 0 && x < 16)
            Screen.setPixel(x, y, 1, rungB);
        }
      }
    }
  }

  offset += 0.12f;
  if (offset > 6.28f)
    offset -= 6.28f;
}

const char *DNAHelixPlugin::getName() const
{
  return "DNA Helix";
}
