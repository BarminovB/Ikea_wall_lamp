#include "plugins/MarioPlugin.h"
#include "screen.h"

// Mario run frame 1 (5w x 7h, MSB = leftmost pixel)
static const uint8_t MARIO_RUN1[7] = {
    0x70, // .XXX.  hat
    0xF8, // XXXXX  hat brim
    0x50, // .X.X.  eyes
    0x70, // .XXX.  body
    0xF8, // XXXXX  arms
    0x50, // .X.X.  legs apart
    0x88, // X...X  feet wide
};

// Mario run frame 2 (5w x 7h)
static const uint8_t MARIO_RUN2[7] = {
    0x70, // .XXX.
    0xF8, // XXXXX
    0x50, // .X.X.
    0x70, // .XXX.
    0xF8, // XXXXX
    0x20, // ..X..  legs together
    0x50, // .X.X.  feet
};

// Mario jump (5w x 7h)
static const uint8_t MARIO_JUMP[7] = {
    0x70, // .XXX.
    0xF8, // XXXXX
    0x50, // .X.X.
    0x70, // .XXX.
    0xF8, // XXXXX
    0x88, // X...X  legs spread
    0x00, // .....  airborne
};

// Cloud (5w x 2h)
static const uint8_t CLOUD[2] = {
    0x70, // .XXX.
    0xF8, // XXXXX
};

// World parameters
#define WORLD_LEN 80
#define GROUND_Y 14
#define MARIO_X 2
#define MARIO_GROUND_Y 7 // top of 7-tall sprite: 14 - 7 = 7

// Jump arc (vertical offsets)
static const int JUMP_ARC[] = {0, -1, -2, -4, -5, -6, -6, -6, -5, -4, -2, -1, 0};
static const int JUMP_LEN = 13;

// Pipe definitions (world x, height in pixels)
struct PipeDef
{
  int x;
  int h;
};
static const PipeDef PIPES[] = {{20, 4}, {42, 3}, {62, 5}};
static const int NUM_PIPES = 3;

// Coin definitions (world x, screen y)
struct CoinDef
{
  int x;
  int y;
};
static const CoinDef COINS[] = {{13, 5}, {35, 6}, {55, 5}};
static const int NUM_COINS = 3;

// Cloud definitions (world x, screen y) — move at half speed
struct CloudDef
{
  int x;
  int y;
};
static const CloudDef CLOUDS[] = {{5, 0}, {30, 1}, {58, 0}};
static const int NUM_CLOUDS = 3;

// Question block definitions (world x, screen y)
struct QBlockDef
{
  int x;
  int y;
};
static const QBlockDef QBLOCKS[] = {{10, 7}, {33, 8}, {52, 7}};
static const int NUM_QBLOCKS = 3;

static int screenX(int worldX, int offset)
{
  int sx = worldX - (offset % WORLD_LEN);
  while (sx < -16)
    sx += WORLD_LEN;
  while (sx >= WORLD_LEN)
    sx -= WORLD_LEN;
  return sx;
}

void MarioPlugin::drawSprite(const uint8_t *sprite, int rows, int x, int y,
                             int width, uint8_t brightness)
{
  for (int row = 0; row < rows; row++)
  {
    uint8_t data = sprite[row];
    for (int col = 0; col < width; col++)
    {
      if (data & (0x80 >> col))
      {
        int px = x + col;
        int py = y + row;
        if (px >= 0 && px < 16 && py >= 0 && py < 16)
          Screen.setPixel(px, py, 1, brightness);
      }
    }
  }
}

void MarioPlugin::setup()
{
  frameTimer.forceReady();
  worldOffset = 0;
  marioFrame = 0;
  jumpPhase = -1;
  marioY = MARIO_GROUND_Y;
  frameCount = 0;
}

