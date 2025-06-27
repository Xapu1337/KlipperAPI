/*******************************************************************
 *  ESP32 KlipperAPI Advanced Example
 *  
 *  This example demonstrates advanced features of the KlipperAPI library
 *  including real-time monitoring, automatic print management, and
 *  temperature control with safety features.
 *
 *  Hardware: ESP32
 *  Features:
 *  - Real-time printer monitoring
 *  - Automatic temperature management
 *  - Print job status tracking
 *  - Emergency stop functionality
 *  - Web dashboard (basic HTTP server)
 *******************************************************************/

#include <KlipperAPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Moonraker Configuration
IPAddress moonrakerIp(192, 168, 1, 100);
const uint16_t moonraker_port = 7125;
const char* api_key = nullptr;  // Set if your Moonraker requires authentication

// Clients and servers
WiFiClient client;
WebServer server(80);
KlipperApi api;

// Monitoring intervals
unsigned long fast_update_interval = 5000;   // 5 seconds for active printing
unsigned long slow_update_interval = 30000;  // 30 seconds for idle
unsigned long last_update = 0;
bool is_printing = false;

// Temperature monitoring
float max_extruder_temp = 280.0;  // Safety limit
float max_bed_temp = 120.0;       // Safety limit
bool temp_safety_enabled = true;

// Status tracking
bool printer_connected = false;
String last_error = "";
unsigned long uptime_start = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== KlipperAPI ESP32 Advanced Example ===");
  
  // Start timing
  uptime_start = millis();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize KlipperAPI
  api.init(client, moonrakerIp, moonraker_port, api_key);
  api._debug = false;  // Disable debug for cleaner output
  
  // Test initial connection
  testConnection();
  
  // Setup web server
  setupWebServer();
  
  Serial.println("System ready!");
  Serial.println("=====================================\n");
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Determine update interval based on printer state
  unsigned long current_interval = is_printing ? fast_update_interval : slow_update_interval;
  
  // Update printer status
  if (millis() - last_update > current_interval || last_update == 0) {
    updatePrinterStatus();
    last_update = millis();
  }
  
  // Safety checks if printing
  if (is_printing && temp_safety_enabled) {
    performSafetyChecks();
  }
  
  delay(100);
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Web dashboard: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

void testConnection() {
  Serial.println("Testing Moonraker connection...");
  
  if (api.getServerInfo()) {
    printer_connected = true;
    Serial.println("✓ Connected to Moonraker successfully!");
    Serial.print("Moonraker Version: ");
    Serial.println(api.serverInfo.moonrakerVersion);
  } else {
    printer_connected = false;
    Serial.println("✗ Failed to connect to Moonraker");
  }
}

void updatePrinterStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting reconnect...");
    connectToWiFi();
    return;
  }
  
  bool status_ok = true;
  
  // Get comprehensive printer statistics
  if (api.getPrinterStatistics()) {
    printer_connected = true;
    
    // Update printing status
    bool was_printing = is_printing;
    is_printing = api.printerStats.stateFlags.printing;
    
    // Print status change notifications
    if (was_printing != is_printing) {
      if (is_printing) {
        Serial.println("🖨️  Print job started!");
      } else if (was_printing) {
        Serial.println("⏹️  Print job ended.");
      }
    }
    
    // Get print job details if printing
    if (is_printing) {
      api.getPrintJob();
    }
    
    // Display current status
    displayStatus();
    
  } else {
    printer_connected = false;
    status_ok = false;
    last_error = "Failed to get printer statistics";
    Serial.println("❌ " + last_error);
  }
  
  // Show connection status
  if (!status_ok && printer_connected) {
    Serial.println("Connection lost, will retry...");
  }
}

