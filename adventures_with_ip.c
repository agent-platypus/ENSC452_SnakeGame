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

u8 *bgmBuffer = NULL;      // Background music buffer
size_t bgmBufferSize = 0;  // Size of BGM buffer
size_t bgmPosition = 0;    // Current playback position for BG

u8 *sfxBuffer = NULL;      // Sound effect buffer
size_t sfxBufferSize = 0;  // Size of SFX buffer
size_t sfxPosition = 0;    // Current playback position for SFX

FATFS FS_instance;




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

// Code from sd card tutorial re purposed for loading audio off sd card
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

        if (shared_memory -> COMM_VAL == 1) {
                  	printf("playing soundfx \n");
                  	printf("FILE NAME: %c \n", shared_memory -> filename);
                  	printf("VOLUME: %d \n", shared_memory -> theVolume);
                  	playSoundEffect(shared_memory -> filename);
                  	shared_memory -> COMM_VAL = 0;
    }

    }
}



void throwFatalError(const char *func,const char *msg) {
	printf("%s() : %s\r\n",func,msg);
	for(;;);
}

// function to mount sd card, while loop until successfully found
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
	            loadAudio("bgm.wav", &bgmBuffer, &bgmBufferSize, &bgmPosition);
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

	// loads sd card
	sd_card();

	print("CPU1: init_platform\n\r");

	// Starts the loop for playing audio, receiving messages from core 0
	playAudio1();


	/* Display interactive menu interface via terminal */
    return 1;
}


