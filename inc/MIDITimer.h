#pragma once
#include "mbed.h"
#include "USBMIDI.h"

class MIDITimer {
    bool _running;
    us_timestamp_t _interval;
    Timer _timer;
public:
    // Constructor
    MIDITimer();

    // Start the timer with a specified interval
    void start(us_timestamp_t interval);

    // Stop the timer and reset the beat
    void stop();

    // Pause the timer
    void pause();

    // Check if the timer is currently running
    bool is_running() const;

    // Poll the timer to check if the interval has elapsed
    void poll() ;

    // Handle the timeout event
    void timeout();

    // Send the corresponding MIDI messages for the current beat
    void beatPlay();

    // Send an All Notes Off message for Channels with active notes
    void allNotesOff();
    
};