#include <LittleFS.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "fs_functions.h"

int loadFS(){
    if (!LittleFS.begin()) //avvio filesystem
    {
        Serial.println("Errore nell'avvio del FileSystem");
        return 1;
    }
    return 0;
}

int loadWifiConfiguration(wifi_conf_t *wifi_conf){
    File wc = LittleFS.open("/config/wc.json","r");
    if(!wc){
        Serial.println("Errore in caricamento della configurazione wifi!");
        return 1;
    }

    JsonDocument wifi_conf_json;
    DeserializationError error = deserializeJson(wifi_conf_json, wc);
    
    if (error){
        Serial.println(F("Errore nel retrieve della configurazione wifi!"));
        return 1;
    }
    else{
        strcpy(wifi_conf->ssid,wifi_conf_json["ssid"]);
        strcpy(wifi_conf->pw,wifi_conf_json["password"]);
        strcpy(wifi_conf->client_name,wifi_conf_json["client_name"]);
        }
    wc.close();
    return 0;
}

int storeWifiConfiguration(wifi_conf_t *wifi_conf){
    File wc = LittleFS.open("/config/wc.json","w");
    if(!wc){
        Serial.println("Errore in salvataggio della configurazione wifi!");
        return 1;
    }

    JsonDocument wifi_conf_json;
    wifi_conf_json["ssid"]=wifi_conf->ssid;
    wifi_conf_json["password"]=wifi_conf->pw;
    wifi_conf_json["client_name"]=wifi_conf->client_name;
    if (serializeJson(wifi_conf_json, wc) == 0) {
        Serial.println(F("Errore nel salvataggio della configurazione wifi!"));

    }
    wc.close();
    return 0;
}

int loadMqttConfiguration(mqtt_conf_t *mqtt_conf){
    File mc = LittleFS.open("/config/mc.json","r");
    if(!mc){
        Serial.println("Errore in caricamento della configurazione mqtt!");
        return 1;
    }
    JsonDocument mqtt_conf_json;
    DeserializationError error = deserializeJson(mqtt_conf_json, mc);
    if (error){
        Serial.println(F("Errore nel retrieve della configurazione MQTT!"));
        return 1;
    }
    else{
        strcpy(mqtt_conf->ip,mqtt_conf_json["ip"]);
        mqtt_conf->port=(int)mqtt_conf_json["port"];
        strcpy(mqtt_conf->username,mqtt_conf_json["username"]);
        strcpy(mqtt_conf->password,mqtt_conf_json["password"]);
        strcpy(mqtt_conf->availability_topic,mqtt_conf_json["availability_topic"]);
        strcpy(mqtt_conf->command_topic,mqtt_conf_json["command_topic"]);
        strcpy(mqtt_conf->state_topic,mqtt_conf_json["state_topic"]);
        }
    mc.close();
    return 0;
}

int storeMqttConfiguration(mqtt_conf_t *mqtt_conf){
    File mc = LittleFS.open("/config/mc.json","w");
    if(!mc){
        Serial.println("Errore in salvataggio della configurazione MQTT!");
        return 1;
    }
    JsonDocument mqtt_conf_json;
    mqtt_conf_json["ip"]=mqtt_conf->ip;
    mqtt_conf_json["port"]=mqtt_conf->port;
    mqtt_conf_json["username"]=mqtt_conf->username;
    mqtt_conf_json["password"]=mqtt_conf->password;
    mqtt_conf_json["availability_topic"]=mqtt_conf->availability_topic;
    mqtt_conf_json["command_topic"]=mqtt_conf->command_topic;
    mqtt_conf_json["state_topic"]=mqtt_conf->state_topic;
    if (serializeJson(mqtt_conf_json, mc) == 0) {
        Serial.println(F("Errore nel salvataggio della configurazione MQTT!"));
        return 1;
    }
    mc.close();
    return 0;
}

bool mqttconf_exists(){
    return LittleFS.exists("/config/mc.json");
}

int storeModuleConfiguration(module_conf_t *module_conf){
    File module = LittleFS.open("/config/module.json","w");
    if(!module){
        Serial.println("Errore in salvataggio della configurazione del modulo");
        return 1;
    }
    JsonDocument module_conf_json;
    module_conf_json["relaypin"]=module_conf->relay_pin;
    module_conf_json["relaymode"]=module_conf->relay_mode;
    module_conf_json["contactpin"]=module_conf->contact_pin;
    module_conf_json["contactmode"]=module_conf->contact_mode;
    if (serializeJson(module_conf_json, module) == 0) {
        Serial.println(F("Errore nel salvataggio della configurazione del modulo!"));
        return 1;
    }
    module.close();
    return 0;
}

int loadModuleConfiguration(module_conf_t *module_conf){
    File module = LittleFS.open("/config/module.json","r");
    if(!module){
        Serial.println("Errore in caricamento della configurazione modulo!");
        return 1;
    }
    
    JsonDocument module_conf_json;
    DeserializationError error = deserializeJson(module_conf_json, module);
    if (error){
        Serial.println(F("Errore nel retrieve della configurazione MQTT!"));
        return 1;
    }
    else{
        module_conf->relay_pin=(int)module_conf_json["relaypin"];
        module_conf->relay_mode=(int)module_conf_json["relaymode"];
        module_conf->contact_pin=(int)module_conf_json["contactpin"];
        module_conf->contact_mode=(int)module_conf_json["contactmode"];
        }
    module.close();
    return 0;
}