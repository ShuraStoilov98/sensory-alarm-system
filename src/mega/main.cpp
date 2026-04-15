#include <Arduino.h>

// Define pins
const int STEP_PIN = 44;  // Pin connected to STEP on DRV8825
const int DIR_PIN = 46;   // Pin connected to DIR on DRV8825
const int DIR_BUTTON = 34;
const int BUTTON_PIN = 56; // Pin connected to the power button (analouge A2 = digital pin # 56)
const int ENABLE_PIN = 27; // Pin connected to ENABLE (optional)
const int KILL_SWITCH_CLOSED_PIN = 60;  // Digital input for Kill Switch Front (the one at the front of curtain - right, max closed) :: (analouge A6 = digital pin # 60)
const int KILL_SWITCH_OPENED_PIN = 64;  // Digital input for Kill Switch Back (the one at the back of curtains - left, max opened) :: (analouge A10 = digital pin # 64)
const int ALARM_PIN = 54; //Pin connected to ESP32 module to give alarm triggered signal / for initial test manually input 3.3V / :: (analog A0 = digital pin # 54 )

// Motor settings
const int DELAY_BETWEEN_STEPS = 500; // Microseconds between steps (adjust for speed)

// Setup function runs once at startup
void setup() {
  // Set pins as output
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(DIR_BUTTON, INPUT);
  pinMode(KILL_SWITCH_CLOSED_PIN, INPUT);
  pinMode(KILL_SWITCH_OPENED_PIN, INPUT);
  pinMode(ALARM_PIN, INPUT);

  // Serial monitor for debugging 
  Serial.begin(9600);
}

// Function to run motors in the selected direction
void RunMotors() {
  // Runs motors when command given
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(DELAY_BETWEEN_STEPS); // Wait
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(DELAY_BETWEEN_STEPS); // Wait
}

// Main loop runs continuously
void loop() {
  // main loop of program - motors stationary - waiting for trigger to turn on 
  digitalWrite(ENABLE_PIN, HIGH); // disable motor to prevent noise and slipping 
  bool direction = digitalRead(DIR_BUTTON); // read direction from direction switch 
  digitalWrite(DIR_PIN, direction); // get direction of motor rotation from direction manual switch (assumes we can't and won't change direction while moving)

  //Serial.println("Awaiting alarm trgigger or manual command");

  // Checking if ALARM sequence is triggered to turn on motors IF end position (fully OPENED or CLOSED not reached) 
  if (digitalRead(ALARM_PIN) == HIGH){
    Serial.println("Alarm triggered"); // debugging
    if (digitalRead(KILL_SWITCH_OPENED_PIN) == HIGH){ 
      digitalWrite(ENABLE_PIN, HIGH); // keep power to motor disabled to prevent motor 'humming or slipping' if curtain already OPENED
      Serial.println("Curtain already Opened");
    }
    else{
      direction = 1; // override and set direction to OPENNING 
      digitalWrite(DIR_PIN, direction); // Set direction to OPENNING 
      digitalWrite(ENABLE_PIN, LOW); // alarm has more curtain to open, enable power to motor
      Serial.println("Alarm opening curtain"); // Debugging
      while(digitalRead(KILL_SWITCH_OPENED_PIN) != HIGH){ // run motors untill curtain fully opened 
        RunMotors();
      }
      Serial.println("Alarm finished and curtain fully OPENED");
      direction = digitalRead(DIR_BUTTON); // reset direction to be the one read from sensor
      digitalWrite(DIR_PIN, direction); // Reset direction
    }  
  }

  // Looking for manual signal to turn on motors IF end position (fully OPENED or CLOSED not reached)
  if ((digitalRead(KILL_SWITCH_CLOSED_PIN) == HIGH && direction == 1) || ((digitalRead(KILL_SWITCH_OPENED_PIN) == HIGH && direction == 0))){  // Check if either kill switch triggered when direction is set to push further than end limit !!! TO DO - calibrate & make sure 0,1 correspond to correct direction !!!
    Serial.println("Curtain canot move more in selected direction");
    digitalWrite(ENABLE_PIN, HIGH); // keep power to motor disabled to prevent motor 'humming or slipping'
  }
  else{ // if neigher KILL switch is pressed OR kill smitch pressed but direction is in oposite way - give green light to power motor and open/close curtains
    if (digitalRead(BUTTON_PIN) == HIGH) { 
      digitalWrite(ENABLE_PIN, LOW); // if manual power button pressed, enable power to motor
      Serial.println("Motor Running"); // Debugging 
      while (digitalRead(BUTTON_PIN) == HIGH) {       
        // if not at end posotion of direction of movement and power signal recieved, move motor in given direction
        RunMotors();
        if ((digitalRead(KILL_SWITCH_CLOSED_PIN) == HIGH && direction == 1) || ((digitalRead(KILL_SWITCH_OPENED_PIN) == HIGH && direction == 0))){ 
          break;
        }
      }
      /*
      if (digitalRead(BUTTON_PIN) != HIGH){ Serial.println("Power button unpressed");} // Debugging
      if ((digitalRead(KILL_SWITCH_CLOSED_PIN) == HIGH && direction == 1) || ((digitalRead(KILL_SWITCH_OPENED_PIN) == HIGH && direction == 0)) == 1) {Serial.println("Curtain reached maximum movement in selected direction");} // Debugging
      Serial.println(direction); // Debugging
      Serial.println(digitalRead(KILL_SWITCH_CLOSED_PIN)); // Debugging
      Serial.println(digitalRead(KILL_SWITCH_OPENED_PIN)); // Debugging
      */
    }
  }
}