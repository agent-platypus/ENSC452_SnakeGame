
//Empty C++ Application
#include <stdio.h>
#include <sleep.h>
#include "xil_types.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "xgpio.h"
#include "xil_printf.h"
#include <cstdlib>
#include <cstring>
#include "graphics_driver.h"
#include "menu.h"
#include "xuartps.h"
#include "digits.h"
#include "xil_exception.h"
#include "xpseudo_asm.h"

// Function to check if a position is valid (not on the snake's body)
#define ARM1_STARTADR 0xFFFFFFF0
#define ARM1_BASEADDR 0x10080000
//#define COMM_VAL (*(volatile unsigned long *)(0xFFFF0000))
#define sev() __asm__("sev")
#include "xil_mmu.h"
#define INTC_DEVICE_ID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define BTNS_DEVICE_ID		XPAR_AXI_GPIO_1_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_1_IP2INTC_IRPT_INTR
#define BTN_INT 			XGPIO_IR_CH1_MASK
#define UART_BASEADDR XPAR_PS7_UART_1_BASEADDR
#define STARTING_LENGTH 10
#define BLOCK_SIZE 20
#define PLAYABLE_LEFT 90
#define PLAYABLE_RIGHT 1191
#define PLAYABLE_TOP 130
#define PLAYABLE_BOTTOM 931

#define NUM_BLOCKS_X ((PLAYABLE_RIGHT - PLAYABLE_LEFT) / BLOCK_SIZE)
#define NUM_BLOCKS_Y ((PLAYABLE_BOTTOM - PLAYABLE_TOP) / BLOCK_SIZE)
#define TIMER_DEFAULT 0xFF3CB9FF
#define IP_CORE_BASE_ADDR XPAR_LFSR_IP_0_S00_AXI_BASEADDR
#define SEED_REG_OFFSET          0x04
#define RANDOM_NUMBER_REG_OFFSET 0x00

// Define the base address and offset for the LFSR random number register
#define LFSR_BASE_ADDR XPAR_LFSR_IP_0_S00_AXI_BASEADDR // Replace with your LFSR's base address
#define LFSR_RANDOM_REG_OFFSET 0x00 // Offset for the random number register
#define MAX_OBSTACLES 10 // Maximum number of obstacles

bool isInvincible = false;
int invincible_duration = 0;
bool speed_boost_active = false; // Is the speed boost active?
int speed_boost_duration = 0; // Remaining duration of the speed boost (in timer interrupts)
Food food; // Declare a Food variable

XTmrCtr TimerInstancePtr;


XGpio BTNInst;
volatile bool TIMER_INTR_FLG;
XScuGic INTCInst;
static int btn_value;
int current_direction = 3;
int snakeBody_X[200], snakeBody_Y[200];
int x_coord;
int y_coord;
int SNAKE_LENGTH = 10;
bool GameOver = false;

int food_x, food_y; // Food position
extern int* colors;
extern int colorindex;
static void BTN_Intr_Handler(void *baseaddr_p);
void GameOver_loop();

enum StartMenuOption{
	Start = 1,
	Options = 2
};
enum OptionSelection{
	Volume = 0,
	Difficulty = 1,
	Snake_Color = 2
};

enum State{
	StartMenu = 0,
	GameStart = 1,
	GameOptions = 2
};

enum Difficulty{
	Easy = 1,
	Medium = 2,
	Hard = 3
};

int GameState = 0;
int StartOption = Start;
int OptionSelected = Volume;
int counter = 0;
int volume = 50;
int difficulty = Easy;
int score = 0;
typedef struct {
    int x; // X coordinate
    int y; // Y coordinate
} Obstacle;

Obstacle obstacles[MAX_OBSTACLES]; // Array to store obstacles
int num_obstacles = 0; // Number of obstacles spawned


// Shared memory structure

typedef struct {
    volatile unsigned long COMM_VAL; // COMM_VAL as part of the structure
    char filename[32]; // Filename of the audio file
    int loopMusic;
    int command;       // Command (e.g., play background music, play sound effect)
} __attribute__((packed)) audio_command_t; // Pack the structure to avoid padding

