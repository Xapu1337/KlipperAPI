# KlipperAPI - Arduino Library for Klipper/Moonraker

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

An efficient Arduino library for communicating with Moonraker, the API server for Klipper 3D printer firmware. This library enables ESP8266, ESP32, and Arduino boards to monitor and control Klipper-based 3D printers through the Moonraker API.

## 🚀 Features

### Core Functionality
- **Real-time printer monitoring** - Get current status, temperatures, and positions
- **Print job management** - Start, pause, resume, and cancel prints
- **Temperature control** - Set extruder, bed, and fan temperatures
- **Movement control** - Home axes and move to specific positions
- **G-code execution** - Send single or multiple G-code commands
- **Emergency controls** - Emergency stop and system restart capabilities

### Memory Optimized
- **Bit fields** for boolean states to minimize memory usage
- **Appropriate data types** (uint8_t, int16_t) instead of generic int/long
- **Structured data organization** with clear typedefs
- **Configurable buffer sizes** for different microcontroller capabilities

### Hardware Support
- ✅ ESP8266
- ✅ ESP32  
- ✅ Arduino with Ethernet Shield
- ✅ Arduino with WiFi capabilities

## 📦 Installation

### Arduino IDE Library Manager
1. Open Arduino IDE
2. Go to **Sketch** → **Include Library** → **Manage Libraries**
3. Search for "KlipperAPI"
4. Click **Install**

### Manual Installation
1. Download the latest release from GitHub
2. Extract to your Arduino `libraries` folder
3. Restart Arduino IDE

## 🔧 Dependencies

- **ArduinoJson** (>= 6.0.0) - For JSON parsing
- **WiFi library** (ESP8266WiFi, WiFi, or Ethernet)

## 🚀 Quick Start

### Basic Setup

```cpp
#include <KlipperAPI.h>
#include <ESP8266WiFi.h>  // or WiFi.h for ESP32

WiFiClient client;
KlipperApi api;

// Moonraker configuration
IPAddress moonrakerIp(192, 168, 1, 100);
const uint16_t moonraker_port = 7125;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin("Your_WiFi", "Your_Password");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  // Initialize KlipperAPI
  api.init(client, moonrakerIp, moonraker_port);
}

void loop() {
  // Get printer statistics
  if (api.getPrinterStatistics()) {
    Serial.print("Printer State: ");
    Serial.println(api.printerStats.state);
    
    Serial.print("Extruder Temp: ");
    Serial.print(api.printerStats.extruder.current);
    Serial.print("°C / ");
    Serial.print(api.printerStats.extruder.target);
    Serial.println("°C");
  }
  
  delay(10000); // Update every 10 seconds
}
```

## 📖 API Reference

### Initialization

```cpp
// Constructor with IP address
KlipperApi api(client, moonrakerIp, moonraker_port, api_key);

// Constructor with hostname
KlipperApi api(client, "moonraker.local", moonraker_port, api_key);

// Initialize after construction
api.init(client, moonrakerIp, moonraker_port, api_key);
```

### Printer Information

```cpp
// Get basic printer information
bool getPrinterInfo();

// Get comprehensive printer statistics
bool getPrinterStatistics();

// Get server information
bool getServerInfo();
```

### Print Job Management

```cpp
// Get current print job status
bool getPrintJob();

// Start a print job
bool startPrint(const char* filename);

// Control print job
bool pausePrint();
bool resumePrint();
bool cancelPrint();
```

### Temperature Control

```cpp
// Set temperatures
bool setExtruderTemperature(float temperature, uint8_t extruder = 0);
bool setBedTemperature(float temperature);
bool setFanSpeed(uint8_t speed, uint8_t fan = 0);
```

### Movement Control

```cpp
// Homing
bool homeAll();
bool homeAxis(char axis);  // 'X', 'Y', or 'Z'

// Movement
bool moveRelative(float x, float y, float z, float e, uint16_t feedrate = 3000);
bool moveAbsolute(float x, float y, float z, float e, uint16_t feedrate = 3000);
```

### G-code Execution

```cpp
// Send single G-code command
bool sendGcode(const char* gcode);

// Send multiple G-code commands
const char* gcodes[] = {"G28", "G1 Z10", "M114"};
bool sendGcodeMultiple(gcodes, 3);
```

### Emergency Controls

```cpp
bool emergencyStop();      // Emergency stop
bool restartFirmware();    // Restart Klipper firmware
bool restartHost();        // Restart host system
```

## 📊 Data Structures

### PrinterStatistics
```cpp
typedef struct {
  char state[16];                    // Current printer state
  PrinterStateFlags stateFlags;      // Bit flags for state
  TemperatureData extruder;          // Extruder temperature
  TemperatureData heatedBed;         // Bed temperature
  float positionX, positionY, positionZ, positionE;  // Current position
  uint16_t speedFactor;              // Speed factor percentage
  uint16_t flowFactor;               // Flow factor percentage
  uint8_t hasExtruder : 1;           // Hardware availability flags
  uint8_t hasHeatedBed : 1;
  uint8_t isHomed : 1;
} PrinterStatistics;
```

