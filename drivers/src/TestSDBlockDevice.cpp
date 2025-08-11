
/* If the target has no SPI support then SDCard is not supported */
#if DEVICE_SPI

#include "TestSDBlockDevice.h"
#include "rtos/ThisThread.h"
#include "platform/mbed_debug.h"
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <errno.h>

using namespace mbed;
using namespace std::chrono;
using namespace rtos;

#ifndef MBED_CONF_SD_CMD_TIMEOUT
#define MBED_CONF_SD_CMD_TIMEOUT                 5000   /*!< Timeout in ms for response */
#endif

#ifndef MBED_CONF_SD_CMD0_IDLE_STATE_RETRIES
#define MBED_CONF_SD_CMD0_IDLE_STATE_RETRIES     5      /*!< Number of retries for sending CMDO */
#endif

#ifndef MBED_CONF_SD_INIT_FREQUENCY
#define MBED_CONF_SD_INIT_FREQUENCY              100000 /*!< Initialization frequency Range (100KHz-400KHz) */
#endif


#define SD_COMMAND_TIMEOUT                       milliseconds{MBED_CONF_SD_CMD_TIMEOUT}
#define SD_CMD0_GO_IDLE_STATE_RETRIES            MBED_CONF_SD_CMD0_IDLE_STATE_RETRIES
#define SD_DBG                                   0      /*!< 1 - Enable debugging */
#define SD_CMD_TRACE                             0      /*!< 1 - Enable SD command tracing */

#define SD_BLOCK_DEVICE_ERROR_WOULD_BLOCK        -5001  /*!< operation would block */
#define SD_BLOCK_DEVICE_ERROR_UNSUPPORTED        -5002  /*!< unsupported operation */
#define SD_BLOCK_DEVICE_ERROR_PARAMETER          -5003  /*!< invalid parameter */
#define SD_BLOCK_DEVICE_ERROR_NO_INIT            -5004  /*!< uninitialized */
#define SD_BLOCK_DEVICE_ERROR_NO_DEVICE          -5005  /*!< device is missing or not connected */
#define SD_BLOCK_DEVICE_ERROR_WRITE_PROTECTED    -5006  /*!< write protected */
#define SD_BLOCK_DEVICE_ERROR_UNUSABLE           -5007  /*!< unusable card */
#define SD_BLOCK_DEVICE_ERROR_NO_RESPONSE        -5008  /*!< No response from device */
#define SD_BLOCK_DEVICE_ERROR_CRC                -5009  /*!< CRC error */
#define SD_BLOCK_DEVICE_ERROR_ERASE              -5010  /*!< Erase error: reset/sequence */
#define SD_BLOCK_DEVICE_ERROR_WRITE              -5011  /*!< SPI Write error: !SPI_DATA_ACCEPTED */

#define WRITE_BL_PARTIAL                         0      /*!< Partial block write - Not supported */
#define SPI_CMD(x) (0x40 | (x & 0x3f))

/* R1 Response Format */
#define R1_NO_RESPONSE          (0xFF)
#define R1_RESPONSE_RECV        (0x80)
#define R1_IDLE_STATE           (1 << 0)
#define R1_ERASE_RESET          (1 << 1)
#define R1_ILLEGAL_COMMAND      (1 << 2)
#define R1_COM_CRC_ERROR        (1 << 3)
#define R1_ERASE_SEQUENCE_ERROR (1 << 4)
#define R1_ADDRESS_ERROR        (1 << 5)
#define R1_PARAMETER_ERROR      (1 << 6)

// Types
#define SDCARD_NONE              0           /**< No card is present */
#define SDCARD_V1                1           /**< v1.x Standard Capacity */
#define SDCARD_V2                2           /**< v2.x Standard capacity SD card */
#define SDCARD_V2HC              3           /**< v2.x High capacity SD card */
#define CARD_UNKNOWN             4           /**< Unknown or unsupported card */

/* SIZE in Bytes */
#define PACKET_SIZE              6           /*!< SD Packet size CMD+ARG+CRC */
#define R1_RESPONSE_SIZE         1           /*!< Size of R1 response */
#define R2_RESPONSE_SIZE         2           /*!< Size of R2 response */
#define R3_R7_RESPONSE_SIZE      5           /*!< Size of R3/R7 response */

/* R1b Response */
#define DEVICE_BUSY             (0x00)

/* R2 Response Format */
#define R2_CARD_LOCKED          (1 << 0)
#define R2_CMD_FAILED           (1 << 1)
#define R2_ERROR                (1 << 2)
#define R2_CC_ERROR             (1 << 3)
#define R2_CC_FAILED            (1 << 4)
#define R2_WP_VIOLATION         (1 << 5)
#define R2_ERASE_PARAM          (1 << 6)
#define R2_OUT_OF_RANGE         (1 << 7)

