/*
 * adventures_with_ip.c
 *
 * Main source file. Contains main() and menu() functions.
 */
#include "adventures_with_ip.h"
#define fatalError(msg) throwFatalError(__PRETTY_FUNCTION__,msg)

typedef struct {
    volatile unsigned long COMM_VAL; // COMM_VAL as part of the structure
    char filename[32]; // Filename of the audio file
    int command;    // Command (e.g., play background music, play sound effect)
    volatile int theVolume;
} __attribute__((packed)) audio_command_t; // Pack the structure to avoid padding

volatile audio_command_t *shared_memory = (audio_command_t *)0xFFFF0000; // Adjust address as needed
// Initialize shared memory to zero
#define IP_CORE_BASE_ADDR XPAR_LFSR_IP_0_S00_AXI_BASEADDR
#define SEED_REG_OFFSET          0x04
extern u32 MMUTable;
// Register offset for the random number (e.g., slv_reg0)
#define RANDOM_NUMBER_REG_OFFSET 0x00

typedef struct {
    u8 *buffer;       // Audio data buffer
    size_t size;      // Size of the buffer
    size_t position;  // Current playback position
} SoundEffect;

SoundEffect sfxQueue[3]; // Example: 3 different collision sound effects
int sfxQueueCount = 0;

u8 *bgmBuffer = NULL;      // Background music buffer
size_t bgmBufferSize = 0;  // Size of BGM buffer
size_t bgmPosition = 0;    // Current playback position for BG

u8 *sfxBuffer = NULL;      // Sound effect buffer
size_t sfxBufferSize = 0;  // Size of SFX buffer
size_t sfxPosition = 0;    // Current playback position for SFX

FATFS FS_instance;


// This holds the memory allocated for the wav file currently played.
u8 *theBuffer = NULL;
size_t theBufferSize = 0;

typedef struct {
	char riff[4];
	u32 riffSize;
	char wave[4];
} headerWave_t;

typedef struct {
	char ckId[4];
	u32 cksize;
} genericChunk_t;


typedef struct {
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
	u16 cbSize;
	u16 wValidBitsPerSample;
	u32 dwChannelMask;
	u8 SubFormat[16];
} fmtChunk_t;
headerWave_t headerWave;
   fmtChunk_t fmtChunk;

void loadAudio(const char *filename, u8 **buffer, size_t *bufferSize, size_t *position) {
    headerWave_t headerWave;
    fmtChunk_t fmtChunk;
    FIL file;
    UINT nBytesRead = 0;

    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != 0) {
        fatalError("File not found");
    }
    printf("Loading %s\r\n", filename);

    // Read the RIFF header and do some basic sanity checks
    res = f_read(&file, (void *)&headerWave, sizeof(headerWave), &nBytesRead);
    if (res != 0) {
        fatalError("Failed to read file");
    }
    if (memcmp(headerWave.riff, "RIFF", 4) != 0) {
        fatalError("Illegal file format. RIFF not found");
    }
    if (memcmp(headerWave.wave, "WAVE", 4) != 0) {
        fatalError("Illegal file format. WAVE not found");
    }

    // Walk through the chunks and interpret them
    for (;;) {
        genericChunk_t genericChunk;
        res = f_read(&file, (void *)&genericChunk, sizeof(genericChunk), &nBytesRead);
        if (res != 0) {
            fatalError("Failed to read file");
        }
        if (nBytesRead != sizeof(genericChunk)) {
            break; // probably EOF
        }

        // The "fmt " is compulsory and contains information about the sample format
        if (memcmp(genericChunk.ckId, "fmt ", 4) == 0) {
            res = f_read(&file, (void *)&fmtChunk, genericChunk.cksize, &nBytesRead);
            if (res != 0) {
                fatalError("Failed to read file");
            }
            if (nBytesRead != genericChunk.cksize) {
                fatalError("EOF reached");
            }
            if (fmtChunk.wFormatTag != 1) {
                fatalError("Unsupported format");
            }
            if (fmtChunk.nChannels != 2) {
                fatalError("Only stereo files supported");
            }
            if (fmtChunk.wBitsPerSample != 16) {
                fatalError("Only 16 bit per samples supported");
            }
        }
        // the "data" chunk contains the audio samples
        else if (memcmp(genericChunk.ckId, "data", 4) == 0) {
            *buffer = malloc(genericChunk.cksize);
            if (!*buffer) {
                fatalError("Could not allocate memory");
            }
            *bufferSize = genericChunk.cksize;

            res = f_read(&file, (void *)*buffer, *bufferSize, &nBytesRead);
            if (res != 0) {
                fatalError("Failed to read file");
            }
            if (nBytesRead != *bufferSize) {
                fatalError("Didn't read the complete file");
            }
        }
        // Unknown chunk: Just skip it
        else {
            DWORD fp = f_tell(&file);
            f_lseek(&file, fp + genericChunk.cksize);
        }
    }

    f_close(&file);
    *position = 0; // Reset playback position
}

