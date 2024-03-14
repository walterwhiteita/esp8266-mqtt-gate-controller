#include <LittleFS.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#ifndef FS_FUNCTIONS_H
#define FS_FUNCTIONS_H

typedef struct {
  char ssid[31];
  char pw[31];
  char client_name[51];
} wifi_conf_t;

typedef struct{
  char ip[51];
  int port;
  char username[31];
  char password[31];
  char availability_topic[61];
  char command_topic[61];
  char state_topic[61];
} mqtt_conf_t;

typedef struct{
  int relay_pin;
  int relay_mode;
  int contact_pin;
  int contact_mode;
} module_conf_t;


#endif 

int loadFS();

int loadWifiConfiguration(wifi_conf_t *wifi_conf);

int storeWifiConfiguration(wifi_conf_t *wifi_conf);

int loadMqttConfiguration(mqtt_conf_t *mqtt_conf);

int storeMqttConfiguration(mqtt_conf_t *mqtt_conf);

bool mqttconf_exists();

int storeModuleConfiguration(module_conf_t *module_conf);

int loadModuleConfiguration(module_conf_t *module_conf);

