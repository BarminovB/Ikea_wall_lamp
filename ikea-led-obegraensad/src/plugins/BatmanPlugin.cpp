#include "plugins/BatmanPlugin.h"
#include "screen.h"
#include <math.h>

// Brightness levels for sprite data
#define B_ 0
#define B1 45
#define B2 90
#define B3 140
#define B4 200
#define B5 255

// Batman standing pose A - cape symmetric
// 16x16, each byte = pixel brightness
static const uint8_t BATMAN_A[256] PROGMEM = {
    // Row 0: empty
    B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_,
    // Row 1: ear tips
    B_, B_, B_, B_, B_, B1, B_, B_, B_, B_, B1, B_, B_, B_, B_, B_,
    // Row 2: ears
    B_, B_, B_, B_, B1, B2, B1, B_, B_, B1, B2, B1, B_, B_, B_, B_,
    // Row 3: cowl
    B_, B_, B_, B_, B2, B3, B3, B3, B3, B3, B3, B2, B_, B_, B_, B_,
    // Row 4: eyes + face
    B_, B_, B_, B_, B2, B5, B1, B3, B1, B5, B2, B_, B_, B_, B_, B_,
    // Row 5: lower face
    B_, B_, B_, B_, B_, B2, B3, B4, B3, B2, B_, B_, B_, B_, B_, B_,
    // Row 6: shoulders
    B_, B_, B_, B_, B2, B3, B4, B4, B4, B3, B2, B_, B_, B_, B_, B_,
    // Row 7: chest + bat logo center
    B_, B_, B_, B1, B2, B3, B2, B5, B2, B3, B2, B1, B_, B_, B_, B_,
    // Row 8: torso + cape start
    B_, B_, B1, B1, B_, B3, B4, B4, B4, B3, B_, B1, B1, B_, B_, B_,
    // Row 9: torso + cape
    B_, B1, B1, B_, B_, B3, B4, B4, B4, B3, B_, B_, B1, B1, B_, B_,
    // Row 10: belt (bright!)
    B_, B1, B1, B_, B_, B5, B5, B5, B5, B5, B_, B_, B1, B1, B_, B_,
    // Row 11: legs + cape
    B1, B1, B_, B_, B_, B2, B3, B_, B3, B2, B_, B_, B_, B1, B1, B_,
    // Row 12: legs
    B1, B_, B_, B_, B_, B2, B3, B_, B3, B2, B_, B_, B_, B_, B1, B_,
    // Row 13: boots + cape
    B1, B1, B_, B_, B2, B3, B3, B_, B3, B3, B2, B_, B_, B1, B1, B_,
    // Row 14: cape bottom
    B_, B1, B1, B1, B1, B1, B_, B_, B_, B1, B1, B1, B1, B1, B_, B_,
    // Row 15: empty
    B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_,
};

// Batman standing pose B - cape blown right
static const uint8_t BATMAN_B[256] PROGMEM = {
    // Rows 0-7: same as A
    B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_,
    B_, B_, B_, B_, B_, B1, B_, B_, B_, B_, B1, B_, B_, B_, B_, B_,
    B_, B_, B_, B_, B1, B2, B1, B_, B_, B1, B2, B1, B_, B_, B_, B_,
    B_, B_, B_, B_, B2, B3, B3, B3, B3, B3, B3, B2, B_, B_, B_, B_,
    B_, B_, B_, B_, B2, B5, B1, B3, B1, B5, B2, B_, B_, B_, B_, B_,
    B_, B_, B_, B_, B_, B2, B3, B4, B3, B2, B_, B_, B_, B_, B_, B_,
    B_, B_, B_, B_, B2, B3, B4, B4, B4, B3, B2, B_, B_, B_, B_, B_,
    B_, B_, B_, B1, B2, B3, B2, B5, B2, B3, B2, B1, B_, B_, B_, B_,
    // Row 8: cape shifts right
    B_, B_, B_, B1, B_, B3, B4, B4, B4, B3, B_, B1, B1, B1, B_, B_,
    // Row 9: right cape extends
    B_, B_, B1, B_, B_, B3, B4, B4, B4, B3, B_, B_, B1, B1, B1, B_,
    // Row 10: belt + cape right
    B_, B_, B1, B_, B_, B5, B5, B5, B5, B5, B_, B_, B1, B1, B1, B_,
    // Row 11: legs + cape blown right
    B_, B1, B_, B_, B_, B2, B3, B_, B3, B2, B_, B_, B_, B1, B1, B1,
    // Row 12: legs + right cape
    B_, B_, B_, B_, B_, B2, B3, B_, B3, B2, B_, B_, B_, B1, B1, B_,
    // Row 13: boots + cape right
    B_, B1, B_, B_, B2, B3, B3, B_, B3, B3, B2, B_, B1, B1, B_, B_,
    // Row 14: cape bottom shifted right
    B_, B_, B1, B1, B1, B1, B_, B_, B_, B1, B1, B1, B1, B_, B_, B_,
    // Row 15
    B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_, B_,
};

