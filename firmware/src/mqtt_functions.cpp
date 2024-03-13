#include <Arduino.h>
#include "fs_functions.h"
#include "shared.h"
#include "settings.h"

extern gateStatus actualState;
gateStatus lastLoggedState=GATE_UNKNOWN;

unsigned long lastLoggedTime;

extern int mqttCounter;
unsigned int mqttTimeToReconnect = 0;
void mqttReconnectTimerHandler(){
    if((millis()-mqttTimeToReconnect>120000)){
        mqttTimeToReconnect=millis();
        if(mqttconf_exists())
        mqttCounter=0;
    }
}

void stateLog(bool force){
  if(force){
    /*if(client.connected()){
      String state = stateConversion(actualState);
      Serial.print("Forced log -> ");
      Serial.println(state);
      client.publish(STATE_TOPIC,state.c_str());
      lastLoggedState = actualState;
      lastLoggedTime = millis();
    }*/
  }
  else if(actualState != lastLoggedState){
    /*if(client.connected()){
      String state = stateConversion(actualState);
      Serial.print("Normal log -> ");
      Serial.println(state);
      client.publish(STATE_TOPIC,state.c_str());
      lastLoggedState = actualState;
      lastLoggedTime = millis();
    }*/
  }
}

void mqttForceLog(){
    if (millis() - lastLoggedTime > FORCE_UPDATE_TIME)
    {
        stateLog(true);
    }
    else
    {
        stateLog(false);
    }
}