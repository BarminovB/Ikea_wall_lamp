#include "plugins/SandPlugin.h"

void SandPlugin::setup()
{
  Screen.clear();
  memset(grid, 0, sizeof(grid));
}

void SandPlugin::spawnGrain()
{
  int x = random(0, 16);
  if (grid[0][x] == 0)
  {
    grid[0][x] = random(128, 256);
  }
}

void SandPlugin::simulate()
{
  // Process from bottom to top so grains fall correctly
  for (int y = 14; y >= 0; y--)
  {
    for (int x = 0; x < 16; x++)
    {
      if (grid[y][x] == 0)
        continue;

      uint8_t val = grid[y][x];

      // Try to fall straight down
      if (grid[y + 1][x] == 0)
      {
        grid[y + 1][x] = val;
        grid[y][x] = 0;
      }
      else
      {
        // Try diagonal
        bool canLeft = (x > 0 && grid[y + 1][x - 1] == 0);
        bool canRight = (x < 15 && grid[y + 1][x + 1] == 0);

        if (canLeft && canRight)
        {
          int dir = random(0, 2) ? -1 : 1;
          grid[y + 1][x + dir] = val;
          grid[y][x] = 0;
        }
        else if (canLeft)
        {
          grid[y + 1][x - 1] = val;
          grid[y][x] = 0;
        }
        else if (canRight)
        {
          grid[y + 1][x + 1] = val;
          grid[y][x] = 0;
        }
        // else: grain is stuck, stay in place
      }
    }
  }
}

void SandPlugin::loop()
{
  if (spawnTimer.isReady(100))
  {
    spawnGrain();
  }

  if (!frameTimer.isReady(60))
    return;

  simulate();

  // Check if screen is mostly full, reset
  int count = 0;
  for (int y = 0; y < 3; y++)
    for (int x = 0; x < 16; x++)
      if (grid[y][x] > 0)
        count++;

  if (count > 40)
  {
    memset(grid, 0, sizeof(grid));
    Screen.clear();
    return;
  }

  // Render
  Screen.clear();
  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < 16; x++)
    {
      if (grid[y][x] > 0)
      {
        Screen.setPixel(x, y, 1, grid[y][x]);
      }
    }
  }
}

const char *SandPlugin::getName() const
{
  return "Sand";
}
