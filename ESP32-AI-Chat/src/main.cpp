#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <FS.h>          // 通常需要先包含 FS.h
#include <LittleFS.h>    // 包含 LittleFS 库
#include <Preferences.h> // 包含 NVS 库

#include <ESPmDNS.h>
#include "secrets.h" // 包含你的配置 (WiFi凭证, API密钥等)

// Create WebServer object on port 80
WebServer server(WEB_SERVER_PORT);

// Buffer for user input (for serial interface)
String userInput = "";
boolean newInput = false;

// Preferences object for NVS storage
Preferences preferences;

// Conversation history
const int MAX_HISTORY_ITEMS = 10;        // Maximum number of conversation pairs to store
const char *NVS_NAMESPACE = "aichat";    // Namespace for NVS storage
const char *NVS_HISTORY_COUNT = "count"; // Key for history count
const char *NVS_USER_PREFIX = "user_";   // Prefix for user message keys
const char *NVS_AI_PREFIX = "ai_";       // Prefix for AI response keys

// Function declarations
void setupWiFi();
void setupMDNS();
void setupWebServer();
void handleRoot();
void handleNotFound();
void handleChat();
void handleClearHistory(); // New function to clear history
String callOpenAI(String message);
bool initFileSystem();
String cleanResponse(String input);

// NVS functions
bool initNVS();
void saveConversation(String userMessage, String aiResponse);
String getConversationHistory();
void clearConversationHistory();

void setup()
{
  // Initialize serial communication with higher timeout
  Serial.begin(115200);
  delay(3000); // Longer delay to ensure serial is ready

  // Print multiple debug messages to help identify if serial is working
  Serial.println();
  Serial.println("=============================");
  Serial.println("ESP32 AI Chat starting...");
  Serial.println("Serial communication initialized");
  Serial.println("=============================");
  Serial.println();

  // Initialize file system
  if (!initFileSystem())
  {
    Serial.println("File system initialization failed! Halting.");
    // Consider adding error handling like blinking an LED or stopping execution
    while (true)
    {
      delay(1000);
    } // Halt execution
  }

  // Initialize NVS
  if (!initNVS())
  {
    Serial.println("NVS initialization failed! Continuing without conversation history.");
    // We don't halt execution here, just continue without conversation history
  }

  // Setup WiFi (AP or Station mode)
  setupWiFi();

  // Setup mDNS responder
  setupMDNS();

  // Setup web server
  setupWebServer();

  Serial.println("\n=============================");
  Serial.println("SETUP COMPLETED SUCCESSFULLY");
  Serial.println("Web server started");

  if (USE_AP_MODE)
  {
    Serial.print("Connect to WiFi SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Then access: http://");
    Serial.println(WiFi.softAPIP());
  }
  else
  {
    Serial.print("Access via IP: http://");
    Serial.println(WiFi.localIP());
    Serial.print("Or via mDNS: http://");
    Serial.print(MDNS_HOST_NAME);
    Serial.println(".local");
  }

  Serial.println("You can also type your message in serial monitor and press Enter to chat with AI.");
  Serial.println("=============================");
}

