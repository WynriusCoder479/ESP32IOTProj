#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include "ESPAsyncWebServer.h"
#include <UsingEEPROM.h>
#include <ArduinoJson.h>

#include "FS.h"
#include "SPIFFS.h"

#include <MQTT.h>

#define ledPin 2
#define MQTTPORT 1883

//Set ssidAP ans passwordAP
const char *ssidAP = "TranDangKhoaAP";
const char *passAP = "0915527900";

//Inirialize
WiFiClient net;
MQTTClient client;

AsyncWebServer serverConfig(80);
bool APCheck = true;  

//Varialble
const int ssidAdd = 0;
const int passAdd = 20;
const int hostAdd = 40;
const int usernameAdd = 60;
const int passwordAdd = 80;

const char* ssidParam = "fname";
const char* passParam = "lname";
const char* hostParam = "host";
const char* UsernameParam = "Username";
const char* PasswordParam = "Password";

String StatusState;
unsigned long timeSet;

const int potPin = 34;

//Function

boolean CheckConnection(String ssid, String pass) {
  int count = 0;

  if (ssid == " " || pass == " ") {
    Serial.println("Can't connect to Wifi");
    Serial.println("Missing ssid or password");
    return false;
  }
  else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.println("Waiting connection:");

    Serial.print("Connecting...");
    while (count < 10) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected");
        return (true);
      }
      else {
        delay(1000);
        Serial.print(".");
        count++;
      }
    }
    Serial.println("Timed out");
    return false;
  }
}


void SaveDataToEEPROM(String ssid, String pass, String host, String Username, String Password){
  if(!EEPROM.begin(512)){
    Serial.println("Failed to initialize EEPROM");
    delay(50000);
  }

  WriteEPPROM(ssid, ssidAdd);
  WriteEPPROM(pass, passAdd);
  WriteEPPROM(host, hostAdd);
  WriteEPPROM(Username, usernameAdd);
  WriteEPPROM(Password, passwordAdd);

  EEPROM.commit();
}

void ConfigApp() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  serverConfig.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/ConfigApp.html", String());
  });

  serverConfig.on("/WriteEEPROM", HTTP_GET, [](AsyncWebServerRequest * request) {
    String ssid = request->getParam(ssidParam)->value();
    String pass = request->getParam(passParam)->value();
    String host = request->getParam(hostParam)->value();
    String Username = request->getParam(UsernameParam)->value();
    String Password = request->getParam(PasswordParam)->value();

    SaveDataToEEPROM(ssid, pass, host, Username, Password);
    request->send(200, "text/plain", "Write Successful");
    ESP.restart();
  });
}

String processor(const String& var) {
  if (var == "STATE") {
    if (StatusState == "Status 2") {
      StatusState = "Status 1";
    }
    else {
      StatusState = "Status 2";
    }
    return StatusState;
  }
  return String();
}

void MessageReceived(String &topic, String &payload) {
  //Serial.println("incoming: " + topic + " - " + payload);
  
  if(payload == "LED ON"){
    digitalWrite(ledPin, 1);
  }
  else if(payload == "LED OFF"){
    digitalWrite(ledPin, 0);
  }
}

void ConnectMQTT(String username, String password) {
  Serial.print("Connecting...");

  while (!client.connect("ESP", username.c_str(), password.c_str())) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Connected");

  client.subscribe("MyLAP", 1);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  if (!EEPROM.begin(512)) {
    Serial.println("failed to initialise EEPROM");
    delay(1000000);
  }

  String ssid = ReadEEPROM(ssidAdd);
  String pass = ReadEEPROM(passAdd);
  String host = ReadEEPROM(hostAdd);
  String username = ReadEEPROM(usernameAdd);
  String password = ReadEEPROM(passwordAdd);


  if (CheckConnection(ssid.c_str(), pass.c_str())) {
    WiFi.mode(WIFI_STA);
    Serial.println("Wifi Connected");
    Serial.print("Ip:");
    Serial.println(WiFi.localIP());

    Serial.println("Waiting connect to MQTT broker");

    client.begin(host.c_str(), MQTTPORT, net);
    client.onMessage(MessageReceived);

    ConnectMQTT(username, password);

    APCheck = false;
  }
  else {
    WiFi.mode(WIFI_AP);
    Serial.println("Can't connect to Wifi");

    WiFi.softAP(ssidAP, passAP);
    IPAddress myIP = WiFi.softAPIP();

    Serial.print("AP IP address: ");
    Serial.println(myIP);

    ConfigApp();
    serverConfig.begin();
    Serial.println("HTTP server started");
    APCheck = true;
  }
}

void loop() {
  if (!APCheck) {
    StaticJsonDocument<1000> doc;

    doc["sensor"] = "Potention";
    doc["value"] = analogRead(potPin);
    doc["value1"] = random(0, 4095);
    doc["value2"] = random(0, 4095);
    doc["LedState"] = digitalRead(ledPin);

    char out[128];

    int benit = serializeJson(doc, out);

    client.loop();
    if((unsigned long)(millis()-timeSet)>500){
      timeSet = millis();
      // client.publish("MyLAP", (String)analogRead(potPin));
      client.publish("MyLAP", out);
    }
  }
}