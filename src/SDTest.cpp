/*
 * Copyright (c) 2006-2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include "USBMSD.h"
#include "FATFileSystem.h"
#include "SDBlockDevice.h"


#define CMD0        0
#define CMD0_ARG    0x00000000
#define CMD0_CRC    0x94

#define CMD8        8
#define CMD8_ARG    0x0000001AA
#define CMD8_CRC    0x86 //(1000011 << 1)

#define CMD58       58
#define CMD58_ARG   0x00000000
#define CMD58_CRC   0x00

#define CMD55       55
#define CMD55_ARG   0x00000000
#define CMD55_CRC   0x00

#define ACMD41      41
#define ACMD41_ARG  0x40000000
#define ACMD41_CRC  0x00

#define CMD17                   17
#define CMD17_CRC               0x00
#define SD_MAX_READ_ATTEMPTS    1251        // (100 ms * 100 KHz) / 8 bits

#define CMD24               24
#define CMD24_CRC           0x00
#define SD_MAX_WRITE_ATTEMPTS 3126        // (250 ms * 100 KHz) / 8 bits

#define CMD_VER(X)          ((X >> 4) & 0xF0)
#define VOL_ACC(X)          (X & 0x1F)

#define VOLTAGE_ACC_27_33   0b00000001
#define VOLTAGE_ACC_LOW     0b00000010
#define VOLTAGE_ACC_RES1    0b00000100
#define VOLTAGE_ACC_RES2    0b00001000

#define PARAM_ERROR(X)      X & 0b01000000
#define ADDR_ERROR(X)       X & 0b00100000
#define ERASE_SEQ_ERROR(X)  X & 0b00010000
#define CRC_ERROR(X)        X & 0b00001000
#define ILLEGAL_CMD(X)      X & 0b00000100
#define ERASE_RESET(X)      X & 0b00000010
#define IN_IDLE(X)          X & 0b00000001

#define POWER_UP_STATUS(X)  X & 0x40
#define CCS_VAL(X)          X & 0x40
#define VDD_2728(X)         X & 0b10000000
#define VDD_2829(X)         X & 0b00000001
#define VDD_2930(X)         X & 0b00000010
#define VDD_3031(X)         X & 0b00000100
#define VDD_3132(X)         X & 0b00001000
#define VDD_3233(X)         X & 0b00010000
#define VDD_3334(X)         X & 0b00100000
#define VDD_3435(X)         X & 0b01000000
#define VDD_3536(X)         X & 0b10000000

#define SD_SUCCESS  0
#define SD_ERROR    1
//#define SD_READY    0
#define SD_R1_NO_ERROR(X)   X < 0x02

#define SD_START_TOKEN          0xFE
#define SD_ERROR_TOKEN          0x00

#define SD_DATA_ACCEPTED        0x05
#define SD_DATA_REJECTED_CRC    0x0B
#define SD_DATA_REJECTED_WRITE  0x0D

#define SD_BLOCK_LEN            512

#define SPITEST 0
#define USBMON 0

FATFileSystem fs("fs");

#if SPITEST

SPI _spi(p3, p0, p2, NC);
DigitalOut cs(p1);
uint8_t i = 0;
uint8_t res0;
uint8_t res55;
uint8_t res41;

void SD_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    // transmit command to sd card
    _spi.write(cmd|0x40);

    // transmit argument
    _spi.write((uint8_t)(arg >> 24));
    _spi.write((uint8_t)(arg >> 16));
    _spi.write((uint8_t)(arg >> 8));
    _spi.write((uint8_t)(arg));

    // transmit crc
    _spi.write(crc|0x01);
}

uint8_t SD_readRes1()
{
    uint8_t res1;

    // keep polling until actual data received
    while((res1 = _spi.write(0xFF)) == 0xFF)
    {
        i++;

        // if no data received for 8 bytes, break
        if(i > 16) break;
    }

    return res1;
}
    
void SD_readRes3_7(uint8_t *res)
{
    // read response 1 in R7
    res[0] = SD_readRes1();

    // if error reading R1, return
    if(res[0] > 1) return;

    // read remaining bytes
    res[1] = _spi.write(0xFF);
    res[2] = _spi.write(0xFF);
    res[3] = _spi.write(0xFF);
    res[4] = _spi.write(0xFF);
}

void SD_sendIfCond(uint8_t *res)
{
    // assert chip select
    _spi.write(0xFF);
    cs = 0;
    _spi.write(0xFF);

    // send CMD8
    SD_command(CMD8, CMD8_ARG, CMD8_CRC);

    // read response
    SD_readRes3_7(res);

    // deassert chip select
    _spi.write(0xFF);
    cs = 1;
    _spi.write(0xFF);
}

void SD_printR1(uint8_t res)
{
    if(res & 0b10000000)
        { printf("\tError: MSB = 1\r\n"); return; }
    if(res == 0)
        { printf("\tCard Ready\r\n"); return; }
    if(PARAM_ERROR(res))
        printf("\tParameter Error\r\n");
    if(ADDR_ERROR(res))
        printf("\tAddress Error\r\n");
    if(ERASE_SEQ_ERROR(res))
        printf("\tErase Sequence Error\r\n");
    if(CRC_ERROR(res))
        printf("\tCRC Error\r\n");
    if(ILLEGAL_CMD(res))
        printf("\tIllegal Command\r\n");
    if(ERASE_RESET(res))
        printf("\tErase Reset Error\r\n");
    if(IN_IDLE(res))
        printf("\tIn Idle State\r\n");
}

void SD_printR7(uint8_t *res)
{
    SD_printR1(res[0]);

    if(res[0] > 1) return;

    printf("\tCommand Version: ");
    printf("0x%02X",CMD_VER(res[1]));
    printf("\r\n");

    printf("\tVoltage Accepted: ");
    if(VOL_ACC(res[3]) == VOLTAGE_ACC_27_33)
        printf("2.7-3.6V\r\n");
    else if(VOL_ACC(res[3]) == VOLTAGE_ACC_LOW)
        printf("LOW VOLTAGE\r\n");
    else if(VOL_ACC(res[3]) == VOLTAGE_ACC_RES1)
        printf("RESERVED\r\n");
    else if(VOL_ACC(res[3]) == VOLTAGE_ACC_RES2)
        printf("RESERVED\r\n");
    else
        printf("NOT DEFINED\r\n");

    printf("\tEcho: ");
    printf("0x%02X",res[4]);
    printf("\r\n");
}

void SDSPI_init(){
    cs = 1;
    // Set to SCK for initialization, and clock card with cs = 1
    _spi.frequency(MBED_CONF_SD_INIT_FREQUENCY);
    printf("SPI frequency: %d\n", MBED_CONF_SD_INIT_FREQUENCY);
    _spi.format(8, 0);
}

void SDSPI_powerUp(){
    _spi.set_default_write_value(SPI_FILL_CHAR);
    // Initial 74 cycles required for few cards, before selecting SPI mode
    for (uint8_t j = 0; j < 10; ++j) {
        _spi.write(SPI_FILL_CHAR);
    }
    cs = 1;
    _spi.write(SPI_FILL_CHAR);
}

uint8_t SDSPI_goIdle(){
    _spi.write(SPI_FILL_CHAR);
    cs = 0;
    _spi.write(SPI_FILL_CHAR);

    SD_command(CMD0, CMD0_ARG, CMD0_CRC);
    uint8_t res1 = SD_readRes1();

    _spi.write(SPI_FILL_CHAR);
    cs = 1;
    _spi.write(SPI_FILL_CHAR);

    return res1;
}

void SD_readOCR(uint8_t *res)
{
    // assert chip select
    _spi.write(0xFF);
    cs = 0;
    uint8_t tmp = _spi.write(0xFF);

    if(tmp != 0xFF) while(_spi.write(0xFF) != 0xFF) ThisThread::sleep_for(10ms) ;

    // send CMD58
    SD_command(CMD58, CMD58_ARG, CMD58_CRC);

    // read response
    SD_readRes3_7(res);

    // deassert chip select
    _spi.write(0xFF);
    cs = 1;
    _spi.write(0xFF);
}

void SD_printR3(uint8_t *res)
{
    SD_printR1(res[0]);

    if(res[0] > 1) return;

    printf("\tCard Power Up Status: ");
    if(POWER_UP_STATUS(res[1]))
    {
        printf("READY\r\n");
        printf("\tCCS Status: ");
        if(CCS_VAL(res[1])){ printf("1\r\n"); }
        else printf("0\r\n");
    }
    else
    {
        printf("BUSY\r\n");
    }

    printf("\tVDD Window: ");
    if(VDD_2728(res[3])) printf("2.7-2.8, ");
    if(VDD_2829(res[2])) printf("2.8-2.9, ");
    if(VDD_2930(res[2])) printf("2.9-3.0, ");
    if(VDD_3031(res[2])) printf("3.0-3.1, ");
    if(VDD_3132(res[2])) printf("3.1-3.2, ");
    if(VDD_3233(res[2])) printf("3.2-3.3, ");
    if(VDD_3334(res[2])) printf("3.3-3.4, ");
    if(VDD_3435(res[2])) printf("3.4-3.5, ");
    if(VDD_3536(res[2])) printf("3.5-3.6");
    printf("\r\n");
}

uint8_t SD_sendApp()
{
    // assert chip select
    _spi.write(0xFF);
    cs = 0;
    _spi.write(0xFF);

    // send CMD0
    SD_command(CMD55, CMD55_ARG, CMD55_CRC);

    // read response
    uint8_t res1 = SD_readRes1();

    // deassert chip select
    _spi.write(0xFF);
    cs = 1;
    _spi.write(0xFF);

    return res1;
}

uint8_t SD_sendOpCond()
{
    // assert chip select
    _spi.write(0xFF);
    cs = 0;
    _spi.write(0xFF);

    // send CMD0
    SD_command(ACMD41, ACMD41_ARG, ACMD41_CRC);

    // read response
    uint8_t res1 = SD_readRes1();

    // deassert chip select
    _spi.write(0xFF);
    cs = 1;
    _spi.write(0xFF);

    return res1;
}

uint8_t SD_init()
{
    uint8_t res[5], cmdAttempts = 0;

    SDSPI_powerUp();

    // command card to idle
    while((res[0] = SDSPI_goIdle()) != 0x01)
    {
        cmdAttempts++;
        printf("Idle Attempts: %i\n",cmdAttempts);
        if(cmdAttempts > 10) return SD_ERROR;
    }

    // send interface conditions
    SD_sendIfCond(res);
    if(res[0] != 0x01)
    {
        return SD_ERROR;
    }
    printf("IfCond\n");
    // check echo pattern
    if(res[4] != 0xAA)
    {
        return SD_ERROR;
    }
    printf("Echo\n");
    // attempt to initialize card
    cmdAttempts = 0;
    do
    {
        if(cmdAttempts > 100) return SD_ERROR;

        // send app cmd
        res[0] = SD_sendApp();
        ThisThread::sleep_for(200ms);
        // if no error in response
        if(res[0] < 2)
        {
            printf("No Error on App\n");
            res[0] = SD_sendOpCond();
            ThisThread::sleep_for(100ms);
        }
        printf("CMD41 Res: %02X\n", res[0]);
        // wait
        ThisThread::sleep_for(100ms);

        cmdAttempts++;
    }
    while(res[0] != SD_READY);
    //ThisThread::sleep_for(10000ms);
    //printf("PreOCR");
    // read OCR
    //SD_readOCR(res);

    // check card is ready
    //if(!(res[1] & 0x80)) return SD_ERROR;

    return SD_SUCCESS;
}

/*******************************************************************************
 Read single 512 byte block
 token = 0xFE - Successful read
 token = 0x0X - Data error
 token = 0xFF - Timeout
*******************************************************************************/
uint8_t SD_readSingleBlock(uint32_t addr, uint8_t *buf, uint8_t *token)
{
    uint8_t res1, read;
    uint16_t readAttempts;

    // set token to none
    *token = 0xFF;

    // assert chip select
    _spi.write(0xFF);
    cs = 0;
    _spi.write(0xFF);

    // send CMD17
    SD_command(CMD17, addr, CMD17_CRC);

    // read R1
    res1 = SD_readRes1();

    // if response received from card
    if(res1 != 0xFF)
    {
        // wait for a response token (timeout = 100ms)
        readAttempts = 0;
        while(++readAttempts != SD_MAX_READ_ATTEMPTS)
            if((read = _spi.write(0xFF)) != 0xFF) break;

        // if response token is 0xFE
        if(read == 0xFE)
        {
            // read 512 byte block
            for(uint16_t i = 0; i < 512; i++) *buf++ = _spi.write(0xFF);

            // read 16-bit CRC
            _spi.write(0xFF);
            _spi.write(0xFF);
        }

        // set token to card response
        *token = read;
    }

    // deassert chip select
    _spi.write(0xFF);
    cs = 1;
    _spi.write(0xFF);

    return res1;
}