### PrintJobInfo
```cpp
typedef struct {
  char filename[64];                 // Current print filename
  char state[16];                    // Print job state
  float progress;                    // Progress 0.0-1.0
  uint32_t printTime;                // Current print time in seconds
  uint32_t estimatedTime;            // Estimated total time
  uint32_t timeLeft;                 // Estimated time left
  uint32_t fileSize;                 // File size in bytes
  uint8_t isPrinting : 1;            // Status flags
  uint8_t isPaused : 1;
  uint8_t isComplete : 1;
} PrintJobInfo;
```

### TemperatureData
```cpp
typedef struct {
  float current;                     // Current temperature
  float target;                      // Target temperature
  int16_t power;                     // Heater power (0-255)
} TemperatureData;
```

## 🔍 Examples

### Temperature Monitoring
```cpp
if (api.getPrinterStatistics()) {
  if (api.printerStats.hasExtruder) {
    Serial.print("Extruder: ");
    Serial.print(api.printerStats.extruder.current, 1);
    Serial.print("°C / ");
    Serial.print(api.printerStats.extruder.target, 1);
    Serial.println("°C");
  }
  
  if (api.printerStats.hasHeatedBed) {
    Serial.print("Bed: ");
    Serial.print(api.printerStats.heatedBed.current, 1);
    Serial.print("°C / ");
    Serial.print(api.printerStats.heatedBed.target, 1);
    Serial.println("°C");
  }
}
```

### Print Progress Monitoring
```cpp
if (api.getPrintJob() && api.printJob.isPrinting) {
  Serial.print("Printing: ");
  Serial.println(api.printJob.filename);
  Serial.print("Progress: ");
  Serial.print(api.printJob.progress * 100, 1);
  Serial.println("%");
  
  Serial.print("Time Left: ");
  Serial.print(api.printJob.timeLeft / 3600);
  Serial.print("h ");
  Serial.print((api.printJob.timeLeft % 3600) / 60);
  Serial.println("m");
}
```

### Safety Temperature Monitoring
```cpp
const float MAX_EXTRUDER_TEMP = 280.0;
const float MAX_BED_TEMP = 120.0;

if (api.getPrinterStatistics()) {
  if (api.printerStats.extruder.current > MAX_EXTRUDER_TEMP) {
    Serial.println("⚠️ Extruder temperature too high!");
    api.emergencyStop();
  }
  
  if (api.printerStats.heatedBed.current > MAX_BED_TEMP) {
    Serial.println("⚠️ Bed temperature too high!");
    api.emergencyStop();
  }
}
```

## ⚙️ Configuration

### Memory Configuration
Adjust buffer sizes in `KlipperAPI.h`:

```cpp
#define KAPI_TIMEOUT       5000      // Request timeout (ms)
#define POSTDATA_SIZE      256       // POST data buffer size
#define JSONDOCUMENT_SIZE  2048      // JSON parsing buffer size
```

### Debug Mode
Enable debug output:
```cpp
api._debug = true;  // Enable debug output to Serial
```

## 🔧 Moonraker Setup

Ensure your Moonraker configuration allows API access:

```ini
# moonraker.conf
[web_server]
host: 0.0.0.0
port: 7125
cors_domains:
    *

[api_key_file]
# Optional: Enable API key authentication
```

## 🐛 Troubleshooting

### Common Issues

**Connection Failed**
- Verify Moonraker IP address and port
- Check network connectivity
- Ensure Moonraker is running and accessible

**JSON Parsing Errors**
- Increase `JSONDOCUMENT_SIZE` if responses are large
- Check Moonraker API responses for changes

**Memory Issues**
- Reduce buffer sizes for smaller microcontrollers
- Use `api._debug = false` to reduce memory usage

**Authentication Errors**
- Check API key if required by your Moonraker setup
- Verify CORS configuration in Moonraker

## 📝 API Endpoints Used

This library uses the following Moonraker API endpoints:

- `/printer/info` - Basic printer information
- `/printer/objects/query` - Printer status and statistics  
- `/printer/gcode/script` - G-code command execution
- `/printer/print/start` - Start print job
- `/printer/print/pause` - Pause print job
- `/printer/print/resume` - Resume print job
- `/printer/print/cancel` - Cancel print job
- `/printer/emergency_stop` - Emergency stop
- `/server/info` - Server information

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add examples and documentation
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Klipper firmware team for the excellent 3D printer firmware
- Moonraker developers for the robust API server
- ArduinoJson library for efficient JSON parsing
- Arduino community for hardware abstraction libraries

## 📬 Support

- **Issues**: [GitHub Issues](https://github.com/your-repo/KlipperAPI/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-repo/KlipperAPI/discussions)
- **Documentation**: [Wiki](https://github.com/your-repo/KlipperAPI/wiki)

---

**Happy 3D Printing with Arduino! 🎯**
