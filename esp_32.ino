#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "Airtel_kumm_0120";
const char* password = "Air@24885";

// MQTT Broker
const char* mqtt_server = "192.168.1.5";
const int mqtt_port = 1883;

// GPIO Configuration
const int relayPins[] = {2, 4, 5, 18};  // Relay control pins (GPIO 2 included)
const int motionSensorPin = 19;          // Motion sensor pin
const int relayCount = 4;

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT Topics
const char* statusTopic = "home/relays/status";
const char* motionTopic = "home/motion";
const char* controlTopics[] = {
  "home/relay1/set",
  "home/relay2/set",
  "home/relay3/set",
  "home/relay4/set"
};

// Variables
bool lastMotionState = false;
unsigned long lastMsgTime = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Handle relay control commands
  for (int i = 0; i < relayCount; i++) {
    if (String(topic) == controlTopics[i]) {
      if (message == "ON") {
        digitalWrite(relayPins[i], HIGH);
        publishStatus();
      } else if (message == "OFF") {
        digitalWrite(relayPins[i], LOW);
        publishStatus();
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32RelayController")) {
      Serial.println("connected");
      // Subscribe to control topics
      for (int i = 0; i < relayCount; i++) {
        client.subscribe(controlTopics[i]);
      }
      publishStatus();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void publishStatus() {
  String statusMessage = "";
  for (int i = 0; i < relayCount; i++) {
    statusMessage += digitalRead(relayPins[i]) ? "ON" : "OFF";
    if (i < relayCount - 1) statusMessage += ",";
  }
  client.publish(statusTopic, statusMessage.c_str());
}

void setup() {
  Serial.begin(115200);
  
  // Initialize relays
  for (int i = 0; i < relayCount; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);  // Start with relays off
  }
  
  // Initialize motion sensor
  pinMode(motionSensorPin, INPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Check motion sensor every 500ms
  if (millis() - lastMsgTime > 500) {
    bool currentMotionState = digitalRead(motionSensorPin);
    
    if (currentMotionState != lastMotionState) {
      client.publish(motionTopic, currentMotionState ? "MOTION_DETECTED" : "NO_MOTION");
      lastMotionState = currentMotionState;
    }
    
    lastMsgTime = millis();
  }
}