extern "C" {
    volatile audio_command_t *shared_memory = (audio_command_t *)0xFFFF0000; // Adjust address as needed
}

// Function to trigger background music
//extern "C"{
//void triggerBackgroundMusic(const char *filename) {
//    shared_memory -> loopMusic = 1; // Enable looping
//
//    printf("Attempting to play: %s\r\n", filename); // Debug print
//	strcpy((char *)shared_memory->filename, filename);
//    printf("Copied filename to shared memory: %s\r\n", shared_memory->filename); // Debug print
//   // shared_memory->command = 1; // Command to play background music
//    __sync_synchronize(); // Memory barrier to ensure data is read
//    shared_memory -> COMM_VAL = 1;
//    printf("Core 0: Raw filename bytes: ");
//                        for (int i = 0; i < sizeof(shared_memory->filename); i++) {
//                            printf("%02X ", (unsigned char)shared_memory->filename[i]);
//                        }
//                        printf("\r\n");
//    // Signal Core 1
//   // printf("Core 0: COMM_VAL set to 1\r\n"); // Debug print
//  //  printf("Core 0: Command value: %d\r\n", shared_memory->command); // Debug print
//
//}
//}

void triggerSoundFx(const char *filename) {

    __sync_synchronize(); // Memory barrier to ensure data is read
    printf("Attempting to play: %s\r\n", filename); // Debug print
    strcpy((char *)shared_memory->filename, filename);
    printf("Copied filename to shared memory: %s\r\n", shared_memory->filename); // Debug print
    shared_memory -> COMM_VAL = 1;
    //shared_memory -> command = 1;
}

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

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        int x, y;
        do {
            // Generate random block indices using the hardware LFSR
            x = lfsr_random() % NUM_BLOCKS_X;
            y = lfsr_random() % NUM_BLOCKS_Y;

            // Convert block indices to pixel coordinates
            x = PLAYABLE_LEFT + x * BLOCK_SIZE;
            y = PLAYABLE_TOP + y * BLOCK_SIZE;
        } while (!is_position_valid(x, y, snakeBody_X, snakeBody_Y, snake_length)); // Ensure the position is valid

        // Add the obstacle to the array
        obstacles[num_obstacles].x = x;
        obstacles[num_obstacles].y = y;
        num_obstacles++;

        // Draw the obstacle
        DrawBlock(x, y, false, 0x00808080);
    }
}

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
    // Generate random coordinates
    get_random_block(&food_x, &food_y, snakeBody_X, snakeBody_Y, snake_length);

    // Generate a random value and constrain it to 0-6
        uint16_t random_value = lfsr_random();
        food->type = (FoodType)(random_value % 7); // 7 types of food (0 to 6)
        food->lifetime = 150;

    // Draw the food with the corresponding color
    DrawFood(food_x, food_y, food->type);
}
// resetting the game: redrawing the map and resetting the snake coordinates
void restart_game(){
	speed_boost_active = false;
	speed_boost_duration = 0; // Remaining duration of the speed boost (in timer interrupts)
	isInvincible = false;
	invincible_duration = 0;
	XTmrCtr_SetResetValue(&TimerInstancePtr, 0, TIMER_DEFAULT);
	counter = 0;
	Draw_Map();
	score = 0;
	draw_score(score);
	x_coord = 410;
	y_coord = 530;
	SNAKE_LENGTH = STARTING_LENGTH;
	current_direction = 3;
	DrawBlock(x_coord, y_coord, false, colors[1]);
	for(int i = 0; i<STARTING_LENGTH; i++) {
		DrawBlock(x_coord+i*20, y_coord, false,colors[1]);
		snakeBody_X[i] = x_coord+i*20;
		snakeBody_Y[i] = y_coord;
	}

     spawn_food(&food, snakeBody_X, snakeBody_Y, SNAKE_LENGTH);

	 spawn_obstacles(snakeBody_X, snakeBody_Y, SNAKE_LENGTH);


}

