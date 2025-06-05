#include "mbed.h"
//#include "USBMIDI.h"
#include "USBSerial.h"


// Blinking rate in milliseconds
#define BLINKING_RATE_MS     50
//USBMIDI midi;
USBSerial serial(false); // false for non-blocking

int main()
{
    // Give the USB a moment to initialize and enumerate
    ThisThread::sleep_for(5s);
    serial.printf("Hello from BluePill USB!\r\n");
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
       // Echo characters received over USBSerial
        if (serial.readable()) {
            serial.putc(serial.getc());
        }
        serial.printf("Blinky and USBSerial test...\r\n");
        ThisThread::sleep_for(BLINKING_RATE_MS);
    }
}