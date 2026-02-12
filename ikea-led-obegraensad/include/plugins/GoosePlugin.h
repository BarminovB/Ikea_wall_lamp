#pragma once

#include "PluginManager.h"
#include "timing.h"

class GoosePlugin : public Plugin
{
private:
  NonBlockingDelay frameTimer;
  int phase = 0;
  unsigned long phaseStart = 0;
  int gooseX = -12;
  int walkFrame = 0;

  void drawSprite(const uint8_t *sprite, int sprW, int sprH,
                  int xPos, int yPos, uint8_t brightnessScale = 255);
  void drawExclamation(int x, int y, uint8_t brightness);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
