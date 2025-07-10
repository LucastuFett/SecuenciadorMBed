#include "Buttons.h"

// Global variables for MIDI messages and control
extern uint8_t midiMessages[320][3]; // MIDI messages for each beat
extern uint8_t offMessages[320][3]; // Off messages for each beat
extern uint32_t control; // Control mask for beats
extern uint8_t channels[16]; // Active channels
extern uint8_t beat; // Current beat index
extern uint8_t note;
extern uint8_t octave;
extern DigitalIn mode32;
extern bool half;
extern uint8_t channel;
extern uint8_t velocity;
extern uint32_t beatsPerTone[1536]; // Beats per tone structure
extern map<holdKey, uint8_t> holded; // Holds the start and end of held notes
extern uint8_t hold; // Hold state: 0 = No Hold, 1 = Waiting 1st, 2 = Waiting 2nd

Buttons::Buttons(){

}

void Buttons::press(uint8_t num){
	uint8_t noteIndex = note + (octave * 12);
	uint8_t midiNote = noteIndex + 24;
	uint32_t beatMask = 0;
	if (mode32 && half) num += 16;
	beatMask = 0x80000000 >> num;	
	uint8_t origNum = num;
	uint8_t i = 0;
	uint16_t bptIndex = (noteIndex * 16) + channel;
    holdKey pressKey = {num,channel,midiNote};

	// If active
	if (beatsPerTone[bptIndex] & beatMask != 0){
		channels[channel] --;
		while(i < 10){
			uint16_t index = (i*32) + num;
			if (midiMessages[index][0] & 0xF == channel && midiMessages[index][1] == midiNote){
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
					if (offMessages[j*32 + num][0] & 0xF == channel && offMessages[j*32 + num][1] == midiNote){
						offMessages[j*32 + num][0] = 0;
						break;
                    }
                }
				beatsPerTone[bptIndex] &= ~beatMask;
				updateColors();
				break;
            } else i ++;
        }
		i = 0;
		while (i < 10){
			if ((midiMessages[(i*32) + origNum][0] == 0) || (offMessages[(i*32) + num][0] == 0)) return;
			else i ++;
        }
        control &= ~beatMask;
	}
    // If not Active
	else{
		bool checkIfNotHolded = false;
		bool checkIfNotAfterHold = false;
		for (uint8_t j = 0; j < 32; j++){
            holdKey checkKey = {j,channel,midiNote};
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
					if (control & beatMask == 0) control |= beatMask;
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
                            holdKey insertedKey = {_holdTemporary,channel,midiNote};
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
					if (control & beatMask == 0) control |= beatMask;
					break;
                } else i += 1;
            }
        }
    }
    updateColors();
}

void Buttons::updateColors(){

}

void Buttons::updateStructures(){

}

void Buttons::updateHolded(){

}
