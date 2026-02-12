#pragma once

#include "PluginManager.h"
#include "timing.h"

class MarqueePlugin : public Plugin
{
private:
  char text[512] = {0};  // Initialize to zeros
  int speed = 50;
  int scrollPos = -16;
  NonBlockingDelay scrollTimer;

  uint16_t codepoints[200];
  int numChars = 0;
  int totalWidth = 0;

  void decodeText();
  void renderFrame();
  void drawGlyph(int x, int y, uint16_t codepoint);
  int glyphWidth(uint16_t codepoint);

public:
  void setup() override;
  void loop() override;
  void websocketHook(JsonDocument &request) override;
  const char *getName() const override;
};
