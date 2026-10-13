#pragma once
// Minimal desktop adapter for rendering the unchanged Arduino demo off-device.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
using std::min;
using std::max;
using std::isfinite;
#define PROGMEM
#define DEG_TO_RAD 0.017453292519943295f
#define SET_LOOP_TASK_STACK_SIZE(n)
#define TOUCH_CS 32
#define OUTPUT 1
#define HIGH 1
template<class T, class L, class H> T constrain(T v,L lo,H hi) { return v<lo?T(lo):(v>hi?T(hi):v); }
inline uint8_t pgm_read_byte(const uint8_t* p) { return *p; }
inline float pgm_read_float(const float* p) { return *p; }
inline long map(long v,long a,long b,long c,long d) { return (v-a)*(d-c)/(b-a)+c; }
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
inline void delay(int) {}
inline uint32_t millis() { return 0; }
struct SerialAdapter {
  void begin(int) {}
  void println(const char* s="") { puts(s); }
  template<class... A> void printf(const char* s,A... a) { std::printf(s,a...); }
};
inline SerialAdapter Serial;
struct ESPAdapter { unsigned getFreeHeap() { return 0; } };
inline ESPAdapter ESP;
