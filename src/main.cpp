#include <Arduino.h>
#include "ArduinoJson.h"
#include "TFT_eSPI.h"
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>

void init_lcd(void);
void drawBmp(const char *filename, int16_t x, int16_t y);
uint16_t read16(fs::File &f);
uint32_t read32(fs::File &f);

WiFiClient client;
HTTPClient http;
TFT_eSPI tft = TFT_eSPI();
DynamicJsonDocument iss(1024);

char ssid[] = "Cosmos";        
char pass[] = "voyager2";
String url = "http://api.open-notify.org/iss-now.json";

void setup() {
  SPIFFS.begin();
  init_lcd();
  drawBmp("/intro.bmp", 0, 0);
  tft.setTextWrap(true, true);
  WiFi.begin(ssid,pass);
  tft.setTextDatum(C_BASELINE);
  tft.drawString("Connecting to WiFi", 160, 180, 1);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  tft.drawString("Downloading ISS JSON data", 160, 180, 1);
}

void loop() {
  http.begin(client, url);
  http.GET();
  String payload = http.getString();
  http.end();

  deserializeJson(iss, payload);
  float latitude = iss["iss_position"]["latitude"];
  float longitude = iss["iss_position"]["longitude"];

  drawBmp("/map.bmp", 0, 0);

  int x_pos = (longitude + 180) * 0.8888;
  int y_pos = 240 - (latitude + 90) * 1.3333;
  
  tft.drawLine(0, y_pos, 320, y_pos, TFT_WHITE);
  tft.drawLine(x_pos, 0, x_pos, 240, TFT_WHITE);
  tft.fillCircle(x_pos, y_pos, 2, TFT_RED);
  tft.drawString("Latitude: " + String(latitude) + " Longitude: " + String(longitude), 160, 0, 1);

  delay(5000);
}

void init_lcd(void)
{
  pinMode(16, OUTPUT);    //AZSMZ Touch 2.4 need this
  digitalWrite(16, LOW);  //AZSMZ Touch 2.4 need this
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
}

void drawBmp(const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
      Serial.print("Loaded in "); Serial.print(millis() - startTime);
      Serial.println(" ms");
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}