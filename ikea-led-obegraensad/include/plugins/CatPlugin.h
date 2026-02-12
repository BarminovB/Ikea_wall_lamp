#pragma once
#include "PluginManager.h"
#include "timing.h"

class CatPlugin : public Plugin
{
private:
  NonBlockingDelay frameTimer;
  int frame;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
