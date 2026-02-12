#pragma once

#include "PluginManager.h"
#include "timing.h"

class MazePlugin : public Plugin
{
private:
  // Maze is 8x8 cells, each cell is 2x2 pixels
  static const int MAZE_W = 8;
  static const int MAZE_H = 8;
  uint8_t walls[MAZE_W][MAZE_H]; // bitmask: 1=N, 2=E, 4=S, 8=W
  bool visited[MAZE_W][MAZE_H];

  // Stack for generation
  struct Cell { int x, y; };
  Cell stack[64];
  int stackTop;

  // Solve path
  Cell solvePath[64];
  int solveLen;
  int solveIdx;

  enum State { GENERATING, SOLVING, DONE };
  State state;
  NonBlockingDelay stepTimer;
  NonBlockingDelay restartTimer;

  void initMaze();
  bool generateStep();
  void solveMaze();
  void drawMaze();
  void drawSolveStep();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
