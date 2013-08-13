#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>
#include <Time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "timemod.h"

#define uchar unsigned char

#define OLED_DC 11
#define OLED_CS 12
#define OLED_CLK 10
#define OLED_MOSI 9
#define OLED_RESET 13

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#include <Keypad.h>

const uchar ROWS = 4; //four rows
const uchar COLS = 3; //three columns
const char keys[ROWS][COLS] = {
		{'1','2','3'},
		{'4','5','6'},
		{'7','8','9'},
		{'*','0','#'}
};
byte rowPins[ROWS] = {5, 4, A1, A0}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6}; // connect to the column pinouts of the keypad

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
Keypad           keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool editMode = false;
uchar editingIndex = 0;
time_t t_time;
bool flash  = false;
bool flash2 = false;

uchar t_index = 0;
char t_buf[5];

typedef int (*timefunc)(void);
typedef int (*timefunc2)(time_t);

const uchar NO_OF_EDITING_SEGS = 6;
const uchar     editingBounds   [] = { 0, 2, 4, 6, 8, 10 };
const uchar     editingLengths  [] = { 2, 2, 2, 2, 2, 4  };
const uchar     editingTElements[] = { 2, 1, 0, 4, 5, 6  };
const char*     normalFormats   [] = { "%02d", "%02d", "%02d", "%02d", "%02d", "%04d" };
const char*     editingFormats  [] = { "%-2s", "%-2s", "%-2s", "%-2s", "%-2s", "%-4s" };
const timefunc  editingTFuns    [] = {
	(timefunc)hour, (timefunc)minute, (timefunc)second,
	(timefunc)day, (timefunc)month, (timefunc)year
};

const uchar NO_OF_TIME_ELEMENTS = 7;
const timefunc timeElementFuncsNoArg[] = {
	(timefunc)second, (timefunc)minute, (timefunc)hour,
	(timefunc)weekday, (timefunc)day, (timefunc)month, (timefunc)year
};
const timefunc2 timeElementFuncs[] = {
	(timefunc2)second, (timefunc2)minute, (timefunc2)hour,
	(timefunc2)weekday, (timefunc2)day, (timefunc2)month, (timefunc2)year
};

// sec, min, hour, wday, day, month, year
bool checkTEdit(uchar t_element, int entered_input) {
	// check that element is as expected
	return entered_input == timeElementFuncsNoArg[t_element]();
}

void ResetTBuf() {
	t_index  = 0;
	t_buf[0] = '\0';
}

void editAdv(bool skip) {
	if (!editMode) {
		editMode = true;
		editingIndex = 0;
		ResetTBuf();
		return;
	}

	for (uchar seg = 0; seg < NO_OF_EDITING_SEGS; seg++) {
		if (editingIndex == (editingBounds[seg] + editingLengths[seg] - 1)) {
			int num = atoi(t_buf);
			setTime(
				seg == 0 ? num : hour(),
				seg == 1 ? num : minute(),
				seg == 2 ? num : second(),
				seg == 3 ? num : day(),
				seg == 4 ? num : month(),
				seg == 5 ? num : year()
			);
			if (!checkTEdit(editingTElements[seg], atoi(t_buf))) {
				setTime(t_time);
				if (!skip) {
					editingIndex = editingBounds[seg];
					ResetTBuf();
					return;
				}
			} else {
				ResetTBuf();
			}
			break;
		}
	}

	if (skip) {
		for (uchar seg = 0; seg < NO_OF_EDITING_SEGS; seg++) {
			uchar next = editingBounds[seg] + editingLengths[seg];
			if (editingIndex < next) {
				if (t_index == 0) {
					editingIndex = next;
					if (seg < NO_OF_EDITING_SEGS - 1) return;
				} else {
					editingIndex = next - 1;
				}
			}
		}
	}

	editingIndex++;

	// check if finished editing
	if (
		editingIndex >=
		editingBounds[NO_OF_EDITING_SEGS - 1] +
		editingLengths[NO_OF_EDITING_SEGS - 1]
	) {
		ResetTBuf();
		editingIndex = 0;
		editMode     = false;
	}
}

void printSegment(const int segIndex) {
	char buf[5];
	if (
		editMode &&
		editingIndex >= editingBounds[segIndex] &&
		editingIndex < (editingBounds[segIndex] + editingLengths[segIndex])
	) {
		if (flash) {
			for (uchar i = 0; i < editingLengths[segIndex]; i++) {
				display.print(' ');
			}
		} else {
			if (!t_index) {
				sprintf(buf, normalFormats[segIndex], editingTFuns[segIndex]());
				display.print(buf);
			} else {
				sprintf(buf, editingFormats[segIndex], t_buf);
				display.print(buf);
			}
		}
	} else {
		sprintf(buf, normalFormats[segIndex], editingTFuns[segIndex]());
		display.print(buf);
	}
}

void setup() {
	Serial.begin(9600);

	// generate the high voltage from the 3.3v line
	display.begin(SSD1306_SWITCHCAPVCC);
}

void loop() {
	const char key = keypad.getKey();

	flash  = (millis() % 1000) > 500;
	flash2 = (millis() % 2000) > 1000;

	if (key == '#') {
		if (editMode) {
			editAdv(true);
		} else {
			// going into edit mode
			editAdv(false);
		}
	} else {
		if (editMode) {
			if (isdigit(key)) {
				t_buf[t_index++] = key;
				t_buf[t_index] = '\0';
				t_time = now();
				editAdv(false);
			}
		}
	}
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0,0);

	display.println("Time:     Date:");

	printSegment(0);
	display.print(':');
	printSegment(1);
	display.print(flash2 ? ' ' : ':');
	printSegment(2);
	display.print("  ");
	printSegment(3);
	display.print('/');
	printSegment(4);
	display.print('/');
	printSegment(5);
	display.println();

	display.display();
}
