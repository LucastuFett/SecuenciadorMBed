#pragma once

#include "blockdevice/BlockDevice.h"
#include "drivers/SPI.h"
#include "drivers/Timer.h"
#include "drivers/MbedCRC.h"
#include "drivers/DigitalOut.h"
#include "platform/platform.h"
#include "rtos/Mutex.h"
#include "hal/static_pinmap.h"

// command definitions
#define CMD0                0
#define CMD0_ARG            0x00000000
#define CMD0_CRC            0x94
#define CMD8                8
#define CMD8_ARG            0x0000001AA
#define CMD8_CRC            0x86
#define CMD9                9
#define CMD9_ARG            0x00000000
#define CMD9_CRC            0x00
#define CMD10               9
#define CMD10_ARG           0x00000000
#define CMD10_CRC           0x00
#define CMD12               12
#define CMD12_ARG           0x00000000
#define CMD12_CRC           0x00
#define CMD12_STOP_TRANSMISSION 12
#define CMD13               13
#define CMD13_ARG           0x00000000
#define CMD13_CRC           0x00
#define ACMD13            13

#define CMD17               17
#define CMD17_CRC           0x00
#define CMD24               24
#define CMD24_CRC           0x00
#define CMD38               38
#define CMD38_ARG           0x00000000
#define CMD55               55
#define CMD55_ARG           0x00000000
#define CMD55_CRC           0x00
#define CMD58               58
#define CMD58_ARG           0x00000000
#define CMD58_CRC           0x00
#define ACMD41              41
#define ACMD41_ARG          0x40000000
#define ACMD41_CRC          0x00

#define SD_IN_IDLE_STATE    0x01
#define SD_READY            0x00
#define SD_R1_NO_ERROR(X)   X < 0x02

#define R3_BYTES            4
#define R7_BYTES            4

#define CMD0_MAX_ATTEMPTS       255
#define CMD55_MAX_ATTEMPTS      255
#define SD_ERROR                1
#define SD_SUCCESS              0
#define SD_MAX_READ_ATTEMPTS    1251
#define SD_READ_START_TOKEN     0xFE
#define SD_INIT_CYCLES          80

#define SD_START_TOKEN          0xFE
#define SD_ERROR_TOKEN          0x00

#define SD_DATA_ACCEPTED        0x05
#define SD_DATA_REJECTED_CRC    0x0B
#define SD_DATA_REJECTED_WRITE  0x0D

#define SD_BLOCK_LEN            512

#ifndef MBED_SD_BLOCK_DEVICE_H
#define MBED_SD_BLOCK_DEVICE_H

/* If the target has no SPI support, then SD Card is not supported. */
#if DEVICE_SPI



#ifndef MBED_CONF_SD_SPI_MOSI
#define MBED_CONF_SD_SPI_MOSI NC
#endif
#ifndef MBED_CONF_SD_SPI_MISO
#define MBED_CONF_SD_SPI_MISO NC
#endif
#ifndef MBED_CONF_SD_SPI_CLK
#define MBED_CONF_SD_SPI_CLK NC
#endif
#ifndef MBED_CONF_SD_SPI_CS
#define MBED_CONF_SD_SPI_CS NC
#endif
#ifndef MBED_CONF_SD_INIT_FREQUENCY
#define MBED_CONF_SD_INIT_FREQUENCY 100000
#endif
#ifndef MBED_CONF_SD_TRX_FREQUENCY
#define MBED_CONF_SD_TRX_FREQUENCY  1000000
#endif
#ifndef MBED_CONF_SD_CRC_ENABLED
#define MBED_CONF_SD_CRC_ENABLED 0
#endif

/** TestSDBlockDevice class
 *
 * Access an SD Card using SPI bus
 */
class TestSDBlockDevice : public mbed::BlockDevice {

    // Only HC block size is supported. Making this a static constant reduces code size.
    static constexpr uint32_t _block_size = 512; /*!< Block size supported for SDHC card is 512 bytes  */

public:
    /** Creates an TestSDBlockDevice on a SPI bus specified by pins (using dynamic pin-map)
     *
     *  @param mosi     SPI master out, slave in pin
     *  @param miso     SPI master in, slave out pin
     *  @param sclk     SPI clock pin
     *  @param cs       SPI chip select pin.  Currently, GPIO chip selects are always used.
     *  @param hz       Clock speed of the SPI bus (defaults to 1MHz)
     *  @param crc_on   Enable cyclic redundancy check (defaults to disabled)
     */
    TestSDBlockDevice(PinName mosi = MBED_CONF_SD_SPI_MOSI,
                  PinName miso = MBED_CONF_SD_SPI_MISO,
                  PinName sclk = MBED_CONF_SD_SPI_CLK,
                  PinName cs = MBED_CONF_SD_SPI_CS,
                  uint64_t hz = MBED_CONF_SD_TRX_FREQUENCY,
                  bool crc_on = MBED_CONF_SD_CRC_ENABLED);

