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
const CHUNKSIZE = 65535;
typedef struct {
    // RIFF Header
    char     chunkID[4];     // "RIFF"
    uint32_t chunkSize;      // 4 + (8 + Subchunk1Size) + (8 + Subchunk2Size)
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
	   fread(&audioHeader,44,1,srcFile);
	//2) how many pieces needs to process all the file?
    	number_of_pieces = audioHeader.subchunk2Size / CHUNKSIZE;
	//3) remainder 
	   remainder =  audioHeader.subchunk2Size % CHUNKSIZE;
	//allocate memory
	    input_pcm = malloc(CHUNKSIZE*4);
        output_pcm = malloc(CHUNKSIZE*4);
        wavedata = malloc(CHUNKSIZE*2);
     //creae a new file
	     destFile = fopen("out.wav","wb");
	     if(destFile == NULL){
	     	perror("Can`t create file.");
	     	return 1;
		 }
     //write audio header
       fwrite (&audioHeader,44,1,destFile);

	//4) processing whole data chunks
		for (int a=0; a < number_of_pieces; a++) { 
			//a) read a chunk (16bit int) into the 16-bit buffer
				fread(wavedata, 2, CHUNKSIZE,  srcFile);
			//b)converting it into 32bit integers
				word_to_dword(wavedata, input_pcm, CHUNKSIZE);
			//c)filtering
				filter_proc(input_pcm, output_pcm, CHUNKSIZE);
			//d)Converting back to 16bit PCM
				dword_to_word(output_pcm, wavedata, CHUNKSIZE);
			//e)write filtered data into another new file
				fwrite(wavedata, 2, CHUNKSIZE,  destFile );
		}
	//5) Processing a remainder:
	     if (remainder > 0) {		 
	        //5.1) read a chunk (16bit int) into the 16-bit buffer
				fread(wavedata, remainder, 1, srcFile);
			//5.2)converting it into 32bit integers
				word_to_dword(wavedata, input_pcm, remainder);
			//5.3)filtering
				filter_proc(input_pcm, output_pcm, remainder);
			//5.4)Converting back to 16bit PCM
				dword_to_word(output_pcm, wavedata, remainder);
			//5.5)write filtered data into another new file
				fwrite(wavedata, remainder, 1, destFile );
	    }
    
    
    free((void*)wavedata);
    free((void*)input_pcm);
    free((void*)output_pcm);
    
    fclose(srcFile);
    fclose(destFile);
	return 0;
}