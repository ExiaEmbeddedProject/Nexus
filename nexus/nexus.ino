// Humidity sensor
#include "DHT.h"
// Gyro
#include<Wire.h>
// GPS
#include "TinyGPS++.h"
#include <SoftwareSerial.h>


#define HUMIDITY_PIN 4
#define HUMIDITY_TYPE DHT22
#define MPU 0x68
#define MAX_FILE_ENTRIES 3000
#define GPS_BAUD 4800
#define GPS_RXP_IN 4
#define GPS_TXP_IN 3
#define FREQUENCY 0.2 // Hz


// Humidity sensor
DHT dht(HUMIDITY_PIN, HUMIDITY_TYPE);
// GPS
TinyGPSPlus gps;
SoftwareSerial ss(GPS_RXP_IN, GPS_TXP_IN);


long id = 0;
int lineNumber = 0;
int fileNumber = 1;
char fileName[50];
char json[200];

int humidity;
float temperature;
float gyro[3];
float accel[3];
float latitude;
float longitude;
float speed; // km/h
TinyGPSDate date;
TinyGPSTime time;


void updateJson() {
  sprintf(json, "%s", "{\"DATA\":[]}");
}

void printInfos() {
  Serial.println("------------------------");
  
  Serial.println("----FILE----");
  
  Serial.print("Id: ");
  Serial.println(id);
  Serial.print("LineNumber: ");
  Serial.println(lineNumber);
  Serial.print("FileNumber: ");
  Serial.println(fileNumber);
  Serial.print("FileName: ");
  Serial.println(fileName);
  
  Serial.println("---SENSOR---");
  
  Serial.print("Date: ");
  Serial.print(date.year(), DEC);
  Serial.print('/');
  Serial.print(date.month(), DEC);
  Serial.print('/');
  Serial.print(date.day(), DEC);
  Serial.print(' ');
  Serial.print(time.hour(), DEC);
  Serial.print(':');
  Serial.print(time.minute(), DEC);
  Serial.print(':');
  Serial.print(time.second(), DEC);
  Serial.print('.');
  Serial.print(time.centisecond(), DEC);
  Serial.println();
  
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°c");
  
  Serial.println("Gyro: ");
  Serial.print("   x: ");
  Serial.println(gyro[0]);
  Serial.print("   y: ");
  Serial.println(gyro[1]);
  Serial.print("   z: ");
  Serial.println(gyro[2]);
  
  Serial.println("Acceleration: ");
  Serial.print("   x: ");
  Serial.println(accel[0]);
  Serial.print("   y: ");
  Serial.println(accel[1]);
  Serial.print("   z: ");
  Serial.println(accel[2]);

  Serial.println("GPS: ");
  Serial.print("   lat: ");
  Serial.println(latitude);
  Serial.print("   lng: ");
  Serial.println(longitude);
  Serial.print("   speed: ");
  Serial.print(speed);
  Serial.println("km/h");
}

void writeInFile() {
  
}

void initHumidity() {
  dht.begin();
}

void initGyro() {
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void initGps() {
  ss.begin(GPS_BAUD);
}

void update() {
  id++;
  lineNumber++;
  if (lineNumber == MAX_FILE_ENTRIES) {
    fileNumber++;
    lineNumber = 0;
  }
  if (lineNumber == MAX_FILE_ENTRIES || id == 1) sprintf(fileName, "%s%06d%s", "04", fileNumber, ".txt");
}

void updateHumidity() {
  humidity = dht.readHumidity();
}

void updateGyro() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,14,true);
  accel[0] = Wire.read()<<8|Wire.read();  // (ACCEL_XOUT_L)    
  accel[1] = Wire.read()<<8|Wire.read();  // (ACCEL_YOUT_L)
  accel[2] = Wire.read()<<8|Wire.read();  // (ACCEL_ZOUT_L)
  temperature = (Wire.read()<<8|Wire.read()) / 340.00 + 36.53;  // (TEMP_OUT_L)
  gyro[0] = Wire.read()<<8|Wire.read();  // (GYRO_XOUT_L)
  gyro[1] = Wire.read()<<8|Wire.read();  // (GYRO_YOUT_L)
  gyro[2] = Wire.read()<<8|Wire.read();  // (GYRO_ZOUT_L)
}

void updateGps() {
  if(gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    speed = gps.speed.kmph();
    date = gps.date;
    time = gps.time;
  }
}

void setup() {
  Serial.begin(9600);
  initHumidity();
  initGyro();
  initGps();
  Serial.println("Initialized");
}

void loop() {
  update();
  updateHumidity();
  updateGyro();
  updateGps();
  printInfos();
  writeInFile();
  smartDelay(1/FREQUENCY*1000);
  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("/!\\ No GPS data received, check wiring /!\\"));
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}