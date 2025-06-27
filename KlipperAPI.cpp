/*
  KlipperAPI.cpp - Arduino Library for Klipper/Moonraker API
  
  This library provides an interface to communicate with Moonraker,
  the API server for Klipper 3D printer firmware.
*/

#include "KlipperAPI.h"

// Default constructor
KlipperApi::KlipperApi() {
  _client = nullptr;
  _usingIpAddress = false;
  _moonrakerHost = nullptr;
  _moonrakerPort = 80;
  _hasApiKey = false;
  httpStatusCode = 0;
}

// Constructor with IP address
KlipperApi::KlipperApi(Client &client, IPAddress moonrakerIp, uint16_t moonrakerPort, const char* apiKey) {
  init(client, moonrakerIp, moonrakerPort, apiKey);
}

// Constructor with hostname
KlipperApi::KlipperApi(Client &client, const char *moonrakerHost, uint16_t moonrakerPort, const char* apiKey) {
  init(client, moonrakerHost, moonrakerPort, apiKey);
}

// Initialize with IP address
void KlipperApi::init(Client &client, IPAddress moonrakerIp, uint16_t moonrakerPort, const char* apiKey) {
  _client = &client;
  _moonrakerIp = moonrakerIp;
  _moonrakerPort = moonrakerPort;
  _usingIpAddress = true;
  _hasApiKey = (apiKey != nullptr && strlen(apiKey) > 0);
  if (_hasApiKey) {
    _apiKey = String(apiKey);
  }
  httpStatusCode = 0;
}

// Initialize with hostname
void KlipperApi::init(Client &client, const char *moonrakerHost, uint16_t moonrakerPort, const char* apiKey) {
  _client = &client;
  _moonrakerHost = (char*)moonrakerHost;
  _moonrakerPort = moonrakerPort;
  _usingIpAddress = false;
  _hasApiKey = (apiKey != nullptr && strlen(apiKey) > 0);
  if (_hasApiKey) {
    _apiKey = String(apiKey);
  }
  httpStatusCode = 0;
}

// Send GET request to Moonraker
String KlipperApi::sendGetToMoonraker(const char* endpoint) {
  return sendRequestToMoonraker("GET", endpoint);
}

// Send POST request to Moonraker
String KlipperApi::sendPostToMoonraker(const char* endpoint, const char* postData) {
  return sendRequestToMoonraker("POST", endpoint, postData);
}

// Get results from Moonraker endpoint
String KlipperApi::getMoonrakerEndpointResults(const char* endpoint) {
  return sendGetToMoonraker(endpoint);
}

// Main request method
String KlipperApi::sendRequestToMoonraker(const char* method, const char* endpoint, const char* data) {
  if (_client == nullptr) {
    if (_debug) Serial.println("KlipperAPI: Client not initialized");
    return "";
  }

  String response = "";
  
  // Connect to server
  bool connected = false;
  if (_usingIpAddress) {
    connected = _client->connect(_moonrakerIp, _moonrakerPort);
  } else {
    connected = _client->connect(_moonrakerHost, _moonrakerPort);
  }
  
  if (!connected) {
    if (_debug) Serial.println("KlipperAPI: Connection failed");
    return "";
  }

  // Build HTTP request
  String request = String(method) + " " + String(endpoint) + " HTTP/1.1\r\n";
  
  if (_usingIpAddress) {
    request += "Host: " + _moonrakerIp.toString() + ":" + String(_moonrakerPort) + "\r\n";
  } else {
    request += "Host: " + String(_moonrakerHost) + ":" + String(_moonrakerPort) + "\r\n";
  }
  
  request += "User-Agent: " + String(USER_AGENT) + "\r\n";
  request += "Connection: close\r\n";
  
  // Add API key if available
  if (_hasApiKey) {
    request += "X-Api-Key: " + _apiKey + "\r\n";
  }
  
  // Add content for POST requests
  if (data != nullptr && strlen(data) > 0) {
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + String(strlen(data)) + "\r\n";
  }
  
  request += "\r\n";
  
  // Add POST data
  if (data != nullptr && strlen(data) > 0) {
    request += String(data);
  }
  
  if (_debug) {
    Serial.println("KlipperAPI Request:");
    Serial.println(request);
  }
  
  // Send request
  _client->print(request);
  
  // Wait for response
  unsigned long timeout = millis() + KAPI_TIMEOUT;
  while (!_client->available() && millis() < timeout) {
    delay(10);
  }
  
  // Read response
  bool headerComplete = false;
  String headers = "";
  
  while (_client->available() && millis() < timeout) {
    String line = _client->readStringUntil('\n');
    
    if (!headerComplete) {
      headers += line + "\n";
      if (line.length() <= 1) { // Empty line indicates end of headers
        headerComplete = true;
      }
    } else {
      response += line;
      if (response.length() > maxMessageLength) {
        break; // Prevent memory overflow
      }
    }
  }
  
  // Extract HTTP status code
  httpStatusCode = extractHttpCode(headers, response);
  
  if (_debug) {
    Serial.println("KlipperAPI Response:");
    Serial.println("Status Code: " + String(httpStatusCode));
    Serial.println("Response: " + response);
  }
  
  closeClient();
  return response;
}

