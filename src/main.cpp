#include "mbed.h"
#include <map>
#include "USBMIDI.h"
#include "MIDITimer.h"
#include "WS2812.h"
#include "Encoder.h"
#include "SPI_TFT_ILI9341.h"

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
WS2812_PIO ledStrip(p5, 8); // WS2812 PIO
Encoder encoder(p6, p7, PullUp); // Encoder for tempo adjustment
SPI_TFT_ILI9341 TFT(p3,p0,p2,p1,p5,p4,"TFT");

USBMIDI midi;

bool keys[5][4] = {{0}}; // 5 rows, 4 columns
bool lastKeys[5][4] = {{0}}; // Last state of keys

// Composition

MIDITimer timer; 

// Structs and Enums

enum state {MAIN, PROG, NOTE, MEMORY, SAVELOAD, RENAME, CHANNEL, TEMPO, SCALE, PLAY, LAUNCH, DAW};

struct scale {
    string name;
    uint8_t intervals[12];
};

struct holdKey {
    uint8_t beatStart;
    uint8_t channel;
    uint8_t midiNote;
};

// Programming Mode

const scale scales[3] = {{"Major", {2,2,1,2,2,2}}, {"Minor", {2,1,2,2,1,2}}, {"Chrom", {1,1,1,1,1,1,1,1,1,1,1}}};

enum state mainState = MAIN;
uint8_t midiMessages[320][3] = {{0}};
uint8_t offMessages[320][3] = {{0}}; 
uint32_t beatsPerTone[1536] = {0};
uint8_t beat = 0;
uint8_t tone = 0;
uint8_t mode = 0;
uint8_t note = 0;
uint8_t octave = 3;
uint8_t channel = 0;
uint8_t velocity = 127;
uint8_t channels[16] = {0};
bool mode32 = false;
bool half = false;
uint32_t control = 0;
uint16_t tempo[2] = {0,120}; // 0 = Int, 1 = Ext, in Ext, 0 = Half, 2 = Dbl
string filename = "Test";
string renameFilename = "";
uint8_t bank = 1;
uint8_t hold = 0; // 0 = No Hold, 1 = Waiting 1st, 2 = Waiting 2nd
map<holdKey,uint8_t> holded;

// UI Variables

bool shift = false;
uint8_t prevNote = 0; // Previous note for the UI
uint8_t prevMode = 0; // Previous mode for the UI
uint8_t prevChn = 0; // Previous channel for the UI
uint8_t prevTone = 0; // Previous tone for the UI
uint16_t prevTempo[2] = {0,120};

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

}

void function1() {
    // Function 1 implementation
}

void function2() {
    // Function 2 implementation
}

void function3() {
    // Function 3 implementation
}

void function4() {
    // Function 4 implementation
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

void updateTempo() {
    tempo[1] = static_cast<uint16_t>(tempoPot.read() * 350);
    if (tempo[1] < 120) tempo[1] = 120;
    timer.start(static_cast<us_timestamp_t>((60.0 * 1'000'000) / tempo[1]));
}

*/

void checkTempo() {
    timer.start(static_cast<us_timestamp_t>((60.0 * 1'000'000) / tempo[0])); // Default tempo
}

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
    encthread.start(do_enc);


    TFT.claim(stdout);      // send stdout to the TFT display
    //TFT.claim(stderr);      // send stderr to the TFT display
    TFT.set_orientation(1);
    TFT.background(Green);    // set background to Green
    TFT.foreground(White);    // set chars to white
    TFT.cls();                // clear the screen

    midi.write(MIDIMessage::PitchWheel(TFT.Read_ID() & 0xFFF,0)); // Send TFT width as CC message

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