void playSoundEffect(const char *filename) {
    // Stop any currently playing SFX
    if (sfxBuffer) {
        free(sfxBuffer);
        sfxBuffer = NULL;
        sfxBufferSize = 0;
        sfxPosition = 0;
    }

    // Load the new SFX
    loadAudio(filename, &sfxBuffer, &sfxBufferSize, &sfxPosition);
}

//Based off the playWavFile from the sdcard tutorial, plays the music, and when sfx is queued up then mix them together
void playAudio1() {
    while (1) {
        // Declare and initialize BGM samples
        short bgmLeft = 0;
        short bgmRight = 0;

        // Check if BGM is playing
        if (bgmBuffer && bgmPosition < bgmBufferSize) {
            // Get the next sample from BGM
            bgmLeft = *(short *)(bgmBuffer + bgmPosition);
            bgmRight = *(short *)(bgmBuffer + bgmPosition + 2);
            bgmLeft = (short)((int)bgmLeft * shared_memory -> theVolume / 4); // Apply volume
            bgmRight = (short)((int)bgmRight * shared_memory -> theVolume / 4); // Apply volume

            // Move to the next sample
            bgmPosition += 4; // 16-bit stereo = 4 bytes per sample

            // Loop BGM if necessary
            if (bgmPosition >= bgmBufferSize) {
                bgmPosition = 0;
            }
        }

        // Mix all active SFX
        short mixedLeft = 0;
        short mixedRight = 0;
        if (sfxBuffer && sfxPosition < sfxBufferSize) {
            // Get the next sample from SFX
            short sfxLeft = *(short *)(sfxBuffer + sfxPosition);
            short sfxRight = *(short *)(sfxBuffer + sfxPosition + 2);
            sfxLeft = (short)((int)sfxLeft * shared_memory -> theVolume / 3); // Apply volume
            sfxRight = (short)((int)sfxRight * shared_memory -> theVolume / 3); // Apply volume

            // Add to the mixed samples
            mixedLeft += sfxLeft;
            mixedRight += sfxRight;

            // Move to the next sample
            sfxPosition += 4; // 16-bit stereo = 4 bytes per sample
        }

        // Mix BGM and SFX samples
        mixedLeft += bgmLeft;
        mixedRight += bgmRight;

        // Clamp the mixed samples to the valid range
        if (mixedLeft > 32767) mixedLeft = 32767;
        if (mixedLeft < -32768) mixedLeft = -32768;
        if (mixedRight > 32767) mixedRight = 32767;
        if (mixedRight < -32768) mixedRight = -32768;

        // Send the mixed samples to the audio hardware (e.g., I2S)
        Xil_Out32(I2S_DATA_TX_L_REG, mixedLeft * 100);
        Xil_Out32(I2S_DATA_TX_R_REG, mixedRight * 100);

        // Simulate delay (replace with actual I2S timing)
        for (int i = 0; i < 490; i++) {
            asm("NOP");
        }
    //	printf("COMM_VAL: %d \n", shared_memory -> COMM_VAL);

        if (shared_memory -> COMM_VAL == 1) {
                  	printf("playing soundfx \n");
                  	printf("FILE NAME: %c \n", shared_memory -> filename);
                  	printf("VOLUME: %d \n", shared_memory -> theVolume);
                  	playSoundEffect(shared_memory -> filename);
                  	shared_memory -> COMM_VAL = 0;
    }

    }
}


void loadSoundEffect(SoundEffect *sfx, const char *filename) {
    FIL file;
    UINT bytesRead;

    // Open the file
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        printf("Failed to open file: %s\r\n", filename);
        return;
    }

    // Allocate memory for the file
    sfx->size = f_size(&file);
    sfx->buffer = malloc(sfx->size);
    if (!sfx->buffer) {
        printf("Failed to allocate memory for file: %s\r\n", filename);
        f_close(&file);
        return;
    }

    // Read the file into memory
    res = f_read(&file, sfx->buffer, sfx->size, &bytesRead);
    if (res != FR_OK || bytesRead != sfx->size) {
        printf("Failed to read file: %s\r\n", filename);
        free(sfx->buffer);
        f_close(&file);
        return;
    }

    // Close the file
    f_close(&file);

    // Initialize playback position
    sfx->position = 0;

    printf("Loaded sound effect: %s\r\n", filename);
}

