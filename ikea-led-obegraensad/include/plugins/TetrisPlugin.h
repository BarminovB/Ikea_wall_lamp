#pragma once

#include "PluginManager.h"
#include "timing.h"

class TetrisPlugin : public Plugin
{
private:
  static const int FIELD_W = 10;
  static const int FIELD_H = 16;
  static const int OFFSET_X = 3;

  uint8_t board[FIELD_H][FIELD_W];

  int currentType;
  int currentRot;
  int currentX, currentY;

  enum State { PLAYING, CLEARING, GAMEOVER };
  State state;
  int animCount;
  int gameOverRow;

  NonBlockingDelay dropTimer;

  void newGame();
  void spawnPiece();
  bool canPlace(int type, int rot, int x, int y);
  int findDropY(int type, int rot, int x);
  void lockPiece();
  bool hasCompleteLines();
  void removeCompleteLines();
  void chooseBestPlacement(int type, int &outRot, int &outX);
  int evaluatePlacement(int type, int rot, int x);
  void drawBoard();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
