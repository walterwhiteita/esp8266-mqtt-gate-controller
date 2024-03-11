#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "wifi_functions.h"

extern wifi_conf_t wifi_conf;

void wifi_sta_ap_start(){
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("hagate","password");
  Serial.println("Access Point created while trying to connect to fixed network! IP address: ");
  Serial.println(WiFi.softAPIP());
}

void wifi_reconnection(){
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(wifi_conf.ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.mode(WIFI_STA);
}

void wifi_ap_start(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP("hagate","password");
  Serial.println("Access Point created! IP address: ");
  Serial.println(WiFi.softAPIP());
}

int wifi_sta_start(){
  WiFi.mode(WIFI_STA);
  WiFi.hostname(wifi_conf.client_name);
  WiFi.begin(wifi_conf.ssid, wifi_conf.pw);
  Serial.println("");
  int counter=0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if(counter>50){
      counter=-1;
      break;
    }
  }
  if(counter==-1){
    Serial.println("Wifi not connected. Starting AP mode");
    wifi_sta_ap_start();
    return -1;
  }
  else{
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(wifi_conf.ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  return 0;  
}

int wifi_connection_loop_handler(){
  if(WiFi.getMode()==SOFTAP_MODE){
    return 2;
  }
  else if (WiFi.getMode()==STATIONAP_MODE){
    wifi_reconnection();
    return 0;
  }
  else if(WiFi.status()!=WL_CONNECTED){
    wifi_sta_ap_start();
    return 1;
  }
  else {
    return 0;
  }
}