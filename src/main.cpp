#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include "ESPAsyncWebServer.h"
#include <UsingEEPROM.h>
#include <ArduinoJson.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


#include "FS.h"
#include "SPIFFS.h"

#include <MQTT.h>

#define lamp1Pin 2
#define lamp2Pin 21
#define fanPin 19
#define fanSpeed 18
#define DHTPIN 4
#define gasPin 34
#define gasStatusPin 35

#define MQTTPORT 1883

#define DHTTYPE DHT11

//Set ssidAP ans passwordAP
const char *ssidAP = "TranDangKhoaAP";
const char *passAP = "0915527900";

//Inirialize
WiFiClient net;
MQTTClient client;

AsyncWebServer serverConfig(80);
bool APCheck = true;  

DHT dhtSensor(DHTPIN, DHTTYPE);

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

unsigned long timeSet;

const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

int dutyCycle;

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

void MessageReceived(String &topic, String &payload) {
  //Serial.println("incoming: " + topic + " - " + payload);
  
  if(payload == "LAMP1 ON"){
    digitalWrite(lamp1Pin, 1);
    Serial.println(payload);
  }
  else if(payload == "LAMP1 OFF"){
    digitalWrite(lamp1Pin, 0);
    Serial.println(payload);
  }
  if(payload == "LAMP2 ON"){
    digitalWrite(lamp2Pin, 1);
    Serial.println(payload);
  }
  else if(payload == "LAMP2 OFF"){
    digitalWrite(lamp2Pin, 0);
    Serial.println(payload);
  }

  if(payload == "FAN ON"){
    digitalWrite(fanPin, 1);
    Serial.println(payload);
  }
  else if(payload == "FAN OFF"){
    digitalWrite(fanPin, 0);
    Serial.println(payload);
    dutyCycle = 0;
  }

  if(payload != "LAMP1 ON" && payload != "LAMP1 OFF"&&payload != "LAMP2 ON" && payload != "LAMP2 OFF" && payload != "FAN ON"&&payload != "FAN OFF") {
    if(digitalRead(fanPin)){
      dutyCycle = payload.toInt();
      Serial.println(payload);

    } 
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
  pinMode(lamp1Pin, OUTPUT);
  pinMode(lamp2Pin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(fanSpeed, OUTPUT);
  pinMode(DHTPIN, OUTPUT);
  pinMode(gasPin, INPUT);
  pinMode(gasStatusPin, INPUT);
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(fanSpeed, ledChannel);
  dhtSensor.begin();

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
    float hum = dhtSensor.readHumidity();
    float heat = dhtSensor.readTemperature();
    float gas = analogRead(gasPin);
    int  gasStatus= digitalRead(gasStatusPin);
    ledcWrite(ledChannel, dutyCycle);

    doc["Temp"]= heat;
    doc["Hum"]=hum;
    doc["Gas"]= gas;
    doc["GasStatus"]=gasStatus;
    doc["Lamp1"]=digitalRead(lamp1Pin);
    doc["Lamp2"]=digitalRead(lamp2Pin);
    doc["Fan"]=digitalRead(fanPin);
    doc["FanSpeed"]= dutyCycle;

    char out[128];

    serializeJson(doc, out);

    client.loop();
    if((unsigned long)(millis()-timeSet)>1000){
       timeSet = millis();
        client.publish("CS", out);
    }
  }
}