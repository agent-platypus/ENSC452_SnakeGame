#include "menu.h"
#include <cstring>
#include "stdio.h"
#include "graphics_driver.h"

#define DIFFICULTY_TEXT_X 627
#define DIFFICULTY_TEXT_Y 425

#define GAMEOVER_TEXT_X 100
#define GAMEOVER_TEXT_Y 5

extern int* image_buffer_pointer;
extern int NUM_BYTES_BUFFER;
int* startscreen = (int*) 0x018D2008;
int* startscreenStartGame = (int*) 0x020BB00C;
int* startscreenOptions = (int*) 0x028A4010;
int* optionscreen = (int *) 0x0308D014;
int* easy_text =   (int *) 0x03876018;
int* medium_text = (int *) 0x03896018;
int* hard_text=    (int *) 0x06006018;
int* victory_text= (int *) 0x05006018;
int* defeat_text = (int *) 0x03926018;

enum color{
	blue = 0,
	green =1,
	red=2,
	white=3,
	cyan=4,
	ash_gray=5,
	yellow=6,
	darkblue=7,
	darkgreen=8,
	darkred=9,
	olive=10,
	purple=11,
	teal=12,
	silver=13,
	gray=14
};
int colors[15] = {
    0x00FF0000, // Blue (255, 0, 0)
    0x0000FF00, // Green (0, 255, 0)
    0x000000FF, // Red (0, 0, 255)
    0x00FFFFFF, // White (255, 255, 255)
    0x0000FFFF, // Cyan (0, 255, 255)
    0x00B2BEB5, // Ash Grey (255, 0, 255)
    0x00FFFF00, // Yellow (255, 255, 0)
    0x00800000, // Dark Blue (128, 0, 0)
    0x00008000, // Dark Green (0, 128, 0)
    0x00000080, // Dark Red (0, 0, 128)
    0x00808000, // Olive (128, 128, 0)
    0x00800080, // Purple (128, 0, 128)
    0x00008080, // Teal (0, 128, 128)
    0x00C0C0C0, // Silver (192, 192, 192)
    0x00808080  // Gray (128, 128, 128)
};

int colorindex = green;
int currentcolor = colors[colorindex];

void StartScreen(){
	memcpy(image_buffer_pointer, startscreen,NUM_BYTES_BUFFER);
}
void StartScreenStartGame(){
	memcpy(image_buffer_pointer, startscreenStartGame,NUM_BYTES_BUFFER);
}
void StartScreenOptions(){
	memcpy(image_buffer_pointer, startscreenOptions,NUM_BYTES_BUFFER);
}

void OptionScreen(){
	memcpy(image_buffer_pointer, optionscreen,NUM_BYTES_BUFFER);
}

void OutlineOption(int select, bool remove) {
	switch(select){
	case 0: //volume selected
		DrawOutline(412,296,744,358, remove);
		break;
	case 1: // difficulty
		DrawOutline(412,419,744,481, remove);
		break;
	case 2: // snake color
		DrawOutline(412,556,744,618, remove);
		break;
	default:
		break;
	}
}

void DrawOutline(int x1, int y1, int x2, int y2, bool remove) {
	//top line
	for(int y = y1; y<y1+4; y++) {
		for(int x = x1 ; x < x2; x++) {
				if(!remove)
					image_buffer_pointer [1280*y + x]= 0x000000;
				else
					image_buffer_pointer [1280*y + x]= 0xFFFFFF;
		}
	}
	// bottom line
	for(int y = y2-3; y<=y2; y++) {
			for(int x = x1 ; x < x2; x++) {
				if(!remove)
					image_buffer_pointer [1280*y + x]= 0x000000;
				else
					image_buffer_pointer [1280*y + x]= 0xFFFFFF;
			}
		}
	//left line
	for(int y = y1; y<y2; y++) {
			for(int x = x1 ; x < x1+4; x++) {
				if(!remove)
					image_buffer_pointer [1280*y + x]= 0x000000;
				else
					image_buffer_pointer [1280*y + x]= 0xFFFFFF;
			}
		}
	// right line
	for(int y = y1; y<y2; y++) {
			for(int x = x2-3 ; x < x2; x++) {
				if(!remove)
					image_buffer_pointer [1280*y + x]= 0x000000;
				else
					image_buffer_pointer [1280*y + x]= 0xFFFFFF;
			}
		}
}
void StartMenuToggle(int option) {
	switch(option) {
	case 1:
		StartScreenStartGame();
		break;
	case 2:
		StartScreenOptions();
		break;
	default:
		break;
	}

}

void ToggleColor() {
	colorindex++;
	colorindex = colorindex%15;
	currentcolor = colors[colorindex];
	//printf("current color\n");
}

void DisplayDifficulty(int difficulty) {
	ClearDifficulty();
	switch(difficulty) {
		case 0:
			DisplayEasy();
			break;
		case 1:
			DisplayMedium();
			break;
		case 2:
			DisplayHard();
			break;
		default:
			break;
	}
}
void DisplayEasy() {
	// 627, 429	difficulty
	//for(int y = DIFFICULTY_TEXT_Y; y<DIFFICULTY_TEXT_Y+58; y++) {

	for(int y = 0; y<52; y++) {
		memcpy(image_buffer_pointer + ((y+DIFFICULTY_TEXT_Y)*1280 + DIFFICULTY_TEXT_X), easy_text + (y*109), 436);
	}
}
void DisplayMedium() {
	// 627, 429	difficulty
	//for(int y = DIFFICULTY_TEXT_Y; y<DIFFICULTY_TEXT_Y+ 52; y++) {
	for(int y = 0; y<48; y++) {
		memcpy(image_buffer_pointer +((y+DIFFICULTY_TEXT_Y)*1280 + DIFFICULTY_TEXT_X), medium_text + (y*102), 408);
	}
}

void DisplayHard() {
	for(int y = 0; y<51; y++) {
		memcpy(image_buffer_pointer +((y+DIFFICULTY_TEXT_Y)*1280 + DIFFICULTY_TEXT_X), hard_text + (y*119), 456);
	}
}

void DisplayGameover() {
	for(int y = 0; y<100; y++) {
		memcpy(image_buffer_pointer +((y+GAMEOVER_TEXT_Y)*1280 + GAMEOVER_TEXT_X), defeat_text + (y*313), 1252);
	}
}

void ClearDifficulty() {
	for(int y = DIFFICULTY_TEXT_Y; y< DIFFICULTY_TEXT_Y+53; y++) {
		for(int x = DIFFICULTY_TEXT_X; x<DIFFICULTY_TEXT_X+112; x++) {
			image_buffer_pointer[y*1280 + x] = 0xFFFFFF;
		}
	}
}

void DisplayVictory() {
	for(int y = 0; y<99; y++) {
		memcpy(image_buffer_pointer +((y+GAMEOVER_TEXT_Y)*1280 + GAMEOVER_TEXT_X), victory_text + (y*337), 1348);
	}
}



