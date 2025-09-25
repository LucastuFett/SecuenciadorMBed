#pragma once
#include "USBMIDI.h"
#include "USBPhyHw.h"
#include "mbed.h"
#include "usb_phy_api.h"

class MIDITimer : public USBMIDI {
    bool _playing;
    us_timestamp_t _interval;
    Timer _timer;
    Callback<void()> _timeoutCallback;
    bool _usb;
    int16_t _lastTempo[2] = {0, 120};
    BufferedSerial _midiUART;
    uint8_t _clockPulses = 0;

    Thread _usbThread;
    EventQueue _usbQueue;

    virtual const uint8_t* string_iproduct_desc() override {
        static const uint8_t custom_desc[] = {
            0x1A, STRING_DESCRIPTOR,
            'L', 0, 'u', 0, 'c', 0, 'a', 0, 's', 0, ' ', 0, 'G', 0, 'r', 0, 'o', 0, 'o', 0, 'v', 0, 'e', 0};
        return custom_desc;
    }

    virtual const uint8_t* configuration_desc(uint8_t index) override {
        const uint8_t* base = USBMIDI::configuration_desc(index);
        if (!base) return nullptr;

        const uint16_t len = static_cast<uint16_t>(base[2]) | (static_cast<uint16_t>(base[3]) << 8);

        // Make a local copy we can edit; 0x65 is the current size in mbed's USBMIDI
        static uint8_t cfg[0x65];
        const uint16_t n = (len <= sizeof(cfg)) ? len : sizeof(cfg);
        memcpy(cfg, base, n);

        // cfg[7] = bmAttributes; cfg[8] = bMaxPower (units of 2 mA)
        // Example 1: Bus-powered, 100 mA
        cfg[7] = 0xC0;
        cfg[8] = 100;  // 100 * 2 mA = 200 mA

        return cfg;
    }

    // Start the timer with saved interval
    void start();

    // Check MIDI Messages
    void checkMIDI();

    // Check MIDI Messages from USB
    void checkUSB();

    // ISR management
    void onUSBMessageISR();

   public:
    // Constructor
    MIDITimer(Callback<void()> timeoutCallback);

    // Connect USBMIDI Device
    void initUSB();

    // Deinit USBMIDI Device
    void deinitUSB();

    // Get USB Status
    bool getUSB();

    // Stop the timer and reset the beat
    void stop();

    // Play/Pause the timer
    void playPause();

    // Check if the timer is currently running
    bool isRunning() const;

    // Poll the timer to check if the interval has elapsed
    void poll();

    // Send the corresponding MIDI messages for the current beat
    void beatPlay();

    // Send an All Notes Off message for Channels with active notes
    void allNotesOff();

    // Send an All Notes Off for just one channel
    void allNotesOff(uint8_t channel);

    // Send Arbitrary MIDI Message
    void send(uint8_t status, uint8_t data1, uint8_t data2);
};