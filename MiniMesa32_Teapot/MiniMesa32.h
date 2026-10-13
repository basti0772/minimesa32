#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

enum MMMatrixMode : uint8_t { MM_MODELVIEW, MM_PROJECTION };
enum MMPrimitive : uint8_t { MM_TRIANGLES };

class MiniMesa32 {
 public:
  static constexpr int MAX_TRIANGLES = 1152;
  static constexpr int BAND_HEIGHT = 16;

  explicit MiniMesa32(TFT_eSPI &display);

  void beginFrame(uint16_t clearColor);
  void glMatrixMode(MMMatrixMode mode);
  void glLoadIdentity();
  void glTranslatef(float x, float y, float z);
  void glRotatef(float degrees, float x, float y, float z);
  void glScalef(float x, float y, float z);
  void gluPerspective(float fovyDegrees, float aspect, float nearZ, float farZ);
  void glColor565(uint16_t color);
  void glBegin(MMPrimitive primitive);
  void glNormal3f(float x, float y, float z);
  void glVertex3f(float x, float y, float z);
  void glEnd();
  // Preserve a static UI rectangle at the top left of the display.
  void present(int reservedWidth = 0, int reservedHeight = 0);

  int triangleCount() const { return triangleCount_; }

 private:
  struct ScreenVertex { float x, y, z, light; };
  struct Triangle { ScreenVertex v[3]; uint16_t color; };

  TFT_eSPI &tft_;
  int width_ = 320;
  int height_ = 240;
  uint16_t clearColor_ = TFT_BLACK;
  uint16_t drawColor_ = TFT_ORANGE;
  MMMatrixMode matrixMode_ = MM_MODELVIEW;
  float modelView_[16];
  float projection_[16];
  float normal_[3] = {0.0f, 0.0f, 1.0f};
  ScreenVertex pending_[3];
  uint8_t pendingCount_ = 0;
  int triangleCount_ = 0;
  Triangle triangles_[MAX_TRIANGLES];
  uint16_t colorBand_[320 * BAND_HEIGHT];
  float depthBand_[320 * BAND_HEIGHT];

  float *activeMatrix();
  static void identity(float *m);
  static void multiply(float *out, const float *a, const float *b);
  static float edge(const ScreenVertex &a, const ScreenVertex &b, float x, float y);
  void postMultiply(const float *rhs);
  ScreenVertex transformVertex(float x, float y, float z) const;
  void rasterTriangle(const Triangle &triangle, int bandY, int bandRows);
  uint16_t shade(uint16_t color, float light) const;
};
