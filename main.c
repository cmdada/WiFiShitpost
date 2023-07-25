#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"

#define ADC_PIN 34
#define ADC_EN 14

const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 4, 4); // Default Android DNS
DNSServer dnsServer;
AsyncWebServer server(80);
int vref = 1100;

void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP("WiFi Network");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  pinMode(ADC_EN, OUTPUT);
  digitalWrite(ADC_EN, HIGH);

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint16_t voltage = analogRead(ADC_PIN);
    float batteryVoltage = ((float)voltage / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);

    File file = SPIFFS.open("/count.txt");
    String count;
    if (!file) {
      count = "Count: 0";
    } else {
      count = "Count: " + file.readString();
    }
    file.close();

    String responseHTML = R"(
      <!DOCTYPE html>
      <html>
        <head>
          <title>WiFi Network</title>
          <style>
            body {
              background-color: #fff;
              font-family: Arial, Helvetica, sans-serif;
              color: #333;
            }
            h1 {
              color: #F68B1F;
            }
            p {
              padding: 5px 0;
            }
            form {
              margin: 20px 0;
            }
            .container {
              margin: 0 auto;
              max-width: 600px;
              padding: 20px;
              text-align: center;
              border: 1px solid #ddd;
              border-radius: 10px;
            }
          </style>
        </head>
        <body>
          <div class='container'>
            <h1>WiFi Network</h1>
            <p>This network is provided for demonstration purposes only.</p>
            <p>Battery Voltage: )" + String(batteryVoltage) + R"(V</p>
            <p>)" + count + R"(</p>
            <form action="/login" method="POST">
              <input type="submit" value="Login" style='padding:10px;'>
            </form>
          </div>
        </body>
      </html>
    )";

    request->send(200, "text/html", responseHTML);
  });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    File file = SPIFFS.open("/count.txt", FILE_READ);
    int loginCount;
    if (!file) {
      loginCount = 0;
    } else {
      loginCount = file.parseInt();
    }
    file.close();

    loginCount++;

    file = SPIFFS.open("/count.txt", FILE_WRITE);
    if (!file) {
      Serial.println("Error opening the file for writing");
      return;
    }
    file.print(loginCount);
    file.close();

    String responseHTML = R"(
      <!DOCTYPE html>
      <html>
        <head>
          <title>Login Page</title>
          <style>
            body {
              background-color: #fff;
              font-family: Arial, Helvetica, sans-serif;
              color: #333;
            }
            h1 {
              color: #F68B1F;
            }
            p {
              padding: 5px 0;
            }
            form {
              margin: 20px 0;
            }
            .container {
              margin: 0 auto;
              max-width: 600px;
              padding: 20px;
              text-align: center;
              border: 1px solid #ddd;
              border-radius: 10px;
            }
          </style>
        </head>
        <body>
          <div class='container'>
            <h1>Not a Real Network</h1>
            <p>This is not a real WiFi network. Connecting to unknown WiFi networks can be risky due to potential security threats.</p>
            <p>Logged in )" + String(loginCount) + R"( times.</p>
          </div>
        </body>
      </html>
    )";

    request->send(200, "text/html", responseHTML);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
}
