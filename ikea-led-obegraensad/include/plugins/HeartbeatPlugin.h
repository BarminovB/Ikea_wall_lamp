#pragma once

#include "PluginManager.h"
#include "timing.h"

class HeartbeatPlugin : public Plugin
{
private:
  unsigned long beatStart = 0;
  NonBlockingDelay frameTimer;

  void drawHeart(int offsetX, int offsetY, uint8_t brightness);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
