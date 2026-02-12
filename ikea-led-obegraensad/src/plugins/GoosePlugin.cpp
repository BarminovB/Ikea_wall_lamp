#include "plugins/GoosePlugin.h"
#include "screen.h"
#include <math.h>

// Brightness levels for gradients
#define __ 0
#define OL 70   // outline
#define SH 120  // shadow (body edge)
#define WH 200  // white body
#define BT 255  // bright highlight (body center)
#define BK 190  // beak (orange, bright)
#define LG 160  // legs (orange)
#define EY 25   // eye (nearly dark)

// Walking goose, right-facing, frame A (legs spread)
// 13 wide x 14 tall
#define WALK_W 13
#define WALK_H 14

static const uint8_t GOOSE_WALK_A[WALK_H * WALK_W] PROGMEM = {
    // Row 0: head top
    __, __, __, __, __, __, __, __, __, OL, OL, __, __,
    // Row 1: head (gradient dim→bright)
    __, __, __, __, __, __, __, __, OL, WH, BT, OL, __,
    // Row 2: head + eye + 2px beak
    __, __, __, __, __, __, __, __, OL, EY, BT, BK, BK,
    // Row 3: chin
    __, __, __, __, __, __, __, __, __, OL, OL, __, __,
    // Row 4: neck upper
    __, __, __, __, __, __, __, OL, WH, OL, __, __, __,
    // Row 5: neck lower
    __, __, __, __, __, __, OL, WH, WH, OL, __, __, __,
    // Row 6: shoulder / back
    __, __, __, __, OL, OL, SH, WH, WH, OL, __, __, __,
    // Row 7: body top (gradient edge→center)
    __, __, __, OL, SH, WH, BT, BT, BT, WH, OL, __, __,
    // Row 8: body widest
    __, __, OL, SH, WH, BT, BT, BT, WH, SH, OL, __, __,
    // Row 9: body
    __, OL, SH, WH, BT, BT, BT, WH, SH, OL, __, __, __,
    // Row 10: body rear
    __, OL, SH, WH, WH, BT, WH, SH, OL, __, __, __, __,
    // Row 11: tail
    __, __, OL, OL, OL, OL, OL, __, __, __, __, __, __,
    // Row 12: legs (spread A)
    __, __, __, LG, __, __, __, LG, __, __, __, __, __,
    // Row 13: feet (spread A)
    __, __, LG, LG, __, __, __, __, LG, __, __, __, __,
};

// Walking goose, right-facing, frame B (legs passing)
static const uint8_t GOOSE_WALK_B[WALK_H * WALK_W] PROGMEM = {
    // Rows 0-11: same body as WALK_A
    __, __, __, __, __, __, __, __, __, OL, OL, __, __,
    __, __, __, __, __, __, __, __, OL, WH, BT, OL, __,
    __, __, __, __, __, __, __, __, OL, EY, BT, BK, BK,
    __, __, __, __, __, __, __, __, __, OL, OL, __, __,
    __, __, __, __, __, __, __, OL, WH, OL, __, __, __,
    __, __, __, __, __, __, OL, WH, WH, OL, __, __, __,
    __, __, __, __, OL, OL, SH, WH, WH, OL, __, __, __,
    __, __, __, OL, SH, WH, BT, BT, BT, WH, OL, __, __,
    __, __, OL, SH, WH, BT, BT, BT, WH, SH, OL, __, __,
    __, OL, SH, WH, BT, BT, BT, WH, SH, OL, __, __, __,
    __, OL, SH, WH, WH, BT, WH, SH, OL, __, __, __, __,
    __, __, OL, OL, OL, OL, OL, __, __, __, __, __, __,
    // Row 12: legs (passing)
    __, __, __, __, LG, __, LG, __, __, __, __, __, __,
    // Row 13: feet (passing)
    __, __, __, __, LG, LG, LG, __, __, __, __, __, __,
};

