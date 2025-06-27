/*******************************************************************
 *  A "Hello World" sketch to demonstrate the KlipperAPI library.
 *  This example shows how to connect to a Moonraker server and
 *  retrieve printer statistics, temperature data, and job information.
 *
 *  You will need:
 *  - IP address or hostname of your Moonraker server
 *  - Port number (usually 7125 for Moonraker)
 *  - API key (optional, depending on your Moonraker configuration)
 *
 *  Compatible with ESP8266, ESP32, and Arduino with Ethernet/WiFi
 *******************************************************************/

#include <KlipperAPI.h>  // Include the KlipperAPI library

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WiFiClient client;

// Moonraker server configuration
IPAddress moonrakerIp(192, 168, 1, 100);        // Your Moonraker server IP
// char* moonraker_host = "mainsail.local";      // Or use hostname (comment out IP line above)
const uint16_t moonraker_port = 7125;           // Moonraker default port
const char* api_key = nullptr;                  // API key if required (set to nullptr if not needed)

// Initialize KlipperAPI
KlipperApi api;

// Timing variables
unsigned long api_interval = 30000;  // 30 seconds between API calls
unsigned long last_api_call = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== KlipperAPI Library Demo ===");
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize KlipperAPI
  api.init(client, moonrakerIp, moonraker_port, api_key);
  // api.init(client, moonraker_host, moonraker_port, api_key);  // Use this for hostname
  
  // Enable debug output (optional)
  api._debug = true;
  
  Serial.println("KlipperAPI initialized successfully!");
  Serial.println("=====================================\n");
}

void loop() {
  // Check if it's time to make an API call
  if (millis() - last_api_call > api_interval || last_api_call == 0) {
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("--- Fetching Printer Data ---");
      
      // Get server information
      if (api.getServerInfo()) {
        Serial.println("\n=== SERVER INFO ===");
        Serial.print("Moonraker Version: ");
        Serial.println(api.serverInfo.moonrakerVersion);
        Serial.print("Hostname: ");
        Serial.println(api.serverInfo.hostname);
      } else {
        Serial.println("Failed to get server info");
      }
      
      // Get printer information
      if (api.getPrinterInfo()) {
        Serial.println("\n=== PRINTER INFO ===");
        Serial.print("Klipper Version: ");
        Serial.println(api.serverInfo.klipperVersion);
        Serial.print("Printer State: ");
        Serial.println(api.printerStats.state);
        
        // Show state flags
        Serial.print("State Flags - Ready: ");
        Serial.print(api.printerStats.stateFlags.ready ? "YES" : "NO");
        Serial.print(", Printing: ");
        Serial.print(api.printerStats.stateFlags.printing ? "YES" : "NO");
        Serial.print(", Error: ");
        Serial.println(api.printerStats.stateFlags.error ? "YES" : "NO");
      } else {
        Serial.println("Failed to get printer info");
      }
      
      // Get detailed printer statistics
      if (api.getPrinterStatistics()) {
        Serial.println("\n=== PRINTER STATISTICS ===");
        
        // Temperature information
        if (api.printerStats.hasExtruder) {
          Serial.print("Extruder Temperature: ");
          Serial.print(api.printerStats.extruder.current, 1);
          Serial.print("°C / ");
          Serial.print(api.printerStats.extruder.target, 1);
          Serial.print("°C (Power: ");
          Serial.print(api.printerStats.extruder.power);
          Serial.println("%)");
        }
        
        if (api.printerStats.hasHeatedBed) {
          Serial.print("Bed Temperature: ");
          Serial.print(api.printerStats.heatedBed.current, 1);
          Serial.print("°C / ");
          Serial.print(api.printerStats.heatedBed.target, 1);
          Serial.print("°C (Power: ");
          Serial.print(api.printerStats.heatedBed.power);
          Serial.println("%)");
        }
        
        // Position information
        Serial.print("Position - X: ");
        Serial.print(api.printerStats.positionX, 2);
        Serial.print(", Y: ");
        Serial.print(api.printerStats.positionY, 2);
        Serial.print(", Z: ");
        Serial.print(api.printerStats.positionZ, 2);
        Serial.print(", E: ");
        Serial.println(api.printerStats.positionE, 2);
        
        // Speed and flow factors
        Serial.print("Speed Factor: ");
        Serial.print(api.printerStats.speedFactor);
        Serial.print("%, Flow Factor: ");
        Serial.print(api.printerStats.flowFactor);
        Serial.println("%");
        
        // Homing status
        Serial.print("Axes Homed: ");
        Serial.println(api.printerStats.isHomed ? "YES" : "NO");
      } else {
        Serial.println("Failed to get printer statistics");
      }
      
      // Get print job information
      if (api.getPrintJob()) {
        Serial.println("\n=== PRINT JOB INFO ===");
        Serial.print("Filename: ");
        Serial.println(api.printJob.filename);
        Serial.print("Job State: ");
        Serial.println(api.printJob.state);
        
        if (api.printJob.isPrinting || api.printJob.isPaused) {
          Serial.print("Progress: ");
          Serial.print(api.printJob.progress * 100, 1);
          Serial.println("%");
          
          Serial.print("Print Time: ");
          Serial.print(api.printJob.printTime / 3600);
          Serial.print("h ");
          Serial.print((api.printJob.printTime % 3600) / 60);
          Serial.print("m ");
          Serial.print(api.printJob.printTime % 60);
          Serial.println("s");
          
          Serial.print("Time Left: ");
          Serial.print(api.printJob.timeLeft / 3600);
          Serial.print("h ");
          Serial.print((api.printJob.timeLeft % 3600) / 60);
          Serial.print("m ");
          Serial.print(api.printJob.timeLeft % 60);
          Serial.println("s");
          
          Serial.print("File Size: ");
          Serial.print(api.printJob.fileSize / 1024);
          Serial.print(" KB, Printed: ");
          Serial.print(api.printJob.printedBytes / 1024);
          Serial.println(" KB");
        }
        
        // Job state flags
        Serial.print("Job Flags - Printing: ");
        Serial.print(api.printJob.isPrinting ? "YES" : "NO");
        Serial.print(", Paused: ");
        Serial.print(api.printJob.isPaused ? "YES" : "NO");
        Serial.print(", Complete: ");
        Serial.println(api.printJob.isComplete ? "YES" : "NO");
      } else {
        Serial.println("Failed to get print job info");
      }
      
      // Show HTTP status
      Serial.print("\nLast HTTP Status: ");
      Serial.println(api.httpStatusCode);
      
      Serial.println("\n==============================");
      
    } else {
      Serial.println("WiFi not connected!");
    }
    
    // Update last API call time
    last_api_call = millis();
  }
  
  // Small delay to prevent overwhelming the processor
  delay(100);
}

