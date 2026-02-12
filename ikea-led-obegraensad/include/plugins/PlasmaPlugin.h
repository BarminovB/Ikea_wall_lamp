#pragma once

#include "PluginManager.h"
#include "timing.h"

class PlasmaPlugin : public Plugin
{
private:
  float time_ = 0.0f;
  NonBlockingDelay frameTimer;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
