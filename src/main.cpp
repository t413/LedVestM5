#include "GifDecoder.h"
#include <FastLED.h>
#include <LEDMatrix.h>
#include <SPIFFS.h>
#include <M5StickC.h>

#define MATRIXPIN 26
#define FRONT_PINA 0
#define FRONTLEDS_NUM 22
#define CANVAS_WIDTH 16
#define CANVAS_HEIGHT 16
cLEDMatrix<CANVAS_WIDTH, CANVAS_HEIGHT, HORIZONTAL_ZIGZAG_MATRIX> matrix;
CRGB frontLedsL[2 * FRONTLEDS_NUM];
CRGB *frontLedsR = &(frontLedsL[FRONTLEDS_NUM]);

File file;

GifDecoder<16, 16, 12> decoder;
const uint8_t frontLedsLcdPxW = 4;
const uint8_t gifLcdPxSize = 4;
const uint8_t gifLcdPxBorder = ((80 - gifLcdPxSize * 16) / 2);
const uint8_t brightnesses[] = { 5, 8, 16, 32, 64, 128, 255 };
uint8_t bright = 0;
uint8_t fadeout_ = 0;
bool autoplay_ = false;

uint16_t c16(CRGB c) { return ((c.red & 0xF8) << 8) | ((c.green & 0xFC) << 3) | (c.blue >> 3); }

bool fileSeekCallback(unsigned long position) { return file.seek(position); }
unsigned long filePositionCallback(void) { return file.position(); }
int fileReadCallback(void) { return file.read(); }
int fileReadBlockCallback(void * buffer, int numberOfBytes) { return file.read((uint8_t*)buffer, numberOfBytes); }
void screenClearCallback(void) { FastLED.clear(); M5.Lcd.drawRect(gifLcdPxBorder, 0, gifLcdPxSize * CANVAS_WIDTH, gifLcdPxSize * CANVAS_HEIGHT, 0); }
void updateScreenCallback(void) { FastLED.show(); }
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  CRGB c = CRGB(red, green, blue).fadeToBlackBy(fadeout_);
  M5.Lcd.fillRect(gifLcdPxBorder + x * gifLcdPxSize, y * gifLcdPxSize, gifLcdPxSize, gifLcdPxSize, c16(c));
  matrix.DrawPixel(CANVAS_WIDTH - 1 - x, y, c);
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

  FastLED.addLeds<WS2811, MATRIXPIN, GRB>(matrix[0], matrix.Size()).setCorrection(0xC9E2FF);
  FastLED.addLeds<WS2811, FRONT_PINA, GRB>(frontLedsL, 2 * FRONTLEDS_NUM);
  FastLED.setMaxPowerInVoltsAndMilliamps(4, 500);
  Serial.printf("Neomatrix %d total LEDs, running on pin %d\n", CANVAS_WIDTH * CANVAS_HEIGHT, MATRIXPIN);
  setBrightnesses(brightnesses[bright = 2]);

  SPIFFS.begin();
}

void renderLCD(String name, CRGB c, bool autoplay);
void renderFrontLEDs(uint32_t now, CRGB &colorL, CRGB &colorR);
void fadeRender(bool fadein, uint16_t fadeTime, CRGB &colorL, CRGB &colorR) {
  uint32_t now = millis(), start = now, end = now + fadeTime;
  while ((now = millis()) < end) {
    fadeout_ = map(now, start, end, fadein? 255 : 0, fadein? 0 : 255);
    decoder.decodeFrame();
    renderFrontLEDs(now, colorL, colorR);
  }
  Serial.printf("fade %d done after %d ms\n", fadein, now - start);
}
void bigMsg(String small, String big) {
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(frontLedsLcdPxW, gifLcdPxSize * CANVAS_HEIGHT + 4, 2);
  M5.Lcd.println(small);
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.setTextFont(4);
  M5.Lcd.println(big);
}

