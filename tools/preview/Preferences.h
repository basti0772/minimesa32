#pragma once
struct Preferences {
  bool begin(const char*,bool) { return false; }
  unsigned getBytesLength(const char*) { return 0; }
  void getBytes(const char*,void*,unsigned) {}
  void end() {}
};
