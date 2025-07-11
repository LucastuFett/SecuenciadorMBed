#include "mbed.h"
#include <map>
#include "USBMIDI.h"
#include "MIDITimer.h"
#include "Buttons.h"
#include "WS2812.h"
#include "Encoder.h"
#include "ILI9341.h"

#define SCALE_COUNT 3

// Hardware

DigitalIn columns[4] = {
    DigitalIn(p21, PullDown),
    DigitalIn(p22, PullDown),
    DigitalIn(p26, PullDown),
    DigitalIn(p27, PullDown)
};

DigitalOut rows[5] ={
    DigitalOut(p16),
    DigitalOut(p17),
    DigitalOut(p18),
    DigitalOut(p19),
    DigitalOut(p20)
};

DigitalIn exitSW(p15, PullDown);
DigitalIn selectSW(p14, PullDown);

DigitalIn toggle32(p28, PullNone);

BufferedSerial midiUART(p12, p13, 31250); // MIDI UART
//WS2812_PIO ledStrip(p5, 8); // WS2812 PIO
Encoder encoder(p6, p7, PullUp); // Encoder for tempo adjustment
ILI9341 TFT(p3,p0,p2,p1,p5,p4);

USBMIDI midi;

bool keys[5][4] = {{0}}; // 5 rows, 4 columns
bool lastKeys[5][4] = {{0}}; // Last state of keys

// Function Declarations
void readKeys();
void select();
void function1();
void function2();
void function3();
void function4();
void exit();
void left();
void right();
void changeState();
void tempoChange();
void timeout();

// Composition

MIDITimer timer(callback(timeout)); 

// Structs and Enums

enum state {MAIN, PROG, NOTE, MEMORY, SAVELOAD, RENAME, CHANNEL, TEMPO, SCALE, PLAY, LAUNCH, DAW};

struct scale {
    string name;
    uint8_t intervals[12];
};

// Programming Mode

const scale scales[SCALE_COUNT] = {{"Major", {2,2,1,2,2,2}}, {"Minor", {2,1,2,2,1,2}}, {"Chrom", {1,1,1,1,1,1,1,1,1,1,1}}};

enum state mainState = MAIN;
uint8_t midiMessages[320][3] = {{0}};
uint8_t offMessages[320][3] = {{0}}; 
uint32_t beatsPerTone[1536] = {0};
int8_t beat = 0;
int8_t tone = 0;
int8_t mode = 0;
int8_t note = 0;
int8_t octave = 3;
int8_t channel = 0;
int8_t velocity = 127;
uint8_t channels[16] = {0};
bool mode32 = false;
bool half = false;
uint32_t control = 0;
int16_t tempo[2] = {0,120}; // 0 = Int, 1 = Ext, in Ext, 0 = Half, 2 = Dbl
string filename = "Test";
string renameFilename = "";
uint8_t bank = 1;
uint8_t hold = 0; // 0 = No Hold, 1 = Waiting 1st, 2 = Waiting 2nd
map<holdKey,uint8_t, CompareHoldKey> holded;

// UI Variables

bool shift = false;
int8_t prevNote = 0; // Previous note for the UI
int8_t prevMode = 0; // Previous mode for the UI
int8_t prevChn = 0; // Previous channel for the UI
int8_t prevTone = 0; // Previous tone for the UI
int16_t prevTempo[2] = {0,120};

// Play Mode

uint8_t nextMessages[320][3] = {{0}};
uint8_t nextOffMessages[320][3] = {{0}}; 
uint16_t nextTempo[2] = {0,120}; // 0 = Int, 1 = Ext, in Ext, 0 = Half, 2 = Dbl
string nextFilename = "";
bool queue = false;

// Methods

Mutex keysMutex;

void readKeys() {
    for (int i = 0; i < 5; i++) {
        rows[i] = 1; // Set current row high
        ThisThread::sleep_for(1ms); // Wait for the row to stabilize
        keysMutex.lock(); // Lock the mutex to prevent concurrent access
        for (int j = 0; j < 4; j++) {
            keys[i][j] = columns[j].read();
        }
        keysMutex.unlock(); // Unlock the mutex after writing to keys
        rows[i] = 0; // Set current row low
    }
}

