#include "MIDITimer.h"

// Global variables for MIDI messages and control
extern uint8_t midiMessages[320][3];  // MIDI messages for each beat
extern uint8_t offMessages[320][3];   // Off messages for each beat
extern uint32_t control;              // Control mask for beats
extern uint8_t channels[16];          // Active channels
extern int8_t beat;                   // Current beat index
extern bool channelEnabled[16];
extern int16_t tempo[2];  // Tempo
extern bool usbMode;
extern bool clockSource;

extern Mutex messagesMutex;
extern Mutex controlMutex;
extern Mutex channelEnabledMutex;
extern Mutex tempoMutex;
extern Mutex beatMutex;

Mutex playingMutex;
Mutex clockMutex;

MIDITimer::MIDITimer(Callback<void()> timeoutCallback) : USBMIDI(get_usb_phy(), 0x0700, 0x0101, 1),
                                                         _timeoutCallback(timeoutCallback),
                                                         _midiUART(p20, p5, 31250),
                                                         _usbQueue(32 * EVENTS_EVENT_SIZE) {
    // Constructor initializes the timer state
    _playing = false;
    _usb = false;
    _interval = static_cast<us_timestamp_t>((60.0 * 1'000'000) / (tempo[1] * 2));

    USBMIDI::connect();
    ThisThread::sleep_for(500ms);
    if (configured()) {
        _usb = true;
        attach(callback(this, &MIDITimer::onUSBMessageISR));
        _usbThread.start(callback(&_usbQueue, &EventQueue::dispatch_forever));
        _usbThread.set_priority(osPriorityHigh);
    }

    _midiUART.set_format(8, BufferedSerial::None, 1);
    _midiUART.set_flow_control(BufferedSerial::Disabled);
}

void MIDITimer::initUSB() {
    USBMIDI::connect();
    ThisThread::sleep_for(500ms);
    if (configured()) {
        _usb = true;
        _usbThread.~Thread();
        new (&_usbThread) Thread();
        _usbThread.start(callback(&_usbQueue, &EventQueue::dispatch_forever));
        _usbThread.set_priority(osPriorityHigh);
    }
}

void MIDITimer::deinitUSB() {
    USBMIDI::deinit();
    _usb = false;
    _usbQueue.break_dispatch();
    _usbThread.join();
}

bool MIDITimer::getUSB() {
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
        beatMutex.lock();
        beatPlay();
        beatMutex.unlock();
    }
}

bool MIDITimer::isRunning() const {
    return _playing;
}

void MIDITimer::poll() {
    us_timestamp_t now = _timer.elapsed_time().count();
    checkMIDI();
    tempoMutex.lock();
    if (tempo[0] != _lastTempo[0] || tempo[1] != _lastTempo[1]) {
        // Tempo has changed
        _lastTempo[0] = tempo[0];
        _lastTempo[1] = tempo[1];
        if (!tempo[0]) {
            _interval = static_cast<us_timestamp_t>((60.0 * 1'000'000) / (tempo[1] * 2));
            if (_playing) _timer.start();
        } else {
            clockMutex.lock();
            _clockPulses = 0;  // Reset clock pulses for external clock
            clockMutex.unlock();
            _timer.stop();
            _timer.reset();
        }
    }
    tempoMutex.unlock();
    if (!_playing) return;  // If not playing, exit
    tempoMutex.lock();
    if (!tempo[0] && now >= _interval) {
        _timeoutCallback();  // Call the timeout callback
        _timer.reset();
    } else if (tempo[0]) {
        clockMutex.lock();
        uint8_t expectedPulses = 24 >> tempo[1];
        if (_clockPulses >= expectedPulses) {
            _clockPulses = 0;
            _timeoutCallback();  // Call the timeout callback
        }
        clockMutex.unlock();
    }
    tempoMutex.unlock();
}

void MIDITimer::onUSBMessageISR() {
    // Post an event to process USB messages outside ISR
    _usbQueue.call(callback(this, &MIDITimer::checkUSB));
}

