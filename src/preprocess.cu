#include "globals.h"
#include "AudioFile.h"

//convert the wav file to an array of complex numbers
//complex numbers to be used in FFT computation
using namespace std;

void wavToComplex(char* inputFilePath, Complex** output, int* outputSize)
{
    //load waveform from disk
    //the samples field will contain waveform values for each channel (stereo = 2)
    AudioFile<float> audioFile;
    audioFile.load(inputFilePath);

    //sample rate == 22.05 kHz

    bool isMono = audioFile.isMono();
    int sampleRate = audioFile.getSampleRate();

    if (sampleRate != 22050)
    {
        cout << "ERROR: Not correct sample rate" << endl;
        exit(-1);
    }

    int numSamples = audioFile.getNumSamplesPerChannel();
    //allocates memory for the output array of complex numbers
    //each complex number will store real which is waveform amplitude, and imaginary which starts at 0 init
    *output = (Complex *) malloc(sizeof(Complex) * numSamples);
    int channel = 0;
    //set real part of complex number to waveform val
    //set imaginary to 0
    //FFT expects complex imputs

    for (int i = 0; i < numSamples; i++)
    {
        (*output)[i].re = audioFile.samples[channel][i];
        (*output)[i].im = 0;
    }

    *outputSize = numSamples;
    return;
}