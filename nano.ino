// Arduino Nano Railway Crossing Control System
// Works with ESP32 via serial communication
// Includes 16x2 I2C LCD Display
// FIXED: Normal buzzer logic (HIGH = ON, LOW = OFF)

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize LCD (address 0x27, 16 columns, 2 rows)
// Common I2C addresses: 0x27, 0x3F, 0x20, 0x38
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin definitions
#define SERVO_PIN 9           // Servo motor for gate control
#define IR_SENSOR_1 2         // First IR sensor (approach detection)
#define IR_SENSOR_2 3         // Second IR sensor (exit detection)
#define MANUAL_OPEN_BTN 4     // Manual open button
#define MANUAL_CLOSE_BTN 5    // Manual close button
#define AUTO_MODE_BTN 6       // Auto mode button
#define STATUS_LED 13         // Built-in LED for status
#define BUZZER_PIN 8          // Buzzer for alerts (NORMAL LOGIC - HIGH = ON)
#define RED_LED 10            // Red LED for gate closed
#define GREEN_LED 11          // Green LED for gate open

// Gate positions
#define GATE_OPEN_POS 90      // Servo position for open gate
#define GATE_CLOSED_POS 0     // Servo position for closed gate

// System states
enum GateState {
  GATE_OPEN = 0,
  GATE_CLOSED = 1,
  GATE_OPENING = 2,
  GATE_CLOSING = 3
};

// Global variables
Servo gateServo;
GateState currentGateState = GATE_CLOSED;
bool trainDetected = false;
bool manualMode = false;
bool trainApproaching = false;
bool trainExiting = false;

// Timing variables
unsigned long lastStatusSend = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastSensorCheck = 0;
unsigned long gateOperationStart = 0;
unsigned long lastBuzzerUpdate = 0;
unsigned long statusLEDTimer = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long trainClearTime = 0;

// Button debouncing
bool lastOpenBtnState = HIGH;
bool lastCloseBtnState = HIGH;
bool lastAutoBtnState = HIGH;

// Custom LCD characters
byte trainChar[8] = {
  0b00000,
  0b00000,
  0b01110,
  0b11111,
  0b10101,
  0b11111,
  0b01010,
  0b00000
};

byte gateClosedChar[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};