void loop()
{
  // Handle web server client requests
  server.handleClient();

  // Check for user input from Serial
  while (Serial.available())
  {
    char c = Serial.read();
    if (c == '\n' || c == '\r')
    {
      // End of line detected
      if (userInput.length() > 0)
      {
        newInput = true;
      }
    }
    else
    {
      // Add character to input buffer
      userInput += c;
    }
  }

  // Process new input
  if (newInput)
  {
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

  delay(10); // Short delay for stability
}

// Initialize file system
bool initFileSystem()
{
  Serial.println("Initializing file system...");
  if (!LittleFS.begin(false))
  { // 先尝试不格式化挂载
    Serial.println("Mount failed, attempting to format...");
    if (LittleFS.format())
    { // 显式调用格式化
      Serial.println("Format successful. Trying to mount again...");
      if (!LittleFS.begin(false))
      { // 再次尝试挂载
        Serial.println("ERROR: Mount failed even after explicit formatting!");
        return false;
      }
      Serial.println("Mount successful after formatting.");
      // 注意：格式化后文件系统是空的，除非你之后上传了文件
    }
    else
    {
      Serial.println("ERROR: Formatting LittleFS failed!");
      return false;
    }
  }

  // --- 文件系统挂载成功 ---
  Serial.println("SUCCESS: LittleFS mounted successfully.");

  // --- 开始列出根目录文件 (新添加的代码) ---
  Serial.println("Listing files in LittleFS root directory:");
  File root = LittleFS.open("/");
  if (!root)
  {
    Serial.println("- failed to open root directory");
  }
  else if (!root.isDirectory())
  {
    Serial.println(" - root is not a directory");
  }
  else
  {
    File file = root.openNextFile();
    bool fileFound = false; // 用于标记是否至少找到一个文件/目录
    while (file)
    {
      fileFound = true;
      if (file.isDirectory())
      {
        Serial.print("  DIR : ");
        Serial.println(file.name());
      }
      else
      {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("\tSIZE: ");
        Serial.println(file.size());
      }
      file.close(); // 关闭当前文件句柄
      file = root.openNextFile();
    }
    // 检查循环是否从未执行过（即目录为空）
    if (!fileFound)
    {
      Serial.println("  (Directory is empty)");
    }
    root.close(); // 关闭根目录句柄
  }
  Serial.println("--------------------");
  // --- 文件列出结束 ---

  return true; // 文件系统初始化和文件列出（如果成功）完成
}

// Setup WiFi (AP or Station mode)
void setupWiFi()
{
  Serial.println("Setting up WiFi...");

  if (USE_AP_MODE)
  {
    // Access Point Mode
    Serial.println("Mode: Access Point");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);

    WiFi.mode(WIFI_AP);
    bool apResult = WiFi.softAP(AP_SSID, AP_PASSWORD);

    if (apResult)
    {
      Serial.print("SUCCESS: AP started. IP address: ");
      Serial.println(WiFi.softAPIP());
    }
    else
    {
      Serial.println("ERROR: Failed to start AP mode");
    }
  }
  else
  {
    // Station Mode
    Serial.println("Mode: Station");
    Serial.print("Connecting to WiFi SSID: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait for connection with timeout
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20)
    { // 10 second timeout (20 * 500ms)
      delay(500);
      Serial.print(".");
      timeout++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("SUCCESS: Connected to WiFi. IP address: ");
      Serial.println(WiFi.localIP());
    }
    else
    {
      Serial.println("ERROR: Failed to connect to WiFi");
      Serial.print("WiFi status: ");
      Serial.println(WiFi.status());
      // Consider adding more robust error handling here
    }
  }

  Serial.println("WiFi setup completed");
}

// Setup mDNS responder
void setupMDNS()
{
  Serial.println("Setting up mDNS responder...");
  Serial.print("Hostname: ");
  Serial.println(MDNS_HOST_NAME);

  // Initialize mDNS service
  if (MDNS.begin(MDNS_HOST_NAME))
  {
    Serial.print("SUCCESS: mDNS responder started. You can access the server at http://");
    Serial.print(MDNS_HOST_NAME);
    Serial.println(".local");

    // Add service to mDNS
    MDNS.addService("http", "tcp", WEB_SERVER_PORT);
    Serial.println("HTTP service added to mDNS");
  }
  else
  {
    Serial.println("ERROR: Failed to set up mDNS responder!");
  }

  Serial.println("mDNS setup completed");
}