// Standing goose, right-facing
// 12 wide x 16 tall (fills full display height)
#define STAND_W 12
#define STAND_H 16

static const uint8_t GOOSE_STAND[STAND_H * STAND_W] PROGMEM = {
    // Row 0: head top
    __, __, __, __, __, OL, OL, __, __, __, __, __,
    // Row 1: head
    __, __, __, __, OL, WH, BT, OL, __, __, __, __,
    // Row 2: head + eye + 2px beak
    __, __, __, __, OL, EY, BT, OL, BK, BK, __, __,
    // Row 3: chin
    __, __, __, __, __, OL, WH, OL, __, __, __, __,
    // Row 4: neck
    __, __, __, __, __, OL, WH, OL, __, __, __, __,
    // Row 5: neck widens
    __, __, __, __, OL, SH, WH, OL, __, __, __, __,
    // Row 6: shoulder (gradient)
    __, __, __, OL, SH, WH, BT, WH, OL, __, __, __,
    // Row 7: body
    __, __, OL, SH, WH, BT, BT, WH, WH, OL, __, __,
    // Row 8: body widest
    __, OL, SH, WH, BT, BT, BT, WH, SH, OL, __, __,
    // Row 9: body
    __, OL, SH, WH, BT, BT, BT, WH, SH, OL, __, __,
    // Row 10: body narrows
    __, __, OL, SH, WH, BT, WH, SH, OL, __, __, __,
    // Row 11: body
    __, __, __, OL, SH, WH, WH, OL, __, __, __, __,
    // Row 12: bottom
    __, __, __, __, OL, SH, OL, __, __, __, __, __,
    // Row 13: under
    __, __, __, __, __, OL, __, __, __, __, __, __,
    // Row 14: legs
    __, __, __, __, LG, __, __, LG, __, __, __, __,
    // Row 15: feet
    __, __, __, LG, LG, __, __, LG, LG, __, __, __,
};

// Honking goose, right-facing (beak wide open)
// 13 wide x 16 tall
#define HONK_W 13
#define HONK_H 16

static const uint8_t GOOSE_HONK[HONK_H * HONK_W] PROGMEM = {
    // Row 0: head top
    __, __, __, __, __, OL, OL, __, __, __, __, __, __,
    // Row 1: head
    __, __, __, __, OL, WH, BT, OL, __, __, __, __, __,
    // Row 2: upper beak (3px wide open!)
    __, __, __, __, OL, EY, BT, BK, BK, BK, __, __, __,
    // Row 3: lower beak (open mouth gap)
    __, __, __, __, __, OL, WH, __, BK, BK, __, __, __,
    // Row 4: neck
    __, __, __, __, __, OL, WH, OL, __, __, __, __, __,
    // Row 5: neck widens
    __, __, __, __, OL, SH, WH, OL, __, __, __, __, __,
    // Row 6: shoulder
    __, __, __, OL, SH, WH, BT, WH, OL, __, __, __, __,
    // Row 7: body
    __, __, OL, SH, WH, BT, BT, WH, WH, OL, __, __, __,
    // Row 8: body widest
    __, OL, SH, WH, BT, BT, BT, WH, SH, OL, __, __, __,
    // Row 9: body
    __, OL, SH, WH, BT, BT, BT, WH, SH, OL, __, __, __,
    // Row 10: body narrows
    __, __, OL, SH, WH, BT, WH, SH, OL, __, __, __, __,
    // Row 11: body
    __, __, __, OL, SH, WH, WH, OL, __, __, __, __, __,
    // Row 12: bottom
    __, __, __, __, OL, SH, OL, __, __, __, __, __, __,
    // Row 13: under
    __, __, __, __, __, OL, __, __, __, __, __, __, __,
    // Row 14: legs
    __, __, __, __, LG, __, __, LG, __, __, __, __, __,
    // Row 15: feet
    __, __, __, LG, LG, __, __, LG, LG, __, __, __, __,
};

#undef __
#undef OL
#undef SH
#undef WH
#undef BT
#undef BK
#undef LG
#undef EY

