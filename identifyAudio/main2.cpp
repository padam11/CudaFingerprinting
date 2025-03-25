#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "../include/globals.h"
#include "../include/preprocess.h"
#include "../include/fft_transform.h"
#include "../include/database.h"

int main(int argc, char *argv[])
{
    char inputPath[100];
    cout << "Enter path of audio file you need to identify: ";

    cin.getline(inputPath, sizeof inputPath);

    cout << endl;

    //step 2: get array of auudio samples, saved into a complex struct

    Complex *audio;
    int audioSize, numChunks;

    //wavtoComplex(inputPath, &audio, &audioSize);
    numChunks = audioSize / CHUNK_SAMPLES;

    //step 3: compute hashes for every single chunk of 1024 samples

    unsigned long * hashes;

    audioToHashes(audio, numChunks, &hashes);

    //step 4: find matching song from database object using
    //        hashes we computed

    database* db = new database();
    db->initFromFile((char *) "../../database/HASHES.txt", (char *) "../../database/FILENAMES.txt");
    db->getBestMatchingSong(numChunks, hashes);
}