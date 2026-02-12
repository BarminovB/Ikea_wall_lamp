#pragma once
#include "PluginManager.h"
#include "timing.h"

class DinoPlugin : public Plugin
{
private:
  NonBlockingDelay frameTimer;

  // Dino state
  int jumpOffset;
  bool isJumping;
  int jumpTick;
  int runFrame;
  int frameTick;

  // Obstacles
  struct Cactus
  {
    int x;
    int h;
    int w;
  };
  static constexpr int MAX_CACTI = 3;
  Cactus cacti[MAX_CACTI];
  int numCacti;
  int spawnTimer;

  void drawDino(int bx, int by);
  void drawCactus(const Cactus &c);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