// Handle root URL
void handleRoot()
{
  Serial.println("Handling root URL request...");

  // Read the HTML file from file system and send it to the client
  // 使用 LittleFS (首字母大写) 对象
  File file = LittleFS.open("/index.html", "r");
  if (file)
  {
    Serial.println("SUCCESS: index.html file opened, streaming to client");
    server.streamFile(file, "text/html; charset=UTF-8");
    file.close();
    Serial.println("File sent and closed");
  }
  else
  {

    Serial.println("Sending basic error message");

    // Very simple fallback message if both files are missing
    String basicHtml = "<html><head><title>ESP32 AI Chat</title></head><body>";
    basicHtml += "<h1>ESP32 AI Chat</h1>";
    basicHtml += "<p>Error: Interface files (index.html or fallback.html) not found in LittleFS.</p>";
    basicHtml += "<p>Please upload the filesystem image containing the web files.</p>";
    basicHtml += "</body></html>";

    server.send(200, "text/html", basicHtml);
  }
}

// Handle 404 Not Found
void handleNotFound()
{
  server.send(404, "text/plain", "404: Not found");
}

// Handle chat API requests
void handleChat()
{
  if (server.hasArg("plain") == false)
  {
    server.send(400, "application/json", "{\"error\":\"No message received\"}");
    return;
  }

  String body = server.arg("plain");

  // Parse JSON request
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);

  if (error)
  {
    Serial.print("JSON Deserialization Error: ");
    Serial.println(error.c_str());
    server.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
    return;
  }

  // Get message from request
  String message = doc["message"].as<String>();

  if (message.length() == 0)
  {
    // 如果消息为空，只返回历史记录而不调用API
    String history = getConversationHistory();
    Serial.println("Returning history: " + history);

    DynamicJsonDocument responseDoc(8192);
    responseDoc["history"] = history;

    String jsonResponse;
    serializeJson(responseDoc, jsonResponse);

    Serial.println("History response: " + jsonResponse);

    // 发送响应
    server.send(200, "application/json", jsonResponse);
    return;
  }

  // Call OpenAI API
  String aiResponse = callOpenAI(message);

  // Save conversation to NVS
  saveConversation(message, aiResponse);

  // Create JSON response
  DynamicJsonDocument responseDoc(4096); // Ensure this size is adequate for expected responses
  responseDoc["response"] = aiResponse;

  // Add conversation history to response
  responseDoc["history"] = getConversationHistory();

  String jsonResponse;
  serializeJson(responseDoc, jsonResponse);

  // Send response
  server.send(200, "application/json", jsonResponse);
}

// Setup web server
void setupWebServer()
{
  Serial.println("Setting up web server...");

  // Route for root / web page
  server.on("/", HTTP_GET, handleRoot);
  Serial.println("Route configured: GET /");

  // Route to handle chat API requests
  server.on("/chat", HTTP_POST, handleChat);
  Serial.println("Route configured: POST /chat");

  // Route to clear conversation history
  server.on("/clear-history", HTTP_POST, handleClearHistory);
  Serial.println("Route configured: POST /clear-history");

  // Set up 404 handler
  server.onNotFound(handleNotFound);
  Serial.println("404 handler configured");

  // Start server
  server.begin();

  Serial.print("SUCCESS: HTTP server started on port ");
  Serial.println(WEB_SERVER_PORT);

  Serial.println("Web server setup completed");
}

// Handle clear history request
void handleClearHistory()
{
  clearConversationHistory();
  server.send(200, "application/json", "{\"success\":true}");
}

// Initialize NVS
bool initNVS()
{
  Serial.println("Initializing NVS...");
  if (!preferences.begin(NVS_NAMESPACE, false))
  {
    Serial.println("ERROR: Failed to initialize NVS");
    return false;
  }
  Serial.println("SUCCESS: NVS initialized");
  return true;
}