/* R3 Response : OCR Register */
#define OCR_HCS_CCS             (0x1 << 30)
#define OCR_LOW_VOLTAGE         (0x01 << 24)
#define OCR_3_3V                (0x1 << 20)

/* R7 response pattern for CMD8 */
#define CMD8_PATTERN             (0xAA)

/*  CRC Enable  */
#define CRC_ENABLE               (0)         /*!< CRC 1 - Enable 0 - Disable */

/* Control Tokens   */
#define SPI_DATA_RESPONSE_MASK   (0x1F)
#define SPI_DATA_ACCEPTED        (0x05)
#define SPI_DATA_CRC_ERROR       (0x0B)
#define SPI_DATA_WRITE_ERROR     (0x0D)
#define SPI_START_BLOCK          (0xFE)      /*!< For Single Block Read/Write and Multiple Block Read */
#define SPI_START_BLK_MUL_WRITE  (0xFC)      /*!< Start Multi-block write */
#define SPI_STOP_TRAN            (0xFD)      /*!< Stop Multi-block write */

#define SPI_DATA_READ_ERROR_MASK (0xF)       /*!< Data Error Token: 4 LSB bits */
#define SPI_READ_ERROR           (0x1 << 0)  /*!< Error */
#define SPI_READ_ERROR_CC        (0x1 << 1)  /*!< CC Error*/
#define SPI_READ_ERROR_ECC_C     (0x1 << 2)  /*!< Card ECC failed */
#define SPI_READ_ERROR_OFR       (0x1 << 3)  /*!< Out of Range */

#if MBED_CONF_SD_CRC_ENABLED
TestSDBlockDevice::TestSDBlockDevice(PinName mosi, PinName miso, PinName sclk, PinName cs, uint64_t hz, bool crc_on)
    : _sectors(0), _spi(mosi, miso, sclk, cs, use_gpio_ssel), _is_initialized(0),
      _init_ref_count(0), _crc_on(crc_on)
#else
TestSDBlockDevice::TestSDBlockDevice(PinName mosi, PinName miso, PinName sclk, PinName cs, uint64_t hz, bool crc_on)
    : _sectors(0), _spi(mosi, miso, sclk, NC), _is_initialized(0),
      _init_ref_count(0), _cs(cs)
#endif
{
    // Set default to 100kHz for initialisation and 1MHz for data transfer
    static_assert(((MBED_CONF_SD_INIT_FREQUENCY >= 100000) && (MBED_CONF_SD_INIT_FREQUENCY <= 400000)),
                  "Initialization frequency should be between 100KHz to 400KHz");
    _init_sck = MBED_CONF_SD_INIT_FREQUENCY;
    _transfer_sck = hz;

    _erase_size = _block_size;
}

#if MBED_CONF_SD_CRC_ENABLED
TestSDBlockDevice::TestSDBlockDevice(const spi_pinmap_t &spi_pinmap, PinName cs, uint64_t hz, bool crc_on)
    : _sectors(0), _spi(spi_pinmap, cs), _is_initialized(0),
      _init_ref_count(0), _crc_on(crc_on)
#else
TestSDBlockDevice::TestSDBlockDevice(const spi_pinmap_t &spi_pinmap, PinName cs, uint64_t hz, bool crc_on)
    : _sectors(0), _spi(spi_pinmap, NC), _is_initialized(0),
      _init_ref_count(0), _cs(cs)
#endif
{
    // Set default to 100kHz for initialisation and 1MHz for data transfer
    static_assert(((MBED_CONF_SD_INIT_FREQUENCY >= 100000) && (MBED_CONF_SD_INIT_FREQUENCY <= 400000)),
                  "Initialization frequency should be between 100KHz to 400KHz");
    _init_sck = MBED_CONF_SD_INIT_FREQUENCY;
    _transfer_sck = hz;

    _erase_size = _block_size;
}

TestSDBlockDevice::~TestSDBlockDevice()
{
    if (_is_initialized) {
        deinit();
    }
}

