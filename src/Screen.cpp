#include "Screen.h"

// Constants

const string labels[10][8] = {{"Program", "Play", "Launch", "DAW","","","",""},
				{"Note", "Play/Pause", "Stop", "Hold","Memory","Channel","Tempo","Scale"},
				{"Accept", "Octave -", "Octave +", "Cancel","","","",""},
				{"Save","Shift","Backspace","Load","","Special","Space",""},
				{"Accept","Bank -","Bank +","Cancel","","Rename","Delete",""},
				{"Save","Shift","Backspace","Cancel","","Special","Space",""},
				{"Accept","","","Cancel","","","",""},
				{"Accept","Internal","External","Cancel","","","",""},
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



// Global variables for MIDI messages and control
extern enum state mainState;
extern uint8_t midiMessages[320][3]; // MIDI messages for each beat
extern uint8_t offMessages[320][3]; // Off messages for each beat
extern uint32_t control; // Control mask for beats
extern uint8_t channels[16]; // Active channels
extern int8_t beat; // Current beat index
extern int8_t note;
extern int8_t octave;
extern DigitalIn toggle32;
extern bool mode32;
extern bool half;
extern int8_t channel;
extern int8_t velocity;
extern uint32_t beatsPerTone[1536]; // Beats per tone structure
extern map<holdKey,uint8_t,CompareHoldKey> holded; // Holds the start and end of held notes
extern uint8_t hold; // Hold state: 0 = No Hold, 1 = Waiting 1st, 2 = Waiting 2nd

Screen::Screen(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset, PinName dc, const char *name)
    : SPI_TFT_ILI9341(mosi, miso, sclk, cs, reset, dc, name) {
		_typePointer = 0;
		_letterPointer = 0;
		_specialPointer = 0;
		_edit = 0;
		_curPointer = false;
		_upper = false;
		_selectedFile = 0;
		_currentFile = 0;

		
		set_orientation(1);
		background(Black);    // set background to Black
		foreground(White);    // set chars to white

		
	}

void Screen::start() {
	cls();
	// Set the font and display the title
	set_font((unsigned char*) Arial24x23);
	locate(65,5);
	puts(titles[mainState].c_str());

	// Draw the main state buttons
	fillrect(3,223,315,224, DarkGrey);
	fillrect(3,223,4,239, DarkGrey);
	fillrect(315,223,316,239, DarkGrey);
	fillrect(81,223,82,239, DarkGrey);
	fillrect(159,223,160,239, DarkGrey);
	fillrect(237,223,238,239, DarkGrey);

	// Display the labels for the main state
	set_font((unsigned char*) Arial12x12);

	locate(8,228);
	puts(labels[mainState][0].c_str());

	locate(86,228);
	puts(labels[mainState][1].c_str());

	locate(164,228);
	puts(labels[mainState][2].c_str());

	locate(242,228);
	puts(labels[mainState][3].c_str());

	// Display the shift labels
	background(White);
	foreground(Black);
	fillrect(7,211,78,223, White);
	locate(8,212);
	puts(labels[mainState][4].c_str());
	fillrect(85,211,156,223, White);
	locate(86,212);
	puts(labels[mainState][5].c_str());
	fillrect(163,211,234,223, White);
	locate(164,212);
	puts(labels[mainState][6].c_str());
	fillrect(241,211,312,223, White);
	locate(242,212);
	puts(labels[mainState][7].c_str());

	background(Black);
	foreground(White);
}

void Screen::updateText(){
	// Cover the previous title
	fillrect(0,0,320,35, Black);
	// Display the title
	set_font((unsigned char*) Arial24x23);
	locate(65,5);
	puts(titles[mainState].c_str());

	// Cover the previous labels
	fillrect(5,225,80,239, Black);
	fillrect(83,225,158,239, Black);
	fillrect(161,225,236,239, Black);
	fillrect(239,225,314,239, Black);

	// Display the labels for the main state
	set_font((unsigned char*) Arial12x12);
	locate(8,228);
	puts(labels[mainState][0].c_str());

	locate(86,228);
	puts(labels[mainState][1].c_str());

	locate(164,228);
	puts(labels[mainState][2].c_str());

	locate(242,228);
	puts(labels[mainState][3].c_str());

	// Display the shift labels
	background(White);
	foreground(Black);
	fillrect(7,211,78,223, White);
	locate(8,212);
	puts(labels[mainState][4].c_str());
	fillrect(85,211,156,223, White);
	locate(86,212);
	puts(labels[mainState][5].c_str());
	fillrect(163,211,234,223, White);
	locate(164,212);
	puts(labels[mainState][6].c_str());
	fillrect(241,211,312,223, White);
	locate(242,212);
	puts(labels[mainState][7].c_str());

	background(Black);
	foreground(White);
}

void Screen::updateScreen() {
	updateText();
}