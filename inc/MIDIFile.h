#include "mbed.h"
#include "BlockDevice.h"
#include "FATFileSystem.h"

class MIDIFile {
    BlockDevice *_sd;
    FATFileSystem _fs;
public:
    // Constructor
    MIDIFile();
    
    int init();
};