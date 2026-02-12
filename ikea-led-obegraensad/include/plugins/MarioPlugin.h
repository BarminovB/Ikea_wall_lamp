#pragma once

#include "PluginManager.h"
#include "timing.h"

class MarioPlugin : public Plugin
{
private:
  NonBlockingDelay frameTimer;
  int worldOffset = 0;
  int marioY = 7;
  int marioFrame = 0;
  int jumpPhase = -1; // -1 = not jumping, 0..JUMP_LEN-1 = arc
  int frameCount = 0;

  void drawSprite(const uint8_t *sprite, int rows, int x, int y,
                  int width, uint8_t brightness);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
