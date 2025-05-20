#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <secrets.h> // 重要: 您需要创建此文件，包含WiFi凭据 (见下方说明)

// OTA password - change this to secure your OTA updates
const char* otaPassword = "esp01s-relay";

// Relay pin - ESP-01S typically uses GPIO0 or GPIO2 for relay control
// GPIO0 is also used for bootloader mode, so GPIO2 is often preferred
const int RELAY_PIN = 2;  // GPIO2

// Web server on port 80
ESP8266WebServer server(80);

// Device state
bool relayState = false;
String deviceName = "ESP01S-Relay";

// Function prototypes
void setupWiFi();
void setupOTA();
void setupWebServer();
void handleRoot();
void handleRelay();
void handleNotFound();
void toggleRelay();
void setRelay(bool state);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nESP-01S Relay with OTA starting...");

  // Initialize relay pin
  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false);  // Start with relay off

  // Setup WiFi, OTA and web server
  setupWiFi();
  setupOTA();
  setupWebServer();

  Serial.println("Setup complete");
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();

  // Handle web server requests
  server.handleClient();

  // Add any additional logic here
  yield();  // Allow the ESP8266 to handle background tasks
}

// Setup WiFi connection
void setupWiFi() {
  Serial.printf("Connecting to %s ", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(deviceName);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

// Setup OTA updates
void setupOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(deviceName.c_str());

  // Set authentication password
  ArduinoOTA.setPassword(otaPassword);

  // OTA callbacks
  ArduinoOTA.onStart([]() {
    Serial.println("OTA update starting...");
    // Turn off relay during update for safety
    setRelay(false);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA update complete!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

// Setup web server
void setupWebServer() {
  // Define server endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/relay", HTTP_GET, handleRelay);
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

// Handle root page
void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + deviceName + "</title>";
  html += "<style>";
  html += "body { font-family: Arial; text-align: center; margin: 0; padding: 20px; }";
  html += "h1 { color: #0066cc; }";
  html += ".button { display: inline-block; background-color: #4CAF50; border: none; color: white; padding: 16px 40px;";
  html += "text-decoration: none; font-size: 24px; margin: 10px; cursor: pointer; border-radius: 4px; }";
  html += ".button.off { background-color: #f44336; }";
  html += ".status { margin: 20px; font-size: 18px; }";
  html += "</style></head>";

  html += "<body>";
  html += "<h1>" + deviceName + "</h1>";
  html += "<p class='status'>Relay is currently " + String(relayState ? "ON" : "OFF") + "</p>";

  // Toggle button
  if (relayState) {
    html += "<a href='/relay?state=0' class='button off'>Turn OFF</a>";
  } else {
    html += "<a href='/relay?state=1' class='button'>Turn ON</a>";
  }

  html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// Handle relay control
void handleRelay() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "1") {
      setRelay(true);
    } else if (state == "0") {
      setRelay(false);
    } else if (state == "toggle") {
      toggleRelay();
    }
  }

  // Redirect back to root page
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// Handle 404 Not Found
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

// Toggle relay state
void toggleRelay() {
  setRelay(!relayState);
}

// Set relay to specific state
void setRelay(bool state) {
  relayState = state;

  // ESP-01S relay modules are typically active-low (relay ON when pin is LOW)
  // If your relay works the opposite way, swap LOW and HIGH here
  digitalWrite(RELAY_PIN, state ? LOW : HIGH);

  Serial.print("Relay turned ");
  Serial.println(state ? "ON" : "OFF");
}