void loadAllSoundEffects() {
  //  loadSoundEffect(&collisionSfx[0], "Food.wav");
    //loadSoundEffect(&collisionSfx[1], "collision2.wav");
    //loadSoundEffect(&collisionSfx[2], "collision3.wav");
}
void throwFatalError(const char *func,const char *msg) {
	printf("%s() : %s\r\n",func,msg);
	for(;;);
}

// Size of the buffer which holds the DMA Buffer Descriptors (BDs)
#define DMA_BDUFFERSIZE 4000
/*
typedef struct
{
	XLlFifo spiFifoController;
	u8 spiChipAddr;
	int spiFifoWordsize;

	XAxiDma dmaAxiController;
	XAxiDma_Bd dmaBdBuffer[DMA_BDUFFERSIZE] __attribute__((aligned(XAXIDMA_BD_MINIMUM_ALIGNMENT)));
	int dmaWritten;
} adau1761_config;
*/



int readButton(int buttonPin) {

    return XGpio_DiscreteRead(&Gpio, 1) & (1 << buttonPin);
}
void stopWavFile() {
	// If there is already a WAV file playing, stop it

    // If there was already a WAV file, free the memory
    if (theBuffer){
    	free(theBuffer);
    	theBuffer = NULL;
    	theBufferSize = 0;
    }
}


//void playWavFile(const char *filename) {
//    headerWave_t headerWave;
//    fmtChunk_t fmtChunk;
//    FIL file;
//    UINT nBytesRead=0;
//
//    stopWavFile();
//
//    FRESULT res = f_open(&file, filename, FA_READ);
//    if (res!=0) {
//    	printf("FILE : %c", filename);
//    	fatalError("File not found");
//    }
//    printf("Loading %s\r\n",filename);
//
//    // Read the RIFF header and do some basic sanity checks
//    res = f_read(&file,(void*)&headerWave,sizeof(headerWave),&nBytesRead);
//    if (res!=0) {
//    	fatalError("Failed to read file");
//    }
//	if (memcmp(headerWave.riff,"RIFF",4)!=0) {
//		fatalError("Illegal file format. RIFF not found");
//	}
//	if (memcmp(headerWave.wave,"WAVE",4)!=0) {
//		fatalError("Illegal file format. WAVE not found");
//	}
//
//	// Walk through the chunks and interpret them
//	for(;;) {
//		// read chunk header
//		genericChunk_t genericChunk;
//		res = f_read(&file,(void*)&genericChunk,sizeof(genericChunk),&nBytesRead);
//		if (res!=0) {
//			fatalError("Failed to read file");
//		}
//		if (nBytesRead!=sizeof(genericChunk)) {
//			break; // probably EOF
//		}
//
//		// The "fmt " is compulsory and contains information about the sample format
//		if (memcmp(genericChunk.ckId,"fmt ",4)==0) {
//			res = f_read(&file,(void*)&fmtChunk,genericChunk.cksize,&nBytesRead);
//			if (res!=0) {
//				fatalError("Failed to read file");
//			}
//			if (nBytesRead!=genericChunk.cksize) {
//				fatalError("EOF reached");
//			}
//			if (fmtChunk.wFormatTag!=1) {
//				fatalError("Unsupported format");
//			}
//			if (fmtChunk.nChannels!=2) {
//				fatalError("Only stereo files supported");
//			}
//			if (fmtChunk.wBitsPerSample!=16) {
//				fatalError("Only 16 bit per samples supported");
//			}
//		}
//		// the "data" chunk contains the audio samples
//		else if (memcmp(genericChunk.ckId,"data",4)==0) {
//		    theBuffer = malloc(genericChunk.cksize);
//		    if (!theBuffer){
//		    	fatalError("Could not allocate memory");
//		    }
//		    theBufferSize = genericChunk.cksize;
//
//		    res = f_read(&file,(void*)theBuffer,theBufferSize,&nBytesRead);
//		    if (res!=0) {
//		    	fatalError("Failed to read file");
//		    }
//		    if (nBytesRead!=theBufferSize) {
//		    	fatalError("Didn't read the complete file");
//		    }
//		}
//		// Unknown chunk: Just skip it
//		else {
//			DWORD fp = f_tell(&file);
//			f_lseek(&file,fp + genericChunk.cksize);
//		}
//	}
//
//	// If we have data to play
//    if (theBuffer) {
//        printf("Playing %s\r\n",filename);
//
//        // Crude in-place down-sampling: Basically taking every n'th of a sample
//        // Jerobeam Fenderson's WAV files use a sampling rate of 192kHz (https://oscilloscopemusic.com)
//        // Our sampling rate is actually 39.0625, so a 44.1kHz file will play a at 88.5% the speed (and lower in pitch).
//        double subSample = (double)fmtChunk.nSamplesPerSec/44100;
//            	if (subSample>1.6) {
//            		int skip = (int)(subSample+0.5);
//            		u32 nNewTotal = theBufferSize/4/skip;
//            		u32 *pSource = (u32*) theBuffer;
//            		u32 *pDest = (u32*) theBuffer;
//            		for(u32 i=0;i<nNewTotal;++i,pSource+=skip,pDest++) {
//            			*pDest = *pSource;
//            		}
//            		theBufferSize = nNewTotal*4;
//            	}
//    	// Changing the volume and swap left/right channel and polarity
//    	{
//    		while (shared_memory -> loopMusic){
//    			u32 *pSource = (u32*) theBuffer;
//    			    		for(u32 i=0;i<theBufferSize/4;++i) {
//    			    			if (XUartPs_IsReceiveData(UART_BASEADDR)) {
//    			    			                char key = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
//    			    			                if (key == 'q') {
//    			    			                    printf("Playback stopped by user.\r\n");
//    			    			                    break;
//    			    			                }
//    			    			            }
//
//    		//	printf("Buffer[%d] = 0x%08X\n", i, pSource[i]);
//    			short left  = (short) ((pSource[i]>>16) & 0xFFFF);
//    			short right = (short) ((pSource[i]>> 0) & 0xFFFF);
//    			//printf("Raw Sample %d: Left = %d, Right = %d\n", i, left, right);
//    			int left_i  = -(int)left * theVolume / 4;
//    			int right_i = -(int)right * theVolume / 4;
//    			if (left>32767) left = 32767;
//    			if (left<-32767) left = -32767;
//    			if (right>32767) right = 32767;
//    			if (right<-32767) right = -32767;
//    			left = (short)left_i;
//    			right = (short)right_i;
//    			pSource[i] = ((u32)right<<16) + (u32)left;
//
//    			for(int i = 0; i<490; i++) //experiment with the i value to make it play at normal speed
//    				asm("NOP");
//    			Xil_Out32(I2S_DATA_TX_L_REG, left*100);
//    			Xil_Out32(I2S_DATA_TX_R_REG, right*100);
//
//
//    		if (shared_memory->loopMusic) {
//    			printf("Looping %s\r\n", filename);
//    		}
//
//    		}
//    		//printf("Writing to I2S: Left = %d, Right = %d\n", left, right);
//
//    		}
//
//
//    	//adau1761_dmaTransmitBLOB(&codec, (u32*)theBuffer, theBufferSize/4);
//    }
//
//
//
//
//}
//}


