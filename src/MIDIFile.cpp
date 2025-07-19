#include "MIDIFile.h"

extern uint8_t midiMessages[320][3];
extern uint8_t offMessages[320][3]; 
extern int16_t tempo[2];
extern string filename;
extern string renameFilename;
extern uint8_t bank;

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
    //DIR *d = opendir("/fs/");
    mkdir("/fs/1",0x1FF);
    mkdir("/fs/2",0x1FF);
    mkdir("/fs/3",0x1FF);
    mkdir("/fs/4",0x1FF);
    mkdir("/fs/5",0x1FF);
    mkdir("/fs/6",0x1FF);
    mkdir("/fs/7",0x1FF);
    mkdir("/fs/8",0x1FF);
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