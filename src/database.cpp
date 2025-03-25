#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <fstream>
#include <sstream>
#include "globals.h"
#include "../include/database.h"
#include "../include/preprocess.h"
//#include "../include/fft_transform.h"
using namespace std;

//constructor

database::database() {
    numFiles = 0;
}

int database::initFromScratch(char* folderPath)
{
    //for all wavs, assign a filename to each file ID
    DIR* dir;
    dirent* pdir;

    vector<string> files;

    dir = opendir(folderPath);
    pdir = readdir(dir);

    int i = 0;
    while (pdir)
    {
        string filename = pdir->d_name;
        if (filename.find(".wav") != string::npos)
        {
            fileNames.push_back(pdir->d_name); 
            i++;
        }
        pdir = readdir(dir);
    }
    numFiles = i;

    //compute hashes and datapoints
    for (int i = 0; i < fileNames.size(); i++)
    {
        char* filepath;
        filepath = (char *) malloc(strlen(folderPath)+strlen(fileNames[i])+1);
        strcpy(filepath, folderPath);
        strcat(filepath, fileNames[i]);

        Complex* audio;
        int audioSize, numChunks;
        wavToComplex(filepath, &audio, &audioSize);
        numChunks = audioSize / CHUNK_SAMPLES;
        unsigned long * hashes;

        audioToHashes(audio, numChunks, &hashes);

        fileLengths.push_back(numChunks);

        for (int j = 0; j < numChunks; j++)
        {
            DataPoint dp;
            dp.file_id = i;
            dp.time = j;
            unsigned long hash = hashes[j];

            hashmap[hash].push_back(dp);
        }
    }
    return 0;
}