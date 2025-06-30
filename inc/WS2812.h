/*
 * Copyright (c) 2022, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#pragma once

#include "mbed.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/sem.h"
#include "pico/time.h"
#include "pico/util/pheap.h"
#include "ws2812.pio.h"

#define NEOC_NOF_LANES 1
#define NEOC_NOF_COLORS 3

class WS2812_PIO {
    unsigned int _pin_num;
    uint32_t _led_count;
    PIO _pio;
    unsigned int _WS2812_sm;
    unsigned int _offset;
    void dma_init(PIO pio, unsigned int sm);
public:
    /*!
    * \brief Module initialization
    */
    WS2812_PIO(PinName pin, uint32_t led_count);

    /*!
    * \brief Wait until the buffer for the pixel data is available and ready for writing.
    */
    void WS2812_WaitForBufferReady(void);

    /*!
    * \brief Decides if a DMA transfer is still going on.
    * \return true if a DMA transfer is going on, false otherwise.
    */
    bool WS2812_DMATransferIsOngoing(void);

    /*!
    * \brief Transfer the LED data buffer to the hardware
    * \param address Address of data
    * \param nofBytes Number of byes
    * \return 0 for success, non-zero otherwise
    */
    int WS2812_Transfer(uint32_t address, size_t nofBytes);
    
};