bool soundFX;
bool collisionFX;
void update_block() {



		DrawBlock(snakeBody_X[SNAKE_LENGTH-1], snakeBody_Y[SNAKE_LENGTH-1], true, colors[colorindex]);
		int prevX = x_coord;
		int prevY = y_coord;
		//int prev2X, prev2Y;
		if(current_direction == 1) { //DOWN
			y_coord+=20;
		}
		else if(current_direction == 2) { //UP
			y_coord-=20;
		}
		else if(current_direction == 3) { //LEFT
			x_coord-=20;
		}
		else if(current_direction == 4) { //RIGHT
			x_coord+=20;
		}
		// body collision: check if the updated coordinate of the head has any intersection
				// with the body blocks
				// change the condition to a variable condition later
		for(int i = 0; i<SNAKE_LENGTH; i++) {
			if((x_coord == snakeBody_X[i] && y_coord == snakeBody_Y[i])) {
				DisplayGameover();
				GameOver = true;
				GameOver_loop();
				collisionFX = true;
				}
		}
		// Collision check for obstacles, has invincibility check
		 for (int i = 0; i < num_obstacles; i++) {
		        if (x_coord == obstacles[i].x && y_coord == obstacles[i].y) {
		            if (isInvincible) {
		                printf("Deleting obstacle at (%d, %d)\n", obstacles[i].x, obstacles[i].y); // Debug print
		                fflush(stdout); // Flush the output buffer

		                // Erase the obstacle
		                DrawBlock(obstacles[i].x, obstacles[i].y, true, 5);
		                // Remove the obstacle from the array
		                for (int j = i; j < num_obstacles - 1; j++) {
		                    obstacles[j] = obstacles[j + 1];
		                }
		                num_obstacles--;
		            } else {
		                // Handle normal collision (e.g., game over)
		            	DisplayGameover();
		                GameOver = true;
		                GameOver_loop();
		                collisionFX = true;
		            }
		        }
		 }

		// problem: when the snake runs into right and bottom side of the gamespace boundary
				// since each block coordinate is located at the top left of the block,
		if (y_coord > 929 || y_coord < 130 ||  x_coord > 1189 || x_coord < 90) {
//			if(y_coord > 929 || x_coord > 1189) {
//				DrawBlock(x_coord, y_coord, false);
//			}
			DisplayGameover();
			GameOver = true;
			GameOver_loop();
            collisionFX = true;

		}
		  // Check for collision with the food
		    if (x_coord == food_x && y_coord == food_y) {
		        printf("Food eaten: Type = %d\n", food.type); // Debug print
		    	        fflush(stdout); // Flush the output buffer
		    	  switch (food.type) {
		    	            case FOOD_REGULAR: // blue
		    	                // Increase snake length
		    	            	SNAKE_LENGTH++;
		    	            	clear_score(score);
		    	            	score++;
		    	            	draw_score(score);
		    	            	xil_printf("REGULAR FOOD AQCUIRED \n ");
		    	            	xil_printf("COUNTER: %d\n", counter);
		    	            	soundFX = true;
		    	                break;
		    	            case FOOD_SPEED_BOOST: // red
		    	            	xil_printf("SPEED FOOD AQCUIRED \n ");
		    	                // Increase speed (e.g., reduce timer delay)

		  		    	        XTmrCtr_SetResetValue(&TimerInstancePtr, 0, 0xFFD00000);
		    	            	speed_boost_active = true;
		    	            	speed_boost_duration = 50;
		    	            	soundFX = true;

		    	                break;
		    	            case FOOD_SLOW_DOWN: // light blue
		    	            	xil_printf("SLOW FOOD AQCUIRED \n ");
		  		    	        XTmrCtr_SetResetValue(&TimerInstancePtr, 0, 0xFBF0BDC0);
		  		    	        speed_boost_active = true;
		  		    	      	speed_boost_duration = 5;
		  		    	      	soundFX = true;

		    	                // Decrease speed (e.g., increase timer delay)
		    	                break;
		    	            case FOOD_REVERSE: // pink
		    	                // Reverse controls
		    	            	xil_printf("REVERSE FOOD AQCUIRED \n ");
		    	            	soundFX = true;

		    	                current_direction = 5 - current_direction; // Reverse direction
		    	                break;
		    	            case FOOD_SHRINK: // purple
		    	            	soundFX = true;
		    	            	xil_printf("SHRINK FOOD AQCUIRED \n ");
		    	            	counter --;
		    	            	xil_printf("COUNTER: %d\n", counter);

		    	                // Reduce snake length
		    	                break;
		    	            case FOOD_INVINCIBLE: // cyan
		    	            	soundFX = true;

		    	            	xil_printf("INVINCIBLE FOOD AQCUIRED \n ");
		    	            	isInvincible = true;
		    	            	invincible_duration = 50;
		    	                // Make snake invincible
		    	                break;
		    	            case FOOD_BONUS: // yellow
		    	            	SNAKE_LENGTH++;
		    	            	clear_score(score);
		    	            	score = score + 5;
		    	            	draw_score(score);
		    	            	xil_printf("BONUS FOOD AQCUIRED \n ");
		    	            	soundFX = true;

		    	                // Award extra points
		    	            	counter = counter + 5;
		    	            	xil_printf("COUNTER: %d", counter);
		    	                break;
		    	            default:
		    	            	xil_printf("DEFAULT CASE FOOD TYPE: %d \n", food.type);
		    	            	break;

		    	        }


		        // For now, just spawn new food
		        spawn_food(&food, snakeBody_X, snakeBody_Y, SNAKE_LENGTH);		    }



		    else {
		    		DrawBlock(x_coord, y_coord, false, colors[colorindex]);
		    		int tempX, tempY;
		    		tempX = snakeBody_X[0];
		    		tempY = snakeBody_Y[0];
		    		snakeBody_X[0] = prevX;
		    		snakeBody_Y[0] = prevY;

		    		for(int i = 1; i<SNAKE_LENGTH; i++) {
		    			int nextX = snakeBody_X[i];
		    			int nextY = snakeBody_Y[i];
		    			snakeBody_X[i] = tempX;
		    			snakeBody_Y[i] = tempY;
		    			tempX = nextX;
		    			tempY = nextY;
		    			}
		    		}

}


