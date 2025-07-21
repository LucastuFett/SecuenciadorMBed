#include "MIDIFile.h"
#include "Screen.h"

extern uint8_t midiMessages[320][3];
extern uint8_t offMessages[320][3]; 
extern int16_t tempo[2];
extern string filename;
extern string renameFilename;
extern uint8_t bank;
extern Screen screen;

// VID_0703&PID_0104
MIDIFile::MIDIFile() :
USBMSD(get_usb_phy(),&_bd,0x0703,0x0104,1),
_heap_fs("fs"),
_bd(HEAP_BLOCK_DEVICE_SIZE, DEFAULT_BLOCK_SIZE)
{
    _bd.init();

    FATFileSystem::format(&_bd);

    int err = _heap_fs.mount(&_bd);

    if (err) {
        err = _heap_fs.reformat(&_bd);
    }

    // If still error, then report failure
    if (err) {
        while (1);
    }    
    _usb = false;
}

void MIDIFile::init() {
	mkdir("/fs/1",0x1FF);
    mkdir("/fs/2",0x1FF);
    mkdir("/fs/3",0x1FF);
    mkdir("/fs/4",0x1FF);
    mkdir("/fs/5",0x1FF);
    mkdir("/fs/6",0x1FF);
    mkdir("/fs/7",0x1FF);
    mkdir("/fs/8",0x1FF);

	loadTestFiles();
}

void MIDIFile::loadTestFiles(){
	FILE *file = fopen("/fs/1/Jump.mid","w");
	for (uint8_t c : Jump) fputc(c,file);
	fflush(file);
	fclose(file);
	FILE *file = fopen("/fs/1/Out of Touch.mid","w");
	for (uint8_t c : OutOfTouch) fputc(c,file);
	fflush(file);
	fclose(file);
	FILE *file = fopen("/fs/1/Smalltown Boy.mid","w");
	for (uint8_t c : Smalltown) fputc(c,file);
	fflush(file);
	fclose(file);
	FILE *file = fopen("/fs/1/Take On Me.mid","w");
	for (uint8_t c : Take) fputc(c,file);
	fflush(file);
	fclose(file);
}

void MIDIFile::initUSB(){
    USBMSD::connect();
    _usb = true;
}

void MIDIFile::deinitUSB(){
    USBMSD::deinit();
    _usb = false;
}

bool MIDIFile::getUSB(){
    return _usb;
}

void MIDIFile::saveToFile(){
	FILE *file = fopen(("/fs/" + to_string(bank) + "/" + trim(filename) + ".mid").c_str(),"w");
	const uint8_t delta = 24;
	const uint8_t initMthd[] = {0x4D,0x54,0x68,0x64,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x01,0x00,delta};
    const uint8_t initMtrk[] = {0x4D,0x54,0x72,0x6B};
	const uint8_t tone = 0; //C
	const uint8_t mode = 0; //Major
    // TODO: If tempo is ext
	uint32_t tempoInt = uint32_t((float(1)/(float(tempo[1])/60))*1000000);
    for (uint8_t b : initMthd) fputc(b,file);
    for (uint8_t b : initMtrk) fputc(b,file);
    uint8_t mtrkHeader[] = {0x00,0xFF,0x58,0x04,0x04,0x02,0x18,0x08,0x00,0xFF,0x51,0x03,uint8_t((tempoInt >> 16) & 0xFF),uint8_t((tempoInt >> 8) & 0xFF),uint8_t(tempoInt & 0xFF),0x00,0xFF,0x59,0x02, tone, mode};
    for (uint8_t b : mtrkHeader) _mtrk.push_back(b);
	uint32_t curOff = 0;
	for (uint8_t j = 0; j < 32; j++){
		bool flag = false;
		for (uint8_t i = 0; i < 10; i++){
			uint16_t index = (i * 32) + j;
			//print(messages[index][0])
			if (offMessages[index][0] != 0){
				flag = true;
				if (curOff > 0x7F) calcDelta(curOff);
				else _mtrk.push_back(curOff);
				_mtrk.push_back(offMessages[index][0]);
				_mtrk.push_back(offMessages[index][1]);
				_mtrk.push_back(offMessages[index][2]);
				curOff = 0;
			}if (midiMessages[index][0] != 0){
				flag = true;
				if (curOff > 0x7F) calcDelta(curOff);
				else _mtrk.push_back(curOff & 0xFF);
				_mtrk.push_back(midiMessages[index][0]);
				_mtrk.push_back(midiMessages[index][1]);
				_mtrk.push_back(midiMessages[index][2]);
				curOff = 0;
            }
		}	
		if (!flag) curOff += delta;
		else curOff = delta;
    }
    const uint8_t end[] = {0x00,0xFF,0x2F,0x00};
	for (uint8_t b : end) _mtrk.push_back(b);
    uint32_t chunkLen = _mtrk.size();
    uint8_t chunkBigEndian[] = {uint8_t((chunkLen >> 24) & 0xFF),uint8_t( (chunkLen >> 16) & 0xFF),uint8_t( (chunkLen >> 8) & 0xFF),uint8_t( chunkLen & 0xFF)};
	for (uint8_t b : chunkBigEndian) fputc(b,file);
	for (uint8_t b : _mtrk) fputc(b,file);
	fflush(file);
	fclose(file);
}

