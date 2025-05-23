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

wifi_conf_t wifi_conf;
mqtt_conf_t mqtt_conf;
module_conf_t module_conf;

AsyncWebServer server(80);

bool shouldReboot = false;
bool hasWifiConfig = false;
bool hasMqttConfig = false;

int mqttCounter=0;
String clientId = DEFAULT_CLIENTID_PREFIX;


void setup() {
  WiFi.scanNetworks();
  setupPin();
  Serial.begin(115200);
  clientId += String(ESP.getChipId(), DEC);
  initializeServerEndpoints();
  if(loadFS()==0){
    hasWifiConfig = (loadWifiConfiguration(&wifi_conf)==0);
    if(!hasWifiConfig){
      wifi_ap_start();
    }
    else{
      if(wifi_sta_start() == 0){
        if(loadMqttConfiguration(&mqtt_conf)==0){
          hasMqttConfig = true;
          mqttSetupClient();
        }
      }
    }
  }
  setupOta();
}

void loop() {
  shouldRebootCheck();
  if(wifi_connection_loop_handler()==0&&hasMqttConfig&&!mqttConnected()&&mqttCounter!=-1){
    mqttReconnect();
  }
  mqttReconnectTimerHandler();
  mqttForceLog();
  mqttLoop();
  stateUpdate();
  loopOta();
}