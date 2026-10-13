#include "TeapotModel.h"
#include "TeapotData.h"

#include <math.h>

void TeapotModel::basis(float t, float b[4], float d[4]) {
  float s = 1.0f - t;
  b[0] = s*s*s; b[1] = 3.0f*t*s*s; b[2] = 3.0f*t*t*s; b[3] = t*t*t;
  d[0] = -3.0f*s*s;
  d[1] = 3.0f*s*s - 6.0f*t*s;
  d[2] = 6.0f*t*s - 3.0f*t*t;
  d[3] = 3.0f*t*t;
}

TeapotModel::Vec3 TeapotModel::controlPoint(uint8_t patch, uint8_t instance,
                                             uint8_t row, uint8_t col) {
  uint8_t index = pgm_read_byte(&TEAPOT_PATCHES[patch][row * 4 + col]);
  Vec3 p = {pgm_read_float(&TEAPOT_POINTS[index][0]),
            pgm_read_float(&TEAPOT_POINTS[index][1]),
            pgm_read_float(&TEAPOT_POINTS[index][2])};
  if (patch < 6) {
    switch (instance & 3) {
      case 1: { float x = p.x; p.x = -p.y; p.y = x; break; }
      case 2: p.x = -p.x; p.y = -p.y; break;
      case 3: { float x = p.x; p.x = p.y; p.y = -x; break; }
    }
  } else if (instance == 1) {
    p.y = -p.y;
  }
  return p;
}

TeapotModel::Sample TeapotModel::evaluate(uint8_t patch, uint8_t instance,
                                           float u, float v) {
  float bu[4], bv[4], du[4], dv[4];
  basis(u, bu, du); basis(v, bv, dv);
  Vec3 p = {}, tu = {}, tv = {};
  for (uint8_t r = 0; r < 4; ++r) for (uint8_t c = 0; c < 4; ++c) {
    Vec3 cp = controlPoint(patch, instance, r, c);
    float w = bu[r] * bv[c], wu = du[r] * bv[c], wv = bu[r] * dv[c];
    p.x += cp.x*w; p.y += cp.y*w; p.z += cp.z*w;
    tu.x += cp.x*wu; tu.y += cp.y*wu; tu.z += cp.z*wu;
    tv.x += cp.x*wv; tv.y += cp.y*wv; tv.z += cp.z*wv;
  }
  Vec3 n = {tu.y*tv.z - tu.z*tv.y,
            tu.z*tv.x - tu.x*tv.z,
            tu.x*tv.y - tu.y*tv.x};
  float length = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
  if (length < 1.0e-6f) n = {0.0f, 0.0f, 1.0f};
  else { n.x /= length; n.y /= length; n.z /= length; }
  return {p, n};
}

void TeapotModel::emit(MiniMesa32 &gl, const Sample &s) {
  gl.glNormal3f(s.normal.x, s.normal.y, s.normal.z);
  gl.glVertex3f(s.position.x, s.position.y, s.position.z);
}

void TeapotModel::draw(MiniMesa32 &gl) const {
  const uint8_t n = constrain(tessellation_, 2, 6);
  Sample grid[7][7];
  gl.glBegin(MM_TRIANGLES);
  for (uint8_t patch = 0; patch < 10; ++patch) {
    uint8_t instances = patch < 6 ? 4 : 2;
    for (uint8_t instance = 0; instance < instances; ++instance) {
      for (uint8_t y = 0; y <= n; ++y)
        for (uint8_t x = 0; x <= n; ++x)
          grid[y][x] = evaluate(patch, instance, (float)y/n, (float)x/n);
      for (uint8_t y = 0; y < n; ++y) for (uint8_t x = 0; x < n; ++x) {
        emit(gl, grid[y][x]); emit(gl, grid[y+1][x]); emit(gl, grid[y+1][x+1]);
        emit(gl, grid[y][x]); emit(gl, grid[y+1][x+1]); emit(gl, grid[y][x+1]);
      }
    }
  }
  gl.glEnd();
}
