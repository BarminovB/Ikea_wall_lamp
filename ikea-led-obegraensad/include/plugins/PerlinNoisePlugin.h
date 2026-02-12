#pragma once

#include "PluginManager.h"
#include "timing.h"

class PerlinNoisePlugin : public Plugin
{
private:
  float time_ = 0.0f;
  uint8_t perm[256];
  NonBlockingDelay frameTimer;

  float fade(float t);
  float lerp(float a, float b, float t);
  float grad(int hash, float x, float y);
  float noise2d(float x, float y);
  void initPermutation();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
