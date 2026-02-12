#include "plugins/PerlinNoisePlugin.h"
#include <math.h>

void PerlinNoisePlugin::initPermutation()
{
  for (int i = 0; i < 256; i++)
    perm[i] = i;
  for (int i = 255; i > 0; i--)
  {
    int j = random(i + 1);
    uint8_t tmp = perm[i];
    perm[i] = perm[j];
    perm[j] = tmp;
  }
}

float PerlinNoisePlugin::fade(float t)
{
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoisePlugin::lerp(float a, float b, float t)
{
  return a + t * (b - a);
}

float PerlinNoisePlugin::grad(int hash, float x, float y)
{
  int h = hash & 3;
  float u = h < 2 ? x : y;
  float v = h < 2 ? y : x;
  return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float PerlinNoisePlugin::noise2d(float x, float y)
{
  int xi = ((int)floorf(x)) & 255;
  int yi = ((int)floorf(y)) & 255;
  float xf = x - floorf(x);
  float yf = y - floorf(y);

  float u = fade(xf);
  float v = fade(yf);

  int aa = perm[(perm[xi] + yi) & 255];
  int ab = perm[(perm[xi] + yi + 1) & 255];
  int ba = perm[(perm[(xi + 1) & 255] + yi) & 255];
  int bb = perm[(perm[(xi + 1) & 255] + yi + 1) & 255];

  float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0f, yf), u);
  float x2 = lerp(grad(ab, xf, yf - 1.0f), grad(bb, xf - 1.0f, yf - 1.0f), u);

  return lerp(x1, x2, v);
}

void PerlinNoisePlugin::setup()
{
  Screen.clear();
  time_ = 0.0f;
  initPermutation();
  frameTimer.forceReady();
}

void PerlinNoisePlugin::loop()
{
  if (!frameTimer.isReady(50))
    return;

  float scale = 0.25f;
  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < 16; x++)
    {
      float n = noise2d(x * scale + time_, y * scale + time_ * 0.7f);
      // n is approximately in [-1, 1]
      uint8_t brightness = (uint8_t)constrain((int)((n + 1.0f) * 127.5f), 0, 255);
      Screen.setPixel(x, y, 1, brightness);
    }
  }

  time_ += 0.05f;
  if (time_ > 1000.0f)
    time_ = 0.0f;
}

const char *PerlinNoisePlugin::getName() const
{
  return "Perlin Noise";
}
