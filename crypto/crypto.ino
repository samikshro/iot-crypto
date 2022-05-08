#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Heltec ESP32 LoRa (V2) datasheet: https://resource.heltec.cn/download/WiFi_LoRa_32/WIFI_LoRa_32_V2.pdf
//
//
//
const char* WIFI_SSID     = "Linden 234 Unit 3";
const char* WIFI_PASSWORD = "Jameson1334";
const char* NET_ID = "ss2659";

// ESP32 Pinout:
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
// Pinouts defined as macros
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

// DEFINE states
#define CRYPTO_INFO_BUY  1
#define CRYPTO_INFO_SELL  2
#define CRYPTO_INFO_NEXT  3
#define BUY_TOKEN_BUY  4
#define BUY_TOKEN_BACK 5
#define SELL_TOKEN_SELL  6
#define SELL_TOKEN_BACK 7

struct Coin {
  String coinName;
  int id;
  double coinPrice;
  double coinValue;
  int tokenOwned;
}; 

int state = 1;
int tokenDisplayID = 2;
int tokensToPurchase = 0;

struct Coin coins[4];

void setup() {
  Serial.begin(115200);
  
  InitTFTDisplay();
  TestTFTDisplay("hello world");
  
  InitJoystick();
  InitWifi();

  CryptoInfoDisplay("AVAX", 36080.67, 3, 400.08);
  
}

void wait(long add) {
  unsigned long time_now = millis();
  while(millis() < time_now + add){
    //wait approx. [period] ms
  }
}

void getCurrentCoinList() {
    HTTPClient http;
    http.useHTTP10(true);
    http.begin("https://api.sheety.co/3819fb057a19f6f9f01665dde28e5f08/iotCryptoTracker/dbTable"); //Specify the URL
    int httpCode = http.GET();                                        //Make the request
    if (httpCode > 0) { //Check for the returning code
      // Stream& input;
        StaticJsonDocument<768> doc;
        
        DeserializationError error = deserializeJson(doc, http.getStream());
        if (error) {
          Serial.println("at getCurrentCoinList 5");
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }
        
        for (JsonObject dbTable_item : doc["dbTable"].as<JsonArray>()) {
        
          String dbTable_item_name = dbTable_item["name"]; // "Bitcoin", "Ethereum", "Litecoin", "USDC"
          double dbTable_item_price = dbTable_item["price"]; // 34393.4, 2540.65, 93.27000000000001, 1
          int dbTable_item_tokenOwned = dbTable_item["tokenOwned"]; // 0, 2, 0, 0
          double dbTable_item_value = dbTable_item["value"]; // 0, 5081.3, 0, 0
          int dbTable_item_id = dbTable_item["id"]; // 2, 3, 4, 5
          int id = dbTable_item_id;
          coins[id-2].coinPrice = dbTable_item_price;
          coins[id-2].tokenOwned = dbTable_item_tokenOwned;
          coins[id-2].id = dbTable_item_id;
          coins[id-2].coinName = dbTable_item_name;
          coins[id-2].coinValue = dbTable_item_value;
//        Serial.println(dbTable_item_name);
        }

  
//        String payload = http.getString();
//        Serial.println(httpCode);
//        Serial.println(payload);
//        Serial.println(coins[0].coinName);
//        Serial.println(coins[1].coinName);
//        Serial.println(coins[2].coinName);
//        Serial.println(coins[3].coinName);
      }
  
    else {
      Serial.println("Error on HTTP request");
    }
  
    http.end(); //Free the resources
}

