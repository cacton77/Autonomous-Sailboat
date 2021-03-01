
#include <Servo.h>
#include <SPI.h>

#define CS 3 //Chip or Slave select
#define ledPin 11
#define hallPin 2

uint16_t ABSposition = 0;
uint16_t ABSposition_last = 0;
uint8_t temp[1];

unsigned long lastDegUpdate = 0;
unsigned long lastRpsUpdate = 0;
volatile bool ledState = LOW;
float deg = 0.00;
int rps = 0;
int numRot = 0;
int lastSailupdate1;
int lastSailupdate2;

Servo myservo;  // create servo object to control a servo
Servo myservo2;

int bigpos = 0;    // variable to store the servo position
int smallpos = 0;
int pos;
int poswant;
int p;
int newpos;
int currentbigpos;
int wantedbigpos;
int wantedsmallpos;
int diffromstraight;
float maxrps=30;
float minrps=5;


char receivedChar;
unsigned int integerValue = 0; // Max value is 65535
char incomingByte;

void setup() {
  pinMode(CS, OUTPUT); //Slave Select
  pinMode(ledPin, OUTPUT);
  pinMode(hallPin, INPUT);
  digitalWrite(CS, HIGH);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  Serial.begin(115200);
  Serial.println("starting");
  Serial.flush();
  delay(2000);
  SPI.end();
  attachInterrupt(digitalPinToInterrupt(hallPin), hall_ISR, CHANGE);

  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  myservo2.attach(10);
  moveBigSail(bigpos);
  moveSmallSail(smallpos);
}

void loop() {
  digitalWrite(ledPin, ledState);
  unsigned long currTime = millis();
  if (currTime >= lastDegUpdate + 100)
  {
    updateDegree();
    lastDegUpdate = currTime;
  }
  if (currTime-lastRpsUpdate >= 10000)
  {
    rps = numRot;
    numRot = 0;
    lastRpsUpdate = currTime;
    Serial.println(rps);
  }
  degsToCamber(deg, rps);
//  if (currTime-lastSailupdate1 >= 30)
//  {
//    moveBigSail(wantedbigpos);
//    lastSailupdate1=currTime;
//  }
  moveBigSail(wantedbigpos);
  if (currTime-lastSailupdate2 >= 20)
  {
    moveSmallSail(wantedsmallpos);
    lastSailupdate2=currTime;
  }
}

void degsToCamber(int degs, int rps) {
  if (degs==0){
    diffromstraight=0;
    wantedbigpos=90-diffromstraight;
    wantedsmallpos=90;
  }
  if (degs>=355){
    diffromstraight=0;
    wantedbigpos=90-diffromstraight;
    diffromstraight=int (40-((rps/(maxrps-minrps))*(20)));
    wantedsmallpos=90-diffromstraight;
  }
   if (degs<=5 && degs>0){
    diffromstraight=0;
    wantedbigpos=90-diffromstraight;
    diffromstraight=int (40-((rps/(maxrps-minrps))*(20)));
    wantedsmallpos=90+diffromstraight;
  }
  if (degs>=265 && degs<355){
    diffromstraight=360-degs;
    wantedbigpos=90+diffromstraight-5;
    diffromstraight=int (40-((rps/(maxrps-minrps))*(20)));
    wantedsmallpos=90-diffromstraight;
  }
  if (degs>5 && degs<=95){
    diffromstraight=degs;
    wantedbigpos=90-diffromstraight+5;
    diffromstraight=int (40-((rps/(maxrps-minrps))*(20)));
    wantedsmallpos=90+diffromstraight;
  }
  if (degs>95 && degs<180){
    wantedbigpos=0;
    wantedsmallpos=90;
  }
  if (degs<265 && degs>=180){
    wantedbigpos=180;
    wantedsmallpos=90;
  }
}

uint8_t SPI_T (uint8_t msg)    //Repetive SPI transmit sequence
{
  uint8_t msg_temp = 0;  //vairable to hold recieved data
  digitalWrite(CS, LOW);    //select spi device
  msg_temp = SPI.transfer(msg);    //send and recieve
  digitalWrite(CS, HIGH);   //deselect spi device
  return (msg_temp);     //return recieved byte
}

void updateDegree()
{
  uint8_t recieved = 0xA5;    //just a temp vairable
  ABSposition = 0;    //reset position vairable

  SPI.begin();    //start transmition
  digitalWrite(CS, LOW);

  SPI_T(0x10);   //issue read command

  recieved = SPI_T(0x00);    //issue NOP to check if encoder is ready to send

  while (recieved != 0x10)    //loop while encoder is not ready to send
  {
    recieved = SPI_T(0x00);    //cleck again if encoder is still working
    delay(2);    //wait a bit
  }

  temp[0] = SPI_T(0x00);    //Recieve MSB
  temp[1] = SPI_T(0x00);    // recieve LSB

  digitalWrite(CS, HIGH); //just to make sure
  SPI.end();    //end transmition

  temp[0] &= ~ 0xF0;   //mask out the first 4 bits

  ABSposition = temp[0] << 8;    //shift MSB to correct ABSposition in ABSposition message
  ABSposition += temp[1];    // add LSB to ABSposition message to complete message

  if (ABSposition != ABSposition_last)    //if nothing has changed dont wast time sending position
  {
    ABSposition_last = ABSposition;    //set last position to current position
    deg = ABSposition;
    deg = deg * 0.08789;    // aprox 360/4096
    //Serial.println(deg);     //send position in degrees
  }
  //delay(10);    //wait a bit till next check
}

void updateRps()
{
  rps = numRot;
  numRot = 0;
}

void hall_ISR()
{
  numRot = numRot + 1;
  ledState = !ledState;
}

void moveBigSail(int poswant) {
  poswant=poswant+2;
  if (poswant < 0) {
    poswant = 0;
  }
  if (poswant > 180) {
    poswant = 180;
  }
  if (poswant > bigpos) {
    myservo.write(bigpos+1);
    bigpos=bigpos+1;
  }
  if (poswant < bigpos) {
    myservo.write(bigpos-1);
    bigpos=bigpos-1;
  }
}

void moveSmallSail(int poswant) {
  poswant=poswant-13;
  if (poswant < 45) {
    poswant = 45;
  }
  if (poswant > 135) {
    poswant = 135;
  }
  if (poswant > smallpos) {
    myservo2.write(smallpos+1);
    smallpos = smallpos+1;
  }
  if (poswant < smallpos) {
    myservo2.write(smallpos-1);
    smallpos = smallpos-1;
  }
}
