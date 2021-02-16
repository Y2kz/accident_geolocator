#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include<Wire.h>


SoftwareSerial myGps(3, 2);
TinyGPSPlus gps;
bool getDataFlag = false;
String content;

String latitude, longitude;

const int MPU6050_addr = 0x68;
int16_t AccX, AccY, AccZ, Temp, GyroX, GyroY, GyroZ;
float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;

boolean fall = false; //stores if a fall has occurred
boolean trigger1 = false; //stores if first trigger (lower threshold) has occurred
boolean trigger2 = false; //stores if second trigger (upper threshold) has occurred
boolean trigger3 = false; //stores if third trigger (orientation change) has occurred

byte trigger1count = 0; //stores the counts past since trigger 1 was set true
byte trigger2count = 0; //stores the counts past since trigger 2 was set true
byte trigger3count = 0; //stores the counts past since trigger 3 was set true
int angleChange = 0;
bool smsFlag = false;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.begin(9600);
  delay(100);

  delay(1000);
  myGps.begin(9600);
  delay(100);

  Serial.println("init");
  //sms("+91xxxxxxxxxx"); // For debugging 
}

void loop() {


  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_addr, 14, true);
  AccX = Wire.read() << 8 | Wire.read();
  AccY = Wire.read() << 8 | Wire.read();
  AccZ = Wire.read() << 8 | Wire.read();
  Temp = Wire.read() << 8 | Wire.read();
  GyroX = Wire.read() << 8 | Wire.read();
  GyroY = Wire.read() << 8 | Wire.read();
  GyroZ = Wire.read() << 8 | Wire.read();

  ax = (AccX - 2050) / 16384.00;
  ay = (AccY - 77) / 16384.00;
  az = (AccZ - 1947) / 16384.00;

  //270, 351, 136 for gyroscope
  gx = (GyroX + 270) / 131.07;
  gy = (GyroY - 351) / 131.07;
  gz = (GyroZ + 136) / 131.07;

  float Raw_AM = pow(pow(ax, 2) + pow(ay, 2) + pow(az, 2), 0.5);
  int AM = Raw_AM * 10;  // as values are within 0 to 1, I multiplied

  Serial.println(AM);
  if (trigger3 == true) {
    trigger3count++;
    //Serial.println(trigger3count);
    if (trigger3count >= 10) {
      angleChange = pow(pow(gx, 2) + pow(gy, 2) + pow(gz, 2), 0.5);
      //delay(10);
      Serial.println(angleChange);
      if ((angleChange >= 0) && (angleChange <= 10)) { //if orientation changes remains between 0-10 degrees
        fall = true; trigger3 = false; trigger3count = 0;
        Serial.println(angleChange);
      }
      else { //user regained normal orientation
        trigger3 = false; trigger3count = 0;
        Serial.println("TRIGGER 3 DEACTIVATED");
      }
    }
  }
  if (fall == true) { //in event of a fall detection
    Serial.println("FALL DETECTED");

    // exit(1);
  }
  if (trigger2count >= 6) { //allow 0.5s for orientation change
    trigger2 = false; trigger2count = 0;
    Serial.println("TRIGGER 2 DECACTIVATED");
  }
  if (trigger1count >= 6) { //allow 0.5s for AM to break upper threshold
    trigger1 = false; trigger1count = 0;
    Serial.println("TRIGGER 1 DECACTIVATED");
  }
  if (trigger2 == true) {
    trigger2count++;
    //angleChange=acos(((double)x*(double)bx+(double)y*(double)by+(double)z*(double)bz)/(double)AM/(double)BM);
    angleChange = pow(pow(gx, 2) + pow(gy, 2) + pow(gz, 2), 0.5); Serial.println(angleChange);
    if (angleChange >= 30 && angleChange <= 400) { //if orientation changes by between 80-100 degrees
      trigger3 = true; trigger2 = false; trigger2count = 0;
      Serial.println(angleChange);
      Serial.println("TRIGGER 3 ACTIVATED");
    }
  }
  if (trigger1 == true) {
    trigger1count++;
    if (AM >= 12) { //if AM breaks upper threshold (3g)
      trigger2 = true;
      Serial.println("TRIGGER 2 ACTIVATED");
      trigger1 = false; trigger1count = 0;
    }
  }
  if (AM <= 2 && trigger2 == false) { //if AM breaks lower threshold (0.4g)
    trigger1 = true;
    Serial.println("TRIGGER 1 ACTIVATED");
  }
  //It appears that delay is needed in order not to clog the port
  delay(100);
  if (getDataFlag) {
    delay(500);
    sms("+91xxxxxxxxxx"); //add your number here
    getDataFlag = false;
  }
  if (fall) {
    getGps();
  }

}
void sms(String number) {
  Serial.println("Sending sms...");
  Serial.println("AT"); //Once the handshake test is successful, it will back to OK
  // updateSerial();
  delay(500);
  Serial.println("AT+CMGF=1\r\n"); // Configuring TEXT mode
  //updateSerial();
  delay(500);
  Serial.println("AT+CMGS=\"" + number + "\"\r\n"); //change ZZ with country code and xxxxxxxxxxx with phone number to sms
  //updateSerial();
  delay(500);

  Serial.print(content); //text content
  //updateSerial();
  delay(500);
  Serial.write(26);
  // updateSerial();
  delay(500);
}


void getGps()
{

  // This sketch displays information every time a new sentence is correctly encoded.
  while (myGps.available() > 0)
    if (gps.encode(myGps.read()))
      displayInfo();
}

void displayInfo()
{
  if (gps.location.isValid())
  {
    //Serial.print("Latitude: ");
    //Serial.println(gps.location.lat(), 6);

    latitude = String(gps.location.lat(), 6);
    //Serial.print("Longitude: ");
    //Serial.println(gps.location.lng(), 6);
    longitude = String(gps.location.lng(), 6);
    content = "Accident detected at location " + latitude + " , " + longitude;
    Serial.println(content);
    getDataFlag = true;
    fall = false;
  }
  else
  {
    Serial.println("Location: Not Available");
    getDataFlag = false;
  }
  delay(100);
}
