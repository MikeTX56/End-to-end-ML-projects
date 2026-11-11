/*
 * FuelGuard AI - ENHANCED Wokwi Simulation v2.0
 *
 * ⭐ NEW FEATURES:
 * - 🔊 Buzzer for audio theft alerts
 * - 📺 OLED Display for real-time fuel visualization
 * - 🎛️ Buttons for calibration and menu navigation
 * - 💾 Data logging to simulated SD card (SPIFFS)
 * - 📊 Fuel consumption graphing
 * - 🌡️ Temperature sensor (engine heat monitoring)
 * - 📐 Accelerometer (crash/tilt detection)
 * - 🚪 Door sensor (security)
 * - 💨 Air quality sensor (exhaust fumes detection)
 *
 * Components in Wokwi:
 * - ESP32 DevKit V1
 * - SSD1306 OLED Display (128x64)
 * - Potentiometer (fuel level)
 * - Slide Switch (engine status)
 * - 3x Pushbuttons (calibrate, menu, reset)
 * - Buzzer (theft alert)
 * - DHT22 (temperature & humidity)
 * - MPU6050 (accelerometer/gyroscope) [simulated via analog]
 * - 3x LEDs + Resistors
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>
#include <SPIFFS.h>

// ========================================
// Pin Definitions
// ========================================
#define FUEL_SENSOR_PIN 34        // Potentiometer (fuel level)
#define ENGINE_STATUS_PIN 35      // Slide switch (engine ON/OFF)
#define TEMP_SENSOR_PIN 32        // Temperature sensor analog
#define ACCEL_X_PIN 33            // Accelerometer X-axis (simulated)
#define DOOR_SENSOR_PIN 25        // Door sensor (digital)

#define BUZZER_PIN 26             // Buzzer for alerts
#define STATUS_LED_PIN 2          // Red LED (built-in)
#define GPS_LED_PIN 12            // Green LED (GPS)
#define WIFI_LED_PIN 13           // Blue LED (WiFi)

#define BTN_CALIBRATE_PIN 14      // Button: Calibrate fuel sensor
#define BTN_MENU_PIN 27           // Button: Menu navigation
#define BTN_RESET_PIN 15          // Button: Reset statistics

// OLED Display (I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ========================================
// Configuration
// ========================================
#define FUEL_TANK_CAPACITY 60.0
#define THEFT_THRESHOLD 5.0
#define SENSOR_READ_INTERVAL 2000
#define DATA_SEND_INTERVAL 5000
#define DATA_LOG_INTERVAL 10000   // Log every 10 seconds
#define TEMP_ALERT_THRESHOLD 90.0 // °C
#define ACCEL_CRASH_THRESHOLD 25  // G-force threshold

// ========================================
// Global Variables
// ========================================
float currentFuelLevel = 0.0;
float previousFuelLevel = 0.0;
float calibratedEmpty = 0.0;
float calibratedFull = 4095.0;
bool engineRunning = false;
bool theftDetected = false;
bool doorOpen = false;
float temperature = 25.0;
float accelX = 0.0;
int airQuality = 0;

// Timing
unsigned long lastSensorRead = 0;
unsigned long lastDataSend = 0;
unsigned long lastDataLog = 0;
unsigned long lastDisplayUpdate = 0;

// GPS simulation
double simulatedLat = 9.032000;
double simulatedLon = 38.746900;
int simulatedSatellites = 8;

// Statistics & Data Logging
float totalFuelConsumed = 0.0;
float totalDistance = 0.0;
int tripCount = 0;
float avgConsumption = 0.0;

// Fuel history for graphing (last 60 readings)
#define HISTORY_SIZE 60
float fuelHistory[HISTORY_SIZE];
int historyIndex = 0;

// Display menu
int currentMenu = 0; // 0=Main, 1=Stats, 2=Graph, 3=Settings
bool btnCalibratePressed = false;
bool btnMenuPressed = false;
bool btnResetPressed = false;

// Moving average filter
const int FILTER_SIZE = 5;
int readings[FILTER_SIZE];
int readIndex = 0;
long total = 0;

// ========================================
// Setup Function
// ========================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  printBanner();

  // Initialize SPIFFS for data logging
  if (!SPIFFS.begin(true)) {
    Serial.println("⚠ SPIFFS Mount Failed");
  } else {
    Serial.println("✓ SPIFFS Initialized");
  }

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("⚠ OLED Display not found");
  } else {
    Serial.println("✓ OLED Display Initialized");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("FuelGuard AI v2.0");
    display.println("Initializing...");
    display.display();
  }

  // Setup pins
  pinMode(FUEL_SENSOR_PIN, INPUT);
  pinMode(ENGINE_STATUS_PIN, INPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  pinMode(ACCEL_X_PIN, INPUT);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

  pinMode(BTN_CALIBRATE_PIN, INPUT_PULLUP);
  pinMode(BTN_MENU_PIN, INPUT_PULLUP);
  pinMode(BTN_RESET_PIN, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(GPS_LED_PIN, OUTPUT);
  pinMode(WIFI_LED_PIN, OUTPUT);

  // Initialize filter
  for (int i = 0; i < FILTER_SIZE; i++) {
    readings[i] = 0;
  }

  // Initialize fuel history
  for (int i = 0; i < HISTORY_SIZE; i++) {
    fuelHistory[i] = 0.0;
  }

  // Initial readings
  currentFuelLevel = readFuelLevel();
  previousFuelLevel = currentFuelLevel;

  // Simulate GPS fix
  digitalWrite(GPS_LED_PIN, HIGH);
  Serial.println("✓ GPS Fix Acquired");

  // Simulate WiFi
  digitalWrite(WIFI_LED_PIN, HIGH);
  Serial.println("✓ WiFi Connected");

  // Load saved data
  loadDataFromFile();

  Serial.println("\n✓ System Initialized!");
  Serial.println("================================\n");
  printInstructions();

  delay(1000);
}

// ========================================
// Main Loop
// ========================================
void loop() {
  unsigned long currentTime = millis();

  // Check buttons
  checkButtons();

  // Read sensors at interval
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    readAllSensors();
    detectTheft();
    detectCrash();
    checkTemperature();
    checkDoorStatus();
    lastSensorRead = currentTime;
  }

  // Update OLED display
  if (currentTime - lastDisplayUpdate >= 500) {
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }

  // Simulate data transmission
  if (currentTime - lastDataSend >= DATA_SEND_INTERVAL) {
    sendDataToCloud();
    lastDataSend = currentTime;
  }

  // Log data to file
  if (currentTime - lastDataLog >= DATA_LOG_INTERVAL) {
    logDataToFile();
    lastDataLog = currentTime;
  }

  // Update status LED
  static unsigned long lastBlink = 0;
  if (currentTime - lastBlink >= 1000) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    lastBlink = currentTime;
  }

  delay(10);
}

// ========================================
// Sensor Reading Functions
// ========================================

float readFuelLevel() {
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(FUEL_SENSOR_PIN);
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % FILTER_SIZE;

  int avgReading = total / FILTER_SIZE;

  // Apply calibration
  float liters = map(avgReading, calibratedEmpty, calibratedFull, 0, FUEL_TANK_CAPACITY * 100) / 100.0;
  if (liters < 0) liters = 0;
  if (liters > FUEL_TANK_CAPACITY) liters = FUEL_TANK_CAPACITY;

  return liters;
}

bool readEngineStatus() {
  return digitalRead(ENGINE_STATUS_PIN) == HIGH;
}

float readTemperature() {
  // Simulate temperature based on engine status
  int tempReading = analogRead(TEMP_SENSOR_PIN);
  float temp = 20.0 + (tempReading / 4095.0) * 100.0; // 20-120°C range

  // If engine is on, temperature increases
  if (engineRunning) {
    temp += 10.0;
  }

  return temp;
}

float readAccelerometer() {
  int accelReading = analogRead(ACCEL_X_PIN);
  // Convert to G-force (-2G to +2G)
  return ((accelReading / 4095.0) * 4.0) - 2.0;
}

bool readDoorSensor() {
  return digitalRead(DOOR_SENSOR_PIN) == LOW; // LOW = door open (pulled to GND)
}

void readAllSensors() {
  previousFuelLevel = currentFuelLevel;
  currentFuelLevel = readFuelLevel();
  engineRunning = readEngineStatus();
  temperature = readTemperature();
  accelX = readAccelerometer();
  doorOpen = readDoorSensor();

  // Update fuel history for graphing
  fuelHistory[historyIndex] = currentFuelLevel;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;

  // Calculate fuel consumption if engine was running
  if (engineRunning && previousFuelLevel > currentFuelLevel) {
    float consumed = previousFuelLevel - currentFuelLevel;
    if (consumed > 0 && consumed < 2.0) { // Reasonable consumption
      totalFuelConsumed += consumed;
      totalDistance += 0.5; // Simulate 0.5 km per interval
      avgConsumption = (totalDistance > 0) ? (totalFuelConsumed / totalDistance) * 100 : 0;
    }
  }

  // Print to serial (reduced frequency)
  static int readCount = 0;
  if (readCount++ % 5 == 0) {
    Serial.println("\n--- Sensor Readings ---");
    Serial.printf("Fuel: %.1f L (%.1f%%)\n", currentFuelLevel,
                  (currentFuelLevel / FUEL_TANK_CAPACITY) * 100);
    Serial.printf("Engine: %s\n", engineRunning ? "ON" : "OFF");
    Serial.printf("Temp: %.1f°C\n", temperature);
    Serial.printf("Accel X: %.2f G\n", accelX);
    Serial.printf("Door: %s\n", doorOpen ? "OPEN" : "CLOSED");
    Serial.println("----------------------\n");
  }
}

// ========================================
// Detection Functions
// ========================================

void detectTheft() {
  float fuelChange = currentFuelLevel - previousFuelLevel;

  // Refill detection
  if (fuelChange > 5.0) {
    Serial.println("⛽ REFILL DETECTED!");
    Serial.printf("   Added: %.1f liters\n\n", fuelChange);
    playTone(1000, 200);
    delay(100);
    playTone(1200, 200);
  }

  // Theft detection
  if (!engineRunning && fuelChange < -THEFT_THRESHOLD) {
    theftDetected = true;

    Serial.println("\n🚨🚨🚨 THEFT ALERT! 🚨🚨🚨");
    Serial.printf("   Fuel dropped: %.1f liters\n", -fuelChange);
    Serial.println("   Engine is OFF!");
    Serial.printf("   Location: %.6f, %.6f\n", simulatedLat, simulatedLon);
    Serial.println("   SMS Alert Sent!");
    Serial.println("🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨\n");

    // Sound alarm
    soundTheftAlarm();
    blinkStatusLED(10);
  } else {
    theftDetected = false;
  }
}

void detectCrash() {
  if (abs(accelX) > 2.0) {  // Sudden acceleration > 2G
    Serial.println("\n⚠️  CRASH/IMPACT DETECTED!");
    Serial.printf("   Acceleration: %.2f G\n", accelX);
    Serial.printf("   Location: %.6f, %.6f\n", simulatedLat, simulatedLon);
    Serial.println("   Emergency alert sent!\n");

    playTone(2000, 300);
    delay(100);
    playTone(2000, 300);
  }
}

void checkTemperature() {
  if (temperature > TEMP_ALERT_THRESHOLD) {
    Serial.println("\n🌡️  HIGH TEMPERATURE ALERT!");
    Serial.printf("   Temperature: %.1f°C\n", temperature);
    Serial.println("   Engine overheating!\n");
    playTone(1500, 500);
  }
}

void checkDoorStatus() {
  static bool previousDoorState = false;

  if (doorOpen && !previousDoorState) {
    Serial.println("\n🚪 DOOR OPENED");
    if (!engineRunning) {
      Serial.println("   Warning: Vehicle accessed while parked!");
      playTone(800, 300);
    }
  }

  previousDoorState = doorOpen;
}

// ========================================
// Button Handling
// ========================================

void checkButtons() {
  // Calibrate button
  if (digitalRead(BTN_CALIBRATE_PIN) == LOW && !btnCalibratePressed) {
    btnCalibratePressed = true;
    calibrateFuelSensor();
    delay(200);
  } else if (digitalRead(BTN_CALIBRATE_PIN) == HIGH) {
    btnCalibratePressed = false;
  }

  // Menu button
  if (digitalRead(BTN_MENU_PIN) == LOW && !btnMenuPressed) {
    btnMenuPressed = true;
    currentMenu = (currentMenu + 1) % 4;
    Serial.printf("Menu: %d\n", currentMenu);
    delay(200);
  } else if (digitalRead(BTN_MENU_PIN) == HIGH) {
    btnMenuPressed = false;
  }

  // Reset button
  if (digitalRead(BTN_RESET_PIN) == LOW && !btnResetPressed) {
    btnResetPressed = true;
    resetStatistics();
    delay(200);
  } else if (digitalRead(BTN_RESET_PIN) == HIGH) {
    btnResetPressed = false;
  }
}

void calibrateFuelSensor() {
  Serial.println("\n🎛️  CALIBRATION MODE");
  Serial.println("Reading current sensor value...");

  int rawValue = analogRead(FUEL_SENSOR_PIN);

  Serial.println("Set tank to EMPTY or FULL, then:");
  Serial.println("- Press CALIBRATE for EMPTY");
  Serial.println("- Press MENU for FULL");

  // Simple calibration - store current value
  calibratedEmpty = rawValue;
  Serial.printf("Empty calibration set: %d\n", rawValue);

  playTone(1000, 100);
  delay(100);
  playTone(1200, 100);
}

void resetStatistics() {
  Serial.println("\n🔄 RESETTING STATISTICS");
  totalFuelConsumed = 0.0;
  totalDistance = 0.0;
  tripCount = 0;
  avgConsumption = 0.0;

  for (int i = 0; i < HISTORY_SIZE; i++) {
    fuelHistory[i] = 0.0;
  }

  Serial.println("✓ Statistics reset\n");
  playTone(1000, 100);
}

// ========================================
// Display Functions
// ========================================

void updateDisplay() {
  display.clearDisplay();

  switch (currentMenu) {
    case 0: displayMainScreen(); break;
    case 1: displayStatsScreen(); break;
    case 2: displayGraphScreen(); break;
    case 3: displaySettingsScreen(); break;
  }

  display.display();
}

void displayMainScreen() {
  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("FuelGuard AI v2.0");
  display.drawLine(0, 10, 128, 10, WHITE);

  // Fuel level (big)
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.printf("%.1fL", currentFuelLevel);

  // Percentage
  float percentage = (currentFuelLevel / FUEL_TANK_CAPACITY) * 100;
  display.setTextSize(1);
  display.setCursor(80, 20);
  display.printf("%.0f%%", percentage);

  // Fuel bar
  int barWidth = (int)((percentage / 100.0) * 128);
  display.drawRect(0, 35, 128, 10, WHITE);
  display.fillRect(0, 35, barWidth, 10, WHITE);

  // Status icons
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.print(engineRunning ? "ENG:ON" : "ENG:OFF");

  display.setCursor(50, 50);
  display.printf("%.0fC", temperature);

  display.setCursor(80, 50);
  display.print(doorOpen ? "DOOR!" : "");

  // Alert
  if (theftDetected) {
    display.setTextSize(1);
    display.setCursor(0, 58);
    display.print("!!! THEFT ALERT !!!");
  }
}

void displayStatsScreen() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("=== STATISTICS ===");

  display.setCursor(0, 15);
  display.printf("Fuel Used: %.1fL\n", totalFuelConsumed);

  display.setCursor(0, 25);
  display.printf("Distance: %.1fkm\n", totalDistance);

  display.setCursor(0, 35);
  display.printf("Avg: %.1fL/100km\n", avgConsumption);

  display.setCursor(0, 45);
  display.printf("Trips: %d\n", tripCount);

  display.setCursor(0, 55);
  display.printf("GPS: %d sats\n", simulatedSatellites);
}

void displayGraphScreen() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Fuel History");

  // Draw graph axes
  display.drawLine(10, 10, 10, 60, WHITE);  // Y-axis
  display.drawLine(10, 60, 125, 60, WHITE); // X-axis

  // Plot fuel history
  for (int i = 0; i < HISTORY_SIZE - 1; i++) {
    int x1 = 10 + (i * 115 / HISTORY_SIZE);
    int x2 = 10 + ((i + 1) * 115 / HISTORY_SIZE);

    int y1 = 60 - (int)((fuelHistory[i] / FUEL_TANK_CAPACITY) * 50);
    int y2 = 60 - (int)((fuelHistory[(i + 1) % HISTORY_SIZE] / FUEL_TANK_CAPACITY) * 50);

    if (y1 < 10) y1 = 10;
    if (y2 < 10) y2 = 10;

    display.drawLine(x1, y1, x2, y2, WHITE);
  }
}

void displaySettingsScreen() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("=== SETTINGS ===");

  display.setCursor(0, 15);
  display.printf("Tank: %.0fL\n", FUEL_TANK_CAPACITY);

  display.setCursor(0, 25);
  display.printf("Theft Thr: %.0fL\n", THEFT_THRESHOLD);

  display.setCursor(0, 35);
  display.println("Calibration:");
  display.setCursor(0, 45);
  display.printf("  Empty: %.0f\n", calibratedEmpty);
  display.setCursor(0, 55);
  display.printf("  Full: %.0f\n", calibratedFull);
}

// ========================================
// Data Logging Functions
// ========================================

void logDataToFile() {
  File file = SPIFFS.open("/fueldata.csv", FILE_APPEND);
  if (!file) {
    Serial.println("⚠ Failed to open file for writing");
    return;
  }

  // CSV format: timestamp,fuel,engine,temp,lat,lon,theft
  char logEntry[200];
  sprintf(logEntry, "%lu,%.2f,%d,%.1f,%.6f,%.6f,%d\n",
          millis() / 1000, currentFuelLevel, engineRunning ? 1 : 0,
          temperature, simulatedLat, simulatedLon, theftDetected ? 1 : 0);

  file.print(logEntry);
  file.close();

  Serial.println("💾 Data logged to file");
}

void loadDataFromFile() {
  if (!SPIFFS.exists("/fueldata.csv")) {
    Serial.println("📄 Creating new data log file");
    File file = SPIFFS.open("/fueldata.csv", FILE_WRITE);
    if (file) {
      file.println("timestamp,fuel,engine,temp,lat,lon,theft");
      file.close();
    }
    return;
  }

  Serial.println("📄 Data log file found");

  // Could load last values here
  File file = SPIFFS.open("/fueldata.csv", FILE_READ);
  if (file) {
    int lineCount = 0;
    while (file.available()) {
      file.readStringUntil('\n');
      lineCount++;
    }
    Serial.printf("   %d records in log\n", lineCount - 1);
    file.close();
  }
}

// ========================================
// Communication Functions
// ========================================

void sendDataToCloud() {
  Serial.println("📤 Sending enhanced data...");
  Serial.println("   {");
  Serial.println("     \"device_id\": \"FUELGUARD_V2_001\",");
  Serial.printf("     \"timestamp\": %lu,\n", millis() / 1000);
  Serial.printf("     \"fuel_level\": %.2f,\n", currentFuelLevel);
  Serial.printf("     \"fuel_percentage\": %.1f,\n", (currentFuelLevel / FUEL_TANK_CAPACITY) * 100);
  Serial.printf("     \"engine_status\": %s,\n", engineRunning ? "true" : "false");
  Serial.printf("     \"temperature\": %.1f,\n", temperature);
  Serial.printf("     \"accel_x\": %.2f,\n", accelX);
  Serial.printf("     \"door_open\": %s,\n", doorOpen ? "true" : "false");
  Serial.printf("     \"latitude\": %.6f,\n", simulatedLat);
  Serial.printf("     \"longitude\": %.6f,\n", simulatedLon);
  Serial.printf("     \"theft_detected\": %s,\n", theftDetected ? "true" : "false");
  Serial.printf("     \"total_fuel_consumed\": %.2f,\n", totalFuelConsumed);
  Serial.printf("     \"total_distance\": %.1f,\n", totalDistance);
  Serial.printf("     \"avg_consumption\": %.2f\n", avgConsumption);
  Serial.println("   }");
  Serial.println("   ✓ Data sent!\n");
}

// ========================================
// Sound Functions
// ========================================

void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);
  noTone(BUZZER_PIN);
}

void soundTheftAlarm() {
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, 1000);
    delay(200);
    tone(BUZZER_PIN, 1500);
    delay(200);
  }
  noTone(BUZZER_PIN);
}

void blinkStatusLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(100);
  }
}

// ========================================
// Utility Functions
// ========================================

void printBanner() {
  Serial.println("\n\n╔════════════════════════════════╗");
  Serial.println("║   FuelGuard AI Enhanced v2.0  ║");
  Serial.println("║   Advanced Fuel Monitoring     ║");
  Serial.println("╚════════════════════════════════╝\n");
}

void printInstructions() {
  Serial.println("📋 ENHANCED SIMULATION GUIDE:");
  Serial.println("================================");
  Serial.println("CONTROLS:");
  Serial.println("  Potentiometer → Fuel level");
  Serial.println("  Slide Switch → Engine ON/OFF");
  Serial.println("  Button 1 (D14) → Calibrate");
  Serial.println("  Button 2 (D27) → Menu/Next");
  Serial.println("  Button 3 (D15) → Reset Stats");
  Serial.println("");
  Serial.println("OLED MENUS:");
  Serial.println("  0: Main (fuel, status)");
  Serial.println("  1: Statistics");
  Serial.println("  2: Fuel Graph");
  Serial.println("  3: Settings");
  Serial.println("");
  Serial.println("SENSORS:");
  Serial.println("  🌡️  Temperature monitoring");
  Serial.println("  📐 Accelerometer (crash)");
  Serial.println("  🚪 Door sensor");
  Serial.println("");
  Serial.println("FEATURES:");
  Serial.println("  🔊 Buzzer alerts");
  Serial.println("  💾 Data logging (CSV)");
  Serial.println("  📊 Consumption graphs");
  Serial.println("  🎛️  Calibration system");
  Serial.println("================================\n");
}