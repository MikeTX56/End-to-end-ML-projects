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

// Forward declarations
float readFuelLevel();
void printBanner();
void printInstructions();
void checkButtons();
void readAllSensors();
void detectTheft();
void detectCrash();
void checkTemperature();
void checkDoorStatus();
void updateDisplay();
void sendDataToCloud();
void logDataToFile();
void loadDataFromFile();
void playAlertTone(int duration, int frequency);
void handleMenu();
void calibrateSensor();
void resetStatistics();

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

void readAllSensors() {
  previousFuelLevel = currentFuelLevel;
  currentFuelLevel = readFuelLevel();

  // Engine status from switch
  engineRunning = digitalRead(ENGINE_STATUS_PIN) == HIGH;

  // Temperature simulation (analog to Celsius)
  int tempReading = analogRead(TEMP_SENSOR_PIN);
  temperature = map(tempReading, 0, 4095, 20, 120);

  // Accelerometer simulation
  int accelReading = analogRead(ACCEL_X_PIN);
  accelX = map(accelReading, 0, 4095, -30, 30);

  // Door sensor
  doorOpen = digitalRead(DOOR_SENSOR_PIN) == LOW;

  // Air quality simulation
  airQuality = random(50, 200);

  // Update history
  fuelHistory[historyIndex] = currentFuelLevel;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;

  // Calculate consumption when engine running
  if (engineRunning && previousFuelLevel > currentFuelLevel) {
    float consumed = previousFuelLevel - currentFuelLevel;
    totalFuelConsumed += consumed;
    totalDistance += random(1, 5) / 10.0; // Simulate distance in km

    if (totalDistance > 0) {
      avgConsumption = (totalFuelConsumed / totalDistance) * 100;
    }
  }

  // Print status to Serial
  Serial.println("=== Sensor Data ===");
  Serial.print("Fuel: "); Serial.print(currentFuelLevel, 2); Serial.println(" L");
  Serial.print("Engine: "); Serial.println(engineRunning ? "ON" : "OFF");
  Serial.print("Temp: "); Serial.print(temperature, 1); Serial.println(" C");
  Serial.print("Door: "); Serial.println(doorOpen ? "OPEN" : "CLOSED");
  Serial.print("Accel X: "); Serial.print(accelX, 1); Serial.println(" G");
  Serial.println("===================\n");
}

// ========================================
// Theft Detection
// ========================================

void detectTheft() {
  if (!engineRunning && (previousFuelLevel - currentFuelLevel) > THEFT_THRESHOLD) {
    theftDetected = true;

    Serial.println("🚨🚨🚨 THEFT ALERT! 🚨🚨🚨");
    Serial.print("Fuel dropped by: ");
    Serial.print(previousFuelLevel - currentFuelLevel, 2);
    Serial.println(" L");
    Serial.print("Location: ");
    Serial.print(simulatedLat, 6);
    Serial.print(", ");
    Serial.println(simulatedLon, 6);

    // Activate buzzer alert pattern
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 1000, 200);
      delay(300);
      tone(BUZZER_PIN, 1500, 200);
      delay(300);
    }
    noTone(BUZZER_PIN);

    // Flash all LEDs
    for (int i = 0; i < 5; i++) {
      digitalWrite(STATUS_LED_PIN, HIGH);
      digitalWrite(GPS_LED_PIN, HIGH);
      digitalWrite(WIFI_LED_PIN, HIGH);
      delay(100);
      digitalWrite(STATUS_LED_PIN, LOW);
      digitalWrite(GPS_LED_PIN, LOW);
      digitalWrite(WIFI_LED_PIN, LOW);
      delay(100);
    }

    // Log theft event
    File file = SPIFFS.open("/theft_log.txt", FILE_APPEND);
    if (file) {
      file.print(millis());
      file.print(",THEFT,");
      file.print(previousFuelLevel - currentFuelLevel);
      file.print(",");
      file.print(simulatedLat, 6);
      file.print(",");
      file.println(simulatedLon, 6);
      file.close();
    }
  } else {
    theftDetected = false;
  }
}

// ========================================
// Additional Safety Features
// ========================================

void detectCrash() {
  if (abs(accelX) > ACCEL_CRASH_THRESHOLD) {
    Serial.println("⚠ CRASH DETECTED! High acceleration!");
    tone(BUZZER_PIN, 2000, 1000);
  }
}

void checkTemperature() {
  if (temperature > TEMP_ALERT_THRESHOLD) {
    Serial.println("🔥 HIGH TEMPERATURE ALERT!");
    Serial.print("Engine temp: ");
    Serial.print(temperature, 1);
    Serial.println(" C");
  }
}

void checkDoorStatus() {
  if (doorOpen && !engineRunning) {
    Serial.println("🚪 Security Alert: Door open while engine off!");
  }
}

// ========================================
// Display Functions
// ========================================

