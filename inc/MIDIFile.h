#include "mbed.h"
#include "USBMSD.h"
#include "HeapBlockDevice.h"
#include "FATFileSystem.h"
#include "USBPhyHw.h"
#include "usb_phy_api.h"

#define DEFAULT_BLOCK_SIZE  512
#define HEAP_BLOCK_DEVICE_SIZE (128 * DEFAULT_BLOCK_SIZE)


class MIDIFile : public USBMSD{
    FATFileSystem _heap_fs;
    HeapBlockDevice _bd;
    bool _usb;

virtual const uint8_t *string_iproduct_desc() override {
        static const uint8_t custom_desc[] = {
            0x1A, STRING_DESCRIPTOR,
            'L', 0, 'u', 0, 'c', 0, 'a', 0, 's', 0, ' ', 0, 'G', 0, 'r', 0, 'o', 0, 'o', 0, 'v', 0, 'e', 0
        };
        return custom_desc;
}

public:
    // Constructor
    MIDIFile();
    
    void init();

    // Connect USBMIDI Device
    void initUSB();

    // Deinit USBMIDI Device
    void deinitUSB();

    // Get USB State
    bool getUSB();

};