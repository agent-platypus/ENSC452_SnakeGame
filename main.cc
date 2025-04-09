
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
#define ARM1_STARTADR 0xFFFFFFF0
#define ARM1_BASEADDR 0x10080000
#define sev() __asm__("sev")
#include "xil_mmu.h"
#define INTC_DEVICE_ID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define BTNS_DEVICE_ID		XPAR_AXI_GPIO_1_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_1_IP2INTC_IRPT_INTR
#define BTN_INT 			XGPIO_IR_CH1_MASK
#define UART_BASEADDR XPAR_PS7_UART_1_BASEADDR
#define TIMER_DEFAULT 0xFF3CB9FF
#define SPEED_EASY    0xFF3CB9FF  // Original/default speed 4282169855 4291821568
#define SPEED_MEDIUM  SPEED_EASY * 1.0005  //
#define SPEED_HARD    SPEED_EASY  * 1.001//
#define STARTING_LENGTH 10

XGpio BTNInst;
volatile bool TIMER_INTR_FLG;
XScuGic INTCInst;
static int btn_value;

static void BTN_Intr_Handler(void *baseaddr_p);
int max_score;
u32 current_speed;
XTmrCtr TimerInstancePtr;


// Function to read current timer speed
u32 GetTimerSpeed(XTmrCtr *InstancePtr) {
    return XTmrCtr_ReadReg(InstancePtr->BaseAddress, 0, XTC_TLR_OFFSET);
}

// Function to interact with core 1 and play soundFx when needed
void triggerSoundFx(const char *filename) {

    printf("Attempting to play: %s\r\n", filename); // Debug print
    // copies filename of audio that needs to be played and writes to shared memory
    strcpy((char *)shared_memory->filename, filename);

    printf("Copied filename to shared memory: %s\r\n", shared_memory->filename); // Debug print
    // Ensures communication between the two is locked to the comm_val so no memory errors happen
    shared_memory -> COMM_VAL = 1;
}

// Initialize everything and restart game state
void restart_game(){
	soundFX = false;
	speed_boost_active = false;
	speed_boost_duration = 0;
	isInvincible = false;
	invincible_duration = 0;

	// Set speed and max score based on difficulty
	switch(difficulty) {
	        case Easy:
	            XTmrCtr_SetResetValue(&TimerInstancePtr, 0, SPEED_EASY);
	            max_score = 10;
	            break;
	        case Medium:
	            XTmrCtr_SetResetValue(&TimerInstancePtr, 0, SPEED_MEDIUM);
	            max_score = 20;
	            break;
	        case Hard:
	            XTmrCtr_SetResetValue(&TimerInstancePtr, 0, SPEED_HARD);
	            max_score = 30;
	            break;
	    }
	// Get current speed because difficulty would change default timer. For later use
	current_speed = GetTimerSpeed(&TimerInstancePtr);
	Draw_Map();
	score = 0;
	draw_score(score);
	x_coord = 410;
	y_coord = 530;
	SNAKE_LENGTH = STARTING_LENGTH;
	current_direction = 3;
	DrawSnakeHead(x_coord, y_coord, false, colors[colorindex], true);

	for(int i = 0; i<STARTING_LENGTH; i++) {
		DrawBlock(x_coord+i*20, y_coord, false,colors[colorindex]);
		snakeBody_X[i] = x_coord+i*20;
		snakeBody_Y[i] = y_coord;
	}


     spawn_food(&food, snakeBody_X, snakeBody_Y, SNAKE_LENGTH);
	 spawn_obstacles(snakeBody_X, snakeBody_Y, SNAKE_LENGTH);


}

