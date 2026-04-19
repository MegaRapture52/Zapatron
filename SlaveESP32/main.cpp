#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

BluetoothSerial SerialBT;
//omg wow Izaiah is so cool :)
// --- LED ---
const int LED_PIN = 2;   // Built-in blue LED

// Servos
Servo servo1;
Servo servo2;
int servo1Pin = 26;
int servo2Pin = 27;

// LOCATIONS
int restS1 = 140; //OG IS 140
int goS1 = 160; //OG-170

int restS2 = 30; //OG- 30
int goS2 = 5; //OG- 0

// State
bool belowActive = false;
bool bootActive = false;
unsigned long lastToggle = 0;
int servo1Pos = restS1;
bool connectionLostHandled = true;

void setup() {
  Serial.begin(115200);

  // LED setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // OFF until connected

  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);

  servo1.write(restS1);
  servo2.write(restS2);

  SerialBT.begin("ESP32_Slave");
  Serial.println("Slave ready. Waiting for master...");

  // Servos go to REST if no connection
  while (!SerialBT.connected()) {
    servo1.write(restS1);
    servo2.write(restS2);
    Serial.println("Waiting for master...");
  }

  Serial.println("Master connected!");
}

void loop() {
  // Check BLE connection
  if (!SerialBT.connected()) {

    if (!connectionLostHandled) { 
      digitalWrite(LED_PIN, LOW); //LED OFF on disconnect
      servo1.write(restS1);
      servo2.write(restS2);
      delay(100);
      servo2.write(goS2);
      delay(1600);
      servo2.write(restS2);
      connectionLostHandled = true; 
    }
    return;
  }
  digitalWrite(LED_PIN, HIGH); // <<< ON RECONNECT BTW
  // --- CONNECTION / RECONNECTION SEQUENCE (and handling) ---
  if (connectionLostHandled) {
    belowActive = false;
    bootActive = false;
    servo1Pos = restS1; 

    servo1.write(restS1);
    servo2.write(restS2);
    delay(100);
    servo2.write(goS2);
    delay(1600);
    servo2.write(restS2);

    connectionLostHandled = false; 
  }

  // Read BLE JSON
  if (SerialBT.available()) {
    String jsonStr = SerialBT.readStringUntil('\n');
    Serial.print("RX: "); Serial.println(jsonStr);

    StaticJsonDocument<200> doc;
    if (deserializeJson(doc, jsonStr)) return;

    belowActive = doc["below"] | false;
    bootActive  = doc["boot"]  | false;
  }

  unsigned long now = millis();

  // Servo1 loop if below active
  if (belowActive && now - lastToggle >= 10) { //SPEED
    lastToggle = now;
    servo1.write(servo1Pos);
    servo1Pos += 1; //DEGREES PER
    if (servo1Pos > goS1) servo1Pos = restS1;
  }

  // "Boot" sequence for servo2
  if (bootActive) {
    Serial.println(">>> BOOT SEQUENCE START <<<");
    for (int i = 0; i < 2; i++) {
      servo1.write(restS1);
      servo2.write(restS2);
      delay(100);
      servo2.write(goS2);
      delay(1600);
      servo2.write(restS2);
    }
    bootActive = false;
    Serial.println(">>> BOOT SEQUENCE END <<<");
  }
}