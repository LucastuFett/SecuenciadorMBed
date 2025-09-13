#include "Buttons.h"
#include "MIDITimer.h"

// Global variables for MIDI messages and control
extern state mainState;
extern uint8_t midiMessages[320][3]; // MIDI messages for each beat
extern uint8_t offMessages[320][3]; // Off messages for each beat
extern uint32_t control; // Control mask for beats
extern uint8_t channels[16]; // Active channels
extern int8_t beat; // Current beat index
extern int8_t note;
extern int8_t octave;
extern DigitalIn toggle32;
extern bool mode32;
extern bool half;
extern int8_t channel;
extern int8_t velocity;
extern uint32_t beatsPerTone[1536]; // Beats per tone structure
extern map<holdKey,uint8_t,CompareHoldKey> holded; // Holds the start and end of held notes
extern uint8_t hold; // Hold state: 0 = No Hold, 1 = Waiting 1st, 2 = Waiting 2nd
extern uint32_t ledData[16];
extern uint32_t lastLedData[16];
extern bool channelEnabled[16];
extern bool keys[5][4];
extern uint8_t launchMessages[16][3];
extern Mutex messagesMutex;
extern Mutex beatsPerToneMutex;
extern Mutex beatMutex;
extern Mutex ledDataMutex;
extern Mutex controlMutex;
extern Mutex holdedMutex;
extern Mutex channelEnabledMutex;

extern MIDITimer timer;

Buttons::Buttons(){
	_holdTemporary = 0;
	for (uint8_t i = 0; i < 16; i++) {
		ledData[i] = 0x00000000;
		lastLedData[i] = 0x00000000;
		_DAWSent[i] = false;
	}

}

void Buttons::press(uint8_t num){
	switch(mainState){
		case PROG:{
			uint8_t noteIndex = note + (octave * 12);
			uint8_t midiNote = noteIndex + 24;
			uint32_t beatMask = 0;
			if (mode32 && half) num += 16;
			beatMask = 0x80000000 >> num;	
			uint8_t origNum = num;
			uint8_t i = 0;
			uint16_t bptIndex = (noteIndex * 16) + channel;
			holdKey pressKey = {num,uint8_t(channel),midiNote};
			messagesMutex.lock();
			controlMutex.lock();
			beatsPerToneMutex.lock();
			holdedMutex.lock();
			// If active
			if ((beatsPerTone[bptIndex] & beatMask) != 0){
				channels[channel] --;
				while(i < 10){
					uint16_t index = (i*32) + num;
					if (((midiMessages[index][0] & 0xF) == channel) && (midiMessages[index][1] == midiNote)){
						midiMessages[index][0] = 0;
						if (auto search = holded.find(pressKey); search != holded.end()){
							num = holded.at(pressKey);
							holded.erase(pressKey);
						}
						if (mode32){
							if (num == 31) num = 0;
							else num ++;
						} else {
							if (num == 15) num = 0;
							else num ++;
						}
						for (uint8_t j = 0; j < 10; j++){
							if (((offMessages[j*32 + num][0] & 0xF) == channel) && (offMessages[j*32 + num][1] == midiNote)){
								offMessages[j*32 + num][0] = 0;
								break;
							}
						}
						beatsPerTone[bptIndex] &= ~beatMask;
						//updateColors();
						break;
					} else i ++;
				}
				i = 0;
				while (i < 10){
					if ((midiMessages[(i*32) + origNum][0] == 0) || (offMessages[(i*32) + num][0] == 0)){
						holdedMutex.unlock();
						beatsPerToneMutex.unlock();
						controlMutex.unlock();
						messagesMutex.unlock();
						updateColors();
						return;
					} 
					else i ++;
				}
				control &= ~beatMask;
			}
			// If not Active
			else{
				bool checkIfNotHolded = false;
				bool checkIfNotAfterHold = false;
				for (uint8_t j = 0; j < 32; j++){
					holdKey checkKey = {j,uint8_t(channel),midiNote};
					if (auto search = holded.find(checkKey); search != holded.end()){
						checkIfNotHolded = (num >= j && num <= holded.at(checkKey));
						checkIfNotAfterHold = (_holdTemporary < j && num > holded.at(checkKey));
					}
				}
				if (!checkIfNotHolded){
					channels[channel] ++;
					while (i < 10){
						uint16_t index = (i*32) + num;
						if (midiMessages[index][0] == 0){
							if ((control & beatMask) == 0) control |= beatMask;
							if (hold != 2){
								midiMessages[index][0] = 0x90 | channel;
								midiMessages[index][1] = midiNote;
								midiMessages[index][2] = velocity;
								beatsPerTone[bptIndex] |= beatMask;
							}
							if (hold == 1) _holdTemporary = num;
							if (mode32){
								if (num == 31) num = 0;
								else num ++;
							} else {
								if (num == 15) num = 0;
								else num ++;
							}
							if (hold == 0){
								for (uint8_t j = 0; j < 10; j++){
									if (offMessages[j*32 + num][0] == 0){
										offMessages[(j*32) + num][0] = 0x90 | channel;
										offMessages[(j*32) + num][1] = midiNote;
										offMessages[(j*32) + num][2] = 0;
										break;
									}
								}
							}else if (hold == 1) hold = 2;
							else if (hold == 2){
								if (!checkIfNotAfterHold){
									holdKey insertedKey = {_holdTemporary,uint8_t(channel),midiNote};
									if (num > _holdTemporary){
										for (uint8_t j = 0; j < 10; j++){
											if (offMessages[j*32 + num][0] == 0){
												offMessages[(j*32) + num][0] = 0x90 | channel;
												offMessages[(j*32) + num][1] = midiNote;
												offMessages[(j*32) + num][2] = 0;
												break;
											}
										}
										holded.emplace(insertedKey,num-1);
									}else if (num == 0){
										for (uint8_t j = 0; j < 10; j++){
											if (offMessages[j*32 + num][0] == 0){
												offMessages[(j*32) + num][0] = 0x90 | channel;
												offMessages[(j*32) + num][1] = midiNote;
												offMessages[(j*32) + num][2] = 0;
												break;
											}
										}
										if (mode32) holded.emplace(insertedKey,31);
										else holded.emplace(insertedKey,15);
									}
								}
								hold = 0;
							}
							beatMask = 0x80000000 >> num;
							if ((control & beatMask) == 0) control |= beatMask;
							break;
						} else i += 1;
					}
				}
			}
			holdedMutex.unlock();
			beatsPerToneMutex.unlock();
			controlMutex.unlock();
			messagesMutex.unlock();
			break;
		}
		case CHANNEL:
		case PLAY: {
			// Turn on/off channel
			channelEnabledMutex.lock();
			channelEnabled[num] = !channelEnabled[num];
			channelEnabledMutex.unlock();
			break;
		}
		case LAUNCH:
			if (launchMessages[num][0] != 0) timer.send(launchMessages[num][0], launchMessages[num][1], launchMessages[num][2]);
			break;
		case DAW:
			if (!_DAWSent[num]) timer.send(launchMessages[num][0], launchMessages[num][1], launchMessages[num][2]);
			else timer.send(launchMessages[num][0], launchMessages[num][1], 0);
			_DAWSent[num] = !_DAWSent[num];
			break;
	}
	updateColors();
}

