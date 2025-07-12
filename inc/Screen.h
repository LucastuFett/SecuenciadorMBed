#pragma once
#include "definitions.h"
#include "SPI_TFT_ILI9341.h"


class Screen : public SPI_TFT_ILI9341 {
    uint8_t _typePointer;
    uint8_t _letterPointer;
    uint8_t _specialPointer;
    uint8_t _edit;
    bool _curPointer;
    bool _upper;
    uint8_t _selectedFile;
    uint8_t _currentFile;

    // Updates Screen Text, Leaving Graphics
    void updateText();
    
public:
    // Constructor
    Screen(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset, PinName dc, const char* name ="TFT");

    // Set the Default Text and Graphics
    void start();

    // Left Action on Screen
    void left();

    // Right Action on Screen
    void right();

    // Update Screen Content
    void updateScreen();

    // Update Text Writing
    void updateMemoryText();

    // Select a letter when Writing
    void selectLetter();

    // Save Filename when Writing
    string saveFilename();
    
    // Load Filenames
    void updateBanks();

    // Get filename from selected file
    string getFilename();
};