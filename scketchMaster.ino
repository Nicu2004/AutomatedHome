#include <LiquidCrystal.h>
#include <Servo.h>
#include <NewPing.h>
#include <SoftwareSerial.h>
//define the connection between boards
SoftwareSerial slaveSerial(A4, A5); // RX, TX
//define pin 
//sonar
#define TRIG_PIN 8
#define ECHO_PIN 10
//buzzer
#define BUZZER_PIN 12 
//led
#define LED_0 A0

//servo motor
#define SERVO_PIN 9

//data struct motor modes
typedef struct {
  int manual;
  int motorA;
  int motorB;
} MasterToSlave;
//temperatures data struct
typedef struct {
  int temp;
  int hum;
} SlaveFeedback;
//default values if nothoing is read
SlaveFeedback feedbackData = {0, 0};

//props for servo
int startStepServo = 30;
int maxStepServo = 130;
int currentServoStep = startStepServo;
unsigned long timeServo = 0;
//global vars
int state = 0; 
int lastDistance = 0;
unsigned long triggerTime = 0; 

int manual = 0; 
int motorA = 0; 
int motorB = 0;
int ledState = 0;
int lastManual = -1, lastMotorA = -1, lastMotorB = -1;
//define servo obj
Servo servo;
LiquidCrystal lcd(7, 6, 5, 4, 3, 11); // RS, E, D4, D5, D6, D7
NewPing sonar(TRIG_PIN, ECHO_PIN, 200); 

//function definition
void updateStatusLCD();
void updateSensorLCD();
void servoAction(int dist);

void setup() {
  Serial.begin(9600);      
  slaveSerial.begin(9600); 
  //pin modes
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_0, OUTPUT);


  servo.attach(SERVO_PIN);
  servo.write(startStepServo);
  delay(500); 
  servo.detach(); 


  lcd.begin(16, 2);
  lcd.print("Serial Control");
  delay(1000);
  lcd.clear();
  
  Serial.println("System Ready. Monitoring Distance...");
}

void loop() {

//read data from computer for commands
  if (Serial.available() > 0) {
    char input = Serial.read();
    bool validCommand = true;
      
    //switch for command, manual, auto
    switch(input) {
      case 'm': manual = !manual; motorA = 0; motorB = 0; break;
      case 'a': manual = 1; motorA = !motorA; break;
      case 'b': manual = 1; motorB = !motorB; break;
      case 'l': ledState = !ledState; break;
      default: validCommand = false; break;
    }

    if (validCommand) {
      digitalWrite(BUZZER_PIN, HIGH); 
      delay(50); 
      digitalWrite(BUZZER_PIN, LOW);
      updateStatusLCD();
    }
  }


  lastDistance = sonar.ping_cm();

  if (lastDistance == 0) lastDistance = -1; 

  static unsigned long lastCommTime = 0;
  bool dataChanged = (manual != lastManual || motorA != lastMotorA || motorB != lastMotorB);
  
  if (dataChanged || (millis() - lastCommTime >= 1000)) {
    MasterToSlave msslv = {manual, motorA, motorB};
    slaveSerial.write(0xAA); 
    slaveSerial.write((uint8_t*)&msslv, sizeof(msslv));
    lastManual = manual; lastMotorA = motorA; lastMotorB = motorB;
    lastCommTime = millis();
  }


  if (slaveSerial.available() > 0) {
    if (slaveSerial.peek() == 0xBB) { 
      if (slaveSerial.available() >= sizeof(SlaveFeedback) + 1) {
        slaveSerial.read(); 
        slaveSerial.readBytes((char*)&feedbackData, sizeof(SlaveFeedback));
        updateSensorLCD();
      }
    } else { slaveSerial.read(); }
  }

  digitalWrite(LED_0, ledState == 1 ? HIGH : LOW);
  servoAction(lastDistance);

  delay(50);
}


void servoAction(int dist) {
  //print distance
  Serial.print("Distance: ");
  if (dist == -1) {
    Serial.println("Out of Range");
  } else {
    Serial.print(dist);
    Serial.println(" cm");
  }
  // -------------------------------

  /// state 0, closed
  if (state == 0) {
    if (dist > 0 && dist < 21) {
      if (triggerTime == 0) triggerTime = millis();
      
      if (millis() - triggerTime >= 1500) { 
        servo.attach(SERVO_PIN);
        delay(10); 
        state = 1;
        lcd.setCursor(0, 0);
        lcd.print("Opening...      ");
      }
    } else {
      triggerTime = 0; 
    }
  }

  // state 1 Opening 
  if (state == 1) {
    if (currentServoStep < maxStepServo) {
      if (millis() - timeServo >= 15) { 
        servo.write(currentServoStep++);
        timeServo = millis();
      }
    } else {
      servo.detach(); 
      state = 2;
      updateSensorLCD(); 
    }
  }

  // state 2 oppened
  if (state == 2) {
    if (dist > 35 || dist == -1) { 
      servo.attach(SERVO_PIN);
      delay(10);
      state = 3;
      lcd.setCursor(0, 0);
      lcd.print("Closing...      ");
    }
  }

  //state 3, closing
  if (state == 3) {
    if (currentServoStep > startStepServo) {
      if (millis() - timeServo >= 15) {
        servo.write(currentServoStep--);
        timeServo = millis();
      }
    } else {
      servo.detach(); 
      state = 0;
      updateSensorLCD(); 
    }
  }
}

void updateStatusLCD() {
  lcd.setCursor(0, 1);
  if(manual) lcd.print("MAN: A="); else lcd.print("AUT: A=");
  lcd.print(motorA); lcd.print(" B="); lcd.print(motorB);
}

void updateSensorLCD() {
  if(state == 0 || state == 2) {
    lcd.setCursor(0, 0);
    lcd.print("T:"); lcd.print(feedbackData.temp);
    lcd.print("C H:"); lcd.print(feedbackData.hum);
    lcd.print("%  "); 
  }
}