void change_direction(int direction) {
	// 1 = up, 2 = down, 3 = left, 4 = right
	// default left
	// changing direction is only valid for perpendicular directions
	// i.e. up -> left or up->right   up->down is not valid
	if(direction ==3  && current_direction ==1 ) {
		current_direction = 3;
		}
	else if(direction==4  && current_direction ==1 ) {
		current_direction = 4;
		}
	else if(direction==3  && current_direction ==2 ) {
		current_direction = 3;
		}
	else if(direction==4  && current_direction ==2 ) {
		current_direction = 4;
		}
	else if(direction==1  && current_direction ==3 ) {
		current_direction = 1;
		}
	else if(direction==2  && current_direction ==3 ) {
		current_direction = 2;
		}
	else if(direction==1  && current_direction ==4 ) {
		current_direction = 1;
		}
	else if(direction==2  && current_direction ==4 ) {
		current_direction = 2;
		}
}

void GameOver_loop() {
	//do something
	//send a message in UART
}
void Timer_InterruptHandler(XTmrCtr *data, u8 TmrCtrNumber)
{
	if(!GameOver)
		update_block();

	  if (soundFX){
		  triggerSoundFx("Food.wav");
		  soundFX = false;
	  }

	  if(collisionFX){
		  triggerSoundFx("Death.wav");
		  collisionFX = false;
	  }

//	  if(GameOver){
//		  triggerSoundFx("Game_Over.wav");
//
//	  }
	  if (speed_boost_active) {
			            speed_boost_duration--;
			          //  printf("DURATION: %d", speed_boost_duration);
			            if (speed_boost_duration <= 0) {
			                // Revert to normal speed
			                XTmrCtr_SetResetValue(&TimerInstancePtr, 0, TIMER_DEFAULT);
			                speed_boost_active = false;
			            }
			            }

	  if (food.lifetime > 0) {
	          food.lifetime--;
	          if (food.lifetime == 0) {
	              printf("Food expired at (%d, %d)\n", food_x, food_y); // Debug print
	              fflush(stdout); // Flush the output buffer

	              // Remove the food
	              DrawBlock(food_x, food_y, true, 5); // Erase the food block

	              // Spawn new food
	              spawn_food(&food, snakeBody_X, snakeBody_Y, 10);
	          }
	      }

	  if (isInvincible) {
	         invincible_duration--;
	         if (invincible_duration <= 0) {
	             printf("Invincibility expired\n"); // Debug print
	             fflush(stdout); // Flush the output buffer

	             // Revert to normal state
	             isInvincible = false;
	         }
	     }


	//xil_printf("COUNTER: %d\n", counter);

	XTmrCtr_Stop(data,TmrCtrNumber);
	XTmrCtr_Reset(data,TmrCtrNumber);
	//Update Stuff

	TIMER_INTR_FLG = true;
}

