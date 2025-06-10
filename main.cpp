#include "mbed.h"
#include "USBMIDI.h"
#include "WS2812.h"

USBMIDI midi;
uint8_t beat = 0;
uint8_t midiMessages[320][3] = {{144, 36, 127}, {144, 49, 127}, {144, 38, 127}, {144, 36, 127}, {144, 49, 127}, {144, 49, 127}, {144, 38, 127}, {144, 49, 127}, {144, 36, 127}, {144, 49, 127}, {144, 38, 127}, {144, 36, 127}, {144, 49, 127}, {144, 36, 127}, {144, 38, 127}, {144, 49, 127}, {144, 36, 127}, {144, 49, 127}, {144, 38, 127}, {144, 36, 127}, {144, 49, 127}, {144, 49, 127}, {144, 38, 127}, {144, 49, 127}, {144, 36, 127}, {144, 49, 127}, {144, 38, 127}, {144, 36, 127}, {144, 49, 127}, {144, 36, 127}, {144, 38, 127}, {144, 49, 127}, {144, 49, 127}, {145, 53, 100}, {144, 49, 127}, {144, 49, 127}, {145, 53, 100}, {145, 53, 100}, {144, 49, 127}, {145, 53, 100}, {144, 49, 127}, {145, 50, 100}, {144, 49, 127}, {144, 49, 127}, {145, 55, 100}, {144, 49, 127}, {144, 49, 127}, {145, 55, 100}, {144, 49, 127}, {145, 57, 100}, {144, 49, 127}, {144, 49, 127}, {145, 55, 103}, {145, 55, 103}, {144, 49, 127}, {145, 55, 103}, {144, 49, 127}, {145, 57, 100}, {144, 49, 127}, {144, 49, 127}, {145, 57, 103}, {144, 49, 127}, {144, 49, 127}, {145, 52, 103}, {145, 53, 100}, {0, 0, 0}, {145, 53, 100}, {145, 53, 100}, {0, 0, 0}, {148, 76, 110}, {145, 53, 100}, {0, 0, 0}, {145, 50, 100}, {0, 0, 0}, {145, 55, 100}, {145, 55, 100}, {0, 0, 0}, {145, 55, 100}, {145, 55, 100}, {0, 0, 0}, {145, 57, 100}, {0, 0, 0}, {145, 57, 100}, {145, 57, 100}, {146, 43, 125}, {148, 72, 110}, {145, 55, 103}, {146, 45, 125}, {145, 57, 100}, {0, 0, 0}, {145, 57, 100}, {145, 57, 100}, {148, 72, 110}, {145, 52, 103}, {145, 52, 103}, {148, 69, 110}, {146, 41, 127}, {0, 0, 0}, {148, 76, 110}, {148, 74, 110}, {0, 0, 0}, {0, 0, 0}, {148, 74, 110}, {0, 0, 0}, {146, 38, 127}, {0, 0, 0}, {146, 43, 127}, {148, 74, 110}, {0, 0, 0}, {148, 72, 110}, {148, 76, 110}, {0, 0, 0}, {146, 45, 127}, {0, 0, 0}, {148, 69, 110}, {148, 69, 110}, {147, 59, 127}, {0, 0, 0}, {148, 74, 110}, {0, 0, 0}, {147, 60, 127}, {0, 0, 0}, {148, 74, 110}, {148, 74, 110}, {0, 0, 0}, {146, 40, 125}, {0, 0, 0}, {0, 0, 0}, {147, 57, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 127}, {0, 0, 0}, {147, 59, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 60, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 55, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 55, 127}, {0, 0, 0}, {0, 0, 0}, {147, 53, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 53, 127}, {0, 0, 0}, {147, 55, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 50, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 127}, {0, 0, 0}, {0, 0, 0}, {147, 48, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 48, 127}, {0, 0, 0}, {147, 50, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 43, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 45, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 48, 127}, {0, 0, 0}, {0, 0, 0}, {147, 41, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 41, 127}, {0, 0, 0}, {147, 43, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 45, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 72, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 76, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 40, 127}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 72, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 71, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
uint8_t offMessages[320][3] = {{144, 49, 0}, {144, 36, 0}, {144, 49, 0}, {144, 38, 0}, {144, 36, 0}, {144, 49, 0}, {144, 49, 0}, {144, 38, 0}, {144, 49, 0}, {144, 36, 0}, {144, 49, 0}, {144, 38, 0}, {144, 36, 0}, {144, 49, 0}, {144, 36, 0}, {144, 38, 0}, {144, 49, 0}, {144, 36, 0}, {144, 49, 0}, {144, 38, 0}, {144, 36, 0}, {144, 49, 0}, {144, 49, 0}, {144, 38, 0}, {144, 49, 0}, {144, 36, 0}, {144, 49, 0}, {144, 38, 0}, {144, 36, 0}, {144, 49, 0}, {144, 36, 0}, {144, 38, 0}, {145, 52, 0}, {144, 49, 0}, {145, 53, 0}, {144, 49, 0}, {144, 49, 0}, {145, 53, 0}, {145, 53, 0}, {144, 49, 0}, {145, 53, 0}, {144, 49, 0}, {145, 50, 0}, {144, 49, 0}, {144, 49, 0}, {145, 55, 0}, {144, 49, 0}, {144, 49, 0}, {145, 55, 0}, {144, 49, 0}, {145, 57, 0}, {144, 49, 0}, {144, 49, 0}, {145, 55, 0}, {145, 55, 0}, {144, 49, 0}, {145, 55, 0}, {144, 49, 0}, {145, 57, 0}, {144, 49, 0}, {144, 49, 0}, {145, 57, 0}, {144, 49, 0}, {144, 49, 0}, {146, 40, 0}, {145, 53, 0}, {0, 0, 0}, {145, 53, 0}, {145, 53, 0}, {148, 74, 0}, {148, 76, 0}, {145, 53, 0}, {146, 41, 0}, {145, 50, 0}, {146, 38, 0}, {145, 55, 0}, {145, 55, 0}, {148, 74, 0}, {145, 55, 0}, {145, 55, 0}, {146, 43, 0}, {145, 57, 0}, {0, 0, 0}, {145, 57, 0}, {145, 57, 0}, {148, 72, 0}, {148, 72, 0}, {145, 55, 0}, {147, 59, 0}, {145, 57, 0}, {148, 76, 0}, {145, 57, 0}, {145, 57, 0}, {146, 45, 0}, {145, 52, 0}, {145, 52, 0}, {147, 55, 0}, {0, 0, 0}, {0, 0, 0}, {148, 76, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 0}, {147, 57, 0}, {0, 0, 0}, {148, 72, 0}, {0, 0, 0}, {0, 0, 0}, {148, 72, 0}, {0, 0, 0}, {147, 59, 0}, {0, 0, 0}, {0, 0, 0}, {148, 69, 0}, {146, 45, 0}, {0, 0, 0}, {0, 0, 0}, {146, 43, 0}, {147, 55, 0}, {0, 0, 0}, {0, 0, 0}, {148, 74, 0}, {148, 74, 0}, {147, 60, 0}, {148, 71, 0}, {0, 0, 0}, {147, 52, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 53, 0}, {147, 53, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 55, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 60, 0}, {0, 0, 0}, {0, 0, 0}, {148, 74, 0}, {147, 50, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 0}, {0, 0, 0}, {0, 0, 0}, {147, 48, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 48, 0}, {147, 48, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 50, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 57, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 43, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 0}, {0, 0, 0}, {0, 0, 0}, {147, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 41, 0}, {147, 41, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 43, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 52, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 45, 0}, {0, 0, 0}, {0, 0, 0}, {148, 69, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 74, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 76, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {147, 45, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 72, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {148, 69, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
uint16_t tempo[] = {0,230};
uint32_t control = 0xFFFFFFFF;
uint8_t channels[16] = {50, 32, 7, 28, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int channelEnabled[5] = {1, 1, 1, 1, 1}; // Channel enable states

class MIDITimer {
public:
    MIDITimer() : _running(false), _interval(0) {}

    void start(us_timestamp_t interval) {
        _interval = interval;
        _running = true;
        _timer.start();
    }

    void stop() {
        pause();
        beat = 0;
    }

    void pause() {
        _running = false;
        _timer.stop();
        _timer.reset();
        allNotesOff();
    }

    bool is_running() const {
        return _running;
    }

    void poll() {
        if (_running) {
            us_timestamp_t now = _timer.elapsed_time().count();
            if (now >= _interval) {
                timeout();
                _timer.reset();
            }
        }
    }

    void timeout(){
        beat ++;
        if (beat == 32) {
            beat = 0;
        }
        beatPlay();
    }

    void beatPlay(){
        uint16_t index;
        uint32_t beatMask = 0x80000000 >> beat;
        if ((control & beatMask) == 0){
            return;
        }
        for(int i = 0; i < 10; i++){
            index = i * 32 + beat;
            if (offMessages[index][0] != 0){
                midi.write(MIDIMessage::NoteOn(offMessages[index][1], offMessages[index][2], offMessages[index][0] & 0xF));
            }
        }
        for(int i = 0; i < 10; i++){
            index = i * 32 + beat;
            if (midiMessages[index][0] != 0){
                if (channelEnabled[midiMessages[index][0] & 0xF] == 0) {
                    continue; // Skip if channel is disabled
                }
                midi.write(MIDIMessage::NoteOn(midiMessages[index][1], midiMessages[index][2], midiMessages[index][0] & 0xF));
            }
        }
    }

    void allNotesOff(){
        for(int i = 0; i < 16; i++){
            if (channels[i] != 0){
                midi.write(MIDIMessage::AllNotesOff(i));
            }
        }
    }

private:
    bool _running;
    us_timestamp_t _interval;
    Timer _timer;
};

MIDITimer timer; 
DigitalIn playPauseButton(p29, PullDown);
DigitalIn stopButton(p28, PullDown);
DigitalIn enableTempoSwitch(p27, PullDown);
DigitalOut ledTempo(p15, 0); // LED to indicate tempo mode
AnalogIn tempoPot(p26, 3.3f);

DigitalIn channel1(p0, PullDown);
DigitalIn channel2(p1, PullDown);
DigitalIn channel3(p2, PullDown);
DigitalIn channel4(p3, PullDown);
DigitalIn channel5(p4, PullDown);

DigitalOut ledChannel1(p8, 0); // LED for channel 1
DigitalOut ledChannel2(p9, 0); // LED for channel 2
DigitalOut ledChannel3(p10, 0); // LED for channel 3
DigitalOut ledChannel4(p11, 0); // LED for channel 4
DigitalOut ledChannel5(p12, 0); // LED for channel 5

//WS2812_PIO ledStrip(p7, 8); // SPI pins for WS2812 strip

int lastKeys[8] = {0}; // Keys for play/pause, stop, and tempo switch
int keys[8] = {0}; // Current state of keys
int tempoEnabled = 0;
/*uint8_t ledData[8][3] = {
    {0, 0, 0}, // Channel 1
    {0, 0, 0}, // Channel 2
    {0, 0, 0}, // Channel 3
    {0, 0, 0}, // Channel 4
    {0, 0, 0}, // Channel 5
    {0, 0, 0}, // -
    {0, 0, 0}, // -
    {0, 0, 0}  // -
};*/

void updateTempo() {
    tempo[1] = static_cast<uint16_t>(tempoPot.read() * 255);
    timer.start(static_cast<us_timestamp_t>((60.0 * 1'000'000) / tempo[1]));
}

void readKeys(int currentKeys[]) {
    currentKeys[0] = playPauseButton.read();
    currentKeys[1] = stopButton.read();
    currentKeys[2] = enableTempoSwitch.read();
    currentKeys[3] = channel1.read();
    currentKeys[4] = channel2.read();
    currentKeys[5] = channel3.read();
    currentKeys[6] = channel4.read();
    currentKeys[7] = channel5.read();
}

int main()
{
    // Give the USB a moment to initialize and enumerate
    while (!midi.ready()) {
        ThisThread::sleep_for(100ms);
    }
    // Set and Attach Trigger
    timer.start(static_cast<us_timestamp_t>((60.0 * 1'000'000) / tempo[1]));
    ledTempo = tempoEnabled;
    // Update channel LEDs
    ledChannel1 = channelEnabled[0]; // Update LED for channel 1
    ledChannel2 = channelEnabled[1]; // Update LED for channel 2
    ledChannel3 = channelEnabled[2]; // Update LED for channel 3
    ledChannel4 = channelEnabled[3]; // Update LED for channel 4
    ledChannel5 = channelEnabled[4]; // Update LED for channel 5
    while (true) {
        timer.poll();
        readKeys(keys);
        if (memcmp(keys, lastKeys, sizeof(keys)) != 0) {
            if (keys[0] && !lastKeys[0]) { // Play/Pause button pressed
                if (timer.is_running()) {
                    timer.pause();
                } else {
                    timer.start(static_cast<us_timestamp_t>((60.0 * 1'000'000) / tempo[1]));
                }
            }
            if (keys[1] && !lastKeys[1]) { // Stop button pressed
                timer.stop();
            }
            if (keys[2] && !lastKeys[2]) { // Tempo switch pressed
                tempoEnabled = tempoEnabled ? 0 : 1; // Toggle tempo mode
                ledTempo = tempoEnabled;
            }
            if (keys[3] && !lastKeys[3]) { // Channel 1 toggle
                channelEnabled[0] = channelEnabled[0] ? 0 : 1; // Toggle channel 1
            }
            if (keys[4] && !lastKeys[4]) { // Channel 2 toggle
                channelEnabled[1] = channelEnabled[1] ? 0 : 1;
            }
            if (keys[5] && !lastKeys[5]) { // Channel 3 toggle
                channelEnabled[2] = channelEnabled[2] ? 0 : 1;
            }
            if (keys[6] && !lastKeys[6]) { // Channel 4 toggle
                channelEnabled[3] = channelEnabled[3] ? 0 : 1;
            }
            if (keys[7] && !lastKeys[7]) { // Channel 5 toggle
                channelEnabled[4] = channelEnabled[4] ? 0 : 1;
            }/*
            for (int i = 0; i < 5; i++) {
                if (channelEnabled[i]) {
                    ledData[i][0] = 0; // Red
                    ledData[i][1] = 255; // Green
                    ledData[i][2] = 0; // Blue
                } else {
                    ledData[i][0] = 255; // Red
                    ledData[i][1] = 0; // Green
                    ledData[i][2] = 0; // Blue
                }
            }*/
            // Update channel LEDs
            ledChannel1 = channelEnabled[0]; // Update LED for channel 1
            ledChannel2 = channelEnabled[1]; // Update LED for channel 2
            ledChannel3 = channelEnabled[2]; // Update LED for channel 3
            ledChannel4 = channelEnabled[3]; // Update LED for channel 4
            ledChannel5 = channelEnabled[4]; // Update LED for channel 5

            memcpy(keys, lastKeys, sizeof(keys)); // Update lastKeys
        }
        if (timer.is_running()) {
            if (tempoEnabled) {
                updateTempo();
            }else{
                timer.start(static_cast<us_timestamp_t>((60.0 * 1'000'000) / 230)); // Default tempo
            }
        }
        /*for (int i = 0; i < 8; i++) {
            ledStrip.setPixel(i, ledData[i][0], ledData[i][1], ledData[i][2]);
        }
        ledStrip.show();*/

    }
}