void MIDITimer::checkUSB() {
    MIDIMessage m;
    if (read(&m) && !clockSource) {
        if ((m.data[1] & 0xF0) == 0xF0) {  // System
            switch (m.data[1]) {
                case 0xF8:  // Timing Clock
                    clockMutex.lock();
                    _clockPulses++;
                    clockMutex.unlock();
                    break;
                case 0xFA:  // Start
                    clockMutex.lock();
                    _clockPulses = 0;
                    clockMutex.unlock();
                    beatMutex.lock();
                    beat = 0;
                    beatMutex.unlock();
                    start();
                    break;
                case 0xFB:  // Continue
                    start();
                    break;
                case 0xFC:  // Stop
                    clockMutex.lock();
                    _clockPulses = 0;
                    clockMutex.unlock();
                    stop();
                    allNotesOff();
                    break;
                default:
                    break;
            }
        }
    }
}

void MIDITimer::checkMIDI() {
    char byte;
    if (clockSource) {
        while (_midiUART.read(&byte, 1)) {
            if ((byte & 0xF0) == 0xF0) {  // System
                switch (byte) {
                    case 0xF0:  // Start of SysEx
                        // Read until End of SysEx (0xF7) or timeout
                        {
                            char sysExByte;
                            do {
                                if (_midiUART.read(&sysExByte, 1) == 0) break;  // Timeout or no data
                            } while (sysExByte != 0xF7);
                        }
                        break;
                    case 0xF1:
                    case 0xF3:
                        char ignore1;
                        _midiUART.read(&ignore1, 1);  // Read the next byte
                        break;
                    case 0xF2:
                        char ignore2[2];
                        _midiUART.read(&ignore2, 2);  // Read the next two bytes
                        break;
                    case 0xF8:  // Timing Clock
                        clockMutex.lock();
                        _clockPulses++;
                        clockMutex.unlock();
                        break;
                    case 0xFA:  // Start
                        clockMutex.lock();
                        _clockPulses = 0;
                        clockMutex.unlock();
                        beatMutex.lock();
                        beat = 0;
                        beatMutex.unlock();
                        start();
                        break;
                    case 0xFB:  // Continue
                        start();
                        break;
                    case 0xFC:  // Stop
                        clockMutex.lock();
                        _clockPulses = 0;
                        clockMutex.unlock();
                        stop();
                        allNotesOff();
                        break;
                    default:
                        break;
                }
            } else {
                // Ignore non system messages
                if ((byte & 0xF0) == 0xC0 || (byte & 0xF0) == 0xD0) {
                    char ignore[1];
                    _midiUART.read(&ignore, 1);  // Read the next byte
                    continue;
                }
                char ignore[2];
                _midiUART.read(&ignore, 2);  // Read the next two bytes
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
                continue;  // Skip if channel is disabled
            }
            channelEnabledMutex.unlock();
            if (_usb) write(MIDIMessage::NoteOn(offMessages[index][1], offMessages[index][2], offMessages[index][0] & 0xF));
            _midiUART.write(offMessages[index], 3);
        }
    }
    for (int i = 0; i < 10; i++) {
        index = i * 32 + beat;
        if (midiMessages[index][0] != 0) {
            channelEnabledMutex.lock();
            if (channelEnabled[midiMessages[index][0] & 0xF] == false) {
                channelEnabledMutex.unlock();
                continue;  // Skip if channel is disabled
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
    if (_usb) write(MIDIMessage::AllNotesOff(channel));
    uint8_t message[3] = {0, 0x7B, 0};
    message[0] = 0xB0 | (channel & 0xF);
    _midiUART.write(message, 3);
}

void MIDITimer::send(uint8_t status, uint8_t data1, uint8_t data2) {
    if (((status & 0xF0) == 0x90) && _usb)
        write(MIDIMessage::NoteOn(data1, data2, status & 0xF));
    else if (((status & 0xF0) == 0xB0) && _usb)
        write(MIDIMessage::ControlChange(data1, data2, status & 0xF));
    uint8_t message[3] = {0};
    message[0] = status;
    message[1] = data1;
    message[2] = data2;
    _midiUART.write(message, 3);
}