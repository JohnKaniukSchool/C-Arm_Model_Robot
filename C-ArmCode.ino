#include <Servo.h>


// Development Board Pins

const int SWDIP1 =  18;        //INTERRUPT
const int SWDIP2 =  19;        //INTERRUPT
const int SWDIP3 =  20;        //INTERRUPT
const int SWDIP4 =  21;        //INTERRUPT

const int PUSHBUTTON1 =  13;   //INTERRUPT
const int PUSHBUTTON2 =  12;   //INTERUPT

const int POT1PIN =  A0;     //ANALOG
const int POT2PIN =  A1;     //ANALOG

const int CLK1 = 52;
const int DO1  = 50;
const int DIN1 = 48;
const int CS1  = 46;

const int CLK2 = 53;
const int DO2  = 51;
const int DIN2 = 49;
const int CS2  = 47;

const int CH3 =  A2;   //ANALOG
const int CH4 =  A3;   //ANALOG

const int DIR =  2;
const int STP =  9;
const int SP  = 
const int RS  =  
const int M2  =  5;
const int M1  =  4;
const int M0  =  3;
const int EN  =  43;


const int LEDGPIO1  = 38;
const int LEDGPIO2  = 39;
const int LEDGPIO3  = 40;
const int LEDGPIO4  = 41;
const int LEDGPIO5  = 42;


// Pin Definitions
const int stepPin = 9;
const int dirPin = 2;
const int limitSwitch1Pin = 12;
const int limitSwitch2Pin = 13;
const int M0Pin = 3;
const int M1Pin = 4;
const int M2Pin = 5;
const int stepDelay = 800; // Microseconds
const int servoPin1 = 6;
const int servoPin2 = 7;
const int potPin = A0;
const int buttonUp = 31;
const int buttonDown = 33;
const int motorPin1 = 32;
const int motorPin2 = 34;
const int encoderPin = 36;
const int ledPins[4] = {13, 14, 15, 16};
const unsigned long interval = 3000;

// Variables
volatile int mode = 0;
long currentPosition = 0;
long calibratedPosition2 = 0;
long limit1Position = 0;
long limit2Position = 0;
long midPosition = 0;
long range = 0;
int potCalState = 0;

bool PotCalBool = true;

unsigned long previousMillis = 0;

float lowerDev = 50.0;
float higherDev = 50.0;


Servo servo1;
Servo servo2;

// State machine modes
enum Modes {
  MODE_C_ARM,
  MODE_SERVO_X,
  MODE_SERVO_Y,
  MODE_GEAR_MOTOR
};

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(limitSwitch1Pin, INPUT_PULLUP);
  pinMode(limitSwitch2Pin, INPUT_PULLUP);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(encoderPin, INPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  servo1.attach(servoPin1);
  servo2.attach(servoPin2);

  attachInterrupt(digitalPinToInterrupt(buttonUp), increaseMode, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonDown), decreaseMode, FALLING);
  attachInterrupt()

  Serial.begin(9600);

  HomeStepperMotor();
  //calibratePotentiometer();
  simplePotCalibration();
}

void loop() {
  updateLEDs();
  Serial.print("Current Mode: ");
  Serial.println(mode);

  switch (mode) {
    case MODE_C_ARM:
      controlCArm();
      break;
    case MODE_SERVO_X:
      controlServo(servo1);
      break;
    case MODE_SERVO_Y:
      controlServo(servo2);
      break;
    case MODE_GEAR_MOTOR:
      controlGearMotor();
      break;
  }
}

void updateLEDs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], (i == mode) ? HIGH : LOW);
  }
}

void controlCArm() {
  int stepCycleTimerStart = millis();
  int potValue = analogRead(potPin);

  long targetPosition = potValueToSteps(potValue, 0, 1023, 0, calibratedPosition2);
  moveToPosition(targetPosition);

  Serial.print("C-Arm | Pot: ");
  Serial.print(potValue);
  Serial.print(" | Target: ");
  Serial.print(targetPosition);
  Serial.print(" | Current: ");
  Serial.println(currentPosition);

  int stepCycleTimerEnd = millis();
  int cycleTime = (stepCycleTimerEnd - stepCycleTimerStart);
  Serial.println(cycleTime);

  float lowerDev = (cycleTime <= lowerDev) ? cycleTime : lowerDev;
  float higherDev = (cycleTime <= higherDev) ? cycleTime : higherDev;

  Serial.print("LowerDeviation: ");
  Serial.print(lowerDev);

  Serial.print("Higher Deviation: ");
  Serial.print(higherDev);


}

