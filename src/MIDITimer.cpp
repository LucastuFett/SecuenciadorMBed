#include "MIDITimer.h"

// Global variables for MIDI messages and control
extern uint8_t midiMessages[320][3]; // MIDI messages for each beat
extern uint8_t offMessages[320][3]; // Off messages for each beat
extern uint32_t control; // Control mask for beats
extern uint8_t channels[16]; // Active channels
extern int8_t beat; // Current beat index
extern bool channelEnabled[16];
extern int16_t tempo[2]; // Tempo

extern Mutex messagesMutex;
extern Mutex controlMutex;
extern Mutex channelEnabledMutex;
extern Mutex tempoMutex;
extern Mutex beatMutex;

Mutex playingMutex;
Mutex clockMutex;

MIDITimer::MIDITimer(Callback <void()> timeoutCallback) : 
    USBMIDI(get_usb_phy(),0x0700,0x0101,1), 
    _timeoutCallback(timeoutCallback),
    _midiUART(p20, p5, 31250){
    // Constructor initializes the timer state
    _playing = false;
    _interval = 0;
    _usb = false;
    initUSB();
    _midiUART.set_format(8, BufferedSerial::None, 1);
}

void MIDITimer::initUSB(){
    USBMIDI::connect();
    ThisThread::sleep_for(500ms);
    if (configured()){
        _usb = true;
        //attach(callback(this,&MIDITimer::checkUSB));
    } 
}

void MIDITimer::deinitUSB(){
    USBMIDI::deinit();
    _usb = false;
}

bool MIDITimer::getUSB(){
    return _usb;
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
    checkMIDI();
    if (_usb) checkUSB();
    tempoMutex.lock();
    if(tempo[0] != _lastTempo[0] || tempo[1] != _lastTempo[1]) {
        // Tempo has changed
        _lastTempo[0] = tempo[0];
        _lastTempo[1] = tempo[1];
        if (!tempo[0]) {
            _interval = static_cast<us_timestamp_t>((60.0 * 1'000'000) / (tempo[1] * 2));
            if (_playing) _timer.start();
        }
        else {
            clockMutex.lock();
            _clockPulses = 0; // Reset clock pulses for external clock
            clockMutex.unlock();
            _timer.stop();
            _timer.reset();
        }
    }
    tempoMutex.unlock();
    if (!_playing) return; // If not playing, exit
    tempoMutex.lock();
    if (!tempo[0] && now >= _interval) {
        _timeoutCallback(); // Call the timeout callback
        _timer.reset();
    }else if (tempo[0]){
        clockMutex.lock();
        if (_clockPulses >= 12) { // 24 MIDI Clocks per quarter note
            _clockPulses = 0;
            _timeoutCallback(); // Call the timeout callback
        }
        clockMutex.unlock();
    }
    tempoMutex.unlock();
    
}

void MIDITimer::checkMIDI() {
    char byte;
    if (!_usb){
        while (_midiUART.read(&byte, 1)) {
            if ((byte & 0xF0) == 0xF0){ // System
                switch (byte) {
                    case 0xF8: // Timing Clock
                        clockMutex.lock();
                        _clockPulses++;
                        clockMutex.unlock();
                        break;
                    case 0xFA: // Start
                        clockMutex.lock();
                        _clockPulses = 0;
                        clockMutex.unlock();
                        beatMutex.lock();
                        beat = 0;
                        beatMutex.unlock();
                        start();
                        break;
                    case 0xFB: // Continue
                        start();
                        break;
                    case 0xFC: // Stop
                        clockMutex.lock();
                        _clockPulses = 0;
                        clockMutex.unlock();
                        stop();
                        break;
                }
            }
            else {
                // Ignore non system messages
                _midiUART.read(&byte, 2); // Read the next two bytes
            }
        }
    }

}

void MIDITimer::checkUSB() {
    MIDIMessage m;
    while(read(&m)){
        if ((m.data[1] & 0xF0) == 0xF0){ // System
            switch (m.data[1]) {
                case 0xF8: // Timing Clock
                    clockMutex.lock();
                    _clockPulses++;
                    clockMutex.unlock();
                    break;
                case 0xFA: // Start
                    clockMutex.lock();
                    _clockPulses = 0;
                    clockMutex.unlock();
                    beatMutex.lock();
                    beat = 0;
                    beatMutex.unlock();
                    start();
                    break;
                case 0xFB: // Continue
                    start();
                    break;
                case 0xFC: // Stop
                    clockMutex.lock();
                    _clockPulses = 0;
                    clockMutex.unlock();
                    stop();
                    break;
            }
        }
    }
    
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
        if (offMessages[index][0] != 0) {
            channelEnabledMutex.lock();
            if (channelEnabled[offMessages[index][0] & 0xF] == false) {
                channelEnabledMutex.unlock();
                continue; // Skip if channel is disabled
            }
            channelEnabledMutex.unlock();
            if (_usb) write(MIDIMessage::NoteOn(offMessages[index][1], offMessages[index][2], offMessages[index][0] & 0xF));
            _midiUART.write(offMessages[index], 3);
        }
        
    }
    for (int i = 0; i < 10; i++) {
        index = i * 32 + beat;
        if (midiMessages[index][0] != 0 ) {
            channelEnabledMutex.lock();
            if (channelEnabled[midiMessages[index][0] & 0xF] == false) {
                channelEnabledMutex.unlock();
                continue; // Skip if channel is disabled
            }
            channelEnabledMutex.unlock();
            if (_usb) write(MIDIMessage::NoteOn(midiMessages[index][1], midiMessages[index][2], midiMessages[index][0] & 0xF));
            _midiUART.write(midiMessages[index], 3);
        }
    }
    messagesMutex.unlock();
}

void MIDITimer::allNotesOff() {
    for (uint8_t i = 0; i < 16; i++) {
        if (channels[i] != 0) {
            if (_usb) write(MIDIMessage::AllNotesOff(i));
            uint8_t message[3] = {0, 0x7B, 0};
            message[0] = 0xB0 | (i & 0xF);
            _midiUART.write(message, 3);
        }
    }
}

void MIDITimer::allNotesOff(uint8_t channel) {
    if(_usb) write(MIDIMessage::AllNotesOff(channel));
    uint8_t message[3] = {0, 0x7B, 0};
    message[0] = 0xB0 | (channel & 0xF);
    _midiUART.write(message, 3);
}

void MIDITimer::send(uint8_t status, uint8_t data1, uint8_t data2) {
    if (((status & 0xF0) == 0x90) && _usb) write(MIDIMessage::NoteOn(data1, data2, status & 0xF));
    else if (((status & 0xF0) == 0xB0) && _usb) write(MIDIMessage::ControlChange(data1, data2, status & 0xF));
    _midiUART.write((const char[]){status, data1, data2}, 3);
}