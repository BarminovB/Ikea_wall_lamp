#pragma once

#include "PluginManager.h"
#include "timing.h"

class BatmanPlugin : public Plugin
{
private:
  NonBlockingDelay frameTimer;
  int phase = 0;
  unsigned long phaseStart = 0;

  void drawBatSignal(float pulse, float fade);
  void drawSkyline(uint8_t brightness);
  void drawBatmanFrame(const uint8_t *frame, int yOff, uint8_t scale, bool mirror);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
