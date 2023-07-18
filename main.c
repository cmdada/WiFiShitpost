#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"

#define ADC_PIN 34
#define ADC_EN 14  //ADC_EN is the ADC detection enable port

const byte DNS_PORT = 53;
IPAddress apIP(8,8,4,4); // The default android DNS
DNSServer dnsServer;
AsyncWebServer server(80);
int vref = 1100;

void setup() {
  Serial.begin(115200);
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP("wifi");  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  pinMode(ADC_EN, OUTPUT);
  digitalWrite(ADC_EN, HIGH);

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint16_t v = analogRead(ADC_PIN);
    float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);

    File file = SPIFFS.open("/count.txt");
    String count;
    if(!file){
      count = "uwu: 0";
    } else {
      count = "uwu: " + file.readString();
    }
    file.close();

    String responseHTML =
    "<!DOCTYPE html><html><head><title>uwu</title>"
    "<style>"
    "body { background-color: #fff; font-family: Arial, Helvetica, sans-serif; color: #333;}"
    "h1 { color: #F68B1F; }"
    "p { padding: 5px 0; }"
    "form { margin: 20px 0;}"
    ".container { margin: 0 auto; max-width: 600px; padding: 20px; text-align: center; border: 1px solid #ddd; border-radius: 10px;}"
    "</style>"
    "</head><body>"
    "<div class='container'>"
    "<h1>uwu</h1>"
    "<p>This is a wifi network.</p>"
    "<p>Battery Voltage :" + String(battery_voltage) + "V</p>"
    "<p>" + count + "</p>"
    "<form action=\"/login\" method=\"POST\"><input type=\"submit\" value=\"Login\" style='padding:10px;'></form>"
    "</div>"
    "</body></html>";

    request->send(200, "text/html", responseHTML);
  });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    File file = SPIFFS.open("/count.txt", FILE_READ);
    int count;
    if(!file){
      count = 0;
    } else {
      count = file.parseInt();
    }
    file.close();

    count++;

    file = SPIFFS.open("/count.txt", FILE_WRITE);
    if(!file){
      Serial.println("There was an error opening the file for writing");
      return;
    }
    file.print(count);
    file.close();

    String responseHTML =
    "<!DOCTYPE html><html><head><title>Login Page</title>"
    "<style>"
    "body { background-color: #fff; font-family: Arial, Helvetica, sans-serif; color: #333;}"
    "h1 { color: #F68B1F; }"
    "p { padding: 5px 0; }"
    "form { margin: 20px 0;}"
    ".container { margin: 0 auto; max-width: 600px; padding: 20px; text-align: center; border: 1px solid #ddd; border-radius: 10px;}"
    "</style>"
    "</head><body>"
    "<div class='container'>"
    "<h1>Not a Real Network</h1>"
    "<p>This isn't a real wifi network and even if it was, you shouldn't connect to random wifi networks. Bad actors can set up "
    "fraudulent wifi networks to trick you into connecting. "
    "</p>"
    "<p>UwU " + String(count) + " times.</p>"
    "</div></body></html>";

    request->send(200, "text/html", responseHTML);
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("/");
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
}
