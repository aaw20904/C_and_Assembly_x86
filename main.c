///NOTE:
//To compile project in Dev C++:
//choose 32bit compiler  Project->options->compiler
////OPTION: open in projectOptions->parameters->add library     and add object file
//The audio file to process must be  mono, with 44100Hz sampling rate
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
FILE* srcFile;
FILE* destFile;
const READ_BUFF_SIZE = 65536; //must divided by 16 without remainder!
                              //without it you have white noise or spikes in each buffer
typedef struct {
    // RIFF Header
    char     chunkID[4];     // "RIFF"
   uint32_t chunkSize;      // 4 + (8 + subchunk1Size) + (8 + subchunk2Size)
    char     format[4];      // "WAVE"

    // fmt subchunk
    char     subchunk1ID[4];   // "fmt "
    uint32_t subchunk1Size;    // 16 for PCM
    uint16_t audioFormat;      // PCM = 1 (Linear quantization)
    uint16_t numChannels;      // Mono = 1, Stereo = 2
    uint32_t sampleRate;       // 44100, 48000, etc.
    uint32_t byteRate;         // SampleRate * NumChannels * BitsPerSample / 8
    uint16_t blockAlign;       // NumChannels * BitsPerSample / 8
    uint16_t bitsPerSample;    // 8, 16, 24, 32

    // data subchunk
    char     subchunk2ID[4];   // "data"
    uint32_t subchunk2Size;    // audio data size.  NumSamples * NumChannels * BitsPerSample / 8
    // uint8_t  data[];        // Actual sound data follows (not declared in struct)
} WAVHeader;

int number_of_pieces;
int remainder;
int *input_pcm;
int *output_pcm;
short *wavedata;
WAVHeader audioHeader;
//prototypes
extern filter_proc(unsigned int* src, unsigned int* dest, unsigned int amount_samples);
extern word_to_dword( short int*, unsigned int*, unsigned int amount_samples);
extern dword_to_word( unsigned int*, short int*, unsigned int amount_samples);
extern cic16ord2( unsigned int*, short int*, unsigned int amount_samples);
extern integerBandpass( unsigned int*, short int*, unsigned int amount_samples);

 
int main (int argc, char *argv[]) {
	if(argv[1]==NULL){
		perror("File not found!Please Enter name as the first parameter in the command prompt!");
		return 1;
	}
	srcFile = fopen(argv[1],"rb");
	if (srcFile == NULL) {
		perror("IO Error!");
		return 1;
	}
	//1)read audio header
	   fread(&audioHeader,sizeof(WAVHeader),1,srcFile);
	//2) how many pieces needs to process all the file?
    	number_of_pieces = audioHeader.chunkSize / READ_BUFF_SIZE;
    	printf("Size of header %d  \n", sizeof(WAVHeader));
    	printf("pieces: %d, totaldata: %d \n", number_of_pieces,audioHeader.subchunk2Size);
	//3) remainder 
	   remainder = (audioHeader.chunkSize+8) - ( READ_BUFF_SIZE * number_of_pieces); // audioHeader.subchunk2Size % READ_BUFF_SIZE;
	//allocate memory
	    input_pcm = malloc(READ_BUFF_SIZE*4); //dwords
        output_pcm = malloc(READ_BUFF_SIZE*4); //dwords 
        wavedata = malloc(READ_BUFF_SIZE*2); //words
     //creae a new file
     
	     destFile = fopen("out.wav","wb");
	     if(destFile == NULL){
	     	perror("Can`t create file.");
	     	return 1;
		 }
     //write audio header
       fwrite (&audioHeader,sizeof(WAVHeader),1,destFile);

	//4) processing whole data chunks
		for (int a=0; a < number_of_pieces; a++) { 
			//a) read a chunk (16bit int) into the 16-bit buffer
				fread(wavedata, 1, READ_BUFF_SIZE,  srcFile);
			//b)converting it into 32bit integers
				word_to_dword(wavedata, input_pcm, (READ_BUFF_SIZE>>1)); //amount of words
			//c)filtering
		 integerBandpass(input_pcm, output_pcm, (READ_BUFF_SIZE>>1)); 
			//d)Converting back to 16bit PCM
				dword_to_word(output_pcm, wavedata, (READ_BUFF_SIZE>>1));
			//e)write filtered data into another new file
				fwrite(wavedata, 1, READ_BUFF_SIZE,  destFile );
		}
		printf("Remainder is %d \n", remainder);
	//5) Processing a remainder:
	     if (remainder > 0) {		 
	        //5.1) read a chunk (16bit int) into the 16-bit buffer
				fread(wavedata, 1, remainder,  srcFile);
			//5.2)converting it into 32bit integers
				word_to_dword(wavedata, input_pcm, remainder>>1);
			//5.3)filtering
		 integerBandpass(input_pcm, output_pcm, remainder>>1);
			//5.4)Converting back to 16bit PCM
				dword_to_word(output_pcm, wavedata, remainder>>1);
			//5.5)write filtered data into another new file
				fwrite(wavedata, 1, remainder,  destFile );
	    }
    
    
    free((void*)wavedata);
    free((void*)input_pcm);
    free((void*)output_pcm);
    
    fclose(srcFile);
    fclose(destFile);
	return 0;
}
