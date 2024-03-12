#include <Arduino.h>
#include "shared.h"
void relayClick(int pin);
void handleCmnd(gateCmnd cmnd);
void setupPin();
String stateConversion(gateStatus input);
void stateLog(bool force);
void stateUpdate();