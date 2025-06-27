/*******************************************************************
 *  Arduino Ethernet KlipperAPI Example
 *  
 *  This example demonstrates how to use the KlipperAPI library with
 *  an Arduino board and Ethernet shield to monitor a Klipper 3D printer
 *  through the Moonraker API.
 *
 *  Hardware Requirements:
 *  - Arduino Uno/Mega/Leonardo
 *  - Ethernet Shield (W5100/W5500 based)
 *  - Network connection to Moonraker server
 *
 *  Wiring:
 *  - Connect Ethernet shield to Arduino as per manufacturer instructions
 *  - Ensure proper power supply for both Arduino and Ethernet shield
 *******************************************************************/

#include <KlipperAPI.h>
#include <SPI.h>
#include <Ethernet.h>

// Network configuration
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};  // MAC address for Ethernet shield
IPAddress ip(192, 168, 1, 177);                       // Static IP for Arduino (adjust for your network)
IPAddress dns_server(192, 168, 1, 1);                 // DNS server (usually your router)
IPAddress gateway(192, 168, 1, 1);                    // Gateway (usually your router)
IPAddress subnet(255, 255, 255, 0);                   // Subnet mask

// Moonraker server configuration
IPAddress moonrakerIp(192, 168, 1, 100);              // Your Moonraker server IP
const uint16_t moonraker_port = 7125;                 // Moonraker port
const char* api_key = nullptr;                        // API key if needed

// Initialize clients and API
EthernetClient client;
KlipperApi api;

// Timing variables
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 15000;  // 15 seconds between updates (slower for Arduino)

// Status variables
bool networkConnected = false;
bool printerConnected = false;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for Serial port to connect (needed for Leonardo)
  }
  
  Serial.println("=== KlipperAPI Arduino Ethernet Example ===");
  Serial.println("Initializing Ethernet connection...");
  
  // Initialize Ethernet with static IP
  Ethernet.begin(mac, ip, dns_server, gateway, subnet);
  
  // Check for Ethernet hardware
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found!");
    while (true) {
      delay(1000);
    }
  }
  
  // Check for cable connection
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
  
  // Wait for network connection
  delay(2000);
  networkConnected = true;
  
  Serial.print("Arduino IP address: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Connecting to Moonraker at: ");
  Serial.print(moonrakerIp);
  Serial.print(":");
  Serial.println(moonraker_port);
  
  // Initialize KlipperAPI
  api.init(client, moonrakerIp, moonraker_port, api_key);
  
  // Test initial connection
  testConnection();
  
  Serial.println("Setup complete!");
  Serial.println("=====================================");
}

void loop() {
  // Maintain DHCP lease (if using DHCP)
  Ethernet.maintain();
  
  // Update printer status at regular intervals
  if (millis() - lastUpdate > updateInterval || lastUpdate == 0) {
    
    // Check network connectivity
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Network cable disconnected!");
      networkConnected = false;
      printerConnected = false;
    } else if (!networkConnected) {
      Serial.println("Network cable reconnected!");
      networkConnected = true;
    }
    
    if (networkConnected) {
      updatePrinterStatus();
    }
    
    lastUpdate = millis();
  }
  
  // Small delay to prevent overwhelming the processor
  delay(100);
}

void testConnection() {
  Serial.println("Testing Moonraker connection...");
  
  if (api.getServerInfo()) {
    printerConnected = true;
    Serial.println("✓ Connected to Moonraker successfully!");
    Serial.print("Moonraker Version: ");
    Serial.println(api.serverInfo.moonrakerVersion);
    Serial.print("Hostname: ");
    Serial.println(api.serverInfo.hostname);
  } else {
    printerConnected = false;
    Serial.println("✗ Failed to connect to Moonraker");
    Serial.print("HTTP Status: ");
    Serial.println(api.httpStatusCode);
  }
}