void Buttons::release(uint8_t num){
	if (launchMessages[num][0] != 0) timer.send(launchMessages[num][0], launchMessages[num][1], 0);
	updateColors();
}

void Buttons::updateColors(){
	ledDataMutex.lock();
	for (uint8_t i = 0; i < 16; i++) ledData[i] = 0x00000000;
	switch (mainState){
		case PROG:
			uint16_t bptIndex = (note + (octave * 12)) * 16 + channel;
			uint32_t beatMask = 0;
			for (uint8_t i = 0; i < 16; i++) {
				if(ledData[i] != 0) continue;
				uint8_t num = i;
				if (mode32 && half)	num += 16;
				beatMask = 0x80000000 >> num;
				beatsPerToneMutex.lock();
				if ((beatsPerTone[bptIndex] & beatMask) != 0) ledData[i] = BlueBtn;
				beatsPerToneMutex.unlock();
				holdKey key = {num, uint8_t(channel), uint8_t(note + octave * 12 + 24)};
				holdedMutex.lock();
				if (auto search = holded.find(key); search != holded.end()){
					// If not in mode32, if num >= 16 or endBeat >= 16, continue
					// If in mode32, if num >= 16 and half, if endBeat >= 16, continue
					// If in mode32, if num < 16 and half, if endBeat >= 16, paint end
					uint8_t endBeat = holded.at(key);
					bool n = num >= 16;
					bool e = endBeat >= 16;
					
					if (((!mode32) && (n || e)) || (n && !half) || ((!n) && (!e) && mode32 && half)) {
						holdedMutex.unlock();
						continue;
					}	
					if (e && half) endBeat -= 16;
					else if (n && !e) endBeat = 15;
					else if (e && !half) endBeat = 16;
					
					if ((!half) || (!mode32) || (n & e))  ledData[i] = RedBtn;
						
					if ((!n) && e && half){
						for (uint8_t j = 0; j < endBeat; j++) ledData[i] = OrangeBtn;
					}else{
						for (uint8_t j = i + 1; j < endBeat; j++) ledData[i] = OrangeBtn;
					}
					if (half || (!e)) ledData[endBeat] = RedBtn;
				}
				holdKey iKey = {i, uint8_t(channel), uint8_t(note + octave * 12 + 24)};
				if (auto search = holded.find(iKey); search != holded.end() && mode32 && half){
					if (holded.at(iKey) > 15){
						for (uint8_t j = 0; j < holded.at(iKey) - 16; j++) {
							ledData[j] = OrangeBtn;
						}
						ledData[holded.at(iKey) - 16] = RedBtn;
					}
				}
				holdedMutex.unlock();
				//if (note+octave*12+24 != lastNote and lastNote != 0): //Agregar cuando se pasa de 16 a 32
				//	listbtn[i].add_theme_stylebox_override("normal",greyStyle)
				beatMutex.lock();
				if (beat == num) ledData[i] = PurpleBtn; // Highlight current beat	
				beatMutex.unlock();
			}
			break;
		case PLAY:
		case CHANNEL:
			channelEnabledMutex.lock();
			for (uint8_t i = 0; i < 16; i++){
				if (channelEnabled[i] && channels[i] > 0) ledData[i] = BlueBtn;
			}
			channelEnabledMutex.unlock();
			break;
		case LAUNCH:
			for (uint8_t i = 0; i < 16; i++) if(keys[(i/4) + 1][i%4]) ledData[i] = launchColorsGRB[i];
			break;
		case DAW:
			for (uint8_t i = 0; i < 16; i++) if(_DAWSent[i]) ledData[i] = launchColorsGRB[i];
			break;
	}
	
	ledDataMutex.unlock();
}