int TestSDBlockDevice::_initialise_card()
{
    uint8_t res[5], cmdAttempts = 0;
    int32_t status = BD_ERROR_OK;

    // Initialize the SPI interface: Card by default is in SD mode
    _spi_init();
    
// command card to idle
    while((res[0] = _SDSPI_goIdle()) != 0x01)
    {
        cmdAttempts++;
        //printf("Idle Attempts: %i\n",cmdAttempts);
        if(cmdAttempts > 10) return SD_ERROR;
    }

    // send interface conditions
    status = _cmd8();
    if(BD_ERROR_OK != status)
    {
        return SD_ERROR;
    }
    //printf("IfCond\n");
    // check echo pattern
    //printf("Echo\n");
    // attempt to initialize card
    cmdAttempts = 0;
    do
    {
        if(cmdAttempts > 100) return SD_ERROR;

        // send app cmd
        res[0] = _SD_sendApp();
        ThisThread::sleep_for(200ms);
        // if no error in response
        if(res[0] < 2)
        {
            //printf("No Error on App\n");
            res[0] = _SD_sendOpCond();
            ThisThread::sleep_for(100ms);
        }
        //printf("CMD41 Res: %02X\n", res[0]);
        // wait
        ThisThread::sleep_for(100ms);

        cmdAttempts++;
    }
    while(res[0] != SD_READY);

    return SD_SUCCESS;
}


int TestSDBlockDevice::init()
{
    int err;

    lock();

    if (!_is_initialized) {
        _init_ref_count = 0;
    }

    _init_ref_count++;

    if (_init_ref_count != 1) {
        goto end;
    }

    err = _initialise_card();
    _is_initialized = (err == SD_SUCCESS);
    if (!_is_initialized) {
        debug_if(SD_DBG, "Fail to initialize card\n");
        unlock();
        return err;
    }

end:
    unlock();
    return BD_ERROR_OK;
}

#if DEVICE_SPI_ASYNCH
void TestSDBlockDevice::set_async_spi_mode(bool enabled, DMAUsage dma_usage_hint)
{
    _async_spi_enabled = enabled;
    _spi.set_dma_usage(dma_usage_hint);
}
#endif

int TestSDBlockDevice::deinit()
{
    lock();

    if (!_is_initialized) {
        _init_ref_count = 0;
        goto end;
    }

    _init_ref_count--;

    if (_init_ref_count) {
        goto end;
    }

    _is_initialized = false;
    _sectors = 0;

end:
    unlock();
    return BD_ERROR_OK;
}


int TestSDBlockDevice::program(const void *b, bd_addr_t addr, bd_size_t size)
{
    if (!is_valid_program(addr, size)) {
        return SD_BLOCK_DEVICE_ERROR_PARAMETER;
    }

    lock();
    if (!_is_initialized) {
        unlock();
        return SD_BLOCK_DEVICE_ERROR_NO_INIT;
    }

    const uint8_t *buffer = static_cast<const uint8_t *>(b);
    int status = BD_ERROR_OK;
    uint8_t response;

    // Get block count
    size_t blockCnt = size / _block_size;

    // SDSC Card (CCS=0) uses byte unit address
    // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit)
    addr = addr / _block_size;
    

    // Send command to perform write operation
    if (blockCnt == 1) {
        // Single block write command
        if (BD_ERROR_OK != (status = _cmd(CMD24, addr))) {
            unlock();
            return status;
        }

        // Write data
        response = _write(buffer, SPI_START_BLOCK, _block_size);

        // Only CRC and general write error are communicated via response token
        if (response != SPI_DATA_ACCEPTED) {
            debug_if(SD_DBG, "Single Block Write failed: 0x%x \n", response);
            status = SD_BLOCK_DEVICE_ERROR_WRITE;
        }
    } else {
        // Pre-erase setting prior to multiple block write operation
        _cmd(23, blockCnt, 1);

        // Multiple block write command
        if (BD_ERROR_OK != (status = _cmd(25, addr))) {
            unlock();
            return status;
        }

        // Write the data: one block at a time
        do {
            response = _write(buffer, SPI_START_BLK_MUL_WRITE, _block_size);
            if (response != SPI_DATA_ACCEPTED) {
                debug_if(SD_DBG, "Multiple Block Write failed: 0x%x \n", response);
                break;
            }
            buffer += _block_size;
        } while (--blockCnt);     // Receive all blocks of data

        /* In a Multiple Block write operation, the stop transmission will be done by
         * sending 'Stop Tran' token instead of 'Start Block' token at the beginning
         * of the next block
         */
        _spi.write(SPI_STOP_TRAN);
    }

    _postclock_then_deselect();
    unlock();
    return status;
}

