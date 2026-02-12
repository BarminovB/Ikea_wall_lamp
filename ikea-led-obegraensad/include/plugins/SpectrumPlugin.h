#pragma once

#include "PluginManager.h"
#include "timing.h"

class SpectrumPlugin : public Plugin
{
private:
  float levels[16];
  float targets[16];
  float peaks[16];
  NonBlockingDelay frameTimer;
  NonBlockingDelay targetTimer;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
