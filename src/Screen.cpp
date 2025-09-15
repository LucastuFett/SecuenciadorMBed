#include "Screen.h"
#include "MIDIFile.h"

// Global variables for MIDI messages and control
extern enum state mainState;
extern int8_t tone;
extern int8_t mode;
extern int16_t tempo[2];
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
extern string filename;
extern string renameFilename;
extern uint8_t bank;
extern bool launchType;
extern int8_t launchTone;
extern int8_t launchMode;
extern int8_t launchOctave;
extern int8_t launchChn;
extern vector<uint16_t> launchPossible[2];
extern uint8_t launchMessages[16][3];
extern MIDIFile midiFile;
extern Mutex beatsPerToneMutex;
extern Mutex holdedMutex;

// Public Functions
// General Functions

Screen::Screen(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset, PinName dc, const char *name)
    : SPI_TFT_ILI9341(mosi, miso, sclk, cs, reset, dc, name) {
		_typePointer = 0;
		_letterPointer = 0;
		_specialPointer = 0;
		_edit = 0;
		_curPointer = false;
		_upper = false;
		_selectedFile = 0;
		_currentFile = -1;
		_curOctave = 2;

		for (uint8_t i = 0; i < 18; i++) _typing[i] = ' ';
		for (uint8_t i = 0; i < 12; i++) _files[i] = "";
		for (uint8_t i = 0; i < 12; i++) _lastFiles[i] = "";
		for (uint8_t i = 0; i < 12; i++) _bkgColors[i] = 0;
		for (uint8_t i = 0; i < 12; i++) _lastBkgColors[i] = 0;

		set_orientation(1);
		background(Black);    // set background to Black
		foreground(White);    // set chars to white
	}

