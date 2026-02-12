#include "plugins/FlockingPlugin.h"
#include <math.h>

void FlockingPlugin::setup()
{
  Screen.clear();
  for (int i = 0; i < NUM_BOIDS; i++)
  {
    boids[i].x = random(0, 160) / 10.0f;
    boids[i].y = random(0, 160) / 10.0f;
    boids[i].vx = (random(-10, 11)) / 10.0f;
    boids[i].vy = (random(-10, 11)) / 10.0f;
  }
}

void FlockingPlugin::updateBoids()
{
  for (int i = 0; i < NUM_BOIDS; i++)
  {
    float cohX = 0, cohY = 0; // cohesion
    float sepX = 0, sepY = 0; // separation
    float aliX = 0, aliY = 0; // alignment
    int neighbors = 0;

    for (int j = 0; j < NUM_BOIDS; j++)
    {
      if (i == j)
        continue;

      float dx = boids[j].x - boids[i].x;
      float dy = boids[j].y - boids[i].y;
      float dist = sqrtf(dx * dx + dy * dy);

      if (dist < 6.0f)
      {
        cohX += boids[j].x;
        cohY += boids[j].y;
        aliX += boids[j].vx;
        aliY += boids[j].vy;
        neighbors++;

        if (dist < 2.0f)
        {
          sepX -= dx;
          sepY -= dy;
        }
      }
    }

    if (neighbors > 0)
    {
      // Cohesion: steer toward center of neighbors
      cohX = (cohX / neighbors - boids[i].x) * 0.02f;
      cohY = (cohY / neighbors - boids[i].y) * 0.02f;

      // Alignment: match velocity
      aliX = (aliX / neighbors - boids[i].vx) * 0.05f;
      aliY = (aliY / neighbors - boids[i].vy) * 0.05f;

      // Separation
      sepX *= 0.1f;
      sepY *= 0.1f;
    }

    boids[i].vx += cohX + sepX + aliX;
    boids[i].vy += cohY + sepY + aliY;

    // Limit speed
    float speed = sqrtf(boids[i].vx * boids[i].vx + boids[i].vy * boids[i].vy);
    if (speed > 1.2f)
    {
      boids[i].vx = (boids[i].vx / speed) * 1.2f;
      boids[i].vy = (boids[i].vy / speed) * 1.2f;
    }

    // Boundary avoidance
    if (boids[i].x < 1.0f) boids[i].vx += 0.3f;
    if (boids[i].x > 14.0f) boids[i].vx -= 0.3f;
    if (boids[i].y < 1.0f) boids[i].vy += 0.3f;
    if (boids[i].y > 14.0f) boids[i].vy -= 0.3f;

    boids[i].x += boids[i].vx;
    boids[i].y += boids[i].vy;

    // Hard clamp
    boids[i].x = constrain(boids[i].x, 0.0f, 15.0f);
    boids[i].y = constrain(boids[i].y, 0.0f, 15.0f);
  }
}

void FlockingPlugin::loop()
{
  if (!frameTimer.isReady(80))
    return;

  // Fade existing pixels
  uint8_t *buf = Screen.getRenderBuffer();
  for (int i = 0; i < 256; i++)
  {
    if (buf[i] > 30)
      buf[i] -= 30;
    else
      buf[i] = 0;
  }

  updateBoids();

  for (int i = 0; i < NUM_BOIDS; i++)
  {
    int px = (int)(boids[i].x + 0.5f);
    int py = (int)(boids[i].y + 0.5f);
    if (px >= 0 && px < 16 && py >= 0 && py < 16)
    {
      Screen.setPixel(px, py, 1, 255);
    }
  }
}

const char *FlockingPlugin::getName() const
{
  return "Flocking";
}
