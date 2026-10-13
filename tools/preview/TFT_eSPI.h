#pragma once
#include "Arduino.h"
#include <fstream>
#define TFT_BLACK 0
#define TFT_WHITE 65535
#define TFT_ORANGE 0xFDA0
class TFT_eSPI {
 public:
  uint16_t pixels[320*240] = {};
  uint16_t foreground = TFT_WHITE;
  void init() {}
  void setRotation(int) {}
  void setSwapBytes(bool) {}
  int width() { return 320; }
  int height() { return 240; }
  uint16_t getTouchRawZ() { return 0; }
  void getTouchRaw(uint16_t* x,uint16_t* y) { *x=*y=0; }
  uint16_t color565(int r,int g,int b) { return ((r&248)<<8)|((g&252)<<3)|(b>>3); }
  void fillScreen(uint16_t c) { std::fill(pixels,pixels+320*240,c); }
  void pixel(int x,int y,uint16_t c) { if(x>=0&&x<320&&y>=0&&y<240) pixels[y*320+x]=c; }
  void pushImage(int x,int y,int w,int h,uint16_t* data) {
    for(int j=0;j<h;++j) for(int i=0;i<w;++i) pixel(x+i,y+j,data[j*w+i]);
  }
  static bool inside(int x,int y,int w,int h,int r) {
    if(x<0||y<0||x>=w||y>=h) return false;
    int dx=x<r?r-x:(x>=w-r?x-(w-r-1):0);
    int dy=y<r?r-y:(y>=h-r?y-(h-r-1):0);
    return dx*dx+dy*dy<=r*r;
  }
  void fillRoundRect(int x,int y,int w,int h,int r,uint16_t c) {
    for(int j=0;j<h;++j) for(int i=0;i<w;++i) if(inside(i,j,w,h,r)) pixel(x+i,y+j,c);
  }
  void drawRoundRect(int x,int y,int w,int h,int r,uint16_t c) {
    for(int j=0;j<h;++j) for(int i=0;i<w;++i)
      if(inside(i,j,w,h,r)&&!inside(i-1,j-1,w-2,h-2,r-1)) pixel(x+i,y+j,c);
  }
  void setTextColor(uint16_t f,uint16_t) { foreground=f; }
  void drawCentreString(const char* text,int cx,int y,int) {
    // 5x7 desktop glyphs for the compass labels; hardware uses TFT_eSPI font 1.
    const uint8_t glyphs[4][7]={{17,25,21,19,17,17,17},{31,16,16,30,16,16,31},
                              {15,16,16,14,1,1,30},{17,17,17,21,21,21,10}};
    int x=cx-int(strlen(text))*3;
    for(const char* p=text;*p;++p,x+=6) {
      const char* names="NESW"; const char* found=strchr(names,*p);
      if(!found) continue;
      for(int row=0;row<7;++row) for(int col=0;col<5;++col)
        if(glyphs[found-names][row]&(16>>col)) pixel(x+col,y+row,foreground);
    }
  }
  void saveBMP(const char* path) {
    std::ofstream out(path,std::ios::binary);
    auto u16=[&](unsigned v){out.put(v&255);out.put((v>>8)&255);};
    auto u32=[&](unsigned v){u16(v&65535);u16(v>>16);};
    out.write("BM",2);u32(54+320*240*3);u32(0);u32(54);
    u32(40);u32(320);u32(240);u16(1);u16(24);u32(0);u32(320*240*3);
    u32(0);u32(0);u32(0);u32(0);
    for(int y=239;y>=0;--y) for(int x=0;x<320;++x) {
      auto c=pixels[y*320+x];
      out.put((c&31)*255/31);out.put(((c>>5)&63)*255/63);out.put(((c>>11)&31)*255/31);
    }
    if(!out) throw "Cannot write preview";
  }
};