void select() {
    switch (mainState){
        case NOTE:
        case SCALE:
        case TEMPO:
            mainState = PROG;
            break;
        case PROG:
            if (mode32) half = !half;
            break;
        case MEMORY:
        case RENAME:
            // Select Letter
            break;
        case SAVELOAD:
            // Get Filename
            break;
        case PLAY:
            if (timer.isRunning()) {
                // Next Filename and Read
                queue = true;
            } else {
                // Get Filename and Read
                // Update Structs
            }
            break;
        default:
            break;
    }
    changeState();
}

void function1() {
    switch (mainState) {
        case MAIN:
        case NOTE:
        case SCALE:
        case TEMPO:
        case CHANNEL:
            mainState = PROG;
            break;
        case PROG:
            if (shift) {
                // Update Text, Edit = 0
                mainState = MEMORY;
                shift = false;
            } else {
                mainState = NOTE;
                prevNote = note;
            }
            break;
        case MEMORY:
            // Save Filename and File
            mainState = PROG;
            break;
        case SAVELOAD:
            // Read from File
            mainState = PROG;
            break;
        case RENAME:
            // Rename File
            mainState = SAVELOAD;
            break;
        case PLAY:
            if (!timer.isRunning()) {
                beat --;
            } else {
                timer.allNotesOff();
                // Update Colors
            }
            timer.playPause(); // Toggle Play/Pause
            break;
        default:
            break;
    }
    changeState();
}

void function2() {
    switch (mainState) {
        case MAIN:
            mainState = PLAY;
            break;
        case PROG:
            if (shift) {
                prevChn = channel;
                mainState = CHANNEL;
                shift = false;
            } else {
                if (!timer.isRunning()) {
                    beat --;
                } else {
                    timer.allNotesOff();
                    // Update Colors
                }
                timer.playPause(); // Toggle Play/Pause
            }
            break;
        case NOTE:
            octave --;
            if (octave < 0) octave = 0; // Prevent negative octave
            break;
        case SCALE:
            mode --;
            if (mode < 0) mode = SCALE_COUNT - 1; // Wrap around to last scale
            break;
        case TEMPO:
            tempo[0] = 0;
            break;
        case MEMORY:
        case RENAME:
            if (shift) {
                // Change Pointer
            } else {
                // Change Upper
            }
            break;
        case SAVELOAD:
            if (shift) {
                mainState = RENAME;
                // Rename Filename
            } else {
                bank --;
                if (bank < 1) bank = 8; // Wrap around to last bank
            }
            break;
        case PLAY:
            bank --;
            if (bank < 1) bank = 8; // Wrap around to last bank
            break;
        default:
            break;
    }
    changeState();
}

void function3() {
    switch (mainState) {
        case PROG:
            if (shift) {
                memcpy(prevTempo, tempo, sizeof(tempo)); // Save previous tempo
                mainState = TEMPO;
                shift = false;
            } else {
                timer.stop();
                timer.allNotesOff();
                // Update Colors
                beat = 0;
            }
            break;
        case NOTE:
            octave ++;
            if (octave > 7) octave = 7; // Prevent upper octave
            break;
        case SCALE:
            mode ++;
            if (mode == SCALE_COUNT) mode = 0; // Wrap around to first scale
            break;
        case TEMPO:
            tempo[0] = 1;
            break;
        case MEMORY:
        case RENAME:
            // Set Text to ""
            if (!shift) {
                // Left
            }
            break;
        case SAVELOAD:
            if (shift) {
                // Delete
            } else {
                bank ++;
                if (bank > 8) bank = 1; // Wrap around to first bank
            }
            break;
        case PLAY:
            bank ++;
            if (bank > 8) bank = 1; // Wrap around to first bank
            break;
        default:
            break;
    }
    changeState();
}

void function4() {
    switch (mainState) {
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
            memcpy(tempo, prevTempo, sizeof(tempo)); // Restore previous tempo
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
            } else {
                if (hold != 0) hold = 0;
                else hold = 1;
            }
            break;
        case MEMORY:
            // Save Filename
            mainState = SAVELOAD;
            break;
        case SAVELOAD:
            mainState = MEMORY;
            break;
        case RENAME:
            mainState = SAVELOAD;
            break;
        case PLAY:
            timer.stop();
            timer.allNotesOff();
            // Update Colors
            beat = 0;
            break;
        default:
            break;
    }
    changeState();
}

