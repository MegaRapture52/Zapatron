#include <Arduino.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include "MPU6050.h"
#include <ArduinoJson.h>

BluetoothSerial SerialBT;
MPU6050 mpu;
//Izaiah is so cool :)
// --- LED ---
const int LED_PIN = 2;   // Built-in blue LED

// --- Settings ---
float pitchThreshold = -50;       // below = trigger ***FOR TESTING SET TO -20!! 
unsigned long holdTime = 3000;    // SHOULD BE ~20 SECS, ***FOR TESTING SET TO 3000 (3 secs)!!
                      // ^^1000 = 1 second
unsigned long belowStart = 0;
bool belowActive = false;
bool wasBelow = false;            // Track previous state

void setup() {
  Serial.begin(115200);

  // LED setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // LED OFF until connected

  // I2C pins
  Wire.begin(32, 33);  // SDA = 32, SCL = 33
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }
  Serial.println("MPU6050 Connected.");

  // Bluetooth
  SerialBT.begin("ESP32_Master", true);
  Serial.println("Master Bluetooth started.");

  while (!SerialBT.connect("ESP32_Slave")) {
    Serial.println("Trying to connect to slave...");
    delay(1000);
  }

  Serial.println("Connected to ESP32_Slave!");
  digitalWrite(LED_PIN, HIGH); // LED ON when connected to master
}

void loop() {
  // Read MPU6050
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
  Serial.print("Pitch: ");
  Serial.println(pitch);

  unsigned long now = millis();

  // BELOW threshold logic
  if (pitch < pitchThreshold) {
    if (!belowActive) {
      belowActive = true;
      belowStart = now;
    }
    if (belowActive && now - belowStart >= holdTime) {
      StaticJsonDocument<200> doc;
      doc["below"] = true;
      doc["above"] = false;
      doc["boot"] = false;
      doc["pitch"] = pitch;

      String jsonStr;
      serializeJson(doc, jsonStr);
      SerialBT.println(jsonStr);
      Serial.println(">>> BELOW sent <<<");

      wasBelow = true;
    }
  } else {
    StaticJsonDocument<200> doc;
    doc["below"] = false;
    doc["above"] = true;
    doc["boot"] = false;
    doc["pitch"] = pitch;

    String jsonStr;
    serializeJson(doc, jsonStr);
    SerialBT.println(jsonStr);
    Serial.println(">>> ABOVE sent <<<");

    if (wasBelow) {
      StaticJsonDocument<200> bootDoc;
      bootDoc["below"] = false;
      bootDoc["above"] = false;
      bootDoc["boot"] = true;

      String bootStr;
      serializeJson(bootDoc, bootStr);
      SerialBT.println(bootStr);
      Serial.println(">>> BOOT sent <<<");

      wasBelow = false;
    }

    belowActive = false;
  }

  delay(50);
}