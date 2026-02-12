#include "plugins/RotatingCubePlugin.h"
#include <math.h>

// Cube vertices (centered at origin, size 1)
static const float cubeVerts[8][3] = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    {-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1}};

// 12 edges as pairs of vertex indices
static const int cubeEdges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, // front face
    {4, 5}, {5, 6}, {6, 7}, {7, 4}, // back face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  // connecting edges
};

void RotatingCubePlugin::setup()
{
  Screen.clear();
  angleX = 0.3f;
  angleY = 0.5f;
  angleZ = 0.0f;
  frameTimer.forceReady();
}

void RotatingCubePlugin::loop()
{
  if (!frameTimer.isReady(50))
    return;

  Screen.clear();

  // Precompute trig values (48 -> 6 calls)
  float cx = cosf(angleX), sx_r = sinf(angleX);
  float cy = cosf(angleY), sy_r = sinf(angleY);
  float cz = cosf(angleZ), sz = sinf(angleZ);

  // Project all vertices
  int projected[8][2];
  for (int i = 0; i < 8; i++)
  {
    float x = cubeVerts[i][0];
    float y = cubeVerts[i][1];
    float z = cubeVerts[i][2];

    // Rotate around X axis
    float y1 = y * cx - z * sx_r;
    float z1 = y * sx_r + z * cx;

    // Rotate around Y axis
    float x2 = x * cy + z1 * sy_r;
    float z2 = -x * sy_r + z1 * cy;

    // Rotate around Z axis
    float x3 = x2 * cz - y1 * sz;
    float y3 = x2 * sz + y1 * cz;

    // Perspective projection
    float dist = 4.0f + z2;
    if (dist < 0.5f) dist = 0.5f;

    int px = (int)(x3 * 16.0f / dist + 7.5f);
    int py = (int)(y3 * 16.0f / dist + 7.5f);

    // Clamp to avoid excessive drawLine iterations for off-screen points
    if (px < -8) px = -8;
    if (px > 23) px = 23;
    if (py < -8) py = -8;
    if (py > 23) py = 23;

    projected[i][0] = px;
    projected[i][1] = py;
  }

  // Draw edges
  for (int i = 0; i < 12; i++)
  {
    int v0 = cubeEdges[i][0];
    int v1 = cubeEdges[i][1];
    Screen.drawLine(projected[v0][0], projected[v0][1],
                    projected[v1][0], projected[v1][1], 1, 200);
  }

  // Draw vertices as brighter points
  for (int i = 0; i < 8; i++)
  {
    int px = projected[i][0];
    int py = projected[i][1];
    if (px >= 0 && px < 16 && py >= 0 && py < 16)
    {
      Screen.setPixel(px, py, 1, 255);
    }
  }

  angleX += 0.03f;
  angleY += 0.05f;
  angleZ += 0.02f;

  if (angleX > 6.28f) angleX -= 6.28f;
  if (angleY > 6.28f) angleY -= 6.28f;
  if (angleZ > 6.28f) angleZ -= 6.28f;
}

const char *RotatingCubePlugin::getName() const
{
  return "3D Cube";
}