int TestSDBlockDevice::read(void *b, bd_addr_t addr, bd_size_t size)
{
    if (!is_valid_read(addr, size)) {
        return SD_BLOCK_DEVICE_ERROR_PARAMETER;
    }

    lock();
    if (!_is_initialized) {
        unlock();
        return SD_BLOCK_DEVICE_ERROR_PARAMETER;
    }

    uint8_t *buffer = static_cast<uint8_t *>(b);
    int status = BD_ERROR_OK;
    size_t blockCnt =  size / _block_size;

    // SDSC Card (CCS=0) uses byte unit address
    // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit)
        addr = addr / _block_size;

    // Write command ro receive data
    if (blockCnt > 1) {
        status = _cmd(18, addr);
    } else {
        status = _cmd(17, addr);
    }
    if (BD_ERROR_OK != status) {
        unlock();
        return status;
    }

    // receive the data : one block at a time
    while (blockCnt) {
        if (0 != _read(buffer, _block_size)) {
            status = SD_BLOCK_DEVICE_ERROR_NO_RESPONSE;
            break;
        }
        buffer += _block_size;
        --blockCnt;
    }
    _postclock_then_deselect();

    // Send CMD12(0x00000000) to stop the transmission for multi-block transfer
    if (size > _block_size) {
        status = _cmd(CMD12_STOP_TRANSMISSION, 0x0);
    }
    unlock();
    return status;
}

bool TestSDBlockDevice::_is_valid_trim(bd_addr_t addr, bd_size_t size)
{
    return (
               addr % _erase_size == 0 &&
               size % _erase_size == 0 &&
               addr + size <= this->size());
}

int TestSDBlockDevice::trim(bd_addr_t addr, bd_size_t size)
{
    if (!_is_valid_trim(addr, size)) {
        return SD_BLOCK_DEVICE_ERROR_PARAMETER;
    }

    lock();
    if (!_is_initialized) {
        unlock();
        return SD_BLOCK_DEVICE_ERROR_NO_INIT;
    }
    int status = BD_ERROR_OK;

    size -= _block_size;
    // SDSC Card (CCS=0) uses byte unit address
    // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit)
    size = size / _block_size;
    addr = addr / _block_size;
    

    // Start lba sent in start command
    if (BD_ERROR_OK != (status = _cmd(32, addr))) {
        unlock();
        return status;
    }

    // End lba = addr+size sent in end addr command
    if (BD_ERROR_OK != (status = _cmd(33, addr + size))) {
        unlock();
        return status;
    }
    status = _cmd(38, 0x0);
    unlock();
    return status;
}

bd_size_t TestSDBlockDevice::get_read_size() const
{
    return _block_size;
}

bd_size_t TestSDBlockDevice::get_program_size() const
{
    return _block_size;
}

bd_size_t TestSDBlockDevice::size() const
{
    return _block_size * _sectors;
}

const char *TestSDBlockDevice::get_type() const
{
    return "SD";
}

void TestSDBlockDevice::debug(bool dbg)
{
    _dbg = dbg;
}

int TestSDBlockDevice::frequency(uint64_t freq)
{
    lock();
    _transfer_sck = freq;
    int err = _freq();
    unlock();
    return err;
}

// PRIVATE FUNCTIONS
int TestSDBlockDevice::_freq(void)
{
    // Max frequency supported is 25MHZ
    if (_transfer_sck <= 25000000) {
        _spi.frequency(_transfer_sck);
        return 0;
    } else {  // TODO: Switch function to be implemented for higher frequency
        _transfer_sck = 25000000;
        _spi.frequency(_transfer_sck);
        return -EINVAL;
    }
}

uint8_t TestSDBlockDevice::_cmd_spi(uint8_t cmd, uint32_t arg)
{
    uint8_t response;
    char cmdPacket[PACKET_SIZE];

    // Prepare the command packet
    cmdPacket[0] = SPI_CMD(cmd);
    cmdPacket[1] = (arg >> 24);
    cmdPacket[2] = (arg >> 16);
    cmdPacket[3] = (arg >> 8);
    cmdPacket[4] = (arg >> 0);

#if MBED_CONF_SD_CRC_ENABLED
    if (_crc_on) {
        MbedCRC<POLY_7BIT_SD, 7> crc7;
        uint32_t crc;
        crc7.compute(cmdPacket, 5, &crc);
        cmdPacket[5] = ((uint8_t) crc << 1) | 0x01;
    } else
#endif
    {
        // CMD0 is executed in SD mode, hence should have correct CRC
        // CMD8 CRC verification is always enabled
        switch (cmd) {
            case CMD0:
                cmdPacket[5] = 0x95;
                break;
            case CMD8:
                cmdPacket[5] = 0x87;
                break;
            default:
                cmdPacket[5] = 0xFF;    // Make sure bit 0-End bit is high
                break;
        }
    }

    // send a command
#if DEVICE_SPI_ASYNCH
    if (_async_spi_enabled) {
        _spi.transfer_and_wait(cmdPacket, PACKET_SIZE, nullptr, 0);
    } else
#endif
    {
        _spi.write(cmdPacket, PACKET_SIZE, nullptr, 0);
    }

    // The received byte immediataly following CMD12 is a stuff byte,
    // it should be discarded before receive the response of the CMD12.
    if (CMD12_STOP_TRANSMISSION == cmd) {
        _spi.write(SPI_FILL_CHAR);
    }

    // Loop for response: Response is sent back within command response time (NCR), 0 to 8 bytes for SDC
    for (int i = 0; i < 0x10; i++) {
        response = _spi.write(SPI_FILL_CHAR);
        // Got the response
        if (!(response & R1_RESPONSE_RECV)) {
            break;
        }
    }
    return response;
}

