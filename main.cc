
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
#include "game_logic.h"
#define BLOCK_SIZE 20

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
#define TIMER_DEFAULT 0xFF3CB9FF
#define SPEED_EASY    0xFF3CB9FF  // Original/default speed 4282169855 4291821568
#define SPEED_MEDIUM  SPEED_EASY * 1.0005  // 20% faster
#define SPEED_HARD    SPEED_EASY  * 1.001// 40% faster
#define STARTING_LENGTH 10

XGpio BTNInst;
volatile bool TIMER_INTR_FLG;
XScuGic INTCInst;
static int btn_value;

static void BTN_Intr_Handler(void *baseaddr_p);
void GameOver_loop();


XTmrCtr TimerInstancePtr;

// Shared memory structure

//typedef struct {
//    volatile unsigned long COMM_VAL; // COMM_VAL as part of the structure
//    char filename[32]; // Filename of the audio file
//    int command;       // Command (e.g., play background music, play sound effect)
//    volatile int theVolume;
//} __attribute__((packed)) audio_command_t; // Pack the structure to avoid padding
//
//extern "C" {
//    volatile audio_command_t *shared_memory = (audio_command_t *)0xFFFF0000; // Adjust address as needed
//}

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

u32 GetTimerSpeed(XTmrCtr *InstancePtr) {
    return XTmrCtr_ReadReg(InstancePtr->BaseAddress, 0, XTC_TLR_OFFSET);
}

void triggerSoundFx(const char *filename) {

    __sync_synchronize(); // Memory barrier to ensure data is read
    printf("Attempting to play: %s\r\n", filename); // Debug print
    strcpy((char *)shared_memory->filename, filename);
  	//printf("VOLUME: %d \n", shared_memory -> theVolume);

    printf("Copied filename to shared memory: %s\r\n", shared_memory->filename); // Debug print
    shared_memory -> COMM_VAL = 1;
    //shared_memory -> command = 1;
}