void exit() {
    switch (mainState){
		case PROG:
        case PLAY:
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
			memcpy(tempo, prevTempo, sizeof(tempo)); // Restore previous tempo
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
            velocity --;
            if (velocity < 0) velocity = 0; // Prevent lower velocity
            break;
        case NOTE:
            note --;
            if (note < 0) {
                octave --;
                note = 11; // Wrap around to last note in lower octave
            }
            if (octave < 0) octave = 0; // Prevent negative octave
            break;
        case SCALE:
            tone --;
            if (tone < 0) tone = 11; // Wrap around to last tone
            break;
        case TEMPO:
            tempo[1] --;
            if (tempo[0]) {
                if (tempo[1] < 0) tempo[1] = 2;
            } else {
                if (tempo[1] < 60) tempo[1] = 360; // Prevent lower tempo
            }
            break;
        case CHANNEL:
            channel --;
            if (channel < 0) channel = 15; // Wrap around to last channel
            break;
        default:
            break;
    }
    // Screen Left
    changeState();
}

void right() {
    switch (mainState) {
        case PROG:
            velocity ++;
            if (velocity == -128) velocity = 127; // Prevent upper velocity
            break;
        case NOTE:
            note ++;
            if (note > 11) {
                octave ++;
                note = 0; // Wrap around to first note in upper octave
            }
            if (octave > 7) octave = 7; // Prevent upper octave
            break;
        case SCALE:
            tone ++;
            if (tone > 11) tone = 0; // Wrap around to first tone
            break;
        case TEMPO:
            tempo[1] ++;
            if (tempo[0]) {
                if (tempo[1] > 2) tempo[1] = 0;
            } else {
                if (tempo[1] > 360) tempo[1] = 60; // Prevent upper tempo
            }
            break;
        case CHANNEL:
            channel ++;
            if (channel > 15) channel = 0; // Wrap around to first channel
            break;
        default:
            break;
    }
    // Screen Right
    changeState();
}

void changeState() {
    // Update Screen
    // Update Buttons Colors
    tempoChange();
}

void tempoChange() {
    if (!tempo[0]) {
        timer.setInterval(static_cast<us_timestamp_t>((60.0 * 1'000'000) / tempo[1])); // Internal tempo
    }
}

void timeout() {
    beat ++;
    if ((mode32 && beat == 32) || (!mode32 && beat == 16)) {
        beat = 0; // Reset beat after 32 or 16 beats
        if (mainState == PLAY && queue) {
            // Load next composition
            memcpy(midiMessages, nextMessages, sizeof(midiMessages));
            memcpy(offMessages, nextOffMessages, sizeof(offMessages));
            memcpy(tempo, nextTempo, sizeof(tempo));
            filename = nextFilename;
            timer.allNotesOff(); // Stop all notes
            // Update Structures
            queue = false;
        }
    }
    timer.beatPlay(); // Send MIDI messages for the current beat
    // Update Colors
}

/*
Mutex ledDataMutex;

uint32_t ledData[8] = {
    0xFF000000, // Channel 1 - Green
    0xFF000000, // Channel 2 - Green
    0x00FF0000, // Channel 3 - Red
    0x00FF0000, // Channel 4 - Red
    0x0000FF00, // Channel 5 - Blue
    0x0000FF00, // Channel 6 - Blue
    0x77777700, // Channel 7 - Off
    0x77007700  // Channel 8 - Off
};

uint32_t lastLedData[8] = {
    0xFF000000, // Channel 1 - Green
    0xFF000000, // Channel 2 - Green
    0x00FF0000, // Channel 3 - Red
    0x00FF0000, // Channel 4 - Red
    0x0000FF00, // Channel 5 - Blue
    0x0000FF00, // Channel 6 - Blue
    0x77777700, // Channel 7 - Off
    0x77007700  // Channel 8 - Off
}; // Last state of LEDs

*/
/*
void do_enc(void)
{
    int evalue=0;
    while (1)
    {
        if (encoder.getRotaryEncoder()){
            int current=encoder.read();
            if (current != 0)
                evalue += current; // Increment or decrement based on encoder direction
            midi.write(MIDIMessage::ControlChange(0,evalue & 0x7F)); // Send encoder value as CC message
        }
        ThisThread::sleep_for(1ms);
    }
}

Thread encthread;    // thread for encoder
*/
/*

void changeLED(void){
    while(1) {
        ledDataMutex.lock();
        ledData[0] ^= 0x00FF0000;
        ledData[1] ^= 0x0000FF00;
        ledData[2] ^= 0xFF000000;
        ledData[3] ^= 0x0000FF00;
        ledData[4] ^= 0xFF000000;
        ledData[5] ^= 0x00FF0000;
        ledDataMutex.unlock();
        ThisThread::sleep_for(500ms);
    }
}

Thread ledThread;
*/

