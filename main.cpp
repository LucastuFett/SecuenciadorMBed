#include "mbed.h"
#include "USBMIDI.h"

// Blinking rate in milliseconds
#define BLINKING_RATE_MS     500
USBMIDI midi;

int main()
{
    // Initialise the digital pin LED1 as an output
    DigitalOut led(PC_13);
    while (true) {
        led = !led;           
        /*
        for(int i=48; i<83; i++) {     // send some messages!
            midi.write(MIDIMessage::NoteOn(i));
            ThisThread::sleep_for(50ms);
            midi.write(MIDIMessage::NoteOff(i));
            ThisThread::sleep_for(50ms);
        }*/
        ThisThread::sleep_for(BLINKING_RATE_MS);
    }
}
