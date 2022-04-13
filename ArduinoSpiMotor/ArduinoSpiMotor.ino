#include <SPI.h>
#include "GyverStepper.h"
GStepper< STEPPER2WIRE> stepperLeft(800, 4, 3, 2);
byte leftMoving;

#define DATA_SIZE 40
byte data[DATA_SIZE];
int int_data[DATA_SIZE / 2];
byte sendData[DATA_SIZE];
volatile byte counter = 0;
volatile byte in_byte = 0;
volatile byte spiTranferEnd = 0;
volatile byte spiTranferStarted = 0;

void fillSendData() {
  for (byte i = 1; i < 40; i++) {
    sendData[i] = i;
  }
}

void setup() {
  pinMode(5, OUTPUT);
  digitalWrite(5, 1);
  Serial.begin(9600);

  stepperLeft.setRunMode(FOLLOW_POS);
  stepperLeft.setMaxSpeed(6000);
  stepperLeft.setAcceleration(3000);
  stepperLeft.autoPower(1);
  stepperLeft.setTarget(1000, RELATIVE);

  pinMode(MISO, OUTPUT);
  SPCR |= _BV(SPE);
  SPI.attachInterrupt();
  fillSendData();
}

ISR (SPI_STC_vect)
{
  in_byte = SPDR;
  if (in_byte == 240 and !spiTranferStarted) {
    spiTranferStarted = 1;
    counter = 0;
    SPDR = sendData[counter];
  }
  if (spiTranferStarted and counter > 0) {
    data[counter - 1] = in_byte;
    SPDR = sendData[counter];
  }
  counter++;

  if (counter == DATA_SIZE) {
    SPDR = sendData[counter - 1];
    counter = 0;
    spiTranferStarted = 0;
    spiTranferEnd = 1;
  }
}

void joinRecievedBytes() {
  for (int i = 0; i < DATA_SIZE; i += 2) {
    int_data[i / 2] = data[i] << 8 | data[i + 1];
  }
  spiTranferEnd = 0;
}
void printSpiData() {
  for (int i = 0; i < DATA_SIZE / 2; i++) {
    Serial.print(int_data[i]);
    Serial.print(" ");
  }
  Serial.println();

}

void loop () {
  sendData[0] = leftMoving;
  if (spiTranferEnd) {
    joinRecievedBytes();
    //printSpiData();
    if (int_data[0] == 1) { //установка новой цели для моторов
      printSpiData();
      if (int_data[1] and int_data[2] and int_data[3]) {
        stepperLeft.setMaxSpeed(int_data[1]);
        stepperLeft.setAcceleration(int_data[2]);
        stepperLeft.setTarget(int_data[3], RELATIVE);
      }
    } else if (int_data[0] == 2){//установка servo
      int num = int_data[1];
      int start_angle= int_data[2];
      int fin_angle = int_data[3];
      int servo_delay = int_data[4];
      Serial.print(num);
      Serial.print("\t");
      Serial.print(start_angle);
      Serial.print("\t");
      Serial.print(fin_angle);
      Serial.print("\t");
      Serial.print(servo_delay);
      Serial.print("\r\n");
    }
  }
  leftMoving = stepperLeft.tick();
}