void sd_card() {
	setvbuf(stdin, NULL, _IONBF, 0);
	    FRESULT result;
	    int retry_count = 0;
	    const int max_retries = 10;

	    while(retry_count < max_retries) {
	      //  print("Mount attempt %d of %d...\n\r", retry_count+1, max_retries);
	        result = f_mount(&FS_instance, "0:/", 1);

	        if (result == FR_OK) {
	            printf("SD card mounted successfully.\r\n");
	            loadAudio("piano.wav", &bgmBuffer, &bgmBufferSize, &bgmPosition);
	            return;
	        }

	        printf("Mount failed (Error: %d)\r\n", result);
	        retry_count++;
	        usleep(1000000); // Wait 1 second before retrying
	    }

	    printf("Failed to mount SD card after %d attempts\n\r", max_retries);
	    // Handle permanent failure here
	}


// Base address of the IP core (defined in xparameters.h)
// Shared memory structure
#define AUDIO_COMMAND_BGM 1
#define AUDIO_COMMAND_STOP 0


/* ---------------------------------------------------------------------------- *
 * 									main()										*
 * ---------------------------------------------------------------------------- *
 * Runs all initial setup functions to initialise the audio codec and IP
 * peripherals, before calling the interactive menu system.
 * ---------------------------------------------------------------------------- */

int main(void)
{
	memset((void *)shared_memory, 0, sizeof(audio_command_t));
	shared_memory -> theVolume = 2;
    //Disable cache on OCM
	    // S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0
	Xil_SetTlbAttributes(0xFFFF0000,0x14de2);
	xil_printf("Entering Main\r\n");
	//Configure the IIC data structure
	IicConfig(XPAR_XIICPS_0_DEVICE_ID);

	//Configure the Audio Codec's PLL
	AudioPllConfig();

	//Configure the Line in and Line out ports.
	//Call LineInLineOutConfig() for a configuration that
	//enables the HP jack too.
	AudioConfigureJacks();
	//VerifyAudioConfiguration();
	xil_printf("ADAU1761 configured\n\r");

	/* Initialise GPIO and NCO peripherals */
	gpio_init();
	//nco_init(&Nco);
	sd_card();

	print("CPU1: init_platform\n\r");
	//playWavFile("piano.wav");
	playAudio1();
	//audioCoreTask();

	/* Display interactive menu interface via terminal */
    return 1;
}


