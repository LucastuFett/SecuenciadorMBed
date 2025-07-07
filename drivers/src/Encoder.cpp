/*

Public Domain 2022 Al Williams
2021 Ralph Bacon

 */

#include "Encoder.h"


// Our constructor
Encoder::Encoder(PinName pinA, PinName pinB, PinMode mode) :
    APin(pinA, mode),
    BPin(pinB, mode)
{
// set up interrupts
    APin.rise(callback(this, &Encoder::isrCheck));
    APin.fall(callback(this, &Encoder::isrCheck));
    BPin.rise(callback(this, &Encoder::isrCheck));
    BPin.fall(callback(this, &Encoder::isrCheck));
}

// Read a value. May or may not clear the count depending on accumulate
int Encoder::read()
{
    // Reset the flag that brought us here (from ISR)
    rotaryEncoder = false;

    static uint8_t lrmem = 3;
    static int lrsum = 0;
    static int8_t TRANS[] = {0, -1, 1, 14, 1, 0, 14, -1, -1, 14, 0, 1, 14, 1, -1, 0};

    // Read BOTH pin states to deterimine validity of rotation (ie not just switch bounce)
    int8_t l = APin.read();
    int8_t r = BPin.read();

    // Move previous value 2 bits to the left and add in our new values
    lrmem = ((lrmem & 0x03) << 2) + 2 * l + r;

    // Convert the bit pattern to a movement indicator (14 = impossible, ie switch bounce)
    lrsum += TRANS[lrmem];

    /* encoder not in the neutral (detent) state */
    if (lrsum % 4 != 0)
    {
        return 0;
    }

    /* encoder in the neutral state - clockwise rotation*/
    if (lrsum == 4)
    {
        lrsum = 0;
        return 1;
    }

    /* encoder in the neutral state - anti-clockwise rotation*/
    if (lrsum == -4)
    {
        lrsum = 0;
        return -1;
    }

    // An impossible rotation has been detected - ignore the movement
    lrsum = 0;
    return 0;
}


void Encoder::isrCheck()
{
    rotaryEncoder = true;
}