// Helper function to demonstrate temperature control
void demonstrateTemperatureControl() {
  Serial.println("\n=== TEMPERATURE CONTROL DEMO ===");
  
  // Set extruder temperature to 200°C
  if (api.setExtruderTemperature(200.0)) {
    Serial.println("Extruder temperature set to 200°C");
  } else {
    Serial.println("Failed to set extruder temperature");
  }
  
  // Set bed temperature to 60°C
  if (api.setBedTemperature(60.0)) {
    Serial.println("Bed temperature set to 60°C");
  } else {
    Serial.println("Failed to set bed temperature");
  }
  
  // Set fan speed to 50%
  if (api.setFanSpeed(127)) {  // 50% of 255
    Serial.println("Fan speed set to 50%");
  } else {
    Serial.println("Failed to set fan speed");
  }
}

// Helper function to demonstrate movement control
void demonstrateMovementControl() {
  Serial.println("\n=== MOVEMENT CONTROL DEMO ===");
  
  // Home all axes
  if (api.homeAll()) {
    Serial.println("Homing all axes...");
  } else {
    Serial.println("Failed to home all axes");
  }
  
  // Move to center position (adjust coordinates for your printer)
  if (api.moveAbsolute(100, 100, 50, 0, 3000)) {
    Serial.println("Moving to center position...");
  } else {
    Serial.println("Failed to move to position");
  }
  
  // Relative move
  if (api.moveRelative(10, 10, 0, 0, 1500)) {
    Serial.println("Moving 10mm in X and Y...");
  } else {
    Serial.println("Failed to perform relative move");
  }
}

// Helper function to demonstrate G-code sending
void demonstrateGcodeSending() {
  Serial.println("\n=== G-CODE DEMO ===");
  
  // Send a simple G-code command
  if (api.sendGcode("M114")) {  // Get current position
    Serial.println("Sent M114 command (get position)");
  } else {
    Serial.println("Failed to send G-code");
  }
  
  // Send multiple G-code commands
  const char* gcodes[] = {
    "G90",        // Absolute positioning
    "G1 F3000",   // Set feedrate
    "M117 Hello from KlipperAPI"  // Display message
  };
  
  if (api.sendGcodeMultiple(gcodes, 3)) {
    Serial.println("Sent multiple G-code commands");
  } else {
    Serial.println("Failed to send multiple G-codes");
  }
}