void loop() {
  getCurrentCoinList();
    int x_pos = analogRead(JOYSTICK_X);
    int y_pos = analogRead(JOYSTICK_Y);

    if (x_pos > 3800) { //RIGHT IS 4095
      if (state == CRYPTO_INFO_BUY) {
         state = CRYPTO_INFO_SELL;
         CryptoInfoDisplay("AVAX", 36080.67, 3, 400.08);
         wait(1000);
      } else if (state == CRYPTO_INFO_SELL) {
         state = CRYPTO_INFO_NEXT;
         CryptoInfoDisplay("AVAX", 36080.67, 3, 400.08);
         wait(1000);
         } else if (state == BUY_TOKEN_BACK) {
         state = BUY_TOKEN_BUY;
         BuyScreenDisplay("AVAX", 36080.67, 100000.00);
         wait(1000);
      } else if (state == SELL_TOKEN_BACK) {
         state = SELL_TOKEN_SELL;
//         SellScreenDisplay("AVAX", 36080.67, 100000.00);
         wait(1000);
      }
     
    } else if (x_pos < 200){ //LEFT IS 0
      if (state == CRYPTO_INFO_SELL) {
         state = CRYPTO_INFO_BUY;
         CryptoInfoDisplay("AVAX", 36080.67, 3, 400.08);
         wait(1000);
         
      } else if (state == CRYPTO_INFO_NEXT) {
         state = CRYPTO_INFO_SELL;
         CryptoInfoDisplay("AVAX", 36080.67, 3, 400.08);
         wait(1000);
      } else if (state == BUY_TOKEN_BUY) {
         state = BUY_TOKEN_BACK;
         BuyScreenDisplay("AVAX", 36080.67, 100000.00);
         wait(1000);
      } else if (state == SELL_TOKEN_SELL) {
         state = SELL_TOKEN_BACK;
//         SellScreenDisplay("AVAX", 36080.67, 100000.00);
         wait(1000);
      }
      
    } else if (y_pos > 3800) { //DOWN IS 4095
      if (state == BUY_TOKEN_BACK || state == BUY_TOKEN_BUY ||
          state == SELL_TOKEN_BACK || state == SELL_TOKEN_SELL) {
        (tokensToPurchase == 0) ? tokensToPurchase = 0 : tokensToPurchase--;

        if (tokensToPurchase != 0) {
          tokensToPurchase = tokensToPurchase + 1;
          BuyScreenDisplay("AVAX", 36080.67, 100000.00);
          wait(100);
        }
      }
    } else if (y_pos < 200){ //UP IS 0
      if (state == BUY_TOKEN_BACK || state == BUY_TOKEN_BUY ||
          state == SELL_TOKEN_BACK || state == SELL_TOKEN_SELL) {
        tokensToPurchase = tokensToPurchase + 1;
        BuyScreenDisplay("AVAX", 36080.67, 100000.00);
        wait(100);
      }
      
    } else if (digitalRead(JOYSTICK_SW) == LOW) { //IF THE BUTTON IS SELECTED
      if (state == CRYPTO_INFO_BUY) { //AND YOU WANT TO GO TO THE BUY PAGE
        state = BUY_TOKEN_BUY;
        BuyScreenDisplay("AVAX", 36080.67, 100000.00);
      } else if(state == CRYPTO_INFO_SELL) {//AND YOU WANT TO GO TO THE SELL PAGE
        
      } else if (state == CRYPTO_INFO_NEXT) {//AND YOU WANT TO GO TO THE NEXT TOKEN PAGE
        
      } else if (state == BUY_TOKEN_BUY) { //AND YOU WANT TO COMPLETE PURCHASE
        
      } else if (state == BUY_TOKEN_BACK) { //AND YOU WANT TO INFO PAGE FROM THE BUY PAGE
        state = CRYPTO_INFO_BUY;
        tokensToPurchase = 0;
        CryptoInfoDisplay("AVAX", 36080.67, 3, 400.08);
        wait(1000);
      } else if (state == SELL_TOKEN_SELL) { //AND YOU WANT TO COMPLETE SELLING
        
      } else if (state == SELL_TOKEN_BACK) { //AND YOU WANT TO INFO PAGE FROM THE SELL PAGE
        state = CRYPTO_INFO_BUY;
        tokensToPurchase = 0;
        CryptoInfoDisplay("AVAX", 36080.67, 3, 400.08);
        wait(1000);
      }
    }
}

void InitTFTDisplay() {
  display.begin();
  display.setRotation(3);
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

void BuyScreenDisplay(String tokenName, double price, double liquid) {
  
  display.fillScreen(BLACK);
  
  display.setCursor(1, 1);
  display.print("BUY " + tokenName);

  int cursorX = 10;
  int cursorY = 30;

  display.setCursor(cursorX, cursorY);
  display.print("Price ($): ");
  display.setCursor(cursorX + 150, cursorY);
  display.print(price);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("Liquid ($): ");
  display.setCursor(cursorX + 150, cursorY);
  display.print(liquid);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("Tokens to purchase: ");


  int width = 80;
  int height = 40;
  cursorY = cursorY + 50;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print(tokensToPurchase);
  
  cursorX = cursorX + 110;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ state == BUY_TOKEN_BACK ? CYAN : BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print("BACK");
  
  cursorX = cursorX + 110;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ state == BUY_TOKEN_BUY ? CYAN : BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print("BUY");
  
}

void CryptoInfoDisplay(String tokenName, double price, int tokensOwned, double valueOwned) {
  display.fillScreen(BLACK);
  
  display.setCursor(1, 1);
  display.print(tokenName);

  int cursorX = 10;
  int cursorY = 30;

  display.setCursor(cursorX, cursorY);
  display.print("Price ($): ");
  display.setCursor(cursorX + 200, cursorY);
  display.print(price);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("Tokens Owned: ");
  display.setCursor(cursorX + 200, cursorY);
  display.print(tokensOwned);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("Value Owned ($): ");
  display.setCursor(cursorX + 200, cursorY);
  display.print(valueOwned);

  int width = 80;
  int height = 40;
  cursorY = cursorY + 50;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ state == CRYPTO_INFO_BUY ? CYAN : BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print("BUY");
  
  cursorX = cursorX + 110;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ state == CRYPTO_INFO_SELL ? CYAN : BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print("SELL");
  
  cursorX = cursorX + 110;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ state == CRYPTO_INFO_NEXT ? CYAN : BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print("NEXT");
  
}

// This is a test function to check that your TFT display is correctly wired up. Remove
// this after you are done with testing the TFT.
void TestTFTDisplay(String text) {
  display.fillScreen(BLACK);

  // Note that the screen is 240 pixels wide by 320 pixels tall.
  // (0,0) corresponds to the top left corner of the TFT display while
  // (239, 319) corresponds to the bottom right corner.
  display.fillRect(/*x_coordinate=*/ 10, /*y_coordinate=*/20, /*width=*/100, /*height=*/10, /*color=*/ WHITE);

  display.drawRect(/*x_coordinate=*/ 10, /*y_coordinate=*/200, /*width=*/100, /*height=*/10, /*color=*/ WHITE);

  // You can also write text to the display. Setting the cursor beforehand will dictate the
  // screen coordinates to start rendering the text to.
  display.setCursor(1, 1);
  display.print(text);
}
