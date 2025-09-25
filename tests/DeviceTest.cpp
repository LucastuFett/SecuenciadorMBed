#include "Buttons.h"
#include "Encoder.h"
#include "MIDIFiles.h"
#include "MIDITimer.h"
#include "Screen.h"
#include "USBMIDI.h"
#include "WS2812.h"
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

uint32_t ledData[16] = {0};
uint32_t lastLedData[16] = {0};

Mutex ledDataMutex;

// Composition
void timeout();

Screen screen(p27, p28, p26, p22, NC, p21, "TFT");
MIDIFiles midiFiles;
MIDITimer timer(callback(timeout));

// Testing

enum testStates { BUTTONS,
                  ENCODER,
                  MIDI,
                  MSD,
                  FULL };
testStates testState = BUTTONS;
bool btnGridTest = false;
bool exitBtnTest = false;
int8_t encoderTest = 64;
bool externalClock = false;
bool usbMode = false;
bool clockSource = false;

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

void buttonsTest(uint8_t num) {
    static uint8_t currentBtn = 0;
    ledDataMutex.lock();
    for (uint8_t i = 0; i < 16; i++) ledData[i] = 0;
    if (currentBtn < 3 && num == currentBtn) {
        ledData[num + 3] = PurpleBtn;
        currentBtn++;
    } else if (currentBtn == 3 && num == currentBtn) {
        ledData[0] = BlueBtn;
        currentBtn++;
    } else if (currentBtn < 19 && num == currentBtn) {
        ledData[num - 3] = BlueBtn;
        currentBtn++;
    } else if (currentBtn == 19 && num == currentBtn) {
        ledData[7] = PurpleBtn;
        btnGridTest = true;
    }
    // Invert last 8 LEDs
    for (uint8_t i = 8; i < 12; i++) {
        uint32_t temp = ledData[i];
        ledData[i] = ledData[23 - i];
        ledData[23 - i] = temp;
    }
    ledDataMutex.unlock();
}

void timeout() {
    static bool noteOff = false;
    if (!noteOff)
        timer.send(0x90, 64, 127);
    else
        timer.send(0x90, 64, 0);
    noteOff = !noteOff;
}

int main() {
    // Give the USB a moment to initialize and enumerate
    ThisThread::sleep_for(100ms);

    // Initialize components
    midiFiles.init();
    ledData[2] = PurpleBtn;
    ledStrip.WS2812_Transfer((uint32_t)&ledData, sizeof(ledData) / sizeof(ledData[0]));  // Update LED strip

    while (true) {
        readKeys();
        switch (testState) {
            case BUTTONS:
                if (!btnGridTest) {
                    if (memcmp(keys, lastKeys, sizeof(keys)) != 0) {
                        for (int i = 0; i < 5; i++) {
                            for (int j = 0; j < 4; j++) {
                                if (keys[i][j] && !lastKeys[i][j]) {  // Key pressed
                                    uint8_t num = i * 4 + j;
                                    if (num < 2)
                                        num += 2;
                                    else if (num < 4)
                                        num -= 2;
                                    buttonsTest(num);
                                }
                            }
                        }
                        memcpy(lastKeys, keys, sizeof(keys));  // Update lastKeys
                    }
                } else if (!exitBtnTest) {
                    if (memcmp(switches, lastSwitches, sizeof(switches)) != 0) {
                        if (switches[1] && !lastSwitches[1]) {
                            ledDataMutex.lock();
                            ledData[7] = 0;
                            ledDataMutex.unlock();
                            exitBtnTest = true;
                            testState = ENCODER;
                            screen.cls();
                            screen.locate(100, 110);
                            screen.puts(to_string(encoderTest).c_str());
                        }
                        memcpy(lastSwitches, switches, sizeof(switches));
                    }
                }
                break;
            case ENCODER:
                if (encoder.getRotaryEncoder()) {
                    int current = encoder.read();
                    if (current == 1)
                        encoderTest++;
                    if (current == -1)
                        encoderTest--;
                    screen.locate(100, 110);
                    screen.puts(to_string(encoderTest).c_str());
                }
                if (memcmp(switches, lastSwitches, sizeof(switches)) != 0) {
                    if (switches[0] && !lastSwitches[0]) {
                        testState = MIDI;
                        screen.cls();
                        screen.locate(100, 110);
                        screen.puts("Internal");
                        timer.playPause();
                    }
                    memcpy(lastSwitches, switches, sizeof(switches));
                }
                break;
            case MIDI:
                timer.poll();
                if (memcmp(switches, lastSwitches, sizeof(switches)) != 0) {
                    if (switches[0] && !lastSwitches[0]) {
                        if (!externalClock) {
                            externalClock = true;
                            screen.cls();
                            screen.locate(100, 110);
                            screen.puts("External USB");
                        } else {
                            if (!clockSource) {
                                clockSource = true;
                                screen.cls();
                                screen.locate(100, 110);
                                screen.puts("External UART");
                            } else {
                                screen.cls();
                                screen.locate(100, 110);
                                screen.puts("MSD");
                                testState = MSD;
                                if (!midiFiles.getUSB()) {
                                    timer.deinitUSB();
                                    midiFiles.initUSB();
                                }
                                usbMode = midiFiles.getUSB();
                            }
                        }
                    }
                    memcpy(lastSwitches, switches, sizeof(switches));
                }
                break;
            case MSD:
                midiFiles.process();
                if (memcmp(switches, lastSwitches, sizeof(switches)) != 0) {
                    if (switches[0] && !lastSwitches[0]) {
                        if (!timer.getUSB()) {
                            midiFiles.deinitUSB();
                            timer.initUSB();
                        }
                        usbMode = !timer.getUSB();
                        testState = FULL;
                        ledDataMutex.lock();
                        for (uint8_t i = 0; i < 16; i++) ledData[i] = launchColors[i];
                        ledDataMutex.unlock();
                        externalClock = false;
                        clockSource = false;
                        screen.cls();
                    }
                    memcpy(lastSwitches, switches, sizeof(switches));
                }
                break;
            case FULL:
                timer.poll();
                screen.locate(100, 120);
                screen.puts("Test");
                break;
        }

        ledDataMutex.lock();
        if (memcmp(ledData, lastLedData, sizeof(ledData)) != 0) {
            ledStrip.WS2812_Transfer((uint32_t)&ledData, sizeof(ledData) / sizeof(ledData[0]));  // Update LED strip
            memcpy(lastLedData, ledData, sizeof(ledData));                                       // Update last LED data
        }
        ledDataMutex.unlock();
        // ThisThread::sleep_for(1ms);
    }
}