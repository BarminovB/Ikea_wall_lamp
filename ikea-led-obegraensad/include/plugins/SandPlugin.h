#pragma once

#include "PluginManager.h"
#include "timing.h"

class SandPlugin : public Plugin
{
private:
  uint8_t grid[16][16];
  NonBlockingDelay frameTimer;
  NonBlockingDelay spawnTimer;

  void spawnGrain();
  void simulate();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
