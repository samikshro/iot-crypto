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
#define PORTFOLIO_NEXT 9
#define MAX_PAGES 4
#define LOW_AXIS 1000
#define HIGH_AXIS 3800

#define WAIT_TIME 400

#define BLANK  0
#define LOADING  2
#define SUCCESS  3
#define ERROR_BUY  4
#define ERROR_SELL  1


struct Coin {
  String coinName;
  int id;
  double coinPrice;
  double coinValue;
  int tokenOwned;
}; 

struct Portfolio {
  double liquid;
  double tvl;
  double totalValue;
  int id;
}; 

int state = 1;
int tokensToPurchase = 0;
int currentCoinID = 0;
int buySellMsg = BLANK;

struct Coin coins[4];
struct Portfolio portfolio;

void setup() {
  Serial.begin(115200);
  
  InitTFTDisplay();
  TestTFTDisplay("Loading wallet...");
  
  InitJoystick();
  InitWifi();
  
  getCurrentCoinList();
  getCurrentPortfolio();
  CryptoInfoDisplay(currentCoinID);
  
}

void wait(long add) {
  unsigned long time_now = millis();
  while(millis() < time_now + add){
    //wait approx. [period] ms
  }
}

void getCurrentPortfolio() {
    HTTPClient http;
    http.useHTTP10(true);
    http.begin("https://api.sheety.co/3819fb057a19f6f9f01665dde28e5f08/iotCryptoTracker/earnings"); //Specify the URL
    int httpCode = http.GET();                                        //Make the request
    if (httpCode > 0) { //Check for the returning code
      // Stream& input;
      StaticJsonDocument<192> doc;

      DeserializationError error = deserializeJson(doc, http.getStream());
      
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      
      JsonObject earnings_0 = doc["earnings"][0];
      double earnings_0_liquid = earnings_0["liquid"]; // 50
      double earnings_0_tvl = earnings_0["tvl"]; // 210449.15000000002
      double earnings_0_totalValue = earnings_0["totalValue"]; // 210499.15000000002
      int earnings_0_id = earnings_0["id"]; // 2
      portfolio.liquid = earnings_0_liquid;
      portfolio.tvl = earnings_0_tvl;
      portfolio.totalValue = earnings_0_totalValue;
      portfolio.id = earnings_0_id;

    }
  
    else {
      Serial.println("Error on HTTP request");
    }
  
    http.end(); //Free the resources
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
        }
      }
  
    else {
      Serial.println("Error on HTTP request");
    }
  
    http.end(); //Free the resources
}

void putLiquidity(double newLiquid) {
  HTTPClient http;
  http.begin("https://api.sheety.co/3819fb057a19f6f9f01665dde28e5f08/iotCryptoTracker/earnings/2"); //Specify the URL
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  StaticJsonDocument<32> doc;
  JsonObject root = doc.to<JsonObject>();
  root["earning"]["liquid"] = newLiquid;
  String json;
  serializeJson(root, json);
  
  int httpCode = http.PUT(json); 
  Serial.println(httpCode);
  Serial.print(http.getString());
  http.end();    
}

void putTokenOwned(int localID, int tokenOwnedUpdate, bool isBuy) {
  HTTPClient http;
  char snum[2];
  sprintf(snum, "%d", coins[localID].id);
  char str[81]= "https://api.sheety.co/3819fb057a19f6f9f01665dde28e5f08/iotCryptoTracker/dbTable/";
  strcat(str, snum);
 
  http.begin(str); //Specify the URL
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  StaticJsonDocument<32> doc;
  JsonObject root = doc.to<JsonObject>();
  if (isBuy) {
    root["dbTable"]["tokenOwned"] = coins[localID].tokenOwned + tokenOwnedUpdate;
  } else {
    root["dbTable"]["tokenOwned"] = coins[localID].tokenOwned - tokenOwnedUpdate;
  }
  
  String json;
  serializeJson(root, json);
//  Serial.println(json);
  
  int httpCode = http.PUT(json); 
  Serial.println(httpCode);
  Serial.print(http.getString());
  http.end();    
}

void putBuyOrder(int localID, int tokenToOwn) {
  double newLiquid = portfolio.liquid - (tokenToOwn*coins[localID].coinPrice);
  if (newLiquid >= 0) {
    putTokenOwned(localID, tokenToOwn, true);
    putLiquidity(newLiquid);
    getCurrentCoinList();
    getCurrentPortfolio();
    buySellMsg = SUCCESS;
    
  } else {
    buySellMsg = ERROR_BUY;
    Serial.println("Error: too little liquidity");
  }
}

void putSellOrder(int localID, int tokenToSell) {
  
  double newLiquid = portfolio.liquid + (tokenToSell*coins[localID].coinPrice);
  if (tokenToSell <= coins[localID].tokenOwned) {
    putTokenOwned(localID, tokenToSell, false);
    putLiquidity(newLiquid);
    getCurrentCoinList();
    getCurrentPortfolio();
    buySellMsg = SUCCESS;
  } else {
    buySellMsg = ERROR_SELL;
    Serial.println("Error: too many tokens");
  }
}