int TestSDBlockDevice::_cmd(uint8_t cmd, uint32_t arg, bool isAcmd, uint32_t *resp)
{
    int32_t status = BD_ERROR_OK;
    uint32_t response;

    // Select card and wait for card to be ready before sending next command
    // Note: next command will fail if card is not ready
    _preclock_then_select();

    // No need to wait for card to be ready when sending the stop command
    if (CMD12_STOP_TRANSMISSION != cmd) {
        if (false == _wait_ready(SD_COMMAND_TIMEOUT)) {
            debug_if(SD_DBG, "Card not ready yet \n");
        }
    }


    // Re-try command
    for (int i = 0; i < 3; i++) {
        // Send CMD55 for APP command first
        if (isAcmd) {
            response = _cmd_spi(CMD55, 0x0);
            // Wait for card to be ready after CMD55
            if (false == _wait_ready(SD_COMMAND_TIMEOUT)) {
                debug_if(SD_DBG, "Card not ready yet \n");
            }
        }

        // Send command over SPI interface
        response = _cmd_spi(cmd, arg);
        if (R1_NO_RESPONSE == response) {
            debug_if(SD_DBG, "No response CMD:%d \n", cmd);
            continue;
        }
        break;
    }

    // Pass the response to the command call if required
    if (NULL != resp) {
        *resp = response;
    }

    // Process the response R1  : Exit on CRC/Illegal command error/No response
    if (R1_NO_RESPONSE == response) {
        _postclock_then_deselect();
        debug_if(SD_DBG, "No response CMD:%d response: 0x%" PRIx32 "\n", cmd, response);
        return SD_BLOCK_DEVICE_ERROR_NO_DEVICE;         // No device
    }
    if (response & R1_COM_CRC_ERROR) {
        _postclock_then_deselect();
        debug_if(SD_DBG, "CRC error CMD:%d response 0x%" PRIx32 "\n", cmd, response);
        return SD_BLOCK_DEVICE_ERROR_CRC;                // CRC error
    }
    if (response & R1_ILLEGAL_COMMAND) {
        _postclock_then_deselect();
        debug_if(SD_DBG, "Illegal command CMD:%d response 0x%" PRIx32 "\n", cmd, response);
        return SD_BLOCK_DEVICE_ERROR_UNSUPPORTED;      // Command not supported
    }

    debug_if(_dbg, "CMD:%d \t arg:0x%" PRIx32 " \t Response:0x%" PRIx32 "\n", cmd, arg, response);
    // Set status for other errors
    if ((response & R1_ERASE_RESET) || (response & R1_ERASE_SEQUENCE_ERROR)) {
        status = SD_BLOCK_DEVICE_ERROR_ERASE;            // Erase error
    } else if ((response & R1_ADDRESS_ERROR) || (response & R1_PARAMETER_ERROR)) {
        // Misaligned address / invalid address block length
        status = SD_BLOCK_DEVICE_ERROR_PARAMETER;
    }

    // Get rest of the response part for other commands
    switch (cmd) {
        case CMD8:             // Response R7
            debug_if(_dbg, "V2-Version Card\n");
        // Note: No break here, need to read rest of the response
        case CMD58:                // Response R3
            response  = (_spi.write(SPI_FILL_CHAR) << 24);
            response |= (_spi.write(SPI_FILL_CHAR) << 16);
            response |= (_spi.write(SPI_FILL_CHAR) << 8);
            response |= _spi.write(SPI_FILL_CHAR);
            debug_if(_dbg, "R3/R7: 0x%" PRIx32 "\n", response);
            break;

        case CMD12_STOP_TRANSMISSION:       // Response R1b
        case CMD38:
            _wait_ready(SD_COMMAND_TIMEOUT);
            break;

        case ACMD13:             // Response R2
            response = _spi.write(SPI_FILL_CHAR);
            debug_if(_dbg, "R2: 0x%" PRIx32 "\n", response);
            break;

        default:                            // Response R1
            break;
    }

    // Pass the updated response to the command
    if (NULL != resp) {
        *resp = response;
    }

    // Do not deselect card if read is in progress.
    if (((CMD9 == cmd) || (22 == cmd) ||
            (24 == cmd) || (25 == cmd) ||
            (17 == cmd) || (18 == cmd))
            && (BD_ERROR_OK == status)) {
        return BD_ERROR_OK;
    }
    // Deselect card
    _postclock_then_deselect();
    return status;
}

