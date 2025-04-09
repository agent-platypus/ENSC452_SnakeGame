#include "game_logic.h"
#include "graphics_driver.h"
#include "menu.h"
#include "digits.h"
#include "xil_printf.h"
#include "cstdio"
#include "xil_types.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "xgpio.h"
#include <stdint.h>
#include "xil_types.h"

#define STARTING_LENGTH 10
// Define the base address and offset for the LFSR random number register
#define LFSR_BASE_ADDR XPAR_LFSR_IP_0_S00_AXI_BASEADDR // Replace with your LFSR's base address
#define LFSR_RANDOM_REG_OFFSET 0x00 // Offset for the random number register
#define MAX_OBSTACLES 30 // Maximum number of obstacles
#define PLAYABLE_LEFT 90
#define PLAYABLE_RIGHT 1191
#define PLAYABLE_TOP 130
#define PLAYABLE_BOTTOM 931
#define BLOCK_SIZE 20

#define NUM_BLOCKS_X ((PLAYABLE_RIGHT - PLAYABLE_LEFT) / BLOCK_SIZE)
#define NUM_BLOCKS_Y ((PLAYABLE_BOTTOM - PLAYABLE_TOP) / BLOCK_SIZE)
#define TIMER_DEFAULT 0xFF3CB9FF
// Timer reload values (lower = faster)
#define SPEED_EASY    0xFF3CB9FF  // Original/default speed 4282169855 4291821568
#define SPEED_MEDIUM  SPEED_EASY * 1.0005  // 20% faster
#define SPEED_HARD    SPEED_EASY  * 1.001// 40% faster
#define IP_CORE_BASE_ADDR XPAR_LFSR_IP_0_S00_AXI_BASEADDR
#define SEED_REG_OFFSET          0x04
#define RANDOM_NUMBER_REG_OFFSET 0x00

//Shared memory struct
volatile audio_command_t *shared_memory = (audio_command_t *)0xFFFF0000; // Adjust address as needed

Food food;

//1 = up, 2 = down, 3 = left, 4 = right
enum Direction {
	up = 1,
	down = 2,
	left = 3,
	right = 4
};
int current_direction = 3;
int SNAKE_LENGTH = 10;
int x_coord;
int y_coord;
int counter;


int GameState = 0;
int StartOption = Start;
int OptionSelected = Volume;
int difficulty = Easy;
int score = 0;
bool GameOver;
bool isInvincible;
int invincible_duration;
bool speed_boost_active;
int speed_boost_duration;
bool soundFX;
bool collisionFX;
int snakeBody_X[200], snakeBody_Y[200];
int food_x, food_y; // Food position


int num_obstacles = 0; // Number of obstacles spawned


const Obstacle EASY_OBSTACLES[] = {
		// Vertical guides (1 block from walls)
		    {110, 150}, {110, 350}, {110, 550}, {110, 750},  // Left
		    {1150, 150}, {1150, 350}, {1150, 550}, {1150, 750},  // Right

			// Top obstacles (y = 130 + n*20)
			{210, 150}, {410, 150}, {610, 150}, {810, 150}, {1010, 150},  // Top
			 // Bottom obstacles (y = 930 - 20 = 910)
			{210, 890}, {410, 890}, {610, 890}, {810, 890}, {1010, 890},  // Bottom

		    // Central safe zone markers
		    {630, 530}  // Dead center

};

Obstacle obstacles[MAX_OBSTACLES]; // Array to store obstacles

const int NUM_EASY_OBSTACLES = sizeof(EASY_OBSTACLES)/sizeof(EASY_OBSTACLES[0]);


// Function to generate a random number using the hardware LFSR
uint16_t lfsr_random() {
    uint32_t random_value = Xil_In32(LFSR_BASE_ADDR + LFSR_RANDOM_REG_OFFSET);
    return (uint16_t)(random_value & 0xFFFF); // Use lower 16 bits
}



// Function to generate a random block index within the playable area
int is_position_valid(int x, int y, int snakeBody_X[], int snakeBody_Y[], int snake_length) {
    for (int i = 0; i < snake_length; i++) {
    	if (snakeBody_X[i] == x && snakeBody_Y[i] == y) {
    		return 0; // Position is invalid
        }
    }
    // Check for collisions with obstacles
       for (int i = 0; i < num_obstacles; i++) {
           if (obstacles[i].x == x && obstacles[i].y == y) {
               return 0; // Position is invalid
           }
       }
    return 1; // Position is valid
}

void spawn_obstacles(int snakeBody_X[], int snakeBody_Y[], int snake_length) {
    num_obstacles = 0; // Reset obstacle count
    if (difficulty == Easy || difficulty == Medium) {
           // Fixed obstacles for easy mode
           for (int i = 0; i < NUM_EASY_OBSTACLES; i++) {
                   obstacles[num_obstacles] = EASY_OBSTACLES[i];
                   DrawBlock(obstacles[num_obstacles].x, obstacles[num_obstacles].y, false, 0x00808080);
                   num_obstacles++;
               }
    	}
    else {
    	for (int j = 0; j < MAX_OBSTACLES; j++) {
    		int x, y;
			do {
				// Generate random block indices using the hardware LFSR
				x = lfsr_random() % NUM_BLOCKS_X;
				y = lfsr_random() % NUM_BLOCKS_Y;

				// Convert block indices to pixel coordinates
				x = PLAYABLE_LEFT + x * BLOCK_SIZE;
				y = PLAYABLE_TOP + y * BLOCK_SIZE;
			} while (!is_position_valid(x, y, snakeBody_X, snakeBody_Y, snake_length)); // Ensure the position is valid
			if (num_obstacles >= MAX_OBSTACLES) {
						    xil_printf("Warning: Too many obstacles!\n");
						    return;
						}
			// spawns obstacles until we reach max
			if (num_obstacles < MAX_OBSTACLES) {
			            obstacles[num_obstacles].x = x;
			            obstacles[num_obstacles].y = y;
			            DrawBlock(x, y, false, 0x00808080);
			            num_obstacles++;

			        }

		}
    }
}

