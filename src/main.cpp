#include "Buttons.h"
#include "Encoder.h"
#include "MIDIFiles.h"
#include "MIDITimer.h"
#include "Screen.h"
#include "USBMIDI.h"
#include "WS2812.h"
#include "definitions.h"
#include "mbed.h"

#define TESTING_BTN 0
#define TESTING_MODES 0

// Hardware

#if TESTING_BTN

DigitalInOut columns[4] = {
    DigitalInOut(p8, PIN_INPUT, PullNone, 0),
    DigitalInOut(p7, PIN_INPUT, PullNone, 0),
    DigitalInOut(p20, PIN_INPUT, PullNone, 0),
    DigitalInOut(p21, PIN_INPUT, PullNone, 0)};

DigitalIn rows[5] = {
    DigitalIn(p5, PullDown),
    DigitalIn(p6, PullDown),
    DigitalIn(p22, PullDown),
    DigitalIn(p26, PullDown),
    DigitalIn(p27, PullDown)};

Encoder encoder(p19, p17, PullUp);
DigitalIn exitSW(p28, PullDown);
DigitalIn selectSW(p18, PullDown);
WS2812_PIO ledStrip(p10, 16);

#else

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

#endif

bool keys[5][4] = {{0}};  // 5 rows, 4 columns
bool lastKeys[5][4] = {{0}};
bool switches[2] = {0};  // Select and Exit
bool lastSwitches[2] = {0};

// Function Declarations

void readKeys();
void selectFunc();
void function1();
void function2();
void function3();
void function4();
void exitFunc();
void left();
void right();
void changeState();
void timeout();

// Composition

Screen screen(p27, p28, p26, p22, NC, p21, "TFT");
MIDIFiles midiFiles;
MIDITimer timer(callback(timeout));
Buttons buttons;

// Programming Mode

