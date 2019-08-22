#include "GifDecoder.h"
#include <FastLED.h>
#include <LedMatrix.h>
#include <SPIFFS.h>
#include <M5StickC.h>

#define MATRIXPIN 26
#define FRONT_PINA 0
#define FRONTLEDS_NUM 21
#define CANVAS_WIDTH 16
#define CANVAS_HEIGHT 16
cLEDMatrix<CANVAS_WIDTH, CANVAS_HEIGHT, HORIZONTAL_ZIGZAG_MATRIX> matrix;
CRGB frontLeds[FRONTLEDS_NUM];

File file;

GifDecoder<16, 16, 12> decoder;
#define FRONT_TFT_W 2
#define FRONT_TFT_H (M5.Lcd.height() / FRONTLEDS_NUM)
#define GIF_PXW 4
const uint8_t GIF_PXBDR = ((80 - GIF_PXW*16) / 2);

uint16_t c16(CRGB c) { return ((c.red & 0xF8) << 8) | ((c.green & 0xFC) << 3) | (c.blue >> 3); }

bool fileSeekCallback(unsigned long position) { return file.seek(position); }
unsigned long filePositionCallback(void) { return file.position(); }
int fileReadCallback(void) { return file.read(); }
int fileReadBlockCallback(void * buffer, int numberOfBytes) { return file.read((uint8_t*)buffer, numberOfBytes); }
void screenClearCallback(void) { FastLED.clear(); M5.Lcd.fillScreen(BLACK); }
void updateScreenCallback(void) { FastLED.show(); }
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  M5.Lcd.fillRect(GIF_PXBDR + x * GIF_PXW, y * GIF_PXW, GIF_PXW, GIF_PXW, c16(CRGB(red, green, blue)));
  matrix.DrawPixel(CANVAS_WIDTH - 1 - x, y, CRGB(red, green, blue));
}

void setBrightnesses(uint8_t b) {
  M5.Axp.ScreenBreath(b);
  FastLED.setBrightness(b);
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

  FastLED.addLeds<WS2811, MATRIXPIN, GRB>(matrix[0], matrix.Size()).setCorrection(TypicalSMD5050);
  FastLED.addLeds<WS2811, FRONT_PINA, GRB>(frontLeds, FRONTLEDS_NUM);
  Serial.printf("Neomatrix %d total LEDs, running on pin %d\n", CANVAS_WIDTH * CANVAS_HEIGHT, MATRIXPIN);
  setBrightnesses(20);

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
      uint32_t now = millis();
      decoder.decodeFrame();
      M5.Lcd.setCursor(FRONT_TFT_W, 82, 1);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.println(name);
      M5.Lcd.setTextColor(BLUE, BLACK);
      M5.Lcd.printf("vbat:%.3fV\r\n", M5.Axp.GetVbatData() * 1.1 / 1000);
      M5.Lcd.printf("btn:%d\r\n", M5.Axp.GetBtnPress());
      M5.Lcd.setTextColor(ORANGE, BLACK);
      M5.Lcd.println((M5.Axp.GetIchargeData() > 0)? "charging" : "        ");
      if (M5.Axp.GetWarningLeve()){
        M5.Lcd.setTextColor(RED, BLACK);
        M5.Lcd.println("Warning: low battery");
        delay(1000);
        M5.Axp.SetSleep();
      }
      M5.BtnB.read();
      if (M5.BtnB.isPressed()) {
        setBrightnesses(map(now - M5.BtnB.lastChange(), 0, 6000, 0, 255));
      }

      CRGB domGifColor;
      for (uint16_t i=0; i < matrix.Size(); i++)
        if (rgb2hsv_approximate(matrix(i)).val > 50)
          nblend(domGifColor, matrix(i), 128);

      fadeToBlackBy(frontLeds, FRONTLEDS_NUM, 40);
      int pos = random16(FRONTLEDS_NUM);
      frontLeds[pos] += domGifColor; //TODO change saturation with this random (random8(64), 200, 255);
      for (uint8_t i=0; i<FRONTLEDS_NUM; i++)
        for (uint8_t p=0; p<GIF_PXBDR; p++) {
          M5.Lcd.fillRect(0,    i * GIF_PXBDR, FRONT_TFT_W, FRONT_TFT_H, c16(frontLeds[i]) );
          M5.Lcd.fillRect(80-FRONT_TFT_W, i * GIF_PXBDR, 2, FRONT_TFT_H, c16(frontLeds[i]) );
        }
    }
    M5.Lcd.fillScreen(BLACK);
    delay(200); //delay = cheap button debounce
  }
}