// Save conversation to NVS
void saveConversation(String userMessage, String aiResponse)
{
  // Get current count
  int count = preferences.getInt(NVS_HISTORY_COUNT, 0);

  // If we've reached the maximum, remove the oldest entry
  if (count >= MAX_HISTORY_ITEMS)
  {
    // Shift all entries down by one
    for (int i = 0; i < MAX_HISTORY_ITEMS - 1; i++)
    {
      String userKey = String(NVS_USER_PREFIX) + String(i);
      String aiKey = String(NVS_AI_PREFIX) + String(i);
      String nextUserKey = String(NVS_USER_PREFIX) + String(i + 1);
      String nextAiKey = String(NVS_AI_PREFIX) + String(i + 1);

      String nextUserMsg = preferences.getString(nextUserKey.c_str(), "");
      String nextAiMsg = preferences.getString(nextAiKey.c_str(), "");

      preferences.putString(userKey.c_str(), nextUserMsg);
      preferences.putString(aiKey.c_str(), nextAiMsg);
    }

    // Add new entry at the end
    String lastUserKey = String(NVS_USER_PREFIX) + String(MAX_HISTORY_ITEMS - 1);
    String lastAiKey = String(NVS_AI_PREFIX) + String(MAX_HISTORY_ITEMS - 1);

    preferences.putString(lastUserKey.c_str(), userMessage);
    preferences.putString(lastAiKey.c_str(), aiResponse);
  }
  else
  {
    // Add new entry
    String userKey = String(NVS_USER_PREFIX) + String(count);
    String aiKey = String(NVS_AI_PREFIX) + String(count);

    preferences.putString(userKey.c_str(), userMessage);
    preferences.putString(aiKey.c_str(), aiResponse);

    // Update count
    preferences.putInt(NVS_HISTORY_COUNT, count + 1);
  }
}

// Get conversation history as JSON string
String getConversationHistory()
{
  int count = preferences.getInt(NVS_HISTORY_COUNT, 0);

  // Create JSON array
  DynamicJsonDocument doc(8192); // Adjust size as needed
  JsonArray historyArray = doc.to<JsonArray>();

  // Add each conversation pair
  for (int i = 0; i < count; i++)
  {
    String userKey = String(NVS_USER_PREFIX) + String(i);
    String aiKey = String(NVS_AI_PREFIX) + String(i);

    String userMsg = preferences.getString(userKey.c_str(), "");
    String aiMsg = preferences.getString(aiKey.c_str(), "");

    if (userMsg.length() > 0 || aiMsg.length() > 0)
    {
      JsonObject entry = historyArray.createNestedObject();
      entry["user"] = userMsg;
      entry["ai"] = aiMsg;
    }
  }

  String result;
  serializeJson(historyArray, result);
  return result;
}

// Get the last N conversation pairs for OpenAI context
void getLastNConversations(JsonArray &messages, int n)
{
  int count = preferences.getInt(NVS_HISTORY_COUNT, 0);
  int startIdx = max(0, count - n); // Get the last n conversations

  Serial.print("Getting last ");
  Serial.print(n);
  Serial.print(" conversations, starting from index ");
  Serial.println(startIdx);

  // Add each conversation pair to the messages array
  for (int i = startIdx; i < count; i++)
  {
    String userKey = String(NVS_USER_PREFIX) + String(i);
    String aiKey = String(NVS_AI_PREFIX) + String(i);

    String userMsg = preferences.getString(userKey.c_str(), "");
    String aiMsg = preferences.getString(aiKey.c_str(), "");

    if (userMsg.length() > 0)
    {
      JsonObject userMessage = messages.createNestedObject();
      userMessage["role"] = "user";
      userMessage["content"] = userMsg;

      Serial.print("Added user message from history: ");
      Serial.println(userMsg);
    }

    if (aiMsg.length() > 0)
    {
      JsonObject assistantMessage = messages.createNestedObject();
      assistantMessage["role"] = "assistant";
      assistantMessage["content"] = aiMsg;

      Serial.print("Added assistant message from history: ");
      Serial.println(aiMsg);
    }
  }
}

