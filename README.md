# MiniMesa32 – Utah Teapot auf dem ESP32

**[Firmware im Browser installieren](https://basti0772.github.io/minimesa32/)**

Ein kompakter, Mesa/OpenGL-artiger Software-Renderer fuer den klassischen
**ESP32-WROOM-32** mit einem **ILI9341-Touchdisplay (320 x 240)**.
Die Demo zeigt einen orangefarbenen Utah Teapot mit Beleuchtung, Perspektive
und Z-Puffer. Acht Touch-Tasten drehen das Modell schrittweise.

## Screenshot / Vorschau

![MiniMesa32: Utah Teapot und acht Richtungstasten](docs/teapot-preview.png)

PC-gerenderte Vorschau, 320 x 240 Pixel, X=15 und Y=30 Grad. Die 3D-Darstellung
wurde mit dem unveraenderten Projekt-Renderer erzeugt. Die Display-API und die
Tastenbeschriftung werden fuer die Vorschau durch einen kleinen Desktop-Adapter
ersetzt. Dies ist kein Foto und kein vom ESP32 ausgelesener Screenshot.
[Vorschau reproduzieren](tools/preview/README.md).

## Funktionen

- Immediate-Mode-API: `glBegin(MM_TRIANGLES)`, `glNormal3f()`, `glVertex3f()`, `glEnd()`.
- ModelView- und Projektionsmatrix, Translation, Rotation, Skalierung und Perspektive.
- RGB565-Rasterisierung, Software-Z-Puffer und interpolierte Helligkeit.
- Speichersparendes Band-Rendering mit jeweils 16 Displayzeilen.
- Utah Teapot aus 32 bicubischen Bezier-Patches; Tessellation 4 ergibt 1024 Dreiecke.
- Statisches Tastenfeld: Die oberen linken 82 x 82 Pixel werden vom Renderer
  ausgespart, damit die Bedienelemente nicht bei jedem Bild geloescht werden.
- Korrigierte RGB565-Bytefolge durch `tft.setSwapBytes(true)`.

MiniMesa32 ist eine eigenstaendige kleine Implementierung mit aehnlichen
Funktionsnamen, **keine Portierung von Mesa und keine vollstaendige OpenGL-Implementierung**.
Der aktuelle Renderer ist fuer diese Demo ausgelegt: maximal 1152 Dreiecke,
320 Pixel breite Baender, kein allgemeines Near-Plane-Clipping und keine
inverse transponierte Normalenmatrix fuer nichtuniforme Skalierung.
Die Beleuchtung verwendet den Betrag des Skalarprodukts (beidseitige Beleuchtung).

## Hardware

| Anschluss | ESP32 GPIO |
| --- | ---: |
| TFT MISO / SDO | 19 |
| TFT MOSI / SDI | 23 |
| TFT SCLK | 18 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT RST | 4 |
| Touch CS (XPT2046) | 32 |

Display und Touch teilen sich den SPI-Bus. Gemeinsame Masse und eine zum
Displaymodul passende Versorgung sind erforderlich. SPI-Takt: 27 MHz fuer das
Display, 2,5 MHz fuer Touch. Anzeige-Rotation: `1` (Querformat).

## Browser Installation

Installer: **https://basti0772.github.io/minimesa32/**

Voraussetzungen: klassischer **ESP32-WROOM-32 mit 4 MB Flash**, ILI9341/XPT2046
mit der oben dokumentierten Verdrahtung, USB-Datenkabel und aktuelles Chrome oder
Edge auf einem Desktop-PC. Nicht fuer ESP32-S2/S3/C3. Web Serial benoetigt HTTPS
(lokal ist auch localhost moeglich); eventuell ist ein USB-UART-Treiber erforderlich.

1. ESP32 per USB anschliessen und andere serielle Programme schliessen.
2. Installer-Seite oeffnen und **Install** anklicken.
3. Den zum ESP32 gehoerenden seriellen Port auswaehlen.
4. Installation bestaetigen, Fortschritt abwarten und USB nicht trennen.

**Achtung:** Die Installation ersetzt die Firmware. Das merged Image umfasst
auch den NVS-Bereich und ueberschreibt gespeicherte Touch-Kalibrierungen, selbst
wenn im Installer kein vollstaendiges Loeschen gewaehlt wird. Daten vorher sichern.
Die Demo verwendet danach ihre feste Standardzuordnung, die je nach Touchpanel
angepasst werden muss. Der klassische Arduino-Upload einzelner Images ist der
Fallback, wenn bestehende NVS-Daten erhalten bleiben sollen (keinen Full-Erase auswaehlen).

Falls die Verbindung scheitert: Datenkabel/Port pruefen, seriellen Monitor schliessen.
Wenn der Bootloader nicht automatisch startet, BOOT waehrend des Verbindungsaufbaus
gedrueckt halten und nach erfolgreicher Verbindung loslassen.

### Einmalig GitHub Pages aktivieren

Unter [Repository Settings → Pages](https://github.com/basti0772/minimesa32/settings/pages):

- **Source: Deploy from a branch**
- **Branch: main**
- **Folder: /docs**
- **Save**; danach den Pages-Deployment-Lauf abwarten.

Die Dateien allein aktivieren diese Repository-Einstellung nicht. Bei einem 404
zuerst diese Einstellung und den Deployment-Status pruefen.

### Firmware selbst bauen

Voraussetzungen: Arduino CLI (CI: 1.5.1), PowerShell **7+** (`pwsh`) und Git.
Einmalig die festgelegten Abhaengigkeiten installieren:

```powershell
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
arduino-cli lib install TFT_eSPI@2.5.43
```

Aus dem Repository-Verzeichnis:

```powershell
pwsh -NoProfile -File ./tools/Build-Web-Firmware.ps1
pwsh -NoProfile -File ./tools/Test-Web-Firmware.ps1
pwsh -NoProfile -File ./tools/Test-Web-Firmware-Negative.ps1
```

Der Build-Helfer kompiliert vollstaendig fuer `esp32:esp32:esp32` und bindet die
Projekt-Datei `config/User_Setup.h` explizit in alle C++-Uebersetzungseinheiten ein.
**Keine globale TFT_eSPI-Konfiguration muss dafuer geaendert werden.** Er benoetigt
keinen COM-Port. Versionen, Board und Projektname stehen in `tools/web-firmware.json`;
die Firmware-Version steht zentral in **`VERSION`**. Die Seite liest die daraus
generierte Manifest-Version, statt eine zweite Versionsnummer zu pflegen.

Die Quell-Images und Adressen stammen aus `.build/web/flash_args`, erzeugt vom
ESP32-Arduino-Core. Das zum Core gehoerende esptool fuehrt sie ausdruecklich mit
`--target-offset 0` zusammen. Modus, Frequenz und Groesse werden gegen die
expandierten Build-Eigenschaften geprueft. Jedes Teil-Image wird per SHA-256 an
seiner Zieladresse im merged Image nachgeprueft.
Compiler-Pfadzuordnungen verhindern persoenliche Windows-Pfade in den BINs.
Die Negativtests pruefen falsche Chipfamilie, Offsets, Pfade, Versionen und
beschaedigte Images in isolierten Testkopien unter `.build/`.

Ergebnisse:

- `firmware/minimesa32.bin`: dauerhaft eingechecktes merged Image, Flash-Offset 0.
- `firmware/build-info.json`: reale Quell-Offsets, Groessen, SHA-256 und Build-Konfiguration.
- `firmware/flash_args` und `firmware/partitions.csv`: Nachweis aus dem Build.
- `docs/firmware/`: identische Kopien fuer Pages; `docs/manifest.json` verweist auf
  `firmware/minimesa32.bin`, relativ zum Manifest. `../firmware/` waere bei `/docs`
  als Pages-Quelle nicht erreichbar.

Das Standard-Layout des verwendeten Cores umfasst NVS, OTA-Daten, zwei App-Slots,
SPIFFS und Coredump. Es wird unveraendert aus dem Build uebernommen. Das merged
Image ist nicht bis zum Ende der 4 MB aufgefuellt; es endet nach dem App-Image.

Vor einer Veroeffentlichung `VERSION` bei Bedarf erhoehen, den Build-Helfer
ausfuehren und **beide Firmware-Verzeichnisse sowie Manifest und Metadaten** committen.
Der Actions-Workflow prueft zuerst die eingecheckten Dateien und baut anschliessend
erneut. Er stellt Build-Artefakte bereit, aktualisiert aber absichtlich nicht selbst
den Branch. Der Installer verwendet niemals eine temporaere Actions-Artifact-URL.

Lokaler Seitentest, wenn Python installiert ist:

```powershell
python -m http.server 8000 --bind 127.0.0.1 --directory docs
# http://localhost:8000 im Browser oeffnen
```

### Manuelles Flashen als Fallback

Aus dem vollstaendigen Repository baut der bestehende Helfer dieselbe Firmware,
erkennt anschliessend den COM-Port und laedt die einzelnen Arduino-Images hoch:

```powershell
pwsh -NoProfile -File ./MiniMesa32_Teapot/Build-And-Flash.ps1
powershell -ExecutionPolicy Bypass -File ./MiniMesa32_Teapot/Read-Serial.ps1 -Seconds 10
```

Bei mehreren Boards vorher den richtigen Port ermitteln und explizit mit `-Port`
angeben. Der automatische Helfer waehlt den ersten passenden Port. Es gibt keinen
fest im Projekt hinterlegten COM-Port. Fuer Arduino IDE und das eigenstaendige
ZIP gelten weiterhin die folgenden Anweisungen.

## Arduino IDE / eigenstaendiges ZIP

1. Dieses Repository herunterladen oder klonen:

   ```sh
   git clone https://github.com/basti0772/minimesa32.git
   cd minimesa32
   ```

2. Arduino IDE oder Arduino CLI installieren. Boardpaket `esp32` von Espressif
   und Bibliothek `TFT_eSPI` von Bodmer installieren. Der bisher getestete Stand:
   ESP32-Core **3.3.12**, TFT_eSPI **2.5.43**.

3. TFT_eSPI konfigurieren: Die vorhandene `User_Setup.h` im installierten
   TFT_eSPI-Bibliotheksordner zuerst sichern und anschliessend durch
   [`MiniMesa32_Teapot/config/User_Setup.h`](MiniMesa32_Teapot/config/User_Setup.h)
   ersetzen. `User_Setup_Select.h` muss dieses Setup einbinden (Standard).
   Die Datei im Projekt wird von TFT_eSPI **nicht automatisch geladen**.
   Diese Bibliothekseinstellung wirkt auch auf andere TFT_eSPI-Projekte.

4. In der Arduino IDE `MiniMesa32_Teapot/MiniMesa32_Teapot.ino` oeffnen,
   **ESP32 Dev Module** und den angeschlossenen Port auswaehlen, dann hochladen.

Alternativ mit Arduino CLI, vom Repository-Verzeichnis aus:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 .\MiniMesa32_Teapot
$espPort = powershell -ExecutionPolicy Bypass -File .\MiniMesa32_Teapot\Find-ESP32Port.ps1
arduino-cli upload -p $espPort --fqbn esp32:esp32:esp32 .\MiniMesa32_Teapot
powershell -ExecutionPolicy Bypass -File .\MiniMesa32_Teapot\Read-Serial.ps1 -Seconds 10
```

Oder `MiniMesa32_Teapot/Build-And-Flash.ps1` ausfuehren. Bei mehreren seriellen
Boards den Port explizit ueber `-Port COMx` angeben und vorab kontrollieren.
Der mitgelieferte Erkennungshelfer waehlt den ersten passenden Port.

[Eigenstaendiges Arduino-Projekt als ZIP herunterladen](MiniMesa32_Teapot.zip).
TFT_eSPI und der ESP32-Core sind externe Abhaengigkeiten und nicht im ZIP enthalten.

## Bedienung und Touch-Kalibrierung

Nach dem Start dreht sich der Teapot automatisch. Die erste gueltige
Tastenberuehrung stoppt die Automatik. `N`/`S` aendern X um -/+45 Grad,
`W`/`E` aendern Y um -/+45 Grad. Die Diagonaltasten aendern beide Achsen
jeweils um 45 Grad; dies ist keine einzelne 45-Grad-Drehung um eine diagonale Achse.
Vor dem naechsten Tastendruck loslassen. Ein Neustart aktiviert die Automatik wieder.

Der Sketch uebernimmt, falls vorhanden, die auf dem bisherigen Aufbau gespeicherte
affine Touch-Kalibrierung aus NVS (`chess-touch`, Schluessel `rot3-v1`) und
rechnet sie von Display-Rotation 3 auf Rotation 1 um. Ohne diese Daten nutzt er
eine feste Rohwertzuordnung (200 bis 3900, Achsen vertauscht). Diese ist
hardwareabhaengig: Treffen Beruehrungen die Tasten nicht, muss
`readTouchPosition()` an das Display angepasst werden. Eine eigene
Kalibrierungsoberflaeche ist in diesem Projekt noch nicht enthalten.

## Projektinhalt

- `MiniMesa32_Teapot/`: vollstaendiger Arduino-Sketch, Renderer, Bezier-Modell,
  Displaykonfiguration, PowerShell-Helfer und FreeGLUT-Lizenz.
- `MiniMesa32_Teapot.zip`: separat entpackbarer Arduino-Projektordner.
- `docs/teapot-preview.png`: eingebundene Vorschau.
- `docs/`: ESP-Web-Tools-Installer und dauerhaft erreichbare Firmware-Kopie.
- `firmware/`: merged BIN, Build-Metadaten und verwendetes Partitionslayout.
- `tools/Build-Web-Firmware.ps1`: gemeinsamer Build fuer Browser- und USB-Installation.
- `.github/workflows/web-firmware.yml`: Build, Konsistenzpruefung und Artefakt-Upload.
- `tools/preview/`: reproduzierbarer Desktop-Renderlauf mit unveraendertem Sketch.

## Bisherige Hardwarepruefung

Die Version mit flackerfreiem Tastenfeld wurde am 30.09.2026 mit Arduino CLI
kompiliert, auf einem ESP32-D0WDQ6 geflasht und seriell geprueft:
328304 Byte Flash, 113372 Byte statischer RAM, 1024 Dreiecke,
173–177 ms pro Bild und konstant 213464 Byte freier Heap.
Das sind Messwerte des damaligen Aufbaus, keine garantierten Leistungswerte.

## Herkunft und Lizenzhinweise

Die Teapot-Kontrolldaten stammen aus
[FreeGLUT](https://github.com/freeglut/freeglut/blob/master/src/fg_teapot_data.h).
Der zugehoerige Lizenztext liegt in
[`FREEGLUT-LICENSE.txt`](MiniMesa32_Teapot/FREEGLUT-LICENSE.txt).
TFT_eSPI und das ESP32-Boardpaket unterliegen ihren eigenen Lizenzen.
Fuer den projektspezifischen Code wurde bislang keine separate Lizenz festgelegt.
