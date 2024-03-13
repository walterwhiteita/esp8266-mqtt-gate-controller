#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include "html_pages.h"
#include "fs_functions.h"
#include "shared.h"

extern AsyncWebServer server;
extern wifi_conf_t wifi_conf;
extern bool shouldReboot;
extern bool hasWifiConfig;
extern String clientId;

String wifiprocessor(const String& var);


unsigned long ota_progress_millis = 0;

void initializeServerEndpoints()
{
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
    {
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
    request->send_P(200, "text/html",wifi_config_html,wifiprocessor); });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/css", style); });
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
    server.begin();
    Serial.println("HTTP server started");
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


void loopOta(){
    ElegantOTA.loop();
}