// Extract HTTP status code from response
int KlipperApi::extractHttpCode(const String& statusCode, const String& body) {
  if (statusCode.length() > 12) {
    int firstSpace = statusCode.indexOf(' ');
    int secondSpace = statusCode.indexOf(' ', firstSpace + 1);
    
    if (firstSpace != -1 && secondSpace != -1) {
      String code = statusCode.substring(firstSpace + 1, secondSpace);
      return code.toInt();
    }
  }
  return 0;
}

// Close client connection
void KlipperApi::closeClient() {
  if (_client != nullptr && _client->connected()) {
    _client->stop();
  }
}

// Get printer information
bool KlipperApi::getPrinterInfo() {
  String response = sendGetToMoonraker("/printer/info");
  
  if (httpStatusCode != 200 || response.length() == 0) {
    return false;
  }
  
  DynamicJsonDocument doc(JSONDOCUMENT_SIZE);
  deserializeJson(doc, response);
  
  if (doc.containsKey("result")) {
    JsonObject result = doc["result"];
    
    // Extract printer state
    if (result.containsKey("state")) {
      String state = result["state"];
      state.toCharArray(printerStats.state, sizeof(printerStats.state));
      parsePrinterState(state.c_str(), printerStats.stateFlags);
    }
    
    // Extract software versions
    if (result.containsKey("software_version")) {
      String version = result["software_version"];
      version.toCharArray(serverInfo.klipperVersion, sizeof(serverInfo.klipperVersion));
    }
    
    if (result.containsKey("hostname")) {
      String hostname = result["hostname"];
      hostname.toCharArray(serverInfo.hostname, sizeof(serverInfo.hostname));
    }
    
    return true;
  }
  
  return false;
}

// Get comprehensive printer statistics
bool KlipperApi::getPrinterStatistics() {
  // Query multiple printer objects in one request
  String response = sendGetToMoonraker("/printer/objects/query?heater_bed&extruder&toolhead&print_stats&gcode_move");
  
  if (httpStatusCode != 200 || response.length() == 0) {
    return false;
  }
  
  DynamicJsonDocument doc(JSONDOCUMENT_SIZE);
  deserializeJson(doc, response);
  
  if (!doc.containsKey("result") || !doc["result"].containsKey("status")) {
    return false;
  }
  
  JsonObject status = doc["result"]["status"];
  
  // Parse extruder temperature
  if (status.containsKey("extruder")) {
    JsonObject extruder = status["extruder"];
    parseTemperatureData(extruder, printerStats.extruder);
    printerStats.hasExtruder = 1;
  }
  
  // Parse heated bed temperature
  if (status.containsKey("heater_bed")) {
    JsonObject bed = status["heater_bed"];
    parseTemperatureData(bed, printerStats.heatedBed);
    printerStats.hasHeatedBed = 1;
  }
  
  // Parse toolhead position and status
  if (status.containsKey("toolhead")) {
    JsonObject toolhead = status["toolhead"];
    
    if (toolhead.containsKey("position")) {
      JsonArray position = toolhead["position"];
      if (position.size() >= 4) {
        printerStats.positionX = position[0];
        printerStats.positionY = position[1];
        printerStats.positionZ = position[2];
        printerStats.positionE = position[3];
      }
    }
    
    if (toolhead.containsKey("homed_axes")) {
      String homedAxes = toolhead["homed_axes"];
      printerStats.isHomed = (homedAxes.indexOf('x') != -1 && 
                             homedAxes.indexOf('y') != -1 && 
                             homedAxes.indexOf('z') != -1);
    }
  }
  
  // Parse print statistics
  if (status.containsKey("print_stats")) {
    JsonObject printStats = status["print_stats"];
    
    if (printStats.containsKey("state")) {
      String state = printStats["state"];
      state.toCharArray(printerStats.state, sizeof(printerStats.state));
      parsePrinterState(state.c_str(), printerStats.stateFlags);
    }
  }
  
  // Parse GCode move information
  if (status.containsKey("gcode_move")) {
    JsonObject gcodeMove = status["gcode_move"];
    
    if (gcodeMove.containsKey("speed_factor")) {
      printerStats.speedFactor = (uint16_t)(gcodeMove["speed_factor"].as<float>() * 100);
    }
    
    if (gcodeMove.containsKey("extrude_factor")) {
      printerStats.flowFactor = (uint16_t)(gcodeMove["extrude_factor"].as<float>() * 100);
    }
  }
  
  return true;
}

