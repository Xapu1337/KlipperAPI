/* ___  ___  _ _                          _    ____ ___
  |  \/  | |(_)                        | |  |  _ \_ _|
  | .  . |_| |_ _ __  _ __   ___ _ __    | |  | |_) | |
  | |\/| | | | | '_ \| '_ \ / _ \ '__|   | |  |  __/| |
  | |  | | | | | |_) | |_) |  __/ |      | |  | |  | |
  \_|  |_/_|_|_| .__/| .__/ \___|_|      |_|  |_|  |___|
               | |   | |
               |_|   |_|
.......Arduino Library for Klipper/Moonraker API.........

*/

#ifndef KlipperApi_h
#define KlipperApi_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Client.h>

#define KAPI_TIMEOUT       5000
#define POSTDATA_SIZE      256
#define POSTDATA_GCODE_SIZE 128
#define JSONDOCUMENT_SIZE  2048
#define USER_AGENT         "KlipperAPI/1.0.0 (Arduino)"

// Printer state flags using bit fields for memory efficiency
typedef struct {
  uint8_t ready         : 1;
  uint8_t error         : 1;
  uint8_t paused        : 1;
  uint8_t printing      : 1;
  uint8_t standby       : 1;
  uint8_t shutdown      : 1;
  uint8_t startup       : 1;
  uint8_t reserved      : 1; // For future use
} PrinterStateFlags;

// Temperature data structure (optimized for memory)
typedef struct {
  float current;
  float target;
  int16_t power;    // 0-255, using int16_t for safety
} TemperatureData;

// Printer statistics structure
typedef struct {
  char state[16];                    // Current printer state string
  PrinterStateFlags stateFlags;      // Bit flags for quick state checking
  
  // Temperature data
  TemperatureData extruder;          // Primary extruder
  TemperatureData extruder1;         // Secondary extruder (if available)
  TemperatureData heatedBed;         // Heated bed
  
  // Position data (in mm)
  float positionX;
  float positionY;
  float positionZ;
  float positionE;
  
  // Speed and flow
  uint16_t speedFactor;              // Speed factor percentage
  uint16_t flowFactor;               // Flow factor percentage
  
  // Available tools flags
  uint8_t hasExtruder     : 1;
  uint8_t hasExtruder1    : 1;
  uint8_t hasHeatedBed    : 1;
  uint8_t isHomed         : 1;
  uint8_t reserved        : 4;       // For future use
} PrinterStatistics;

// Print job information
typedef struct {
  char filename[64];                 // Current print filename
  char state[16];                    // Print job state
  float progress;                    // Progress 0.0-1.0
  uint32_t printTime;                // Current print time in seconds
  uint32_t estimatedTime;            // Estimated total time in seconds
  uint32_t timeLeft;                 // Estimated time left in seconds
  
  // File information
  uint32_t fileSize;                 // File size in bytes
  uint32_t printedBytes;             // Bytes already processed
  
  // Job state flags
  uint8_t isPrinting     : 1;
  uint8_t isPaused       : 1;
  uint8_t isComplete     : 1;
  uint8_t isCancelled    : 1;
  uint8_t hasError       : 1;
  uint8_t reserved       : 3;        // For future use
} PrintJobInfo;

// Moonraker server information
typedef struct {
  char klipperVersion[32];
  char moonrakerVersion[32];
  char hostname[32];
  uint16_t port;
} ServerInfo;

// Motion system information
typedef struct {
  float maxVelocity;
  float maxAcceleration;
  float squareCornerVelocity;
  
  // Axis limits (min, max)
  float xMin, xMax;
  float yMin, yMax;
  float zMin, zMax;
} MotionLimits;

class KlipperApi {
public:
  KlipperApi(void);
  KlipperApi(Client &client, IPAddress moonrakerIp, uint16_t moonrakerPort, const char* apiKey = nullptr);
  KlipperApi(Client &client, const char *moonrakerHost, uint16_t moonrakerPort, const char* apiKey = nullptr);
  
  // Initialization methods
  void init(Client &client, IPAddress moonrakerIp, uint16_t moonrakerPort, const char* apiKey = nullptr);
  void init(Client &client, const char *moonrakerHost, uint16_t moonrakerPort, const char* apiKey = nullptr);
  
  // Basic communication methods
  String sendGetToMoonraker(const char* endpoint);
  String sendPostToMoonraker(const char* endpoint, const char* postData);
  String getMoonrakerEndpointResults(const char* endpoint);
  
  // Printer information and status
  bool getPrinterInfo();
  bool getPrinterStatistics();
  bool getServerInfo();
  
  // Print job management
  bool getPrintJob();
  bool startPrint(const char* filename);
  bool pausePrint();
  bool resumePrint();
  bool cancelPrint();
  
  // Temperature control
  bool setExtruderTemperature(float temperature, uint8_t extruder = 0);
  bool setBedTemperature(float temperature);
  bool setFanSpeed(uint8_t speed, uint8_t fan = 0);
  
  // Movement and positioning
  bool homeAll();
  bool homeAxis(char axis);
  bool moveRelative(float x, float y, float z, float e, uint16_t feedrate = 3000);
  bool moveAbsolute(float x, float y, float z, float e, uint16_t feedrate = 3000);
  
  // G-code execution
  bool sendGcode(const char* gcode);
  bool sendGcodeMultiple(const char* gcodes[], uint8_t count);
  
  // Emergency controls
  bool emergencyStop();
  bool restartFirmware();
  bool restartHost();
  
  // System information
  bool getMotionLimits();
  
  // Data structures (public for easy access)
  PrinterStatistics printerStats;
  PrintJobInfo printJob;
  ServerInfo serverInfo;
  MotionLimits motionLimits;
  
  // Status and debugging
  bool _debug = false;
  int httpStatusCode = 0;
  String httpErrorBody = "";

private:
  Client *_client;
  String _apiKey;
  IPAddress _moonrakerIp;
  bool _usingIpAddress;
  char *_moonrakerHost;
  uint16_t _moonrakerPort;
  bool _hasApiKey;
  
  static const int maxMessageLength = 1500;
  
  // Private helper methods
  void closeClient();
  int extractHttpCode(const String& statusCode, const String& body);
  String sendRequestToMoonraker(const char* method, const char* endpoint, const char* data = nullptr);
  bool parseTemperatureData(JsonObject& obj, TemperatureData& tempData);
  void parsePrinterState(const char* stateStr, PrinterStateFlags& flags);
  bool isValidTemperature(float temp);
  bool isValidPosition(float pos);
};

#endif