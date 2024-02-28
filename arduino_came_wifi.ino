#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <PubSubClient.h>
#include "InputDebounce.h"

#define BUTTON_DEBOUNCE_DELAY   20   // [ms]
#define MSG_BUFFER_SIZE	(50)
#define STATE_TOPIC "gate/stat"
#define COMMAND_TOPIC "gate/cmnd"
#define AVAILABILITY_TOPIC "gate/availability"
#define FORCE_UPDATE_TIME 60000
#define CLICK_TIME 500
// Update these with values suitable for your network.

const char* ssid = "Pablitonet";
const char* password = "angelo2000";
const char* mqtt_server = "192.168.178.5";

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;

enum gateStatus {GATE_UNKNOWN=-1,GATE_OPENING, GATE_CLOSING, GATE_OPEN, GATE_CLOSED};
enum gateCmnd {CMND_OPEN=0, CMND_CLOSE, CMND_STOP};
gateStatus actualState;
gateStatus lastLoggedState=GATE_UNKNOWN;

static const int ACIn1 = 14;
static const int ACIn2 = 12;
static const int ACIn3 = 13;

static const int Rele1 = 5;
static const int Rele2 = 4;
static const int Rele3 = 0;

static InputDebounce ACIn1Debounce;
static InputDebounce ACIn2Debounce;
static InputDebounce ACIn3Debounce;

unsigned long lastLoggedTime;

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

void ACInUpCallback(uint8_t pinIn)
{
  // handle pressed state
  //digitalWrite(pinLED, HIGH); // turn the LED on
  Serial.print("HIGH (pin: ");
  Serial.print(pinIn);
  Serial.println(")");
}

void ACInDownCallback(uint8_t pinIn,unsigned long duration)
{
  // handle released state
  //digitalWrite(pinLED, LOW); // turn the LED off
  Serial.print("LOW (pin: ");
  Serial.print(pinIn);
  Serial.println(")");
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
    // Create a random client ID
    String clientId = "GateController-";
    clientId += String(ESP.getChipId(), DEC);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"angelo","mosquitto",AVAILABILITY_TOPIC,0,1,"offline")) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(COMMAND_TOPIC);
      client.publish(AVAILABILITY_TOPIC, "online", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(ACIn1,INPUT_PULLUP);
  pinMode(ACIn2,INPUT_PULLUP);
  pinMode(ACIn3,INPUT_PULLUP);

  pinMode(Rele1,OUTPUT);
  pinMode(Rele2,OUTPUT);
  pinMode(Rele3,OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");
  /*ACIn1Debounce.setup(ACIn1, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  ACIn2Debounce.setup(ACIn2, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  ACIn3Debounce.setup(ACIn3, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  
  ACIn1Debounce.registerCallbacks(ACInUpCallback, NULL, NULL,ACInDownCallback);
  ACIn2Debounce.registerCallbacks(ACInUpCallback, NULL, NULL,ACInDownCallback);
  ACIn3Debounce.registerCallbacks(ACInUpCallback, NULL, NULL,ACInDownCallback);*/
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
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
  /*
  ACIn1Debounce.process(now); //motore apertura
  ACIn2Debounce.process(now); //motore chiusura
  ACIn3Debounce.process(now); //spia aperto*/
  /*
  funzione -> controlla se pubblicare o meno*/
}

void stateUpdate(){
  if(!digitalRead(ACIn2)){
    actualState = GATE_OPENING;
  }
  else if(!digitalRead(ACIn3)){
    actualState = GATE_CLOSING;
  }
  else if(!digitalRead(ACIn1)){
    actualState = GATE_OPEN;
  }
  else{
    actualState = GATE_CLOSED;
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
