#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <string.h>


#include "../include/fs.h"
#include "../include/operations.h"
#include "../include/utils.h"
using namespace std;



void HandleOperation(int argc,  char *argv[]){
     if(argc < 3){
        cout << "Invalid arguments" << endl;
        exit(EXIT_FAILURE);
     }
    char* fileSystemName = argv[1];
    FileSystem fs = LoadFileSystem(fileSystemName);
    // cout << fs.filenames << endl;
    // map strings to integers to be able to use switch case
    map <string, int> operations = {
        {"dir",         0},
        {"mkdir",       1},
        {"rmdir",       2},
        {"dumpe2fs",    3},
        {"write",       4},
        {"read",        5},
        {"del",         6},
        {"chmod",       7},
        {"addpw",       8}
    };

    auto operation = operations.find(argv[2]);
    if(operation != operations.end()){
        switch (operation->second)
        {
        case 0: // dir
            argc == 4 ? dir(fs, argv[3], nullptr) : dir(fs, argv[3], argv[4]);
            break;
        case 1: // mkdir
            argc == 4 ? mkdir(fs, argv[3], nullptr) : mkdir(fs, argv[3], argv[4]);
            break;
        case 2: // rmdir
            argc == 4 ? rmdir(fs, argv[3], nullptr) : rmdir(fs, argv[3], argv[4]);
            break;
        case 3: // dumpe2fs
            dumpe2fs(fs);
            break;
        case 4: // write
            argc == 5 ? write(fs, argv[3], argv[4], nullptr) : write(fs, argv[3], argv[4], argv[5]);
            break;
        case 5: // read
            argc == 5 ? read(fs, argv[3], argv[4], nullptr) : read(fs, argv[3], argv[4], argv[5]);

            break;
        case 6: // del
            argc == 4 ? del(fs, argv[3], nullptr) : del(fs, argv[3], argv[4]);

            break;
        case 7: // chmod
            chmod(fs,argv[3],argv[4]);


            break;
        case 8: // addpw
            addpw(fs,argv[3],argv[4]);

            break;

        default:
            break;
        }
    }else {
        cout << "Invalid operation!" << endl;
    }

    SaveFileSystem(fs,fileSystemName);

    
    // Clean up
    delete[] fs.fat.blockNum;
    delete[] fs.filenames;
    for (int i = 0; i < fs.superBlock.numBlocks; i++)
    {
        delete[] fs.dataBlocks[i].data;
    }
    delete[] fs.dataBlocks;
    delete[] fs.bitmap.blocks;


}
int main(int argc, char *argv[])
{
   
    HandleOperation(argc, argv);


    return 0;
}
