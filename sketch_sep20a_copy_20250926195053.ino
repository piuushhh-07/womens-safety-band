#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// GSM module on pins 7 (RX), 8 (TX)
SoftwareSerial gsm(7, 8);

// GPS module on hardware Serial (pins 0,1)
TinyGPSPlus gps;

// Pins
const int buzzer = 9;
const int button = 2;

float lat = 0.0;
float lon = 0.0;
bool buttonPressed = false;

void setup() {
  pinMode(buzzer, OUTPUT);
  pinMode(button, INPUT_PULLUP);

  Serial.begin(9600); // GPS
  gsm.begin(9600);    // GSM

  Serial.println("System Starting...");
}

void loop() {
  // Read GPS data
  while (Serial.available() > 0) {
    gps.encode(Serial.read());
    if (gps.location.isUpdated()) {
      lat = gps.location.lat();
      lon = gps.location.lng();
      Serial.print("GPS Updated: ");
      Serial.print(lat, 6);
      Serial.print(", ");
      Serial.println(lon, 6);
    }
  }

  // Check SOS button
  if (digitalRead(button) == LOW) {
    delay(50); // debounce
    if (digitalRead(button) == LOW) {
      buttonPressed = true;
      digitalWrite(buzzer, HIGH);
      Serial.println("!! EMERGENCY BUTTON PRESSED !!");
      delay(1000);
      digitalWrite(buzzer, LOW);
    }
  }

  // If button pressed, try sending SMS until success
  if (buttonPressed) {
    bool sent = false;
    int attempts = 0;

    while (!sent && attempts < 5) {   // retry up to 5 times
      if (gsmRegistered()) {
        sent = sendSMS();
      } else {
        Serial.println("GSM not registered. Retrying...");
      }
      attempts++;
      delay(5000); // wait 5s before retry
    }

    if (sent) {
      Serial.println("✅ SMS Sent Successfully!");
    } else {
      Serial.println("❌ Failed to send SMS after 5 attempts.");
    }

    buttonPressed = false;
  }
}

// Check GSM network registration
bool gsmRegistered() {
  gsm.println("AT+CREG?");
  delay(1000);
  while (gsm.available()) {
    String response = gsm.readString();
    Serial.println("GSM Response: " + response);
    if (response.indexOf(",1") != -1 || response.indexOf(",5") != -1) {
      return true;  // Registered
    }
  }
  return false;
}

// Send SMS with location
bool sendSMS() {
  gsm.println("AT+CMGF=1");  
  delay(1000);

  gsm.println("AT+CMGS=\"+91xxxxxxxxx47\""); // your phone number
  delay(1000);

  gsm.print("🚨 Emergency! I need help.\nMy location:\n");
  gsm.print("https://maps.google.com/?q=");
  gsm.print(lat, 6);
  gsm.print(",");
  gsm.print(lon, 6);

  gsm.write(26); // CTRL+Z
  delay(5000);

  // Check for confirmation
  if (gsm.available()) {
    String response = gsm.readString();
    Serial.println("SMS Response: " + response);
    if (response.indexOf("OK") != -1) {
      return true;  // SMS sent
    }
  }
  return false; // failed
}