void updatePrinterStatus() {
  Serial.println("\n--- Updating Printer Status ---");
  
  // Get server information
  if (api.getServerInfo()) {
    if (!printerConnected) {
      Serial.println("✓ Reconnected to Moonraker!");
      printerConnected = true;
    }
  } else {
    if (printerConnected) {
      Serial.println("✗ Lost connection to Moonraker");
      printerConnected = false;
    }
    Serial.print("Connection failed - HTTP Status: ");
    Serial.println(api.httpStatusCode);
    return;
  }
  
  // Get basic printer information
  if (api.getPrinterInfo()) {
    Serial.print("Printer State: ");
    Serial.println(api.printerStats.state);
    
    // Display state flags
    if (api.printerStats.stateFlags.ready) {
      Serial.println("Status: ✅ Ready");
    } else if (api.printerStats.stateFlags.printing) {
      Serial.println("Status: 🖨️ Printing");
    } else if (api.printerStats.stateFlags.paused) {
      Serial.println("Status: ⏸️ Paused");
    } else if (api.printerStats.stateFlags.error) {
      Serial.println("Status: ❌ Error");
    }
  }
  
  // Get detailed printer statistics
  if (api.getPrinterStatistics()) {
    // Display temperature information
    if (api.printerStats.hasExtruder) {
      Serial.print("Extruder: ");
      Serial.print(api.printerStats.extruder.current, 1);
      Serial.print("°C / ");
      Serial.print(api.printerStats.extruder.target, 1);
      Serial.print("°C (Power: ");
      Serial.print(map(api.printerStats.extruder.power, 0, 255, 0, 100));
      Serial.println("%)");
    }
    
    if (api.printerStats.hasHeatedBed) {
      Serial.print("Heated Bed: ");
      Serial.print(api.printerStats.heatedBed.current, 1);
      Serial.print("°C / ");
      Serial.print(api.printerStats.heatedBed.target, 1);
      Serial.print("°C (Power: ");
      Serial.print(map(api.printerStats.heatedBed.power, 0, 255, 0, 100));
      Serial.println("%)");
    }
    
    // Display position information
    Serial.print("Position - X:");
    Serial.print(api.printerStats.positionX, 2);
    Serial.print(" Y:");
    Serial.print(api.printerStats.positionY, 2);
    Serial.print(" Z:");
    Serial.print(api.printerStats.positionZ, 2);
    Serial.print(" E:");
    Serial.println(api.printerStats.positionE, 2);
    
    // Display motion information
    Serial.print("Speed: ");
    Serial.print(api.printerStats.speedFactor);
    Serial.print("% Flow: ");
    Serial.print(api.printerStats.flowFactor);
    Serial.println("%");
    
    Serial.print("Axes Homed: ");
    Serial.println(api.printerStats.isHomed ? "Yes" : "No");
  }
  
  // Get print job information
  if (api.getPrintJob()) {
    if (api.printJob.isPrinting || api.printJob.isPaused) {
      Serial.println("\n--- Print Job Information ---");
      Serial.print("File: ");
      Serial.println(api.printJob.filename);
      Serial.print("Progress: ");
      Serial.print(api.printJob.progress * 100, 1);
      Serial.println("%");
      
      // Display print time
      Serial.print("Print Time: ");
      displayTime(api.printJob.printTime);
      
      Serial.print("Estimated Total: ");
      displayTime(api.printJob.estimatedTime);
      
      Serial.print("Time Remaining: ");
      displayTime(api.printJob.timeLeft);
      
      // File size information
      Serial.print("File Size: ");
      Serial.print(api.printJob.fileSize / 1024);
      Serial.print(" KB, Printed: ");
      Serial.print(api.printJob.printedBytes / 1024);
      Serial.println(" KB");
      
      // Job status flags
      if (api.printJob.isPrinting) Serial.println("Job Status: Printing");
      else if (api.printJob.isPaused) Serial.println("Job Status: Paused");
      else if (api.printJob.isComplete) Serial.println("Job Status: Complete");
      else if (api.printJob.isCancelled) Serial.println("Job Status: Cancelled");
      
    } else {
      Serial.println("No active print job");
    }
  }
  
  Serial.println("-------------------------------");
}

// Helper function to display time in readable format
void displayTime(uint32_t seconds) {
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
  Serial.println("s");
}

// Example function for basic temperature control
void demonstrateBasicControl() {
  Serial.println("\n=== Basic Control Example ===");
  
  // This function demonstrates basic control capabilities
  // Uncomment and modify as needed for your specific use case
  
  /*
  // Set extruder temperature
  if (api.setExtruderTemperature(200.0)) {
    Serial.println("Extruder temperature set to 200°C");
  }
  
  // Set bed temperature
  if (api.setBedTemperature(60.0)) {
    Serial.println("Bed temperature set to 60°C");
  }
  
  // Home all axes
  if (api.homeAll()) {
    Serial.println("Homing all axes...");
  }
  
  // Send a simple G-code command
  if (api.sendGcode("M114")) {  // Get current position
    Serial.println("Position query sent");
  }
  */
  
  Serial.println("Control functions ready (see code comments)");
}

// Function to handle emergency situations
void handleEmergency() {
  Serial.println("🚨 EMERGENCY STOP ACTIVATED!");
  
  if (api.emergencyStop()) {
    Serial.println("Emergency stop command sent successfully");
  } else {
    Serial.println("Failed to send emergency stop command!");
  }
}

// Simple temperature safety check
void performSafetyCheck() {
  const float MAX_EXTRUDER_TEMP = 280.0;
  const float MAX_BED_TEMP = 120.0;
  
  if (api.printerStats.hasExtruder && 
      api.printerStats.extruder.current > MAX_EXTRUDER_TEMP) {
    Serial.print("⚠️ WARNING: Extruder temperature (");
    Serial.print(api.printerStats.extruder.current, 1);
    Serial.print("°C) exceeds safety limit (");
    Serial.print(MAX_EXTRUDER_TEMP, 1);
    Serial.println("°C)");
    // Uncomment the next line to enable automatic emergency stop
    // handleEmergency();
  }
  
  if (api.printerStats.hasHeatedBed && 
      api.printerStats.heatedBed.current > MAX_BED_TEMP) {
    Serial.print("⚠️ WARNING: Bed temperature (");
    Serial.print(api.printerStats.heatedBed.current, 1);
    Serial.print("°C) exceeds safety limit (");
    Serial.print(MAX_BED_TEMP, 1);
    Serial.println("°C)");
    // Uncomment the next line to enable automatic emergency stop
    // handleEmergency();
  }
}