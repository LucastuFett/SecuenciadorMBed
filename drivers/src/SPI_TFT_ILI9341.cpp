/* mbed library for 240*320 pixel display TFT based on ILI9341 LCD Controller
 * Copyright (c) 2013 Peter Drescher - DC2PD
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// 12.06.13 fork from SPI_TFT code because controller is different ...
// 14.07.13 Test with real display and bugfix
// 18.10.13 Better Circle function from Michael Ammann
// 22.10.13 Fixes for Kinetis Board - 8 bit spi

#include "SPI_TFT_ILI9341.h"

#include "mbed.h"

#define BPP 16  // Bits per pixel

// ILI9341 initialization commands (Source: https://github.com/adafruit/Adafruit_ILI9341/blob/master/Adafruit_ILI9341.cpp)
static const uint8_t initCommands[] =
    {
        0xEF, 3, 0x03, 0x80, 0x02,
        0xCF, 3, 0x00, 0xC1, 0x30,
        0xED, 4, 0x64, 0x03, 0x12, 0x81,
        0xE8, 3, 0x85, 0x00, 0x78,
        0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
        0xF7, 1, 0x20,
        0xEA, 2, 0x00, 0x00,
        ILI9341_PWCTR1, 1, 0x23,
        ILI9341_PWCTR2, 1, 0x10,
        ILI9341_VMCTR1, 2, 0x3E, 0x28,
        ILI9341_VMCTR2, 1, 0x86,
        ILI9341_MADCTL, 1, (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR),
        ILI9341_VSCRSADD, 1, 0x00,
        ILI9341_PIXFMT, 1, 0x55,
        ILI9341_FRMCTR1, 2, 0x00, 0x18,
        ILI9341_DFUNCTR, 3, 0x08, 0x82, 0x27,
        0xF2, 1, 0x00,
        ILI9341_GAMMASET, 1, 0x01,
        ILI9341_GMCTRP1, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
        ILI9341_GMCTRN1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
        ILI9341_SLPOUT, 0x00,
        ILI9341_DISPON, 0x00,
        0x00};

// extern Serial pc;
// extern DigitalOut xx;     // debug !!

SPI_TFT_ILI9341::SPI_TFT_ILI9341(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset, PinName dc, const char *name)
    : GraphicsDisplay(name), _spi(mosi, miso, sclk), _cs(cs), _dc(dc) {
    orientation = 0;
    char_x = 0;
    _reset = reset;
    tft_reset();
}

int SPI_TFT_ILI9341::width() {
    if (orientation == 0 || orientation == 2)
        return 240;
    else
        return 320;
}

int SPI_TFT_ILI9341::height() {
    if (orientation == 0 || orientation == 2)
        return 320;
    else
        return 240;
}

void SPI_TFT_ILI9341::set_orientation(unsigned int o) {
    orientation = o;
    wr_cmd(0x36);  // MEMORY_ACCESS_CONTROL
    switch (orientation) {
        case 0:
            _spi.write(0x48);
            break;
        case 1:
            _spi.write(0x28);
            break;
        case 2:
            _spi.write(0x88);
            break;
        case 3:
            _spi.write(0xE8);
            break;
    }
    _cs = 1;
    WindowMax();
}

// write command to tft register

void SPI_TFT_ILI9341::wr_cmd(unsigned char cmd) {
    _dc = 0;
    _cs = 0;
    _spi.write(cmd);  // mbed lib
    _dc = 1;
}

void SPI_TFT_ILI9341::wr_dat(unsigned char dat) {
    _spi.write(dat);  // mbed lib
}

// the ILI9341 can read - has to be implemented later
// A read will return 0 at the moment

// unsigned short SPI_TFT_ILI9341::rd_dat (void)
//{
//     unsigned short val = 0;

// val = _spi.write(0x73ff);                /* Dummy read 1           */
// val   = _spi.write(0x0000);              /* Read D8..D15           */
//    return (val);
//}

// Init code based on MI0283QT datasheet