void displayStatus() {
  Serial.println("\n--- Printer Status ---");
  Serial.print("State: ");
  Serial.print(api.printerStats.state);
  
  if (api.printerStats.stateFlags.error) {
    Serial.print(" ❌ ERROR");
  } else if (api.printerStats.stateFlags.printing) {
    Serial.print(" 🖨️ PRINTING");
  } else if (api.printerStats.stateFlags.paused) {
    Serial.print(" ⏸️ PAUSED");
  } else if (api.printerStats.stateFlags.ready) {
    Serial.print(" ✅ READY");
  }
  Serial.println();
  
  // Temperature status
  if (api.printerStats.hasExtruder) {
    Serial.print("Extruder: ");
    Serial.print(api.printerStats.extruder.current, 1);
    Serial.print("°C / ");
    Serial.print(api.printerStats.extruder.target, 1);
    Serial.print("°C");
    
    if (api.printerStats.extruder.current > max_extruder_temp) {
      Serial.print(" ⚠️ OVER LIMIT!");
    }
    Serial.println();
  }
  
  if (api.printerStats.hasHeatedBed) {
    Serial.print("Bed: ");
    Serial.print(api.printerStats.heatedBed.current, 1);
    Serial.print("°C / ");
    Serial.print(api.printerStats.heatedBed.target, 1);
    Serial.print("°C");
    
    if (api.printerStats.heatedBed.current > max_bed_temp) {
      Serial.print(" ⚠️ OVER LIMIT!");
    }
    Serial.println();
  }
  
  // Print job progress
  if (is_printing && strlen(api.printJob.filename) > 0) {
    Serial.print("Printing: ");
    Serial.println(api.printJob.filename);
    Serial.print("Progress: ");
    Serial.print(api.printJob.progress * 100, 1);
    Serial.println("%");
    
    // Time information
    Serial.print("Time: ");
    printTime(api.printJob.printTime);
    Serial.print(" / Est: ");
    printTime(api.printJob.estimatedTime);
    Serial.print(" / Left: ");
    printTime(api.printJob.timeLeft);
    Serial.println();
  }
  
  Serial.println("---------------------\n");
}

void printTime(uint32_t seconds) {
  uint32_t hours = seconds / 3600;
  uint32_t minutes = (seconds % 3600) / 60;
  uint32_t secs = seconds % 60;
  
  if (hours > 0) {
    Serial.print(hours);
    Serial.print("h ");
  }
  Serial.print(minutes);
  Serial.print("m ");
  Serial.print(secs);
  Serial.print("s");
}

void performSafetyChecks() {
  bool emergency_stop_needed = false;
  String safety_reason = "";
  
  // Check extruder temperature
  if (api.printerStats.hasExtruder && api.printerStats.extruder.current > max_extruder_temp) {
    emergency_stop_needed = true;
    safety_reason = "Extruder temperature exceeded " + String(max_extruder_temp) + "°C";
  }
  
  // Check bed temperature
  if (api.printerStats.hasHeatedBed && api.printerStats.heatedBed.current > max_bed_temp) {
    emergency_stop_needed = true;
    safety_reason = "Bed temperature exceeded " + String(max_bed_temp) + "°C";
  }
  
  // Execute emergency stop if needed
  if (emergency_stop_needed) {
    Serial.println("🚨 EMERGENCY STOP TRIGGERED!");
    Serial.println("Reason: " + safety_reason);
    
    if (api.emergencyStop()) {
      Serial.println("Emergency stop sent successfully");
    } else {
      Serial.println("Failed to send emergency stop!");
    }
    
    is_printing = false;
    last_error = "Emergency stop: " + safety_reason;
  }
}

