#include "graphics_driver.h"
#include <unistd.h>
#include <cstring>
#include <time.h>
#include "game_logic.h"
int black_screen[1280*1024];
int* image_buffer_pointer = (int *)0x00900000;
int* buffer2_pointer = (int *)0x018D2008;
int NUM_BYTES_BUFFER = 5242880;
extern int currentcolor;
extern int* colors;
int SnakeFaceTemp[20][20];
int SnakeAliveFace[20][20] = {

		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1},
		{1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1},
		{1,0,0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0,0,1},
		{1,0,0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,1,2,2,2,2,1,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,1,2,2,2,2,1,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int SnakeDeadFace[20][20] = {
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
		{1,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1},
		{1,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,1},
		{1,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1},
		{1,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

void DrawSnakeHead(int x_coord, int y_coord, bool remove, int color, bool alive) {
	int SnakeFaceSelect;
	for(int y = 0; y<20; y++) {
		for(int x = 0; x<20; x++) {
			int pixel_color;
			if(!remove){
				if(alive){
					SnakeFaceSelect = SnakeAliveFace[y][x];
				}
				else {
					SnakeFaceSelect = SnakeDeadFace[y][x];
				}
				switch(SnakeFaceSelect){
					case 0:
						pixel_color = color;
						break;
					case 1:
						pixel_color = 0x000000; // black
						break;
					case 2:
						pixel_color = 0x0000FF; // tongue color = red
						break;
					default:
						pixel_color = color;
						break;
					}
				image_buffer_pointer[(y_coord+y)*1280+(x_coord+x)] = pixel_color;
			//		image_buffer_pointer[y*1280+x] = color;
			}
			else
				image_buffer_pointer[(y_coord+y)*1280+(x_coord+x)] = 0xFF00FF;
		}
	}
}
//void Rotate90DegClockWise(int mat[20][20]) {
//	// transpose matrix then reverse order of columns to rotate 90 degrees clockwise
//	// to implement the snake head direction
//    for (int i = 0; i < N; i++) {
//        for (int j = 0; j < N; j++) {
//        	SnakeFaceTemp[j][N - i - 1] = mat[i][j];
//        }
//    }
//}
//
//void Rotate90DegCounterClockWise() {
//    for (int i = 0; i < N; i++) {
//        for (int j = 0; j < N; j++) {
//        	SnakeFaceTemp[N - i - 1][i] = mat[i][j];
//        }
//    }
//}
void MatrixRotate() {
	// transpose matrix then reverse order of columns to rotate 90 degrees clockwise
	// to implement the snake head direction

}
void DrawFood(int x_coord, int y_coord, int type){
int color;

	    switch ((FoodType)type) {
	        case FOOD_REGULAR:
	            color = COLOR_FOOD_REGULAR;
	            break;
	        case FOOD_SPEED_BOOST:
	            color = COLOR_FOOD_SPEED_BOOST;
	            break;
	        case FOOD_SLOW_DOWN:
	            color = COLOR_FOOD_SLOW_DOWN;
	            break;
	        case FOOD_REVERSE:
	            color = COLOR_FOOD_REVERSE;
	            break;
	        case FOOD_SHRINK:
	            color = COLOR_FOOD_SHRINK;
	            break;
	        case FOOD_INVINCIBLE:
	            color = COLOR_FOOD_INVINCIBLE;
	            break;
	        case FOOD_BONUS:
	            color = COLOR_FOOD_BONUS;
	            break;

	        default:
	            color = 0x000000; // Black (default)
	            break;
	    }
		for(int y = y_coord; y<y_coord+20; y++) {
				for(int x = x_coord; x<x_coord+20; x++) {

						if(y == y_coord || y == y_coord + 19) // drawing an outline on each block
							image_buffer_pointer[y*1280+x] = 0x000000;
						else if(x == x_coord || x == x_coord + 19)
							image_buffer_pointer[y*1280+x] = 0x000000;
						else
							image_buffer_pointer[y*1280+x] = color;


		}
	}
}
void DrawBlock(int x_coord, int y_coord, bool remove, int color) {
	// I want to make DrawBlock not have buffer_pointer as a parameter
	// TODO: reorganize code
	// x = which column
	// y = which row
	// each block is 20 by 20 pixels
//	    	switch ((BlockType)type) {
//	    	case BLOCK_SNAKE:
//	    		color = 0x00FF00; // Green for snake
//	    		break;
//	    	case BLOCK_OBSTACLE:
//	    		color = 0x808080; // Gray for obstacles
//	    	    break;
//	    	case BLOCK_BACKGROUND:
//	    	     color = 0xFF00FF; // Magenta for background (erase)
//	    	      break;
//	    	 default:
//	    	     color = 0x000000; // Black (default)
//	    	      break;
//	    	}

	for(int y = y_coord; y<y_coord+20; y++) {
		for(int x = x_coord; x<x_coord+20; x++) {
			if(!remove){
				if(y == y_coord || y == y_coord + 19) // drawing an outline on each block
					image_buffer_pointer[y*1280+x] = 0x000000;
				else if(x == x_coord || x == x_coord + 19)
					image_buffer_pointer[y*1280+x] = 0x000000;
				else
					image_buffer_pointer[y*1280+x] = color; // color
			}
			else
				image_buffer_pointer[y*1280+x] = 0xFF00FF;
		}
	}
}

void DrawSnake(int size, int x, int y) {
	for(int i = 0; i < size; i++) { //size = length of snake
		DrawBlock(x+i*20, y, false, colors[currentcolor]);
		//usleep(50000);
	}
}

void Init_Map() {
	for(int y = 0; y<1024; y++) {
			for(int x = 0; x<1280; x++) {
				black_screen[y*1280 + x] = 0xFF00FF;
			}
		}

		// Drawing horizontal lines
		for(int y = 112; y<115; y++) {
				for(int x = 87; x < 1194; x++) {
					black_screen[y*1280 + x] = 0x000000;
				}
			}

		for(int y = 127; y<130; y++) {
			for(int x = 87; x < 1194; x++) {
				black_screen[y*1280 + x] = 0x000000;
			}
		}

		for(int y = 931; y<934; y++) {
			for(int x = 87; x < 1194; x++) {
				black_screen[y*1280 + x] = 0x000000;
			}
		}

		// Drawing vertical lines
		for(int y = 127; y<934; y++){
			for(int x = 87; x<90; x++){
				black_screen[y*1280 + x] = 0x000000;
			}
		}
		for(int y = 127; y<934; y++){
			for(int x = 1191; x< 1194; x++){
				black_screen[y*1280 + x] = 0x000000;
			}
		}
}

void Draw_Map() {
	memcpy(image_buffer_pointer, black_screen, NUM_BYTES_BUFFER);
}
