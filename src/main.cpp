#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

// --- CONFIGURATION ---
#define PULSE_PIN D6
#define EEPROM_ADDR 0

char mqtt_server[40] = "YOUR_MQTT_BROKER";
char mqtt_port[6] = "1883";
char mqtt_base_topic[64] = "water"; // Default base topic
char initial_meter_water_str[10] = "123.456"; // Default as string

// --- GLOBALS ---
volatile unsigned long pulse_count = 0;
unsigned long last_pulse_count = 0;
unsigned long last_publish = 0;
unsigned long last_minute = 0;
int measure_water = 0; // L/min
float meter_water = 0.0; // m³

volatile unsigned long last_pulse_time = 0; // For debounce

WiFiClient espClient;
PubSubClient client(espClient);

// --- PULSE ISR with debounce ---
void IRAM_ATTR pulseISR() {
  unsigned long now = micros();
  if (now - last_pulse_time > 50000) { // 50ms debounce (50000us)
    pulse_count++;
    last_pulse_time = now;
  }
}

// --- EEPROM ---
void saveMeterWater(float value) {
  EEPROM.begin(8);
  EEPROM.put(EEPROM_ADDR, value);
  EEPROM.commit();
  EEPROM.end();
}

float loadMeterWater(float initial_value) {
  float value;
  EEPROM.begin(8);
  EEPROM.get(EEPROM_ADDR, value);
  EEPROM.end();
  if (isnan(value) || value < 0.0) return initial_value;
  return value;
}

// --- MQTT CALLBACK ---
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  String set_topic = String(mqtt_base_topic) + "/set";
  if (String(topic) == set_topic) {
    float new_val = msg.toFloat();
    if (new_val >= 0.0) {
      meter_water = new_val;
      saveMeterWater(meter_water);
    }
  }
}

// --- MQTT RECONNECT ---
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP12F_PulseReader")) {
      String set_topic = String(mqtt_base_topic) + "/set";
      client.subscribe(set_topic.c_str());
    } else {
      delay(5000);
    }
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  pinMode(PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), pulseISR, FALLING);

  // WiFiManager for AP config and custom params
  WiFiManager wifiManager;

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Broker", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_base("basetopic", "MQTT Base Topic", mqtt_base_topic, 64);
  WiFiManagerParameter custom_initial_meter("initmeter", "Initial Meter (m3)", initial_meter_water_str, 10);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_base);
  wifiManager.addParameter(&custom_initial_meter);

  wifiManager.autoConnect("ESP12F-Setup");

  strncpy(mqtt_server, custom_mqtt_server.getValue(), sizeof(mqtt_server));
  strncpy(mqtt_port, custom_mqtt_port.getValue(), sizeof(mqtt_port));
  strncpy(mqtt_base_topic, custom_mqtt_base.getValue(), sizeof(mqtt_base_topic));
  strncpy(initial_meter_water_str, custom_initial_meter.getValue(), sizeof(initial_meter_water_str));

  float initial_meter_water = atof(initial_meter_water_str);

  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);

  meter_water = loadMeterWater(initial_meter_water);

  // --- OTA SETUP ---
  ArduinoOTA.setHostname("esp12f-pulse-reader");
  ArduinoOTA.begin();
}

// --- LOOP ---
void loop() {
  ArduinoOTA.handle(); // <-- Add this line for OTA to work

  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - last_minute >= 60000) { // Every minute
    noInterrupts();
    unsigned long pulses = pulse_count - last_pulse_count;
    last_pulse_count = pulse_count;
    interrupts();

    measure_water = pulses * 10; // Liters per minute
    meter_water += (float)(pulses * 10) / 1000.0; // Convert to m³ (1000L = 1m³)
    meter_water = roundf(meter_water * 1000) / 1000.0; // 3 decimals
    saveMeterWater(meter_water);

    // Add uptime in seconds
    unsigned long uptime = millis() / 1000;

    // Publish JSON
    String payload = "{\"measure_water\":" + String(measure_water) +
                     ",\"meter_water\":" + String(meter_water, 3) +
                     ",\"uptime\":" + String(uptime) + "}";

    String state_topic = String(mqtt_base_topic) + "/state";
    client.publish(state_topic.c_str(), payload.c_str());

    last_minute += 60000; // This keeps the interval exact
  }
}