void Screen::init() {
	cls();
	// Set the font and display the title
	set_font((unsigned char*) Arial24x23);
	locate(130,5);
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

void Screen::left() {
	switch (mainState) {
		case MEMORY:
		case RENAME:
			if (_edit == 1){
				_typePointer --;
				if (_typePointer < 0) _typePointer = 0;
			}
			else if (_edit == 2){
				if (_curPointer == 0){
					_letterPointer --;
					if (_letterPointer < 0)	_letterPointer = 25;
				}else if (_curPointer == 1){
					_specialPointer --;
					if (_specialPointer < 0) _specialPointer = 20;
				}
			}
			break;
		case SAVELOAD:
		case PLAY:
			_selectedFile --;
			if (_selectedFile < 0) _selectedFile = 11;
			break;
		default:
			break;
	}
}

void Screen::right() {
	switch (mainState) {
		case MEMORY:
		case RENAME:
			if (_edit == 1){
				_typePointer ++;
				if (_typePointer > 17 ) _typePointer = 17;
			}
			else if (_edit == 2){
				if (_curPointer == 0){
					_letterPointer ++;
					if (_letterPointer > 25) _letterPointer = 0;
				}else if (_curPointer == 1){
					_specialPointer ++;
					if (_specialPointer > 20) _specialPointer = 0;
				}
			}
		break;
		case SAVELOAD:
		case PLAY:
			_selectedFile ++;
			if (_selectedFile > 11) _selectedFile = 0;
			break;
		default:
			break;
	}
}

void Screen::updateScreen() {
	if (_lastState != mainState) coverTitle();
	switch (mainState) {
		case MAIN:
			showMenu(false);
			showPiano(false);
			showBanks(false);
			showLaunchpad(false);
			break;
		case PROG:
			showTyping(false);
			showBanks(false);
			showMenu(true);
			showPiano(true);
			paintGrid();
			if (velocity != _lastVel) {
				updateMenuText(4);
				_lastVel = velocity;
			}
			if (half != _lastHalf){
				updateMenuText(5);
				_lastHalf = half;
			}
			if (hold != _lastHold){
				updateHold();
				_lastHold = hold;
			}
			if (mode32 != _lastMode32){
				updateMenuText(6);
				_lastMode32 = mode32;
			}
			break;
		case NOTE:
			paintScales();
			paintGrid();
			break;
		case CHANNEL:
			updateMenuText(1);
			paintGrid();
			break;
		case TEMPO:
			// When canceling, does not update
			updateMenuText(2);
			break;
		case SCALE:
			getPossible(_possible, tone, mode);
			paintScales();
			paintGrid();
			updateMenuText(3);
			break;
		case MEMORY:
			showMenu(false);
			showPiano(false);
			showBanks(false);
			showTyping(true);
			updateMemoryText();
			break;
		case SAVELOAD:
			showTyping(false);
			showBanks(true);
			updateBanks();
			break;
		case RENAME:
			showBanks(false);
			showTyping(true);
			updateMemoryText();
			break;
		case PLAY:
			showBanks(true);		
			updateBanks();	
			break;
		case LAUNCH:
		case DAW:
			showLaunchpad(true);
			updateLaunch();
			break;
		default:
			break;
	}
	if (_lastState != mainState) updateText();
	_lastState = mainState;
}

void Screen::showError(string error){
	set_font((unsigned char*) Arial16x16);
	fillrect(10,100,310,140,Black);
	rect(9,109,311,141,Red);
	locate(15,112);
	puts(error.c_str());
	set_font((unsigned char*) Arial12x12);
	ThisThread::sleep_for(10s);
	fillrect(9,109,311,141,Black);
}

// Memory Functions

void Screen::selectLetter() {
	if (_edit == 1){
		_edit = 2;
		const char* findLowerPtr = std::find(letters, letters + 26, _typing[_typePointer]);
		int findLower = (findLowerPtr != letters + 26) ? (findLowerPtr - letters) : -1;
		const char* findUpperPtr = std::find(shLetters, shLetters + 26, _typing[_typePointer]);
		int findUpper = (findUpperPtr != shLetters + 26) ? (findUpperPtr - shLetters) : -1;
		const char* findSpecialPtr = std::find(special, special + 21, _typing[_typePointer]);
		int findSpecial = (findSpecialPtr != special + 21) ? (findSpecialPtr - special) : -1;
		if (findLower != -1) {
			_letterPointer = findLower;
			_curPointer = 0;
			_upper = 0;
		}
		if (findUpper != -1) {
			_letterPointer = findUpper;
			_curPointer = 0;
			_upper = 1;
		}
		if (findSpecial != -1) {
			_specialPointer = findSpecial;
			_curPointer = 1;
		}
	}else if (_edit == 2){
		_edit = 1;
		_letterPointer = 0;
		_specialPointer = 0;
		_curPointer = 0;
		_upper = 0;
	}

}

string Screen::getFilename() {
	if (mainState == PLAY) _currentFile = _selectedFile;
	return _files[_selectedFile];
}

string Screen::saveFilename() {
	string name = filename;
	size_t padding = 18 - filename.length();
	name.append(padding,' ');
	for (uint8_t i = 0; i < 18; i++) name[i] = _typing[i];
	_edit = 0;
	_letterPointer = 0;
	_specialPointer = 0;
	_curPointer = 0;
	_upper = 0;
	return name;
}

// Getters and Setters

bool Screen::getCurPointer() {
	return _curPointer;
}

void Screen::setCurPointer(bool curPointer){
	_curPointer = curPointer;
}

bool Screen::getUpper() {
	return _upper;
}

void Screen::setUpper(bool upper){
	_upper = upper;
}

void Screen::setEdit(int8_t edit){
	_edit = edit;
}

void Screen::setTyping(char type){
	_typing[_typePointer] = type;
}

// Private Functions
// General Functions

void Screen::coverTitle() {
	// Cover the previous title
	if(_lastState != PROG && _lastState != NOTE && _lastState != CHANNEL && _lastState != TEMPO && _lastState != SCALE) fillrect(0,0,320,35, Black);
	else fillrect(240,0,320,70, Black);
}

void Screen::updateText(){
	// Display the title
	const uint16_t titlesOffset[] = {130,0,0,110,110,110,0,0,0,150,15,135};
	set_font((unsigned char*) Arial24x23);
	switch(mainState) {
		case PROG:
			locate(245,10);
			puts("Edit");
			locate(245,35);
			puts("Seq");
			break;

		case NOTE:
			locate(245,10);
			puts("Edit");
			locate(245,35);
			puts("Note");
			break;

		case CHANNEL:
			locate(245,10);
			puts("Edit");
			locate(245,35);
			puts("Chan");
			break;

		case SCALE:
			locate(245,10);
			puts("Edit");
			locate(245,35);
			puts("Scal");
			break;

		case TEMPO:
			locate(245,10);
			puts("Edit");
			locate(245,35);
			puts("Tem");
			break;

		default:
			locate(titlesOffset[mainState],5);
			puts(titles[mainState].c_str());
			break;
	}
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

void Screen::showMenu(bool show) {
	if (show && !_menu) {
		// Draw Sides
		fillrect(2,30,68,31,Cyan);
		fillrect(2,30,3,180,Cyan);
		fillrect(68,30,69,180,Cyan);
		fillrect(2,180,68,181,Cyan);
		fillrect(2,80,68,81,Cyan);
		fillrect(2,130,68,131,Cyan);

		// Draw Velocity Circle
		circle(280,110,16, White);

		updateMenuText(0); // Update the menu text for all menus
	} else if (!show && _menu) {
		fillrect(0,30,70,181, Black); // Hide the menu
		fillrect(250,70,300,200, Black); // Hide the velocity circle and text
	}
	_menu = show;
}

void Screen::updateMenuText(uint8_t menu) {
	// Update the menu text based on the current state
	switch (menu) {
		case 0: // All Menus
			locate(6,43);
			puts("Channel");
			locate(33,65);
			puts(to_string(channel + 1).c_str());

			locate(12,93);
			puts("Tempo");
			locate(8,115);
			if (!tempo[0]) puts(((to_string(tempo[1]))+"BPM").c_str());
			else puts(tempo[1] == 0 ? "Half" : tempo[1] == 1 ? "Normal" : tempo[1] == 2 ? "Double" : "Err");

			locate(18,143);
			puts("Scale");
			locate(6,165);
			puts((tones[tone] + " " + scales[mode].name).c_str());

			locate(270,80);
			puts("Vel");
			locate(268,106);
			puts(to_string(velocity).c_str());

			locate(260,160);
			puts("1-16");
			locate(260,180);
			foreground(DarkGrey);
			puts("17-32");
			foreground(White);

			if (!mode32) {
				locate(260,140);
				puts("16 St");
			}else{
				locate(260,140);
				puts("32 St");
			}
			break;

		case 1: // Channel
			fillrect(33,59,67,71,Black);
			locate(33,65);
			puts(to_string(channel + 1).c_str());
			break;

		case 2: // Tempo
			fillrect(8,109,67,122,Black);
			locate(8,115);
			if (!tempo[0]) puts(((to_string(tempo[1]))+"BPM").c_str());
			else puts(tempo[1] == 0 ? "Half" : tempo[1] == 1 ? "Normal" : tempo[1] == 2 ? "Double" : "Err");
			break;

		case 3: // Scale
			fillrect(6,159,67,171,Black);
			locate(6,165);
			puts((tones[tone] + " " + scales[mode].name).c_str());
			break;

		case 4: // Velocity
			fillrect(268,106,290,118, Black);
			locate(268,106);
			puts(to_string(velocity).c_str());
			break;

		case 5: // Half
			if (!half || !mode32) {
				locate(260,160);
				puts("1-16");
				locate(260,180);
				foreground(DarkGrey);
				puts("17-32");
				foreground(White);				
			} else {
				locate(260,160);
				foreground(DarkGrey);
				puts("1-16");
				locate(260,180);
				foreground(White);
				puts("17-32");
			}
			break;
		case 6: // Mode Change
			fillrect(260,140,300,152,Black);
			if (!mode32) {
				locate(260,140);
				puts("16 St");
			}else{
				locate(260,140);
				puts("32 St");
			}
			break;
		default:
			break;
	}
}

// Piano Functions

void Screen::getPossible(vector <uint16_t> possible[2], int8_t tone, int8_t mode) {
	// possible = [[note, note2],[style, style2]]
	int8_t actKey = tone;
	possible[0].clear();
	possible[1].clear();
	
	possible[0].push_back(tone);
	possible[1].push_back(Magenta);

	for (uint8_t i = 0; i < 12; i++) {
		if (scales[mode].intervals[i] == 0) break;
	
		actKey += scales[mode].intervals[i];
		if (actKey > 11) {
			actKey -= 12;
		}
		possible[0].push_back(actKey);
		possible[1].push_back(Blue);

	}
}

void Screen::paintScales(){
	int8_t pos = -1;
	fillrect(72,10,78,203, Black); // Clear the scale area
	for (uint8_t i = 0; i < _possible[0].size(); i++) {
		// Paint Scale Notes
		fillrect(72,201 - (_possible[0][i] * 8) - 4, 78, 201 - (_possible[0][i] * 8), _possible[1][i]);
		fillrect(72,104 - (_possible[0][i] * 8) - 4, 78, 104 - (_possible[0][i] * 8), _possible[1][i]);
		if (note == _possible[0][i]) {
			pos = i;
		}
	}

	// If necessary, change the current octave to match
	uint8_t offset = 0;
	if (octave == _curOctave - 1) _curOctave -= 2;
	if (octave == _curOctave + 2) _curOctave += 2;
	if (octave == _curOctave + 1) offset += 97;

	// If the note was in the scale, paint it green for selected, if not orange for selected
	if (pos != -1) fillrect(72, 201 - (_possible[0][pos] * 8) - 4 - offset, 78, 201 - (_possible[0][pos] * 8) - offset, Green);
	else fillrect(72, 201 - (note * 8) - 4 - offset, 78, 201 - (note * 8) - offset, Orange);

	// Write the Current Octave down in C
	locate(48,195);
	puts(("C" + to_string(_curOctave)).c_str());
}

void Screen::showPiano(bool show){
	const uint8_t blackOffset[] = {7, 23, 39, 63, 79, 103, 119, 135, 159, 175};
	const uint8_t whiteOffset[] = {55, 95, 151};
	if (show && !_piano) {
		// Draw Piano Roll
		for (uint8_t i = 0; i < 144; i += 9) {
			// Vertical Lines
			line(94 + i, 11, 94 + i, 202, LightGrey);
		}

		for (uint8_t i = 0; i < 192; i += 8) {
			// Horizontal Lines
			line(94, 10 + i, 238, 10 + i, LightGrey);
		}

		// Draw Piano
		fillrect(81,11,93,202, White);
		for (uint8_t i = 0; i < 10; i ++){
			rect(81, 11 + blackOffset[i], 89, 19 + blackOffset[i], DarkGrey);
			fillrect(81, 12 + blackOffset[i], 88, 18 + blackOffset[i], Black);
			line(90, 15 + blackOffset[i], 93, 15 + blackOffset[i], DarkGrey);
		}
		for (uint8_t i = 0; i < 3; i ++){
			line(81, 11 + whiteOffset[i], 93, 11 + whiteOffset[i], DarkGrey);
		}

		// Draw Bounding Box
		rect(80,10,238,203, Cyan);
		line(94,106,238,106, Cyan);
		line(166,11,166,202, Cyan);

		// Draw Selected Scale
		getPossible(_possible, tone, mode);
		paintScales();

	} else if (!show && _piano) {
		fillrect(48, 10, 238, 203, Black); // Hide the piano roll
	}
	_piano = show;
}

void Screen::paintGrid() {
	uint8_t gridBeat = 16;
	int8_t gridNote = 11;
	uint8_t gridOctave = _curOctave;
	uint16_t bptIndex;
	uint32_t beatMask = 0;
	for(uint8_t i = 0; i < 16; i++){
		gridBeat --;
		gridNote = 11;
		gridOctave = _curOctave + 1;

		for(uint8_t j = 0; j < 24; j++){
			int8_t indNote = gridNote;
			if (gridNote < 0) {
				indNote = gridNote + 12;
				if (gridOctave == _curOctave + 1) gridOctave --;
			}
			uint8_t num = gridBeat;
			if (mode32 && half) num += 16;
			bptIndex = (indNote + (gridOctave *12)) * 16 + channel;
			beatMask = 0x80000000 >> num;
			uint16_t xStart = 95 + (gridBeat * 9);
			uint16_t yStart = 11 + (11 - indNote) * 8 + (1 - (gridOctave - _curOctave)) * 96;
			uint16_t xEnd = 102 + (gridBeat * 9);
			uint16_t yEnd = 17 + (11 - indNote) * 8 + (1 - (gridOctave - _curOctave)) * 96;
			beatsPerToneMutex.lock();
			if((beatsPerTone[bptIndex] & beatMask) != 0) { 
				if(count(_possible[0].begin(),_possible[0].end(),indNote)){
					if(note == indNote && octave == gridOctave) fillrect(xStart, yStart, xEnd, yEnd, Green);
					else if (indNote == _possible[0][0]) fillrect(xStart, yStart, xEnd, yEnd, Magenta);
					else fillrect(xStart, yStart, xEnd, yEnd, Blue);
				} 
				else {
					if(note == indNote && octave == gridOctave) fillrect(xStart, yStart, xEnd, yEnd, Orange);
					else fillrect(xStart, yStart, xEnd, yEnd, Red);					
				}
			}else fillrect(xStart, yStart, xEnd, yEnd, Black);
			beatsPerToneMutex.unlock();
			gridNote--;
		}
	}
	holdedMutex.lock();
	for (const auto &i : holded ) {
        uint8_t endBeat = i.second;
		uint8_t firstBeat = i.first.beatStart;
		bool n = firstBeat >= 16;
		bool e = endBeat >= 16;

		if (((!mode32) && (n || e)) || (n && !half) || ((!n) && (!e) && mode32 && half)) continue;
		
		if (e && half) endBeat -= 16;
		else if ((n && !e) || (e && !half)) endBeat = 15;

		if (n) firstBeat -= 16;
		else if (!n && e && half) firstBeat = 0;

		// If it's the correct channel, and the note does not overflow the current limits
		if ((i.first.channel == channel) && (i.first.midiNote >= (_curOctave * 12 + 24) && i.first.midiNote < (_curOctave + 2) * 12 + 24)){
			uint8_t holdNote = (i.first.midiNote - 24) % 12;
			gridNote = 11 - ((i.first.midiNote - 24) % 12);
			gridOctave = (i.first.midiNote - 24) / 12;
			//uint8_t gridEnd = 15 - endBeat;

			for(gridBeat = firstBeat; gridBeat <= endBeat; gridBeat ++) {
				uint16_t xStart = 95 + (gridBeat * 9);
				uint16_t yStart = 11 + (gridNote) * 8 + (1 - (gridOctave - _curOctave)) * 96;
				uint16_t xEnd = 102 + (gridBeat * 9);
				uint16_t yEnd = 17 + (gridNote) * 8 + (1 - (gridOctave - _curOctave)) * 96;
				if(count(_possible[0].begin(),_possible[0].end(),holdNote)){
					if(note == holdNote && octave == gridOctave) fillrect(xStart, yStart, xEnd, yEnd, Green);
					else if (holdNote == _possible[0][0]) fillrect(xStart, yStart, xEnd, yEnd, Magenta);
					else fillrect(xStart, yStart, xEnd, yEnd, Blue);
				} 
				else {
					if(note == holdNote && octave == gridOctave) fillrect(xStart, yStart, xEnd, yEnd, Orange);
					else fillrect(xStart, yStart, xEnd, yEnd, Red);					
				}		
			}
		}
    }
	holdedMutex.unlock();
}

void Screen::updateHold() {
	switch (hold) {
		case 0:
			fillrect(245,190,320,210,Black);
			break;
		case 1:
			locate(245,195);
			puts("1st Value");
			break;
		case 2:
			locate(245,195);
			puts("2nd Value");
			break;			
		}
}

// Memory Functions

void Screen::updateMemoryText() {
	string name = "";
	set_font((unsigned char*) Arial16x16);
	if (mainState == MEMORY) name = filename;
	else if (mainState == RENAME) name = renameFilename;
	if (name != "" && _edit == 0){
		for (uint8_t i = 0; i < name.length(); i++)	_typing[i] = name[i];
		// Draw first letter as selected (grey background, white foreground)
		background(DarkGrey);
		fillrect(7,52,23,68,DarkGrey);
		locate(8,52);
		_putc(_typing[0]);
		// Draw every other letter
		background(Black);
		for (uint8_t i = 1; i < 18; i ++){
			locate(8 + i*17,52);
			_putc(_typing[i]);
		}
		_edit = 1;
	}else if (_edit == 1){
		// Cover left and right letter background
		if(_typePointer > 0) fillrect(7 + (_typePointer - 1)*17 ,52, 23 + (_typePointer - 1)*17,68,Black);
		if(_typePointer < 17) fillrect(7 + (_typePointer + 1)*17 ,52, 23 + (_typePointer + 1)*17,68,Black);
		// Draw every letter
		background(Black);
		for (uint8_t i = 0; i < 18; i ++){
			locate(8 + i*17,52);
			_putc(_typing[i]);
		}
		// Draw the pointed at letter as selected
		background(DarkGrey);
		fillrect(7 + _typePointer*17 ,52, 23 + _typePointer*17,68,DarkGrey);
		locate(8 + _typePointer*17,52);
		_putc(_typing[_typePointer]);
		background(Black);
	}else if (_edit == 2){
		if (_curPointer == 0){
			if (_upper == 0) _typing[_typePointer] = letters[_letterPointer];
			else _typing[_typePointer] = shLetters[_letterPointer];
		}else _typing[_typePointer] = special[_specialPointer];
		// Draw the pointed at letter as editing
		background(White);
		foreground(Black);
		fillrect(7 + _typePointer*17 ,52, 23 + _typePointer*17,68,White);
		locate(8 + _typePointer*17,52);
		_putc(_typing[_typePointer]);
		background(Black);
		foreground(White);
	}
	set_font((unsigned char*) Arial12x12);
}

void Screen::updateBanks() {
	if (bank != _lastBank){
		set_font((unsigned char*) Arial16x16);
		locate(20,12);
		puts(("Bank " + to_string(bank)).c_str());
		_lastBank = bank;
		set_font((unsigned char*) Arial12x12);
	}

	midiFile.getFiles(bank, _files);
	for (uint8_t i = 0; i < 12; i ++){
		if (i == _currentFile) _bkgColors[i] = Purple;
		else if (i == _selectedFile) _bkgColors[i] = Blue;
		else _bkgColors[i] = DarkGrey;
	}
	for (uint8_t i = 0; i < 3; i ++){
		for (uint8_t j = 0; j < 4; j ++){
			if (_bkgColors[i*4 + j] != _lastBkgColors[i*4 + j] || _files[i*4 + j] != _lastFiles[i*4 + j]){
				fillrect(14 + j*74, 40 + i*54, 84 + j*74, 90 + i*54, _bkgColors[i*4 + j]);
				if (_files[i*4 + j] != ""){
					background(_bkgColors[i*4 + j]);
					string name = _files[i * 4 + j];
					locate(17 + j * 74, 43 + i * 54);
					puts(name.substr(0, 7).c_str());
					locate(17 + j * 74, 59 + i * 54);
					puts(name.length() > 7 ? name.substr(7, 7).c_str() : "");
					locate(30 + j * 74, 75 + i * 54);
					puts(name.length() > 14 ? name.substr(14).c_str() : "");
					background(Black);
				}
			}
		}
	}
	copy(begin(_files), end(_files), _lastFiles);
	memcpy(_lastBkgColors,_bkgColors, sizeof(_bkgColors));
	_lastSelectedFile = _selectedFile;
	_lastCurrentFile = _currentFile;

}

void Screen::showTyping(bool show){
	
	if (show && !_typeBox) {
		rect(5,50,314,70,Cyan);
		fillrect(6,51,313,69,Black);
	} else if (!show && _typeBox) {
		fillrect(5,50,314,70, Black); // Hide Typing Box
	}
	_typeBox = show;
}

void Screen::showBanks(bool show){
	
	if (show && !_memBanks) {
		set_font((unsigned char*) Arial16x16);
		locate(20,12);
		puts(("Bank " + to_string(bank)).c_str());
		_lastBank = bank;
		set_font((unsigned char*) Arial12x12);
		midiFile.getFiles(bank, _files);
		for (uint8_t i = 0; i < 12; i ++){
			if (i == _selectedFile) _bkgColors[i] = Blue;
			else _bkgColors[i] = DarkGrey;
		}
		for (uint8_t i = 0; i < 3; i ++){
			for (uint8_t j = 0; j < 4; j ++){
				fillrect(14 + j*74, 40 + i*54, 84 + j*74, 90 + i*54, _bkgColors[i*4 + j]);
				if (_files[i*4 + j] != ""){
					background(_bkgColors[i*4 + j]);
					string name = _files[i * 4 + j];
					locate(17 + j * 74, 43 + i * 54);
					puts(name.substr(0, 7).c_str());
					locate(17 + j * 74, 59 + i * 54);
					puts(name.length() > 7 ? name.substr(7, 7).c_str() : "");
					locate(30 + j * 74, 75 + i * 54);
					puts(name.length() > 14 ? name.substr(14).c_str() : "");
					background(Black);
				}
				
			}
		}
		copy(begin(_files), end(_files), _lastFiles);
		memcpy(_lastBkgColors,_bkgColors, sizeof(_bkgColors));
		_lastSelectedFile = _selectedFile;
		_lastCurrentFile = _currentFile;
	} else if (!show && _memBanks) {
		fillrect(10, 40, 310, 200, Black); // Hide Banks Box
		fillrect(20, 10, 113, 30, Black); // Hide Bank Number
	}
	_memBanks = show;
}

// Launchpad Functions

void Screen::showLaunchpad(bool show){
	// Implement
	if (show && !_launchpad){
		for (uint8_t i = 0; i < 2; i ++){
			for (uint8_t j = 0; j < 8; j ++){
				fillrect(6 + j*39, 97 + i*54, 41 + j*39, 147 + i*54, launchColorsScr[i*8 + j]);
			}
		}	
	} else if (!show && _launchpad) {
		fillrect(5, 40, 320, 205, Black); // Hide Launchpad Box
	}
	_launchpad = show;
}

void Screen::updateLaunch(){
	if (mainState == LAUNCH){	
		if (_lastLaunchChn != launchChn){
			locate(120,40);
			set_font((unsigned char*) Arial16x16);
			puts(("Channel " + to_string(launchChn+1)).c_str());
			set_font((unsigned char*) Arial12x12);
			_lastLaunchChn = launchChn;
		}
		if (!launchType){	// Launchpad
			getPossible(launchPossible,launchTone, launchMode);
			if (_lastLaunchType != launchType){
				fillrect(120,65,320,85,Black);
				locate(200,65);
				set_font((unsigned char*) Arial16x16);
				puts("Launchpad");
				set_font((unsigned char*) Arial12x12);
				
			}
			if (_lastLaunchTone != launchTone || _lastLaunchMode != launchMode || _lastLaunchType != launchType){
				fillrect(40,65,160,85,Black);
				locate(40,65);
				set_font((unsigned char*) Arial16x16);
				puts((tones[launchTone] + " " + scales[launchMode].name).c_str());
				set_font((unsigned char*) Arial12x12);
			}
			if (_lastLaunchTone != launchTone || _lastLaunchMode != launchMode || _lastLaunchOctave != launchOctave || _lastLaunchType != launchType){
				for (uint8_t i = 0; i < 2; i ++){
					for (uint8_t j = 0; j < 8; j ++){
						fillrect(6 + j*39, 97 + i*54, 41 + j*39, 147 + i*54, launchColorsScr[i*8 + j]);
					}
				}	
				for (uint8_t i = 0; i < 8; i ++){
					uint8_t noteIndexLow;
					uint8_t noteIndexHigh;
					if (i == 7){
						noteIndexLow = launchPossible[0][0] + ((launchOctave+1) * 12) + 24;
						noteIndexHigh = launchPossible[0][0] + ((launchOctave+2) * 12) + 24;

						background(launchColorsScr[i]);
						locate(8 + i*39, 117);
						puts((tones[launchPossible[0][0]] + to_string(launchOctave + 1)).c_str());

						background(launchColorsScr[i+8]);
						locate(8 + i*39, 171);
						puts((tones[launchPossible[0][0]] + to_string(launchOctave + 2)).c_str());
						background(Black);
					}else{
						noteIndexLow = launchPossible[0][i] + ((launchOctave) * 12) + 24;
						noteIndexHigh = launchPossible[0][i] + ((launchOctave+1) * 12) + 24;

						background(launchColorsScr[i]);
						locate(8 + i*39, 117);
						puts((tones[launchPossible[0][i]] + to_string(launchOctave)).c_str());

						background(launchColorsScr[i+8]);
						locate(8 + i*39, 171);
						puts((tones[launchPossible[0][i]] + to_string(launchOctave + 1)).c_str());
						background(Black);		
					}
					launchMessages[i][0] = 0x90 | launchChn;
					launchMessages[i][1] = noteIndexLow;
					launchMessages[i][2] = 127;
					launchMessages[i+8][0] = 0x90 | launchChn;
					launchMessages[i+8][1] = noteIndexHigh;
					launchMessages[i+8][2] = 127;
				}		
				_lastLaunchTone = launchTone;
				_lastLaunchMode = launchMode;
				_lastLaunchOctave = launchOctave;
				_lastLaunchType = launchType;
			}

			
		}else{	// Keyboard
			if (_lastLaunchType != launchType){
				fillrect(40,65,320,85,Black);
				locate(120,65);
				set_font((unsigned char*) Arial16x16);
				puts("Keyboard");
				set_font((unsigned char*) Arial12x12);

				fillrect(6,97,41,147,Black);
				fillrect(123,97,158,147,Black);
				fillrect(279,97,314,147,Black);

			}
			const string keyLabels[] = {"","C#","D#","","F#","G#","A#","","C","D","E","F","G","A","B","C"};
			if (_lastLaunchOctave != launchOctave || _lastLaunchType != launchType){
				for (uint8_t i = 0; i < 8; i ++){
					background(launchColorsScr[i]);
					locate(8 + i*39, 117);
					if(keyLabels[i] != "") puts((keyLabels[i] + to_string(launchOctave)).c_str());

					background(launchColorsScr[i+8]);
					locate(8 + i*39, 171);
					if (i == 7) puts((keyLabels[i+8] + to_string(launchOctave + 1)).c_str());
					else puts((keyLabels[i+8] + to_string(launchOctave)).c_str());
					background(Black);		
				}
				_lastLaunchType = launchType;
			}
			
			launchMessages[0][0] = 0;
			launchMessages[0][1] = 0;
			launchMessages[0][2] = 0;

			launchMessages[1][0] = 0x90 | launchChn;
			launchMessages[1][1] = 1 + ((launchOctave) * 12) + 24;
			launchMessages[1][2] = 127;

			launchMessages[2][0] = 0x90 | launchChn;
			launchMessages[2][1] = 3 + ((launchOctave) * 12) + 24;
			launchMessages[2][2] = 127;

			launchMessages[3][0] = 0;
			launchMessages[3][1] = 0;
			launchMessages[3][2] = 0;

			launchMessages[4][0] = 0x90 | launchChn;
			launchMessages[4][1] = 6 + ((launchOctave) * 12) + 24;
			launchMessages[4][2] = 127;

			launchMessages[5][0] = 0x90 | launchChn;
			launchMessages[5][1] = 8 + ((launchOctave) * 12) + 24;
			launchMessages[5][2] = 127;

			launchMessages[6][0] = 0x90 | launchChn;
			launchMessages[6][1] = 10 + ((launchOctave) * 12) + 24;
			launchMessages[6][2] = 127;

			launchMessages[7][0] = 0;
			launchMessages[7][1] = 0;
			launchMessages[7][2] = 0;

			launchMessages[8][0] = 0x90 | launchChn;
			launchMessages[8][1] = 0 + ((launchOctave) * 12) + 24;
			launchMessages[8][2] = 127;

			launchMessages[9][0] = 0x90 | launchChn;
			launchMessages[9][1] = 2 + ((launchOctave) * 12) + 24;
			launchMessages[9][2] = 127;

			launchMessages[10][0] = 0x90 | launchChn;
			launchMessages[10][1] = 4 + ((launchOctave) * 12) + 24;
			launchMessages[10][2] = 127;

			launchMessages[11][0] = 0x90 | launchChn;
			launchMessages[11][1] = 5 + ((launchOctave) * 12) + 24;
			launchMessages[11][2] = 127;

			launchMessages[12][0] = 0x90 | launchChn;
			launchMessages[12][1] = 7 + ((launchOctave) * 12) + 24;
			launchMessages[12][2] = 127;

			launchMessages[13][0] = 0x90 | launchChn;
			launchMessages[13][1] = 9 + ((launchOctave) * 12) + 24;
			launchMessages[13][2] = 127;

			launchMessages[14][0] = 0x90 | launchChn;
			launchMessages[14][1] = 11 + ((launchOctave) * 12) + 24;
			launchMessages[14][2] = 127;

			launchMessages[15][0] = 0x90 | launchChn;
			launchMessages[15][1] = 0 + ((launchOctave+1) * 12) + 24;
			launchMessages[15][2] = 127;

		}
	} else if(mainState == DAW){
		for (uint8_t i = 0; i < 8; i ++){

			background(launchColorsScr[i]);
			locate(8 + i*39, 117);
			puts(to_string(102 + i).c_str());

			background(launchColorsScr[i+8]);
			locate(8 + i*39, 171);
			puts(to_string(102 + i+8).c_str());
			background(Black);		

			launchMessages[i][0] = 0xB0;
			launchMessages[i][1] = 102 + i;
			launchMessages[i][2] = 127;

			launchMessages[i+8][0] = 0xB0;
			launchMessages[i+8][1] = 102 + i+8;
			launchMessages[i+8][2] = 127;
		}
		_lastLaunchType = !launchType; // Fix for next time
	}
}