void GoosePlugin::drawSprite(const uint8_t *sprite, int sprW, int sprH,
                             int xPos, int yPos, uint8_t brightnessScale)
{
  for (int y = 0; y < sprH; y++)
  {
    int sy = yPos + y;
    if (sy < 0 || sy >= 16)
      continue;
    for (int x = 0; x < sprW; x++)
    {
      int sx = xPos + x;
      if (sx < 0 || sx >= 16)
        continue;
      uint8_t b = pgm_read_byte(&sprite[y * sprW + x]);
      if (b > 0)
      {
        int scaled = ((int)b * brightnessScale) >> 8;
        if (scaled > 0)
          Screen.setPixel(sx, sy, 1, (uint8_t)(scaled > 255 ? 255 : scaled));
      }
    }
  }
}

void GoosePlugin::drawExclamation(int x, int y, uint8_t brightness)
{
  // Chunky "!" mark: 2px wide
  for (int dy = 0; dy < 4; dy++)
  {
    Screen.setPixel(x, y + dy, 1, brightness);
    Screen.setPixel(x + 1, y + dy, 1, brightness);
  }
  // dot
  Screen.setPixel(x, y + 5, 1, brightness);
  Screen.setPixel(x + 1, y + 5, 1, brightness);
}

void GoosePlugin::setup()
{
  Screen.clear();
  phase = 0;
  phaseStart = millis();
  gooseX = -WALK_W;
  walkFrame = 0;
  frameTimer.forceReady();
}

void GoosePlugin::loop()
{
  if (!frameTimer.isReady(50))
    return;

  Screen.clear();
  unsigned long elapsed = millis() - phaseStart;

  // Y positions: align feet at row 15 (bottom of display)
  int walkY = 16 - WALK_H; // 2
  int standY = 0;           // standing is 16 tall, fills full height
  int honkY = 0;

  // Target X: position goose centered
  int targetX = 2;

  switch (phase)
  {
  case 0: // Walk in from left → center
  {
    int step = (int)(elapsed / 100);
    gooseX = -WALK_W + step;

    if (gooseX >= targetX)
    {
      gooseX = targetX;
      phase = 1;
      phaseStart = millis();
      break;
    }

    walkFrame = (step % 2);
    const uint8_t *frame = walkFrame ? GOOSE_WALK_B : GOOSE_WALK_A;
    drawSprite(frame, WALK_W, WALK_H, gooseX, walkY);
    break;
  }

  case 1: // Standing idle
  {
    drawSprite(GOOSE_STAND, STAND_W, STAND_H, targetX, standY);

    if (elapsed > 2500)
    {
      phase = 2;
      phaseStart = millis();
    }
    break;
  }

  case 2: // HONK!
  {
    drawSprite(GOOSE_HONK, HONK_W, HONK_H, targetX, honkY);

    // Blinking "!" to the right of the beak
    bool showBang = ((elapsed / 250) % 2) == 0;
    if (showBang)
    {
      drawExclamation(targetX + 11, 0, 220);
    }

    if (elapsed > 2000)
    {
      phase = 3;
      phaseStart = millis();
      gooseX = targetX;
    }
    break;
  }

  case 3: // Run away to the right (faster!)
  {
    int step = (int)(elapsed / 70);
    gooseX = targetX + step;

    if (gooseX > 16)
    {
      phase = 4;
      phaseStart = millis();
      break;
    }

    walkFrame = (step % 2);
    const uint8_t *frame = walkFrame ? GOOSE_WALK_B : GOOSE_WALK_A;
    drawSprite(frame, WALK_W, WALK_H, gooseX, walkY);
    break;
  }

  case 4: // Pause before loop
  {
    if (elapsed > 1000)
    {
      phase = 0;
      phaseStart = millis();
      gooseX = -WALK_W;
    }
    break;
  }
  }
}

const char *GoosePlugin::getName() const
{
  return "Goose";
}