// Game logic when running in background, must be run in main so we can use timer interrupts properly
void update_block() {

	  if (score >= max_score ){
				    	 		DisplayVictory();
				    	 		GameOver = true;
				    	 		triggerSoundFx("win.wav");

				    	 				    	  	  }

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
			// Get current speed because difficulty would change default time. For later use
        	u32 current_speed = GetTimerSpeed(&TimerInstancePtr);

				        printf("Food eaten: Type = %d\n", food.type); // Debug print
				    	  switch (food.type) {
				    	            case FOOD_REGULAR: // Represented by a blueberry, just increases length and score
				    	                // Increase snake length
				    	            	SNAKE_LENGTH++;
				    	            	score++;
				    	            	clear_score(score-1);
				    	            	draw_score(score);
				    	            	soundFX = true;
				    	                break;
				    	            case FOOD_SPEED_BOOST: // Apple, gives speed boost for a set duration
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
				    	            case FOOD_SLOW_DOWN: // Eggplant, slow down for a set duration
				    	            	SNAKE_LENGTH++;
				    	            	xil_printf("SLOW FOOD AQCUIRED \n ");
				    	            	score++;
				    	            	clear_score(score-1);
				    	            	draw_score(score);
				    	            	XTmrCtr_SetResetValue(&TimerInstancePtr, 0, current_speed * .99); // Slow down
				  		    	        speed_boost_active = true;
				  		    	      	speed_boost_duration = 5;
				  		    	      	soundFX = true;
				  		    	        xil_printf("Slow Down! New speed: 0x%08X\n", GetTimerSpeed(&TimerInstancePtr));

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
				    	            case FOOD_INVINCIBLE: // Banana, turn invincible for a certain duration, head turns white and can go through blocks
				    	            	SNAKE_LENGTH++;
				    	            	soundFX = true;
				    	            	score++;
				    	            	clear_score(score-1);
				    	            	draw_score(score);
				    	            	xil_printf("INVINCIBLE FOOD AQCUIRED \n ");
				    	            	isInvincible = true;
				    	            	invincible_duration = 50;

				    	                break;
				    	            case FOOD_BONUS: // Represented by orange, gives 5 extra points, length only by 1

				    	            	SNAKE_LENGTH++;
				    	            	clear_score(score);
				    	            	score = score + 5;
				    	            	draw_score(score);
				    	            	xil_printf("BONUS FOOD AQCUIRED \n ");
				    	            	soundFX = true;

				    	                break;
				    	            default:
				    	            	xil_printf("DEFAULT CASE FOOD TYPE: %d \n", food.type);
				    	            	break;

				    	        }
				    	    //calls spawn_food after it has been eaten to spawn new food
				    		spawn_food(&food, snakeBody_X, snakeBody_Y, SNAKE_LENGTH);



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
				                triggerSoundFx("Death.wav");
				            }
				        }
				 }
		// body collision: check if the updated coordinate of the head has any intersection
				// with the body blocks
		for(int i = 0; i<SNAKE_LENGTH; i++) {
			if((x_coord == snakeBody_X[i] && y_coord == snakeBody_Y[i])) {
				DisplayGameover();
				GameOver = true;
				DrawSnakeHead(prevX, prevY, false, colors[colorindex], false);
				BodyCollisionDetected = true;
                triggerSoundFx("Death.wav");

				}
		}


		// collision checking to check if touching border of the game
		if (y_coord > 929 || y_coord < 130 ||  x_coord > 1189 || x_coord < 90) {

			DisplayGameover();
			DrawSnakeHead(prevX, prevY, false, colors[colorindex], false);
			GameOver = true;
			BodyCollisionDetected = true;
            triggerSoundFx("Death.wav");


		}

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

				// updating the coordinate of every valid snake body block at every
			    	// frame update
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



void Timer_InterruptHandler(XTmrCtr *data, u8 TmrCtrNumber)
{
	if(!GameOver && GameState == GameStart)
				update_block();

	  // soundFX bool to trigger soundfx
	  if (soundFX && GameState == GameStart){
  	  	  triggerSoundFx("Food.wav");
		  soundFX = false;
	  }


	  // duration goes down with timer, reverts back after duration is over
	  if (speed_boost_active) {
			            speed_boost_duration--;
			            if (speed_boost_duration <= 0) {
			                // Revert to previously set speed
			                XTmrCtr_SetResetValue(&TimerInstancePtr, 0, current_speed);
			                speed_boost_active = false;
			            }
			            }

	  // Fodd will despawn after certain duration
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

	  // Checks if Invincible is true, then duration decreases until 0
	  if (isInvincible) {
	         invincible_duration--;
	         if (invincible_duration <= 0) {


	             printf("Invincibility expired\n"); // Debug print
	             fflush(stdout); // Flush the output buffer

	             // Revert to normal state
	             isInvincible = false;
	         }
	     }



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
	CheckState(btn_value);

	if(GameState == GameStart){  // CHANGED PLACEMENT OF CONDITION
	switch (btn_value) {
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
			XTmrCtr_SetResetValue(&TimerInstancePtr, 0, 0xFF3CB9FF);
			//Interrupt Mode and Auto reload
			XTmrCtr_SetOptions(&TimerInstancePtr,
								0,
								(XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION ));

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




	Xil_DCacheDisable();

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