void SPI_TFT_ILI9341::tft_reset() {
    _spi.format(8, 3);
    _spi.frequency(40000000);
    _cs = 1;
    _dc = 1;

    if (_reset != NC) {
        DigitalOut rst(_reset);
        rst = 0;  // display reset
        wait_us(50);
        rst = 1;  // end hardware reset
    }

    // Execute initialization commands for IL9341 chip
    uint8_t index = 0x00;
    uint8_t cmd = 0x00;
    uint8_t numArgs = 0x00;
    while ((cmd = initCommands[index++]) > 0x00) {
        numArgs = initCommands[index++];

        wr_cmd(cmd);

        if (numArgs == 0x00) {
            ThisThread::sleep_for(chrono::milliseconds(150));
        } else {
            uint8_t oldIndex = index;
            for (; index < oldIndex + numArgs; index++) {
                _spi.write(initCommands[index]);
            }
        }

        _cs = 1;
    }
}

void SPI_TFT_ILI9341::pixel(int x, int y, int color) {
    window(x, y, 1, 1);        // Set window to single pixel
    wr_cmd(0x2C);              // send pixel
    _spi.write(color >> 8);    // Write color data
    _spi.write(color & 0xff);  // Write color data
    _cs = 1;
}

void SPI_TFT_ILI9341::window(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
    wr_cmd(0x2A);
    _spi.write(x >> 8);
    _spi.write(x);
    _spi.write((x + w - 1) >> 8);
    _spi.write(x + w - 1);

    _cs = 1;
    wr_cmd(0x2B);
    _spi.write(y >> 8);
    _spi.write(y);
    _spi.write((y + h - 1) >> 8);
    _spi.write(y + h - 1);
    _cs = 1;

    // wr_cmd(ILI9341_RAMWR);  // Write to RAM
}

void SPI_TFT_ILI9341::WindowMax(void) {
    window(0, 0, width(), height());
}

void SPI_TFT_ILI9341::cls(void) {
    unsigned int pixel = (width() * height());
    WindowMax();
    wr_cmd(0x2C);  // send pixel
    unsigned int i;
    for (i = 0; i < pixel; i++) {
        _spi.write(_background >> 8);
        _spi.write(_background & 0xff);
    }
    _cs = 1;
}

void SPI_TFT_ILI9341::circle(int x0, int y0, int r, int color) {
    int x = -r, y = 0, err = 2 - 2 * r, e2;
    do {
        pixel(x0 - x, y0 + y, color);
        pixel(x0 + x, y0 + y, color);
        pixel(x0 + x, y0 - y, color);
        pixel(x0 - x, y0 - y, color);
        e2 = err;
        if (e2 <= y) {
            err += ++y * 2 + 1;
            if (-x == y && e2 <= x) e2 = 0;
        }
        if (e2 > x) err += ++x * 2 + 1;
    } while (x <= 0);
}

void SPI_TFT_ILI9341::fillcircle(int x0, int y0, int r, int color) {
    int x = -r, y = 0, err = 2 - 2 * r, e2;
    do {
        vline(x0 - x, y0 - y, y0 + y, color);
        vline(x0 + x, y0 - y, y0 + y, color);
        e2 = err;
        if (e2 <= y) {
            err += ++y * 2 + 1;
            if (-x == y && e2 <= x) e2 = 0;
        }
        if (e2 > x) err += ++x * 2 + 1;
    } while (x <= 0);
}

void SPI_TFT_ILI9341::hline(int x0, int x1, int y, int color) {
    int w;
    w = x1 - x0 + 1;
    window(x0, y, w, 1);
    wr_cmd(0x2C);  // send pixel
    int j;
    for (j = 0; j < w; j++) {
        _spi.write(color >> 8);
        _spi.write(color & 0xff);
    }
    _cs = 1;
    WindowMax();
    return;
}

