#include <Arduino_GFX_Library.h>
#include <PubSubClient.h>
#include <pb.h>
#include <pb_common.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <WiFi.h>

// IMPORTANT: MAKE SURE YOU PICK THE CORRECT BOARD. DO NOT USE HELTEC ESP32 FOR THIS LAB!
// Use: Tools -> Board -> ESP32 Arduino -> ESP32 Wrover module
//
// Heltec ESP32 LoRa (V2) datasheet: https://resource.heltec.cn/download/WiFi_LoRa_32/WIFI_LoRa_32_V2.pdf
//
//
//
const char* WIFI_SSID     = "Linden 234 Unit 3";
const char* WIFI_PASSWORD = "Jameson1334";
const char* NET_ID = "ss2659";
// const char* LOBBY_NAME = "solo";    // Valid values are "test", "solo", "bot" and "prod". Not all are online yet.

// Suggested ESP32 Pinout:
//
// GPIO # | Purpose
//      2 | TFT DC
//      5 | TFT CS
//     17 | TFT RESET
//     18 | TFT SCLK
//     19 | TFT MISO
//     23 | TFT MOSI
//
//     39 | Joystick Vrx
//     38 | Joystick Vry
//     32 | Joystick SW
//
// Look over the linked Heltec datasheet for ESP32 pinouts.
// Note: GPIOs 34-39 have no pullups/pulldowns.
//
// Pinouts defined as macros. Feel free to change these as needed.
#define TFT_DC     2
#define TFT_CS     5
#define TFT_RESET  17
#define TFT_SCK    18
#define TFT_MISO   19
#define TFT_MOSI   23

#define JOYSTICK_X  38
#define JOYSTICK_Y  39
#define JOYSTICK_SW 32

// Define some global objects.
Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);
WiFiClient wifi_client;

int state = 0;

void setup() {
  Serial.begin(115200);
  
  InitTFTDisplay();
  TestTFTDisplay("hello world");
  
  InitJoystick();
  InitWifi();
}

void loop() {
  
}

void InitTFTDisplay() {
  display.begin();
  display.setRotation(2);
  display.fillScreen(BLACK);
  display.setCursor(20, 20);
  display.setTextSize(2);
  display.setTextColor(BLUE);
}

// Set up the joystick input pins. You will need to AnalogRead()
// on the X/Y pins and digitalRead() on the SW pin.
void InitJoystick() {
  pinMode(JOYSTICK_X, INPUT);
  pinMode(JOYSTICK_Y, INPUT);
  pinMode(JOYSTICK_SW, INPUT_PULLUP);  
}

// Connects to the wifi access point as defined in the macros.
void InitWifi() {
  Serial.print("Connecting to Wifi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected.");
}


// This is a test function to check that your TFT display is correctly wired up. Remove
// this after you are done with testing the TFT.
void TestTFTDisplay(String text) {
  display.fillScreen(BLACK);

  // Note that the screen is 240 pixels wide by 320 pixels tall.
  // (0,0) corresponds to the top left corner of the TFT display while
  // (239, 319) corresponds to the bottom right corner.
  display.fillRect(/*x_coordinate=*/ 10, /*y_coordinate=*/20, /*width=*/100, /*height=*/10, /*color=*/ WHITE);

  // You can also write text to the display. Setting the cursor beforehand will dictate the
  // screen coordinates to start rendering the text to.
  display.setCursor(1, 1);
  display.print(text);
}