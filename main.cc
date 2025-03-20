
//Empty C++ Application
#include <stdio.h>
#include "xil_types.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xiicps.h"
#include "xscugic.h"
#include "xgpio.h"
#include "xil_printf.h"
#include <cstdlib>
#include <cstring>
#include "graphics_driver.h"
#include "xuartps.h"

#define INTC_DEVICE_ID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define BTNS_DEVICE_ID		XPAR_AXI_GPIO_1_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_1_IP2INTC_IRPT_INTR
#define BTN_INT 			XGPIO_IR_CH1_MASK
#define UART_BASEADDR XPAR_PS7_UART_1_BASEADDR
#define STARTING_LENGTH 10

enum State{
	StartMenuState = 0,
	StartGameState = 1,
	OptionState = 2,
	PauseState = 3
};

XGpio BTNInst;
volatile bool TIMER_INTR_FLG;
XScuGic INTCInst;
static int btn_value;
int current_direction = 3;
int GameState = StartMenuState;
int SNAKE_LENGTH = 10;
int snakeBody_X[200], snakeBody_Y[200];
int x_coord;
int y_coord;
bool GameOver = false;
int fruitX= 110;
int fruitY= 230;

static void BTN_Intr_Handler(void *baseaddr_p);
void GameOver_loop();

enum Direction{
	DOWN = 1,
	UP = 2,
	LEFT = 3,
	RIGHT = 4
};

enum State{
	StartMenuState = 0,
	StartGameState = 1,
	OptionState = 2,
	PauseState = 3
};

// resetting the game: redrawing the map and resetting the snake coordinates
void restart_game(){
	Draw_Map();
	x_coord = 410;
	y_coord = 530;
	SNAKE_LENGTH = STARTING_LENGTH;
	fruitX = 110;
	fruitY = 230;
	current_direction = 3;
	DrawFruit(fruitX, fruitY);
	DrawBlock(x_coord, y_coord, false);
	for(int i = 0; i<STARTING_LENGTH; i++) {
		DrawBlock(x_coord+i*20, y_coord, false);
		snakeBody_X[i] = x_coord+i*20;
		snakeBody_Y[i] = y_coord;
	}
}

void update_block() {
		DrawBlock(snakeBody_X[SNAKE_LENGTH-1], snakeBody_Y[SNAKE_LENGTH-1], true);
		int prevX = x_coord;
		int prevY = y_coord;
		if(current_direction == DOWN) { //DOWN
			y_coord+=20;
		}
		else if(current_direction == UP) { //UP
			y_coord-=20;
		}
		else if(current_direction == LEFT) { //LEFT
			x_coord-=20;
		}
		else if(current_direction == RIGHT) { //RIGHT
			x_coord+=20;
		}
		/*COLLISION DETECTION */
		// body collision: check if the updated coordinate of the head has any intersection
				// with the body blocks
				// change the condition to a variable condition later

		/*REVERT THIS LATER */
		if(y_coord == fruitY && x_coord == fruitX) {

			xil_printf("FRUIT YUMMY IN MY TUMMY\n");
			SNAKE_LENGTH++;
			//DrawBlock(snakeBody_X[SNAKE_LENGTH-1], snakeBody_Y[SNAKE_LENGTH-1], false);
			fruitX+=20;
			DrawFruit(fruitX, fruitY);
		}
		/*REVERT THIS LATER */
		for(int i = 0; i<SNAKE_LENGTH; i++) {
			if(x_coord == snakeBody_X[i] && y_coord == snakeBody_Y[i]) {
				xil_printf("BODY COLLISION\n");
				GameOver = true;
				GameOver_loop();
				}
		}

		if (y_coord > 929 || y_coord < 130 ||  x_coord > 1189 || x_coord < 90) {
			xil_printf("BORDER COLLISION\n");
			GameOver = true;
			GameOver_loop();
		}
		/*COLLISION DETECTION */


		else {
		DrawBlock(x_coord, y_coord, false);
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
	// default left
	// changing direction is only valid for perpendicular directions
	// i.e. up -> left or up->right are valid,  up->down is not valid
	
	if(direction ==LEFT  && current_direction == DOWN ) {
		current_direction = LEFT;
		}
	else if(direction==RIGHT  && current_direction == DOWN ) {
		current_direction = RIGHT;
		}
	else if(direction== LEFT  && current_direction == UP ) {
		current_direction = LEFT;
		}
	else if(direction== RIGHT  && current_direction ==UP ) {
		current_direction = RIGHT;
		}
	else if(direction==DOWN  && current_direction ==LEFT ) {
		current_direction = DOWN;
		}
	else if(direction==UP  && current_direction ==LEFT ) {
		current_direction = UP;
		}
	else if(direction==DOWN  && current_direction ==RIGHT ) {
		current_direction = DOWN;
		}
	else if(direction==UP  && current_direction ==RIGHT ) {
		current_direction = UP;
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
	btn_value = XGpio_DiscreteRead(&BTNInst, 1)& 0x1F;
//	TIMER_INTR_FLG = true;

	switch (btn_value) {
		case 1:
			if(GameOver) {
				restart_game();
				GameOver = false;
			}
			break;
	    case 2:
	    	change_direction(DOWN); // UP
	        break;
	    case 4:
	    	change_direction(LEFT); // LEFT
	        break;
	    case 8:
	    	change_direction(RIGHT);  // RIGHT
	        break;
	    case 16:
	    	change_direction(UP); // DOWN
	    	break;
	    default:
	        printf("Default case is Matched.");
	        break;
	    }

    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    XGpio_InterruptClear(&BTNInst, XGPIO_IR_MASK);
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
	xil_printf("It's in the second cpu!!!");
	int xStatus;
	XTmrCtr TimerInstancePtr;

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
			0xFF3CB9FF);
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
	u32 CntrlRegister;
	XUartPs_WriteReg(UART_BASEADDR, XUARTPS_CR_OFFSET,
					  ((CntrlRegister & ~XUARTPS_CR_EN_DIS_MASK) |
					   XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN));
	xil_printf("SNAKE GAME BEGINS SSSSSSSSS \n");
	restart_game();
	//Draw_Map(image_buffer_pointer);
	Xil_DCacheDisable();
	/* initializing snake */
	// playspace dimensions 1100 by 800 pixels
	// starting playspace coordinate = 90, 130


	while(1) {
		TIMER_INTR_FLG = false;
		XTmrCtr_Start(&TimerInstancePtr,0);
				while(TIMER_INTR_FLG == false){
				}
		TIMER_INTR_FLG = false;

	}
	return 0;
}

