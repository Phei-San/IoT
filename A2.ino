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
const char *MQTT_SERVER = "35.225.222.172"; // your VM instance public IP address
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "iot"; // MQTT topic

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

// input sensor
DHT dht(DHT_PIN, DHT_TYPE);
Servo servo;

//Create an instance 
PubSubClient client(espClient);
WiFiClient espClient;

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

  client.loop();
  delay(5000); // adjust the delay according to your requirements
  int temperature = dht.readTemperature();
  int humidity = dht.readHumidity();
  Serial.println("----------------------------------------------");
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);

  float rainValue = rainSensor();
  float curSoilMoist = soilSensor();

  //COVER THE PLANT WHEN RAINING AND THE SOIL IS TOO MOISTURE 
  // if raining
    // cover the plant is the soil is too moisture 
    // uncover the plant if the soil is too dry
    // else do nothing
  if(rainValue == HIGH) {
    Serial.println("Raining: 1");
    //if the soil is too dry
    if(curSoilMoist < min_soilMoisture) {
      servo.write(uncover); //open
    }
    //if the soil is too moisture
    else if(curSoilMoist > max_soilMoisture) {
      servo.write(cover); //close
    }
  }
  // no raining
  // cover the plant is the soil is too dry 
  // uncover the plant if the soil is too wet
  // else do nothing
  else {
    Serial.println("Raining: 0");
    if (curSoilMoist > max_soilMoisture) {
      servo.write(uncover); //open
    }
    else if (curSoilMoist < min_soilMoisture) {
      servo.write(cover); //close
    }
  }

  delay(500);
  //publish data
  char payload[128];
  sprintf(payload, "Temperature: %d; Humidity: %d; Soil Moisture: %.2f; Rain: %.2f", temperature, humidity, curSoilMoist, rainValue);
  client.publish(MQTT_TOPIC, payload);
  
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