// Used to get random position for food blocks
void get_random_block(int* block_x, int* block_y, int snakeBody_X[], int snakeBody_Y[], int snake_length) {
    int x, y;
    do {
        // Generate random block indices using the hardware LFSR
        x = lfsr_random() % NUM_BLOCKS_X;
        y = lfsr_random() % NUM_BLOCKS_Y;

        // Convert block indices to pixel coordinates
        *block_x = PLAYABLE_LEFT + x * BLOCK_SIZE;
        *block_y = PLAYABLE_TOP + y * BLOCK_SIZE;
    } while (!is_position_valid(*block_x, *block_y, snakeBody_X, snakeBody_Y, snake_length)); // Ensure the position is valid
}

void spawn_food(Food* food, int snakeBody_X[], int snakeBody_Y[], int snake_length) {
    // Generate random coordinates, passes through food coordinates
    	get_random_block(&food_x, &food_y, snakeBody_X, snakeBody_Y, snake_length);

    	// Generate a random value and constrain it to 0-4
        uint16_t random_value = lfsr_random();
        food->type = (FoodType)(random_value % 5); // 5 types of food (0 to 4)
        food->lifetime = 150; // how long it lasts

    // Draw the food with the corresponding color
    DrawFood(food_x, food_y, food->type);
}


void change_direction(int direction) {
	// 1 = up, 2 = down, 3 = left, 4 = right
	// default left
	// changing direction is only valid for perpendicular directions
	// i.e. up -> left or up->right   up->down is not valid
	if(direction == left  && (current_direction ==up || current_direction == down) ) {
		current_direction = 3;
		}
	else if(direction==right  && (current_direction ==up || current_direction == down)) {
		current_direction = 4;
		}
	else if(direction==up  && (current_direction ==left || current_direction == right)) {
		current_direction = 1;
		}
	else if(direction==down  && (current_direction ==left || current_direction ==right)) {
		current_direction = 2;
		}
}

void CheckState(int btn_val) {
	switch (btn_val) {
			case 1: // CENTER
				if(GameState == StartMenu) {
						if(StartOption == Start) {
							GameState = GameStart;
							restart_game();
						}
						else if (StartOption == Options) {
							GameState = GameOptions;
							OptionScreen();
							OutlineOption(OptionSelected, false);
							DrawBlock(711, 577, false, colors[colorindex]);
							//draw_score_60x60(1, 634, 429, 0x0000FF);// 627, 429	difficulty
							//draw_score_60x60(100, 609, 309, 0x0000FF);// 617, 309   volume
							DisplayDifficulty(difficulty);
							draw_volume(shared_memory -> theVolume);
						}
				}
				if(GameOver) {
					restart_game();
					GameOver = false;
				}
				break;
		    case 2:
		    	if(GameState == StartMenu) {
		    		StartOption = Options;
		    		StartMenuToggle(StartOption);
		    	}
		    	if(GameState == GameOptions) {
		    		OutlineOption(OptionSelected, true);
		    		OptionSelected++;
		    		OptionSelected = OptionSelected%3;
		    		OutlineOption(OptionSelected, false);
		    		DrawBlock(711, 577, false, colors[colorindex]);
		    	} // DOWN
		        break;
		    case 4:
		    	if(GameState == GameOptions) {
		    		GameState = StartMenu;
		    		StartMenuToggle(StartOption);
		    	} // LEFT
		    	if(GameOver) { // PSEUDO QUIT FUNCTION
		    		GameState = StartMenu;
		    		StartScreenStartGame();
		    		GameOver = false;
		    	}
		        break;
		    case 8:
		    	// RIGHT
		    	if(GameState == GameOptions){
		    		switch(OptionSelected) {
		    		case Volume:
		    			clear_volume(shared_memory -> theVolume);
		    			shared_memory -> theVolume++;
		    			if(shared_memory -> theVolume > 3)
		    				shared_memory -> theVolume = 0;
		    			draw_volume(shared_memory -> theVolume);
		    			break;
		    		case Difficulty:
		    			difficulty++;
		    			difficulty = difficulty%3;
		    			DisplayDifficulty(difficulty);
		    			break;
		    		case Snake_Color:
		    			ToggleColor();
		    			DrawBlock(711, 577, false, colors[colorindex]);
		    			break;
		    		default:
		    			break;
		    		}
		    	}
		    	printf("RIGHT CARIMBA\n");
		        break;
		    case 16:
		    	if(GameState == StartMenu) {
		    		StartOption = Start;
		    		StartMenuToggle(StartOption);
		    	}
		    	if(GameState == GameOptions) {
		    		OutlineOption(OptionSelected, true);
		    		OptionSelected--;
		    		OptionSelected = OptionSelected % 3;
		    		 if (OptionSelected < 0) {
		    		      OptionSelected += 3;
		    		 }
		    		OutlineOption(OptionSelected, false);
		    		DrawBlock(711, 577, false, colors[colorindex]);
		    	}
		    	// UP
		    	break;
		    default:
		        printf("Default case is Matched.");
		        break;
		    }
}