void CheckState(int btn_val) {
	switch (btn_value) {
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
						//	DrawBlock(711, 577, false, colors[colorindex]);
							//draw_score_60x60(1, 634, 429, 0x0000FF);// 627, 429	difficulty
							//draw_score_60x60(100, 609, 309, 0x0000FF);// 617, 309   volume
							DisplayDifficulty(difficulty);
							draw_volume(volume);
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
		    	//	DrawBlock(711, 577, false, colors[colorindex]);
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
		    			clear_volume(volume);
		    			volume++;
		    			if(volume > 100)
		    				volume = 100;
		    			draw_volume(volume);
		    			break;
		    		case Difficulty:
		    			if(difficulty == Medium) {
		    				difficulty = Easy;
		    			}
		    			else {
		    				difficulty = Medium;
		    			}
		    			DisplayDifficulty(difficulty);
		    			break;
		    		case Snake_Color:
		    			ToggleColor();
		    		//	DrawBlock(711, 577, false, colors[colorindex]);
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
		    		//DrawBlock(711, 577, false, colors[colorindex]);
		    	}
		    	// UP
		    	break;
		    default:
		        printf("Default case is Matched.");
		        break;
		    }
}



int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr){
	XGpio_InterruptEnable(&BTNInst, BTN_INT);
	XGpio_InterruptGlobalEnable(&BTNInst);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
	(Xil_ExceptionHandler) XScuGic_InterruptHandler,
	XScuGicInstancePtr);

	Xil_ExceptionEnable();
	return XST_SUCCESS;
}

/*INTERRUPT TUTORIAL FUNCTIONS */
void BTN_Intr_Handler(void *InstancePtr)
{
	// Disable GPIO interrupts
	XGpio_InterruptDisable(&BTNInst, BTN_INT);
	// Ignore additional button presses
	if ((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) !=
			BTN_INT) {
			return;
		}
	btn_value = XGpio_DiscreteRead(&BTNInst, 1);
//	TIMER_INTR_FLG = true;

	switch (btn_value) {
		case 1:
			if(GameOver) {
				restart_game();
				GameOver = false;
			}
			break;
	    case 2:
	    	change_direction(1); // UP
	        break;
	    case 4:
	    	change_direction(3); // LEFT
	        break;
	    case 8:
	    	change_direction(4);  // RIGHT
	        break;
	    case 16:
	    	change_direction(2); // DOWN
	    	break;
	    default:
	       // printf("Default case is Matched.");
	        break;
	    }

    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
}


