#include "MIDIFile.h"

MIDIFile::MIDIFile() :
_fs("fs")
{
    _sd = BlockDevice::get_default_instance();
}

int MIDIFile::init() {
    printf("Mounting the filesystem... ");
    fflush(stdout);
    int err = _fs.mount(_sd);
    printf("%s\n", (err ? "Fail :(" : "OK"));
    return 0;
}