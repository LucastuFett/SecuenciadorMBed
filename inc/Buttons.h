#pragma once
#include "mbed.h"
#include <map>

struct holdKey {
    uint8_t beatStart;
    uint8_t channel;
    uint8_t midiNote;
};


class Buttons {
    uint8_t _holdTemporary;
public:
    // Constructor
    Buttons();

    // Press a button
    // num: Button number (0-15)
    void press(uint8_t num);

    // Update the state of the buttons' LEDs
    void updateColors();

    // Update BeatsPerTone, Channels and Holded
    void updateStructures();

    // Update Holded
    void updateHolded();
    
};