void restart_game(){

	speed_boost_active = false;
	speed_boost_duration = 0; // Remaining duration of the speed boost (in timer interrupts)
	isInvincible = false;
	invincible_duration = 0;
	  // Set speed based on difficulty
	    switch(difficulty) {
	        case Easy:
	            XTmrCtr_SetResetValue(&TimerInstancePtr, 0, SPEED_EASY);
	            break;
	        case Medium:
	            XTmrCtr_SetResetValue(&TimerInstancePtr, 0, SPEED_MEDIUM);
	            break;
	        case Hard:
	            XTmrCtr_SetResetValue(&TimerInstancePtr, 0, SPEED_HARD);
	            break;
	    }
	counter = 0;
	Draw_Map();
	score = 0;
	draw_score(score);
	x_coord = 410;
	y_coord = 530;
	SNAKE_LENGTH = STARTING_LENGTH;
	current_direction = 3;
	//DrawBlock(x_coord, y_coord, false, colors[colorindex]);
	DrawSnakeHead(x_coord, y_coord, false, colors[colorindex], true);

	for(int i = 0; i<STARTING_LENGTH; i++) {
		DrawBlock(x_coord+i*20, y_coord, false,colors[colorindex]);
		snakeBody_X[i] = x_coord+i*20;
		snakeBody_Y[i] = y_coord;
	}

     spawn_food(&food, snakeBody_X, snakeBody_Y, SNAKE_LENGTH);

	 spawn_obstacles(snakeBody_X, snakeBody_Y, SNAKE_LENGTH);


}
void update_block() {


		bool BodyCollisionDetected = false;

		DrawBlock(snakeBody_X[SNAKE_LENGTH-1], snakeBody_Y[SNAKE_LENGTH-1], true, colors[colorindex]);
		DrawBlock(x_coord, y_coord, false, colors[colorindex]);

		int prevX = x_coord;
		int prevY = y_coord;
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

		// Food collision check, different types of food, all give SNAKE_LENGTH++ and score++ (exceptions aswell)
		if (x_coord == food_x && y_coord == food_y) {
        	u32 current_speed = GetTimerSpeed(&TimerInstancePtr);

				        printf("Food eaten: Type = %d\n", food.type); // Debug print
				    	        fflush(stdout); // Flush the output buffer
				    	  switch (food.type) {
				    	            case FOOD_REGULAR: // blue
				    	                // Increase snake length
				    	            	SNAKE_LENGTH++;
				    	            	score++;
				    	            	clear_score(score-1);
				    	            	draw_score(score);
				    	            	soundFX = true;
				    	                break;
				    	            case FOOD_SPEED_BOOST: // red
				    	            	SNAKE_LENGTH++;
				    	            	xil_printf("SPEED FOOD AQCUIRED \n ");
				    	            	score++;
				    	            	clear_score(score-1);
				    	            	draw_score(score);
				    	            	XTmrCtr_SetResetValue(&TimerInstancePtr, 0, current_speed * 1.001); // Speed boost
				    	            	speed_boost_active = true;
				    	            	speed_boost_duration = 50;
				    	            	soundFX = true;
				    	                xil_printf("Speed Boost! New speed: 0x%08X\n", GetTimerSpeed(&TimerInstancePtr));

				    	                break;
				    	            case FOOD_SLOW_DOWN: // light blue
				    	            	SNAKE_LENGTH++;
				    	            	xil_printf("SLOW FOOD AQCUIRED \n ");
				  		    	        //XTmrCtr_SetResetValue(&TimerInstancePtr, 0, 0xFBF0BDC0);
				    	            	score++;
				    	            	clear_score(score-1);
				    	            	draw_score(score);
				    	            	XTmrCtr_SetResetValue(&TimerInstancePtr, 0, current_speed * .99); // Slow down
				  		    	        speed_boost_active = true;
				  		    	      	speed_boost_duration = 5;
				  		    	      	soundFX = true;
				  		    	        xil_printf("Slow Down! New speed: 0x%08X\n", GetTimerSpeed(&TimerInstancePtr));

				    	                // Decrease speed (e.g., increase timer delay)
				    	                break;
				    	            case FOOD_REVERSE: // pink, currently not in use was meant for 2 player mode if implemented
				    	                // Reverse controls
				    	            	xil_printf("REVERSE FOOD AQCUIRED \n ");
				    	            	soundFX = true;

				    	                current_direction = 5 - current_direction; // Reverse direction
				    	                break;
				    	            case FOOD_SHRINK: // purple, currently not in use was meant for 2 player mode if implemented
				    	            	soundFX = true;
				    	            	xil_printf("SHRINK FOOD AQCUIRED \n ");
				    	            	counter --;
				    	            	xil_printf("COUNTER: %d\n", counter);

				    	                // Reduce snake length
				    	                break;
				    	            case FOOD_INVINCIBLE: // cyan, turn invincible for a certain duration, can go through blocks
				    	            	SNAKE_LENGTH++;
				    	            	soundFX = true;
				    	            	score++;
				    	            	clear_score(score-1);
				    	            	draw_score(score);
				    	            	xil_printf("INVINCIBLE FOOD AQCUIRED \n ");
				    	            	isInvincible = true;
				    	            	invincible_duration = 50;
				    	                // Make snake invincible
				    	                break;
				    	            case FOOD_BONUS: // yellow, extra points
				    	            	SNAKE_LENGTH++;
				    	            	clear_score(score);
				    	            	score = score + 5;
				    	            	draw_score(score);
				    	            	xil_printf("BONUS FOOD AQCUIRED \n ");
				    	            	soundFX = true;
				    	            	counter = counter + 5;
				    	            	xil_printf("COUNTER: %d", counter);
				    	                break;
				    	            default:
				    	            	xil_printf("DEFAULT CASE FOOD TYPE: %d \n", food.type);
				    	            	break;

				    	        }

				    		spawn_food(&food, snakeBody_X, snakeBody_Y, SNAKE_LENGTH);


				        // For now, just spawn new food
				    }
		// Collision check for obstacles, has invincibility check

				 for (int i = 0; i < num_obstacles; i++) {
					  // Check if snake head overlaps obstacle (20x20 block)
					    if (x_coord < obstacles[i].x + BLOCK_SIZE &&
					        x_coord + BLOCK_SIZE > obstacles[i].x &&
					        y_coord < obstacles[i].y + BLOCK_SIZE &&
					        y_coord + BLOCK_SIZE > obstacles[i].y) {
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
				                i--;
				            } else {
				                // Handle normal collision (e.g., game over)
				    			DrawSnakeHead(prevX, prevY, false, colors[colorindex], false);

				            	DisplayGameover();
				                GameOver = true;
				                BodyCollisionDetected = true;
				                collisionFX = true;
				            }
				        }
				 }
		// body collision: check if the updated coordinate of the head has any intersection
				// with the body blocks
				// change the condition to a variable condition later
		for(int i = 0; i<SNAKE_LENGTH; i++) {
			if((x_coord == snakeBody_X[i] && y_coord == snakeBody_Y[i])) {
				DisplayGameover();
				GameOver = true;
				DrawSnakeHead(prevX, prevY, false, colors[colorindex], false);
				BodyCollisionDetected = true;
				collisionFX = true;
				}
		}


		// problem: when the snake runs into right and bottom side of the gamespace boundary
				// since each block coordinate is located at the top left of the block,
		if (y_coord > 929 || y_coord < 130 ||  x_coord > 1189 || x_coord < 90) {
//			if(y_coord > 929 || x_coord > 1189) {
//				DrawBlock(x_coord, y_coord, false);
//			}

			DisplayGameover();
			DrawSnakeHead(prevX, prevY, false, colors[colorindex], false);
			GameOver = true;
            collisionFX = true;
			BodyCollisionDetected = true;

		}
		  // Check for collision with the food






		    else
		    {
		    	if(!BodyCollisionDetected) {
		    					DrawSnakeHead(x_coord, y_coord, false, colors[colorindex], true);
		    				}

				if (isInvincible){
					DrawSnakeHead(x_coord, y_coord, false, 0xFFFFFF, true);
				//	DrawBlock(x_coord, y_coord, false, 0xFFFFFF);
				}
				else{
					DrawBlock(snakeBody_X[0], snakeBody_Y[0], false, colors[colorindex]);
				}


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


void GameOver_loop() {
	//do something
	//send a message in UART
}
void Timer_InterruptHandler(XTmrCtr *data, u8 TmrCtrNumber)
{
	if(!GameOver && GameState == GameStart)
				update_block();

	  if (soundFX && GameState == GameStart){
		  triggerSoundFx("Food.wav");
		  soundFX = false;
	  }

	  if(collisionFX && GameState == GameStart){
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

	  if (food.lifetime > 0 && GameState == GameStart && !GameOver) {
	          food.lifetime--;
	          if (food.lifetime == 0) {
	              printf("Food expired at (%d, %d)\n", food_x, food_y); // Debug print
	              fflush(stdout); // Flush the output buffer

	              // Remove the food
	              DrawBlock(food_x, food_y, true, 5); // Erase the food block

	              // Spawn new food
	              spawn_food(&food, snakeBody_X, snakeBody_Y, SNAKE_LENGTH);
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
	CheckState(btn_value);

	switch (btn_value)
	if(GameState == GameStart) {{
		case 1:
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
	 //shared_memory -> theVolume = 2;
   	//printf("VOLUME: %d \n", shared_memory -> theVolume);

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
	StartScreenStartGame();

	//restart_game();
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

