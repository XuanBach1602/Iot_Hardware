#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_Sensor.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp;
SoftwareSerial arduinoSerial(D1, D2);

const char* ssid = "BDNHT";
const char* password = "0347709819";
String locality, city,latitude,longitude;


#define SERVER "broker.hivemq.com"
#define SERVERPORT 1883

#define DHTPIN 5
#define DHTTYPE DHT11

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, SERVER, SERVERPORT, "", "");
Adafruit_MQTT_Publish node = Adafruit_MQTT_Publish(&mqtt, "0981957216");

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void setup() {
  Serial.begin(4800);
  arduinoSerial.begin(4800);
  connectWiFi();
  timeClient.begin();
  timeClient.setTimeOffset(25200);
  timeClient.update();
}

void loop() {

  if (arduinoSerial.available()) {
     timeClient.update();
    String receivedData = arduinoSerial.readStringUntil('\n');
    // Tạo một đối tượng JSON để lưu trữ dữ liệu
    DynamicJsonDocument jsonDoc(400); // Đổi kích thước tùy thuộc vào độ lớn dữ liệu

    // Deserialize dữ liệu từ chuỗi JSON nhận được từ Serial
    DeserializationError error = deserializeJson(jsonDoc, receivedData);

    //Kiểm tra lỗi khi deserialize
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    time_t epochTime = timeClient.getEpochTime(); 
  String formattedTime = timeClient.getFormattedTime();
  int currentHour = timeClient.getHours();  
  int currentMinute = timeClient.getMinutes();  
  int currentSecond = timeClient.getSeconds();  
  String weekDay = weekDays[timeClient.getDay()];   
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  String currentMonthName = months[currentMonth-1];
  int currentYear = ptm->tm_year+1900;
  //Print complete date:
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);   
    jsonDoc["datetime"] = currentDate + " " + formattedTime ;

    serializeJsonPretty(jsonDoc, Serial);
    Serial.println(); // In một dòng mới
    MQTT_connect(jsonDoc);
  } 
}

void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void testConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    const char* host = "api.bigdatacloud.net";
    const int httpPort = 80;

    Serial.println("Starting request...");

    if (!client.connect(host, httpPort)) {
      Serial.println("Connection failed");
      return;
    }

    client.print(String("GET /data/reverse-geocode-client?localityLanguage=vi HTTP/1.1\r\n") +
                 "Host: " + String(host) + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Serial.println("Request sent");

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    String response = client.readString();
    // Serial.println("Response received:");
    // Serial.println(response);

    // Parse JSON response
    const size_t capacity = JSON_OBJECT_SIZE(10) + 400;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, response);

    // Extract 'locality' field from JSON
    locality = String(doc["locality"]);
    latitude = String(doc["latitude"]);
    longitude = String(doc["longitude"]);
    city = String(doc["city"]);
    Serial.print("Address: "+locality +", " + city);
   
    
    client.stop();
  }
}

void MQTT_connect(DynamicJsonDocument& jsonDoc) {
  int8_t ret;
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  Serial.print("Connecting to MQTT... ");
  if (node.publish(jsonString.c_str())) {
      // Serial.println(jsonString);
    Serial.println("Message sent to MQTT!");
  } else {
    Serial.println("Failed to send message to MQTT...");
  }
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    
    mqtt.disconnect();
    delay(5000);
    retries--;
    if (retries == 0) {
      while (1)
        ;
    }
  }
  Serial.println("MQTT Connected!");
  
}

