#pragma once

#include "PluginManager.h"
#include "timing.h"

class DNAHelixPlugin : public Plugin
{
private:
  float offset = 0.0f;
  NonBlockingDelay frameTimer;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
