#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define D_B = D2;
/*
Welcome Screen: 1
Order OK: 2
QR Ready: 3
Order Fail: 4
Order Retrying: 5
TXN Failed: 6
Txn Success: 7
Dispensing: 8
Thankyou: 9

*/

SoftwareSerial display(D2, D3); 

const char * ssid = "iPhone";
const char * password = "12345678";
const char * serverName = "https://api.cornercafe.shop";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

StaticJsonDocument < 130 > inputData;
JsonObject config = inputData.createNestedObject("config");
StaticJsonDocument<6000> doc;

String req_id;

void setup() {
  Serial.begin(115200);
  display.begin(19200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  pinMode(D_B, INPUT);
}

void sendToDisplay(const char * qrdata){
//  display.print('\n');
  display.print(qrdata);
  display.print('\n');
  }

void sendQR(const char * qrData, int width){
  String data = String(qrData);
  char temp;
  Serial.println(data);
  const int len = data.length();
  Serial.println(len);
  display.print("WIDTH: ");
  display.print(width);display.print('\n');
  int i,j = 0;
  unsigned int count = 0;
for(j=0; j<width; j++){
  for(i=0; i<width; i++){
   temp = data.charAt(i+(j*width));
       display.print("QR");
       display.print('X');
       display.print(i);
       display.print('Y');
       display.print(j);
       display.print('D');
       display.print(temp);
       display.print('\n');
       delay(2);
       
  }
}  
}

void sendCommand(int cmd){
  display.print("CMD");
  display.print(String(cmd));
  display.print('\n');
  }

void request_and_send_qr_to_display(){
   WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
      String httpRequestData;
      inputData["count"] = 1;
      inputData["machine_number"] = "C21";
      inputData["amount"] = 10;
      inputData["type"] = "tea";
      config["water_level"] = 60;
      config["tea_powder"] = 70;
      config["coffee_power"] = 80;
      serializeJson(inputData, httpRequestData);
      Serial.println("INPUT: " + httpRequestData);
      http.begin(client, serverName + "/tea/new");
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(httpRequestData);
      Serial.println(httpResponseCode);
      while (httpResponseCode == 200){
        int httpResponseCode = http.POST(httpRequestData);
        }
      DeserializationError error = deserializeJson(doc, http.getString());
      http.end();
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      int width = doc["size"];
      req_id = doc["request_id"];
      sendQR(doc["qr"], width);
      workflow_status = 1;
  }


void check_txn_status(){
   WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
      String link = serverName + "/payment/status/"
       http.begin(client, link + req_id);
       int httpResponseCode = http.GET();
       if (httpResponseCode != 200){
         sendCommand(4);
         return;
        }
        // Continue from here.
  }



unsigned long qr_req_time;
int workflow_status = 0;
boolean swith = false;


void loop() {
  //Send an HTTP POST request every 10 minutes
  digitalRead(D_B) ? swith = true : swith = false;
  
  if (workflow_status == 0){
     swith ? request_and_send_qr_to_display() : sendCommand(1);
     return; 
    }
   if (workflow_status == 1){
    check_txn_status();
    }
       
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
      String httpRequestData;
      inputData["count"] = 2;
      inputData["machine_number"] = "C21";
      inputData["amount"] = 10;
      inputData["type"] = "tea";
      config["water_level"] = 60;
      config["tea_powder"] = 70;
      config["coffee_power"] = 80;
      serializeJson(inputData, httpRequestData);
      Serial.println("INPUT: " + httpRequestData);
      http.begin(client, serverName);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(httpRequestData);
      Serial.println(httpResponseCode);
      DeserializationError error = deserializeJson(doc, http.getString());
      http.end();
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      int width = doc["size"];
      sendQR(doc["qr"], width);
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