uint8_t SD_writeSingleBlock(uint32_t addr, uint8_t *buf, uint8_t *token)
{
    uint16_t readAttempts;
    uint8_t res1, read;

    // set token to none
    *token = 0xFF;

    // assert chip select
    _spi.write(0xFF);
    cs = 0;
    _spi.write(0xFF);

    // send CMD24
    SD_command(CMD24, addr, CMD24_CRC);

    // read response
    res1 = SD_readRes1();

    // if no error
    if(res1 == SD_READY)
    {
        // send start token
        _spi.write(SD_START_TOKEN);

        // write buffer to card
        for(uint16_t i = 0; i < SD_BLOCK_LEN; i++) _spi.write(buf[i]);

        // wait for a response (timeout = 250ms)
        readAttempts = 0;
        while(++readAttempts != SD_MAX_WRITE_ATTEMPTS)
            if((read = _spi.write(0xFF)) != 0xFF) { *token = 0xFF; break; }

        // if data accepted
        if((read & 0x1F) == 0x05)
        {
            // set token to data accepted
            *token = 0x05;

            // wait for write to finish (timeout = 250ms)
            readAttempts = 0;
            while(_spi.write(0xFF) == 0x00)
                if(++readAttempts == SD_MAX_WRITE_ATTEMPTS) { *token = 0x00; break; }
        }
    }

    // deassert chip select
    _spi.write(0xFF);
    cs = 1;
    _spi.write(0xFF);

    return res1;
}

