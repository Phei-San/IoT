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
const int rainPin = A5;
const int soilPin = A2;
int servoPin = 38;

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



void setup()
{
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  //client.setCallback(callback);

  pinMode(rainPin, INPUT);
  pinMode(soilPin, INPUT);
  servo.attach(servoPin);
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
  
  int cover = 0;
  int uncover = 120;

  client.loop();
  delay(5000); // adjust the delay according to your requirements
  int temperature = dht.readTemperature();
  int humidity = dht.readHumidity();
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);

  float rainValue = rainSensor();
  float curSoilMoist = soilSensor();

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

  delay(500);
  char payload[128];
  sprintf(payload, "Temperature: %d; Humidity: %d; Soil Moisture: %.2f", temperature, humidity, curSoilMoist);
  client.publish(MQTT_TOPIC, payload);
  
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
