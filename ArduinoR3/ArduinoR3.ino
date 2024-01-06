#include <SoftwareSerial.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int MQ7_PIN = A0;
const int MQ135_PIN = A1;

const int GP2Y10_PIN = A5;

SoftwareSerial espSerial(10, 11);
StaticJsonDocument<200> jsonDoc;

//GP2y10 config
int measurePin = A5;
int ledPower = 8;

unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;


void setup() {
  Serial.begin(9600);
  dht.begin();
  espSerial.begin(4800);
  pinMode(ledPower,OUTPUT);
  // gpsSerial.begin(9600);
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  int mq7Value = analogRead(MQ7_PIN); 
  float coConcentration = map(mq7Value, 0, 1023, 0, 100);

  int mq135Value = analogRead(MQ135_PIN);
  float nh3Concentration = map(mq135Value, 0, 1023, 10, 300);
  float co2Concentration = map(mq135Value, 0, 1023, 0, 5000);

    int gp2y10Value = analogRead(GP2Y10_PIN); // Đọc giá trị từ cảm biến GP2Y10
    Serial.println(gp2y10Value);

  jsonDoc["humidity"] = humidity;
  jsonDoc["temperature"] = temperature;
  jsonDoc["CO_concentration"] = coConcentration;
  jsonDoc["NH3_concentration"] = nh3Concentration;
  jsonDoc["CO2_concentration"] = co2Concentration;
  jsonDoc["locationId"] = 1;

  //Gp2y10
  digitalWrite(ledPower,LOW);
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin);

  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH);
  delayMicroseconds(sleepTime);

  calcVoltage = voMeasured*(5.0/1024);
  dustDensity = 0.17*calcVoltage-0.1;

  if ( dustDensity < 0)
  {
    dustDensity = 0.00;
  }
  Serial.println(dustDensity);
  jsonDoc["PM25_concentration"] = dustDensity;


  // if (Serial.available()) {
  //   while (gpsSerial.available() > 0) {
  //     byte c = gpsSerial.read();
  //     Serial.write(c);
  //   }
  //   Serial.println();
  //   Serial.println();
  // }

  // Gửi dữ liệu JSON qua Serial
  serializeJson(jsonDoc, espSerial);
  serializeJson(jsonDoc,Serial);
  espSerial.println();
  Serial.println();

  delay(10000);
}
