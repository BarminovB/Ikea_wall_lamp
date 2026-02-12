#include "plugins/DinoPlugin.h"
#include <Arduino.h>
#include <cstring>

// Jump arc: 13 frames, peak at -7 pixels
static const int8_t JUMP_ARC[13] = {-2, -4, -5, -6, -7, -7, -7, -6, -5, -4, -2, -1, 0};
static constexpr int JUMP_LEN = 13;

void DinoPlugin::setup()
{
  Screen.clear();
  frameTimer.forceReady();
  jumpOffset = 0;
  isJumping = false;
  jumpTick = 0;
  runFrame = 0;
  frameTick = 0;
  numCacti = 0;
  spawnTimer = 10;
}

void DinoPlugin::drawDino(int bx, int by)
{
  // T-Rex facing right, 5 wide x 7 tall
  // Row 0: head top   ..###
  Screen.setPixel(bx + 2, by, 1, 220);
  Screen.setPixel(bx + 3, by, 1, 220);
  Screen.setPixel(bx + 4, by, 1, 220);
  // Row 1: head       .####
  Screen.setPixel(bx + 1, by + 1, 1, 220);
  Screen.setPixel(bx + 2, by + 1, 1, 220);
  Screen.setPixel(bx + 3, by + 1, 1, 220);
  Screen.setPixel(bx + 4, by + 1, 1, 220);
  // Row 2: neck       ..##.
  Screen.setPixel(bx + 2, by + 2, 1, 220);
  Screen.setPixel(bx + 3, by + 2, 1, 220);
  // Row 3: body+tail  #####
  Screen.setPixel(bx, by + 3, 1, 220);
  Screen.setPixel(bx + 1, by + 3, 1, 220);
  Screen.setPixel(bx + 2, by + 3, 1, 220);
  Screen.setPixel(bx + 3, by + 3, 1, 220);
  Screen.setPixel(bx + 4, by + 3, 1, 220);
  // Row 4: body       .###.
  Screen.setPixel(bx + 1, by + 4, 1, 220);
  Screen.setPixel(bx + 2, by + 4, 1, 220);
  Screen.setPixel(bx + 3, by + 4, 1, 220);
  // Row 5: thighs     ..##.
  Screen.setPixel(bx + 2, by + 5, 1, 220);
  Screen.setPixel(bx + 3, by + 5, 1, 220);
  // Row 6: feet (animated)
  if (isJumping)
  {
    // Legs together when airborne
    Screen.setPixel(bx + 2, by + 6, 1, 220);
    Screen.setPixel(bx + 3, by + 6, 1, 220);
  }
  else if (runFrame == 0)
  {
    Screen.setPixel(bx + 1, by + 6, 1, 220);
    Screen.setPixel(bx + 4, by + 6, 1, 220);
  }
  else
  {
    Screen.setPixel(bx + 2, by + 6, 1, 220);
    Screen.setPixel(bx + 4, by + 6, 1, 220);
  }
}

void DinoPlugin::drawCactus(const Cactus &c)
{
  int groundY = 14;
  int top = groundY - c.h + 1;
  if (c.w == 1)
  {
    // Thin cactus: vertical line
    for (int y = top; y <= groundY; y++)
    {
      if (c.x >= 0 && c.x < 16)
        Screen.setPixel(c.x, y, 1, 180);
    }
  }
  else
  {
    // Wide cactus: stem + branches
    for (int y = top; y <= groundY; y++)
    {
      if (c.x + 1 >= 0 && c.x + 1 < 16)
        Screen.setPixel(c.x + 1, y, 1, 180); // center stem
    }
    // Branches at 2nd row from top
    int branchY = top + 1;
    if (branchY >= 0 && branchY < 16)
    {
      if (c.x >= 0 && c.x < 16)
        Screen.setPixel(c.x, branchY, 1, 180);
      if (c.x + 2 >= 0 && c.x + 2 < 16)
        Screen.setPixel(c.x + 2, branchY, 1, 180);
    }
  }
}

void DinoPlugin::loop()
{
  if (!frameTimer.isReady(55))
    return;

  frameTick++;

  // Scroll obstacles left
  for (int i = 0; i < numCacti; i++)
    cacti[i].x--;

  // Remove off-screen obstacles
  while (numCacti > 0 && cacti[0].x + cacti[0].w < 0)
  {
    memmove(&cacti[0], &cacti[1], (numCacti - 1) * sizeof(Cactus));
    numCacti--;
  }

  // Spawn new obstacles
  spawnTimer--;
  if (spawnTimer <= 0 && numCacti < MAX_CACTI)
  {
    int type = random(4);
    Cactus c;
    c.x = 16;
    if (type == 0)
    {
      c.h = 3;
      c.w = 1;
    }
    else if (type == 1)
    {
      c.h = 4;
      c.w = 1;
    }
    else if (type == 2)
    {
      c.h = 5;
      c.w = 1;
    }
    else
    {
      c.h = 3;
      c.w = 3;
    }
    cacti[numCacti++] = c;
    spawnTimer = 18 + random(14);
  }

  // Auto-jump: check nearest obstacle ahead of dino
  if (!isJumping)
  {
    for (int i = 0; i < numCacti; i++)
    {
      if (cacti[i].x > 5 && cacti[i].x <= 12)
      {
        isJumping = true;
        jumpTick = 0;
        break;
      }
    }
  }

  // Update jump
  if (isJumping)
  {
    jumpOffset = JUMP_ARC[jumpTick];
    jumpTick++;
    if (jumpTick >= JUMP_LEN)
    {
      isJumping = false;
      jumpOffset = 0;
    }
  }

  // Animate running legs every 4 frames
  if (!isJumping && frameTick % 4 == 0)
    runFrame = 1 - runFrame;

  // Draw
  Screen.clear();

  // Ground: scrolling dotted line at row 15
  for (int x = 0; x < 16; x++)
  {
    if ((x + frameTick / 2) % 3 != 0)
      Screen.setPixel(x, 15, 1, 60);
  }

  // Dino at x=1, base y=8 (feet at y=14 on ground)
  drawDino(1, 8 + jumpOffset);

  // Cacti
  for (int i = 0; i < numCacti; i++)
    drawCactus(cacti[i]);
}

const char *DinoPlugin::getName() const
{
  return "Dino Run";
}