#undef B_
#undef B1
#undef B2
#undef B3
#undef B4
#undef B5

// Bat signal mask: 1 = bat shape (darker), bit15 = x=0
static const uint16_t BAT_MASK[16] PROGMEM = {
    0x0000, // 0
    0x0000, // 1
    0x0000, // 2
    0x1008, // 3: ears x=3, x=12
    0x300C, // 4: x=2,3, x=12,13
    0x73CE, // 5: x=1,2,3, x=6,7,8,9, x=12,13,14
    0x7FFE, // 6: x=1-14 full wingspan
    0x3FFC, // 7: x=2-13
    0x1FF8, // 8: x=3-12
    0x0E70, // 9: x=4,5,6, x=9,10,11
    0x1818, // 10: x=3,4, x=11,12
    0x0000, // 11
    0x0000, // 12
    0x0000, // 13
    0x0000, // 14
    0x0000, // 15
};

// City skyline heights (pixels from bottom)
static const uint8_t SKY_H[16] PROGMEM = {
    1, 2, 4, 2, 3, 2, 1, 5, 5, 2, 3, 4, 1, 2, 4, 3};

// Drop animation: Y offsets (accelerating fall + bounce)
static const int8_t DROP_Y[12] PROGMEM = {
    -16, -15, -14, -12, -10, -8, -6, -4, -2, 0, 1, 0};

// Cape animation sequence: 0=pose A, 1=pose B, 2=pose B mirrored
static const uint8_t CAPE_SEQ[4] = {0, 1, 0, 2};

void BatmanPlugin::drawBatmanFrame(const uint8_t *frame, int yOff,
                                   uint8_t scale, bool mirror)
{
  for (int y = 0; y < 16; y++)
  {
    int sy = y + yOff;
    if (sy < 0 || sy >= 16)
      continue;
    for (int x = 0; x < 16; x++)
    {
      int srcX = mirror ? (15 - x) : x;
      uint8_t b = pgm_read_byte(&frame[y * 16 + srcX]);
      if (b > 0)
      {
        int scaled = ((int)b * scale) >> 8;
        if (scaled > 0)
          Screen.setPixel(x, sy, 1, (uint8_t)(scaled > 255 ? 255 : scaled));
      }
    }
  }
}

void BatmanPlugin::drawBatSignal(float pulse, float fade)
{
  float cx = 7.5f, cy = 5.5f;
  float peak = 255.0f * pulse * fade;

  for (int y = 0; y < 12; y++)
  {
    uint16_t mask = pgm_read_word(&BAT_MASK[y]);
    for (int x = 0; x < 16; x++)
    {
      float dx = x - cx;
      float dy = y - cy;
      float dist = sqrtf(dx * dx + dy * dy);

      // Circular gradient falloff
      float raw = peak * fmaxf(0.0f, 1.0f - dist / 9.0f);

      // Bat shape is much dimmer (silhouette cutout)
      bool isBat = (mask >> (15 - x)) & 1;
      if (isBat)
        raw *= 0.15f;

      // Subtle shimmer
      raw += (float)((int)random(-6, 6));

      int bright = (int)raw;
      if (bright > 1 && bright <= 255)
        Screen.setPixel(x, y, 1, (uint8_t)bright);
      else if (bright > 255)
        Screen.setPixel(x, y, 1, 255);
    }
  }
}

