#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include "html_pages.h"
#include "fs_functions.h"
#include "shared.h"
#include "settings.h"

extern AsyncWebServer server;
extern wifi_conf_t wifi_conf;
extern mqtt_conf_t mqtt_conf;
extern bool shouldReboot;
extern bool hasWifiConfig;
extern bool hasMqttConfig;
extern String clientId;

String wifiprocessor(const String& var);
String mqttprocessor(const String& var);
bool checkWifiParams();
bool checkMqttParams(AsyncWebServerRequest *request);

unsigned long ota_progress_millis = 0;

void initializeServerEndpoints(){
  //Setup response for wifi
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->args()>0){
      strcpy(wifi_conf.ssid,request->arg("ssid").c_str());
      strcpy(wifi_conf.pw,request->arg("password").c_str());
      strcpy(wifi_conf.client_name,request->arg("hostname").c_str());
      if(storeWifiConfiguration(&wifi_conf)==0){
        request->send(200, "text/html",rebootPage);
        shouldReboot = true;
      }
      else{
        request->send(200, "text/html",errorPage);// sostituire con error page
      }
    }
    request->send_P(200, "text/html",wifiConfigHtml,wifiprocessor); });
    //Setup response richiesta stile
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/css", style); });
    //Setup response for scan wifi
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request)
    {
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
    json = String(); });
  //Setup response for mqtt
    server.on("/mqtt", HTTP_ANY, [](AsyncWebServerRequest *request){
      if(request->method() == HTTP_GET){
       request->send_P(200, "text/html",mqttConfigHtml,mqttprocessor);
      }
      else if(request->method() == HTTP_POST){
        if(request->params()== 7 && checkMqttParams(request)){
          strcpy(mqtt_conf.ip,request->arg("broker").c_str());
          mqtt_conf.port = atoi(request->arg("port").c_str());
          strcpy(mqtt_conf.username,request->arg("username").c_str());
          strcpy(mqtt_conf.password,request->arg("password").c_str());
          strcpy(mqtt_conf.availability_topic,request->arg("availability-topic").c_str());
          strcpy(mqtt_conf.command_topic,request->arg("command-topic").c_str());
          strcpy(mqtt_conf.state_topic,request->arg("state-topic").c_str());
          if(storeMqttConfiguration(&mqtt_conf)==0){
            request->send(200, "text/html",rebootPage);
            shouldReboot = true;
          }
          else{
            Serial.println("Errore salvataggio");
            request->send(200, "text/html",errorPage);// sostituire con error page
          }
        }
        else{
          Serial.println("Errore lettura parametri");
          Serial.println(request->args());
          Serial.println(checkMqttParams(request));
          request->send(200, "text/html",errorPage);// sostituire con error page
        }
      }
    });
    server.begin();
    Serial.println("HTTP server started");
}

String wifiprocessor(const String& var) {
    if(var=="SAVED_HOSTNAME"){
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

String mqttprocessor(const String& var){
  if(var=="BROKER"){
    if(hasMqttConfig)
      return mqtt_conf.ip;
    else
      return String();
  }
  else if(var=="PORT")
  {
    if(hasMqttConfig)
      return String(mqtt_conf.port);
    else
      return String("1883");
  }
  else if(var=="USERNAME")
  {
    if(hasMqttConfig)
      return mqtt_conf.username;
    else
      return String();
  }
  else if(var=="PASSWORD")
  {
    if(hasMqttConfig)
      return mqtt_conf.password;
    else
      return String();
  }
  else if(var=="AVAILABILITY_TOPIC")
  {
    if(hasMqttConfig)
      return mqtt_conf.availability_topic;
    else
      return clientId+String(AVAILABILITY_TOPIC);
  }
  else if(var=="COMMAND_TOPIC")
  {
    if(hasMqttConfig)
      return mqtt_conf.command_topic;
    else
      return clientId+String(COMMAND_TOPIC);
  }
  else if(var=="STATE_TOPIC")
  {
    if(hasMqttConfig)
      return mqtt_conf.state_topic;
    else
      return clientId+String(STATE_TOPIC);
  }
  return String();
}

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

void setupOta(){
  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
}

bool checkMqttParams(AsyncWebServerRequest *request){
  return (request->hasParam("broker",true) && request->hasParam("port",true)
          && request->hasParam("user",true) && request->hasParam("password",true) && request->hasParam("availability-topic",true)
          && request->hasParam("command-topic",true) && request->hasParam("state-topic",true));
}

void loopOta(){
    ElegantOTA.loop();
}