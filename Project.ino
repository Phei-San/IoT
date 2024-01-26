/*
  ESP32 publish telemetry data to VOne Cloud (Agriculture)
*/

#include "VOneMqttClient.h"
#include "DHT.h"
#include <ESP32Servo.h> 

//define device id
const char* DHT11Sensor = "77ff2a45-62d1-4ac4-ac75-99f0f9dcb61e";     //Replace this with YOUR deviceID for the DHT11 sensor
const char* RainSensor = "4ddd7511-cdfb-40a3-b158-3117ad9d94c4";      //Replace this with YOUR deviceID for the rain sensor
const char* MoistureSensor = "4fd62586-8498-40c3-9ee9-3fd58425a5e1";  //Replace this with YOUR deviceID for the moisture sensor
const char* AllSensor = "fe5ba358-13e7-45b0-9720-7974d63346fb";

//Used Pins
const int DHT_PIN = A4;
const int DHT_TYPE = DHT11;
const int rainPin = A5;
const int soilPin = A2;
int servoPin = 38;

float min_soilMoisture = (100 - 3276.0/4095*100);  //min mositure value is 20%
float max_soilMoisture = (100 - 2867.0/4095*100);  //max moisture value is 30%
int cover = 0;
int uncover = 120;

//input sensor
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

//Create an instance of VOneMqttClient
VOneMqttClient voneClient;
Servo servo;

//last message time
unsigned long lastMsgTime = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {

  setup_wifi();
  voneClient.setup();

  //sensor
  dht.begin();
  pinMode(rainPin, INPUT);
  pinMode(soilPin, INPUT);
  servo.attach(servoPin);
}

void loop() {

  if (!voneClient.connected()) {
    voneClient.reconnect();
    String errorMsg = "DHTSensor Fail";
    voneClient.publishDeviceStatusEvent(DHT11Sensor, true);
    voneClient.publishDeviceStatusEvent(RainSensor, true);
    voneClient.publishDeviceStatusEvent(MoistureSensor, true);
    voneClient.publishDeviceStatusEvent(AllSensor, true);
  }
  voneClient.loop();

  unsigned long cur = millis();
  if (cur - lastMsgTime > INTERVAL) {
    lastMsgTime = cur;

    //Read sensor data
    float h = dht.readHumidity();
    int t = dht.readTemperature();
    int raining = !digitalRead(rainPin);
    int moistValue = soilSensor();

    // if raining
    // cover the plant is the soil is too moisture 
    // uncover the plant if the soil is too dry
    // else do nothing
    if(raining == HIGH) {
      Serial.println("Raining: 1");
      //if the soil is too dry
      if(moistValue < min_soilMoisture) {
        servo.write(uncover); //open
      }
      //if the soil is too moisture
      else if(moistValue > max_soilMoisture) {
        servo.write(cover); //close
      }
    }
    // no raining
    // cover the plant is the soil is too dry 
    // uncover the plant if the soil is too moisture
    // else do nothing
    else {
      Serial.println("Raining: 0");
      if (moistValue > max_soilMoisture) {
        servo.write(uncover); //open
      }
      else if (moistValue < min_soilMoisture) {
        servo.write(cover); //close
      }
    }

    //Publish telemetry data 1
    JSONVar payloadObject, payloadObject2;
    payloadObject["Humidity"] = h;
    payloadObject["Temperature"] = t;
    voneClient.publishTelemetryData(DHT11Sensor, payloadObject);

    //Sample sensor fail message
    //String errorMsg = "DHTSensor Fail";
    //voneClient.publishDeviceStatusEvent(DHT22Sensor, false, errorMsg.c_str());

    //Publish telemetry data 2
    payloadObject["Raining"] = raining;
    voneClient.publishTelemetryData(RainSensor, "Raining", raining);

    //Publish telemetry data 3
    //Moisture = map(moistValue, MinMoistureValue, MaxMoistureValue, MinMoisture, MaxMoisture);
    payloadObject["Soil moisture"] = moistValue;
    voneClient.publishTelemetryData(MoistureSensor, "Soil moisture", moistValue);

    payloadObject2["Humidity"] = h;
    payloadObject2["Temperature"] = t;
    payloadObject2["Raining"] = raining;
    payloadObject2["Moisture"] = moistValue;
    voneClient.publishTelemetryData(AllSensor, payloadObject2);
    }
}

// highest analog value is 4095 - no rain
int rainSensor() {
  float sensorValue = analogRead(rainPin);
  int raining = 1;
  int sunny = 0;

  if (sensorValue < 4000) {
    return raining;
  }
  else {
    return sunny;
  }
}

// highest analog value is 4095 - dry
float soilSensor() {
  float sensorValue = analogRead(soilPin);  // Read the analog value from sensor
  float soilMoisture = 100 - sensorValue/4095*100;
  Serial.print("Current Soil Moisture: ");
  Serial.println(soilMoisture);
  return soilMoisture;
}
