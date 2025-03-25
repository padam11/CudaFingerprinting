#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "../include/database.h"
#include "../include/globals.h"

using namespace std;

int main(int argc, char *argv[])
{
    char inputPath[100];

    cout << "Enter name of folder containing WAV's to build database: ";
    cin.getline(inputPath, sizeof inputPath);
    cout << endl;

    database* db = new database();
    db->initFromScratch(inputPath);

    db->serializeToFiles((char *) "../../database/HASHES.txt", (char *) "../../database/FILENAMES.txt");

    return 0;
}
