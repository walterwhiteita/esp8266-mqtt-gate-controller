#include <Arduino.h>
#include "core_functions.h"
#include "settings.h"
#include "shared.h"

static const int ACIn1 = 14;
static const int ACIn2 = 12;
static const int ACIn3 = 13;

static const int Rele1 = 5;
static const int Rele2 = 4;
static const int Rele3 = 0;

gateStatus actualState;
gateStatus lastReadedState;
gateStatus lastLoggedState=GATE_UNKNOWN;

unsigned int statusCount=0;
unsigned long lastLoggedTime;

void relayClick(int pin){
  digitalWrite(pin,HIGH);
  delay(CLICK_TIME);
  digitalWrite(pin,LOW);
}

void handleCmnd(gateCmnd cmnd){
  switch(cmnd){
    case CMND_OPEN:
      relayClick(Rele1);
    case CMND_CLOSE:
      relayClick(Rele2);
    case CMND_STOP:
      relayClick(Rele3);  
  }
}
void stateUpdate(){
  if(!digitalRead(ACIn2)){
    if(lastReadedState == GATE_OPENING){
      statusCount++;
    }
    else{
      statusCount=1;
      lastReadedState = GATE_OPENING;
      }
    if(statusCount >  CORRECT_NUM_OF_LECTURES){
      actualState = GATE_OPENING;
    } 
    
  }
  else if(!digitalRead(ACIn3)){
    if(lastReadedState == GATE_CLOSING){
      statusCount++;
    }
    else{
      statusCount=1;
      lastReadedState = GATE_CLOSING;
      }
    if(statusCount >  CORRECT_NUM_OF_LECTURES){
      actualState = GATE_CLOSING;
    } 
  }
  else if(!digitalRead(ACIn1)){
    if(lastReadedState == GATE_OPEN){
      statusCount++;
    }
    else{
      statusCount=1;
      lastReadedState = GATE_OPEN;
      }
    if(statusCount >  CORRECT_NUM_OF_LECTURES){
      actualState = GATE_OPEN;
    }
  }
  else{
    if(lastReadedState == GATE_CLOSED){
      statusCount++;
    }
    else{
      statusCount=1;
      lastReadedState = GATE_CLOSED;
      }
    if(statusCount >  CORRECT_NUM_OF_LECTURES){
      actualState = GATE_CLOSED;
    }
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

String stateConversion(gateStatus input){
  switch(input){
    case GATE_UNKNOWN:
    return "unknown";
    case GATE_OPENING:
    return "opening";
    case GATE_CLOSING:
    return "closing";
    case GATE_OPEN:
    return "open";
    case GATE_CLOSED:
    return "closed";
    default :
    return "none";
  }
}
void setupPin(){
    pinMode(ACIn1,INPUT_PULLUP);
    pinMode(ACIn2,INPUT_PULLUP);
    pinMode(ACIn3,INPUT_PULLUP);

    pinMode(Rele1,OUTPUT);
    pinMode(Rele2,OUTPUT);
    pinMode(Rele3,OUTPUT);
}