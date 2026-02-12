#pragma once

#include "PluginManager.h"
#include "timing.h"

struct Boid
{
  float x, y;
  float vx, vy;
};

class FlockingPlugin : public Plugin
{
private:
  static const int NUM_BOIDS = 10;
  Boid boids[NUM_BOIDS];
  NonBlockingDelay frameTimer;

  void updateBoids();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
