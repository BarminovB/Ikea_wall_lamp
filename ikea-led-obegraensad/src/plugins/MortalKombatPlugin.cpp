#include "plugins/MortalKombatPlugin.h"
#include <cmath>

// MK dragon logo 16x16 bitmap, MSB = leftmost pixel
static const uint16_t MK_BITMAP[16] = {
    0x07E0, // .....######.....
    0x1818, // ...##......##...
    0x26C4, // ..#..##.##...#..
    0x4FC2, // .#..######....#.
    0x5EF2, // .#.####.####..#.
    0x9E79, // #..####..####..#
    0x8E71, // #...###..###...#
    0x87E1, // #....######....#
    0x83C1, // #.....####.....#
    0x81C1, // #......###.....#
    0x41E2, // .#.....####...#.
    0x4372, // .#....##.###..#.
    0x2634, // ..#..##...##.#..
    0x1C18, // ...###.....##...
    0x0C30, // ....##....##....
    0x03C0  // ......####......
};

void MortalKombatPlugin::setup()
{
  Screen.clear();
  angle = 0.0f;
  frameTimer.forceReady();
}

void MortalKombatPlugin::loop()
{
  if (!frameTimer.isReady(40))
    return;

  Screen.clear();

  float cosA = cosf(angle);
  float absCosA = fabsf(cosA);
  const float center = 7.5f;

  if (absCosA < 0.03f)
  {
    // Edge-on: thin vertical line
    for (int y = 0; y < 16; y++)
    {
      if (MK_BITMAP[y])
      {
        Screen.setPixel(7, y, 1, 80);
        Screen.setPixel(8, y, 1, 80);
      }
    }
  }
  else
  {
    float halfVisible = 8.0f * absCosA;

    for (int x = 0; x < 16; x++)
    {
      float dispOffset = x + 0.5f - center;
      if (fabsf(dispOffset) > halfVisible)
        continue;

      float srcOffset = dispOffset / cosA;
      int srcX = (int)(center + srcOffset);
      if (srcX < 0 || srcX > 15)
        continue;

      // Edge brightness falloff for 3D cylinder look
      float edge = fabsf(dispOffset) / halfVisible;
      uint8_t brightness = (uint8_t)(220.0f - 120.0f * edge * edge);

      for (int y = 0; y < 16; y++)
      {
        if (MK_BITMAP[y] & (0x8000 >> srcX))
        {
          Screen.setPixel(x, y, 1, brightness);
        }
      }
    }
  }

  angle += 0.06f;
  if (angle >= 6.2831853f)
    angle -= 6.2831853f;
}

const char *MortalKombatPlugin::getName() const
{
  return "Mortal Kombat";
}
