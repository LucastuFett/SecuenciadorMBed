#pragma once
#include "mbed.h"
#include <map>

struct holdKey {
    uint8_t beatStart;
    uint8_t channel;
    uint8_t midiNote;
};

struct CompareHoldKey {
    bool operator()(const holdKey& a1, const holdKey& a2) const {
        if (a1.beatStart != a2.beatStart) return a1.beatStart < a2.beatStart;
        if (a1.channel != a2.channel) return a1.channel < a2.channel;
        return a1.midiNote < a2.midiNote;
    }
};

// GRB Button Colors
#define BlueBtn 0xb525e700
#define PurpleBtn 0x5798f300
#define RedBtn 0x00c30000
#define OrangeBtn 0x80fc1f

class Buttons {
    uint8_t _holdTemporary;
    uint32_t _ledData[16];
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