byte gateOpenChar[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

void setup() {
  Serial.begin(9600);
  
  // Initialize I2C LCD
  lcd.init();
  lcd.backlight();
  
  // Create custom characters
  lcd.createChar(0, trainChar);
  lcd.createChar(1, gateClosedChar);
  lcd.createChar(2, gateOpenChar);
  
  // Display startup message
  lcd.setCursor(0, 0);
  lcd.print("Railway Crossing");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  
  // Initialize servo
  gateServo.attach(SERVO_PIN);
  gateServo.write(GATE_CLOSED_POS);
  currentGateState = GATE_CLOSED;
  
  // Initialize pins
  pinMode(IR_SENSOR_1, INPUT_PULLUP);
  pinMode(IR_SENSOR_2, INPUT_PULLUP);
  pinMode(MANUAL_OPEN_BTN, INPUT_PULLUP);
  pinMode(MANUAL_CLOSE_BTN, INPUT_PULLUP);
  pinMode(AUTO_MODE_BTN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  
  // Initial LED states
  digitalWrite(RED_LED, HIGH);    // Gate is closed initially
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(STATUS_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);  // Buzzer OFF initially
  
  // Startup sequence
  Serial.println("Railway Crossing System Initialized");
  
  // Blink LEDs to indicate startup
  for(int i = 0; i < 3; i++) {
    digitalWrite(STATUS_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);  // Buzzer ON during startup
    delay(200);
    digitalWrite(STATUS_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);   // Buzzer OFF
    delay(200);
  }
  
  sendStatusUpdate();
  Serial.println("System Ready");
  
  // Display ready message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  delay(1000);
  
  // Initial display update
  updateDisplay();
}

void loop() {
  handleSerialCommands();
  checkButtons();
  checkSensors();
  updateGateOperation();
  updateStatusLED();
  updateBuzzer();
  
  // Update display every 500ms
  if (millis() - lastDisplayUpdate > 500) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  
  // Send status updates every 2 seconds
  if (millis() - lastStatusSend > 2000) {
    sendStatusUpdate();
    lastStatusSend = millis();
  }
  
  delay(10);
}

void updateDisplay() {
  lcd.clear();
  
  // Line 1: Gate status and mode
  lcd.setCursor(0, 0);
  
  // Display gate status with custom character
  if (currentGateState == GATE_OPEN) {
    lcd.write(2); // Open gate character
    lcd.print("OPEN");
  } else if (currentGateState == GATE_CLOSED) {
    lcd.write(1); // Closed gate character
    lcd.print("CLOSED");
  } else if (currentGateState == GATE_OPENING) {
    lcd.print("OPENING");
  } else if (currentGateState == GATE_CLOSING) {
    lcd.print("CLOSING");
  }
  
  // Display mode
  lcd.setCursor(9, 0);
  if (manualMode) {
    lcd.print("MANUAL");
  } else {
    lcd.print("AUTO");
  }
  
  // Line 2: Train status and sensor information
  lcd.setCursor(0, 1);
  
  if (trainDetected) {
    lcd.write(0); // Train character
    lcd.print("TRAIN:");
    
    if (trainApproaching && !trainExiting) {
      lcd.print("APPR");
    } else if (trainExiting && !trainApproaching) {
      lcd.print("EXIT");
    } else if (trainApproaching && trainExiting) {
      lcd.print("BOTH");
    } else {
      lcd.print("DETECT");
    }
  } else {
    lcd.print("NO TRAIN");
    
    // Show sensor status when no train
    lcd.setCursor(10, 1);
    lcd.print("S1:");
    lcd.print(digitalRead(IR_SENSOR_1) == LOW ? "1" : "0");
    lcd.print(" S2:");
    lcd.print(digitalRead(IR_SENSOR_2) == LOW ? "1" : "0");
  }
}

void displayMessage(String line1, String line2, int duration = 1000) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  if (line2.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
  delay(duration);
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "STATUS") {
      sendStatusUpdate();
    }
    else if (command == "MANUAL_OPEN") {
      manualMode = true;
      openGate();
      displayMessage("MANUAL MODE", "Opening Gate", 1000);
      Serial.println("MANUAL_ACTIVE");
    }
    else if (command == "MANUAL_CLOSE") {
      manualMode = true;
      closeGate();
      displayMessage("MANUAL MODE", "Closing Gate", 1000);
      Serial.println("MANUAL_ACTIVE");
    }
    else if (command == "AUTO_MODE") {
      manualMode = false;
      displayMessage("AUTO MODE", "Activated", 1000);
      Serial.println("AUTO_ACTIVE");
      // In auto mode, check sensors and act accordingly
      if (trainDetected && currentGateState != GATE_CLOSED && currentGateState != GATE_CLOSING) {
        closeGate();
      } else if (!trainDetected && currentGateState != GATE_OPEN && currentGateState != GATE_OPENING) {
        openGate();
      }
    }
    else if (command.startsWith("ERROR:")) {
      Serial.println("ERROR:UNKNOWN_COMMAND");
    }
  }
}

void checkButtons() {
  if (millis() - lastButtonCheck > 50) { // Debounce delay
    
    // Manual Open Button
    bool openBtnState = digitalRead(MANUAL_OPEN_BTN);
    if (openBtnState == LOW && lastOpenBtnState == HIGH) {
      manualMode = true;
      openGate();
      displayMessage("MANUAL MODE", "Button: Open", 1000);
      Serial.println("MANUAL_ACTIVE");
    }
    lastOpenBtnState = openBtnState;
    
    // Manual Close Button
    bool closeBtnState = digitalRead(MANUAL_CLOSE_BTN);
    if (closeBtnState == LOW && lastCloseBtnState == HIGH) {
      manualMode = true;
      closeGate();
      displayMessage("MANUAL MODE", "Button: Close", 1000);
      Serial.println("MANUAL_ACTIVE");
    }
    lastCloseBtnState = closeBtnState;
    
    // Auto Mode Button
    bool autoBtnState = digitalRead(AUTO_MODE_BTN);
    if (autoBtnState == LOW && lastAutoBtnState == HIGH) {
      manualMode = false;
      displayMessage("AUTO MODE", "Button Activated", 1000);
      Serial.println("AUTO_ACTIVE");
    }
    lastAutoBtnState = autoBtnState;
    
    lastButtonCheck = millis();
  }
}

