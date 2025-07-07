/*

Public Domain 2022 Al Williams
2021 Ralph Bacon

 */


#ifndef __Encoder_h__
#define __Encoder_h__

#include "mbed.h"

class Encoder{
public:
// constructor with default no pulldown
    Encoder(PinName pinA, PinName pinB, PinMode mode=PullNone);
    int read();
    bool getRotaryEncoder() const { return rotaryEncoder; } 

protected:
    InterruptIn APin;  // pin A
    InterruptIn  BPin;   // pin B
    bool       rotaryEncoder=false;
    // ISR
    void isrCheck();
};

#endif