void BatmanPlugin::drawSkyline(uint8_t brightness)
{
  for (int x = 0; x < 16; x++)
  {
    int h = pgm_read_byte(&SKY_H[x]);
    for (int row = 0; row < h; row++)
    {
      int y = 15 - row;
      // Top edge of building slightly brighter
      uint8_t b = (row == h - 1)
                      ? (uint8_t)(brightness + 15 > 255 ? 255 : brightness + 15)
                      : brightness;
      Screen.setPixel(x, y, 1, b);
    }
  }
}

void BatmanPlugin::setup()
{
  Screen.clear();
  phase = 0;
  phaseStart = millis();
  frameTimer.forceReady();
}

void BatmanPlugin::loop()
{
  if (!frameTimer.isReady(50))
    return;

  Screen.clear();
  unsigned long elapsed = millis() - phaseStart;

  switch (phase)
  {
  case 0: // Bat signal with city skyline (5 sec)
  {
    float pulse = 0.7f + 0.3f * sinf(elapsed * 0.003f);
    drawBatSignal(pulse, 1.0f);
    drawSkyline(35);

    if (elapsed > 5000)
    {
      phase = 1;
      phaseStart = millis();
    }
    break;
  }

  case 1: // Batman drops + signal fades (900ms)
  {
    // Signal fades during first 600ms
    if (elapsed < 600)
    {
      float fade = 1.0f - elapsed / 600.0f;
      drawBatSignal(0.7f, fade);
      drawSkyline((uint8_t)(35.0f * fade));
    }

    // Batman drops (12 frames at ~70ms each)
    int dropIdx = (int)(elapsed / 75);
    if (dropIdx > 11)
      dropIdx = 11;
    int8_t yOff = (int8_t)pgm_read_byte(&DROP_Y[dropIdx]);
    drawBatmanFrame(BATMAN_A, yOff, 255, false);

    // Landing impact flash at ground line
    if (dropIdx >= 9 && dropIdx <= 10)
    {
      for (int x = 0; x < 16; x++)
        Screen.setPixel(x, 15, 1, 80);
    }

    if (elapsed > 950)
    {
      phase = 2;
      phaseStart = millis();
    }
    break;
  }

  case 2: // Standing pose with cape flutter (7 sec)
  {
    // Cape animation: A -> B -> A -> mirror(B) cycle
    int capeIdx = ((int)(elapsed / 600)) % 4;
    uint8_t seq = CAPE_SEQ[capeIdx];

    // Subtle breathing: brightness oscillates 225-255
    uint8_t breath = 230 + (uint8_t)(25.0f * sinf(elapsed * 0.002f));

    if (seq == 0)
      drawBatmanFrame(BATMAN_A, 0, breath, false);
    else if (seq == 1)
      drawBatmanFrame(BATMAN_B, 0, breath, false);
    else
      drawBatmanFrame(BATMAN_B, 0, breath, true); // mirror = cape left

    if (elapsed > 7000)
    {
      phase = 3;
      phaseStart = millis();
    }
    break;
  }

  case 3: // Fade out (1 sec)
  {
    float fade = fmaxf(0.0f, 1.0f - elapsed / 1000.0f);
    uint8_t scale = (uint8_t)(255.0f * fade);
    drawBatmanFrame(BATMAN_A, 0, scale, false);

    if (elapsed > 1000)
    {
      phase = 0;
      phaseStart = millis();
    }
    break;
  }
  }
}

const char *BatmanPlugin::getName() const
{
  return "Batman";
}