void setupWebServer() {
  // Root page - dashboard
  server.on("/", handleRoot);
  
  // API endpoints
  server.on("/api/status", handleApiStatus);
  server.on("/api/control/home", HTTP_POST, handleHome);
  server.on("/api/control/pause", HTTP_POST, handlePause);
  server.on("/api/control/resume", HTTP_POST, handleResume);
  server.on("/api/control/cancel", HTTP_POST, handleCancel);
  server.on("/api/emergency", HTTP_POST, handleEmergencyStop);
  
  server.begin();
  Serial.println("Web server started");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Klipper Monitor</title>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".status{background:white;padding:15px;margin:10px 0;border-radius:5px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += ".temp{display:inline-block;margin:10px;padding:10px;background:#e8f4fd;border-radius:5px;}";
  html += ".button{display:inline-block;padding:8px 16px;margin:5px;background:#007cba;color:white;text-decoration:none;border-radius:3px;}";
  html += ".emergency{background:#d73527;}</style></head><body>";
  
  html += "<h1>🖨️ Klipper Printer Monitor</h1>";
  
  // Connection status
  html += "<div class='status'>";
  html += "<h3>Connection Status</h3>";
  html += "Moonraker: " + String(printer_connected ? "✅ Connected" : "❌ Disconnected") + "<br>";
  html += "Uptime: " + String((millis() - uptime_start) / 1000) + "s<br>";
  if (last_error.length() > 0) {
    html += "Last Error: " + last_error + "<br>";
  }
  html += "</div>";
  
  if (printer_connected) {
    // Printer status
    html += "<div class='status'>";
    html += "<h3>Printer Status</h3>";
    html += "State: " + String(api.printerStats.state) + "<br>";
    html += "Homed: " + String(api.printerStats.isHomed ? "Yes" : "No") + "<br>";
    html += "</div>";
    
    // Temperature status
    html += "<div class='status'>";
    html += "<h3>Temperatures</h3>";
    if (api.printerStats.hasExtruder) {
      html += "<div class='temp'>Extruder: " + String(api.printerStats.extruder.current, 1) + "°C / " + String(api.printerStats.extruder.target, 1) + "°C</div>";
    }
    if (api.printerStats.hasHeatedBed) {
      html += "<div class='temp'>Bed: " + String(api.printerStats.heatedBed.current, 1) + "°C / " + String(api.printerStats.heatedBed.target, 1) + "°C</div>";
    }
    html += "</div>";
    
    // Print job status
    if (is_printing) {
      html += "<div class='status'>";
      html += "<h3>Print Job</h3>";
      html += "File: " + String(api.printJob.filename) + "<br>";
      html += "Progress: " + String(api.printJob.progress * 100, 1) + "%<br>";
      html += "Print Time: " + String(api.printJob.printTime / 60) + " minutes<br>";
      html += "Time Left: " + String(api.printJob.timeLeft / 60) + " minutes<br>";
      html += "</div>";
    }
    
    // Control buttons
    html += "<div class='status'>";
    html += "<h3>Controls</h3>";
    html += "<a href='/api/control/home' class='button'>🏠 Home All</a>";
    if (is_printing) {
      if (api.printerStats.stateFlags.paused) {
        html += "<a href='/api/control/resume' class='button'>▶️ Resume</a>";
      } else {
        html += "<a href='/api/control/pause' class='button'>⏸️ Pause</a>";
      }
      html += "<a href='/api/control/cancel' class='button'>⏹️ Cancel</a>";
    }
    html += "<a href='/api/emergency' class='button emergency'>🚨 EMERGENCY STOP</a>";
    html += "</div>";
  }
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleApiStatus() {
  String json = "{";
  json += "\"connected\":" + String(printer_connected ? "true" : "false") + ",";
  json += "\"state\":\"" + String(api.printerStats.state) + "\",";
  json += "\"printing\":" + String(is_printing ? "true" : "false") + ",";
  json += "\"extruder_temp\":" + String(api.printerStats.extruder.current) + ",";
  json += "\"bed_temp\":" + String(api.printerStats.heatedBed.current) + ",";
  json += "\"progress\":" + String(api.printJob.progress * 100);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleHome() {
  if (api.homeAll()) {
    server.send(200, "text/plain", "Homing initiated");
  } else {
    server.send(500, "text/plain", "Failed to home");
  }
}

void handlePause() {
  if (api.pausePrint()) {
    server.send(200, "text/plain", "Print paused");
  } else {
    server.send(500, "text/plain", "Failed to pause");
  }
}

void handleResume() {
  if (api.resumePrint()) {
    server.send(200, "text/plain", "Print resumed");
  } else {
    server.send(500, "text/plain", "Failed to resume");
  }
}

void handleCancel() {
  if (api.cancelPrint()) {
    is_printing = false;
    server.send(200, "text/plain", "Print cancelled");
  } else {
    server.send(500, "text/plain", "Failed to cancel");
  }
}

void handleEmergencyStop() {
  if (api.emergencyStop()) {
    is_printing = false;
    server.send(200, "text/plain", "Emergency stop executed");
    Serial.println("🚨 Emergency stop triggered via web interface");
  } else {
    server.send(500, "text/plain", "Failed to execute emergency stop");
  }
}