#pragma once

#include "PluginManager.h"
#include "timing.h"

struct Droplet
{
  float x, y;
  float radius;
  float maxRadius;
  bool active;
};

class DropletPlugin : public Plugin
{
private:
  static const int MAX_DROPLETS = 5;
  Droplet droplets[MAX_DROPLETS];
  NonBlockingDelay frameTimer;
  NonBlockingDelay spawnTimer;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
