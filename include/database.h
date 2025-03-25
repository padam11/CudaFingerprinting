#ifndef DATABASE_H
#define DATABASE_H

#include <unordered_map>
#include <list>
#include <vector>
#include "globals.h"

using namespace std;

class database {

  private:
    unordered_map< unsigned long, list<DataPoint> > hashmap; //Maps a hash (long) --> list of matching datapoints
    vector< char * > fileNames; //Index of vector is file_id. Slot of vector is filename
    vector< unsigned int > fileLengths; //Index of vector is file_id. Slot of vector is the number of chunks in that file
    int numFiles;

  public:
    database();

    int initFromScratch(char * folderPath);

    int serializeToFiles(char * outputHashesFile, char * outputFilenamesFile);

    int initFromFile(char * hashesFile, char * filenamesFile);

    int getBestMatchingSongNaive(int numHashes, unsigned long * hashes);

    int getBestMatchingSong(int numHashes, unsigned long * hashes);

};

#endif