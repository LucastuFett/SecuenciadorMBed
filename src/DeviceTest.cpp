#include "Buttons.h"
#include "Encoder.h"
#include "MIDIFiles.h"
#include "MIDITimer.h"
#include "Screen.h"
#include "USBMIDI.h"
#include "WS2812.h"
#include "definitions.h"
#include "mbed.h"

// Hardware

DigitalInOut columns[4] = {
    DigitalInOut(p16, PIN_INPUT, PullNone, 0),
    DigitalInOut(p15, PIN_INPUT, PullNone, 0),
    DigitalInOut(p18, PIN_INPUT, PullNone, 0),
    DigitalInOut(p17, PIN_INPUT, PullNone, 0)};

DigitalIn rows[5] = {
    DigitalIn(p13, PullDown),
    DigitalIn(p12, PullDown),
    DigitalIn(p11, PullDown),
    DigitalIn(p9, PullDown),
    DigitalIn(p10, PullDown)};

WS2812_PIO ledStrip(p19, 16);
Encoder encoder(p7, p8, PullUp);
DigitalIn exitSW(p14, PullDown);
DigitalIn selectSW(p6, PullDown);

bool keys[5][4] = {{0}};  // 5 rows, 4 columns
bool lastKeys[5][4] = {{0}};
bool switches[2] = {0};  // Select and Exit
bool lastSwitches[2] = {0};

uint32_t ledData[16];
uint32_t lastLedData[16];

// Function Declarations

void readKeys();

// Composition

Screen screen(p27, p28, p26, p22, NC, p21, "TFT");
Buttons buttons;
MIDIFiles midiFiles;

// Function Definitions

void readKeys() {
    for (int j = 0; j < 4; j++) {
        columns[j].output();
        columns[j] = 1;              // Set current column high
        ThisThread::sleep_for(1ms);  // Wait for the column to stabilize
        for (int i = 0; i < 5; i++) {
            keys[i][j] = rows[i].read();
        }
        columns[j] = 0;  // Set current column low
        columns[j].input();
    }
    switches[0] = selectSW;
    switches[1] = exitSW;
}

int main() {
    // Give the USB a moment to initialize and enumerate
    ThisThread::sleep_for(100ms);

    // Initialize components
    midiFiles.init();
    ledStrip.WS2812_Transfer((uint32_t)&ledData, sizeof(ledData) / sizeof(ledData[0]));  // Update LED strip

    while (true) {
        readKeys();
        if (memcmp(keys, lastKeys, sizeof(keys)) != 0) {
            memcpy(lastKeys, keys, sizeof(keys));  // Update lastKeys
        }

        if (memcmp(switches, lastSwitches, sizeof(switches)) != 0) {
            memcpy(lastSwitches, switches, sizeof(switches));
        }

        if (encoder.getRotaryEncoder()) {
            int current = encoder.read();
            if (current == 1)
                ;
            if (current == -1)
                ;
        }

        if (memcmp(ledData, lastLedData, sizeof(ledData)) != 0) {
            ledStrip.WS2812_Transfer((uint32_t)&ledData, sizeof(ledData) / sizeof(ledData[0]));  // Update LED strip
            memcpy(lastLedData, ledData, sizeof(ledData));                                       // Update last LED data
        }

        // ThisThread::sleep_for(1ms);
    }
}