void SPI_TFT_ILI9341::vline(int x, int y0, int y1, int color) {
    int h;
    h = y1 - y0 + 1;
    window(x, y0, 1, h);
    wr_cmd(0x2C);                     // send pixel
#if defined TARGET_RASPBERRY_PI_PICO  // 8 Bit SPI
    for (int y = 0; y < h; y++) {
        _spi.write(color >> 8);
        _spi.write(color & 0xff);
    }
#else
    _spi.format(16, 3);  // switch to 16 bit Mode 3
    for (int y = 0; y < h; y++) {
        _spi.write(color);
    }
    _spi.format(8, 3);
#endif
    _cs = 1;
    WindowMax();
    return;
}

void SPI_TFT_ILI9341::line(int x0, int y0, int x1, int y1, int color) {
    // WindowMax();
    int dx = 0, dy = 0;
    int dx_sym = 0, dy_sym = 0;
    int dx_x2 = 0, dy_x2 = 0;
    int di = 0;

    dx = x1 - x0;
    dy = y1 - y0;

    if (dx == 0) { /* vertical line */
        if (y1 > y0)
            vline(x0, y0, y1, color);
        else
            vline(x0, y1, y0, color);
        return;
    }

    if (dx > 0) {
        dx_sym = 1;
    } else {
        dx_sym = -1;
    }
    if (dy == 0) { /* horizontal line */
        if (x1 > x0)
            hline(x0, x1, y0, color);
        else
            hline(x1, x0, y0, color);
        return;
    }

    if (dy > 0) {
        dy_sym = 1;
    } else {
        dy_sym = -1;
    }

    dx = dx_sym * dx;
    dy = dy_sym * dy;

    dx_x2 = dx * 2;
    dy_x2 = dy * 2;

    if (dx >= dy) {
        di = dy_x2 - dx;
        while (x0 != x1) {
            pixel(x0, y0, color);
            x0 += dx_sym;
            if (di < 0) {
                di += dy_x2;
            } else {
                di += dy_x2 - dx_x2;
                y0 += dy_sym;
            }
        }
        pixel(x0, y0, color);
    } else {
        di = dx_x2 - dy;
        while (y0 != y1) {
            pixel(x0, y0, color);
            y0 += dy_sym;
            if (di < 0) {
                di += dx_x2;
            } else {
                di += dx_x2 - dy_x2;
                x0 += dx_sym;
            }
        }
        pixel(x0, y0, color);
    }
    return;
}

void SPI_TFT_ILI9341::rect(int x0, int y0, int x1, int y1, int color) {
    if (x1 > x0)
        hline(x0, x1, y0, color);
    else
        hline(x1, x0, y0, color);

    if (y1 > y0)
        vline(x0, y0, y1, color);
    else
        vline(x0, y1, y0, color);

    if (x1 > x0)
        hline(x0, x1, y1, color);
    else
        hline(x1, x0, y1, color);

    if (y1 > y0)
        vline(x1, y0, y1, color);
    else
        vline(x1, y1, y0, color);

    return;
}

void SPI_TFT_ILI9341::fillrect(int x0, int y0, int x1, int y1, int color) {
    int h = y1 - y0 + 1;
    int w = x1 - x0 + 1;
    int pixel = h * w;
    window(x0, y0, w, h);
    wr_cmd(0x2C);                     // send pixel
#if defined TARGET_RASPBERRY_PI_PICO  // 8 Bit SPI
    for (int p = 0; p < pixel; p++) {
        _spi.write(color >> 8);
        _spi.write(color & 0xff);
    }
#else
    _spi.format(16, 3);  // switch to 16 bit Mode 3
    for (int p = 0; p < pixel; p++) {
        _spi.write(color);
    }
    _spi.format(8, 3);
#endif
    _cs = 1;
    WindowMax();
    return;
}

void SPI_TFT_ILI9341::locate(int x, int y) {
    char_x = x;
    char_y = y;
}

int SPI_TFT_ILI9341::columns() {
    return width() / font[1];
}

int SPI_TFT_ILI9341::rows() {
    return height() / font[2];
}