void loop() {
    int x_pos = analogRead(JOYSTICK_X);
    int y_pos = analogRead(JOYSTICK_Y);

    if (x_pos > HIGH_AXIS) { //RIGHT IS 4095
      if (state == CRYPTO_INFO_BUY) {
         state = CRYPTO_INFO_SELL;
         CryptoInfoDisplay(currentCoinID);
         wait(WAIT_TIME);
      } else if (state == CRYPTO_INFO_SELL) {
         state = CRYPTO_INFO_NEXT;
         CryptoInfoDisplay(currentCoinID);
         wait(WAIT_TIME);
         } else if (state == BUY_TOKEN_BACK) {
         state = BUY_TOKEN_BUY;
         BuyScreenDisplay(currentCoinID);
         wait(WAIT_TIME);
      } else if (state == SELL_TOKEN_BACK) {
         state = SELL_TOKEN_SELL;
         SellScreenDisplay(currentCoinID);
         wait(WAIT_TIME);
      } 
     
    } else if (x_pos < LOW_AXIS){ //LEFT IS 0
      if (state == CRYPTO_INFO_SELL) {
         state = CRYPTO_INFO_BUY;
         CryptoInfoDisplay(currentCoinID);
         wait(WAIT_TIME);
         
      } else if (state == CRYPTO_INFO_NEXT) {
         state = CRYPTO_INFO_SELL;
         CryptoInfoDisplay(currentCoinID);
         wait(WAIT_TIME);
      } else if (state == BUY_TOKEN_BUY) {
         state = BUY_TOKEN_BACK;
         BuyScreenDisplay(currentCoinID);
         wait(WAIT_TIME);
      } else if (state == SELL_TOKEN_SELL) {
         state = SELL_TOKEN_BACK;
         SellScreenDisplay(currentCoinID);
         wait(WAIT_TIME);
      } 
      
    } else if (y_pos > HIGH_AXIS) { //DOWN IS 4095
      if (state == BUY_TOKEN_BACK || state == BUY_TOKEN_BUY) {
        if (tokensToPurchase != 0) {
          tokensToPurchase = tokensToPurchase - 1;
          BuyScreenDisplay(currentCoinID);
          wait(WAIT_TIME);
        } 
      } else if (state == SELL_TOKEN_BACK || state == SELL_TOKEN_SELL) {
        if (tokensToPurchase != 0) {
          tokensToPurchase = tokensToPurchase - 1;
          SellScreenDisplay(currentCoinID);
          wait(WAIT_TIME);
        } 
      }
    } else if (y_pos < LOW_AXIS){ //UP IS 0
      if (state == BUY_TOKEN_BACK || state == BUY_TOKEN_BUY) {
        tokensToPurchase = tokensToPurchase + 1;
        BuyScreenDisplay(currentCoinID);
        wait(WAIT_TIME);
      } else if (state == SELL_TOKEN_BACK || state == SELL_TOKEN_SELL) {
        tokensToPurchase = tokensToPurchase + 1;
        SellScreenDisplay(currentCoinID);
        wait(WAIT_TIME);
      }
      
    } else if (digitalRead(JOYSTICK_SW) == LOW) { //IF THE BUTTON IS SELECTED
      if (state == CRYPTO_INFO_BUY) { //AND YOU WANT TO GO TO THE BUY PAGE
        state = BUY_TOKEN_BUY;
        BuyScreenDisplay(currentCoinID);
        wait(WAIT_TIME);
      } else if(state == CRYPTO_INFO_SELL) {//AND YOU WANT TO GO TO THE SELL PAGE
        state = SELL_TOKEN_SELL;
        SellScreenDisplay(currentCoinID);
        wait(WAIT_TIME);
      } else if (state == CRYPTO_INFO_NEXT || state == PORTFOLIO_NEXT) {//AND YOU WANT TO GO TO THE NEXT TOKEN PAGE
        if (currentCoinID + 1 < MAX_PAGES) {
          currentCoinID = currentCoinID + 1;
          state = CRYPTO_INFO_BUY; 
          CryptoInfoDisplay(currentCoinID);
          wait(WAIT_TIME);
        } else if (currentCoinID + 1 == MAX_PAGES) {
          currentCoinID = currentCoinID + 1;
          state = PORTFOLIO_NEXT;
          PortfolioDisplay();
          wait(WAIT_TIME);
        } else {
          currentCoinID = 0;
          state = CRYPTO_INFO_BUY; 
          CryptoInfoDisplay(currentCoinID);
          wait(WAIT_TIME);
        }
        
      } else if (state == BUY_TOKEN_BUY) { //AND YOU WANT TO COMPLETE PURCHASE
        buySellMsg = LOADING;
        BuyScreenDisplay(currentCoinID);
        putBuyOrder(currentCoinID, tokensToPurchase);
        tokensToPurchase = 0;
        BuyScreenDisplay(currentCoinID);
        wait(1000);
        buySellMsg = BLANK;
        BuyScreenDisplay(currentCoinID);
      } else if (state == BUY_TOKEN_BACK) { //AND YOU WANT TO INFO PAGE FROM THE BUY PAGE
        state = CRYPTO_INFO_BUY;
        tokensToPurchase = 0;
        CryptoInfoDisplay(currentCoinID);
        wait(WAIT_TIME);
      } else if (state == SELL_TOKEN_SELL) { //AND YOU WANT TO COMPLETE SELLING
        buySellMsg = LOADING;
        SellScreenDisplay(currentCoinID);
        putSellOrder(currentCoinID, tokensToPurchase);
        tokensToPurchase = 0;
        SellScreenDisplay(currentCoinID);
        wait(1000);
        buySellMsg = BLANK;
        SellScreenDisplay(currentCoinID);
      } else if (state == SELL_TOKEN_BACK) { //AND YOU WANT TO INFO PAGE FROM THE SELL PAGE
        state = CRYPTO_INFO_BUY;
        tokensToPurchase = 0;
        CryptoInfoDisplay(currentCoinID);
        wait(WAIT_TIME);
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

void PortfolioDisplay() {
  display.fillScreen(BLACK);
  
  display.setCursor(1, 1);
  display.print("PORTFOLIO");

  int cursorX = 10;
  int cursorY = 30;

  display.setCursor(cursorX, cursorY);
  display.print("Liquid($): ");
  display.setCursor(cursorX + 150, cursorY);
  display.print(portfolio.liquid);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("TVL($): ");
  display.setCursor(cursorX + 160, cursorY);
  display.print(portfolio.tvl);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("Value($): ");
  display.setCursor(cursorX + 150, cursorY);
  display.print(portfolio.totalValue);


  int width = 80;
  int height = 40;
  cursorY = cursorY + 30;
  cursorX = cursorX + 220;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ state == PORTFOLIO_NEXT ? CYAN : BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print("NEXT");
}

void SellScreenDisplay(int localID) {
  String tokenName = coins[localID].coinName; 
  double price= coins[localID].coinPrice; 
  double valueOwned= coins[localID].coinValue;
  double tokenOwned = coins[localID].tokenOwned;
  
  display.fillScreen(BLACK);
  
  display.setCursor(1, 1);
  display.print("SELL " + tokenName);

  int cursorX = 10;
  int cursorY = 30;

  display.setCursor(cursorX, cursorY);
  display.print("Price ($): ");
  display.setCursor(cursorX + 150, cursorY);
  display.print(price);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("Tokens Owned: ");
  display.setCursor(cursorX + 160, cursorY);
  display.print(tokenOwned);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("Value: ");
  display.setCursor(cursorX + 150, cursorY);
  display.print(valueOwned);

  cursorY = cursorY + 30;
  display.setCursor(cursorX, cursorY);
  display.print("Tokens to sell: ");


  int width = 80;
  int height = 40;
  cursorY = cursorY + 30;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print(tokensToPurchase);
  
  cursorX = cursorX + 110;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ state == SELL_TOKEN_BACK ? CYAN : BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print("BACK");
  
  cursorX = cursorX + 110;
  display.drawRect(/*x_coordinate=*/ cursorX, /*y_coordinate=*/cursorY, /*width=*/width, /*height=*/height, 
    /*color=*/ state == SELL_TOKEN_SELL ? CYAN : BLUE);
  display.setCursor(cursorX + (width/4), cursorY + (height/4));
  display.print("SELL");

  cursorY = cursorY + 50;
  cursorX = 10;
  display.setCursor(cursorX, cursorY);
  if (buySellMsg == LOADING) {
    display.print("LOADING...");
  } else if (buySellMsg == SUCCESS) {
    display.print("SUCCESS!");
  } else if (buySellMsg == ERROR_BUY) {
    display.print("ERROR: bad liquid");
  } else if (buySellMsg == ERROR_SELL) {
    display.print("ERROR: bad token num");
  }
}

void BuyScreenDisplay(int localID) {
  String tokenName = coins[localID].coinName; 
  double price= coins[localID].coinPrice; 
  double valueOwned= coins[localID].coinValue;
  double liquid = portfolio.liquid;
  
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

  cursorY = cursorY + 50;
  cursorX = 10;
  display.setCursor(cursorX, cursorY);
  if (buySellMsg == LOADING) {
    display.print("LOADING...");
  } else if (buySellMsg == SUCCESS) {
    display.print("SUCCESS!");
  } else if (buySellMsg == ERROR_BUY) {
    display.print("ERROR: bad liquid");
  } else if (buySellMsg == ERROR_SELL) {
    display.print("ERROR: bad token num");
  }
  
}

void CryptoInfoDisplay(int localID) {
  String tokenName = coins[localID].coinName; 
  double price= coins[localID].coinPrice; 
  int tokensOwned= coins[localID].tokenOwned; 
  double valueOwned= coins[localID].coinValue;
  
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
  
  // You can also write text to the display. Setting the cursor beforehand will dictate the
  // screen coordinates to start rendering the text to.
  display.setCursor(1, 1);
  display.print(text);
}
