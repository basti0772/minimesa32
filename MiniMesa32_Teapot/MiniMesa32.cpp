#include "MiniMesa32.h"

#include <math.h>
#include <string.h>

MiniMesa32::MiniMesa32(TFT_eSPI &display) : tft_(display) {
  identity(modelView_);
  identity(projection_);
}

void MiniMesa32::identity(float *m) {
  memset(m, 0, sizeof(float) * 16);
  m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void MiniMesa32::multiply(float *out, const float *a, const float *b) {
  float r[16];
  for (int c = 0; c < 4; ++c) {
    for (int row = 0; row < 4; ++row) {
      r[c * 4 + row] = 0.0f;
      for (int k = 0; k < 4; ++k) r[c * 4 + row] += a[k * 4 + row] * b[c * 4 + k];
    }
  }
  memcpy(out, r, sizeof(r));
}

float *MiniMesa32::activeMatrix() {
  return matrixMode_ == MM_MODELVIEW ? modelView_ : projection_;
}

void MiniMesa32::postMultiply(const float *rhs) {
  float *m = activeMatrix();
  multiply(m, m, rhs);
}

void MiniMesa32::beginFrame(uint16_t clearColor) {
  width_ = tft_.width();
  height_ = tft_.height();
  clearColor_ = clearColor;
  triangleCount_ = 0;
  pendingCount_ = 0;
}

void MiniMesa32::glMatrixMode(MMMatrixMode mode) { matrixMode_ = mode; }
void MiniMesa32::glLoadIdentity() { identity(activeMatrix()); }

void MiniMesa32::glTranslatef(float x, float y, float z) {
  float m[16]; identity(m);
  m[12] = x; m[13] = y; m[14] = z;
  postMultiply(m);
}

void MiniMesa32::glScalef(float x, float y, float z) {
  float m[16]; identity(m);
  m[0] = x; m[5] = y; m[10] = z;
  postMultiply(m);
}

void MiniMesa32::glRotatef(float degrees, float x, float y, float z) {
  float length = sqrtf(x*x + y*y + z*z);
  if (length < 1.0e-6f) return;
  x /= length; y /= length; z /= length;
  float a = degrees * DEG_TO_RAD;
  float c = cosf(a), s = sinf(a), t = 1.0f - c;
  float m[16]; identity(m);
  m[0] = t*x*x + c;     m[4] = t*x*y - s*z; m[8]  = t*x*z + s*y;
  m[1] = t*x*y + s*z;   m[5] = t*y*y + c;   m[9]  = t*y*z - s*x;
  m[2] = t*x*z - s*y;   m[6] = t*y*z + s*x; m[10] = t*z*z + c;
  postMultiply(m);
}

void MiniMesa32::gluPerspective(float fovyDegrees, float aspect, float nearZ, float farZ) {
  float f = 1.0f / tanf(fovyDegrees * DEG_TO_RAD * 0.5f);
  float m[16] = {};
  m[0] = f / aspect;
  m[5] = f;
  m[10] = (farZ + nearZ) / (nearZ - farZ);
  m[11] = -1.0f;
  m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
  postMultiply(m);
}

void MiniMesa32::glColor565(uint16_t color) { drawColor_ = color; }
void MiniMesa32::glBegin(MMPrimitive) { pendingCount_ = 0; }
void MiniMesa32::glNormal3f(float x, float y, float z) { normal_[0] = x; normal_[1] = y; normal_[2] = z; }

MiniMesa32::ScreenVertex MiniMesa32::transformVertex(float x, float y, float z) const {
  float eye[4];
  const float p[4] = {x, y, z, 1.0f};
  for (int row = 0; row < 4; ++row) {
    eye[row] = 0.0f;
    for (int k = 0; k < 4; ++k) eye[row] += modelView_[k * 4 + row] * p[k];
  }
  float clip[4];
  for (int row = 0; row < 4; ++row) {
    clip[row] = 0.0f;
    for (int k = 0; k < 4; ++k) clip[row] += projection_[k * 4 + row] * eye[k];
  }
  float w = fabsf(clip[3]) < 1.0e-6f ? 1.0e-6f : clip[3];
  float ndcX = clip[0] / w, ndcY = clip[1] / w, ndcZ = clip[2] / w;

  float nx = modelView_[0]*normal_[0] + modelView_[4]*normal_[1] + modelView_[8]*normal_[2];
  float ny = modelView_[1]*normal_[0] + modelView_[5]*normal_[1] + modelView_[9]*normal_[2];
  float nz = modelView_[2]*normal_[0] + modelView_[6]*normal_[1] + modelView_[10]*normal_[2];
  float nl = sqrtf(nx*nx + ny*ny + nz*nz);
  if (nl > 1.0e-6f) { nx /= nl; ny /= nl; nz /= nl; }
  const float lx = -0.35f, ly = -0.45f, lz = 0.82f;
  float diffuse = fabsf(nx*lx + ny*ly + nz*lz);
  float light = 0.20f + 0.80f * diffuse;

  return {(ndcX * 0.5f + 0.5f) * (width_ - 1),
          (0.5f - ndcY * 0.5f) * (height_ - 1), ndcZ, light};
}

void MiniMesa32::glVertex3f(float x, float y, float z) {
  if (triangleCount_ >= MAX_TRIANGLES) return;
  pending_[pendingCount_++] = transformVertex(x, y, z);
  if (pendingCount_ == 3) {
    triangles_[triangleCount_].v[0] = pending_[0];
    triangles_[triangleCount_].v[1] = pending_[1];
    triangles_[triangleCount_].v[2] = pending_[2];
    triangles_[triangleCount_].color = drawColor_;
    ++triangleCount_;
    pendingCount_ = 0;
  }
}

void MiniMesa32::glEnd() { pendingCount_ = 0; }

uint16_t MiniMesa32::shade(uint16_t color, float light) const {
  light = constrain(light, 0.0f, 1.0f);
  int r = ((color >> 11) & 31) * light;
  int g = ((color >> 5) & 63) * light;
  int b = (color & 31) * light;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

float MiniMesa32::edge(const ScreenVertex &a, const ScreenVertex &b, float x, float y) {
  return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

void MiniMesa32::rasterTriangle(const Triangle &t, int bandY, int bandRows) {
  const ScreenVertex &a = t.v[0], &b = t.v[1], &c = t.v[2];
  float area = edge(a, b, c.x, c.y);
  if (fabsf(area) < 0.01f) return;
  int minX = max(0, (int)floorf(min(a.x, min(b.x, c.x))));
  int maxX = min(width_ - 1, (int)ceilf(max(a.x, max(b.x, c.x))));
  int minY = max(bandY, (int)floorf(min(a.y, min(b.y, c.y))));
  int maxY = min(bandY + bandRows - 1, (int)ceilf(max(a.y, max(b.y, c.y))));
  if (minX > maxX || minY > maxY) return;
  float invArea = 1.0f / area;
  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      float px = x + 0.5f, py = y + 0.5f;
      float w0 = edge(b, c, px, py) * invArea;
      float w1 = edge(c, a, px, py) * invArea;
      float w2 = 1.0f - w0 - w1;
      if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;
      int index = (y - bandY) * width_ + x;
      float depth = w0*a.z + w1*b.z + w2*c.z;
      if (depth >= depthBand_[index]) continue;
      depthBand_[index] = depth;
      float light = w0*a.light + w1*b.light + w2*c.light;
      colorBand_[index] = shade(t.color, light);
    }
  }
}

void MiniMesa32::present(int reservedWidth, int reservedHeight) {
  reservedWidth = constrain(reservedWidth, 0, width_);
  reservedHeight = constrain(reservedHeight, 0, height_);
  for (int bandY = 0; bandY < height_; bandY += BAND_HEIGHT) {
    int rows = min(BAND_HEIGHT, height_ - bandY);
    int pixels = width_ * rows;
    for (int i = 0; i < pixels; ++i) {
      colorBand_[i] = clearColor_;
      depthBand_[i] = 2.0f;
    }
    for (int i = 0; i < triangleCount_; ++i) rasterTriangle(triangles_[i], bandY, rows);
    int protectedRows = reservedWidth > 0
        ? constrain(reservedHeight - bandY, 0, rows) : 0;
    if (protectedRows > 0 && reservedWidth < width_) {
      // Pack only the right-hand portion into contiguous rows for pushImage.
      // Packing forward cannot overwrite a later source row or the full-width
      // rows below the reserved rectangle.
      int visibleWidth = width_ - reservedWidth;
      for (int row = 0; row < protectedRows; ++row) {
        memmove(colorBand_ + row * visibleWidth,
                colorBand_ + row * width_ + reservedWidth,
                visibleWidth * sizeof(uint16_t));
      }
      tft_.pushImage(reservedWidth, bandY, visibleWidth, protectedRows, colorBand_);
    }
    if (protectedRows < rows) {
      tft_.pushImage(0, bandY + protectedRows, width_, rows - protectedRows,
                     colorBand_ + protectedRows * width_);
    }
  }
}
