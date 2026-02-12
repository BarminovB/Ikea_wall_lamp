#pragma once

#include "PluginManager.h"
#include "timing.h"

struct MetaBall
{
  float x, y;
  float vx, vy;
  float radius;
};

class LavaLampPlugin : public Plugin
{
private:
  static const int NUM_BALLS = 4;
  MetaBall balls[NUM_BALLS];
  NonBlockingDelay frameTimer;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
