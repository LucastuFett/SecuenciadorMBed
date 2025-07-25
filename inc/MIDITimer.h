#pragma once
#include "mbed.h"
#include "USBMIDI.h"
#include "USBPhyHw.h"
#include "usb_phy_api.h"


class MIDITimer : public USBMIDI{
    bool _playing;
    us_timestamp_t _interval;
    Timer _timer;
    Callback <void()> _timeoutCallback;
    bool _usb;

virtual const uint8_t *string_iproduct_desc() override {
        static const uint8_t custom_desc[] = {
            0x1A, STRING_DESCRIPTOR,
            'L', 0, 'u', 0, 'c', 0, 'a', 0, 's', 0, ' ', 0, 'G', 0, 'r', 0, 'o', 0, 'o', 0, 'v', 0, 'e', 0
        };
        return custom_desc;
}

public:
    // Constructor
    MIDITimer(Callback <void()> timeoutCallback);

    // Connect USBMIDI Device
    void initUSB();

    // Deinit USBMIDI Device
    void deinitUSB();

    // Get USB Status
    bool getUSB();

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