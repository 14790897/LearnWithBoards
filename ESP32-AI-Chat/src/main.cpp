#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// OpenAI API settings
const char* server = "api.openai.com";
const int port = 443;
const char* apiKey = "YOUR_OPENAI_API_KEY";
const char* model = "gpt-3.5-turbo";

// Root certificate for api.openai.com
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----";

// Buffer for user input
String userInput = "";
boolean newInput = false;

// Function declarations
void connectToWiFi();
String callOpenAI(String message);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 AI Chat starting...");

  // Connect to WiFi
  connectToWiFi();

  Serial.println("Type your message and press Enter to chat with AI.");
}

void loop() {
  // Check for user input from Serial
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      // End of line detected
      if (userInput.length() > 0) {
        newInput = true;
      }
    } else {
      // Add character to input buffer
      userInput += c;
    }
  }

  // Process new input
  if (newInput) {
    Serial.print("You: ");
    Serial.println(userInput);

    // Call OpenAI API
    Serial.println("AI is thinking...");
    String response = callOpenAI(userInput);

    // Display response
    Serial.print("AI: ");
    Serial.println(response);

    // Reset for next input
    userInput = "";
    newInput = false;
    Serial.println("\nType your next message:");
  }

  delay(100);
}

// Connect to WiFi network
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
}

// Call OpenAI API and return the response
String callOpenAI(String message) {
  // Create JSON payload
  DynamicJsonDocument doc(1024);
  doc["model"] = model;

  JsonArray messages = doc.createNestedArray("messages");

  JsonObject systemMessage = messages.createNestedObject();
  systemMessage["role"] = "system";
  systemMessage["content"] = "You are a helpful assistant.";

  JsonObject userMessage = messages.createNestedObject();
  userMessage["role"] = "user";
  userMessage["content"] = message;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  // Set up secure client
  WiFiClientSecure client;
  client.setCACert(root_ca);

  Serial.println("Connecting to server...");
  if (!client.connect(server, port)) {
    Serial.println("Connection failed!");
    return "Error connecting to AI service.";
  }

  Serial.println("Connected to server!");

  // Prepare HTTP request
  String request = "POST /v1/chat/completions HTTP/1.1\r\n";
  request += "Host: " + String(server) + "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Authorization: Bearer " + String(apiKey) + "\r\n";
  request += "Content-Length: " + String(jsonPayload.length()) + "\r\n";
  request += "Connection: close\r\n\r\n";
  request += jsonPayload;

  // Send the request
  client.print(request);
  Serial.println("Request sent!");

  // Wait for response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println("Client Timeout!");
      client.stop();
      return "Error: Server response timeout.";
    }
  }

  // Read headers
  String line;
  while (client.available()) {
    line = client.readStringUntil('\n');
    if (line == "\r") {
      // End of headers
      break;
    }
  }

  // Read the response body
  String response = "";
  while (client.available()) {
    char c = client.read();
    response += c;
  }

  // Clean up
  client.stop();
  Serial.println("Connection closed.");

  // Debug response
  Serial.println("Raw response: " + response);

  // Find the JSON part of the response
  int jsonStart = response.indexOf('{');
  if (jsonStart == -1) {
    Serial.println("No JSON found in response");
    return "Error: Invalid response format.";
  }

  String jsonResponse = response.substring(jsonStart);
  Serial.println("JSON response: " + jsonResponse);

  // Parse response
  DynamicJsonDocument respDoc(4096);
  DeserializationError error = deserializeJson(respDoc, jsonResponse);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "Error parsing AI response.";
  }

  // Extract the assistant's message
  String assistantResponse = respDoc["choices"][0]["message"]["content"].as<String>();
  return assistantResponse;
}