/*
ESP32 publish telemetry data to Google Cloud (DHT11 sensor)
*/
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <ESP32Servo.h> 
// #include "DHT11.h"

const char *WIFI_SSID = "wifi9738"; //your WiFi SSID
const char *WIFI_PASSWORD = "huanle_9738"; // your password
const char *MQTT_SERVER = "34.136.246.247"; // your VM instance public IP address
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "iot"; // MQTT topic

const int DHT_PIN = A4;
const int DHT_TYPE = DHT11;
// const int dhtPin = A4
const int rainPin = A5;
const int soilPin = A2;
int servoPin = 38;
float startSoilMoist;
int startTime;

float min_soilMoisture = (100 - 2048.0/4095*100);  
float max_soilMoisture = (100 - 1229.0/4095*100);  

// dht11 DHT;
DHT dht(DHT_PIN, DHT_TYPE);
Servo servo;
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* MQTT_TOPIC, byte* payload, unsigned int length) {
  Serial.print("Message received on MQTT_TOPIC: ");
  Serial.print(MQTT_TOPIC);
  Serial.print(". Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  pinMode(rainPin, INPUT);
  pinMode(soilPin, INPUT);
  servo.attach(servoPin);
  startSoilMoist = (100 - analogRead(soilPin)/4095*100);
  startTime = millis();
}

void reconnect()
{
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT server");
      client.subscribe(MQTT_TOPIC);
    }
    else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }

  // int readData = DHT.read(dhtPin);
  // float t_C = DHT.temperature;
  
  int cover = 0;
  int uncover = 120;

  client.loop();
  delay(5000); // adjust the delay according to your requirements
  float temperature = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.println(temperature);
  char payload[10];
  sprintf(payload, "%.2f", temperature);
  float timeToChange1;

  float rainValue = rainSensor();
  float curSoilMoist = soilSensor();
  int curTime = millis();

  //COVER THE PLANT WHEN RAINING AND THE SOIL IS TOO MOISTURE 
  //servo 120 degree - uncover the plant
  //servo 0 degree - cover the plant
  // if raining
  if(rainValue == LOW) {
    //if the soil is too dry
    if(curSoilMoist < min_soilMoisture) {
      Serial.print("Current Soil Moisture: ");
      Serial.print(curSoilMoist);
      Serial.println("%");
      servo.write(uncover); //open
    }
    //if the soil is too moisture
    else if(curSoilMoist > max_soilMoisture) {
      Serial.print("Current Soil Moisture: ");
      Serial.print(curSoilMoist);
      Serial.println("%");
      servo.write(cover); //close
    }
  }
  // no raining
  // cover the plant is the soil is too dry 
  // uncover the plant if the soil is too wet
  // else do nothing
  else {
    if (curSoilMoist > max_soilMoisture) {
      servo.write(uncover); //open
    }
    else if (curSoilMoist < min_soilMoisture) {
      servo.write(cover); //close
    }
  }

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
  //PREDICT AFTER HOW MANY MINUTES/HOURS SHOULD WATER THE PLANT
  if (curSoilMoist <= min_soilMoisture) {
    Serial.println("You should water the plant now!");
  }
  else if (min_soilMoisture <= curSoilMoist && curSoilMoist <= max_soilMoisture){
    Serial.print("Current Soil Moisture: ");
    Serial.println(curSoilMoist);
    Serial.print("Start Soil Moisture: ");
    Serial.println(startSoilMoist);

    if(startSoilMoist > curSoilMoist) {
      timeToChange1 = (curTime - startTime) / (startSoilMoist - curSoilMoist);
    }
    else {
      timeToChange1 = (curTime - startTime) / (curSoilMoist - startSoilMoist);
    }

    float durationToWater = (curSoilMoist - min_soilMoisture) * timeToChange1;
    float timeTowater = curTime + durationToWater;
    Serial.print("You should water the plant after ");
    Serial.print(timeTowater/1000.0/60/60);
    Serial.println(" hours");
  }
  else {
    startSoilMoist = curSoilMoist;
    startTime = millis();
  }

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

  delay(500);
  //client.publish(MQTT_TOPIC, payload);
}

// highest value is 4095 - no rain
// the lower the value, the heavier the rain
float rainSensor() {
  float sensorValue = digitalRead(rainPin);  // Read the analog value from sensor
  Serial.print("Rain Sensor Value: ");
  Serial.println(sensorValue);
  return sensorValue;
}

// highest value is 4095 - dry
// the lower the value, the moisture the soil
float soilSensor() {
  float sensorValue = analogRead(soilPin);  // Read the analog value from sensor
  Serial.print("Soil Sensor Value: ");
  Serial.println(100 - sensorValue/4095*100);
  return (100 - sensorValue/4095*100);
}