int TestSDBlockDevice::_cmd8()
{
    uint32_t arg = (CMD8_PATTERN << 0);         // [7:0]check pattern
    uint32_t response = 0;
    int32_t status = BD_ERROR_OK;

    arg |= (0x1 << 8);  // 2.7-3.6V             // [11:8]supply voltage(VHS)

    status = _cmd(CMD8, arg, 0x0, &response);
    return status;
}

uint32_t TestSDBlockDevice::_go_idle_state()
{
    uint32_t response;

    /* Reseting the MCU SPI master may not reset the on-board SDCard, in which
     * case when MCU power-on occurs the SDCard will resume operations as
     * though there was no reset. In this scenario the first CMD0 will
     * not be interpreted as a command and get lost. For some cards retrying
     * the command overcomes this situation. */
    for (int i = 0; i < SD_CMD0_GO_IDLE_STATE_RETRIES; i++) {
        _cmd(CMD0, 0x0, 0x0, &response);
        if (R1_IDLE_STATE == response) {
            break;
        }
        rtos::ThisThread::sleep_for(1ms);
    }
    return response;
}

int TestSDBlockDevice::_read_bytes(uint8_t *buffer, uint32_t length)
{
    uint16_t crc;

    // read until start byte (0xFE)
    if (false == _wait_token(SPI_START_BLOCK)) {
        debug_if(SD_DBG, "Read timeout\n");
        _postclock_then_deselect();
        return SD_BLOCK_DEVICE_ERROR_NO_RESPONSE;
    }

    // read data
#if DEVICE_SPI_ASYNCH
    if (_async_spi_enabled) {
        if (length > _async_data_buffer.capacity()) {
            return SD_BLOCK_DEVICE_ERROR_PARAMETER;
        }

        // Do read into cache aligned buffer, then copy data into application buffer
        if (_spi.transfer_and_wait(nullptr, 0, _async_data_buffer, length) != 0) {
            return SD_BLOCK_DEVICE_ERROR_WRITE;
        }
        memcpy(buffer, _async_data_buffer.data(), length);
    } else
#endif
    {
        _spi.write(nullptr, 0, buffer, length);
    }

    // Read the CRC16 checksum for the data block
    crc = (_spi.write(SPI_FILL_CHAR) << 8);
    crc |= _spi.write(SPI_FILL_CHAR);

#if MBED_CONF_SD_CRC_ENABLED
    if (_crc_on) {
        mbed::MbedCRC<POLY_16BIT_CCITT, 16> crc16(0, 0, false, false);
        uint32_t crc_result;
        // Compute and verify checksum
        crc16.compute(buffer, length, &crc_result);
        if (crc_result != crc) {
            debug_if(SD_DBG, "_read_bytes: Invalid CRC received 0x%" PRIx16 " result of computation 0x%" PRIx32 "\n",
                     crc, crc_result);
            _postclock_then_deselect();
            return SD_BLOCK_DEVICE_ERROR_CRC;
        }
    }
#endif

    _postclock_then_deselect();
    return 0;
}

int TestSDBlockDevice::_read(uint8_t *buffer, uint32_t length)
{
    uint16_t crc;

    // read until start byte (0xFE)
    if (false == _wait_token(SPI_START_BLOCK)) {
        debug_if(SD_DBG, "Read timeout\n");
        return SD_BLOCK_DEVICE_ERROR_NO_RESPONSE;
    }

    // read data
#if DEVICE_SPI_ASYNCH
    if (_async_spi_enabled) {
        if (length > _async_data_buffer.capacity()) {
            return SD_BLOCK_DEVICE_ERROR_PARAMETER;
        }

        // Do read into cache aligned buffer, then copy data into application buffer
        if (_spi.transfer_and_wait(nullptr, 0, _async_data_buffer, length) != 0) {
            return SD_BLOCK_DEVICE_ERROR_WRITE;
        }
        memcpy(buffer, _async_data_buffer.data(), length);
    } else
#endif
    {
        _spi.write(NULL, 0, (char *) buffer, length);
    }

    // Read the CRC16 checksum for the data block
    crc = (_spi.write(SPI_FILL_CHAR) << 8);
    crc |= _spi.write(SPI_FILL_CHAR);

#if MBED_CONF_SD_CRC_ENABLED
    if (_crc_on) {
        mbed::MbedCRC<POLY_16BIT_CCITT, 16> crc16(0, 0, false, false);
        uint32_t crc_result;
        // Compute and verify checksum
        crc16.compute((void *)buffer, length, &crc_result);
        if ((uint16_t)crc_result != crc) {
            debug_if(SD_DBG, "_read_bytes: Invalid CRC received 0x%" PRIx16 " result of computation 0x%" PRIx16 "\n",
                     crc, (uint16_t)crc_result);
            return SD_BLOCK_DEVICE_ERROR_CRC;
        }
    }
#endif

    return 0;
}