int SPI_TFT_ILI9341::_putc(int value) {
    if (value == '\n') {  // new line
        char_x = 0;
        char_y = char_y + font[2];
        if (char_y >= height() - font[2]) {
            char_y = 0;
        }
    } else {
        character(char_x, char_y, value);
    }
    return value;
}

void SPI_TFT_ILI9341::character(int x, int y, int c) {
    unsigned int hor, vert, offset, bpl, j, i, b;
    unsigned char *zeichen;
    unsigned char z, w;

    if ((c < 31) || (c > 127)) return;  // test char range

    // read font parameter from start of array
    offset = font[0];  // bytes / char
    hor = font[1];     // get hor size of font
    vert = font[2];    // get vert size of font
    bpl = font[3];     // bytes per line

    if (char_x + hor > width()) {
        char_x = 0;
        char_y = char_y + vert;
        if (char_y >= height() - font[2]) {
            char_y = 0;
        }
    }
    window(char_x, char_y, hor, vert);  // char box
    wr_cmd(0x2C);                       // send pixel
#ifndef TARGET_RASPBERRY_PI_PICO        // 16 Bit SPI
    _spi.format(16, 3);
#endif                                         // switch to 16 bit Mode 3
    zeichen = &font[((c - 32) * offset) + 4];  // start of char bitmap
    w = zeichen[0];                            // width of actual char
    for (j = 0; j < vert; j++) {               //  vert line
        for (i = 0; i < hor; i++) {            //  horz line
            z = zeichen[bpl * i + ((j & 0xF8) >> 3) + 1];
            b = 1 << (j & 0x07);
            if ((z & b) == 0x00) {
#ifndef TARGET_RASPBERRY_PI_PICO  // 16 Bit SPI
                _spi.write(_background);
#else
                _spi.write(_background >> 8);
                _spi.write(_background & 0xff);
#endif
            } else {
#ifndef TARGET_RASPBERRY_PI_PICO  // 16 Bit SPI
                _spi.write(_foreground);
#else
                _spi.write(_foreground >> 8);
                _spi.write(_foreground & 0xff);
#endif
            }
        }
    }
    _cs = 1;
#ifndef TARGET_RASPBERRY_PI_PICO  // 16 Bit SPI
    _spi.format(8, 3);
#endif
    WindowMax();
    if ((w + 2) < hor) {  // x offset to next char
        char_x += w + 2;
    } else
        char_x += hor;
}

void SPI_TFT_ILI9341::set_font(unsigned char *f) {
    font = f;
}

void SPI_TFT_ILI9341::Bitmap(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char *bitmap) {
    unsigned int j;
    int padd;
    unsigned short *bitmap_ptr = (unsigned short *)bitmap;
#if defined TARGET_RASPBERRY_PI_PICO  // 8 Bit SPI
    unsigned short pix_temp;
#endif

    unsigned int i;

    // the lines are padded to multiple of 4 bytes in a bitmap
    padd = -1;
    do {
        padd++;
    } while (2 * (w + padd) % 4 != 0);
    window(x, y, w, h);
    bitmap_ptr += ((h - 1) * (w + padd));
    wr_cmd(0x2C);                 // send pixel
#ifndef TARGET_RASPBERRY_PI_PICO  // 16 Bit SPI
    _spi.format(16, 3);
#endif                                // switch to 16 bit Mode 3
    for (j = 0; j < h; j++) {         // Lines
        for (i = 0; i < w; i++) {     // one line
#if defined TARGET_RASPBERRY_PI_PICO  // 8 Bit SPI
            pix_temp = *bitmap_ptr;
            _spi.write(pix_temp >> 8);
            _spi.write(pix_temp);
            bitmap_ptr++;
#else
            _spi.write(*bitmap_ptr);  // one line
            bitmap_ptr++;
#endif
        }
        bitmap_ptr -= 2 * w;
        bitmap_ptr -= padd;
    }
    _cs = 1;
#ifndef TARGET_RASPBERRY_PI_PICO  // 16 Bit SPI
    _spi.format(8, 3);
#endif
    WindowMax();
}