// Get server information
bool KlipperApi::getServerInfo() {
  String response = sendGetToMoonraker("/server/info");
  
  if (httpStatusCode != 200 || response.length() == 0) {
    return false;
  }
  
  DynamicJsonDocument doc(JSONDOCUMENT_SIZE);
  deserializeJson(doc, response);
  
  if (doc.containsKey("result")) {
    JsonObject result = doc["result"];
    
    if (result.containsKey("moonraker_version")) {
      String version = result["moonraker_version"];
      version.toCharArray(serverInfo.moonrakerVersion, sizeof(serverInfo.moonrakerVersion));
    }
    
    return true;
  }
  
  return false;
}

// Get current print job information
bool KlipperApi::getPrintJob() {
  String response = sendGetToMoonraker("/printer/objects/query?print_stats&virtual_sdcard");
  
  if (httpStatusCode != 200 || response.length() == 0) {
    return false;
  }
  
  DynamicJsonDocument doc(JSONDOCUMENT_SIZE);
  deserializeJson(doc, response);
  
  if (!doc.containsKey("result") || !doc["result"].containsKey("status")) {
    return false;
  }
  
  JsonObject status = doc["result"]["status"];
  
  // Parse print statistics
  if (status.containsKey("print_stats")) {
    JsonObject printStats = status["print_stats"];
    
    if (printStats.containsKey("filename")) {
      String filename = printStats["filename"];
      filename.toCharArray(printJob.filename, sizeof(printJob.filename));
    }
    
    if (printStats.containsKey("state")) {
      String state = printStats["state"];
      state.toCharArray(printJob.state, sizeof(printJob.state));
      
      // Set job state flags
      printJob.isPrinting = (state == "printing");
      printJob.isPaused = (state == "paused");
      printJob.isComplete = (state == "complete");
      printJob.isCancelled = (state == "cancelled");
      printJob.hasError = (state == "error");
    }
    
    if (printStats.containsKey("print_duration")) {
      printJob.printTime = printStats["print_duration"];
    }
    
    if (printStats.containsKey("total_duration")) {
      printJob.estimatedTime = printStats["total_duration"];
    }
  }
  
  // Parse virtual SD card information
  if (status.containsKey("virtual_sdcard")) {
    JsonObject sdcard = status["virtual_sdcard"];
    
    if (sdcard.containsKey("progress")) {
      printJob.progress = sdcard["progress"];
    }
    
    if (sdcard.containsKey("file_size")) {
      printJob.fileSize = sdcard["file_size"];
      printJob.printedBytes = (uint32_t)(printJob.progress * printJob.fileSize);
    }
  }
  
  // Calculate time left
  if (printJob.progress > 0 && printJob.printTime > 0) {
    float totalEstimated = printJob.printTime / printJob.progress;
    printJob.timeLeft = (uint32_t)(totalEstimated - printJob.printTime);
  }
  
  return true;
}

// Start a print job
bool KlipperApi::startPrint(const char* filename) {
  char postData[POSTDATA_SIZE];
  snprintf(postData, sizeof(postData), "{\"filename\":\"%s\"}", filename);
  
  String response = sendPostToMoonraker("/printer/print/start", postData);
  return (httpStatusCode == 200);
}

// Pause current print
bool KlipperApi::pausePrint() {
  String response = sendPostToMoonraker("/printer/print/pause", "{}");
  return (httpStatusCode == 200);
}

// Resume paused print
bool KlipperApi::resumePrint() {
  String response = sendPostToMoonraker("/printer/print/resume", "{}");
  return (httpStatusCode == 200);
}

// Cancel current print
bool KlipperApi::cancelPrint() {
  String response = sendPostToMoonraker("/printer/print/cancel", "{}");
  return (httpStatusCode == 200);
}

// Set extruder temperature
bool KlipperApi::setExtruderTemperature(float temperature, uint8_t extruder) {
  if (!isValidTemperature(temperature)) return false;
  
  char gcode[64];
  snprintf(gcode, sizeof(gcode), "M104 T%d S%.1f", extruder, temperature);
  return sendGcode(gcode);
}