uint8_t TestSDBlockDevice::_write(const uint8_t *buffer, uint8_t token, uint32_t length)
{

    uint32_t crc = (~0);
    uint8_t response = 0xFF;

    // indicate start of block
    _spi.write(token);

    // write the data
#if DEVICE_SPI_ASYNCH
    if (_async_spi_enabled) {
        _spi.transfer_and_wait(buffer, length, nullptr, 0);
    } else
#endif
    {
        _spi.write(buffer, length, nullptr, 0);
    }

#if MBED_CONF_SD_CRC_ENABLED
    if (_crc_on) {
        mbed::MbedCRC<POLY_16BIT_CCITT, 16> crc16(0, 0, false, false);
        // Compute CRC
        crc16.compute(buffer, length, &crc);
    }
#endif

    // write the checksum CRC16
    _spi.write(crc >> 8);
    _spi.write(crc);


    // check the response token
    response = _spi.write(SPI_FILL_CHAR);

    // Wait for last block to be written
    if (false == _wait_ready(SD_COMMAND_TIMEOUT)) {
        debug_if(SD_DBG, "Card not ready yet \n");
    }

    return (response & SPI_DATA_RESPONSE_MASK);
}

static uint32_t ext_bits(unsigned char *data, int msb, int lsb)
{
    uint32_t bits = 0;
    uint32_t size = 1 + msb - lsb;
    for (uint32_t i = 0; i < size; i++) {
        uint32_t position = lsb + i;
        uint32_t byte = 15 - (position >> 3);
        uint32_t bit = position & 0x7;
        uint32_t value = (data[byte] >> bit) & 1;
        bits |= value << i;
    }
    return bits;
}

bd_size_t TestSDBlockDevice::_sd_sectors()
{
    uint32_t c_size, c_size_mult, read_bl_len;
    uint32_t block_len, mult, blocknr;
    uint32_t hc_c_size;
    bd_size_t blocks = 0, capacity = 0;

    // CMD9, Response R2 (R1 byte + 16-byte block read)
    if (_cmd(CMD9, 0x0) != 0x0) {
        debug_if(SD_DBG, "Didn't get a response from the disk\n");
        return 0;
    }
#ifdef DEVICE_SPI_ASYNCH
    StaticCacheAlignedBuffer<uint8_t, 16> csd_buffer;
    uint8_t *csd = csd_buffer.data();
#else
    uint8_t csd[16];
#endif

    if (_read_bytes(csd, 16) != 0) {
        debug_if(SD_DBG, "Couldn't read csd response from disk\n");
        return 0;
    }

    // csd_structure : csd[127:126]
    int csd_structure = ext_bits(csd, 127, 126);
    switch (csd_structure) {
        case 0:
            c_size = ext_bits(csd, 73, 62);              // c_size        : csd[73:62]
            c_size_mult = ext_bits(csd, 49, 47);         // c_size_mult   : csd[49:47]
            read_bl_len = ext_bits(csd, 83, 80);         // read_bl_len   : csd[83:80] - the *maximum* read block length
            block_len = 1 << read_bl_len;                // BLOCK_LEN = 2^READ_BL_LEN
            mult = 1 << (c_size_mult + 2);               // MULT = 2^C_SIZE_MULT+2 (C_SIZE_MULT < 8)
            blocknr = (c_size + 1) * mult;               // BLOCKNR = (C_SIZE+1) * MULT
            capacity = (bd_size_t) blocknr * block_len;  // memory capacity = BLOCKNR * BLOCK_LEN
            blocks = capacity / _block_size;
            debug_if(SD_DBG, "Standard Capacity: c_size: %" PRIu32 " \n", c_size);
            debug_if(SD_DBG, "Sectors: 0x%" PRIx64 " : %" PRIu64 "\n", blocks, blocks);
            debug_if(SD_DBG, "Capacity: 0x%" PRIx64 " : %" PRIu64 " MB\n", capacity, (capacity / (1024U * 1024U)));

            // ERASE_BLK_EN = 1: Erase in multiple of 512 bytes supported
            if (ext_bits(csd, 46, 46)) {
                _erase_size = _block_size;
            } else {
                // ERASE_BLK_EN = 1: Erase in multiple of SECTOR_SIZE supported
                _erase_size = _block_size * (ext_bits(csd, 45, 39) + 1);
            }
            break;

        case 1:
            hc_c_size = ext_bits(csd, 69, 48);            // device size : C_SIZE : [69:48]
            blocks = (hc_c_size + 1) << 10;               // block count = C_SIZE+1) * 1K byte (512B is block size)
            debug_if(SD_DBG, "SDHC/SDXC Card: hc_c_size: %" PRIu32 " \n", hc_c_size);
            debug_if(SD_DBG, "Sectors: 0x%" PRIx64 "x : %" PRIu64 "\n", blocks, blocks);
            debug_if(SD_DBG, "Capacity: %" PRIu64 " MB\n", (blocks / (2048U)));
            // ERASE_BLK_EN is fixed to 1, which means host can erase one or multiple of 512 bytes.
            _erase_size = _block_size;
            break;

        default:
            debug_if(SD_DBG, "CSD struct unsupported\r\n");
            return 0;
    };
    return blocks;
}