void loop() {
  File root = SPIFFS.open("/gifs");
  CRGB domGifColorL, domGifColorR; //persist between gifs
  while ((file = root.openNextFile())) { //file loop
    uint32_t lastChange = millis();
    String name(file.name());
    name.replace("/gifs/", "");
    Serial.println("decoding " + name);
    decoder.startDecoding();

    fadeRender(true, autoplay_? 1200 : 150, domGifColorL, domGifColorR);
    while (true) { //frame render loop
      uint32_t now = millis();

      // Button handling:
      M5.update(); //updates buttons
      if (M5.BtnA.pressedFor(1000)) {
        Serial.println("autoplay toggle");
        autoplay_ = !autoplay_;
        M5.Lcd.fillScreen(BLACK);
        while (M5.BtnA.read()) {
          decoder.decodeFrame();
          renderFrontLEDs(millis(), domGifColorL, domGifColorR);
          bigMsg("autoplay", autoplay_? "   ON" : "   off");
        }
        M5.Lcd.fillScreen(BLACK);
      } else if (M5.BtnA.wasReleased()) { break; } //next gif

      if (M5.BtnB.pressedFor(1000)) { break; } //TODO: long press behavior
      else if (M5.BtnB.wasReleased()) {
        setBrightnesses(brightnesses[(bright = (bright + 1) % sizeof(brightnesses))]);
          uint32_t end = now + 200;
          M5.Lcd.fillScreen(BLACK);
          while ((now = millis()) < end) {
            decoder.decodeFrame();
            renderFrontLEDs(millis(), domGifColorL, domGifColorR);
            bigMsg("brightness", String(brightnesses[bright]));
          }
          M5.Lcd.fillScreen(BLACK);
      }

      if (autoplay_ && (now - lastChange) > 10000)
        break;

      decoder.decodeFrame();

      renderLCD(name, blend(domGifColorL,domGifColorR, 128), autoplay_);
      renderFrontLEDs(now, domGifColorL, domGifColorR);
    }
    M5.Lcd.fillScreen(BLACK);

    fadeRender(false, autoplay_? 1200 : 150, domGifColorL, domGifColorR);
    delay(100); //delay = cheap button debounce
  }
}

void renderLCD(String name, CRGB c, bool autoplay) {
  M5.Lcd.setCursor(frontLedsLcdPxW, gifLcdPxSize * CANVAS_HEIGHT + 2, 0);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println(name);
  M5.Lcd.setTextColor(c16(c), BLACK);
  if (autoplay)
    M5.Lcd.println(" -autoplay-");
  M5.Lcd.setTextFont(2);
  M5.Lcd.cursor_x += frontLedsLcdPxW;
  M5.Lcd.printf("  %.3fV\r\n", M5.Axp.GetVbatData() * 1.1 / 1000);
  M5.Lcd.setTextColor(ORANGE, BLACK);
  M5.Lcd.cursor_x += frontLedsLcdPxW;
  if ((M5.Axp.GetIchargeData() > 0))
    M5.Lcd.println(" charging");
  if (M5.Axp.GetWarningLeve()){
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.println(" Warning: low battery");
    delay(1000);
    M5.Axp.SetSleep();
  }
}

void fuzzyWrite(CRGB c, float position, CRGB*leds, size_t num) {
  uint16_t posfloor = (uint16_t)(position);
  uint16_t remain = (uint16_t)((position - posfloor) * 256.0);
  leds[ posfloor      % num] = c.lerp8(CRGB::Black, remain);
  leds[(posfloor + 1) % num] = c.lerp8(CRGB::Black, 255 - remain);
}

void renderFrontLEDs(uint32_t now, CRGB &colorL, CRGB &colorR) {
  CRGB frameColorL, frameColorR;
  for (uint16_t x=0; x < CANVAS_WIDTH; x++)
    for (uint16_t y=0; y < CANVAS_HEIGHT; y++) //could use rgb2hsv_approximate
      if (matrix(x,y).getLuma() > 64) //only use bright pixels
        nblend(x < (CANVAS_WIDTH/2)? frameColorL : frameColorR, matrix(x,y), 128);
  nblend(colorL, frameColorL, 10);
  nblend(colorR, frameColorR, 10);

  const uint8_t bttns = 4;
  const uint8_t gap = (FRONTLEDS_NUM / bttns);
  float pos = gap - ((uint8_t)(now >> 5) / 256.0) * gap;
  for (uint16_t i=0; i < bttns; i++) { //number of 'buttons'
    float p = i * gap + pos;
    fuzzyWrite(colorL, p, frontLedsL, FRONTLEDS_NUM);
    fuzzyWrite(colorR, p, frontLedsR, FRONTLEDS_NUM);
  }
  // Render vest-front leds to the TFT display:
  for (uint8_t i=0; i<FRONTLEDS_NUM; i++)
    for (uint8_t p=0; p<gifLcdPxBorder; p++) {
      M5.Lcd.fillRect(0,                  5 + i * gifLcdPxBorder, frontLedsLcdPxW, frontLedsLcdPxW, c16(frontLedsL[i]) );
      M5.Lcd.fillRect(80-frontLedsLcdPxW, 5 + i * gifLcdPxBorder, frontLedsLcdPxW, frontLedsLcdPxW, c16(frontLedsR[i]) );
    }
}
