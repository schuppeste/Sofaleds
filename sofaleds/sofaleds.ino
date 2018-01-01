/*
  Idea by Schuppeste@googlemail.com
  Sources: https://github.com/schuppeste/Sofaleds
  Space: http://www.hackaday.io/Schuppeste
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "FS.h"
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#include <FastLED.h>
#define NUM_LEDS 150
#define DATA_PIN 4 //D4 /GPIO2
int ledcount = 0;
int mysegs = 0;

CRGB leds[NUM_LEDS];
const char signMessage[] PROGMEM  = {};
ESP8266WebServer server(80);
int mainconfig[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t colors[8][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
uint8_t band[8] = {0, 0, 0, 0, 0, 0, 0};
String ssid = "";
String key = "";
String ip = "";
int strobe = D1; // strobe pins on digital 4
int res = D2; // reset pins on digital 5
int all[7]; // store band values in these arrays
int bands;
int options = 0;
IPAddress local_IP(192, 168, 4, 22);
IPAddress gateway(192, 168, 4, 9);
IPAddress subnet(255, 255, 255, 0);
boolean startAP = false;
int mysegleds = 0;
void setup() {
  pinMode(res, OUTPUT);
  pinMode(strobe, OUTPUT);
  digitalWrite(res, HIGH);
  digitalWrite(res, LOW);
  int apstatus = digitalRead(5);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  Serial.begin(115200);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
  }
  else if (loadConfig()) {
  }
  WiFi.hostname("sofaleds");
  if (ssid == "" || key == "" || apstatus == LOW) {
    WiFi.softAP("Sofaheld", "sofaheld"); //Create Access Point
    WiFi.softAPConfig(local_IP, gateway, subnet);
  } else {
    WiFi.begin((const char*)ssid.c_str(), (const char*)key.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }
  server.on("/", handleRoot);
  server.on("/index.html", handleRoot);
  server.serveStatic("/config.json", SPIFFS, "/config.json");
  server.on("/live.json", handlelivejson);
  server.on("/json", handleJson);
  server.begin(); //Start the server
  startAP = true;
  ledcount = mainconfig[0];
  mysegs = mainconfig[1];
  mysegleds = ledcount / mysegs;
  if (mainconfig[3] >= mainconfig[4] || mainconfig[4] < 500 || mainconfig[4] == 0)
  {
    mainconfig[3] = 0;
    mainconfig[4] = 1024;
  }
}
void loop() {
  server.handleClient();
  delay(20);//Handling of incoming requests
  readMSGEQ7();
  // display values of left channel on serial monitor
  if (mainconfig[2] == 2) { //Ambient One Color
    for (int h = 0; h < ledcount; h++) {
      leds[h][0] = colors[7][0];
      leds[h][1] = colors[7][1];
      leds[h][2] = colors[7][2];
      leds[h] %= mainconfig[5];
    }
  } else
    for (int i = 0; i < mysegs; i++) {
      int myvalue = map(all[band[i]], mainconfig[3], mainconfig[4], 0, mysegleds);

      if (mainconfig[2] == 0) { //Equalizer
        for (int h = 0; h < mysegleds; h++) {
          if (h < myvalue) {
            leds[i * mysegleds + h][0] = colors[i][0];
            leds[i * mysegleds + h][1] = colors[i][1];
            leds[i * mysegleds + h][2] = colors[i][2];

          } else
          {
            leds[i * mysegleds + h][0] = 0;
            leds[i * mysegleds + h][1] = 0;
            leds[i * mysegleds + h][2] = 0;
          }
        }
      } else if (mainconfig[2] == 1) { //Lichtorgel
        for (int h = 0; h < mysegleds; h++) {
          leds[i * mysegleds + h][0] = colors[i][0];
          leds[i * mysegleds + h][1] = colors[i][1];
          leds[i * mysegleds + h][2] = colors[i][2];
          leds[i] %= myvalue;

        }
      }

   

    }
    
      FastLED.show();
}
void handleRoot() {
  if (server.arg("network") == "") {   //Parameter not found
  } else {    //Parameter found
    ssid = const_cast<char*>(server.arg("ssid").c_str());
    key = const_cast<char*>(server.arg("key").c_str());
    ip = const_cast<char*>(server.arg("ip").c_str());
    saveConfig();
  }
  if (server.arg("speichern") == "") {
    File configFile = SPIFFS.open("/indexlive.html", "r");
    if (!configFile) {
      Serial.println("Failed to open config file");
    }
    size_t sent = server.streamFile(configFile, "text/html");
    configFile.close();
  } else {
    mainconfig[0] = server.arg("ledcount").toInt();
    mainconfig[1] = server.arg("segs").toInt();
    mainconfig[2] = server.arg("mod").toInt();
    mainconfig[3] = server.arg("min").toInt();
    mainconfig[4] = server.arg("max").toInt();
    mainconfig[5] = server.arg("dim").toInt();
    hex_to_rgb(colors[0], server.arg("c1"));
    hex_to_rgb(colors[1], server.arg("c2"));
    hex_to_rgb(colors[2], server.arg("c3"));
    hex_to_rgb(colors[3], server.arg("c4"));
    hex_to_rgb(colors[4], server.arg("c5"));
    hex_to_rgb(colors[5], server.arg("c6"));
    hex_to_rgb(colors[6], server.arg("c7"));
    hex_to_rgb(colors[7], server.arg("c8"));
    band[0] = server.arg("z1").toInt();
    band[1] = server.arg("z2").toInt();
    band[2] = server.arg("z3").toInt();
    band[3] = server.arg("z4").toInt();
    band[4] = server.arg("z5").toInt();
    band[5] = server.arg("z6").toInt();
    band[6] = server.arg("z7").toInt();

    ledcount = mainconfig[0];
    mysegs = mainconfig[1];
    mysegleds = ledcount / mysegs;
    if (server.arg("test") == "") {   //Parameter not found
      File configFile = SPIFFS.open("/indexlive.html", "r");
      if (!configFile) {
        Serial.println("Failed to open config file");
      }
      size_t sent = server.streamFile(configFile, "text/html");
      configFile.close();
    } else {
      saveConfig();
      File configFile = SPIFFS.open("/index.html", "r");
      if (!configFile) {
        Serial.println("Failed to open config file");
      }
      size_t sent = server.streamFile(configFile, "text/html");
      configFile.close();
    }
  }
}
void handleJson() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
  }
  size_t size = configFile.size();

  if (size > 3000) {
    Serial.println("Config file size is too large");
  }
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  StaticJsonBuffer<3000> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  if (!json.success()) {
    Serial.println("Failed to parse config file");
  }
  String jsonStr;
  json.printTo(jsonStr);
  server.send(200, "text/html", jsonStr);
  configFile.close();
}
void handlelivejson() {
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& array4 = root.createNestedArray("mainconfig");
  array4.copyFrom(mainconfig);
  JsonArray& array1 = root.createNestedArray("colors");
  array1.copyFrom(colors);
  JsonArray& array2 = root.createNestedArray("band");
  array2.copyFrom(band);
  root["ssid"] = ssid;
  root["key"] = key;
  root["ip"] = ip;
  String jsonStr = "";
  root.printTo(jsonStr);
  server.send(200, "text/html", jsonStr);
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }
  size_t size = configFile.size();
  if (size > 3000) {
    Serial.println("Config file size is too large");
    return false;
  }
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  StaticJsonBuffer<3000> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  for (int i = 0; i < 7; i++)
  json["colors"][i].as<JsonArray>().copyTo(colors[i]);
  json["band"].as<JsonArray>().copyTo(band);
  json["mainconfig"].as<JsonArray>().copyTo(mainconfig);
  ssid = json["ssid"].as<String>();
  key = json["key"].as<String>();
  ip = json["ip"].as<String>();
  configFile.close();
  return true;
}

bool saveConfig() {
  StaticJsonBuffer<2000> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& array4 = root.createNestedArray("mainconfig");
  array4.copyFrom(mainconfig);
  JsonArray& array1 = root.createNestedArray("colors");
  array1.copyFrom(colors);
  JsonArray& array2 = root.createNestedArray("band");
  array2.copyFrom(band);
  root["ssid"] = ssid;
  root["key"] = key;
  root["ip"] = ip;
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  root.printTo(configFile);
  configFile.close();
  return true;
}
void hex_to_rgb(uint8_t *data, String hexstring) {
  long number = strtol( &hexstring[1], NULL, 16);
  long r = number >> 16;
  long g = number >> 8 & 0xFF;
  long b = number & 0xFF;
  data[0] = r;
  data[1] = g;
  data[2] = b;
}
void readMSGEQ7()
// Function to read 7 band equalizers
{
  digitalWrite(res, HIGH);
  digitalWrite(res, LOW);
  for (bands = 0; bands < 7; bands++)
  {
    digitalWrite(strobe, LOW); // strobe pin on the shield - kicks the IC up to the next band
    delayMicroseconds(30); //
    all[bands] = analogRead(A0); // store left band reading
    digitalWrite(strobe, HIGH);
  }
}