#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#define D_B  D0
#define O_R D5
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
  QR Finished: 10

*/

#define SHOW_WELCOME_SCREEN 1
#define SHOW_ORDER_OK 2
#define SHOW_QR_READY 3
#define SHOW_ORDER_FAIL 4
#define SHOW_ORDER_RETRYING 5
#define SHOW_TXN_FAILED 6
#define SHOW_TXN_SUCCESS 7
#define SHOW_DISPENSING 8
#define SHOW_THANK_YOU 9
#define SHOW_QR_FINISHED 10
#define SHOW_WAITING_FOR_QR 11
#define SHOW_CONNECTING_TO_WIFI 12



const char * ssid = "AKHIL";
const char * password = "123456789";
String serverName = "https://api.cornercafe.shop";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;
int workflow_status = 0;
unsigned long timeOut = 0;

StaticJsonDocument < 130 > inputData;
JsonObject config = inputData.createNestedObject("config");
StaticJsonDocument<6000> doc;




String qr_data;
String req_id;
String httpRequestData;
String url;

int last_command = 0;
void sendCommand(int cmd) {
  if (last_command == cmd) return;
  last_command = cmd;
  Serial.print("CMD");
  Serial.print(String(cmd));
  Serial.print('\n');
  Serial.println("CMD" + String(cmd));
}

void setup() {
//  digitalWrite(O_R, HIGH);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    sendCommand(SHOW_CONNECTING_TO_WIFI);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  pinMode(D_B, INPUT_PULLUP);
  pinMode(O_R, OUTPUT);
  sendCommand(SHOW_QR_FINISHED);

}

void sendToDisplay(const char * qrdata) {
  Serial.print(qrdata);
  Serial.print('\n');
}



void sendQR(String qrData, int width) {
  char temp;

  const int len = qrData.length();
  Serial.print("WIDTH: ");
  Serial.print(width); Serial.print('\n');
  int i, j = 0;
  unsigned int count = 0;
  sendCommand(SHOW_QR_READY);
  for (j = 0; j < width; j++) {
    for (i = 0; i < width; i++) {
      temp = qrData.charAt(i + (j * width));
      Serial.print("QR");
      Serial.print('X');
      Serial.print(i);
      Serial.print('Y');
      Serial.print(j);
      Serial.print('D');
      Serial.print(temp);
      Serial.print('\n');
      delay(2);

    }
  }
  sendCommand(SHOW_QR_FINISHED);
  qrData = "";
}


void request_and_send_qr_to_display() {
  sendCommand(SHOW_WAITING_FOR_QR);
  Serial.println("Request and send QR");
  Serial.print("MEM: ");
  Serial.println(doc.memoryUsage());
  WiFiClientSecure client;
    client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  inputData["count"] = 1;
  inputData["machine_number"] = "C21";
  inputData["amount"] = 10;
  inputData["type"] = "tea";
  config["water_level"] = 60;
  config["tea_powder"] = 70;
  config["coffee_power"] = 80;
  serializeJson(inputData, httpRequestData);
  Serial.println("INPUT: " + httpRequestData);
  url = serverName + "/tea/new";
  http.begin(client, url );
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(httpRequestData);
  Serial.println(httpResponseCode);
  if (httpResponseCode != 200) {
    http.end();
    client.stop();
    request_and_send_qr_to_display();
  }
  Serial.println("Fetched API");
  DeserializationError error = deserializeJson(doc, http.getString());
  http.end();
  client.stop();
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  } else {
    Serial.println("Deserialized.");
  }
  int width = doc["size"];
  req_id = doc["request_id"].as<String>();
  Serial.println("Req ID: " + req_id);
  Serial.print("MEM: ");
  Serial.println(doc.memoryUsage());
  sendQR(doc["qr"], width);
  doc.clear();
  workflow_status = 1;
}

int testCount = 0;
int statusCount = 0;
void check_txn_status() {
  
  statusCount++;
  Serial.print(statusCount);
  WiFiClientSecure client;
    client.setInsecure();
  HTTPClient http;
  url = serverName + "/payment/status/";
  http.begin(client, url + req_id);
  int httpResponseCode = http.GET();
  if (httpResponseCode != 200) {
    sendCommand(SHOW_ORDER_FAIL);
    return;
  }
  DeserializationError error = deserializeJson(doc, http.getString());
  http.end();
  client.stop();
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (doc["status"] == "PENDING") {
    if (timeOut == 0)
      timeOut = millis();
    return;
  }
  if (doc["status"] == "ERROR") {
    timeOut = 0;
    return;
  }
  if (doc["status"] == "SUCCESS") {
    timeOut = 0;
    workflow_status = 2;
    sendCommand(SHOW_TXN_SUCCESS);
    return;
  }
  doc.clear();
}

unsigned long qr_req_time;
boolean swith = false;


void loop() {
  digitalRead(D_B) == LOW ? swith = true : swith = false;
  //  Serial.println("Workflow Status: " + String(workflow_status));
  //  Serial.println("Time Out: " + String(timeOut));
  //  swith ? Serial.println("Switch: HIGH") : Serial.println("Switch: LOW");

  if (workflow_status == 0) {
    swith ? request_and_send_qr_to_display() : sendCommand(SHOW_WELCOME_SCREEN);
    return;
  }
  if (workflow_status == SHOW_WELCOME_SCREEN) {
    check_txn_status();
    if (((millis() - timeOut) > 100000) && timeOut > 0) {
      Serial.println("TIMED OUT");
      sendCommand(SHOW_TXN_FAILED);
      timeOut = 0;
      workflow_status = 0;
      delay(1000);
      swith = false;
      ESP.restart();
      return;
    }

  }
  if (workflow_status == 2) {
    statusCount = 0;
    if (swith) {
      sendCommand(SHOW_DISPENSING);
      digitalWrite(O_R, HIGH);
      delay(500);
      digitalWrite(O_R, LOW);
      sendCommand(SHOW_THANK_YOU);
      workflow_status = 0;
      swith = false;
      ESP.restart();
    }
  }

}