void controlServo(Servo &servo) {
  int potValue = analogRead(potPin);
  int angle = map(potValue, 0, 1023, 0, 180);
  servo.write(angle);
  Serial.print("Servo Control | Pot: ");
  Serial.print(potValue);
  Serial.print(" | Angle: ");
  Serial.println(angle);
}

void controlGearMotor() {
  int encoderCount = digitalRead(encoderPin);
  if (digitalRead(buttonUp) == LOW) {
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
  } else if (digitalRead(buttonDown) == LOW) {
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, HIGH);
  } else {
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
  }
  Serial.print("Gear Motor | Encoder Count: ");
  Serial.println(encoderCount);
}

void increaseMode() {
  mode = (mode + 1) % 4;
}

void decreaseMode() {
  mode = (mode - 1 + 4) % 4;
}


// Library of Functions

void HomeStepperMotor() {
  Serial.println("Starting Calibration");

  // Home to Limit Switch 1
  digitalWrite(dirPin, HIGH); // Assuming HIGH moves towards limitSwitch1
  while (digitalRead(limitSwitch1Pin) == HIGH) {
    stepMotor();
  }

  // Set zero reference
  currentPosition = 0;
  Serial.println("Limit Switch 1 reached. Zeroing position.");

  // Move 370 steps in the opposite direction
  digitalWrite(dirPin, LOW);
  for (int i = 0; i < 330; i++) {
    stepMotor();
    currentPosition++;
  }

  // Store calibrated position
  calibratedPosition2 = currentPosition; // Should be 370
  Serial.print("Calibrated Position 2: ");
  Serial.println(calibratedPosition2);

  // Calculate the midpoint
  midPosition = calibratedPosition2 / 2; // Should be 185
  Serial.print("Mid Position: ");
  Serial.println(midPosition);

  // Move to Midpoint
  moveToPosition(midPosition);
  Serial.println("Calibration Complete. At Midpoint.");
}



void stepMotor() {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(800); // Adjust step delay for motor speed
  digitalWrite(stepPin, LOW);
  delayMicroseconds(800);
}

void calibratePotentiometer() {
  range = calibratedPosition2 / 25;

  long potCalLowStep = midPosition - (range / 2);
  long potCalHighStep = midPosition + (range / 2);

  long potCalLow = stepsToPotValue(potCalLowStep , 0, 1023, 0 , calibratedPosition2);
  long potCalHigh = stepsToPotValue(potCalHighStep , 0, 1023, 0 , calibratedPosition2);

  Serial.println("Entering Calibration Mode...");

  unsigned long startTime = 0; // To track how long the pot stays in range

  while (PotCalBool) { // Stay in this loop until calibration is done
    Serial.print("Loop Running - Current State: ");  
    Serial.println(potCalState);

    int potValue = analogRead(potPin);

    switch (potCalState) {
      case 0:  // Waiting for pot to be in range
        Serial.print("State 0 - Waiting | Pot Value: ");
        Serial.println(potValue);

        if (potValue >= potCalLow && potValue <= potCalHigh) {
          Serial.println("Potentiometer in range! Starting 3-second timer.");
          startTime = millis();  // Start tracking time
          potCalState = 1;
        }
        break;

      case 1:  // Checking if pot stays in range for 3 seconds
        Serial.print("State 1 - Calibrating | Pot Value: ");
        Serial.println(potValue);

        if (potValue < potCalLow || potValue > potCalHigh) {
          Serial.println("Potentiometer out of range. Returning to State 0.");
          potCalState = 0;
        } else if (millis() - startTime >= 3000) {  // 3 seconds have passed
          Serial.println("Potentiometer stayed in range for 3 seconds. Proceeding to stepper movement.");
          potCalState = 2;
        }
        break;

      case 2:  // Match Stepper to Potentiometer
        Serial.print("State 2 - Matching Stepper | Pot Value: ");
        Serial.println(potValue);

        long targetPosition = potValueToSteps(potValue, 0, 1023, 0, calibratedPosition2);
        moveToPosition(targetPosition);
        
        Serial.println("Stepper Matched. Calibration Complete.");
        potCalState = 3;
        break;

      case 3:  // Done
        Serial.println("State 3 - Calibration Complete.");
        PotCalBool = false;
        return; // Exit function
    }

    delay(50); // Prevents excessive polling
  }
}