void MIDIFile::calcDelta(uint32_t value){
	uint32_t buffer = value & 0x7F;
	while ((value >> 7) > 0){
		value >>= 7;
		buffer <<= 8;
		buffer |= 0x80;
		buffer += (value & 0x7F);
	}
    while (true){
		_mtrk.push_back(buffer & 0xFF);
		if (buffer & 0x80) buffer >>= 8;
		else break;    
    }
}

void MIDIFile::readFromFile(uint8_t midiMessages[320][3], uint8_t offMessages[320][3], int16_t bpm[2], string filename, uint8_t bank){
	FILE *file = fopen(("/fs/" + to_string(bank) + "/" + trim(filename) + ".mid").c_str(),"r");
	uint16_t delta = 0;
	uint32_t tempo = 0;
	uint8_t tone = 0;
	uint8_t mode = 0;
	uint8_t type = 0;
	uint8_t messageCount[32] = {0};
	uint8_t offMessageCount[32] = {0};
	uint32_t buffer = get32(file);
	uint32_t chunklen = 0;
	
	// Read MThd
	if (buffer == 0x4D546864){
		chunklen = get32(file);
		if (chunklen != 6){
			screen.showError("Error on MIDI File MThd");
			fclose(file);
			return;
		}
		buffer = get16(file);
		if (buffer > 1){
			screen.showError("MIDI File not Type 0 or 1");
			fclose(file);
			return;
		}
		type = buffer;
		buffer = get16(file);
		if (buffer > 15){
			screen.showError("MIDI Tracks should be less than 16");
			fclose(file);
			return;
		}
		buffer = get16(file);
		if (buffer >= 0x7000){
			screen.showError("Negative Delta not supported");
			fclose(file);
			return;
		}
		delta = buffer;
	}else{
		screen.showError("Error on MIDI File MThd");
		fclose(file);
		return;
	}

	for (uint16_t i = 0; i < 320; i++) for(uint8_t j = 0; j < 3; j++) midiMessages[i][j] = 0; 
	for (uint16_t i = 0; i < 320; i++) for(uint8_t j = 0; j < 3; j++) offMessages[i][j] = 0; 

	// Read MTrk
	while (!feof(file)){
		buffer = get32(file);
		if (buffer == 0x4D54726B){
			chunklen = get32(file);
			uint32_t curOff = 0;
			uint8_t curBeat = 0;
			uint8_t curMsg = 0;
			uint8_t curOffMsg = 0;
			uint16_t index = 0;
			uint16_t indexOff = 0;
			while (chunklen > 0){
				uint32_t result[2] = {0};
				readDelta(file,result);
				curOff = result[0];
				chunklen -= result[1];
				buffer = fgetc(file);
				chunklen --;
				// If System Message
				if (buffer == 0xFF){
					buffer = fgetc(file);
					switch (buffer){
						case 0x01:
						case 0x02:
						case 0x03:
						case 0x04:
						case 0x05:
						case 0x06:
						case 0x07:{
							uint8_t lenText = fgetc(file);
							for (uint8_t i = 0; i < lenText; i++){
								buffer = fgetc(file);
								chunklen --;
							}
							chunklen --;
							break;
						}
						case 0x2F:{
							buffer = fgetc(file);
							chunklen -= 2;
							break;
						}
						case 0x51:{
							buffer = get32(file);
							tempo = buffer & 0xFFFFFF;
							bpm[1] = uint16_t(60000000 / tempo);
							chunklen -= 5;
							break;
						}
						case 0x58:{
							buffer = fgetc(file);
							buffer = get32(file);
							if ((buffer & 0xFFFF0000) != 0x04020000){
								screen.showError("Only 4/4 is supported");
								fclose(file);
								return;
							}
							chunklen -= 6;
							break;
						}
						case 0x59:{
							buffer = fgetc(file);
							buffer = fgetc(file);
							tone = buffer; // TODO: From 5ths Circle, calculate dif
							buffer = fgetc(file);
							mode = buffer;
							chunklen -= 4;
							break;
						}
					}
				// If Control Change
				}else if (buffer >= 0xA0){
					if ((buffer & 0xF0) == 0xB0){
						buffer = get16(file);
						chunklen -= 2;
					}else if ((buffer & 0xF0) == 0xC0){
						buffer = fgetc(file);
						chunklen --;
					}
				// If Track Message
				}else{
					uint8_t msg[3] = {0};
					if (curOff != 0){
						curBeat += curOff/delta;
						curMsg = 0;
						curOffMsg = 0;
					}
					if (curBeat == 32) curBeat = 0;
					// If Type 0, use a variable to see current message
					if (type == 0){
						index = (curMsg * 32) + curBeat;
						indexOff = (curOffMsg * 32) + curBeat;
					}
					// If Type 0, use array to see how many messages per beat
					else{
						if (messageCount[curBeat] == 10 || offMessageCount[curBeat] == 10){
							screen.showError("There are more than 10 notes in a beat");
							fclose(file);
							return;
						}
						index = (messageCount[curBeat] * 32) + curBeat;
						indexOff = (offMessageCount[curBeat] * 32) + curBeat;
					}	
					// If Note Off, transform to Note On Velocity 0
					if ((buffer & 0xF0) == 0x80){
						msg[0] = 0x90 | (buffer & 0xF);
						buffer = fgetc(file);
						msg[1] = buffer;
						buffer = fgetc(file);
						msg[2] = 0;
					}else{
						msg[0] = buffer;
						buffer = fgetc(file);
						msg[1] = buffer;
						buffer = fgetc(file);
						msg[2] = buffer;
					}	
					if (msg[2] != 0){
						memcpy(midiMessages[index],msg,3);
						if (type == 0) curMsg ++;
						else messageCount[curBeat] ++;
					}else{
						//TODO: Only works with 32 beats 
						if (type == 0) curOffMsg ++;
						else offMessageCount[curBeat] ++;
						memcpy(offMessages[indexOff],msg,3);
					}
					chunklen -= 2;
				}
			}
		}else{
			screen.showError("Error on MTrk");
			fclose(file);
			return;
		}
	}
	fclose(file);
}

