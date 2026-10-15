# MiniMesa32 Utah Teapot

Arduino-Projekt fuer einen klassischen ESP32-WROOM-32 und ein ILI9341-Display.
Es zeigt den bekannten Utah Teapot als rotierendes, beleuchtetes 3D-Modell.

Links oben befinden sich acht Richtungstasten (N, NE, E, SE, S, SW, W und NW).
Jeder Tastendruck dreht den Teapot exakt 45 Grad in die entsprechende Richtung.
Beim ersten Tastendruck stoppt die automatische Rotation; die gewaehlte Ansicht
bleibt anschliessend stehen.
Das 82 x 82 Pixel grosse Tastenfeld wird einmal gezeichnet und bei allen
3D-Bilduebertragungen ausgespart. Dadurch werden die Tasten nicht mehr zwischen
zwei Bildern geloescht und neu aufgebaut.

## Hardware und Anzeige

- Board: `esp32:esp32:esp32`
- Display: ILI9341, 320 x 240, Querformat
- Bibliothek: TFT_eSPI
- Pins: MISO 19, MOSI 23, SCLK 18, CS 15, DC 2, RST 4
- Serieller Monitor: 115200 Baud

Die TFT_eSPI-Konfiguration muss diese Pins enthalten. Eine passende Kopie liegt
unter `config/User_Setup.h`.

## Renderer

`MiniMesa32` stellt eine kleine OpenGL/Mesa-aehnliche Immediate-Mode-API bereit:
Matrixwahl, Translation, Rotation, Skalierung, Perspektive, Normalen und
Dreiecke. Der Renderer rastert RGB565-Dreiecke mit Lambert-Beleuchtung und
Z-Puffer. Um RAM zu sparen, wird das Bild in 16 Zeilen hohen Baendern aufgebaut.

Das Modell wird aus 32 bicubischen Bezier-Patches erzeugt. Bei Tessellation 4
entstehen 800 Patch-Vertices und 1024 Dreiecke. Die Kontrolldaten stammen aus
FreeGLUT und stehen unter dessen MIT/X11-artiger Lizenz.

## Kompilieren und Flashen

Im vollstaendigen GitHub-Repository verwendet `Build-And-Flash.ps1` den gemeinsamen
Web-Firmware-Build (PowerShell 7 erforderlich) mit der lokalen Displaykonfiguration.
Im eigenstaendigen ZIP kompiliert der Helfer direkt mit Arduino CLI; dafuer muss
TFT_eSPI zuvor mit `config/User_Setup.h` konfiguriert werden.
Browser-Installer und Anleitung: https://basti0772.github.io/minimesa32/

Die folgenden Befehle werden aus diesem Sketch-Ordner ausgefuehrt:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 .
$port = powershell -ExecutionPolicy Bypass -File .\Find-ESP32Port.ps1
arduino-cli upload -p $port --fqbn esp32:esp32:esp32 .
powershell -ExecutionPolicy Bypass -File .\Read-Serial.ps1 -Seconds 10
```
