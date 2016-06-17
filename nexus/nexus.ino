#include <avr/pgmspace.h>
// Multithreading
#include <ChibiOS_AVR.h>
// Temperature & Humidity
#include "DHT.h"
// Gyro
#include<Wire.h>
// SD read/write
#include "PetitFS.h"
#include "PetitSerial.h"

#define TERMOMETER_PIN 4
#define TERMOMETER_TYPE DHT22
#define SD_PIN 10

PetitSerial PS;
#define Serial PS
FATFS fs;

float data[11];
int line = 0;
int file = 0;
long id = 0;
uint8_t buf[32];

static THD_WORKING_AREA(waThread1, 64);
static THD_FUNCTION(Thread1 ,arg) {
  Serial.println("Thread 1 started."); 

  DHT dht(TERMOMETER_PIN, TERMOMETER_TYPE); // temp & humidity
  dht.begin();
  float humidity;
  
  while(1) {
    chThdSleepMilliseconds(4000);
    humidity = dht.readHumidity();
  
    if (isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      continue;
    }
    //Serial.print("hum: ");
    //Serial.println(humidity);
    data[0] = humidity;
  }
}


static THD_WORKING_AREA(waThread2, 64);
static THD_FUNCTION(Thread2, arg) {
  Serial.println("Thread 2 started.");

  int MPU=0x68; // I2C address of the MPU-6050
  int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  while(1) {
    chThdSleepMilliseconds(2000);
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU,14,true);  // request a total of 14 registers
    data[2] = Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
    data[3] = Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    data[4] = Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
    data[1] = (Wire.read()<<8|Wire.read()) / 340.00 + 36.53;  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    data[5] = Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
    data[6] = Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
    data[7] = Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
    //Serial.print("temp: ");
    //Serial.println(data[1]);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Initialized."); 
  if (pf_mount(&fs)) errorHalt("pf_mount");
  chBegin(mainThread);
}

void errorHalt(char* msg) {
  Serial.print("Error: ");
  Serial.println(msg);
  while(1);
}

void mainThread() {
  Serial.println("Main thread started."); 
  
  Serial.println("Starting Thread 1..."); 
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO + 1, Thread1, NULL);
  
  Serial.println("Starting Thread 2..."); 
  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO + 1, Thread2, NULL);

  if (pf_open("FOO.BAR")) errorHalt("pf_open");
  
  while (1) {
    chThdSleepMilliseconds(5000);
    printFile();
    saveInFile();
  }
}

void loop() {}

char* dataToChar() {
  String str =  " {\"id\":\"";
  str +=        id++;
  str +=        "\",";
  str +=        "\"DateHeure\": ,";
  str +=        "\"Latitude\": ,";
  str +=        "\"Longitude\": ,";
  str +=        "\"Gyro_x\":" + String(data[5]) + ",";
  str +=        "\"Gyro_y\":" + String(data[6]) + ",";
  str +=        "\"Gyro_z\":" + String(data[7]) + ",";
  str +=        "\"Accel_x\":" + String(data[2]) + ",";
  str +=        "\"Accel_y\":" + String(data[3]) + ",";
  str +=        "\"Accel_z\":" + String(data[4]) + ",";
  str +=        "\"Temperature\": " + String(data[1]) + ",";
  str +=        "\"Humidite\": " + String(data[0]) + ",";
  str +=        "},";
  char c[str.length()+1];
  str.toCharArray(c, str.length()+1);
  return c;
}

void saveInFile() {
  id++;
  line++;
  if (line == 3000) {
    file++;
    line = 0;
  }

  UINT nr;

  char buff[512];
  char* strData =  dataToChar();
  long filesize = getFileLengthBytes();

  //sprintf_P(buff, PSTR(dataToChar()));
  
  pf_lseek(0);
  do {
    pf_write(strData, 8, &nr);
    Serial.print("Wrote: ");
    Serial.println(nr);
  }while(nr == 8);
  pf_write(0, 0, &nr);
}

long getFileLengthBytes() {
  pf_lseek(0);
  long length = 0;
  while (1) {
    UINT nr;
    if (pf_read(buf, sizeof(buf), &nr)) errorHalt("pf_read");
    if (nr == 0) break;
    length += nr;
  }

  return length;
}

void printFile() {
  pf_lseek(0);
  Serial.println("------------------");
  Serial.print("Name: ");
  Serial.println("FOO.BAR");
  Serial.print("Content: ");
  long length = 0;
  while (1) {
    UINT nr;
    if (pf_read(buf, sizeof(buf), &nr)) errorHalt("pf_read");
    if (nr == 0) break;
    length += nr;
    Serial.write(buf, nr);
  }
  Serial.print("Size:");
  Serial.println(length);
  Serial.println("------------------");
}