int SPI_TFT_ILI9341::puts(const char *string) {
    int i = 0;
    while (string[i] != '\0') {
        _putc(string[i++]);
    }
    return i;
}

// local filesystem is not implemented in kinetis board
#if DEVICE_LOCALFILESYSTEM

int SPI_TFT_ILI9341::BMP_16(unsigned int x, unsigned int y, const char *Name_BMP) {
#define OffsetPixelWidth 18
#define OffsetPixelHeigh 22
#define OffsetFileSize 34
#define OffsetPixData 10
#define OffsetBPP 28

    char filename[50];
    unsigned char BMP_Header[54];
    unsigned short BPP_t;
    unsigned int PixelWidth, PixelHeigh, start_data;
    unsigned int i, off;
    int padd, j;
    unsigned short *line;

    // get the filename
    LocalFileSystem local("local");
    sprintf(&filename[0], "/local/");
    i = 7;
    while (*Name_BMP != '\0') {
        filename[i++] = *Name_BMP++;
    }

    fprintf(stderr, "filename : %s \n\r", filename);

    FILE *Image = fopen((const char *)&filename[0], "rb");  // open the bmp file
    if (!Image) {
        return (0);  // error file not found !
    }

    fread(&BMP_Header[0], 1, 54, Image);  // get the BMP Header

    if (BMP_Header[0] != 0x42 || BMP_Header[1] != 0x4D) {  // check magic byte
        fclose(Image);
        return (-1);  // error no BMP file
    }

    BPP_t = BMP_Header[OffsetBPP] + (BMP_Header[OffsetBPP + 1] << 8);
    if (BPP_t != 0x0010) {
        fclose(Image);
        return (-2);  // error no 16 bit BMP
    }

    PixelHeigh = BMP_Header[OffsetPixelHeigh] + (BMP_Header[OffsetPixelHeigh + 1] << 8) + (BMP_Header[OffsetPixelHeigh + 2] << 16) + (BMP_Header[OffsetPixelHeigh + 3] << 24);
    PixelWidth = BMP_Header[OffsetPixelWidth] + (BMP_Header[OffsetPixelWidth + 1] << 8) + (BMP_Header[OffsetPixelWidth + 2] << 16) + (BMP_Header[OffsetPixelWidth + 3] << 24);
    if (PixelHeigh > height() + y || PixelWidth > width() + x) {
        fclose(Image);
        return (-3);  // to big
    }

    start_data = BMP_Header[OffsetPixData] + (BMP_Header[OffsetPixData + 1] << 8) + (BMP_Header[OffsetPixData + 2] << 16) + (BMP_Header[OffsetPixData + 3] << 24);

    line = (unsigned short *)malloc(2 * PixelWidth);  // we need a buffer for a line
    if (line == NULL) {
        return (-4);  // error no memory
    }

    // the bmp lines are padded to multiple of 4 bytes
    padd = -1;
    do {
        padd++;
    } while ((PixelWidth * 2 + padd) % 4 != 0);

    // fseek(Image, 70 ,SEEK_SET);
    window(x, y, PixelWidth, PixelHeigh);
    wr_cmd(0x2C);                                        // send pixel
    _spi.format(16, 3);                                  // switch to 16 bit Mode 3
    for (j = PixelHeigh - 1; j >= 0; j--) {              // Lines bottom up
        off = j * (PixelWidth * 2 + padd) + start_data;  // start of line
        fseek(Image, off, SEEK_SET);
        fread(line, 1, PixelWidth * 2, Image);  // read a line - slow !
        for (i = 0; i < PixelWidth; i++) {      // copy pixel data to TFT
            _spi.write(line[i]);                // one 16 bit pixel
        }
    }
    _cs = 1;
    _spi.format(8, 3);
    free(line);
    fclose(Image);
    WindowMax();
    return (1);
}
#endif