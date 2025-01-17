#include "Encoder.h"
#include <Wire.h>

// Here we define our pins and constants

#define M1SPEED       9 // used to modify the motor 1 speed RIGHT WHEEL
#define M2SPEED       10 // used to modify the motoe 2 speed LEFT WHEEL
#define M1DIR         7 // used to modify motor 1 direction 
#define M2DIR         8 // used to modify motor 2 direction 
#define CPR           800 // counts per revolution 
#define SLAVE_ADDRESS 0x04// dawson info 

#define POSTOLERANCE  20
#define NEGTOLERANCE  235

Encoder M1Encoder(2,5); // motor 1 encoder pins (RIGHT WHEEL)
Encoder M2Encoder(3,6); // motor 2 encoder pins (LEFT WHEEL)

// Here we have the variables in charge of recording old and new encoder values  
double newPosition1 = 0;
double newPosition2 = 0;

double errorAngle = 0; // used to calculate difference in target and current angle

// angle is desiredInputs[1], distance flag is desiredInputs[2]
byte desiredInputs[32] = {0}; // the list that will read in the values from the camera 

// here we set the variable to take care of ft to counts 
int desiredAngle;
double rectifiedAngle;

// flags
int state = 0; // used for the case statements 
int flagAngle; // used within the angle control 
int loopCounts = 0; // used within the angle control 

void setup() {
  pinMode(4, OUTPUT); // here we set the tri-state
  digitalWrite(4, HIGH);
  
  pinMode(M1DIR, OUTPUT); // RIGHT WHEEL 
  pinMode(M2DIR, OUTPUT); // LEFT WHEEL 
  
  pinMode(M1SPEED, OUTPUT); // motor 1 speed 
  pinMode(M2SPEED, OUTPUT); // motor 2 speed
  
  pinMode(12, INPUT); // status flag indicator 

  Serial.begin(9600); // baud rate for the serial monitor

  Wire.begin(SLAVE_ADDRESS); // initialize I2C as the subordinate

  // callbacks for I2C
  Wire.onRequest(sendData);
  Wire.onReceive(receiveData);
  Serial.println("Demo 2");
  Serial.println("Ready!");
  
}

// here we switch between the different states/functions 
void loop() {
  newPosition1 = abs(M1Encoder.read()/4); // scale down the values so calculations are easier on the Arduino 
  newPosition2 = abs(M2Encoder.read()/4);
  
  //desiredInputs[1] = 27; // values may be too small!
  //desiredInputs[2] = 0;
  Serial.print("Angle:"); // print the angle flad from the PI 
  Serial.println(desiredInputs[1]);
  Serial.print("Distance:"); // print the distance flag from PI
  Serial.println(desiredInputs[2]);

  switch (state){
    case 0:
    while(desiredInputs[1] > 26) {
      digitalWrite(M1DIR,HIGH); 
      digitalWrite(M2DIR,HIGH);
      analogWrite(M2SPEED, 40); 
      analogWrite(M1SPEED, 40);
      // we are going to slowly rotate until we have a value other than our 27 flag 
    }
    if(desiredInputs[1] <= 26) {
      state = 1;
    }
    break;
    
    case 1:
    
    if((desiredInputs[1] <= 26) && (flagAngle == 0) && (desiredInputs[1] != 0)) {
      desiredAngle = desiredInputs[1]; // here we have the positive angle case 
      rectifiedAngle = 3.55*desiredAngle; // here we calculate the number of counts per degree and multiply by degrees (full for both wheels)
      angleControl(rectifiedAngle);
    }
    
    else if ((desiredInputs[1] >= 229) && (flagAngle == 0) && (desiredInputs[1] != 0)) {
      desiredAngle = desiredInputs[1] - 255; // here we have the negative angle case 
      rectifiedAngle = 3.55*desiredAngle; // here we calculate the number of counts per degree and multiply by degrees (full for both wheels)
      angleControl(rectifiedAngle);
    }
    
    break;
    
    case 2: // here we will go straight until we hit the distance flag 
    loopCounts = 0;
    flagAngle = 0;
    while((desiredInputs[1] <= 26) && ((desiredInputs[1] < POSTOLERANCE) || (desiredInputs[1] > NEGTOLERANCE))){
      digitalWrite(M1DIR,HIGH); 
      digitalWrite(M2DIR, LOW);

      analogWrite(M1SPEED, 64);
      analogWrite(M2SPEED, 64); 
      
    } // PICKLE: Should we rotate when out of the tolerance and then go straight or add velocity to one wheel until we are aligned 

// we send it back to the angle control if we are out of our tolerance 
    if(desiredInputs[1] > POSTOLERANCE){
      state = 0;
    }
    else if (desiredInputs[1] < NEGTOLERANCE) {
      state = 0;
    }
    
    if(desiredInputs[1] > 26){ // if we hit our flag then stop the velocity entirely 
      // throw distance control in here 
      analogWrite(M1SPEED, 0); // WRITES THE SPEED OF THE MOTOR 
      analogWrite(M2SPEED, 0); // set M2 speed to 0.5 * max speed
      state = 0; // state is nonexistent so it should stay still 
    }
    break;
  }
}

// here we have the function which takes care of rotating a specified angle 
void angleControl(int targetAngle){
  // this keeps track of the direction of rotation 
  
  if (targetAngle > 0){ // turn CCW
    digitalWrite(M1DIR,HIGH); 
    digitalWrite(M2DIR,HIGH); 

    errorAngle = (targetAngle - newPosition1); // calculate the difference in our desired angle and current angle 
    //Serial.println(newPosition1);
    //Serial.println(errorAngle);
    if (errorAngle < 2){
      flagAngle = 1;
    }
    
    if (flagAngle == 1) {
      analogWrite(M2SPEED,0);
      analogWrite(M1SPEED, 0);
      state = 2;
      
      if (loopCounts == 0){
        delay(1000); // meant to delay after the angle is done so that we can take care of the angle nicely 
        M1Encoder.write(0);
        M2Encoder.write(0);
      }
      loopCounts = loopCounts + 1; // maybe change this back to normal 
    }
    else{
      analogWrite(M2SPEED, 64);
      analogWrite(M1SPEED, 64 ); 
    }
  }
  else{ // with a negative angle we will turn CW
    digitalWrite(M1DIR,LOW); 
    digitalWrite(M2DIR,LOW); 
    
    errorAngle = (abs(targetAngle) - newPosition2); // calculate the difference in our desired angle and current angle 

    if (errorAngle < 2){
      flagAngle = 1;
    }
    
    if (flagAngle == 1) {
      analogWrite(M1SPEED, 0);
      analogWrite(M2SPEED,0);
      state = 2;
      
      if (loopCounts == 0){
        delay(1000);
        M1Encoder.write(0);
        M2Encoder.write(0);
      }
      loopCounts = loopCounts + 1;
    }
    else{
      analogWrite(M1SPEED, 64);
      analogWrite(M2SPEED, 64);
    }
  }
}

void receiveData(int byteCount){
  int i = 0;
  while(Wire.available()) {
    desiredInputs[i] = Wire.read();
    i++;
  }
  i--;
}

void sendData(){
  Wire.write(desiredInputs[1]);
}
