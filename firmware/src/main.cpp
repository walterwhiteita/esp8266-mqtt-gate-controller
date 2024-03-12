#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <PubSubClient.h>

#include "fs_functions.h"
#include "html_pages.h"
#include "wifi_functions.h"
#include "settings.h"
#include "core_functions.h"
#include "shared.h"

// Update these with values suitable for your network.

const char* ssid = "VF_IT_FWA_DF33";
const char* password = "8iD3965d4284RYA5";
const char* mqtt_server = "broker.hivemq.com";

wifi_conf_t wifi_conf;
mqtt_conf_t mqtt_conf;
module_conf_t module_conf;

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

WiFiClient espClient;
PubSubClient client(espClient);

bool shouldReboot = false;
bool hasWifiConfig = false;


String clientId = DEFAULT_CLIENTID_PREFIX;

extern unsigned long lastLoggedTime;



void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
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
    }
  }
}

String wifiprocessor(const String& var) {
    if(var=="DEFAULT_HOSTNAME"){
      if(hasWifiConfig)
        return wifi_conf.client_name;
      else
        return clientId;
    }
    else if(var=="SAVED_SSID"){
      if(hasWifiConfig)
        return wifi_conf.ssid;
    }
    else if(var=="SAVED_PASSWORD"){
      if(hasWifiConfig)
        return wifi_conf.pw;
    }
    return String();
}

void setup() {
  setupPin();
  Serial.begin(115200);
  clientId += String(ESP.getChipId(), DEC);
  int result=loadFS();
  result=loadWifiConfiguration(&wifi_conf);
  hasWifiConfig = (result==0);
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
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(request->args()>0){
      strcpy(wifi_conf.ssid,request->arg("ssid").c_str());
      strcpy(wifi_conf.pw,request->arg("password").c_str());
      strcpy(wifi_conf.client_name,request->arg("hostname").c_str());
      if(storeWifiConfiguration(&wifi_conf)==0){
        request->send(200, "text/html",reboot_page);
        shouldReboot = true;
      }
      else{
        request->send(200, "text/html",reboot_page);// sostituire con error page
      }
      
    }
    request->send_P(200, "text/html",wifi_config_html,wifiprocessor);
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/css",style);
  });
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    int n = WiFi.scanComplete();
    if(n == -2){
      WiFi.scanNetworks(true);
    } else if(n){
      for (int i = 0; i < n; ++i){
        if(i) json += ",";
        json += "{";
        json += "\"rssi\":"+String(WiFi.RSSI(i));
        json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
        json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
        json += ",\"channel\":"+String(WiFi.channel(i));
        json += ",\"secure\":"+String(WiFi.encryptionType(i));
        json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
        json += "}";
      }
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "application/json", json);
    json = String();
  });
  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  /*if (!client.connected()) {
    reconnect();
  }
  client.loop();*/
  //Leggo i sensori
  if(shouldReboot){
    Serial.println("Rebooting...");
    delay(1000);
    ESP.restart();
  }
  if(wifi_connection_loop_handler()==0)
    //reconnect();
  //client.loop();
  stateUpdate();
  //Controllo se loggare su MQTT
  if(millis() - lastLoggedTime > FORCE_UPDATE_TIME){
    stateLog(true);
  }
  else{
    stateLog(false);  
  }
  ElegantOTA.loop();

}