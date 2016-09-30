#include "arduino.h"

// Needed for SPIFFS
#include <FS.h>

// Wireless / Networking
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266WiFi.h>

// static const uint8_t D0   = 16;
// static const uint8_t D1   = 5;
// static const uint8_t D2   = 4;
// static const uint8_t D3   = 0;
// static const uint8_t D4   = 2;
// static const uint8_t D5   = 14;
// static const uint8_t D6   = 12;
// static const uint8_t D7   = 13;
// static const uint8_t D8   = 15;
// static const uint8_t D9   = 3;
// static const uint8_t D10  = 1;

// PIN used to reset configuration.  Enables internal Pull Up.  Ground to reset.
#define PIN_RESET 13 // Labeled D7 on ESP12E DEVKIT V2
#define RESET_DURATION 30

// MQTT
#include <AsyncMqttClient.h>
AsyncMqttClient mqttClient;
char mqtt_server[40];
uint mqtt_port;
char mqtt_user[20];
char mqtt_pass[20];

// Name for this ESP
char node_name[20];
char topic_status[50];
char topic_control[50];

/* ========================================================================================================
                                           __
                              ______ _____/  |_ __ ________
                             /  ___// __ \   __\  |  \____ \
                             \___ \\  ___/|  | |  |  /  |_> >
                            /____  >\___  >__| |____/|   __/
                                 \/     \/           |__|
   ======================================================================================================== */

bool shouldSaveConfig = false;
void saveConfigCallback()
{
  shouldSaveConfig = true;
}

void saveSetting(const char* key, char* value) {
  char filename[80] = "/config_";
  strcat(filename, key);

  File f = SPIFFS.open(filename, "w");
  if (f) {
    f.print(value);
  }
  f.close();
}

String readSetting(const char* key) {
  char filename[80] = "/config_";
  strcat(filename, key);

  String output;

  File f = SPIFFS.open(filename, "r");
  if (f) {
    output = f.readString();
  }
  f.close();
  return output;
}

void setup() {

  Serial.begin(115200);

  SPIFFS.begin();

  WiFiManager wifiManager;

  // short pause on startup to look for settings RESET
  Serial.println("Waiting for reset");
  pinMode(PIN_RESET, INPUT_PULLUP);
  bool reset = false;
  int resetTimeRemaining = RESET_DURATION;
  while (!reset && resetTimeRemaining-- > 0) {
    if (digitalRead(PIN_RESET) == 0) {
      reset = true;
    }
    Serial.print(".");
    delay(100);
  }
  Serial.println("");
  if (reset) {
    Serial.println("Resetting");
    wifiManager.resetSettings();
  }

  // add bonus parameters to WifiManager
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "", 40);
  wifiManager.addParameter(&custom_mqtt_server);

  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", "1338", 6);
  wifiManager.addParameter(&custom_mqtt_port);

  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", "", 20);
  wifiManager.addParameter(&custom_mqtt_user);

  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", "", 20);
  wifiManager.addParameter(&custom_mqtt_pass);

  WiFiManagerParameter custom_node_name("nodename", "node name", "rename_me", sizeof(node_name));
  wifiManager.addParameter(&custom_node_name);

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point
  wifiManager.autoConnect();

  //print out obtained IP address
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  if (shouldSaveConfig) {
    Serial.println("Saving configuration...");
    saveSetting("mqtt_server", (char *) custom_mqtt_server.getValue());
    saveSetting("mqtt_port", (char *) custom_mqtt_port.getValue());
    saveSetting("mqtt_user", (char *) custom_mqtt_user.getValue());
    saveSetting("mqtt_pass", (char *) custom_mqtt_pass.getValue());
    saveSetting("node_name", (char *) custom_node_name.getValue());
  }

  // read settings from configuration
  readSetting("mqtt_server").toCharArray(mqtt_server, sizeof(mqtt_server));
  mqtt_port = readSetting("mqtt_port").toInt();
  readSetting("mqtt_user").toCharArray(mqtt_user, sizeof(mqtt_user));
  readSetting("mqtt_pass").toCharArray(mqtt_pass, sizeof(mqtt_pass));
  readSetting("node_name").toCharArray(node_name, sizeof(node_name));

  // read ESP name from configuration
  Serial.print("Node Name: ");
  Serial.println(node_name);

  strcat(topic_status, "esp/");
  strcat(topic_status, node_name);
  strcat(topic_status, "/status");

  strcat(topic_control, "esp/");
  strcat(topic_control, node_name);
  strcat(topic_control, "/control");

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  mqttClient.setServer(mqtt_server, mqtt_port);

  mqttClient.setKeepAlive(5);

  mqttClient.setWill(topic_status, 2, true, "offline");

  if (strlen(mqtt_user) > 0) {
    mqttClient.setCredentials(mqtt_user, mqtt_pass);
  }

  mqttClient.setClientId(node_name);

  Serial.println("Connecting to MQTT...");
  mqttClient.connect();

}

/* ========================================================================================================
               _____   ______________________________ ___________                    __
              /     \  \_____  \__    ___/\__    ___/ \_   _____/__  __ ____   _____/  |_  ______
             /  \ /  \  /  / \  \|    |     |    |     |    __)_\  \/ // __ \ /    \   __\/  ___/
            /    Y    \/   \_/.  \    |     |    |     |        \\   /\  ___/|   |  \  |  \___ \
            \____|__  /\_____\ \_/____|     |____|    /_______  / \_/  \___  >___|  /__| /____  >
                    \/        \__>                            \/           \/     \/          \/
   ======================================================================================================== */

uint16_t controlSubscribePacketId;

void onMqttConnect() {
  Serial.println("** Connected to the broker **");
  // subscribe to the control topic
  controlSubscribePacketId = mqttClient.subscribe(topic_control, 2);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("** Subscribe acknowledged **");
  // once successfully subscribed to control, public online status
  if (packetId == controlSubscribePacketId) {
    mqttClient.publish(topic_status, 2, true, "online");
  }
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("** Unsubscribe acknowledged **");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("** Disconnected from the broker **");
  Serial.println("Reconnecting to MQTT...");
  mqttClient.connect();
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("** Publish received **");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  payload: ");
  Serial.println(payload);
}

void onMqttPublish(uint16_t packetId) {
}

/* ========================================================================================================
                         _____         .__         .____
                        /     \ _____  |__| ____   |    |    ____   ____ ______
                       /  \ /  \\__  \ |  |/    \  |    |   /  _ \ /  _ \\____ \
                      /    Y    \/ __ \|  |   |  \ |    |__(  <_> |  <_> )  |_> >
                      \____|__  (____  /__|___|  / |_______ \____/ \____/|   __/
                              \/     \/        \/          \/            |__|
   ======================================================================================================== */

void loop() {
  // sample publish
  // uint16_t packedId = mqttClient.publish("test/lol", 0, true, "test 1");
}
