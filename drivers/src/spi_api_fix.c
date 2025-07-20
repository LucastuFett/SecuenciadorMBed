#include "spi_api.h"
#include "hardware/spi.h"

void spi_free(spi_t *obj){
    /* Disable the SPI peripheral. */
    spi_deinit(obj->dev);
}