#pragma once
#include "definitions.h"


#define RGB 0
#if RGB
    // RGB Button Colors
    #define BlueBtnRGB    0x092D3900
    #define PurpleBtnRGB  0x26153C00
    #define RedBtnRGB     0x30000000
    #define OrangeBtnRGB  0x3F200700

    // RGB colors (uint32_t)
    const uint32_t launchColorsRGB[] = {
        0x7F191200,
        0x00430600,
        0x1F3B7F00,
        0x66420E00,
        0x782A5700,
        0x05556400,
        0x7A2F2600,
        0x00677D00,
        0x00677D00,
        0x7A2F2600,
        0x05556400,
        0x782A5700,
        0x66420E00,
        0x1F3B7F00,
        0x00430600,
        0x7F191200
    };
    
#else
    // GRB Button Colors
    #define BlueBtn    0x2D093900
    #define PurpleBtn  0x15263C00
    #define RedBtn     0x00300000
    #define OrangeBtn  0x203F0700

    // GRB colors (uint32_t) at 50% brightness
    const uint32_t launchColorsGRB[] = {
        0x197F1200,
        0x43000600,
        0x3B1F7F00,
        0x42660E00,
        0x2A785700,
        0x55056400,
        0x2F7A2600,
        0x00677D00,
        0x00677D00,
        0x2F7A2600,
        0x55056400,
        0x2A785700,
        0x42660E00,
        0x3B1F7F00,
        0x43000600,
        0x197F1200
    };
#endif

class Buttons {
    uint8_t _holdTemporary;
    
public:
    // Constructor
    Buttons();

    // Press a button
    // num: Button number (0-15)
    void press(uint8_t num);

    // Release a button
    // num: Button number (0-15)
    void release(uint8_t num);

    // Update the state of the buttons' LEDs
    void updateColors();

    // Update BeatsPerTone, Channels and Holded
    void updateStructures();

    // Update Holded
    void updateHolded();
    
};