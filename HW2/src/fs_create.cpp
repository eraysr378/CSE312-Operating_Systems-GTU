
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <string.h>

#include "../include/types.h"
#include "../include/fs.h"
using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Invalid parameters, correct usage:" << endl;
        cout << "program blockSize fileSystemName" << endl;
        exit(EXIT_FAILURE);
    }
    char *fileSystemName = argv[2];
    int blockSize = atof(argv[1]) * 1024; // convert to bytes
    if(blockSize <= 0){
        cout << "Invalid block size" << endl;
        exit(EXIT_FAILURE);

    }

    CreateFileSystem(fileSystemName, blockSize);
    
}