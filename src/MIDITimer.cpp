#include "MIDITimer.h"

// Global variables for MIDI messages and control
extern uint8_t midiMessages[320][3]; // MIDI messages for each beat
extern uint8_t offMessages[320][3]; // Off messages for each beat
extern uint32_t control; // Control mask for beats
extern uint8_t channelEnabled[16]; // Channel enable flags
extern uint8_t channels[16]; // Active channels
extern int8_t beat; // Current beat index
extern Mutex messagesMutex;
extern Mutex controlMutex;

Mutex intervalMutex;
Mutex playingMutex;

MIDITimer::MIDITimer(Callback <void()> timeoutCallback) : USBMIDI(get_usb_phy(),0x0700,0x0101,1), _timeoutCallback(timeoutCallback){
    // Constructor initializes the timer state
    _playing = false;
    _interval = 0;
    _usb = false;
}

void MIDITimer::initUSB(){
    USBMIDI::connect();
    while (!USBMIDI::ready()) {
        ThisThread::sleep_for(100ms);
    }
    _usb = true;
}

void MIDITimer::deinitUSB(){
    USBMIDI::deinit();
    _usb = false;
}

bool MIDITimer::getUSB(){
    return _usb;
}

void MIDITimer::start(us_timestamp_t interval) {
    intervalMutex.lock();
    _interval = interval;
    intervalMutex.unlock();
    playingMutex.lock();
    _playing = true;
    playingMutex.unlock();
    _timer.start();
}

void MIDITimer::start() {
    playingMutex.lock();
    _playing = true;
    playingMutex.unlock();
    _timer.start();
}

void MIDITimer::stop() {
    playingMutex.lock();
    _playing = false;
    playingMutex.unlock();
    _timer.stop();
    _timer.reset();
}

void MIDITimer::playPause() {
    if (_playing) {
        stop();
    } else {
        start();
    }
}

bool MIDITimer::isRunning() const {
    return _playing;
}

void MIDITimer::poll() {
    us_timestamp_t now = _timer.elapsed_time().count();
    intervalMutex.lock();
    if (now >= _interval) {
        timeout();
        _timer.reset();
    }
    intervalMutex.unlock();
    
}

void MIDITimer::timeout() {
    _timeoutCallback(); // Call the timeout callback
}

void MIDITimer::beatPlay() {
    uint16_t index;
    uint32_t beatMask = 0x80000000 >> beat;
    messagesMutex.lock();
    controlMutex.lock();
    if ((control & beatMask) == 0) {
        controlMutex.unlock();
        messagesMutex.unlock();
        return;
    }
    controlMutex.unlock();
    for (int i = 0; i < 10; i++) {
        index = i * 32 + beat;
        if (offMessages[index][0] != 0 && _usb) {
            write(MIDIMessage::NoteOn(offMessages[index][1], offMessages[index][2], offMessages[index][0] & 0xF));
        }
    }
    for (int i = 0; i < 10; i++) {
        index = i * 32 + beat;
        if (midiMessages[index][0] != 0 && _usb) {
            /*
            if (channelEnabled[midiMessages[index][0] & 0xF] == 0) {
                continue; // Skip if channel is disabled
            }*/
            write(MIDIMessage::NoteOn(midiMessages[index][1], midiMessages[index][2], midiMessages[index][0] & 0xF));
        }
    }
    messagesMutex.unlock();
}

void MIDITimer::allNotesOff() {
    for (int i = 0; i < 16; i++) {
        if (channels[i] != 0 && _usb) {
            write(MIDIMessage::AllNotesOff(i));
        }
    }
}

void MIDITimer::setInterval(us_timestamp_t interval){
    intervalMutex.lock();
    _interval = interval;
    intervalMutex.unlock();
    if (_playing) {
        _timer.start();
    }
}
