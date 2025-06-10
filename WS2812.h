#pragma once

#include "mbed.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

class WS2812_PIO {
public:
    WS2812_PIO(PinName pin, uint led_count, PIO pio = pio0, uint sm = 0)
        : _pin(pin), _led_count(led_count), _pio(pio), _sm(sm)
    {
        _rgb_data.resize(led_count * 3);
        _pin_num = static_cast<uint>(pin);

        _offset = pio_add_program(_pio, &ws2812_program);

        pio_gpio_init(_pio, _pin_num);
        pio_sm_set_consecutive_pindirs(_pio, _sm, _pin_num, 1, true);

        pio_sm_config c = ws2812_program_get_default_config(_offset);
        sm_config_set_sideset_pins(&c, _pin_num);
        sm_config_set_out_shift(&c, false, true, 24);
        sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
        sm_config_set_clkdiv(&c, 1.0f); // ~800kHz signal

        pio_sm_init(_pio, _sm, _offset, &c);
        pio_sm_set_enabled(_pio, _sm, true);
    }

    void setPixel(uint index, uint8_t r, uint8_t g, uint8_t b) {
        if (index >= _led_count) return;
        int i = index * 3;
        _rgb_data[i + 0] = g;
        _rgb_data[i + 1] = r;
        _rgb_data[i + 2] = b;
    }

    void clear() {
        std::fill(_rgb_data.begin(), _rgb_data.end(), 0);
        show();
    }

    void show() {
        for (uint i = 0; i < _led_count; i++) {
            uint32_t grb = (_rgb_data[i * 3] << 16) | (_rgb_data[i * 3 + 1] << 8) | _rgb_data[i * 3 + 2];
            while (pio_sm_is_tx_fifo_full(_pio, _sm)) {}
            pio_sm_put_blocking(_pio, _sm, grb);
        }
        wait_us(80); // Latch
    }

private:
    PinName _pin;
    uint _pin_num;
    uint _led_count;
    std::vector<uint8_t> _rgb_data;
    PIO _pio;
    uint _sm;
    uint _offset;
};
