#include "GifDecoder.h"
#include <FastLED.h>
#include <LedMatrix.h>
#include <SPIFFS.h>
#include <M5StickC.h>

#define MATRIXPIN 26
#define CANVAS_WIDTH 16
#define CANVAS_HEIGHT 16
cLEDMatrix<CANVAS_WIDTH, CANVAS_HEIGHT, HORIZONTAL_ZIGZAG_MATRIX> matrix;

File file;

GifDecoder<16, 16, 12> decoder;

bool fileSeekCallback(unsigned long position) { return file.seek(position); }
unsigned long filePositionCallback(void) { return file.position(); }
int fileReadCallback(void) { return file.read(); }
int fileReadBlockCallback(void * buffer, int numberOfBytes) { return file.read((uint8_t*)buffer, numberOfBytes); }
void screenClearCallback(void) { FastLED.clear(); M5.Lcd.fillScreen(BLACK); }
void updateScreenCallback(void) { FastLED.show(); }
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  M5.Lcd.fillRect(x * 5, y * 5, 5, 5, ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
  if (y < 9)
    matrix.DrawPixel(x, y, CRGB(red, green, blue));
}

void setup() {
  M5.begin();

  delay(1000);
  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);
  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);

  Serial.begin(115200);
  Serial.println("Starting AnimatedGIFs Sketch");

  FastLED.addLeds<WS2811, MATRIXPIN, GRB>(matrix[0], matrix.Size()).setCorrection(UncorrectedColor);
  Serial.printf("Neomatrix %d total LEDs, running on pin %d\n", CANVAS_WIDTH * CANVAS_HEIGHT, MATRIXPIN);
  FastLED.setBrightness(20);

  SPIFFS.begin();
}

void loop() {
  File root = SPIFFS.open("/gifs");
  while ((file = root.openNextFile())) {
    String name(file.name());
    name.replace("/gifs/", "");
    Serial.println("decoding " + name);
    decoder.startDecoding();
    while (!M5.BtnA.read()) {
      decoder.decodeFrame();
      M5.Lcd.setCursor(0, 82, 1);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.println(name);
      M5.Lcd.setTextColor(BLUE, BLACK);
      M5.Lcd.printf("vbat:%.3fV\r\n", M5.Axp.GetVbatData() * 1.1 / 1000);
      M5.Lcd.printf("btn:%d\r\n", M5.Axp.GetBtnPress());

      M5.Lcd.setTextColor(ORANGE, BLACK);
      M5.Lcd.println((M5.Axp.GetIchargeData() > 0)? "charging" : "        ");

      //M5.Lcd.printf("level:%d\r\n",M5.Axp.GetWarningLeve());
      if (M5.Axp.GetWarningLeve()){
        M5.Lcd.setTextColor(RED, BLACK);
        M5.Lcd.println("Warning: low battery");
        delay(1000);
        M5.Axp.SetSleep();
      }
      M5.BtnB.read();
      if (M5.BtnB.isPressed()) {
        uint16_t b = map(millis() - M5.BtnB.lastChange(), 0, 6000, 0, 255);
        M5.Axp.ScreenBreath(b);
        FastLED.setBrightness(b);
      }
    }
    M5.Lcd.fillScreen(BLACK);
    delay(200);
  }
}
