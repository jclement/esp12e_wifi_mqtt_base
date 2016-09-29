#include "arduino.h"

// Needed for SPIFFS
#include <FS.h>

// Wireless / Networking
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266WiFi.h>

// MQTT
#include <AsyncMqttClient.h>
AsyncMqttClient mqttClient;

// Name for this ESP
char* espName;

/* ========================================================================================================
                                           __
                              ______ _____/  |_ __ ________
                             /  ___// __ \   __\  |  \____ \
                             \___ \\  ___/|  | |  |  /  |_> >
                            /____  >\___  >__| |____/|   __/
                                 \/     \/           |__|
   ======================================================================================================== */

void setup() {

    SPIFFS.begin();
    Serial.begin(115200);

    WiFiManager wifiManager;

    // uncomment the following line to reset WIFI settings
    //wifiManager.resetSettings();

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point
    wifiManager.autoConnect();

    //print out obtained IP address
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());

    // read ESP name from configuration
    Serial.print("ESP Name: ");
    Serial.println(espName);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);

    //mqttClient.setServer(IPAddress(10,0,0,100), 1883);
    mqttClient.setServer("mqtthost", 1883);

    mqttClient
      .setKeepAlive(5)
      //.setWill("topic/online", 2, true, "no")
      //.setCredentials("user", "password")
      .setClientId(espName);

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

void onMqttConnect() {
  Serial.println("** Connected to the broker **");

  // subscriptions go here
  // uint16_t packedId = mqttClient.subscribe("test/something", 2);

}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("** Subscribe acknowledged **");
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
