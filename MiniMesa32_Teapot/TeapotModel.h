#pragma once

#include "MiniMesa32.h"

class TeapotModel {
 public:
  explicit TeapotModel(uint8_t tessellation = 4) : tessellation_(tessellation) {}
  void draw(MiniMesa32 &gl) const;

 private:
  struct Vec3 { float x, y, z; };
  struct Sample { Vec3 position, normal; };
  uint8_t tessellation_;

  static void basis(float t, float b[4], float d[4]);
  static Vec3 controlPoint(uint8_t patch, uint8_t instance, uint8_t row, uint8_t col);
  static Sample evaluate(uint8_t patch, uint8_t instance, float u, float v);
  static void emit(MiniMesa32 &gl, const Sample &sample);
};
