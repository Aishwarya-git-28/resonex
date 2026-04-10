/*
 * ═══════════════════════════════════════════════════════════════
 *  RESONEX — Non-Invasive Acoustic Sensing for Infrastructure
 *  Team Visionary Loop | FantomCode 2026
 * ═══════════════════════════════════════════════════════════════
 *
 *  WIRING:
 *  Piezo sensor (+)  → GPIO 34
 *  Piezo sensor (-)  → GND
 *  Green LED         → GPIO 25 (via 220Ω resistor)
 *  Yellow LED        → GPIO 26 (via 220Ω resistor)
 *  Red LED           → GPIO 27 (via 220Ω resistor)
 *  Buzzer            → GPIO 15
 *
 *  BEFORE FLASHING:
 *  1. Change WIFI_SSID and WIFI_PASSWORD to your hotspot
 *  2. Run ipconfig on your laptop, find IPv4 address
 *  3. Replace SERVER_URL with that IP
 *  4. Change MODEL_NAME to match which model this ESP32 is on
 *     Options: "transformer" / "bridge" / "tower"
 */

#include <WiFi.h>
#include <HTTPClient.h>

// ─── CHANGE THESE BEFORE FLASHING ─────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* SERVER_URL    = "http://192.168.1.100:5000/data";
const char* MODEL_NAME    = "transformer";  // transformer / bridge / tower
// ──────────────────────────────────────────────────────────────

// ─── PIN DEFINITIONS ──────────────────────────────────────────
#define SENSOR_PIN    34
#define GREEN_LED     25
#define YELLOW_LED    26
#define RED_LED       27
#define BUZZER_PIN    15

// ─── SAMPLING SETTINGS ────────────────────────────────────────
#define SAMPLES       512     // readings per window
#define SAMPLE_DELAY  500     // microseconds between samples (~2kHz)

// ─── CLASSIFICATION THRESHOLDS ────────────────────────────────
// Tune these based on your Serial Monitor readings:
// - Tap sensor gently  → note the average value → that is your threshold1
// - Tap sensor hard    → note the average value → that is your threshold2
// Rule: average < threshold1 = Normal
//       average < threshold2 = Warning
//       average >= threshold2 = Critical
int threshold1 = 600;
int threshold2 = 1000;

// ─── GLOBALS ──────────────────────────────────────────────────
int sensorBuffer[SAMPLES];
bool wifiConnected = false;
int loopCount = 0;

// ─── SETUP ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("=============================");
  Serial.println("  RESONEX — Starting up");
  Serial.println("=============================");

  // Configure output pins
  pinMode(GREEN_LED,  OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED,    OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Boot sequence — all LEDs flash once
  allLedsOn();
  delay(300);
  allLedsOff();
  delay(200);
  allLedsOn();
  delay(300);
  allLedsOff();

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;

    // Blink yellow while connecting
    digitalWrite(YELLOW_LED, attempts % 2 == 0 ? HIGH : LOW);
  }

  digitalWrite(YELLOW_LED, LOW);

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Sending data to: ");
    Serial.println(SERVER_URL);
    Serial.print("Model: ");
    Serial.println(MODEL_NAME);

    // Green flash = WiFi success
    for (int i = 0; i < 3; i++) {
      digitalWrite(GREEN_LED, HIGH);
      delay(150);
      digitalWrite(GREEN_LED, LOW);
      delay(150);
    }
  } else {
    wifiConnected = false;
    Serial.println("\nWiFi failed — running in serial-only mode");
    Serial.println("Data will print to Serial Monitor only");

    // Red flash = WiFi failed
    for (int i = 0; i < 3; i++) {
      digitalWrite(RED_LED, HIGH);
      delay(150);
      digitalWrite(RED_LED, LOW);
      delay(150);
    }
  }

  Serial.println("-----------------------------");
  Serial.println("RESONEX ready. Monitoring...");
  Serial.println("-----------------------------");
}

// ─── MAIN LOOP ────────────────────────────────────────────────
void loop() {
  loopCount++;

  // ── Step 1: Collect samples ──
  int minVal = 4095, maxVal = 0, sum = 0;

  for (int i = 0; i < SAMPLES; i++) {
    sensorBuffer[i] = analogRead(SENSOR_PIN);
    sum    += sensorBuffer[i];
    if (sensorBuffer[i] < minVal) minVal = sensorBuffer[i];
    if (sensorBuffer[i] > maxVal) maxVal = sensorBuffer[i];
    delayMicroseconds(SAMPLE_DELAY);
  }

  // ── Step 2: Calculate metrics ──
  int   average    = sum / SAMPLES;
  int   peakToPeak = maxVal - minVal;
  float amplitude  = peakToPeak / 4095.0;
  float confidence = 0.91;  // placeholder until CNN model deployed

  // ── Step 3: Classify ──
  String status;
  int statusCode;

  if (average < threshold1) {
    status     = "normal";
    statusCode = 0;
    setLeds(HIGH, LOW, LOW);
    digitalWrite(BUZZER_PIN, LOW);

  } else if (average < threshold2) {
    status     = "warning";
    statusCode = 1;
    setLeds(LOW, HIGH, LOW);
    digitalWrite(BUZZER_PIN, LOW);

  } else {
    status     = "critical";
    statusCode = 2;
    setLeds(LOW, LOW, HIGH);
    beepBuzzer();
  }

  // ── Step 4: Print to Serial Monitor ──
  Serial.printf("[%d] model:%s status:%s avg:%d p2p:%d amp:%.2f\n",
    loopCount, MODEL_NAME, status.c_str(), average, peakToPeak, amplitude);

  // ── Step 5: Send to Flask dashboard ──
  if (wifiConnected && WiFi.status() == WL_CONNECTED) {
    sendToFlask(status, average, amplitude, confidence);
  } else {
    // Try to reconnect every 10 loops if WiFi dropped
    if (loopCount % 10 == 0) {
      Serial.println("Attempting WiFi reconnect...");
      WiFi.reconnect();
      delay(2000);
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("WiFi reconnected!");
      }
    }
  }

  delay(1000);
}

// ─── SEND DATA TO FLASK ───────────────────────────────────────
void sendToFlask(String status, int freq, float amplitude, float confidence) {
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(3000);  // 3 second timeout so it doesn't hang

  String json = "{";
  json += "\"model\":\"";      json += MODEL_NAME;      json += "\",";
  json += "\"status\":\"";     json += status;           json += "\",";
  json += "\"confidence\":";   json += String(confidence, 2); json += ",";
  json += "\"dominant_freq\":"; json += String(freq);    json += ",";
  json += "\"amplitude\":";    json += String(amplitude, 2); json += ",";
  json += "\"timestamp\":\"live\"";
  json += "}";

  int httpCode = http.POST(json);

  if (httpCode == 200) {
    Serial.printf("  → Dashboard updated (HTTP %d)\n", httpCode);
  } else {
    Serial.printf("  → HTTP error: %d — check SERVER_URL and Flask\n", httpCode);
  }

  http.end();
}

// ─── HELPERS ──────────────────────────────────────────────────
void setLeds(int g, int y, int r) {
  digitalWrite(GREEN_LED,  g);
  digitalWrite(YELLOW_LED, y);
  digitalWrite(RED_LED,    r);
}

void allLedsOn() {
  digitalWrite(GREEN_LED,  HIGH);
  digitalWrite(YELLOW_LED, HIGH);
  digitalWrite(RED_LED,    HIGH);
}

void allLedsOff() {
  digitalWrite(GREEN_LED,  LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED,    LOW);
}

void beepBuzzer() {
  // Three short beeps for critical alert
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
  }
}
