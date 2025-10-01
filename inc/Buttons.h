#pragma once
#include "definitions.h"

#define RGBBTN 1
#if RGBBTN
// RGB Button Colors — scaled to 25%
#define BlueBtn 0x020B0E00
#define PurpleBtn 0x0A050F00
#define RedBtn 0x0C000000
#define OrangeBtn 0x10080200

// RGB colors (uint32_t)
const uint32_t launchColors[] = {
    0x20060400, 0x00110200, 0x080F2000, 0x1A100400,
    0x1E0A1600, 0x01151900, 0x1E0C0A00, 0x001A1F00,
    0x001A1F00, 0x1E0C0A00, 0x01151900, 0x1E0A1600,
    0x1A100400, 0x080F2000, 0x00110200, 0x20060400};

#else
// GRB Button Colors — scaled to 25%
#define BlueBtn 0x0B020E00
#define PurpleBtn 0x050A0F00
#define RedBtn 0x000C0000
#define OrangeBtn 0x08100200

// GRB colors (uint32_t)
const uint32_t launchColors[] = {
    0x06200400, 0x11000200, 0x0F082000, 0x101A0400,
    0x0A1E1600, 0x15011900, 0x0C1E0A00, 0x001A1F00,
    0x001A1F00, 0x0C1E0A00, 0x15011900, 0x0A1E1600,
    0x101A0400, 0x0F082000, 0x11000200, 0x06200400};
#endif

class Buttons {
    uint8_t _holdTemporary;
    uint16_t _holdIndex;
    bool _DAWSent[16];

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