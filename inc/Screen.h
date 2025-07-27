#pragma once
#include "definitions.h"
#include "SPI_TFT_ILI9341.h"

// Constants

const string labels[10][8] = {{"Program", "Play", "Launch", "DAW","","USBMIDI","USBMSD",""},
				{"Note", "Play/Pause", "Stop", "Hold","Memory","Channel","Tempo","Scale"},
				{"Accept", "Octave -", "Octave +", "Cancel","","","",""},
				{"Save","Shift","Backspace","Load","","Special","Space",""},
				{"Accept","Bank -","Bank +","Cancel","","Rename","Delete",""},
				{"Save","Shift","Backspace","Cancel","","Special","Space",""},
				{"Accept","","","Cancel","","","",""},
				{"Accept","Internal","External","Cancel","","Half","Double",""},
				{"Accept","Mode -","Mode +","Cancel","","","",""},
				{"Play/Pause","Bank -","Bank +","Stop","","","",""}};

const string titles[] = {"Main - Config", 
				"Programming", 
				"Edit Note",
				"Memory",
				"Save/Load",
				"Rename",
				"Edit Channel",
				"Edit Tempo",
				"Edit Scale",
				"Play"};
				
const char letters[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

const char shLetters[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

const char special[] = {'0','1','2','3','4','5','6','7','8','9','\'','.',',','/',':',';','-','_','?','!','"'};

const string tones[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

class Screen : public SPI_TFT_ILI9341 {
    int8_t _typePointer;
    int8_t _letterPointer;
    int8_t _specialPointer;
    int8_t _edit;
    bool _curPointer;
    bool _upper;
    int8_t _selectedFile;
    int8_t _currentFile;
    int8_t _curOctave;
    vector <uint16_t> _possible[2];
    char _typing[18];
    string _files[12];
    uint16_t _bkgColors[12];
    
    enum state _lastState = MAIN;
    bool _menu = false;
    bool _piano = false;
    bool _typeBox = false;
    bool _memBanks = false;
    bool _lastHalf = false;
    uint8_t _lastBank = 1;
    uint8_t _lastHold = 0;
    int8_t _lastVel = 127;
    int8_t _lastSelectedFile = 0;
    int8_t _lastCurrentFile = -1;
    string _lastFiles[12];
    uint16_t _lastBkgColors[12];

    // Cover Title
    void coverTitle();

    // Updates Screen Text, Leaving Graphics
    void updateText();
    
    // Shows or Hides Menus
    void showMenu(bool show);

    /* Updates Menu Text
        0 = All Menus
        1 = Channel
        2 = Tempo
        3 = Scale
        4 = Velocity
        5 = Half
    */
    void updateMenuText(uint8_t menu);

    // Shows or Hides Piano Roll
    void showPiano(bool show);

    // Shows or Hides Typing Box
    void showTyping(bool show);

    // Shows or Hides Memory Banks
    void showBanks(bool show);

    // Get Possible Notes for the current scale
    void getPossible();

    // Paint Scales and Selected Notes
    void paintScales();

    // Paint Grid
    void paintGrid();

    // Update Hold
    void updateHold();
public:
    // Constructor
    Screen(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset, PinName dc, const char* name ="TFT");

    // Set the Default Text and Graphics
    void init();

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

    // Set Edit Variable
    void setEdit(int8_t edit);

    // Get Current Pointer
    bool getCurPointer();

    // Set Current Pointer
    void setCurPointer(bool curPointer);

    // Get Upper
    bool getUpper();

    // Set Upper
    void setUpper(bool upper);

    // Update Typing Text at Pointer
    void setTyping(char type);

    // Error Message
    void showError(string error);
};