int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr, XTmrCtr* TimerInstancePtr)
{
	XScuGic_Config *IntcConfig;
	int status;

	// Interrupt controller initialisation
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	if(NULL == IntcConfig) {
		return XST_FAILURE;
	}
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
	if(status != XST_SUCCESS)
		return XST_FAILURE;

	// Call to interrupt setup
	status = SetUpInterruptSystem(&INTCInst);
	if(status != XST_SUCCESS)
		return XST_FAILURE;

	// Connect GPIO interrupt to handler
	status = XScuGic_Connect(&INTCInst,
					  	  	 INTC_GPIO_INTERRUPT_ID,
					  	  	 (Xil_ExceptionHandler)BTN_Intr_Handler,
					  	  	 (void *)GpioInstancePtr);
	if(status != XST_SUCCESS)
		return XST_FAILURE;

	// Connect Timer interrupt to handler
	status = XScuGic_Connect(&INTCInst,
			61,
			(Xil_ExceptionHandler)XTmrCtr_InterruptHandler,
			(void *)TimerInstancePtr);
	if(status != XST_SUCCESS)
		return XST_FAILURE;
	// Enable GPIO interrupts interrupt
	XGpio_InterruptEnable(GpioInstancePtr, 1);
	XGpio_InterruptGlobalEnable(GpioInstancePtr);

	// Enable GPIO and timer interrupts in the controller
	XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);
	XScuGic_Enable(&INTCInst, 61);

	return XST_SUCCESS;
}


int main()
{
	memset((void *)shared_memory, 0, sizeof(audio_command_t)); // Initialize shared memory to zero
    printf("Shared memory initialized.\r\n");
	int xStatus;

	 shared_memory -> COMM_VAL = 0;
	 //Disable cache on OCM
	     // S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0
	     Xil_SetTlbAttributes(0xFFFF0000,0x14de2);

	     print("ARM0: writing start address for ARM1\n\r");
	     Xil_Out32(ARM1_STARTADR, ARM1_BASEADDR);
	     dmb(); //waits until write has finished

	     print("ARM0: sending the SEV to wake up ARM1\n\r");
	     sev();

	/*Setting up timer interrupt  */

			xStatus = XTmrCtr_Initialize(&TimerInstancePtr,XPAR_AXI_TIMER_0_DEVICE_ID);
			XTmrCtr_SetHandler(&TimerInstancePtr,
			(XTmrCtr_Handler)Timer_InterruptHandler,
			&TimerInstancePtr);

			//Reset Values
			XTmrCtr_SetResetValue(&TimerInstancePtr,
			0, //Change with generic value
			//0xFFF0BDC0);
			//0x23C34600);
			0xFF3CB9FF
			);
			//Interrupt Mode and Auto reload
//			XTmrCtr_SetOptions(&TimerInstancePtr,
//			XPAR_AXI_TIMER_0_DEVICE_ID,
//			(XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION ));
			XTmrCtr_SetOptions(&TimerInstancePtr,
								0,
								(XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION ));
			//xStatus=ScuGicInterrupt_Init(XPAR_PS7_SCUGIC_0_DEVICE_ID,&TimerInstancePtr);
		/*End of timer setup */

	xStatus = XGpio_Initialize(&BTNInst, BTNS_DEVICE_ID);
	if(xStatus != XST_SUCCESS)
		return XST_FAILURE;
	XGpio_SetDataDirection(&BTNInst, 1, 0xFF);

	xStatus = IntcInitFunction(INTC_DEVICE_ID, &BTNInst, &TimerInstancePtr);
	if(xStatus != XST_SUCCESS)
		return XST_FAILURE;


	/*Enable the interrupt for the device and then cause (simulate) an interrupt so the handlers will be called*/
	XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);
	XScuGic_SetPriorityTriggerType(&INTCInst, 61, 0xa0, 3);
	Init_Map();
	//StartScreenStartGame();

	restart_game();
    //usleep(10000000); // 100 ms delay


	//Draw_Map(image_buffer_pointer);
	Xil_DCacheDisable();
	/* initializing snake */
	// playspace dimensions 1100 by 800 pixels
	// starting playspace coordinate = 90, 130
	// Read the random number from the IP core's register


	while(1) {
		TIMER_INTR_FLG = false;
		XTmrCtr_Start(&TimerInstancePtr,0);
				while(TIMER_INTR_FLG == false){
				}
		TIMER_INTR_FLG = false;

	}
	return 0;
}

