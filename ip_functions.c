/*
 * ip_functions.c
 *
 * Contains all functions which pertain to setup and use of IP periperals.
 */

#include "adventures_with_ip.h"

/* ---------------------------------------------------------------------------- *
 * 								lms_filter()									*
 * ---------------------------------------------------------------------------- *
 * This function adds a tonal noise component to the sampled audio from the
 * audio codec by passing a step size to the input of an NCO component in the
 * PL. A sinusoidal signal is received back from the NCO which is then scaled
 * and added to the sampled audio. The audio + noise sample can then be
 * adaptively filtered using an LMS filter in the PL. The resulting audio,
 * filtered or not, is then output to the audio codec.
 *
 * The main menu can be accessed by entering 'q' on the keyboard.
 * ---------------------------------------------------------------------------- */


/* ---------------------------------------------------------------------------- *
 * 								tonal_noise()									*
 * ---------------------------------------------------------------------------- *
 * This function adds a tonal noise component to the sampled audio from the
 * audio codec by passing a step size to the input of an NCO component in the
 * PL. A sinusoidal signal is received back from the NCO which is then scaled
 * and added to the sampled audio. The audio + noise sample is then sent to
 * the audio codec for output.
 *
 * The main menu can be accessed by entering 'q' on the keyboard.
 * ---------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------- *
 * 								audio_stream()									*
 * ---------------------------------------------------------------------------- *
 * This function performs audio loopback streaming by sampling the input audio
 * from the codec and then immediately passing the sample to the output of the
 * codec.
 *
 * The main menu can be accessed by entering 'q' on the keyboard.
 * ---------------------------------------------------------------------------- */
void audio_stream(){
	u32  in_left, in_right;

	while (!XUartPs_IsReceiveData(UART_BASEADDR)){
		// Read audio input from codec
		in_left = Xil_In32(I2S_DATA_RX_L_REG);
		in_right = Xil_In32(I2S_DATA_RX_R_REG);
		// Write audio output to codec
		Xil_Out32(I2S_DATA_TX_L_REG, in_left);
		Xil_Out32(I2S_DATA_TX_R_REG, in_right);
	}

	/* If input from the terminal is 'q', then return to menu.
	 * Else, continue streaming. */
//	if(XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET) == 'q') menu();
	//else audio_stream();
} // audio_stream()


/* ---------------------------------------------------------------------------- *
 * 								gpio_initi()									*
 * ---------------------------------------------------------------------------- *
 * Initialises the GPIO driver for the push buttons and switches.
 * ---------------------------------------------------------------------------- */
unsigned char gpio_init()
{
	int Status;

	Status = XGpio_Initialize(&Gpio, BUTTON_SWITCH_ID);
	if(Status != XST_SUCCESS) return XST_FAILURE;

	XGpio_SetDataDirection(&Gpio, SWITCH_CHANNEL, 0xFF);
	XGpio_SetDataDirection(&Gpio, BUTTON_CHANNEL, 0xFF);

	return XST_SUCCESS;
}

