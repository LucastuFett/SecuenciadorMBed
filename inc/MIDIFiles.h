#include "FATFileSystem.h"
#include "HeapBlockDevice.h"
#include "PicoSDBlockDevice.h"
#include "USBMSD.h"
#include "USBPhyHw.h"
#include "mbed.h"
#include "testFiles.h"
#include "usb_phy_api.h"

#define DEFAULT_BLOCK_SIZE 512
#define HEAP_BLOCK_DEVICE_SIZE (128 * DEFAULT_BLOCK_SIZE)

class BlockDeviceHolder {
   protected:
    PicoSDBlockDevice _picoBD;
    HeapBlockDevice _heapBD;
    BlockDevice *_blockDevice;
    bool _heap = false;

    BlockDeviceHolder() : _picoBD(MBED_CONF_SD_SPI_MOSI, MBED_CONF_SD_SPI_MISO, MBED_CONF_SD_SPI_CLK, MBED_CONF_SD_SPI_CS),
                          _heapBD(HEAP_BLOCK_DEVICE_SIZE, DEFAULT_BLOCK_SIZE),
                          _blockDevice(&_picoBD) {
        // Try to init SD card first
        int err = _picoBD.init();
        if (err == 0) {
            _blockDevice = &_picoBD;
        } else {
            // Fall back to heap
            _picoBD.deinit();
            _blockDevice = &_heapBD;
            _heapBD.init();
            _heap = true;
        }
    }
};

class MIDIFiles : private BlockDeviceHolder, public USBMSD {
    FATFileSystem _fs;
    vector<uint8_t> _mtrk;
    bool _usb = false;

    virtual const uint8_t *string_iproduct_desc() override {
        static const uint8_t custom_desc[] = {
            0x1A, STRING_DESCRIPTOR,
            'L', 0, 'u', 0, 'c', 0, 'a', 0, 's', 0, ' ', 0, 'G', 0, 'r', 0, 'o', 0, 'o', 0, 'v', 0, 'e', 0};
        return custom_desc;
    }

    std::string trim(const std::string &s) {
        std::string::const_iterator it = s.begin();
        while (it != s.end() && isspace(*it))
            it++;

        std::string::const_reverse_iterator rit = s.rbegin();
        while (rit.base() != it && isspace(*rit))
            rit++;

        return std::string(it, rit.base());
    }

    uint32_t get32(FILE *f) {
        uint8_t msb = fgetc(f);
        uint8_t smsb = fgetc(f);
        uint8_t tmsb = fgetc(f);
        uint8_t lsb = fgetc(f);
        return (msb << 24) | (smsb << 16) | (tmsb << 8) | lsb;
    }

    uint32_t get16(FILE *f) {
        uint8_t msb = fgetc(f);
        uint8_t lsb = fgetc(f);
        return (msb << 8) | lsb;
    }
    // Calculate Delta
    void calcDelta(uint32_t value);

    // Read Delta Value from File
    void readDelta(FILE *f, uint32_t response[]);

    // Load Test FIles
    void loadTestFiles();

   public:
    // Constructor
    MIDIFiles();

    // Create Directory Structure
    void init();

    // Connect USBMIDI Device
    void initUSB();

    // Deinit USBMIDI Device
    void deinitUSB();

    // Get USB State
    bool getUSB();

    // Save a file
    void saveToFile();

    // Read from a file
    void readFromFile(uint8_t midiMessages[320][3], uint8_t offMessages[320][3], int16_t bpm[2], string filename, uint8_t bank, int8_t &mode, int8_t &tone);

    // Get files in a bank folder
    void getFiles(uint8_t bank, string files[12]);

    // Delete a file
    void deleteFile(string filename, uint8_t bank);

    // Rename a FIle
    void renameFile(string origFilename, string filename, uint8_t bank);
};