    /** Creates an TestSDBlockDevice on a SPI bus specified by pins (using static pin-map)
     *
     *  @param spi_pinmap Static SPI pin-map
     *  @param cs         Chip select pin (can be any GPIO)
     *  @param hz         Clock speed of the SPI bus (defaults to 1MHz)
     *  @param crc_on     Enable cyclic redundancy check (defaults to disabled)
     */
    TestSDBlockDevice(const spi_pinmap_t &spi_pinmap,
                  PinName cs = MBED_CONF_SD_SPI_CS,
                  uint64_t hz = MBED_CONF_SD_TRX_FREQUENCY,
                  bool crc_on = MBED_CONF_SD_CRC_ENABLED);

    virtual ~TestSDBlockDevice();

    /** Initialize a block device
     *
     *  @return         BD_ERROR_OK(0) - success
     *                  BD_ERROR_DEVICE_ERROR - device driver transaction failed
     *                  SD_BLOCK_DEVICE_ERROR_NO_DEVICE - device (SD card) is missing or not connected
     *                  SD_BLOCK_DEVICE_ERROR_UNUSABLE - unusable card
     *                  SD_BLOCK_DEVICE_ERROR_CRC - crc error
     */
    virtual int init();

#if DEVICE_SPI_ASYNCH
    /**
     * @brief Configure the usage of asynchronous %SPI by this class.
     *
     * By default, async %SPI is not enabled, so this class will simply busy-wait while
     * communicating with the card.  When async %SPI is enabled, %SPI operations
     * will be done in blocking asynchronous mode, so other threads may execute
     * in the background while data is going to and from the card.
     *
     * @param enabled Whether usage of async %SPI is enabled.
     * @param dma_usage_hint DMA usage hint to pass to the underlying #mbed::SPI instance.
     */
    void set_async_spi_mode(bool enabled, DMAUsage dma_usage_hint = DMAUsage::DMA_USAGE_NEVER);
#endif

    /** Deinitialize a block device
     *
     *  @return         BD_ERROR_OK(0) - success
     */
    virtual int deinit();

    /** Read blocks from a block device
     *
     *  @param buffer   Buffer to write blocks to
     *  @param addr     Address of block to begin reading from
     *  @param size     Size to read in bytes, must be a multiple of read block size
     *  @return         BD_ERROR_OK(0) - success
     *                  SD_BLOCK_DEVICE_ERROR_NO_DEVICE - device (SD card) is missing or not connected
     *                  SD_BLOCK_DEVICE_ERROR_CRC - crc error
     *                  SD_BLOCK_DEVICE_ERROR_PARAMETER - invalid parameter
     *                  SD_BLOCK_DEVICE_ERROR_NO_RESPONSE - no response from device
     *                  SD_BLOCK_DEVICE_ERROR_UNSUPPORTED - unsupported command
     */
    virtual int read(void *buffer, mbed::bd_addr_t addr, mbed::bd_size_t size);

    /** Program blocks to a block device
     *
     *  @note The blocks must be erased prior to programming
     *
     *  @param buffer   Buffer of data to write to blocks
     *  @param addr     Address of block to begin writing to
     *  @param size     Size to write in bytes. Must be a multiple of program block size
     *  @return         BD_ERROR_OK(0) - success
     *                  SD_BLOCK_DEVICE_ERROR_NO_DEVICE - device (SD card) is missing or not connected
     *                  SD_BLOCK_DEVICE_ERROR_CRC - crc error
     *                  SD_BLOCK_DEVICE_ERROR_PARAMETER - invalid parameter
     *                  SD_BLOCK_DEVICE_ERROR_UNSUPPORTED - unsupported command
     *                  SD_BLOCK_DEVICE_ERROR_NO_INIT - device is not initialized
     *                  SD_BLOCK_DEVICE_ERROR_WRITE - SPI write error
     *                  SD_BLOCK_DEVICE_ERROR_ERASE - erase error
     */
    virtual int program(const void *buffer, mbed::bd_addr_t addr, mbed::bd_size_t size);

