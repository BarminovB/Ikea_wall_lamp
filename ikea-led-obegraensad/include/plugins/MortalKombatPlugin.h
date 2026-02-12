#pragma once
#include "PluginManager.h"
#include "timing.h"

class MortalKombatPlugin : public Plugin
{
private:
  NonBlockingDelay frameTimer;
  float angle;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