void updateDisplay() {
  display.clearDisplay();

  switch (currentMenu) {
    case 0: { // Main screen
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("FuelGuard AI");

      display.setTextSize(2);
      display.setCursor(0, 15);
      display.print(currentFuelLevel, 1);
      display.print("L");

      // Fuel bar
      int barWidth = map(currentFuelLevel * 10, 0, FUEL_TANK_CAPACITY * 10, 0, 120);
      display.drawRect(4, 40, 120, 10, WHITE);
      display.fillRect(4, 40, barWidth, 10, WHITE);

      display.setTextSize(1);
      display.setCursor(0, 55);
      display.print(engineRunning ? "Engine:ON" : "Engine:OFF");

      if (theftDetected) {
        display.setCursor(70, 55);
        display.print("ALERT!");
      }
      break;
    }

    case 1: // Statistics screen
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Trip Statistics");
      display.println("------------");
      display.print("Consumed: ");
      display.print(totalFuelConsumed, 1);
      display.println("L");
      display.print("Distance: ");
      display.print(totalDistance, 1);
      display.println("km");
      display.print("Avg: ");
      display.print(avgConsumption, 1);
      display.println("L/100km");
      break;

    case 2: // Graph screen
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Fuel History");

      // Draw simple graph
      for (int i = 0; i < 60; i++) {
        int idx = (historyIndex + i) % HISTORY_SIZE;
        int y = map(fuelHistory[idx] * 10, 0, FUEL_TANK_CAPACITY * 10, 63, 15);
        display.drawPixel(i * 2, y, WHITE);
      }
      break;

    case 3: // Settings screen
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Settings");
      display.println("--------");
      display.println("CAL: Recalibrate");
      display.println("RST: Reset stats");
      display.print("Threshold:");
      display.print(THEFT_THRESHOLD);
      display.println("L");
      break;
  }

  display.display();
}

// ========================================
// Button Handling
// ========================================

void checkButtons() {
  static unsigned long lastDebounce = 0;
  if (millis() - lastDebounce < 200) return;

  if (digitalRead(BTN_MENU_PIN) == LOW) {
    currentMenu = (currentMenu + 1) % 4;
    lastDebounce = millis();
    Serial.print("Menu changed to: ");
    Serial.println(currentMenu);
  }

  if (digitalRead(BTN_CALIBRATE_PIN) == LOW) {
    calibrateSensor();
    lastDebounce = millis();
  }

  if (digitalRead(BTN_RESET_PIN) == LOW) {
    resetStatistics();
    lastDebounce = millis();
  }
}

// ========================================
// Data Functions
// ========================================

void sendDataToCloud() {
  Serial.println("📤 Sending data to cloud...");
  Serial.print("{");
  Serial.print("\"fuel\":"); Serial.print(currentFuelLevel, 2);
  Serial.print(",\"engine\":"); Serial.print(engineRunning ? "true" : "false");
  Serial.print(",\"temp\":"); Serial.print(temperature, 1);
  Serial.print(",\"location\":{\"lat\":"); Serial.print(simulatedLat, 6);
  Serial.print(",\"lon\":"); Serial.print(simulatedLon, 6);
  Serial.println("}}");
}

void logDataToFile() {
  File file = SPIFFS.open("/fuel_data.csv", FILE_APPEND);
  if (file) {
    file.print(millis());
    file.print(",");
    file.print(currentFuelLevel, 2);
    file.print(",");
    file.print(engineRunning);
    file.print(",");
    file.print(temperature, 1);
    file.print(",");
    file.print(totalFuelConsumed, 2);
    file.println();
    file.close();
    Serial.println("💾 Data logged to SPIFFS");
  }
}

void loadDataFromFile() {
  File file = SPIFFS.open("/fuel_data.csv", FILE_READ);
  if (file) {
    Serial.println("📂 Previous data found");
    file.close();
  }
}

// ========================================
// Utility Functions
// ========================================

void calibrateSensor() {
  Serial.println("🎛️ Calibrating sensor...");
  Serial.println("Set tank EMPTY and press button...");
  delay(2000);
  calibratedEmpty = analogRead(FUEL_SENSOR_PIN);

  Serial.println("Set tank FULL and press button...");
  delay(2000);
  calibratedFull = analogRead(FUEL_SENSOR_PIN);

  Serial.println("✓ Calibration complete!");
  Serial.print("Empty: "); Serial.println(calibratedEmpty);
  Serial.print("Full: "); Serial.println(calibratedFull);

  tone(BUZZER_PIN, 1500, 500);
}

void resetStatistics() {
  totalFuelConsumed = 0;
  totalDistance = 0;
  avgConsumption = 0;
  tripCount = 0;

  Serial.println("🔄 Statistics reset");
  tone(BUZZER_PIN, 1000, 200);
}

void printBanner() {
  Serial.println("========================================");
  Serial.println("   FUELGUARD AI - ENHANCED v2.0");
  Serial.println("========================================");
  Serial.println("Features:");
  Serial.println("  ✓ Theft Detection");
  Serial.println("  ✓ GPS Tracking");
  Serial.println("  ✓ OLED Display");
  Serial.println("  ✓ Buzzer Alerts");
  Serial.println("  ✓ Data Logging");
  Serial.println("  ✓ Crash Detection");
  Serial.println("  ✓ Temperature Monitoring");
  Serial.println("========================================\n");
}

void printInstructions() {
  Serial.println("📋 Controls:");
  Serial.println("  - MENU button: Switch display screens");
  Serial.println("  - CAL button: Calibrate fuel sensor");
  Serial.println("  - RESET button: Reset trip statistics");
  Serial.println("\n🔧 Wokwi Components:");
  Serial.println("  - Potentiometer: Adjust fuel level");
  Serial.println("  - Slide Switch: Engine ON/OFF");
  Serial.println("  - Buttons: Menu/Calibrate/Reset");
  Serial.println("  - DHT22: Temperature sensor");
  Serial.println("  - OLED: Real-time display");
  Serial.println();
}