    /** Mark blocks as no longer in use
     *
     *  This function provides a hint to the underlying block device that a region of blocks
     *  is no longer in use and may be erased without side effects. Erase must still be called
     *  before programming, but trimming allows flash-translation-layers to schedule erases when
     *  the device is not busy.
     *
     *  @param addr     Address of block to mark as unused
     *  @param size     Size to mark as unused in bytes, must be a multiple of erase block size
     *  @return         BD_ERROR_OK(0) - success
     *                  SD_BLOCK_DEVICE_ERROR_NO_DEVICE - device (SD card) is missing or not connected
     *                  SD_BLOCK_DEVICE_ERROR_CRC - crc error
     *                  SD_BLOCK_DEVICE_ERROR_PARAMETER - invalid parameter
     *                  SD_BLOCK_DEVICE_ERROR_UNSUPPORTED - unsupported command
     *                  SD_BLOCK_DEVICE_ERROR_NO_INIT - device is not initialized
     *                  SD_BLOCK_DEVICE_ERROR_ERASE - erase error
     */
    virtual int trim(mbed::bd_addr_t addr, mbed::bd_size_t size);

    /** Get the size of a readable block
     *
     *  @return         Size of a readable block in bytes
     */
    virtual mbed::bd_size_t get_read_size() const;

    /** Get the size of a programmable block
     *
     *  @return         Size of a programmable block in bytes
     *  @note Must be a multiple of the read size
     */
    virtual mbed::bd_size_t get_program_size() const;

    /** Get the total size of the underlying device
     *
     *  @return         Size of the underlying device in bytes
     */
    virtual mbed::bd_size_t size() const;

    /** Enable or disable debugging
     *
     *  @param dbg        State of debugging
     */
    virtual void debug(bool dbg);

    /** Set the transfer frequency
     *
     *  @param freq     Transfer frequency
     *  @note Max frequency supported is 25MHZ
     */
    virtual int frequency(uint64_t freq);

    /** Get the BlockDevice class type.
     *
     *  @return         A string representation of the BlockDevice class type.
     */
    virtual const char *get_type() const;

private:
    
    int _cmd(uint8_t cmd, uint32_t arg, bool isAcmd = 0, uint32_t *resp = NULL);
    int _cmd8();
    uint8_t _SDSPI_goIdle();
    //void _SD_sendIfCond(uint8_t *res);
    uint8_t _SD_sendApp();
    uint8_t _SD_sendOpCond();

    /*  Move the SD Card into the SPI Mode idle state
     *
     *  The card is transitioned from SD Card mode to SPI mode by sending the
     *  CMD0 (GO_IDLE_STATE) command with CS asserted. See the notes in the
     *  "SPI Startup" section of the comments at the head of the
     *  implementation file for further details and specification references.
     *
     *  @return         Response form the card. R1_IDLE_STATE (0x1), the successful
     *                  response from CMD0. R1_XXX_XXX for more response
     */
    uint32_t _go_idle_state();
    int _initialise_card();

    mbed::bd_size_t _sectors;
    mbed::bd_size_t _sd_sectors();

    bool _is_valid_trim(mbed::bd_addr_t addr, mbed::bd_size_t size);

    /* SPI functions */
    mbed::Timer _spi_timer;               /**< Timer Class object used for busy wait */
    uint32_t _init_sck;             /**< Initial SPI frequency */
    uint32_t _transfer_sck;         /**< SPI frequency during data transfer/after initialization */
    mbed::SPI _spi;                       /**< SPI Class object */
    mbed::DigitalOut _cs;                /**< Chip Select pin */

    /* SPI initialization function */
    void _spi_init();
    uint8_t _cmd_spi(uint8_t cmd, uint32_t arg);
    void _spi_wait(uint8_t count);

    bool _wait_token(uint8_t token);        /**< Wait for token */
    bool _wait_ready(std::chrono::duration<uint32_t, std::milli> timeout = std::chrono::milliseconds{300});    /**< 300ms default wait for card to be ready */
    int _read(uint8_t *buffer, uint32_t length);
    int _read_bytes(uint8_t *buffer, uint32_t length);
    uint8_t _write(const uint8_t *buffer, uint8_t token, uint32_t length);
    int _freq(void);
    void _preclock_then_select();
    void _postclock_then_deselect();

    virtual void lock()
    {
        _mutex.lock();
    }

    virtual void unlock()
    {
        _mutex.unlock();
    }

    rtos::Mutex _mutex;
    uint32_t _erase_size;
    bool _is_initialized;
    bool _dbg;
    uint32_t _init_ref_count;

#if MBED_CONF_SD_CRC_ENABLED
    bool _crc_on;
#endif

#if DEVICE_SPI_ASYNCH
    bool _async_spi_enabled = false;
    mbed::StaticCacheAlignedBuffer<uint8_t, _block_size> _async_data_buffer;
#endif
};

#endif  /* DEVICE_SPI */

#endif  /* MBED_SD_BLOCK_DEVICE_H */
