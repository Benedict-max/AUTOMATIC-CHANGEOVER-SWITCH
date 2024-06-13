#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Project Title
const char* projectTitle = "A.C.S";

// Define the I2C address for the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Define pins for sensors and components
const int mainsSwitchPin = 4;   // Digital pin for mains switch input (change as needed)
const int fuelTrigPin = 9;      // Ultrasonic fuel sensor trigger pin
const int fuelEchoPin = 10;     // Ultrasonic fuel sensor echo pin
const int oilTrigPin = 11;      // Ultrasonic oil sensor trigger pin
const int oilEchoPin = 12;      // Ultrasonic oil sensor echo pin
const int tempPin = A0;         // LM35 temperature sensor analog pin
const int relayPin = 7;         // Relay control pin for changeover
const int buzzerPin = 6;        // Buzzer control pin

// GSM Module pins (SoftwareSerial)
SoftwareSerial gsm(2, 3); // RX, TX

// Function to read distance from ultrasonic sensor
long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.034 / 2; // Convert duration to distance (cm)

  return distance;
}

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  gsm.begin(9600);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print(projectTitle);

  // Initialize pins
  pinMode(mainsSwitchPin, INPUT_PULLUP); // Use internal pull-up resistor
  pinMode(fuelTrigPin, OUTPUT);
  pinMode(fuelEchoPin, INPUT);
  pinMode(oilTrigPin, OUTPUT);
  pinMode(oilEchoPin, INPUT);
  pinMode(tempPin, INPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Set relay to default position (mains power initially assumed available)
  digitalWrite(relayPin, LOW);

  delay(2000);
  lcd.clear();
}

void loop() {
  // Read the state of the mains switch
  bool mainsAvailable = digitalRead(mainsSwitchPin) == LOW; // LOW means mains power available

  // Read fuel level
  long fuelLevel = readUltrasonic(fuelTrigPin, fuelEchoPin);

  // Read oil level
  long oilLevel = readUltrasonic(oilTrigPin, oilEchoPin);

  // Read temperature
  int tempValue = analogRead(tempPin);
  float temperature = (tempValue * 5.0 * 100.0) / 1024.0;

  // Display values on LCD
  lcd.setCursor(0, 0);
  lcd.print("Fuel:");
  lcd.print(fuelLevel);
  lcd.print("cm");

  lcd.setCursor(0, 1);
  lcd.print("Oil:");
  lcd.print(oilLevel);
  lcd.print("cm");

  // Check if mains power is available
  if (mainsAvailable) {
    // Mains power is available
    digitalWrite(relayPin, LOW); // Switch to mains
    lcd.setCursor(12, 0);
    lcd.print("Mains");
  } else {
    // Mains power is not available, check generator conditions
    lcd.setCursor(12, 0);
    lcd.print("Gen. ");

    if (fuelLevel > 200 && oilLevel > 100 && temperature < 40) {
      // Conditions are okay, switch to generator
      digitalWrite(relayPin, HIGH); // Switch to generator
    } else {
      // Trigger buzzer and keep relay to mains position (if available)
      digitalWrite(buzzerPin, HIGH);
      if (gsm.available() == 0) {
        gsm.println("AT+CMGF=1");  // Set SMS mode to text
        delay(100);
        gsm.println("AT+CMGS=\"+1234567890\"");  // Replace with your number
        delay(100);
        gsm.print("Warning: Check fuel, oil or temp!");
        delay(100);
        gsm.write(26);  // ASCII code for CTRL+Z to send SMS
      }
    }
  }

  // Reset buzzer if conditions are met
  if (fuelLevel > 200 && oilLevel > 100 && temperature < 40) {
    digitalWrite(buzzerPin, LOW);
  }

  // Wait before the next loop iteration
  delay(2000);
}
