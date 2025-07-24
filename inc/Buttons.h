#pragma once
#include "definitions.h"



// GRB Button Colors
#define BlueBtn    0x2D093900
#define PurpleBtn  0x15263C00
#define RedBtn     0x00300000
#define OrangeBtn  0x203F0700

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