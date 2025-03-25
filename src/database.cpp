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
#include "../include/fft_transform.h"
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

/*
  serializeToFiles
  ====================
  Takes the contents of the database and serializes them to text files
  that can be used to initialize the database again at 

  @param - folderPath: string containing input file for WAV

  @return - 0 for success, 1 for error
*/

int database::serializeToFiles(char * outputHashesFile, char * outputfileNamesFile){
  
    fstream fileNamesFile(outputfileNamesFile, ios_base::out);
    
    for(int i=0; i<fileNames.size(); i++){
      fileNamesFile << i << " " << fileNames[i] << " " << fileLengths[i] << " " << endl;
    }
  
    fstream hashFile(outputHashesFile, ios_base::out);
  
    for (auto it : hashmap){
      long hash = it.first;
      
      list<DataPoint> dpList = hashmap[hash];
  
      hashFile << std::hex << hash;
  
      for(auto &i : dpList){
        DataPoint dp = i;
        hashFile << std::dec << " ( " << dp.file_id << " " << dp.time << " )";
      }
      hashFile << endl;
    }
  
  
    fileNamesFile.close();
    hashFile.close();
  
    return 0;
      
  }
  
  
  /*
  dbInitFromFile
  ====================
  Initializes database from a previously serialized instance of a database.
  This is useful so we don't have to perform FFTs and determine frequencies
  from hours of audio all over again.
  
  @param - hashesFile: file containing serialized hashmap
          - fileNamesFile: file containing serialized fileIDs and fileNames
  
  @return - 0 for success, 1 for error
  */
  int database::initFromFile(char * hashesFile, char * fileNamesFile){
  
    //Step 1: Read in the fileNames and assign file IDs
    
    fstream fstreamfileNames(fileNamesFile, ios_base::in);
    if (!fstreamfileNames) {
      cout << "Unable to open fileNames file";
      exit(1); // terminate with error
    }
  
    int id;
    int length;
    string fname;
  
    while ( fstreamfileNames >> id >> fname >> length ){
      char * fnameCstr = new char[fname.length() + 1];
      strcpy(fnameCstr, fname.c_str());
      fileNames.push_back(fnameCstr);
      fileLengths.push_back(length);
      numFiles++;
    }
  
    fstreamfileNames.close();
  
    //Step 2: Read in the hash table
  
    fstream fstreamHashes(hashesFile, ios_base::in);
    if (!fstreamHashes) {
      cout << "Unable to open hashes file";
      exit(1); // terminate with error
    }
  
    string line;
    while (getline( fstreamHashes, line))
    {
      stringstream stream(line);
  
      unsigned long hash;
      stream >> std::hex >> hash >> std::dec;
  
  
      list<DataPoint> dpList;
      char openParentheses;
      char closeParentheses;
      unsigned int dataPointTime;
      unsigned int dataPointFileId;
      int i = 0;
  
  
      while( stream >> openParentheses >> dataPointFileId >> dataPointTime >> closeParentheses){          
        if(openParentheses!='(' || closeParentheses!=')'){
          cout << "HASHES text file not formatted correctly at line: '" << line << "'" << endl;
          exit(1); // terminate with error
        }
        DataPoint * dp = new DataPoint;
        dp->file_id = dataPointFileId;
        dp->time = dataPointTime;
        
        dpList.push_back(*dp);
      }
      
      hashmap[hash] = dpList;
    }
  
    return 0;
  }
  
  //using a histogram voting system
  //for each hash from the query:
  // 1. look up which songs have the same hash
  // 2. track how well their time offsets align with the query.
  // 3. songs with the most consistent time aligned matches get higher scores
  // 4. the song with the most votes in the histogram is declared the best match.

  int database::getBestMatchingSong(int numHashes, unsigned long * hashes){
  
    int biggestFileLength = 0;
    for(int i=0; i<fileLengths.size(); i++) if(fileLengths[i] > biggestFileLength) biggestFileLength = fileLengths[i];
    
    //Compute histogram
    //offsethistory: tracks where in time within a song the matching hash appeared, for alignment checking

    int histogram[numFiles];  //Histogram of what files match each audio chunk
    memset(histogram, 0, numFiles * sizeof(int));
  
    int offsetHistory[numFiles][biggestFileLength]; //Keeps track of at what point in time did we last see the hashes
    memset(offsetHistory, -1, sizeof(offsetHistory[0][0]) * numFiles * biggestFileLength);
  
    //check which songs have this hash, and what time offset the hash appeared
    for(int t=0; t<numHashes; t++){
  
      bool histogramCandidates[numFiles]; //Histogram candidates for this iteration
      memset(histogramCandidates, false, sizeof(bool)*numFiles);
  
      //each hash may match multiple entries in the hashmap database, which are stored as DataPoint objects containing a filename and time

      unsigned long hash = hashes[t];
      list<DataPoint> datapoints = hashmap[hash];
  
      //this block checks if a hash for a given song file_id appears at a consistent offset (ie the current query hash t aligns well with the stored hash's time)
      // if so, mark it as a match candidate for that file
      for(auto dp : datapoints){
        if(dp.time > 0 && offsetHistory[dp.file_id][dp.time-1] == (t-1)){ 
          histogramCandidates[dp.file_id] = true; //We found a candidate
        }
        offsetHistory[dp.file_id][dp.time] = t;
      }
      //tally histogram votes
      //after each hash, update histogram scores for all candidate songs
      for(int i=0; i<numFiles; i++) if(histogramCandidates[i] == true) histogram[i]++;
    }
  
    //Get best song from the histogram
    int best = 0;
    int bestScore = 0;
    cout << endl << "---HISTOGRAM RESULTS---" << endl;
    for(int i=0; i<numFiles; i++){
      cout << "FILE ID " << i << " HAS SCORE OF " << histogram[i] << " ( " << fileNames[i] << " ) " <<endl;
      if(histogram[i] > bestScore){
        best = i;
        bestScore = histogram[i];
      }
    }
    cout << endl << "The audiofile " << fileNames[best] << " was the BEST matching song! " << endl << endl;
  
  
    return best;
  }