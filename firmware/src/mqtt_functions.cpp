#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <PubSubClient.h>
#include "core_functions.h"
#include "mqtt_functions.h"
#include "fs_functions.h"
#include "shared.h"
#include "settings.h"
void callback(char *topic, byte *payload, unsigned int length);

gateStatus lastLoggedState=GATE_UNKNOWN;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastLoggedTime;
unsigned int mqttTimeToReconnect = 0;

extern int mqttCounter;
extern mqtt_conf_t mqtt_conf;
extern gateStatus actualState;
extern String clientId;

void mqttReconnectTimerHandler(){
    if((millis()-mqttTimeToReconnect>120000)){
        mqttTimeToReconnect=millis();
        if(mqttconf_exists())
        mqttCounter=0;
    }
}

void stateLog(bool force){
  if(force){
    if(client.connected()){
      String state = stateConversion(actualState);
      Serial.print("Forced log -> ");
      Serial.println(state);
      client.publish(STATE_TOPIC,state.c_str());
      lastLoggedState = actualState;
      lastLoggedTime = millis();
    }
  }
  else if(actualState != lastLoggedState){
    if(client.connected()){
      String state = stateConversion(actualState);
      Serial.print("Normal log -> ");
      Serial.println(state);
      client.publish(STATE_TOPIC,state.c_str());
      lastLoggedState = actualState;
      lastLoggedTime = millis();
    }
  }
}

void mqttForceLog(){
    if (millis() - lastLoggedTime > FORCE_UPDATE_TIME){
        stateLog(true);
    }
    else{
        stateLog(false);
    }
}

void mqttSetupClient(){
  Serial.println("Connessione MQTT in corso");
  client.setServer(mqtt_conf.ip, mqtt_conf.port);
  client.setCallback(callback);
  client.setKeepAlive(60);
  if(!client.connected())
    mqttReconnect();
}

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

void mqttReconnect() {
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
bool mqttConnected(){
  return client.connected();
}
void mqttLoop(){
  client.loop();
}