void Buttons::updateStructures(){
	for (uint16_t i = 0; i < 1536; i++) beatsPerTone[i] = 0;
	for (uint8_t i = 0; i < 16; i++) channels[i] = 0;
	uint16_t bptIndex = 0;
	uint32_t beatMask = 0x80000000;
	for (uint8_t j = 0; j < 32; j++){
		for (uint8_t i = 0; i < 10; i++){
			uint16_t index = (i * 32) + j;
			if (midiMessages[index][0] != 0 && midiMessages[index][2] != 0){
				bptIndex = ((midiMessages[index][1] - 24)*16) + (midiMessages[index][0] & 0xF);
				beatsPerTone[bptIndex] |= (beatMask >> j);
				if ((control & (beatMask >> j)) == 0) control |= (beatMask >> j);
				channels[midiMessages[index][0] & 0xF] ++;
			}
		}
	}
	updateHolded();
}

void Buttons::updateHolded(){
	// If the next message in midiMessages does not have the same number but velocity 0, it is considered a hold
	// Searchs index of the offMessage, saving channel, midiNote and both beats
	holded.clear();
	bool found = false;
	bool found32 = false;
	for (uint8_t j = 16; j < 32; j++){
		for (uint8_t i = 0; i < 10; i++){
			if (midiMessages[i*32 + j][0] != 0){
				found32 = true;
				break;
			}
		}
		if (found32) break;
	}
	for (uint8_t i = 0; i < 10; i++){
		for (uint8_t j = 0; j < 32; j++){
			uint16_t index = (i * 32) + j;
			bool flag = false;					// Flag to check if it's not a hold
			if (midiMessages[index][0] != 0){
				for (uint8_t l = 0; l < 10; l++){
					if ((((j == 31 && found32) || (j == 15 && !found32)) 	// If it's the last beat
					&& (offMessages[l*32][1] == midiMessages[index][1]))    // And the off message is in the first position
					|| (offMessages[(l*32) + j + 1][1] == midiMessages[index][1])){		// Or if the off message is in the next position
						flag = true;	// If the off message is found, it's not a hold
						break;
					}
				}
				if (!flag){
					for (uint8_t k = j; k < 32; k++){
						for (uint8_t l = 0; l < 10; l++){
							if (offMessages[(l*32) + k][0] == midiMessages[index][0] && offMessages[(l*32) + k][1] == midiMessages[index][1]){
								holdKey jKey = {j, uint8_t(midiMessages[index][0] & 0xF), midiMessages[index][1]};
								holded.emplace(jKey,k-1);
								found = true;
								break;
							}
						}
						if (found == true) break;
					}
					if (!found){
						for (uint8_t l = 0; l < 10; l++){
							if ((offMessages[(l*32)][0] == midiMessages[index][0] && offMessages[(l*32)][1] == midiMessages[index][1]) && found32){
								holdKey jKey = {j, uint8_t(midiMessages[index][0] & 0xF), midiMessages[index][1]};
								holded.emplace(jKey,31);
								break;
							}
							else if ((offMessages[(l*32)][0] == midiMessages[index][0] && offMessages[(l*32)][1] == midiMessages[index][1]) && !found32){
								holdKey jKey = {j, uint8_t(midiMessages[index][0] & 0xF), midiMessages[index][1]};
								holded.emplace(jKey,15);
								break;
							}
						}
					}
					found = false;
				}
			}
		}
	}
	mode32 = found32;
}
