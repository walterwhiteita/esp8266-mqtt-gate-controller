#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <PubSubClient.h>

#include "fs_functions.h"
#include "html_pages.h"
#include "wifi_functions.h"


#define BUTTON_DEBOUNCE_DELAY   20   // [ms]
#define MSG_BUFFER_SIZE	(50)
#define STATE_TOPIC "gate/stat"
#define COMMAND_TOPIC "gate/cmnd"
#define AVAILABILITY_TOPIC "gate/availability"
#define FORCE_UPDATE_TIME 60000
#define CLICK_TIME 500
#define CORRECT_NUM_OF_LECTURES 50
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
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;
int n = 0;
unsigned int statusCount=0;

enum gateStatus {GATE_UNKNOWN=-1,GATE_OPENING, GATE_CLOSING, GATE_OPEN, GATE_CLOSED};
enum gateCmnd {CMND_OPEN=0, CMND_CLOSE, CMND_STOP};
gateStatus actualState;
gateStatus lastReadedState;
gateStatus lastLoggedState=GATE_UNKNOWN;

static const int ACIn1 = 14;
static const int ACIn2 = 12;
static const int ACIn3 = 13;

static const int Rele1 = 5;
static const int Rele2 = 4;
static const int Rele3 = 0;




unsigned long lastLoggedTime;

String stateConversion(gateStatus input);
void stateLog(bool force);
void stateUpdate();

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


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

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
    String clientId = "GateController-";
    clientId += String(ESP.getChipId(), DEC);
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
    if(var=="W_NETWORKS"){
      String response = "";
      if(n){
        for (int i = 0; i < n; i++)
        {
          response.concat("<option>");
          response.concat(WiFi.SSID(i));
          response.concat("</option>");
        }
      }
      return response;
    }
    return String();
}

void setup() {
  pinMode(ACIn1,INPUT_PULLUP);
  pinMode(ACIn2,INPUT_PULLUP);
  pinMode(ACIn3,INPUT_PULLUP);

  pinMode(Rele1,OUTPUT);
  pinMode(Rele2,OUTPUT);
  pinMode(Rele3,OUTPUT);
  Serial.begin(115200);
  //setup_wifi();
  int result=loadFS();
  result=loadWifiConfiguration(&wifi_conf);
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
  n = WiFi.scanNetworks();
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(request->args()>0){
      strcpy(wifi_conf.ssid,request->arg("ssid").c_str());
      strcpy(wifi_conf.pw,request->arg("password").c_str());
      strcpy(wifi_conf.client_name,request->arg("hostname").c_str());
      Serial.println(storeWifiConfiguration(&wifi_conf));
    }
    request->send_P(200, "text/html",wifi_config_html,wifiprocessor);
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/css",style);
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