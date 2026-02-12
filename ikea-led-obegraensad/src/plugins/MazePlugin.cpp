#include "plugins/MazePlugin.h"

void MazePlugin::initMaze()
{
  memset(walls, 0x0F, sizeof(walls));  // all walls present
  memset(visited, false, sizeof(visited));
  stackTop = 0;
  solveLen = 0;
  solveIdx = 0;

  // Start from top-left
  int sx = 0, sy = 0;
  visited[sy][sx] = true;
  stack[stackTop++] = {sx, sy};
  state = GENERATING;
}

bool MazePlugin::generateStep()
{
  if (stackTop <= 0)
    return false;

  Cell current = stack[stackTop - 1];
  int cx = current.x, cy = current.y;

  // Find unvisited neighbors
  int dirs[4];
  int numDirs = 0;
  // N
  if (cy > 0 && !visited[cy - 1][cx]) dirs[numDirs++] = 0;
  // E
  if (cx < MAZE_W - 1 && !visited[cy][cx + 1]) dirs[numDirs++] = 1;
  // S
  if (cy < MAZE_H - 1 && !visited[cy + 1][cx]) dirs[numDirs++] = 2;
  // W
  if (cx > 0 && !visited[cy][cx - 1]) dirs[numDirs++] = 3;

  if (numDirs == 0)
  {
    stackTop--;
    return true;
  }

  int dir = dirs[random(0, numDirs)];
  int nx = cx, ny = cy;
  int wallBit = 0, oppBit = 0;

  switch (dir)
  {
  case 0: ny--; wallBit = 1; oppBit = 4; break; // N
  case 1: nx++; wallBit = 2; oppBit = 8; break; // E
  case 2: ny++; wallBit = 4; oppBit = 1; break; // S
  case 3: nx--; wallBit = 8; oppBit = 2; break; // W
  }

  // Remove walls
  walls[cy][cx] &= ~wallBit;
  walls[ny][nx] &= ~oppBit;

  visited[ny][nx] = true;
  stack[stackTop++] = {nx, ny};

  return true;
}

void MazePlugin::solveMaze()
{
  // Simple BFS from (0,0) to (MAZE_W-1, MAZE_H-1)
  bool vis[MAZE_W][MAZE_H];
  memset(vis, false, sizeof(vis));
  Cell parent[MAZE_W][MAZE_H];

  Cell queue[64];
  int qHead = 0, qTail = 0;

  queue[qTail++] = {0, 0};
  vis[0][0] = true;
  parent[0][0] = {-1, -1};

  int dx[] = {0, 1, 0, -1};
  int dy[] = {-1, 0, 1, 0};
  int wallBits[] = {1, 2, 4, 8};

  while (qHead < qTail)
  {
    Cell c = queue[qHead++];

    if (c.x == MAZE_W - 1 && c.y == MAZE_H - 1)
    {
      // Reconstruct path
      solveLen = 0;
      Cell p = c;
      while (p.x >= 0)
      {
        solvePath[solveLen++] = p;
        p = parent[p.y][p.x];
      }
      // Reverse
      for (int i = 0; i < solveLen / 2; i++)
      {
        Cell tmp = solvePath[i];
        solvePath[i] = solvePath[solveLen - 1 - i];
        solvePath[solveLen - 1 - i] = tmp;
      }
      solveIdx = 0;
      return;
    }

    for (int d = 0; d < 4; d++)
    {
      if (walls[c.y][c.x] & wallBits[d])
        continue;

      int nx = c.x + dx[d];
      int ny = c.y + dy[d];
      if (nx < 0 || nx >= MAZE_W || ny < 0 || ny >= MAZE_H || vis[ny][nx])
        continue;

      vis[ny][nx] = true;
      parent[ny][nx] = c;
      queue[qTail++] = {nx, ny};
    }
  }
}

void MazePlugin::drawMaze()
{
  Screen.clear();
  for (int my = 0; my < MAZE_H; my++)
  {
    for (int mx = 0; mx < MAZE_W; mx++)
    {
      int px = mx * 2;
      int py = my * 2;

      // Cell center is always open (off)
      // Draw walls as lit pixels
      if (walls[my][mx] & 1) // North
        Screen.setPixel(px, py, 1, 80);
      if (walls[my][mx] & 2) // East
        Screen.setPixel(px + 1, py, 1, 80);
      if (walls[my][mx] & 4) // South
        Screen.setPixel(px, py + 1, 1, 80);
      if (walls[my][mx] & 8) // West
      {
        if (px > 0)
          Screen.setPixel(px - 1, py, 1, 80);
      }

      // Corner posts
      if (mx < MAZE_W - 1 && my < MAZE_H - 1)
        Screen.setPixel(px + 1, py + 1, 1, 80);
    }
  }
}

void MazePlugin::drawSolveStep()
{
  if (solveIdx < solveLen)
  {
    Cell c = solvePath[solveIdx];
    int px = c.x * 2;
    int py = c.y * 2;
    Screen.setPixel(px, py, 1, 255);
    solveIdx++;
  }
}

void MazePlugin::setup()
{
  Screen.clear();
  stepTimer.forceReady();
  initMaze();
}

void MazePlugin::loop()
{
  if (!stepTimer.isReady(50))
    return;

  switch (state)
  {
  case GENERATING:
    // Do multiple steps per frame for speed
    for (int i = 0; i < 3; i++)
    {
      if (!generateStep())
      {
        drawMaze();
        solveMaze();
        state = SOLVING;
        break;
      }
    }
    if (state == GENERATING)
      drawMaze();
    break;

  case SOLVING:
    drawSolveStep();
    if (solveIdx >= solveLen)
    {
      state = DONE;
      restartTimer.reset();
    }
    break;

  case DONE:
    // Wait 3 seconds then restart
    if (restartTimer.isReady(3000))
    {
      initMaze();
    }
    break;
  }
}

const char *MazePlugin::getName() const
{
  return "Maze";
}
