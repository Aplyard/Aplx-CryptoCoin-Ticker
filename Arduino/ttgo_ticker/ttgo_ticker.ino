//------Aplx Crypto Ticker------//

//------Libraries------//
#include <TFT_eSPI.h> 
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  

#include <Pangodream_18650_CL.h>
#include<JC_Button.h>
#include <WiFi.h>
#include <Wire.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NTPClient.h> 
#include <WiFiUdp.h>
#include <ArduinoJson.h>

//------COLORS------//
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GREY  0x18E3
#define ORANGE 0xFCA6
#define PURPLE 0x8454
#define RVNBLUE 0x3A10
#define RVNRED 0xF287

//------Battery------//
int previous_battery;
int battery;
float battery_volts;
int is_charging;
#define MIN_USB_VOL 4.9
#define ADC_PIN 34
#define CONV_FACTOR 1.7
#define READS 20
Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

//------Timing------//
unsigned long previousMillis = 0;
const long interval = 2500;

//------HTTP------//
HTTPClient http;

const int httpsPort = 443;

String url;
String btc_url = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=bitcoin&per_page=4&page=1&sparkline=false&price_change_percentage=1h%2C24h%2C30d%2C200d";
String eth_url = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=ethereum&per_page=4&page=1&sparkline=false&price_change_percentage=1h%2C24h%2C30d%2C200d";
String lnk_url = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=chainlink&per_page=4&page=1&sparkline=false&price_change_percentage=1h%2C24h%2C30d%2C200d";
String rvn_url = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=ravencoin&per_page=4&page=1&sparkline=false&price_change_percentage=1h%2C24h%2C30d%2C200d";

//------Prices------//
String coin;
String price;
String previousprice;
String price1h;
String price24h;
String price30d;
String price200d;
String previouscoin;

//------Button Pins------//
int page = 0;
int h_page = 0;
const byte
  top_btn_pin(35),
  btm_btn_pin(0);

ToggleButton
  top_btn(top_btn_pin,150),
  btm_btn(btm_btn_pin,150);

//------Bitmaps------//
#include "bat_empty.h"
#include "bat_1.h"
#include "bat_2.h"
#include "bat_3.h"
#include "bat_4.h"
#include "bat_chg.h"
#include "bat_full.h"
#include "wifi.h"

//------LED------//
#define Led 21

