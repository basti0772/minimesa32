# PC-Vorschau reproduzieren

Dieser kleine Desktop-Adapter kompiliert den unveraenderten Sketch sowie
`MiniMesa32.cpp` und `TeapotModel.cpp` mit einem C++17-Compiler. Er ersetzt nur
Arduino-, NVS- und Display-Zugriffe. Die 3D-Pixel stammen vom Originalrenderer;
Rundungen und die 5x7-Tastenbeschriftung werden vom Desktop-Adapter gezeichnet.
Die Vorschau ist kein Foto und kein aus dem ESP32 ausgelesener Bildschirm.

Vom Repository-Verzeichnis mit g++ auf dem PATH:

```sh
g++ -std=c++17 -O2 -Itools/preview tools/preview/render.cpp MiniMesa32_Teapot/MiniMesa32.cpp MiniMesa32_Teapot/TeapotModel.cpp -o preview
./preview preview.bmp
```

Die Ansicht verwendet X=15 und Y=30 Grad. Der Lauf prueft zugleich, dass alle
1024 Dreiecke erzeugt werden und der Renderer das Tastenfeld nicht ueberschreibt.
Die BMP-Datei kann verlustfrei in PNG konvertiert werden.