// Set bed temperature
bool KlipperApi::setBedTemperature(float temperature) {
  if (!isValidTemperature(temperature)) return false;
  
  char gcode[32];
  snprintf(gcode, sizeof(gcode), "M140 S%.1f", temperature);
  return sendGcode(gcode);
}

// Set fan speed
bool KlipperApi::setFanSpeed(uint8_t speed, uint8_t fan) {
  char gcode[32];
  snprintf(gcode, sizeof(gcode), "M106 P%d S%d", fan, (int)(speed * 2.55));
  return sendGcode(gcode);
}

// Home all axes
bool KlipperApi::homeAll() {
  return sendGcode("G28");
}

// Home specific axis
bool KlipperApi::homeAxis(char axis) {
  char gcode[16];
  snprintf(gcode, sizeof(gcode), "G28 %c", axis);
  return sendGcode(gcode);
}

// Move relative
bool KlipperApi::moveRelative(float x, float y, float z, float e, uint16_t feedrate) {
  char gcode[128];
  snprintf(gcode, sizeof(gcode), "G91\nG1 X%.2f Y%.2f Z%.2f E%.2f F%d\nG90", x, y, z, e, feedrate);
  return sendGcode(gcode);
}

// Move absolute
bool KlipperApi::moveAbsolute(float x, float y, float z, float e, uint16_t feedrate) {
  char gcode[128];
  snprintf(gcode, sizeof(gcode), "G90\nG1 X%.2f Y%.2f Z%.2f E%.2f F%d", x, y, z, e, feedrate);
  return sendGcode(gcode);
}

// Send G-code command
bool KlipperApi::sendGcode(const char* gcode) {
  char postData[POSTDATA_SIZE];
  snprintf(postData, sizeof(postData), "{\"script\":\"%s\"}", gcode);
  
  String response = sendPostToMoonraker("/printer/gcode/script", postData);
  return (httpStatusCode == 200);
}

// Send multiple G-code commands
bool KlipperApi::sendGcodeMultiple(const char* gcodes[], uint8_t count) {
  String script = "";
  for (uint8_t i = 0; i < count; i++) {
    if (i > 0) script += "\\n";
    script += String(gcodes[i]);
  }
  
  char postData[POSTDATA_SIZE];
  snprintf(postData, sizeof(postData), "{\"script\":\"%s\"}", script.c_str());
  
  String response = sendPostToMoonraker("/printer/gcode/script", postData);
  return (httpStatusCode == 200);
}

// Emergency stop
bool KlipperApi::emergencyStop() {
  String response = sendPostToMoonraker("/printer/emergency_stop", "{}");
  return (httpStatusCode == 200);
}

// Restart firmware
bool KlipperApi::restartFirmware() {
  String response = sendPostToMoonraker("/printer/restart", "{}");
  return (httpStatusCode == 200);
}

// Restart host
bool KlipperApi::restartHost() {
  String response = sendPostToMoonraker("/machine/reboot", "{}");
  return (httpStatusCode == 200);
}

// Helper function to parse temperature data
bool KlipperApi::parseTemperatureData(JsonObject& obj, TemperatureData& tempData) {
  if (obj.containsKey("temperature")) {
    tempData.current = obj["temperature"];
  }
  
  if (obj.containsKey("target")) {
    tempData.target = obj["target"];
  }
  
  if (obj.containsKey("power")) {
    tempData.power = (int16_t)(obj["power"].as<float>() * 255);
  }
  
  return true;
}

// Parse printer state string into bit flags
void KlipperApi::parsePrinterState(const char* stateStr, PrinterStateFlags& flags) {
  // Reset all flags
  flags.ready = 0;
  flags.error = 0;
  flags.paused = 0;
  flags.printing = 0;
  flags.standby = 0;
  flags.shutdown = 0;
  flags.startup = 0;
  
  String state = String(stateStr);
  state.toLowerCase();
  
  if (state == "ready") flags.ready = 1;
  else if (state == "error") flags.error = 1;
  else if (state == "paused") flags.paused = 1;
  else if (state == "printing") flags.printing = 1;
  else if (state == "standby") flags.standby = 1;
  else if (state == "shutdown") flags.shutdown = 1;
  else if (state == "startup") flags.startup = 1;
}

// Validate temperature value
bool KlipperApi::isValidTemperature(float temp) {
  return (temp >= 0 && temp <= 500); // Reasonable temperature range
}

// Validate position value
bool KlipperApi::isValidPosition(float pos) {
  return (pos >= -1000 && pos <= 1000); // Reasonable position range
}