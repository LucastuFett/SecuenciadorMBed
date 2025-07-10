#pragma once
#include "mbed.h"
#include "USBMIDI.h"

class MIDITimer {
    bool _running;
    us_timestamp_t _interval;
    Timer _timer;
    Callback <void()> _timeoutCallback;
public:
    // Constructor
    MIDITimer(Callback <void()> timeoutCallback);

    // Start the timer with a specified interval
    void start(us_timestamp_t interval);

    // Start the timer with saved interval
    void start();

    // Stop the timer and reset the beat
    void stop();

    // Play/Pause the timer
    void playPause();

    // Check if the timer is currently running
    bool isRunning() const;

    // Poll the timer to check if the interval has elapsed
    void poll() ;

    // Handle the timeout event
    void timeout();

    // Send the corresponding MIDI messages for the current beat
    void beatPlay();

    // Send an All Notes Off message for Channels with active notes
    void allNotesOff();

    // Set Saved Interval
    void setInterval(us_timestamp_t interval);
    
};