#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

// Keypad setup
const byte ROWS = 4;  // 4 rows
const byte COLS = 4;  // 4 columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};  // Connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9};  // Connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Relay pin
const int relayPin = A0;  // Pin connected to the relay's IN1
const int lightPin = A1;  // Pin connected to the light's IN1

// OLED display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Predefined codes for turning the lamp on and off
String turnOnCode = "1234";  // Code to turn the lamp on
String turnOffCode = "4321"; // Code to turn the lamp off

String inputCode = "";  // Stores the input from the keypad

void setup() {
  pinMode(relayPin, OUTPUT);  // Set relay pin as output
  pinMode(lightPin, OUTPUT);  // Set relay pin as output
  digitalWrite(relayPin, LOW); // Start with the relay off (lamp off)
  digitalWrite(lightPin, LOW); // Start with the relay off (lamp off)

  Serial.begin(115200);  // Initialize serial communication (optional for debugging)

  // Initialize the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // I2C address for SSD1306
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Lamp Control");
  display.display();
  delay(2000); // Display for 2 seconds
}

void loop() {
    
   if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Read the command from ESP32
    command.trim();  // Remove any newline characters

    if (command == "L1") {
      digitalWrite(relayPin, HIGH);  // Turn on the relay (and the lamp)
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lamp 1");
      display.display();
    } else if (command == "L2") {
      digitalWrite(relayPin, LOW);  // Turn off the relay (and the lamp)
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lamp 2");
      display.display();
    }
    else if (command == "On") {
      digitalWrite(lightPin, HIGH);  // Turn on the second lamp
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lamp On");
      display.display();
    }
    else if (command == "Off") {
      digitalWrite(lightPin, LOW);  // Turn off the second lamp
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lamp Off");
      display.display();
    }
  }
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);  // Print the pressed key to the Serial Monitor (for debugging)

    if (key == '#') {
      // When '#' is pressed, the code is submitted
      display.clearDisplay();
      display.setCursor(0, 0);
      if (inputCode == turnOnCode) {
        analogWrite(lightPin, 255);  // Turn the relay (and the lamp) on (HIGH)
        display.println("Lamp On");
        Serial.println("Lamp On");
      } else if (inputCode == turnOffCode) {
        analogWrite(lightPin, 0);  // Turn the relay (and the lamp) off (LOW)
        display.println("Lamp Off");
        Serial.println("Lamp Off");
      } else {
        display.println("Invalid Code");
        Serial.println("Invalid Code");
      }
      display.display(); // Update the OLED display
      inputCode = "";  // Clear the input after submission
      delay(2000); // Display the result for 2 seconds
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lamp Control");
      display.display();
    } else if (key == '*') {
      // '*' is used to clear the current input
      inputCode = "";
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Input Cleared");
      display.display();
      delay(1000); // Show the cleared message for 1 second
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lamp Control");
      display.display();
    } else if (key == 'A'){
      analogWrite(relayPin, 255);
            display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lamp 1");
      display.display();
    }
     else if (key == 'B'){
      analogWrite(relayPin, 0);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lamp 2");
      display.display();
    }
        
    else {
      // Append the key to the input code
      inputCode += key;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Entering Code:");
      display.setCursor(0, 10);
      display.print(inputCode); // Show the current input
      display.display();
    }
  }
}