#else
SDBlockDevice bd(p3, p0, p2, p1);
#endif

int main()
{
    #if USBMON
    ThisThread::sleep_for(10000ms);
    printf("Test");
    ThisThread::sleep_for(1000ms);
    #endif

    #if SPITEST
    uint8_t res[5], sdBuf[512], token, reswr;
    //uint8_t res7[5];
    //uint8_t res3[5];
    //uint16_t attempts = 0;
    SDSPI_init();
    
    if(SD_init() != SD_SUCCESS)
    {
        printf("Error initializaing SD CARD\r\n"); while(1);
    }
    else
    {
        printf("SD Card initialized\r\n");
    }

    // read sector 0
    res[0] = SD_readSingleBlock(0x00000100, sdBuf, &token);
    

    /*
    SDSPI_powerUp();
    res0 = SDSPI_goIdle();
    SD_sendIfCond(res7);
    SD_readOCR(res3);
    ThisThread::sleep_for(100ms);
    do{
        res55 = SD_sendApp();
        ThisThread::sleep_for(200ms);
        res41 = SD_sendOpCond();
        ThisThread::sleep_for(100ms);
        attempts++;
        printf("%i, r55=%02X,r41=%02X\n",attempts, res55, res41);
    }while(res41 != 0);
    */

    // print response
    if(SD_R1_NO_ERROR(res[0]) && (token == 0xFE))
    {
        for(uint16_t i = 0; i < 512; i++) printf("0x%02X",sdBuf[i]);
        printf("\r\n");
    }
    else
    {
        printf("Error reading sector\r\n");
    }

    ThisThread::sleep_for(100ms);

    // fill buffer with 0x55
    for(uint16_t i = 0; i < 512; i++) sdBuf[i] = 0x55;

    // write 0x55 to address 0x100 (256)
    reswr = SD_writeSingleBlock(0x00000100, sdBuf, &token);

    SD_printR1(reswr);
    if(token == SD_DATA_ACCEPTED) printf("Data accepted\r\n");
    else if(token == SD_DATA_REJECTED_CRC) printf("Data rejected, CRC error\r\n");
    else if(token == SD_DATA_REJECTED_WRITE) printf("Data rejected, write error\r\n");
    else if(token == SD_ERROR_TOKEN) printf("Error token received\r\n");
    else printf("Unknown token received: 0x%02X\r\n", token);
    while(1){
        /*
        printf("R1:\n");
        SD_printR1(res0);
        printf("R7:\n");
        SD_printR7(res7);
        printf("R3:\n");
        SD_printR3(res3);
        printf("R55:\n");
        SD_printR1(res55);
        printf("R41:\n");
        SD_printR1(res41);
        printf("\nAttempts: %i\n");
        */
        

        
    }
    #else    
    
    ThisThread::sleep_for(10000ms);
    bd.init();

    FATFileSystem::format(&bd);

    int err = fs.mount(&bd);

    if (err) {
        #if USBMON
        printf("%s filesystem mount failed\ntry to reformat device... \r\n", fs.getName());
        #endif
        err = fs.reformat(&bd);
    }

    // If still error, then report failure
    if (err) {
        #if USBMON
        printf("Error: Unable to format/mount the device.\r\n");
        #endif
        while (1);
    }
    
    #if !USBMON
    USBMSD usb(&bd);

    while (true) {
        usb.process();
    }

    #else 
    while(true);
    #endif

    #endif
    return 0;
}