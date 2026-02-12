#pragma once

#include "PluginManager.h"
#include "timing.h"

class RotatingCubePlugin : public Plugin
{
private:
  float angleX = 0, angleY = 0, angleZ = 0;
  NonBlockingDelay frameTimer;

  void project(float x, float y, float z, int &sx, int &sy);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
