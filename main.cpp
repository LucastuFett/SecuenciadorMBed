#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE_MS     50

int main()
{
    // Initialise the digital pin LED1 as an output
    DigitalOut led(LED1);

    while (true) {
        led = !led;           
        printf("Blinky\r\n");
        ThisThread::sleep_for(BLINKING_RATE_MS);
    }
}