// Clear conversation history
void clearConversationHistory()
{
  Serial.println("Clearing conversation history...");

  // Get current count
  int count = preferences.getInt(NVS_HISTORY_COUNT, 0);

  // Clear all entries
  for (int i = 0; i < count; i++)
  {
    String userKey = String(NVS_USER_PREFIX) + String(i);
    String aiKey = String(NVS_AI_PREFIX) + String(i);

    preferences.remove(userKey.c_str());
    preferences.remove(aiKey.c_str());
  }

  // Reset count
  preferences.putInt(NVS_HISTORY_COUNT, 0);

  Serial.println("Conversation history cleared");
}

// Helper function to clean up OpenAI API response
String cleanResponse(String input)
{
  // Find the first '{' character which indicates the start of JSON
  int jsonStart = input.indexOf('{');
  if (jsonStart == -1)
  {
    return ""; // No JSON found
  }

  // Find the last '}' character which indicates the end of JSON
  int jsonEnd = input.lastIndexOf('}');
  if (jsonEnd == -1)
  {
    return ""; // No JSON end found
  }

  // Extract the JSON portion
  return input.substring(jsonStart, jsonEnd + 1);
}

// Call OpenAI API and return the response
String callOpenAI(String message)
{
  // Create JSON payload
  DynamicJsonDocument doc(8192); // Increased size to accommodate conversation history
  doc["model"] = API_MODEL;

  JsonArray messages = doc.createNestedArray("messages");

  JsonObject systemMessage = messages.createNestedObject();
  systemMessage["role"] = "system";
  systemMessage["content"] = "You are a helpful assistant."; // Customize system prompt if needed

  // Add the last 3 conversation pairs to provide context
  getLastNConversations(messages, 3);

  // Add the current user message
  JsonObject userMessage = messages.createNestedObject();
  userMessage["role"] = "user";
  userMessage["content"] = message;

  Serial.println("Messages array with history:");
  String messagesDebug;
  serializeJson(messages, messagesDebug);
  Serial.println(messagesDebug);

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  Serial.println("Full payload size (bytes): " + String(jsonPayload.length()));

  // Set up secure client
  WiFiClientSecure client;

  // --- SECURITY WARNING ---
  // setInsecure() skips server certificate validation, making the connection vulnerable
  // to Man-in-the-Middle attacks. For production or sensitive data,
  // use client.setCACert(root_ca_openai) with the appropriate root CA certificate.
  client.setInsecure();
  // ------------------------

  Serial.println("Connecting to AI service...");
  if (!client.connect(API_SERVER, API_PORT))
  {
    Serial.println("ERROR: Connection to AI service failed!");
    return "Error: Could not connect to AI service."; // Return descriptive error
  }

  Serial.println("Connected to AI service!");

  // Prepare HTTP request
  String request = "POST /v1/chat/completions HTTP/1.1\r\n";
  request += "Host: " + String(API_SERVER) + "\r\n";
  request += "Content-Type: application/json\r\n";
  // Ensure API_KEY is correctly defined in config.h and kept secret
  request += "Authorization: Bearer " + String(API_KEY) + "\r\n";
  request += "Content-Length: " + String(jsonPayload.length()) + "\r\n";
  request += "Connection: close\r\n\r\n";
  request += jsonPayload;

  // Log request details (excluding Authorization header for security)
  Serial.println("\n===== REQUEST DETAILS =====");
  Serial.println("POST /v1/chat/completions HTTP/1.1");
  Serial.println("Host: " + String(API_SERVER));
  Serial.println("Content-Type: application/json");
  Serial.println("Authorization: Bearer [API_KEY_HIDDEN]");
  Serial.println("Content-Length: " + String(jsonPayload.length()));
  Serial.println("Connection: close");
  Serial.println("\n----- REQUEST PAYLOAD -----");
  Serial.println(jsonPayload);
  Serial.println("===========================\n");

  // Send the request
  client.print(request);
  Serial.println("OpenAI request sent!");

  // Wait for response with timeout
  unsigned long timeout = millis();
  unsigned long lastDebugTime = millis();
  Serial.println("Waiting for OpenAI response...");

  while (client.available() == 0)
  {
    // Print a waiting message every 5 seconds for debugging
    if (millis() - lastDebugTime > 5000)
    {
      Serial.print("Still waiting for response... ");
      Serial.print((millis() - timeout) / 1000);
      Serial.println(" seconds elapsed");
      lastDebugTime = millis();
    }

    if (millis() - timeout > 60000)
    { // Increased timeout to 60 seconds
      Serial.println("ERROR: Client Timeout waiting for OpenAI response!");
      client.stop();
      return "Error: AI service response timeout."; // Return descriptive error
    }
    delay(10); // Small delay while waiting
  }

  Serial.print("Response received after ");
  Serial.print((millis() - timeout) / 1000);
  Serial.println(" seconds");

  // Read HTTP status line
  String statusLine = client.readStringUntil('\n');
  Serial.print("HTTP Status: ");
  Serial.println(statusLine);

  Serial.println("\n===== RESPONSE HEADERS =====");
  Serial.println(statusLine);

  // Read headers and find end of headers
  String allHeaders = "";
  while (client.available())
  {
    String line = client.readStringUntil('\n');
    allHeaders += line + "\n";

    // Print each header for debugging
    Serial.println(line);

    if (line == "\r")
    {
      // Empty line indicates end of headers
      Serial.println("End of headers found.");
      break;
    }
  }
  Serial.println("===========================\n");

  // Read the response body
  String responseBody = "";
  // Read response incrementally to avoid large buffer issues if response is huge
  while (client.connected() || client.available())
  {
    if (client.available())
    {
      responseBody += client.readString(); // Read available data
    }
    delay(10); // Small delay
    // Add a safety break if something goes wrong with client state
    if (millis() - timeout > 90000)
    { // Total timeout for reading response (increased to 90 seconds)
      Serial.println("ERROR: Timeout while reading response body.");
      break;
    }
  }

  // Clean up
  client.stop();
  Serial.println("Connection closed.");

  // Debug response body
  Serial.println("\n===== RAW RESPONSE BODY =====");
  Serial.println(responseBody);
  Serial.println("===========================\n");

  // Clean the response to get valid JSON
  String cleanedResponse = cleanResponse(responseBody);
  Serial.println("\n===== CLEANED JSON RESPONSE =====");
  Serial.println(cleanedResponse);
  Serial.println("===========================\n");

  // Parse JSON response
  // Increase size if necessary, check OpenAI typical response size
  DynamicJsonDocument respDoc(8192);
  DeserializationError error = deserializeJson(respDoc, cleanedResponse);

  if (error)
  {
    Serial.print("ERROR: deserializeJson() failed: ");
    Serial.println(error.c_str());
    Serial.println("Failed to parse JSON response body.");
    Serial.println("Response body was: ");
    Serial.println(cleanedResponse);              // Print the problematic body
    return "Error: Could not parse AI response."; // Return descriptive error
  }

  // Extract the assistant's message safely
  String assistantResponse = "";
  if (respDoc.containsKey("choices") &&
      respDoc["choices"].is<JsonArray>() &&
      respDoc["choices"].size() > 0 &&
      respDoc["choices"][0].is<JsonObject>() &&
      respDoc["choices"][0].containsKey("message") &&
      respDoc["choices"][0]["message"].is<JsonObject>() &&
      respDoc["choices"][0]["message"].containsKey("content"))
  {
    assistantResponse = respDoc["choices"][0]["message"]["content"].as<String>();
  }
  else
  {
    Serial.println("ERROR: Could not find 'choices[0].message.content' in JSON response.");
    assistantResponse = "Error: Unexpected AI response format."; // Return descriptive error
  }

  return assistantResponse;
}