enum state mainState = MAIN;
#if TESTING_MODES
uint8_t midiMessages[320][3] = {{144, 36, 127}, {144, 49, 127}, {144, 38, 127}, {144, 36, 127}, {144, 49, 127}, {144, 49, 127}, {144, 38, 127}, {144, 49, 127}, {144, 36, 127}, {144, 49, 127}, {144, 38, 127}, {144, 36, 127}, {144, 49, 127}, {144, 36, 127}, {144, 38, 127}, {144, 49, 127}, {144, 36, 127}, {144, 49, 127}, {144, 38, 127}, {144, 36, 127}, {144, 49, 127}, {144, 49, 127}, {144, 38, 127}, {144, 49, 127}, {144, 36, 127}, {144, 49, 127}, {144, 38, 127}, {144, 36, 127}, {144, 49, 127}, {144, 36, 127}, {144, 38, 127}, {144, 49, 127}, {144, 49, 127}, {145, 53, 100}, {144, 49, 127}, {144, 49, 127}, {145, 53, 100}, {145, 53, 100}, {144, 49, 127}, {145, 53, 100}, {144, 49, 127}, {145, 50, 100}, {144, 49, 127}, {144, 49, 127}, {145, 55, 100}, {144, 49, 127}, {144, 49, 127}, {145, 55, 100}, {144, 49, 127}, {145, 57, 100}, {144, 49, 127}, {144, 49, 127}, {145, 55, 103}, {145, 55, 103}, {144, 49, 127}, {145, 55, 103}, {144, 49, 127}, {145, 57, 100}, {144, 49, 127}, {144, 49, 127}, {145, 57, 103}, {144, 49, 127}, {144, 49, 127}, {145, 52, 103}, {145, 53, 100}, {0, 0, 0}, {145, 53, 100}, {145, 53, 100}, {0, 0, 0}, {148, 76, 110}, {145, 53, 100}, {0, 0, 0}, {145, 50, 100}, {0, 0, 0}, {145, 55, 100}, {145, 55, 100}, {0, 0, 0}, {145, 55, 100}, {145, 55, 100}, {0, 0, 0}, {145, 57, 100}, {0, 0, 0}, {145, 57, 100}, {145, 57, 100}, {146, 43, 125}, {148, 72, 110}, {145, 55, 103}, {146, 45, 125}, {145, 57, 100}, {0, 0, 0}, {145, 57, 100}, {145, 57, 100}, {148, 72, 110}, {145, 52, 103}, {145, 52, 103}, {148, 69, 110}, {146, 41, 127}, {0, 0, 0}, {148, 76, 110}, {148, 74, 110}, {0, 0, 0}, {0, 0, 0}, {148, 74, 110}, {0, 0, 0}, {146, 38, 127}, {0, 0, 0}, {146, 43, 127}, {148, 74, 110}, {0, 0, 0}, {148, 72, 110}, {148, 76, 110}, {0, 0, 0}, {146, 45, 127}, {0, 0, 0}, {148, 69, 110}, {148, 69, 110}, {147, 59, 127}, {0, 0, 0}, {148, 74, 110}, {0, 0, 0}, {147, 60, 127}, {0, 0, 0}, {148, 74, 110}, {148, 74, 110}, {0, 0, 0}, {146, 40, 125}, {0, 0, 0}, {0, 0, 0}, {147, 57, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 127}, {0, 0, 0}, {147, 59, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 60, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 55, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 55, 127}, {0, 0, 0}, {0, 0, 0}, {147, 53, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 53, 127}, {0, 0, 0}, {147, 55, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 50, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 127}, {0, 0, 0}, {0, 0, 0}, {147, 48, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 48, 127}, {0, 0, 0}, {147, 50, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 43, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 45, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 48, 127}, {0, 0, 0}, {0, 0, 0}, {147, 41, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 41, 127}, {0, 0, 0}, {147, 43, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 45, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 72, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 76, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 40, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 72, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 71, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
uint8_t offMessages[320][3] = {{144, 49, 0}, {144, 36, 0}, {144, 49, 0}, {144, 38, 0}, {144, 36, 0}, {144, 49, 0}, {144, 49, 0}, {144, 38, 0}, {144, 49, 0}, {144, 36, 0}, {144, 49, 0}, {144, 38, 0}, {144, 36, 0}, {144, 49, 0}, {144, 36, 0}, {144, 38, 0}, {144, 49, 0}, {144, 36, 0}, {144, 49, 0}, {144, 38, 0}, {144, 36, 0}, {144, 49, 0}, {144, 49, 0}, {144, 38, 0}, {144, 49, 0}, {144, 36, 0}, {144, 49, 0}, {144, 38, 0}, {144, 36, 0}, {144, 49, 0}, {144, 36, 0}, {144, 38, 0}, {145, 52, 0}, {144, 49, 0}, {145, 53, 0}, {144, 49, 0}, {144, 49, 0}, {145, 53, 0}, {145, 53, 0}, {144, 49, 0}, {145, 53, 0}, {144, 49, 0}, {145, 50, 0}, {144, 49, 0}, {144, 49, 0}, {145, 55, 0}, {144, 49, 0}, {144, 49, 0}, {145, 55, 0}, {144, 49, 0}, {145, 57, 0}, {144, 49, 0}, {144, 49, 0}, {145, 55, 0}, {145, 55, 0}, {144, 49, 0}, {145, 55, 0}, {144, 49, 0}, {145, 57, 0}, {144, 49, 0}, {144, 49, 0}, {145, 57, 0}, {144, 49, 0}, {144, 49, 0}, {146, 40, 0}, {145, 53, 0}, {0, 0, 0}, {145, 53, 0}, {145, 53, 0}, {148, 74, 0}, {148, 76, 0}, {145, 53, 0}, {146, 41, 0}, {145, 50, 0}, {146, 38, 0}, {145, 55, 0}, {145, 55, 0}, {148, 74, 0}, {145, 55, 0}, {145, 55, 0}, {146, 43, 0}, {145, 57, 0}, {0, 0, 0}, {145, 57, 0}, {145, 57, 0}, {148, 72, 0}, {148, 72, 0}, {145, 55, 0}, {147, 59, 0}, {145, 57, 0}, {148, 76, 0}, {145, 57, 0}, {145, 57, 0}, {146, 45, 0}, {145, 52, 0}, {145, 52, 0}, {147, 55, 0}, {0, 0, 0}, {0, 0, 0}, {148, 76, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 0}, {147, 57, 0}, {0, 0, 0}, {148, 72, 0}, {0, 0, 0}, {0, 0, 0}, {148, 72, 0}, {0, 0, 0}, {147, 59, 0}, {0, 0, 0}, {0, 0, 0}, {148, 69, 0}, {146, 45, 0}, {0, 0, 0}, {0, 0, 0}, {146, 43, 0}, {147, 55, 0}, {0, 0, 0}, {0, 0, 0}, {148, 74, 0}, {148, 74, 0}, {147, 60, 0}, {148, 71, 0}, {0, 0, 0}, {147, 52, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 53, 0}, {147, 53, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 55, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 60, 0}, {0, 0, 0}, {0, 0, 0}, {148, 74, 0}, {147, 50, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 0}, {0, 0, 0}, {0, 0, 0}, {147, 48, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 48, 0}, {147, 48, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 50, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 43, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 0}, {0, 0, 0}, {0, 0, 0}, {147, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 41, 0}, {147, 41, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 43, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 45, 0}, {0, 0, 0}, {0, 0, 0}, {148, 69, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 74, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 76, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 45, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 72, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 69, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
uint8_t channels[16] = {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // 1 = Enabled, 0 = Disabled
uint32_t control = 0xFFFFFFFF;
bool mode32 = true;
#else
// MIDI Messages (On and Off, 10 rows, 32 columns)

uint8_t midiMessages[320][3] = {{0}};
uint8_t offMessages[320][3] = {{0}};

// Control Variables

uint8_t channels[16] = {0};
uint32_t control = 0;
bool mode32 = false;
#endif

// State Variables

uint32_t beatsPerTone[1536] = {0};  // 96 tones, 16 channels
int8_t beat = 0;
int8_t tone = 0;
int8_t mode = 0;
int8_t note = 0;
int8_t octave = 3;
int8_t channel = 0;
int8_t velocity = 127;
bool half = false;
int16_t tempo[2] = {0, 120};  // 0 = Int, 1 = Ext, in Ext, 0 = Half, 2 = Dbl
uint8_t hold = 0;             // 0 = No Hold, 1 = Waiting 1st, 2 = Waiting 2nd
bool usbMode = false;         // false = MIDI, true = MSD
bool clockSource = false;     // false = USB, true = UART
bool usbData = true;

// Memory Variables

string filename = "Song";
string renameFilename = "";
uint8_t bank = 1;
map<holdKey, uint8_t, CompareHoldKey> holded;

// UI Variables

bool shift = false;
Timer shiftTimer;
int8_t prevNote = 0;
int8_t prevMode = 0;
int8_t prevChn = 0;
int8_t prevTone = 0;
int16_t prevTempo[2] = {0, 120};
int16_t prevBPM = 120;
uint32_t ledData[16];
uint32_t lastLedData[16];

// Play Mode

uint8_t nextMessages[320][3] = {{0}};
uint8_t nextOffMessages[320][3] = {{0}};
int16_t nextTempo[2] = {0, 120};  // 0 = Int, 1 = Ext, in Ext, 0 = Half, 2 = Dbl
string nextFilename = "";
int8_t nextTone = 0;
int8_t nextMode = 0;
bool queue = false;
bool channelEnabled[16] = {true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true};

// Launch Mode

bool launchType = false;
uint8_t launchMessages[16][3] = {{0}};
int8_t launchOctave = 3;
int8_t launchMode = 0;
int8_t launchTone = 0;
int8_t launchChn = 0;
vector<uint16_t> launchPossible[2];

// RTOS Variables

Mutex messagesMutex;
Mutex ledDataMutex;
Mutex beatMutex;
Mutex beatsPerToneMutex;
Mutex controlMutex;
Mutex holdedMutex;
Mutex channelEnabledMutex;
Mutex tempoMutex;

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

void selectFunc() {
    switch (mainState) {
        case MAIN:
            break;
        case PROG:
            if (shift) {  // Toggle 32/16 mode
                mode32 = !mode32;
                shift = false;
            } else if (mode32)
                half = !half;  // Toggle First or Second 16 beats
            break;
        case NOTE:
        case SCALE:
        case TEMPO:
        case CHANNEL:
            mainState = PROG;
            break;
        case MEMORY:
        case RENAME:
            screen.selectLetter();  // When writing, select the letter
            break;
        case SAVELOAD:
            messagesMutex.lock();
            filename = screen.getFilename();
            midiFiles.readFromFile(midiMessages, offMessages, tempo, filename, bank, mode, tone);
            buttons.updateStructures();
            messagesMutex.unlock();
            mainState = PROG;
            break;
        case PLAY:
            if (timer.isRunning()) {  // If playing, queue next song
                nextFilename = screen.getFilename();
                if (nextFilename != "") {
                    midiFiles.readFromFile(nextMessages, nextOffMessages, nextTempo, nextFilename, bank, nextMode, nextTone);
                    queue = true;
                }
            } else {  // If not playing, load new song
                messagesMutex.lock();
                filename = screen.getFilename();
                if (filename != "") {
                    midiFiles.readFromFile(midiMessages, offMessages, tempo, filename, bank, mode, tone);
                    buttons.updateStructures();
                }
                messagesMutex.unlock();
            }
            break;
        case LAUNCH:
            launchType = !launchType;  // Toggle Launchpad/Keyboard
        case DAW:
        default:
            break;
    }
    changeState();
}

void function1() {
    switch (mainState) {
        case MAIN:
            if (shift && usbData) {  // If USBMIDI mode is disabled, enable it
                if (!timer.getUSB()) {
                    midiFiles.deinitUSB();
                    timer.initUSB();
                }
                usbMode = !timer.getUSB();
                shift = false;
            } else
                mainState = PROG;
            break;
        case NOTE:
        case SCALE:
        case TEMPO:
        case CHANNEL:
            mainState = PROG;
            break;
        case PROG:
            if (shift && !usbMode) {  // Go to memory mode if fs not used
                screen.setEdit(0);
                mainState = MEMORY;
                shift = false;
            } else {  // Go to note mode
                mainState = NOTE;
                prevNote = note;
            }
            break;
        case MEMORY:  // Save current song
            filename = screen.saveFilename();
            midiFiles.saveToFile();
            mainState = PROG;
            break;
        case SAVELOAD:  // Load selected song
            messagesMutex.lock();
            filename = screen.getFilename();
            midiFiles.readFromFile(midiMessages, offMessages, tempo, filename, bank, mode, tone);
            buttons.updateStructures();
            messagesMutex.unlock();
            mainState = PROG;
            break;
        case RENAME:
            midiFiles.renameFile(renameFilename, screen.saveFilename(), bank);
            mainState = SAVELOAD;
            break;
        case PLAY:
            if (!timer.isRunning()) {
                beatMutex.lock();
                beat--;
                if (beat < 0) beat = 0;
                beatMutex.unlock();
            } else {
                timer.allNotesOff();
                buttons.updateColors();
            }
            timer.playPause();  // Toggle Play/Pause
            break;
        case LAUNCH:
            launchTone--;
            if (launchTone < 0) launchTone = 11;  // Prevent negative tone
            break;
        case DAW:
        default:
            break;
    }
    changeState();
}

void function2() {
    switch (mainState) {
        case MAIN:
            if (shift && usbData) {  // If USBMSD disabled, enable it
                if (!midiFiles.getUSB()) {
                    timer.deinitUSB();
                    midiFiles.initUSB();
                }
                usbMode = midiFiles.getUSB();
                clockSource = true;
                shift = false;
            } else if (!usbMode)
                mainState = PLAY;
            break;
        case PROG:
            if (shift) {  // Go to channel mode
                prevChn = channel;
                mainState = CHANNEL;
                shift = false;
            } else {  // Toggle Play/Pause
                if (!timer.isRunning()) {
                    beatMutex.lock();
                    beat--;
                    if (beat < 0) beat = 0;
                    beatMutex.unlock();
                } else {
                    timer.allNotesOff();
                    buttons.updateColors();
                }
                timer.playPause();
            }
            break;
        case NOTE:
            octave--;
            if (octave < 0) octave = 0;  // Prevent negative octave
            break;
        case MEMORY:
        case RENAME:
            if (shift) {  // Special Characters
                if (screen.getCurPointer())
                    screen.setCurPointer(false);
                else
                    screen.setCurPointer(true);
                shift = false;
            } else {  // Shift in text
                if (screen.getUpper())
                    screen.setUpper(false);
                else
                    screen.setUpper(true);
            }
            break;
        case SAVELOAD:
            if (shift) {  // Go to renaming mode
                mainState = RENAME;
                renameFilename = screen.getFilename();
                screen.setEdit(0);
                shift = false;
            } else {
                bank--;
                if (bank < 1) bank = 8;  // Wrap around to last bank
            }
            break;
        case SCALE:
            mode--;
            if (mode < 0) mode = SCALE_COUNT - 1;  // Wrap around to last scale
            break;
        case TEMPO:
            tempoMutex.lock();
            if (shift) {  // Halve the BPM
                if (!tempo[0]) {
                    tempo[1] /= 2;
                    if (tempo[1] < 60) tempo[1] = 60;  // Prevent lower tempo
                } else
                    tempo[1] = 0;
                shift = false;
            } else {  // Set to internal clock with previous BPM
                tempo[0] = 0;
                tempo[1] = prevBPM;
            }
            tempoMutex.unlock();
            break;
        case PLAY:
            bank--;
            if (bank < 1) bank = 8;  // Wrap around to last bank
            break;
        case LAUNCH:
            if (shift) {
                launchChn--;
                if (launchChn < 0) launchChn = 15;  // Prevent negative channel
                shift = false;
            } else {
                launchMode = launchMode ? 0 : 1;
            }
            break;
        case DAW:
        default:
            break;
    }
    changeState();
}

void function3() {
    switch (mainState) {
        case MAIN:
            if (shift && usbData) {
                if (!usbMode) clockSource = false;
                shift = false;
            } else
                mainState = LAUNCH;
            break;
        case PROG:
            if (shift) {
                memcpy(prevTempo, tempo, sizeof(tempo));  // Save previous tempo
                mainState = TEMPO;
                shift = false;
            } else {  // Stop sequence
                timer.stop();
                timer.allNotesOff();
                buttons.updateColors();
                beatMutex.lock();
                beat = 0;
                beatMutex.unlock();
            }
            break;
        case NOTE:
            octave++;
            if (octave > 7) octave = 7;  // Prevent upper octave
            break;
        case MEMORY:
        case RENAME:  // Backspace
            screen.setTyping(' ');
            if (!shift) {
                screen.left();
            } else
                shift = false;
            break;
        case SAVELOAD:
            if (shift) {  // Delete selected file
                midiFiles.deleteFile(screen.getFilename(), bank);
                shift = false;
            } else {
                bank++;
                if (bank > 8) bank = 1;  // Wrap around to first bank
            }
            break;
        case SCALE:
            mode++;
            if (mode == SCALE_COUNT) mode = 0;  // Wrap around to first scale
            break;
        case TEMPO:
            tempoMutex.lock();
            if (shift) {  // Double Tempo
                if (!tempo[0]) {
                    tempo[1] *= 2;
                    if (tempo[1] > 480) tempo[1] = 480;  // Prevent upper tempo
                } else
                    tempo[1] = 2;
                shift = false;
            } else {  // Set external clock
                tempo[0] = 1;
                prevBPM = tempo[1];
                tempo[1] = 1;
            }
            tempoMutex.unlock();
            break;
        case PLAY:
            bank++;
            if (bank > 8) bank = 1;  // Wrap around to first bank
            break;
        case LAUNCH:
            if (shift) {
                launchChn++;
                if (launchChn > 15) launchChn = 0;  // Prevent >15 channel
                shift = false;
            } else {
                launchMode = launchMode ? 0 : 1;
            }
            break;
        case DAW:
        default:
            break;
    }
    changeState();
}

void function4() {
    switch (mainState) {
        case MAIN:
            if (shift && usbData) {
                clockSource = true;
                shift = false;
            } else
                mainState = DAW;
            break;
        case NOTE:
            note = prevNote;
            mainState = PROG;
            break;
        case SCALE:
            mode = prevMode;
            tone = prevTone;
            mainState = PROG;
            break;
        case TEMPO:
            tempoMutex.lock();
            memcpy(tempo, prevTempo, sizeof(tempo));  // Restore previous tempo
            tempoMutex.unlock();
            mainState = PROG;
            break;
        case CHANNEL:
            channel = prevChn;
            mainState = PROG;
            break;
        case PROG:
            if (shift) {
                prevMode = mode;
                prevTone = tone;
                mainState = SCALE;
                shift = false;
            } else {  // Enable/Disable Hold
                if (hold != 0)
                    hold = 0;
                else
                    hold = 1;
            }
            break;
        case MEMORY:  // Save current written filename
            filename = screen.saveFilename();
            mainState = SAVELOAD;
            break;
        case SAVELOAD:
            mainState = MEMORY;
            break;
        case RENAME:
            mainState = SAVELOAD;
            break;
        case PLAY:  // Stop playing
            timer.stop();
            timer.allNotesOff();
            buttons.updateColors();
            beatMutex.lock();
            beat = 0;
            beatMutex.unlock();
            break;
        case LAUNCH:
            launchTone++;
            if (launchTone > 11) launchTone = 0;  // Prevent >11 tone
            break;
        case DAW:
        default:
            break;
    }
    changeState();
}

void exitFunc() {
    switch (mainState) {
        case MAIN:
        case PROG:
        case PLAY:
        case LAUNCH:
        case DAW:
            mainState = MAIN;
            break;
        case NOTE:
            note = prevNote;
            mainState = PROG;
            break;
        case SCALE:
            mode = prevMode;
            tone = prevTone;
            mainState = PROG;
            break;
        case TEMPO:
            tempoMutex.lock();
            memcpy(tempo, prevTempo, sizeof(tempo));  // Restore previous tempo
            tempoMutex.unlock();
            mainState = PROG;
            break;
        case CHANNEL:
            channel = prevChn;
            mainState = PROG;
            break;
        case MEMORY:
            mainState = PROG;
            break;
        case SAVELOAD:
            mainState = MEMORY;
            break;
        default:
            break;
    }
    changeState();
}

void left() {
    switch (mainState) {
        case PROG:
            velocity--;
            if (velocity < 0) velocity = 0;  // Prevent lower velocity
            break;
        case NOTE:
            note--;
            if (note < 0) {
                octave--;
                note = 11;  // Wrap around to last note in lower octave
            }
            if (octave < 0) octave = 0;  // Prevent negative octave
            break;
        case SCALE:
            tone--;
            if (tone < 0) tone = 11;  // Wrap around to last tone
            break;
        case TEMPO:
            tempoMutex.lock();
            tempo[1]--;
            if (tempo[0]) {
                if (tempo[1] < 0) tempo[1] = 2;
            } else {
                if (tempo[1] < 60) tempo[1] = 60;  // Prevent lower tempo
            }
            tempoMutex.unlock();
            break;
        case CHANNEL:
            channel--;
            if (channel < 0) channel = 15;  // Wrap around to last channel
            break;
        case LAUNCH:
            launchOctave--;
            if (launchOctave < 0) launchOctave = 0;  // Prevent negative octave
            break;
        default:
            break;
    }
    screen.left();
    changeState();
}

void right() {
    switch (mainState) {
        case PROG:
            velocity++;
            if (velocity == -128) velocity = 127;  // Prevent upper velocity
            break;
        case NOTE:
            note++;
            if (note > 11) {
                octave++;
                note = 0;  // Wrap around to first note in upper octave
            }
            if (octave > 7) octave = 7;  // Prevent upper octave
            break;
        case SCALE:
            tone++;
            if (tone > 11) tone = 0;  // Wrap around to first tone
            break;
        case TEMPO:
            tempoMutex.lock();
            tempo[1]++;
            if (tempo[0]) {
                if (tempo[1] > 2) tempo[1] = 0;
            } else {
                if (tempo[1] > 480) tempo[1] = 480;  // Prevent upper tempo
            }
            tempoMutex.unlock();
            break;
        case CHANNEL:
            channel++;
            if (channel > 15) channel = 0;  // Wrap around to first channel
            break;
        case LAUNCH:
            launchOctave++;
            if (launchOctave > 6) launchOctave = 6;  // Prevent upper octave
            break;
        default:
            break;
    }
    screen.right();
    changeState();
}

void changeState() {
    screen.updateScreen();
    buttons.updateColors();
}

void timeout() {
    beatMutex.lock();
    beat++;
    if ((mode32 && beat == 32) || (!mode32 && beat == 16)) {
        beat = 0;  // Reset beat after 32 or 16 beats
        if (mainState == PLAY && queue) {
            // Load next composition
            memcpy(midiMessages, nextMessages, sizeof(midiMessages));
            memcpy(offMessages, nextOffMessages, sizeof(offMessages));
            memcpy(tempo, nextTempo, sizeof(tempo));
            filename = nextFilename;
            timer.allNotesOff();  // Stop all notes
            buttons.updateStructures();
            queue = false;
        }
    }
    timer.beatPlay();  // Send MIDI messages for the current beat
    beatMutex.unlock();
    buttons.updateColors();
}

void pollTimer() {
    while (true) {
        timer.poll();
        ThisThread::sleep_for(1ms);
    }
}

Thread timerThread(osPriorityHigh);

int main() {
    // Give the USB a moment to initialize and enumerate
    ThisThread::sleep_for(100ms);

    // Initialize components
    screen.init();
    midiFiles.init();
    timerThread.start(pollTimer);
    ledStrip.WS2812_Transfer((uint32_t)&ledData, sizeof(ledData) / sizeof(ledData[0]));  // Update LED strip
    if (!timer.getUSB()) {
        clockSource = true;
        usbData = false;
    }
    screen.updateScreen();

#if TESTING_MODES

    // Programming Test
    function1();
    ThisThread::sleep_for(500ms);

    shift = true;
    function3();
    ThisThread::sleep_for(500ms);
    function3();

    function1();
    ThisThread::sleep_for(500ms);
    function2();

#endif

    while (true) {
#if !TESTING_MODES
        readKeys();
        if (memcmp(keys, lastKeys, sizeof(keys)) != 0) {
            if (switches[1]) {
                shift = true;
            }
            if (keys[0][0] && !lastKeys[0][0]) {  // Function 3
                function3();
            }
            if (keys[0][1] && !lastKeys[0][1]) {  // Function 4
                function4();
            }
            if (keys[0][2] && !lastKeys[0][2]) {  // Function 1
                function1();
            }
            if (keys[0][3] && !lastKeys[0][3]) {  // Function 2
                function2();
            }
            for (int i = 1; i < 5; i++) {
                for (int j = 0; j < 4; j++) {
                    if (keys[i][j] && !lastKeys[i][j]) {  // Key pressed
                        uint8_t num = (i - 1) * 4 + j;
                        buttons.press(num);
                        if (!channelEnabled[num]) timer.allNotesOff(num);
                        screen.updateScreen();
                    } else if (!keys[i][j] && lastKeys[i][j] && mainState == LAUNCH) {  // Key released
                        uint8_t num = (i - 1) * 4 + j;
                        buttons.release(num);
                    }
                }
            }
            memcpy(lastKeys, keys, sizeof(keys));  // Update lastKeys
        }

        if (memcmp(switches, lastSwitches, sizeof(switches)) != 0) {
            if (switches[0] && !lastSwitches[0]) selectFunc();
            if (switches[1] && !lastSwitches[1]) {  // Reset shift timer
                shiftTimer.reset();
                shiftTimer.start();
                shift = true;
            }
            if (!switches[1] && lastSwitches[1]) {  // If exit released
                // If time pressed is less than a second, count as exit
                if (shiftTimer.elapsed_time() < std::chrono::microseconds(1000000) && shift == true) {
                    exitFunc();
                    shift = false;
                } else  // If not, quit shifting
                    shift = false;
                shiftTimer.stop();
            }
            memcpy(lastSwitches, switches, sizeof(switches));
        }

        if (encoder.getRotaryEncoder()) {
            int current = encoder.read();
            if (current == 1) right();
            if (current == -1) left();
        }
#endif
        if (midiFiles.getUSB()) midiFiles.notifyUSB();
        /*
        if (midiFiles.media_removed() && midiFiles.getUSB()) {
            shift = true;
            function1();
        }*/

        ledDataMutex.lock();
        if (memcmp(ledData, lastLedData, sizeof(ledData)) != 0) {
            ledStrip.WS2812_Transfer((uint32_t)&ledData, sizeof(ledData) / sizeof(ledData[0]));  // Update LED strip
            memcpy(lastLedData, ledData, sizeof(ledData));                                       // Update last LED data
        }
        ledDataMutex.unlock();

        // ThisThread::sleep_for(1ms);
    }
}