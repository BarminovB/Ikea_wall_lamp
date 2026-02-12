#include "plugins/TetrisPlugin.h"

struct TBlock { int8_t x, y; };

// 7 piece types (I,O,T,S,Z,L,J), 4 rotations each, 4 blocks each
static const TBlock PIECES[7][4][4] = {
  // I
  { {{0,0},{1,0},{2,0},{3,0}}, {{0,0},{0,1},{0,2},{0,3}},
    {{0,0},{1,0},{2,0},{3,0}}, {{0,0},{0,1},{0,2},{0,3}} },
  // O
  { {{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}},
    {{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}} },
  // T
  { {{0,0},{1,0},{2,0},{1,1}}, {{1,0},{0,1},{1,1},{1,2}},
    {{1,0},{0,1},{1,1},{2,1}}, {{0,0},{0,1},{1,1},{0,2}} },
  // S
  { {{1,0},{2,0},{0,1},{1,1}}, {{0,0},{0,1},{1,1},{1,2}},
    {{1,0},{2,0},{0,1},{1,1}}, {{0,0},{0,1},{1,1},{1,2}} },
  // Z
  { {{0,0},{1,0},{1,1},{2,1}}, {{1,0},{0,1},{1,1},{0,2}},
    {{0,0},{1,0},{1,1},{2,1}}, {{1,0},{0,1},{1,1},{0,2}} },
  // L
  { {{0,0},{0,1},{0,2},{1,2}}, {{0,0},{1,0},{2,0},{0,1}},
    {{0,0},{1,0},{1,1},{1,2}}, {{2,0},{0,1},{1,1},{2,1}} },
  // J
  { {{1,0},{1,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{2,1}},
    {{0,0},{1,0},{0,1},{0,2}}, {{0,0},{1,0},{2,0},{2,1}} },
};

static const int NUM_ROTATIONS[7] = {2, 1, 4, 2, 2, 4, 4};
static const uint8_t PIECE_BRIGHT[7] = {255, 230, 210, 190, 170, 200, 180};

void TetrisPlugin::setup()
{
  Screen.clear();
  dropTimer.forceReady();
  newGame();
}

void TetrisPlugin::newGame()
{
  memset(board, 0, sizeof(board));
  state = PLAYING;
  spawnPiece();
}

void TetrisPlugin::spawnPiece()
{
  currentType = random(0, 7);
  chooseBestPlacement(currentType, currentRot, currentX);

  // Start piece so topmost block is at row 0
  int minY = 3;
  for (int i = 0; i < 4; i++)
  {
    if (PIECES[currentType][currentRot][i].y < minY)
      minY = PIECES[currentType][currentRot][i].y;
  }
  currentY = -minY;

  if (!canPlace(currentType, currentRot, currentX, currentY))
  {
    state = GAMEOVER;
    animCount = 0;
    gameOverRow = 0;
  }
}

bool TetrisPlugin::canPlace(int type, int rot, int x, int y)
{
  for (int i = 0; i < 4; i++)
  {
    int bx = x + PIECES[type][rot][i].x;
    int by = y + PIECES[type][rot][i].y;
    if (bx < 0 || bx >= FIELD_W || by >= FIELD_H)
      return false;
    if (by >= 0 && board[by][bx] != 0)
      return false;
  }
  return true;
}

int TetrisPlugin::findDropY(int type, int rot, int x)
{
  int minY = 3;
  for (int i = 0; i < 4; i++)
  {
    if (PIECES[type][rot][i].y < minY)
      minY = PIECES[type][rot][i].y;
  }
  int y = -minY;
  while (canPlace(type, rot, x, y + 1))
    y++;
  return y;
}

void TetrisPlugin::lockPiece()
{
  for (int i = 0; i < 4; i++)
  {
    int bx = currentX + PIECES[currentType][currentRot][i].x;
    int by = currentY + PIECES[currentType][currentRot][i].y;
    if (by >= 0 && by < FIELD_H && bx >= 0 && bx < FIELD_W)
      board[by][bx] = currentType + 1;
  }
}

bool TetrisPlugin::hasCompleteLines()
{
  for (int y = 0; y < FIELD_H; y++)
  {
    bool full = true;
    for (int x = 0; x < FIELD_W; x++)
    {
      if (board[y][x] == 0) { full = false; break; }
    }
    if (full) return true;
  }
  return false;
}

void TetrisPlugin::removeCompleteLines()
{
  for (int y = FIELD_H - 1; y >= 0; y--)
  {
    bool full = true;
    for (int x = 0; x < FIELD_W; x++)
    {
      if (board[y][x] == 0) { full = false; break; }
    }
    if (full)
    {
      for (int row = y; row > 0; row--)
        memcpy(board[row], board[row - 1], FIELD_W);
      memset(board[0], 0, FIELD_W);
      y++; // recheck this row
    }
  }
}

void TetrisPlugin::chooseBestPlacement(int type, int &outRot, int &outX)
{
  int bestScore = -99999;
  outRot = 0;
  outX = FIELD_W / 2 - 1;

  for (int rot = 0; rot < NUM_ROTATIONS[type]; rot++)
  {
    int minBX = 3, maxBX = 0;
    for (int i = 0; i < 4; i++)
    {
      if (PIECES[type][rot][i].x < minBX) minBX = PIECES[type][rot][i].x;
      if (PIECES[type][rot][i].x > maxBX) maxBX = PIECES[type][rot][i].x;
    }

    for (int x = -minBX; x <= FIELD_W - 1 - maxBX; x++)
    {
      int dropY = findDropY(type, rot, x);
      if (!canPlace(type, rot, x, dropY))
        continue;

      int score = evaluatePlacement(type, rot, x) + random(-15, 16);
      if (score > bestScore)
      {
        bestScore = score;
        outRot = rot;
        outX = x;
      }
    }
  }
}

int TetrisPlugin::evaluatePlacement(int type, int rot, int x)
{
  int y = findDropY(type, rot, x);

  // Temporarily place piece on a copy
  uint8_t temp[FIELD_H][FIELD_W];
  memcpy(temp, board, sizeof(board));

  for (int i = 0; i < 4; i++)
  {
    int bx = x + PIECES[type][rot][i].x;
    int by = y + PIECES[type][rot][i].y;
    if (by >= 0 && by < FIELD_H && bx >= 0 && bx < FIELD_W)
      temp[by][bx] = type + 1;
  }

  // Count complete lines
  int lines = 0;
  for (int row = 0; row < FIELD_H; row++)
  {
    bool full = true;
    for (int col = 0; col < FIELD_W; col++)
    {
      if (temp[row][col] == 0) { full = false; break; }
    }
    if (full) lines++;
  }

  // Column stats: height, holes, bumpiness
  int aggHeight = 0;
  int holes = 0;
  int bumpiness = 0;
  int prevH = 0;

  for (int col = 0; col < FIELD_W; col++)
  {
    int h = 0;
    bool foundBlock = false;
    for (int row = 0; row < FIELD_H; row++)
    {
      if (temp[row][col] != 0)
      {
        if (!foundBlock) { h = FIELD_H - row; foundBlock = true; }
      }
      else if (foundBlock)
      {
        holes++;
      }
    }
    aggHeight += h;
    if (col > 0) bumpiness += abs(h - prevH);
    prevH = h;
  }

  return lines * 100 - aggHeight - holes * 40 - bumpiness * 10;
}

void TetrisPlugin::drawBoard()
{
  Screen.clear();

  // Side borders
  for (int y = 0; y < FIELD_H; y++)
  {
    Screen.setPixel(OFFSET_X - 1, y, 1, 30);
    Screen.setPixel(OFFSET_X + FIELD_W, y, 1, 30);
  }

  // Placed blocks
  for (int y = 0; y < FIELD_H; y++)
  {
    for (int x = 0; x < FIELD_W; x++)
    {
      if (board[y][x] != 0)
        Screen.setPixel(OFFSET_X + x, y, 1, PIECE_BRIGHT[board[y][x] - 1]);
    }
  }

  // Falling piece
  if (state == PLAYING)
  {
    for (int i = 0; i < 4; i++)
    {
      int bx = currentX + PIECES[currentType][currentRot][i].x;
      int by = currentY + PIECES[currentType][currentRot][i].y;
      if (by >= 0 && by < FIELD_H && bx >= 0 && bx < FIELD_W)
        Screen.setPixel(OFFSET_X + bx, by, 1, PIECE_BRIGHT[currentType]);
    }
  }
}

void TetrisPlugin::loop()
{
  int interval;
  switch (state)
  {
  case GAMEOVER: interval = 40; break;
  case CLEARING: interval = 80; break;
  default: interval = 120; break;
  }

  if (!dropTimer.isReady(interval))
    return;

  switch (state)
  {
  case PLAYING:
    if (canPlace(currentType, currentRot, currentX, currentY + 1))
    {
      currentY++;
    }
    else
    {
      lockPiece();
      if (hasCompleteLines())
      {
        state = CLEARING;
        animCount = 0;
      }
      else
      {
        spawnPiece();
      }
    }
    drawBoard();
    break;

  case CLEARING:
    animCount++;
    drawBoard();
    // Flash complete lines
    for (int y = 0; y < FIELD_H; y++)
    {
      bool full = true;
      for (int x = 0; x < FIELD_W; x++)
      {
        if (board[y][x] == 0) { full = false; break; }
      }
      if (full)
      {
        uint8_t b = (animCount % 2 == 0) ? 255 : 0;
        for (int x = 0; x < FIELD_W; x++)
          Screen.setPixel(OFFSET_X + x, y, b ? 1 : 0, b);
      }
    }
    if (animCount >= 6)
    {
      removeCompleteLines();
      state = PLAYING;
      spawnPiece();
    }
    break;

  case GAMEOVER:
    animCount++;
    if (animCount <= 6)
    {
      // Flash board
      if (animCount % 2 == 0) drawBoard();
      else Screen.clear();
    }
    else
    {
      int fillRow = animCount - 7;
      if (fillRow < FIELD_H)
      {
        // Fill from top with bright pixels
        for (int x = 0; x < FIELD_W; x++)
          board[fillRow][x] = 1;
        drawBoard();
      }
      else if (fillRow < FIELD_H + 3)
      {
        // Brief pause
      }
      else
      {
        newGame();
      }
    }
    break;
  }
}

const char *TetrisPlugin::getName() const
{
  return "Tetris";
}