void setup() {
  Serial.begin(115200);                   //serial monitor baudrate

  pinoutInit();
  pinMode(Led, OUTPUT);
  top_btn.begin();                        //init the button objects
  btm_btn.begin();

  tft.init();                             //init the tft screen, set color, rotation etc
  tft.setCursor(0,20);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setFreeFont(&FreeMono9pt7b);
  delay(2000);
  tft.println("Connecting to WiFi...");     

//  WiFi.begin(ssid, password);              //start the WiFi
  WiFiManager wifiManager;
  wifiManager.setClass("invert");
  wifiManager.resetSettings();
  wifiManager.autoConnect("WiFi Manager");  
  
  Serial.print("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED)    //waiting for the WiFi setup to finish
  {
    delay(500);
    Serial.print(".");
    tft.print(".");
  }
  if (WiFi.status() == WL_CONNECTED){       //yay! you are connected
    Serial.println("connected!)");     
    tft.setTextColor(YELLOW);
    tft.println("Connected!");

    delay(1500);
    tft.fillScreen(BLACK);
  }
}

void loop() {
  
//  client.setTimeout(10000);             //this is maybe usefull on some apis (not in our case though)

  digitalWrite(Led, HIGH);                //turn on the green LED (on board) signaling WiFi is ON
  
  top_btn.read();                           //read button states
  btm_btn.read();

  battery = BL.getBatteryChargeLevel();     //read battery level (0% - 100%)
  battery_volts = BL.getBatteryVolts();     //read battery volts (3.x volts to 4.2volts and >=4.6v when charging)
  
  if(top_btn.changed()){                    //check if state has changed aka...you cliked
    if(page > 2) {
      page = 0;                             //top button scrolls the Coin Pages
    }
    else{ 
      page++;
    }
  }     
  if(btm_btn.changed()){
    if(h_page >= 1) {                       //bottom button scrolls the 1h-24h / 30d_200d pages 
      h_page = 0;
    }
    else {
      h_page++;
    }
  }

//  Serial.print("You are on page: ");      //some serial prints for debuging
//  Serial.println(page);
//  Serial.println(h_page);
   
  switch (page){                            //after reading the buttons we switch the pages to display some "non refreshable" stuff
    case 0:                                 // like the header, the wifi symbol, the line on ther bottom of the screen, etc
      coin = "0";
      if(previouscoin != coin){             //these stuff are refreshed when and only if the coin is changed, this is to reduse TFT Flicker
        tft.fillScreen(BLACK);
        tft.drawBitmap(0, 0, wifi, 27, 20, WHITE);
        tft.setTextColor(WHITE);
        tft.setFreeFont(&FreeSans9pt7b);
        tft.setCursor(80, 30);
        tft.print("BTC/USD");
        tft.fillRect(0, 105, 240, 1, WHITE); 
        url = btc_url;                      //at this point we adress the url to look at our 1st coin's url
        previouscoin = coin;                //we set coin to previouscoin for above reasons
        previous_battery = 0;               //we init the bat checker for a switch case later on
      }
      break;
    case 1:                                 //all as mentioned above for case:0
      coin = "1";
      if(previouscoin != coin){      
        tft.fillScreen(BLACK);
        tft.drawBitmap(0, 0, wifi, 27, 20, WHITE);
        tft.setTextColor(WHITE);
        tft.setFreeFont(&FreeSans9pt7b);
        tft.setCursor(80, 30);
        tft.print("ETH/USD");
        tft.fillRect(0, 105, 240, 1, WHITE);
        url = eth_url;   
        previouscoin = coin;
        previous_battery = 0;
      }
      break;  
    case 2:
      coin = "2";
      if(previouscoin != coin){      
        tft.fillScreen(BLACK);
        tft.drawBitmap(0, 0, wifi, 27, 20, WHITE);
        tft.setTextColor(WHITE);
        tft.setFreeFont(&FreeSans9pt7b);
        tft.setCursor(80, 30);
        tft.print("LNK/USD");
        tft.fillRect(0, 105, 240, 1, WHITE);
        url = lnk_url;   
        previouscoin = coin;
        previous_battery = 0;
      }
      break;
    case 3:
      coin = "3";
      if(previouscoin != coin){      
        tft.fillScreen(BLACK);
        tft.drawBitmap(0, 0, wifi, 27, 20, WHITE);
        tft.setTextColor(WHITE);
        tft.setFreeFont(&FreeSans9pt7b);
        tft.setCursor(80, 30);
        tft.print("RVN/USD");
        tft.fillRect(0, 105, 240, 1, WHITE);
        url = rvn_url;   
        previouscoin = coin;
        previous_battery = 0;
      }
      break;      
  }
        
  unsigned long currentMillis = millis();           //we set a mills() kind of delay for the HTTP Get every 2500ms
                                                    //so everything bellow here is happening every "interval"
  if (currentMillis - previousMillis >= interval) { //this is cause http & deserialazation fail if we dont implement a delay more than 2500ms
                                                    //We dont use a delay here cause we dont want to mess with our button state checker above
    previousMillis = currentMillis;
  
    http.begin(url);                                //start the http
    int httpCode = http.GET();
    StaticJsonDocument<2000> doc;                   //create a JSON to store our Get values
    DeserializationError error = deserializeJson(doc, http.getString());    //store the values to our JSON

    if (error) {                                    //print somehting is case of error
      Serial.print(F("deserializeJson Failed: "));
      Serial.println(error.f_str());
      return;
    }
  
//    Serial.print("HTTP Status Code: ");             //some serial prints for debuging
//    Serial.println(httpCode);
                                                      //here we navigate in our json and we are storing our valuse in some strings
    String price = doc[0]["current_price"].as<String>();
    String price1h = doc[0]["price_change_percentage_1h_in_currency"].as<String>();
    String price24h = doc[0]["price_change_percentage_24h_in_currency"].as<String>();
    String price30d = doc[0]["price_change_percentage_30d_in_currency"].as<String>();
    String price200d = doc[0]["price_change_percentage_200d_in_currency"].as<String>();
    
    http.end();                                       //stop the http
    
    Serial.println(price);                            //some serial prints for debuging
//    Serial.println(price1h); 
//    Serial.println(price24h);
//    Serial.println(price30d);   
//    Serial.println(price200d);
    
    switch (page){                                    //switch case that happens every interval to check and print a new price       
      case 0:
        if(price != previousprice){
          tft.fillRect(0, 31, 240, 73, BLACK);
          tft.setTextColor(WHITE);
          tft.setFreeFont(&FreeSans18pt7b);
          tft.setCursor(70,80);                         
          tft.print(price);
          previousprice = price;
        }
        break;
      case 1:
        if(price != previousprice){
          tft.fillRect(0, 31, 240, 73, BLACK);
          tft.setTextColor(WHITE);
          tft.setFreeFont(&FreeSans18pt7b);
          tft.setCursor(70,80);                         
          tft.print(price);
          previousprice = price;
        }
        break;
      case 2: 
        if(price != previousprice){
          tft.setTextColor(WHITE);
          tft.fillRect(0, 31, 240, 73, BLACK);
          tft.setFreeFont(&FreeSans18pt7b);
          tft.setCursor(70,80);                         
          tft.print(price);
          previousprice = price;
        }
        break;
      case 3: 
        if(price != previousprice){
          tft.setTextColor(WHITE);
          tft.fillRect(0, 31, 240, 73, BLACK);
          tft.setFreeFont(&FreeSans18pt7b);
          tft.setCursor(70,80);                         
          tft.print(price.substring(0,5));
          previousprice = price;
        }
        break;  
    }
    
    switch(h_page){                                //switch case that happens every interval to check the bottom button and display
      case 0:                                      // the approptiate 1h 24h or 30d 200d change
          tft.fillRect(0, 107, 240, 28, BLACK);
          tft.setCursor(10,128);
          tft.setFreeFont(&FreeSansBold9pt7b);
          tft.print("1h ");
          tft.setFreeFont(&FreeSans9pt7b);
          tft.print(price1h.substring(0, 5));
          tft.setCursor(140,128);
          tft.setFreeFont(&FreeSansBold9pt7b);
          tft.print("24h ");
          tft.setFreeFont(&FreeSans9pt7b);
          tft.print(price24h.substring(0, 5));
        break;
      case 1:
          tft.fillRect(0, 107, 240, 28, BLACK);
          tft.setCursor(10,128);
          tft.setFreeFont(&FreeSansBold9pt7b);
          tft.print("30d ");
          tft.setFreeFont(&FreeSans9pt7b);
          tft.print(price30d.substring(0, 5));
          tft.setCursor(140,128); 
          tft.setFreeFont(&FreeSansBold9pt7b);
          tft.print("200d ");
          tft.setFreeFont(&FreeSans9pt7b);
          tft.print(price200d.substring(0, 5));
        break;
    }
    
    if(previous_battery != battery){              //battery checks and displays
      if(battery_volts >= 4.6){
        tft.fillRect(206, 0, 34, 20, BLACK);
        tft.drawBitmap(206, 0, bat_chg, 34, 20, WHITE); 
      }
      else{
        if(battery <= 10){
        tft.fillRect(206, 0, 34, 20, BLACK);
        tft.drawBitmap(206, 0, bat_empty, 34, 20, WHITE);  
        previous_battery = battery;
        }
        else if(battery > 10 || battery <= 30){
        tft.fillRect(206, 0, 34, 20, BLACK);
        tft.drawBitmap(206, 0, bat_1, 34, 20, WHITE);  
        previous_battery = battery;
        }
        else if(battery > 30 || battery <= 50){
        tft.fillRect(206, 0, 34, 20, BLACK);
        tft.drawBitmap(206, 0, bat_2, 34, 20, WHITE);  
        previous_battery = battery;
        }
        else if(battery > 50 || battery <= 70){
        tft.fillRect(206, 0, 34, 20, BLACK);
        tft.drawBitmap(206, 0, bat_3, 34, 20, WHITE);  
        previous_battery = battery;
        }
        if(battery > 70 || battery <= 90){
        tft.fillRect(206, 0, 34, 20, BLACK);
        tft.drawBitmap(206, 0, bat_4, 34, 20, WHITE);  
        previous_battery = battery;
        }
        if(battery > 90){
        tft.fillRect(206, 0, 34, 20, BLACK);
        tft.drawBitmap(206, 0, bat_full, 34, 20, WHITE);  
        previous_battery = battery;
        }  
      } 
    }  
  }
}

void pinoutInit(){          //this is something that has to do with the onboard battery charge circuit.
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
}
