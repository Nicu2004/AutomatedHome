#include <Stepper.h>
#include <DHT11.h>          
#include <SoftwareSerial.h>

// comunication between two boards using SoftwareSerial
SoftwareSerial masterLink(10, 11); // RX, TX

// pin defintions
#define pwmB 9
#define rgtB 8
#define lftB 7
#define pwmA 6
#define rgtA 5
#define lftA 4
#define dhtPin 2 
#define photoPin A0  
#define moistSensor A1
#define ledPns 3     

// defining the objects
//defining the stepper
Stepper stepper(250, A2, A3, A4, A5); 
const int stepsToMove = 500; //steps to move the stepper
//temp module
DHT11 dht11(dhtPin);

// data struct for motors
typedef struct {
  int16_t mode; //automated or manual
  int16_t motorA; 
  int16_t motorB;
} MasterToSlave;
//data struct temperature
typedef struct {
  int16_t temp;
  int16_t hum; 
} temperatures;

MasterToSlave data = {0, 0, 0}; //default init, all stopped
temperatures temHum = {0, 0};

// Flags
int isOppened = 0;//is the door oppened?
bool stepperOverride = false; // flag for manual ovveride
//global vars
unsigned long sensorTime = 0;
int currentTemp = 0; 
int currentHum = 0;
int autoMotorA = 0; 
int autoMotorB = 0; 

void setup() {
  // Motor A Setup (Fan)
  pinMode(pwmA, OUTPUT); pinMode(rgtA, OUTPUT); pinMode(lftA, OUTPUT);
  digitalWrite(lftA, LOW); digitalWrite(rgtA, LOW); digitalWrite(pwmA, LOW);

  // Motor B Setup (Pump)
  pinMode(pwmB, OUTPUT); pinMode(rgtB, OUTPUT); pinMode(lftB, OUTPUT);
  digitalWrite(lftB, LOW); digitalWrite(rgtB, LOW); digitalWrite(pwmB, LOW);
  
  // Sensors & Actuators
  pinMode(ledPns, OUTPUT); 
  pinMode(photoPin, INPUT);
  pinMode(moistSensor, INPUT);
  
  // Init
  stepper.setSpeed(20); 
  masterLink.begin(9600);
  Serial.begin(9600); // USB Serial for debugging/manual commands
  
  Serial.println(F("Slave Ready. Cmds: 'c'=Close, 'o'=Open, 'r'=Resume Auto"));
}

void loop() {
  //serial ovveride
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // commnad C for close
    if (cmd == 'c') {
      Serial.println(F("Manual CMD: Closing..."));
      stepperOverride = true; // Stop temperature from interfering
      if (isOppened == 1) {
        stepper.step(stepsToMove); // Close
        isOppened = 0;
      }
    }
    
    // command o for Opend
    else if (cmd == 'o') {
      Serial.println(F("Manual CMD: Opening..."));
      stepperOverride = true; //temperature is not interferring
      if (isOppened == 0) {
        stepper.step(-stepsToMove); // Open
        isOppened = 1;
      }
    }
    
    // command r for resume auto mode
    else if (cmd == 'r') {
      Serial.println(F("Manual CMD: Resume Auto Mode"));
      stepperOverride = false; //temperarture will be taken in account
    }
  }

//recieve data from master
  if (masterLink.available() > 0) {
    if (masterLink.peek() == 0xAA) {
      if (masterLink.available() >= sizeof(MasterToSlave) + 1) {
        masterLink.read(); 
        masterLink.readBytes((char*)&data, sizeof(MasterToSlave));
      }
    } else { 
      masterLink.read(); 
    }
  }

  //every 2 seconds data is red
  if (millis() - sensorTime > 2000) {
    sensorTime = millis(); 

    //get the temeperature
    if (dht11.readTemperatureHumidity(currentTemp, currentHum) == 0) {
      temHum.temp = currentTemp;
      
      //only run if manual mode is off
      if (stepperOverride == false) {
        // open if 25-30C
        if (currentTemp >= 25 && currentTemp <= 30) { 
          if (isOppened == 0) { 
            Serial.println("Auto: Opening Window...");
            stepper.step(-stepsToMove); 
            isOppened = 1; 
          }
        } 
        // close if not in the interval 25-30, cold or hot
        else { 
          if (isOppened == 1) { 
            Serial.println("Auto: Closing Window...");
            stepper.step(stepsToMove); 
            isOppened = 0; 
          }
        }
      }

      // Fan Logic
      autoMotorA = (currentTemp > 30) ? 1 : 0;
      
    } else {
      Serial.println("DHT Error");
    }

    // led mapping
    int lightVal = analogRead(photoPin);
    analogWrite(ledPns, map(lightVal, 0, 1023, 255, 0));
    
    // soil mositure
    int soilMoist = analogRead(moistSensor);
    temHum.hum = soilMoist; 
    autoMotorB = (soilMoist > 800) ? 1 : 0; 
    //release steppe coils, stepper wont keep hot
    digitalWrite(A2, LOW); digitalWrite(A3, LOW); 
    digitalWrite(A4, LOW); digitalWrite(A5, LOW);

    // send data to master
    masterLink.write(0xBB); 
    masterLink.write((uint8_t*)&temHum, sizeof(temHum));
  }

  //motor logic
  int finalMotorA, finalMotorB;
  
  if (data.mode == 1) { 
    finalMotorA = data.motorA;
    finalMotorB = data.motorB;
  } else { 
    finalMotorA = autoMotorA;
    finalMotorB = autoMotorB; 
  }

  digitalWrite(rgtA, finalMotorA == 1 ? HIGH : LOW);
  digitalWrite(pwmA, finalMotorA == 1 ? HIGH : LOW);

  digitalWrite(rgtB, finalMotorB == 1 ? HIGH : LOW);
  digitalWrite(pwmB, finalMotorB == 1 ? HIGH : LOW);
}
