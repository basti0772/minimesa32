#include "../../MiniMesa32_Teapot/MiniMesa32_Teapot.ino"
#include <cassert>
int main(int argc,char** argv) {
  setup();
  angleX=15; angleY=30; automaticRotation=false;
  uint16_t panel[82*82];
  for(int y=0;y<82;++y) for(int x=0;x<82;++x) panel[y*82+x]=tft.pixels[y*320+x];
  loop();
  assert(gl.triangleCount()==1024);
  for(int y=0;y<82;++y) for(int x=0;x<82;++x) assert(panel[y*82+x]==tft.pixels[y*320+x]);
  tft.saveBMP(argc>1?argv[1]:"preview.bmp");
  puts("Desktop render: 1024 triangles; static button panel preserved.");
}
