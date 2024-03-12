#ifndef SHARED_H
#define SHARED_H
typedef enum {GATE_UNKNOWN=-1,GATE_OPENING, GATE_CLOSING, GATE_OPEN, GATE_CLOSED}gateStatus;
typedef enum {CMND_OPEN=0, CMND_CLOSE, CMND_STOP}gateCmnd;
#endif