void checkSensors() {
  if (millis() - lastSensorCheck > 100) { // Check sensors every 100ms
    
    bool sensor1Active = (digitalRead(IR_SENSOR_1) == LOW); // Active LOW
    bool sensor2Active = (digitalRead(IR_SENSOR_2) == LOW); // Active LOW
    
    // Train detection logic
    bool previousTrainDetected = trainDetected;
    trainDetected = sensor1Active || sensor2Active;
    
    // Show alert when train is first detected
    if (!previousTrainDetected && trainDetected) {
      displayMessage("ALERT!", "Train Detected", 1500);
    }
    
    // Determine train direction and state
    if (sensor1Active && !sensor2Active) {
      trainApproaching = true;
      trainExiting = false;
    } else if (!sensor1Active && sensor2Active) {
      trainApproaching = false;
      trainExiting = true;
    } else if (!sensor1Active && !sensor2Active) {
      trainApproaching = false;
      trainExiting = false;
    }
    
    // Auto mode gate control
    if (!manualMode) {
      if (trainDetected && (currentGateState == GATE_OPEN || currentGateState == GATE_OPENING)) {
        closeGate();
      } else if (!trainDetected && (currentGateState == GATE_CLOSED || currentGateState == GATE_CLOSING)) {
        // Wait a bit before opening to ensure train has completely passed
        if (previousTrainDetected && !trainDetected) {
          trainClearTime = millis();
          displayMessage("TRAIN CLEARED", "Opening in 3s", 1500);
        }
        if (millis() - trainClearTime > 3000) { // 3 second delay
          openGate();
        }
      }
    }
    
    lastSensorCheck = millis();
  }
}

void updateGateOperation() {
  static int targetPosition = GATE_CLOSED_POS;
  
  if (currentGateState == GATE_OPENING) {
    targetPosition = GATE_OPEN_POS;
    if (millis() - gateOperationStart > 2000) { // 2 seconds to open
      currentGateState = GATE_OPEN;
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      displayMessage("GATE OPENED", "Safe to Cross", 1500);
      Serial.println("GATE_OPENED");
    }
  }
  else if (currentGateState == GATE_CLOSING) {
    targetPosition = GATE_CLOSED_POS;
    if (millis() - gateOperationStart > 2000) { // 2 seconds to close
      currentGateState = GATE_CLOSED;
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      displayMessage("GATE CLOSED", "Do Not Cross!", 1500);
      Serial.println("GATE_CLOSED");
    }
  }
  
  // Smooth servo movement
  int currentPosition = gateServo.read();
  if (currentPosition != targetPosition) {
    if (currentPosition < targetPosition) {
      gateServo.write(min(currentPosition + 2, targetPosition));
    } else {
      gateServo.write(max(currentPosition - 2, targetPosition));
    }
  }
}

void updateStatusLED() {
  if (millis() - statusLEDTimer > 100) {
    if (trainDetected) {
      // Fast blink when train detected
      digitalWrite(STATUS_LED, (millis() / 200) % 2);
    } else if (manualMode) {
      // Slow blink in manual mode
      digitalWrite(STATUS_LED, (millis() / 1000) % 2);
    } else {
      // Solid on in auto mode
      digitalWrite(STATUS_LED, HIGH);
    }
    statusLEDTimer = millis();
  }
}

// FIXED: Normal buzzer logic - HIGH = ON, LOW = OFF
void updateBuzzer() {
  if (millis() - lastBuzzerUpdate > 250) { // Update every 250ms for better timing
    
    if (trainDetected && (currentGateState == GATE_CLOSING || currentGateState == GATE_CLOSED)) {
      // Intermittent beep when train is detected and gate is closing/closed
      // Pattern: 250ms ON, 250ms OFF
      digitalWrite(BUZZER_PIN, (millis() / 250) % 2 == 0 ? HIGH : LOW);
    } 
    else if (currentGateState == GATE_OPENING || currentGateState == GATE_CLOSING) {
      // Continuous beep during gate operation
      digitalWrite(BUZZER_PIN, HIGH);
    } 
    else {
      // Buzzer OFF in all other cases
      digitalWrite(BUZZER_PIN, LOW);
    }
    
    lastBuzzerUpdate = millis();
  }
}

void openGate() {
  if (currentGateState != GATE_OPEN && currentGateState != GATE_OPENING) {
    currentGateState = GATE_OPENING;
    gateOperationStart = millis();
    Serial.println("Gate opening...");
  }
}

void closeGate() {
  if (currentGateState != GATE_CLOSED && currentGateState != GATE_CLOSING) {
    currentGateState = GATE_CLOSING;
    gateOperationStart = millis();
    Serial.println("Gate closing...");
  }
}

void sendStatusUpdate() {
  String status = "STATE:";
  status += String((int)currentGateState);
  status += ",TRAIN:";
  status += String(trainDetected ? 1 : 0);
  status += ",MANUAL:";
  status += String(manualMode ? 1 : 0);
  
  Serial.println(status);
}
