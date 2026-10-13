#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "MiniMesa32.h"
#include "TeapotModel.h"

SET_LOOP_TASK_STACK_SIZE(32768);

TFT_eSPI tft;
MiniMesa32 gl(tft);
TeapotModel teapot(4);
float angleY = 0.0f;
float angleX = 0.0f;
bool touchActive = false;
bool touchHasCalibration = false;
bool automaticRotation = true;
float touchMapping[6] = {};
constexpr int BUTTON_X = 3;
constexpr int BUTTON_Y = 3;
constexpr int BUTTON_SIZE = 24;
constexpr int BUTTON_GAP = 2;
constexpr int PANEL_WIDTH = BUTTON_X + 3 * BUTTON_SIZE + 2 * BUTTON_GAP + 3;
constexpr int PANEL_HEIGHT = BUTTON_Y + 3 * BUTTON_SIZE + 2 * BUTTON_GAP + 3;

void readTouchPosition(int &x, int &y) {
  uint16_t rawX, rawY;
  tft.getTouchRaw(&rawX, &rawY);
  if (touchHasCalibration) {
    // Die vorhandene Schach-App hat die Matrix fuer Rotation 3 gespeichert.
    // Rotation 1 ist dazu um 180 Grad gedreht.
    float x3 = touchMapping[0]*rawX + touchMapping[1]*rawY + touchMapping[2];
    float y3 = touchMapping[3]*rawX + touchMapping[4]*rawY + touchMapping[5];
    x = 319 - lroundf(x3);
    y = 239 - lroundf(y3);
  } else {
    // Relative Gesten funktionieren auch ohne exakte Kalibrierung.
    x = map(rawY, 200, 3900, 0, 319);
    y = map(rawX, 200, 3900, 0, 239);
  }
}

void pollTouch() {
  uint16_t pressure = tft.getTouchRawZ();
  if (pressure < 300) {
    touchActive = false;
    return;
  }
  if (touchActive) return;  // Nur einmal pro Antippen ausloesen.
  touchActive = true;
  int x, y;
  readTouchPosition(x, y);
  int pitch = BUTTON_SIZE + BUTTON_GAP;
  int relX = x - BUTTON_X;
  int relY = y - BUTTON_Y;
  if (relX < 0 || relY < 0 || relX >= 3*pitch || relY >= 3*pitch) return;
  int col = relX / pitch;
  int row = relY / pitch;
  if (relX % pitch >= BUTTON_SIZE || relY % pitch >= BUTTON_SIZE) return;
  if (row == 1 && col == 1) return;
  int horizontal = col - 1;
  int vertical = row - 1;
  angleY += horizontal * 45.0f;
  angleX += vertical * 45.0f;
  automaticRotation = false;
  while (angleY >= 360.0f) angleY -= 360.0f;
  while (angleY < 0.0f) angleY += 360.0f;
  while (angleX >= 360.0f) angleX -= 360.0f;
  while (angleX < 0.0f) angleX += 360.0f;
  Serial.printf("TOUCH BUTTON row=%d col=%d -> X=%.0f Y=%.0f\n",
                row, col, angleX, angleY);
}

void drawDirectionButtons() {
  static const char *labels[3][3] = {
    {"NW", "N", "NE"}, {"W", "", "E"}, {"SW", "S", "SE"}
  };
  uint16_t fill = tft.color565(35, 42, 55);
  uint16_t border = tft.color565(165, 180, 200);
  for (int row = 0; row < 3; ++row) for (int col = 0; col < 3; ++col) {
    if (row == 1 && col == 1) continue;
    int x = BUTTON_X + col * (BUTTON_SIZE + BUTTON_GAP);
    int y = BUTTON_Y + row * (BUTTON_SIZE + BUTTON_GAP);
    tft.fillRoundRect(x, y, BUTTON_SIZE, BUTTON_SIZE, 4, fill);
    tft.drawRoundRect(x, y, BUTTON_SIZE, BUTTON_SIZE, 4, border);
    tft.setTextColor(TFT_WHITE, fill);
    tft.drawCentreString(labels[row][col], x + BUTTON_SIZE/2, y + 8, 1);
  }
}

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println();
  Serial.println("MiniMesa32 Utah Teapot startet");
  tft.init();
  tft.setRotation(1);
  // MiniMesa32 erzeugt RGB565-Werte als native uint16_t-Woerter. Beim
  // blockweisen pushImage()-Transfer muessen deren Bytes fuer den ILI9341
  // in Display-Reihenfolge ausgegeben werden.
  tft.setSwapBytes(true);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  Preferences preferences;
  if (preferences.begin("chess-touch", true)) {
    if (preferences.getBytesLength("rot3-v1") == sizeof(touchMapping)) {
      preferences.getBytes("rot3-v1", touchMapping, sizeof(touchMapping));
      touchHasCalibration = true;
      for (float value : touchMapping) if (!isfinite(value)) touchHasCalibration = false;
    }
    preferences.end();
  }
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("MiniMesa32", 160, 96, 4);
  tft.drawCentreString("Utah Teapot", 160, 132, 2);
  delay(700);
  tft.fillScreen(tft.color565(5, 10, 22));
  // Draw the static controls once. The 3D renderer preserves this rectangle.
  drawDirectionButtons();
  Serial.printf("UI: statisches Tastenfeld %dx%d, vom Renderer ausgespart\n",
                PANEL_WIDTH, PANEL_HEIGHT);
  Serial.printf("Touch: %s, acht Richtungstasten drehen jeweils 45 Grad\n",
                touchHasCalibration ? "gespeicherte Kalibrierung" : "Rohwert-Fallback");
}

void loop() {
  pollTouch();
  uint32_t started = millis();
  gl.beginFrame(tft.color565(5, 10, 22));

  gl.glMatrixMode(MM_PROJECTION);
  gl.glLoadIdentity();
  gl.gluPerspective(42.0f, (float)tft.width() / tft.height(), 0.1f, 50.0f);
  gl.glMatrixMode(MM_MODELVIEW);
  gl.glLoadIdentity();
  gl.glTranslatef(0.0f, 0.30f, -7.2f);
  gl.glRotatef(angleX, 1.0f, 0.0f, 0.0f);
  gl.glRotatef(angleY, 0.0f, 1.0f, 0.0f);
  gl.glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
  gl.glTranslatef(0.0f, 0.0f, -1.50f);
  gl.glScalef(0.82f, 0.82f, 0.82f);
  gl.glColor565(tft.color565(235, 118, 26));

  teapot.draw(gl);
  gl.present(PANEL_WIDTH, PANEL_HEIGHT);

  uint32_t elapsed = millis() - started;
  static uint32_t lastReport = 0;
  if (millis() - lastReport >= 2000) {
    lastReport = millis();
    Serial.printf("Teapot: %d Dreiecke, %lu ms/Frame, Heap %u Bytes\n",
                  gl.triangleCount(), (unsigned long)elapsed, ESP.getFreeHeap());
  }
  if (automaticRotation) {
    angleY += 3.0f;
    if (angleY >= 360.0f) angleY -= 360.0f;
  }
}
