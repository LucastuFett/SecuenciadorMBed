#include "MIDITimer.h"

// Global variables for MIDI messages and control
extern uint8_t midiMessages[320][3]; // MIDI messages for each beat
extern uint8_t offMessages[320][3]; // Off messages for each beat
extern uint32_t control; // Control mask for beats
extern uint8_t channelEnabled[16]; // Channel enable flags
extern uint8_t channels[16]; // Active channels
extern USBMIDI midi; // MIDI interface instance
extern int8_t beat; // Current beat index

MIDITimer::MIDITimer(Callback <void()> timeoutCallback) : _timeoutCallback(timeoutCallback) {
    // Constructor initializes the timer state
    _running = false;
    _interval = 0;
}

void MIDITimer::start(us_timestamp_t interval) {
    _interval = interval;
    _running = true;
    _timer.start();
}

void MIDITimer::start() {
    _running = true;
    _timer.start();
}

void MIDITimer::stop() {
    _running = false;
    _timer.stop();
    _timer.reset();
}

void MIDITimer::playPause() {
    if (_running) {
        stop();
    } else {
        start();
    }
}

bool MIDITimer::isRunning() const {
    return _running;
}

void MIDITimer::poll() {
    if (_running) {
        us_timestamp_t now = _timer.elapsed_time().count();
        if (now >= _interval) {
            timeout();
            _timer.reset();
        }
    }
}

void MIDITimer::timeout() {
    _timeoutCallback(); // Call the timeout callback
}

void MIDITimer::beatPlay() {
    uint16_t index;
    uint32_t beatMask = 0x80000000 >> beat;
    if ((control & beatMask) == 0) {
        return;
    }
    for (int i = 0; i < 10; i++) {
        index = i * 32 + beat;
        if (offMessages[index][0] != 0) {
            midi.write(MIDIMessage::NoteOn(offMessages[index][1], offMessages[index][2], offMessages[index][0] & 0xF));
        }
    }
    for (int i = 0; i < 10; i++) {
        index = i * 32 + beat;
        if (midiMessages[index][0] != 0) {
            /*
            if (channelEnabled[midiMessages[index][0] & 0xF] == 0) {
                continue; // Skip if channel is disabled
            }*/
            midi.write(MIDIMessage::NoteOn(midiMessages[index][1], midiMessages[index][2], midiMessages[index][0] & 0xF));
        }
    }
}

void MIDITimer::allNotesOff() {
    for (int i = 0; i < 16; i++) {
        if (channels[i] != 0) {
            midi.write(MIDIMessage::AllNotesOff(i));
        }
    }
}

void MIDITimer::setInterval(us_timestamp_t interval){
    _interval = interval;
    if (_running) {
        _timer.start();
    }
}
