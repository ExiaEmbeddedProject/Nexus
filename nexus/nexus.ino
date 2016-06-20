// Humidity sensor
// Gyro

#include <Wire.h>

#include <SPI.h>
#include <SD.h>

#include "DHT.h"
// GPS
#include "TinyGPS++.h"
#include <SoftwareSerial.h>


#define HUMIDITY_PIN 2
#define HUMIDITY_TYPE DHT22
#define MPU 0x68
#define MAX_FILE_ENTRIES 3000
#define GPS_BAUD 9600
#define GPS_RXP_IN 4
#define GPS_TXP_IN 3
#define FREQUENCY 0.2 // Hz
#define SD_PIN 10

// Humidity sensor
DHT dht(HUMIDITY_PIN, HUMIDITY_TYPE);
// GPS
TinyGPSPlus gps;
SoftwareSerial ss(GPS_RXP_IN, GPS_TXP_IN);

unsigned long id = 0;
unsigned int lineNumber = 0;
unsigned int fileNumber = 1;
unsigned int humidity = 0;
char fileName[50];
char json[40];

float gyro[3];
float accel[3];
float latitude = 0;
float longitude = 0;
float temperature = 0;
TinyGPSDate date;
TinyGPSTime time;

void writeInitialStructure() {
  writeInFile(F("{\"DATA\":[]}"), false, false);
}


void writeData() {
  writeInFile(F("{\"id\":\""), true, true);
  writeInFile((String)(id), true, false);

  writeInFile(F("\",\"DateHeure\":"), true, false);
  writeInFile((String)(date.year()), true, false);
  writeInFile(F("-"), true, false);
  writeInFile((String)(date.month()), true, false);
  writeInFile(F("-"), true, false);
  writeInFile((String)(date.day()), true, false);
  writeInFile(F("T"), true, false);
  writeInFile((String)(time.hour()), true, false);
  writeInFile(F(":"), true, false);
  writeInFile((String)(time.minute()), true, false);
  writeInFile(F(":"), true, false);
  writeInFile((String)(time.second()), true, false);
  writeInFile(F(":"), true, false);
  writeInFile((String)(time.centisecond()), true, false);
  writeInFile(F("Z"), true, false);
  
  
  writeInFile(F("\",\"Humidite\":"), true, false);
  writeInFile((String)(humidity), true, false);

  writeInFile(F(",\"Temperature\":"), true, false);
  writeInFile((String)(temperature), true, false);

  writeInFile(F(",\"Accel_x\":"), true, false);
  writeInFile((String)(accel[0]), true, false);

  writeInFile(F(",\"Accel_y\":"), true, false);
  writeInFile((String)(accel[1]), true, false);

  writeInFile(F(",\"Accel_z\":"), true, false);
  writeInFile((String)(accel[2]), true, false);

  writeInFile(F(",\"Gyro_yaw\":"), true, false);
  writeInFile((String)(gyro[0]), true, false);

  writeInFile(F(",\"Gyro_pitch\":"), true, false);
  writeInFile((String)(gyro[1]), true, false);

  writeInFile(F(",\"Gyro_roll\":"), true, false);
  writeInFile((String)(gyro[2]), true, false);

  writeInFile(F(",\"Latitude\":"), true, false);
  writeInFile((String)(latitude), true, false);

  writeInFile(F(",\"Longitude\":"), true, false);
  writeInFile((String)(longitude), true, false);

  writeInFile(F("}]}"), true, false);
  
  //sprintf(json, "{\"id\":\"%d\"", id);
  //writeInFile(json, true, true);
  /*sprintf(json, ",\"Humidite\":%d", humidity);
  writeInFile(json, true, false);
  char str_temp[6];
  dtostrf(temperature, 3, 2, str_temp);
  sprintf(json, ",\"Temperature\":%s", str_temp);
  writeInFile(json, true, false);
  dtostrf(accel[0], 2, 2, str_temp);
  sprintf(json, ",\"Accel_x\":%s}]}", str_temp);
  writeInFile(json, true, false);
  sprintf(json, ",\"Accel_y\":%s}]}", str_temp);
  writeInFile(json, true, false);
  sprintf(json, ",\"Accel_z\":%s}]}", str_temp);
  writeInFile(json, true, false);
  */
}

void writeInFile(String data, bool inject, bool newObject) {
  File dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
    if (inject) {
      if(newObject) {
        dataFile.seek(dataFile.size() - 2 * sizeof(char));
      }
      else {
        dataFile.seek(dataFile.size());
      }
    }
    if (lineNumber != 1 && newObject)
      dataFile.print(F(","));
    dataFile.print(data);
    dataFile.close();
  }
  else
  {
    //Serial.println(F("Failed to access to file."));
  }
}

void initGyro() {
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void update() {
  id++;
  lineNumber++;
  if (lineNumber == MAX_FILE_ENTRIES || id == 1) {
    sprintf(fileName, "%s%06d%s", "04", fileNumber, ".txt");
    writeInitialStructure();
  }
  if (lineNumber == MAX_FILE_ENTRIES) {
    fileNumber++;
    lineNumber = 0;
  }
}

void updateHumidity() {
  humidity = dht.readHumidity();
}

void updateGyro() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);
  accel[0] = (Wire.read() << 8 | Wire.read()) / 16384 * 9.80665; // (ACCEL_XOUT_L)
  accel[1] = (Wire.read() << 8 | Wire.read()) / 16384 * 9.80665; // (ACCEL_YOUT_L)
  accel[2] = (Wire.read() << 8 | Wire.read()) / 16384 * 9.80665; // (ACCEL_ZOUT_L)
  temperature = (Wire.read() << 8 | Wire.read()) / 340.00 + 36.53 + 273.15; // (TEMP_OUT_L)
  gyro[0] = (Wire.read() << 8 | Wire.read()) / 131; // (GYRO_XOUT_L)
  gyro[1] = (Wire.read() << 8 | Wire.read()) / 131; // (GYRO_YOUT_L)
  gyro[2] = (Wire.read() << 8 | Wire.read()) / 131; // (GYRO_ZOUT_L)
}

void updateGps() {
  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    date = gps.date;
    time = gps.time;
  }
}

void setup() {
  Serial.begin(9600);
  SD.begin(SD_PIN);
  dht.begin();
  ss.begin(GPS_BAUD);
  initGyro();
  Serial.println(F("K"));
}



void loop() {
  update();
  updateHumidity();
  updateGyro();
  updateGps();
  writeData();
  smartDelay(1 / FREQUENCY * 1000);
  //if (millis() > 5000 && gps.charsProcessed() < 10)
    //Serial.println(F("/!\\ No GPS data received, check wiring /!\\"));
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
