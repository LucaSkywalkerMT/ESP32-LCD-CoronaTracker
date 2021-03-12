#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <ESP32Ping.h>
#include <Wire.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Button2.h>
#include "esp_wifi.h"
#include "esp_adc_cal.h"

//Settings////////////////////////////////////////////////////////////////////////////////////////////////
int BUTTON_1         = 35;
int BUTTON_2         = 0;
int vref             = 1100;
int adc_refresh      = 500;
int covid_refresh    = 1 * 60 * 60 * 1000;
const char* ssid     = "WhitePower";
const char* password = "12345666";
const char* MT = "https://services1.arcgis.com/0MSEUqKaxRlEPj5g/arcgis/rest/services/Coronavirus_2019_nCoV_Cases/FeatureServer/1/query?where=Country_Region%20like%20'%25MALTA%25'&outFields=Last_Update,Confirmed,Deaths,Recovered&returnGeometry=false&outSR=4326&f=json";
const char* ZA = "https://services1.arcgis.com/0MSEUqKaxRlEPj5g/arcgis/rest/services/Coronavirus_2019_nCoV_Cases/FeatureServer/1/query?where=Country_Region%20like%20'%25SOUTH%20AFRICA%25'&outFields=Last_Update,Confirmed,Deaths,Recovered&returnGeometry=false&outSR=4326&f=json";
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
TFT_eSPI tft = TFT_eSPI(135, 240);
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
int adcMode = true;
String world_80_country_code =  "MT";
int world_80_confirmed;
int world_80_death;
int world_80_cured;
String world_66_country_code =  "ZA";
int world_66_confirmed;
int world_66_death;
int world_66_cured;
static uint64_t timeStamp = 0;
void setup() {
  initTFT();
  initButtons();
  initADC();
  Serial.begin(115200);
  connectWiFi();
  doOTA();
}
void loop() {
  ArduinoOTA.handle();
  readButtons();
  showVoltageTemp();
  GetCovidCount();
}
void initTFT() {
#define TFT_INVERSION_ON
#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif
#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif
#define TFT_MOSI        19
#define TFT_SCLK        18
#define TFT_CS          5
#define TFT_DC          16
#define TFT_RST         23
#define TFT_BL          4
#define ADC_EN          14
#define ADC_PIN         34
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  if (TFT_BL > 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  }
  tft.setSwapBytes(true);
}
void initADC() {
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_6, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    vref = adc_chars.vref;
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP);
}
void initButtons() {
  btn1.setLongClickHandler([](Button2 & b) {
    //do this when btn1 is long pressed
  });
  btn2.setLongClickHandler([](Button2 & b) {
    //do this when btn2 is long pressed
  });
  btn1.setPressedHandler([](Button2 & b) {
    //do this when btn1 is pressed
  });
  btn2.setPressedHandler([](Button2 & b) {
    //do this when btn2 is pressed
  });
}
void readButtons() {
  btn1.loop();
  btn2.loop();
}
void espDelay(int ms) {
  esp_sleep_enable_timer_wakeup(ms * 1000);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_light_sleep_start();
}
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps (WIFI_PS_NONE);
  WiFi.begin(ssid, password);
  int timeoutcount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (timeoutcount > 10) {
      delay(1000);
      WiFi.begin(ssid, password);
      timeoutcount = 0;
    }
    timeoutcount++;
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);
  String IP = IpAddress2String(WiFi.localIP());
  tft.drawString(IP, 0, 3);
  tft.drawLine(0 , 20, tft.width(), 20 , TFT_WHITE);
}
void doOTA() {
  ArduinoOTA.setHostname("ESP32");
  ArduinoOTA.setPassword("");
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}
void showVoltageTemp() {
  static uint64_t timeStamp = 0;
  if (millis() - timeStamp > adc_refresh) {
    timeStamp = millis();
    uint16_t v = analogRead(ADC_PIN);
    float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    String voltage = String(battery_voltage) + " V";
    uint8_t temp_farenheit;
    float temp_celsius;
    temp_farenheit = temprature_sens_read();
    temp_celsius = ( temp_farenheit - 32 ) / 1.8;
    String cels = String(temp_celsius) + " C";
    tft.fillRoundRect(190, 0, 50, 20, 0, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(voltage,  tft.width(), 0);
    double perc = (battery_voltage - 3) / 1.2 * 100;
    String Perc = String(perc) + "%";
    tft.drawString(cels, tft.width(), 10);
  }
}
String IpAddress2String(const IPAddress& ipAddress) {
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}
void GetCovidCount() {
  if (millis() > timeStamp) {
    timeStamp = millis() + covid_refresh;
    HTTPClient httpMT;
    httpMT.begin(MT);
    HTTPClient httpZA;
    httpZA.begin(ZA);
    int httpCodeMT = httpMT.GET();
    int httpCodeZA = httpZA.GET();
    if (httpCodeMT > 0 && httpCodeZA > 0) {
      String payloadMT = httpMT.getString();
      String payloadZA = httpZA.getString();
      char charBufMT[500];
      char charBufZA[500];
      payloadMT.toCharArray(charBufMT, 500);
      payloadZA.toCharArray(charBufZA, 500);
      const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 3 * JSON_OBJECT_SIZE(6) + 2 * JSON_OBJECT_SIZE(7) + 690;
      DynamicJsonDocument doc(capacity);
      deserializeJson(doc, payloadMT);
      JsonArray fields = doc["fields"];
      JsonObject features_0_attributes = doc["features"][0]["attributes"];
      world_80_confirmed = features_0_attributes["Confirmed"];
      world_80_death = features_0_attributes["Deaths"];
      world_80_cured = features_0_attributes["Recovered"];
      deserializeJson(doc, payloadZA);
      fields = doc["fields"];
      features_0_attributes = doc["features"][0]["attributes"];
      world_66_confirmed = features_0_attributes["Confirmed"];
      world_66_death = features_0_attributes["Deaths"];
      world_66_cured = features_0_attributes["Recovered"];
    }
    httpMT.end();
    httpZA.end();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(world_80_country_code), tft.width() / 3, 40);
    tft.drawString(String(world_66_country_code), 2 * tft.width() / 3, 40);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString(String(world_80_confirmed), tft.width() / 3, 60);
    tft.drawString(String(world_66_confirmed), 2 * tft.width() / 3, 60);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString(String(world_80_death), tft.width() / 3, 80);
    tft.drawString(String(world_66_death), 2 * tft.width() / 3, 80);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString(String(world_80_cured), tft.width() / 3, 100);
    tft.drawString(String(world_66_cured),  2 * tft.width() / 3, 100);
  }
}
