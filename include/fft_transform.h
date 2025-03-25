#ifndef FFT_TRANSFORM_H
#define FFT_TRANSFORM_H
#include "globals.h"

void audioToHashes(Complex * input, int numChunks, unsigned long ** outputHashes);

#endif