// SPI function to wait till chip is ready and sends start token
bool TestSDBlockDevice::_wait_token(uint8_t token)
{
    _spi_timer.reset();
    _spi_timer.start();

    do {
        if (token == _spi.write(SPI_FILL_CHAR)) {
            _spi_timer.stop();
            return true;
        }
    } while (_spi_timer.elapsed_time() < 300ms);       // Wait for 300 msec for start token
    _spi_timer.stop();
    debug_if(SD_DBG, "_wait_token: timeout\n");
    return false;
}

// SPI function to wait till chip is ready
// The host controller should wait for end of the process until DO goes high (a 0xFF is received).
bool TestSDBlockDevice::_wait_ready(std::chrono::duration<uint32_t, std::milli> timeout)
{
    uint8_t response;
    _spi_timer.reset();
    _spi_timer.start();
    do {
        response = _spi.write(SPI_FILL_CHAR);
        if (response == 0xFF) {
            _spi_timer.stop();
            return true;
        }
    } while (_spi_timer.elapsed_time() < timeout);
    _spi_timer.stop();
    return false;
}

// SPI function to wait for count
void TestSDBlockDevice::_spi_wait(uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i) {
        _spi.write(SPI_FILL_CHAR);
    }
}

void TestSDBlockDevice::_spi_init()
{
    _cs = 1;
    // Set to SCK for initialization, and clock card with _cs = 1
    _spi.frequency(MBED_CONF_SD_INIT_FREQUENCY);
    _spi.format(8, 0);
    _spi.set_default_write_value(SPI_FILL_CHAR);
    // Initial 74 cycles required for few cards, before selecting SPI mode
    for (uint8_t j = 0; j < 10; ++j) {
        _spi.write(SPI_FILL_CHAR);
    }
    _cs = 1;
    _spi.write(SPI_FILL_CHAR);
}

void TestSDBlockDevice::_preclock_then_select()
{
    _spi.write(0xFF);
    _cs = 0;
    _spi.write(0xFF);
}

void TestSDBlockDevice::_postclock_then_deselect()
{
    _spi.write(0xFF);
    _cs = 1;
    _spi.write(0xFF);
}

uint8_t TestSDBlockDevice::_SDSPI_goIdle(){
    _spi.write(SPI_FILL_CHAR);
    _cs = 0;
    _spi.write(SPI_FILL_CHAR);

    uint8_t res1 = _cmd_spi(CMD0, CMD0_ARG);

    _spi.write(SPI_FILL_CHAR);
    _cs = 1;
    _spi.write(SPI_FILL_CHAR);

    return res1;
}

/*
void TestSDBlockDevice::_SD_sendIfCond(uint8_t *res)
{
    // assert chip select
    _spi.write(0xFF);
    _cs = 0;
    _spi.write(0xFF);

    // send CMD8
    SD_command(CMD8, CMD8_ARG);

    // read response
    SD_readRes3_7(res);

    // deassert chip select
    _spi.write(0xFF);
    _cs = 1;
    _spi.write(0xFF);
}
*/

uint8_t TestSDBlockDevice::_SD_sendApp()
{
    // assert chip select
    _spi.write(0xFF);
    _cs = 0;
    _spi.write(0xFF);

    // send CMD0
    uint8_t res1 = _cmd_spi(CMD55, CMD55_ARG);

    // deassert chip select
    _spi.write(0xFF);
    _cs = 1;
    _spi.write(0xFF);

    return res1;
}

uint8_t TestSDBlockDevice::_SD_sendOpCond()
{
    // assert chip select
    _spi.write(0xFF);
    _cs = 0;
    _spi.write(0xFF);

    // send CMD0
    uint8_t res1 = _cmd_spi(ACMD41, ACMD41_ARG);

    // deassert chip select
    _spi.write(0xFF);
    _cs = 1;
    _spi.write(0xFF);

    return res1;
}

#endif  /* DEVICE_SPI */
