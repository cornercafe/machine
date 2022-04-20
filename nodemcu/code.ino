#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

SoftwareSerial display(2, 3); 

const char * ssid = "AKHIL";
const char * password = "123456789";

//Your Domain name with URL path or IP address with path
const char * serverName = "https://api.cornercafe.shop/tea/new";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

StaticJsonDocument < 130 > inputData;
JsonObject config = inputData.createNestedObject("config");
StaticJsonDocument<6000> doc;

void setup() {
  Serial.begin(115200);
  display.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

}


void sendQR(const char * qrData, int width){
  String data = String(qrData);
  char temp;
  Serial.println(data);
  const int len = data.length();
  Serial.println(len);
  display.print('WIDTH: ');
  display.print(width);
  int i,j = 0;
  unsigned int count = 0;
for(j=0; j<width; j++){
  for(i=0; i<width; i++){
   temp = data.charAt(i+(j*width));
       display.print('QR');
       display.print('X');
       display.print(i);
       display.print('Y');
       display.print(j);
       display.print('D');
       display.print(temp);
  }
}  
}



void displayTxnSuccess(){
  display.print('TXN SUCC');
}

void displayTxnFailed(){
  display.print('TXN FAIL');
}

void displayNewOrder(){
  display.print('NEW TEA ');
}

void displayOrderReady(){
  display.print('ORDER OK');
}

void displayThanks(){
  display.print('ORDER DN');
}

void displayWelcome(){
  display.print('WELCOME ');
}


void loop() {
  //Send an HTTP POST request every 10 minutes
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

      http.begin(client, serverName);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(httpRequestData);
      String response = http.getString();
      http.end();

      DeserializationError error = deserializeJson(doc, response);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      const char * qr = doc["qr"];
      int width = doc["size"];
      sendQR(qr, width);

    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
