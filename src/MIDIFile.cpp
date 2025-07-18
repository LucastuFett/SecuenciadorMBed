#include "MIDIFile.h"

// VID_0703&PID_0104
MIDIFile::MIDIFile() :
USBMSD(get_usb_phy(),&_bd,0x0703,0x0104,1),
_heap_fs("heap_fs"),
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