void MIDIFile::readDelta(FILE *f, uint32_t response[]){
	uint32_t bytecount = 1;
	uint32_t value = fgetc(f);
	if (value & 0x80){
		value &= 0x7F;
		uint8_t c = fgetc(f);
		bytecount ++;
		value = (value << 7) + (c & 0x7F);
		while (c & 0x80){
			c = fgetc(f);
			bytecount += 1;
			value = (value << 7) + (c & 0x7F);
		}
	}
	response[0] = value;
	response[1] = bytecount;
}

void MIDIFile::getFiles(uint8_t bank, string files[12]){
	DIR *d = opendir(("/fs/" + to_string(bank) + "/").c_str());
	uint8_t i = 0;
	while(true){
		struct dirent *e = readdir(d);
		if(!e || i >= 12) break;
		string name = e->d_name;
		size_t pos = name.find(".mid");
    	files[i] = (pos != std::string::npos) ? name.substr(0, pos) : name;
		i++;
	}
	while (i < 12){
		files[i++] = "";
	}
	closedir(d);
}

void MIDIFile::deleteFile(string filename, uint8_t bank){
	if (filename != "") remove(("/fs/" + to_string(bank) + "/" + filename + ".mid").c_str());
}

void MIDIFile::renameFile(string origFilename, string filename, uint8_t bank){
	if (filename != "" && origFilename != "") rename(("/fs/" + to_string(bank) + "/" + origFilename + ".mid").c_str(),("/fs/" + to_string(bank) + "/" + filename + ".mid").c_str());
}