void MarioPlugin::loop()
{
  if (!frameTimer.isReady(70))
    return;

  Screen.clear();

  // --- Ground: brick pattern (rows 14-15) ---
  for (int x = 0; x < 16; x++)
  {
    int wx = (x + worldOffset) & 3; // % 4
    // Row 14: bricks with mortar gap every 4th pixel
    Screen.setPixel(x, 14, 1, (wx == 3) ? (uint8_t)60 : (uint8_t)110);
    // Row 15: offset brick pattern
    int wx2 = (x + worldOffset + 2) & 3;
    Screen.setPixel(x, 15, 1, (wx2 == 3) ? (uint8_t)60 : (uint8_t)110);
  }

  // --- Clouds (half-speed parallax) ---
  int cloudOff = worldOffset / 2;
  for (int i = 0; i < NUM_CLOUDS; i++)
  {
    int cx = CLOUDS[i].x - (cloudOff % WORLD_LEN);
    while (cx < -5)
      cx += WORLD_LEN;
    while (cx >= WORLD_LEN)
      cx -= WORLD_LEN;
    if (cx >= -5 && cx < 16)
    {
      drawSprite(CLOUD, 2, cx, CLOUDS[i].y, 5, 50);
    }
  }

  // --- Question blocks (3x3 blinking) ---
  bool qblockOn = ((frameCount / 10) & 1) == 0;
  for (int i = 0; i < NUM_QBLOCKS; i++)
  {
    int sx = screenX(QBLOCKS[i].x, worldOffset);
    if (sx >= -3 && sx < 16)
    {
      uint8_t br = qblockOn ? 200 : 80;
      for (int r = 0; r < 3; r++)
      {
        for (int c = 0; c < 3; c++)
        {
          int px = sx + c;
          int py = QBLOCKS[i].y + r;
          // Draw border + center dot
          if (px >= 0 && px < 16 && py >= 0 && py < 16)
          {
            if (r == 0 || r == 2 || c == 0 || c == 2)
              Screen.setPixel(px, py, 1, br);
            else if (qblockOn)
              Screen.setPixel(px, py, 1, br); // center dot on blink
          }
        }
      }
    }
  }

  // --- Coins (blinking 2x2) ---
  bool coinOn = ((frameCount / 8) & 1) == 0;
  if (coinOn)
  {
    for (int i = 0; i < NUM_COINS; i++)
    {
      int sx = screenX(COINS[i].x, worldOffset);
      if (sx >= -2 && sx < 16)
      {
        for (int r = 0; r < 2; r++)
        {
          for (int c = 0; c < 2; c++)
          {
            int px = sx + c;
            int py = COINS[i].y + r;
            if (px >= 0 && px < 16 && py >= 0 && py < 16)
              Screen.setPixel(px, py, 1, 180);
          }
        }
      }
    }
  }

  // --- Pipes ---
  for (int i = 0; i < NUM_PIPES; i++)
  {
    int sx = screenX(PIPES[i].x, worldOffset);
    if (sx >= -4 && sx < 16)
    {
      int pipeTop = GROUND_Y - PIPES[i].h;

      // Pipe cap (4 wide, 1 row)
      for (int c = 0; c < 4; c++)
      {
        int px = sx + c;
        if (px >= 0 && px < 16)
          Screen.setPixel(px, pipeTop, 1, 150);
      }

      // Pipe body (4 wide, dimmer center)
      for (int row = pipeTop + 1; row < GROUND_Y; row++)
      {
        for (int c = 0; c < 4; c++)
        {
          int px = sx + c;
          if (px >= 0 && px < 16)
          {
            uint8_t br = (c == 0 || c == 3) ? 130 : 90; // walls brighter
            Screen.setPixel(px, row, 1, br);
          }
        }
      }
    }
  }

  // --- Auto-jump: look ahead for pipes ---
  if (jumpPhase < 0)
  {
    for (int i = 0; i < NUM_PIPES; i++)
    {
      int sx = screenX(PIPES[i].x, worldOffset);
      // Trigger jump when pipe is 7-12 pixels ahead of Mario
      if (sx > MARIO_X + 5 && sx < MARIO_X + 11)
      {
        jumpPhase = 0;
        break;
      }
    }
  }

  // --- Update jump ---
  if (jumpPhase >= 0)
  {
    marioY = MARIO_GROUND_Y + JUMP_ARC[jumpPhase];
    jumpPhase++;
    if (jumpPhase >= JUMP_LEN)
    {
      jumpPhase = -1;
      marioY = MARIO_GROUND_Y;
    }
  }

  // --- Draw Mario ---
  const uint8_t *sprite;
  if (jumpPhase >= 0)
    sprite = MARIO_JUMP;
  else if (((frameCount / 4) & 1) == 0)
    sprite = MARIO_RUN1;
  else
    sprite = MARIO_RUN2;

  drawSprite(sprite, 7, MARIO_X, marioY, 5, MAX_BRIGHTNESS);

  // --- Advance world ---
  worldOffset++;
  if (worldOffset >= WORLD_LEN * 100)
    worldOffset -= WORLD_LEN;

  frameCount++;
  if (frameCount >= 10000)
    frameCount = 0;
}

const char *MarioPlugin::getName() const
{
  return "Mario";
}
