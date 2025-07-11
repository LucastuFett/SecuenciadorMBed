#include "mbed.h"
#include <cstdint>

// ILI9341 SPI Commands
#define ILI9341_NOP         0x00  // No-op register
#define ILI9341_SWRESET     0x01  // Software reset register
#define ILI9341_RDDID       0x04  // Read display identification information
#define ILI9341_RDDST       0x09  // Read Display Status

#define ILI9341_SLPIN       0x10  // Enter Sleep Mode
#define ILI9341_SLPOUT      0x11  // Sleep Out
#define ILI9341_PTLON       0x12  // Partial Mode ON
#define ILI9341_NORON       0x13  // Normal Display Mode ON

#define ILI9341_RDMODE      0x0A  // Read Display Power Mode
#define ILI9341_RDMADCTL    0x0B  // Read Display MADCTL
#define ILI9341_RDPIXFMT    0x0C  // Read Display Pixel Format
#define ILI9341_RDIMGFMT    0x0D  // Read Display Image Format
#define ILI9341_RDSELFDIAG  0x0F  // Read Display Self-Diagnostic Result

#define ILI9341_INVOFF      0x20  // Display Inversion OFF
#define ILI9341_INVON       0x21  // Display Inversion ON
#define ILI9341_GAMMASET    0x26  // Gamma Set
#define ILI9341_DISPOFF     0x28  // Display OFF
#define ILI9341_DISPON      0x29  // Display ON

#define ILI9341_CASET       0x2A  // Column Address Set
#define ILI9341_PASET       0x2B  // Page Address Set
#define ILI9341_RAMWR       0x2C  // Memory Write
#define ILI9341_RAMRD       0x2E  // Memory Read

#define ILI9341_PTLAR       0x30  // Partial Area
#define ILI9341_VSCRDEF     0x33  // Vertical Scrolling Definition
#define ILI9341_MADCTL      0x36  // Memory Access Control
#define ILI9341_VSCRSADD    0x37  // Vertical Scrolling Start Address
#define ILI9341_PIXFMT      0x3A  // COLMOD: Pixel Format Set

#define ILI9341_FRMCTR1     0xB1  // Frame Rate Control (In Normal Mode/Full Colors)
#define ILI9341_FRMCTR2     0xB2  // Frame Rate Control (In Idle Mode/8 colors)
#define ILI9341_FRMCTR3     0xB3  // Frame Rate control (In Partial Mode/Full Colors)
#define ILI9341_INVCTR      0xB4  // Display Inversion Control
#define ILI9341_DFUNCTR     0xB6  // Display Function Control

#define ILI9341_PWCTR1      0xC0  // Power Control 1
#define ILI9341_PWCTR2      0xC1  // Power Control 2
#define ILI9341_PWCTR3      0xC2  // Power Control 3
#define ILI9341_PWCTR4      0xC3  // Power Control 4
#define ILI9341_PWCTR5      0xC4  // Power Control 5
#define ILI9341_VMCTR1      0xC5  // VCOM Control 1
#define ILI9341_VMCTR2      0xC7  // VCOM Control 2

#define ILI9341_RDID1       0xDA  // Read ID 1
#define ILI9341_RDID2       0xDB  // Read ID 2
#define ILI9341_RDID3       0xDC  // Read ID 3
#define ILI9341_RDID4       0xDD  // Read ID 4

#define ILI9341_GMCTRP1     0xE0  // Positive Gamma Correction
#define ILI9341_GMCTRN1     0xE1  // Negative Gamma Correction

#define ILI9341_MADCTL_MY   0x80
#define ILI9341_MADCTL_MX   0x40
#define ILI9341_MADCTL_MV   0x20
#define ILI9341_MADCTL_ML   0x10
#define ILI9341_MADCTL_RGB  0x00
#define ILI9341_MADCTL_BGR  0x08
#define ILI9341_MADCTL_MH   0x04

// RGB Color definitions
#define BLACK               0x0000
#define NAVY                0x000F
#define DARK_GREEN          0x03E0
#define DARK_CYAN           0x03EF
#define MAROON              0x7800
#define PURPLE              0x780F
#define OLIVE               0x7BE0
#define LIGHT_GRAY          0xC618
#define GRAY                0xA514
#define DARK_GRAY           0x7BEF
#define BLUE                0x001F
#define GREEN               0x07E0
#define CYAN                0x07FF
#define RED                 0xF800
#define MAGENTA             0xF81F
#define YELLOW              0xFFE0
#define WHITE               0xFFFF
#define ORANGE              0xFD20
#define GREEN_YELLOW        0xAFE5

#define ILI9341_TFTWIDTH    240
#define ILI9341_TFTHEIGHT   320

#ifndef ILI9341_H
#define ILI9341_H
class ILI9341
{
  public:
    ILI9341(PinName mosi, PinName miso, PinName clk, PinName cs, PinName rst, PinName dc);
    void initialize(void);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
    void drawHLine(uint16_t x, uint16_t	y, uint16_t w, uint16_t color);
    void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    void drawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void fillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void drawCircle(uint16_t xc, uint16_t yc, uint16_t r, uint16_t color);
    void fillCircle(uint16_t xc, uint16_t yc, uint16_t r, uint16_t color);
    void drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
    void fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
    void drawChar(uint16_t x, uint16_t y, unsigned char c, uint16_t size, uint16_t foreColor, uint16_t backColor);
    void drawString(uint16_t x, uint16_t y, const char* str, uint16_t strSize, uint16_t charSize, uint16_t foreColor, uint16_t backColor);
    void fillBackground(uint16_t color);
    void setRotation(uint8_t rot);

  private:
    SPI spi;
    DigitalOut chipSelect;  // Chip Select Pin
    DigitalOut reset;       // Reset Pin
    DigitalOut dataCommand; // Data/Command Select Pin
    uint8_t orientation;
    uint16_t width;
    uint16_t height;

    void writeCommand(uint8_t cmd);
    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void drawCircleHelper(uint16_t xc, uint16_t yc, uint16_t r, uint8_t corners, uint16_t color);
    void fillCircleHelper(uint16_t xc, uint16_t yc, uint16_t r, uint8_t corners, uint16_t color);
    int8_t signumFunc(int16_t x);
};
#endif