int main()
{
    // Give the USB a moment to initialize and enumerate
    while (!midi.ready()) {
        ThisThread::sleep_for(100ms);
    }
    // Set and Attach Trigger
    timer.start(static_cast<us_timestamp_t>((60.0 * 1'000'000) / tempo[0]));
    //ledStrip.WS2812_Transfer((uint32_t)&ledData, sizeof(ledData) / sizeof(ledData[0])); // Update LED strip
    //ledThread.start(changeLED); // Start LED change thread
    //encthread.start(do_enc);
    
    TFT.initialize();
    TFT.setRotation(1); // Set rotation to 1
    TFT.fillRectangle(100,40,160,160,BLUE); // Clear the screen with black background
    TFT.drawString(0,0,"Hello World Hello Hello Hello", 30, 2, WHITE, GREEN); // Draw a string on the screen
    
    /*
    TFT.claim(stdout);      // send stdout to the TFT display
    //TFT.claim(stderr);      // send stderr to the TFT display
    TFT.set_orientation(1);
    TFT.background(Red);    // set background to Green
    TFT.foreground(White);    // set chars to white
    TFT.fillrect(0,0,120,160,Blue);                // clear the screen
    */
    //midi.write(MIDIMessage::PitchWheel(TFT.Read_ID() & 0xFFF,0)); // Send TFT width as CC message
    /*
        TFT.cls();
    TFT.set_font((unsigned char*) Arial24x23);
    TFT.locate(100,100);
    TFT.printf("Graphic");

    TFT.line(0,0,100,0,Green);
    TFT.line(0,0,0,200,Green);
    TFT.line(0,0,100,200,Green);

    TFT.rect(100,50,150,100,Red);
    TFT.fillrect(180,25,220,70,Blue);

    TFT.circle(80,150,33,White);
    TFT.fillcircle(160,190,20,Yellow);
    
    double s;

    for (int i=0; i<320; i++) {
        s =20 * sin((long double) i / 10 );
        TFT.pixel(i,100 + (int)s ,Red);
    }
    */
   
    while (true) {
        
        timer.poll();
        readKeys();
        if (memcmp(keys, lastKeys, sizeof(keys)) != 0) {
            if (keys[0][0] && !lastKeys[0][0]) { // Function 1
                function1();
            }
            if (keys[0][1] && !lastKeys[0][1]) { // Function 2
                function2();
            }
            if (keys[0][2] && !lastKeys[0][2]) { // Function 3
                function3();
            }
            if (keys[0][3] && !lastKeys[0][3]) { // Function 4
                function4();
            }
            for(int i = 1; i < 5; i++) {
                for(int j = 0; j < 4; j++) {
                    if (keys[i][j] && !lastKeys[i][j]) { // Key pressed
                        // Handle key press logic here
                    }
                }
            }
            memcpy(lastKeys, keys, sizeof(keys)); // Update lastKeys
        }
       /*
        ledDataMutex.lock();
        if (memcmp(ledData, lastLedData, sizeof(ledData)) != 0) {
            ledStrip.WS2812_Transfer((uint32_t)&ledData, sizeof(ledData) / sizeof(ledData[0])); // Update LED strip
            memcpy(lastLedData, ledData, sizeof(ledData)); // Update last LED data
        }
        ledDataMutex.unlock();*/
    }
}