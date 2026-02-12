#include "plugins/CatPlugin.h"

// Base cat shape (common rows), MSB = leftmost pixel
static const uint16_t CAT_BASE[16] = {
    0x0420, // .....#....#.....  ears
    0x0DB0, // ....##.##.##....  ears+head
    0x0FF0, // ....########....  head
    0x0990, // ....#..##..#....  eyes (dark gaps)
    0x0FF0, // ....########....  face
    0x07E0, // .....######.....  jaw
    0x0000, // (frame-specific)  neck+arms
    0x0000, // (frame-specific)  body+arms
    0x0000, // (frame-specific)  body+arms
    0x03C0, // ......####......  body
    0x0180, // .......##.......  hips
    0x0240, // ......#..#......  legs
    0x0420, // .....#....#.....  feet
    0x0000,
    0x0000,
    0x0000};

// Rows 6-8 per animation frame (arms change)
static const uint16_t CAT_ARMS[4][3] = {
    {0x03C8, 0x03D0, 0x07F8}, // frame 0: right arm flex
    {0x13C8, 0x0BD0, 0x1FF8}, // frame 1: both arms flex
    {0x13C0, 0x0BC0, 0x1FE0}, // frame 2: left arm flex
    {0x13C8, 0x0BD0, 0x1FF8}  // frame 3: both arms flex (repeat)
};
// Right flex:  ......####..#...  ......####.#....  .....########...
// Both flex:   ...#..####..#...  ....#.####.#....  ...##########...
// Left flex:   ...#..####......  ....#.####......  ...########.....

void CatPlugin::setup()
{
  Screen.clear();
  frame = 0;
  frameTimer.forceReady();
}

void CatPlugin::loop()
{
  if (!frameTimer.isReady(350))
    return;

  Screen.clear();

  for (int y = 0; y < 16; y++)
  {
    uint16_t row;
    if (y >= 6 && y <= 8)
      row = CAT_ARMS[frame][y - 6];
    else
      row = CAT_BASE[y];

    for (int x = 0; x < 16; x++)
    {
      if (row & (0x8000 >> x))
      {
        Screen.setPixel(x, y, 1, 200);
      }
    }
  }

  frame = (frame + 1) % 4;
}

const char *CatPlugin::getName() const
{
  return "Cat";
}
