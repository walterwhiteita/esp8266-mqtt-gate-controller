#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <PubSubClient.h>

#include "fs_functions.h"
#include "html_pages.h"
#include "wifi_functions.h"
#include "settings.h"
#include "core_functions.h"
#include "server_functions.h"
#include "mqtt_functions.h"
#include "shared.h"

// Update these with values suitable for your network.

const char* ssid = "VF_IT_FWA_DF33";
const char* password = "8iD3965d4284RYA5";
const char* mqtt_server = "broker.hivemq.com";

wifi_conf_t wifi_conf;
mqtt_conf_t mqtt_conf;
module_conf_t module_conf;

AsyncWebServer server(80);


WiFiClient espClient;
PubSubClient client(espClient);

bool shouldReboot = false;
bool hasWifiConfig = false;
bool hasMqttConfig = false;

int mqttCounter=0;
String clientId = DEFAULT_CLIENTID_PREFIX;


void callback(char *topic, byte *payload, unsigned int length) {
    if (!strcmp(topic, COMMAND_TOPIC)) {
        if (!strncmp((char *)payload, "OPEN", length)) {
          handleCmnd(CMND_OPEN);
        } else if (!strncmp((char *)payload, "CLOSE", length)) {
          handleCmnd(CMND_CLOSE);
        }else if(!strncmp((char *)payload, "STOP", length)){
          handleCmnd(CMND_STOP);
        }
    }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId.c_str(),mqtt_conf.username,mqtt_conf.password,AVAILABILITY_TOPIC,0,1,"offline")) {
      Serial.println("connected");
      client.subscribe(COMMAND_TOPIC);
      client.publish(AVAILABILITY_TOPIC, "online", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      if(++mqttCounter>5){
        Serial.println("Stop tentativi di riconnessione MQTT");
        mqttCounter=-1;
        break;
      }
    }
  }
}



void setup() {
  setupPin();
  Serial.begin(115200);
  clientId += String(ESP.getChipId(), DEC);
  int result=loadFS();
  result=loadWifiConfiguration(&wifi_conf);
  hasWifiConfig = (result==0);
  hasMqttConfig = mqttconf_exists();
  if(result!=0){
    wifi_ap_start();
  }
  else{
    if(wifi_sta_start() == 0){
      if(loadMqttConfiguration(&mqtt_conf)==0){
        Serial.println("Connessione MQTT in corso");
        client.setServer(mqtt_conf.ip, mqtt_conf.port);
        client.setCallback(callback);
        client.setKeepAlive(60);
        if(!client.connected())
          reconnect();
      }
    }
  }
  initializeServerEndpoints();
  setupOta();

}

void loop() {
  /*if (!client.connected()) {
    reconnect();
  }
  client.loop();*/
  //Leggo i sensori
  shouldRebootCheck();
  if(wifi_connection_loop_handler()==0&&hasMqttConfig&&!client.connected()&&mqttCounter!=-1){
    reconnect();
  }
  
  mqttReconnectTimerHandler();
  mqttForceLog();
    //reconnect();
  //client.loop();
  stateUpdate();
  //Controllo se loggare su MQTT
  
  loopOta();

}