void calibratePotentiometer1() {
  range = calibratedPosition2 / 40;
  
  long potCalLow = midPosition - (range / 2);
  long potCalHigh = midPosition + (range / 2);
  Serial.println("Entering Calibration Mode...");

  
  while (PotCalBool) { // Stay in this loop until calibration is done
    Serial.print("Loop Running - Current State: ");  
    Serial.println(potCalState);

    switch (potCalState) {
      case 0:  // Waiting for pot to be in range
        
        int potValue = analogRead(potPin);
        Serial.print("State 0 - Waiting | Pot Value: ");
        Serial.println(potValue);
        Serial.println("Red LED ON");

        Serial.println(midPosition);
        Serial.println("Midposition");

        Serial.println("Red LED ON");
        Serial.println(range);
        


        if (potValue >= potCalLow && potValue <= potCalHigh) {
          Serial.println("Potentiometer in range! Moving to calibration.");
          
          potCalState = 1;

          Serial.print("State Changed to: ");
          Serial.println(potCalState);
          
          
        }
        break;

      case 1:  // Calibrating
        
        potValue = analogRead(potPin);
        Serial.print("State 1 - Calibrating | Pot Value: ");
        Serial.println(potValue);

        if (potValue < potCalLow || potValue > potCalHigh) {
          Serial.println("Potentiometer out of range. Returning to State 0.");
          potCalState = 0;
        } else {
          if (millis() - previousMillis >= interval) {
            Serial.println("Potentiometer stayed in range for 3 seconds.");
            Serial.println("Green LED FLASHING");
            potCalState = 2;
          } else {
            Serial.println("Potentiometer still in range. Waiting for 3 seconds...");
          }
        }
        break;

      case 2:  // Match Stepper
        potValue = analogRead(potPin);
        Serial.print("State 2 - Matching Stepper | Pot Value: ");
        Serial.println(potValue);
        Serial.println("Stepper Moving to Match Potentiometer.");
        
        long targetPosition = potValueToSteps(potValue,  0, 1023, 0, calibratedPosition2); 
        moveToPosition(targetPosition);
        
        // Simulate stepper movement
        Serial.println("Stepper Matched. Calibration Complete.");
        potCalState = 3;
        break;

      case 3:  // Done
        Serial.println("State 3 - Calibration Complete. Green LED OFF.");
        return; // Exit the function and return to wherever it was called
	      PotCalBool = false;
    }
    delay(50); // Small delay to prevent flooding the serial monitor
  }
}

	


// Convert potentiometer value to stepper motor steps
int potValueToSteps(int potValue, int potMin, int potMax, int stepMin, int stepMax) {
    return map(potValue, potMin, potMax, stepMin, stepMax);
}

// Convert stepper motor steps to potentiometer value
int stepsToPotValue(long stepPosition, int potMin, int potMax, long stepMin, long stepMax) {
    return map(stepPosition, stepMin, stepMax, potMin, potMax);
}







void moveToPosition(long targetPositionInput) {
  long stepsToMove = targetPositionInput - currentPosition;
  digitalWrite(dirPin, (stepsToMove > 0) ? LOW : HIGH);  // Set direction based on target position

  unsigned long currentMillis = micros();  // Get the current time in microseconds
  unsigned long lastStepTime= 0;

  // Use a while loop to keep stepping until the target is reached
  while (currentPosition != targetPositionInput) {
    currentMillis = micros();  // Continuously update the time
    

    // If enough time has passed since the last step, take a step
    while (micros() - lastStepTime >= stepDelay) {
      digitalWrite(stepPin, HIGH);   // Send a high pulse to the STEP pin
      digitalWrite(stepPin, LOW);    // Send a low pulse to the STEP pin to complete the step

      // Increment or decrement the current position based on direction
      currentPosition += (stepsToMove > 0) ? 1 : -1;

      // Update the last step time
      lastStepTime = micros();
    }
  }
}

void simplePotCalibration() {
    Serial.println("Starting Simple Pot Calibration...");
    
    int potValue;
    int tolerance = 20;  // Define acceptable range around 512

    // Step 1: Wait for the pot to be in the middle range
    while (true) {
        potValue = analogRead(potPin);
        Serial.print("Current Pot Value: ");
        Serial.println(potValue);

        if (potValue >= (512 - tolerance) && potValue <= (512 + tolerance)) {
            Serial.println("Potentiometer in middle range!");
            delay(3000);  // Wait 3 seconds to confirm stability

            // Re-read pot value after delay
            potValue = analogRead(potPin);
            if (potValue >= (512 - tolerance) && potValue <= (512 + tolerance)) {
                Serial.println("Potentiometer stayed in range. Proceeding...");
                break;
            } else {
                Serial.println("Potentiometer moved out of range. Restarting...");
            }
        }

        delay(100);  // Small delay to prevent serial flooding
    }

    // Step 2: Move stepper to exact potentiometer position
    long targetPosition = potValueToSteps(potValue, 0, 1023, 0, calibratedPosition2);
    Serial.print("Moving Stepper to Pot Value: ");
    Serial.println(targetPosition);
    
    moveToPosition(targetPosition);

    Serial.println("Simple Pot Calibration Complete!");
}



