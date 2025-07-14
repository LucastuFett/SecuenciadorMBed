#pragma once
#include "definitions.h"



// GRB Button Colors
#define BlueBtn 0xb525e700
#define PurpleBtn 0x5798f300
#define RedBtn 0x00c30000
#define OrangeBtn 0x80fc1f

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