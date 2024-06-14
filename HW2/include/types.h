#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cstring>
#define FREE false
#define FULL true
struct SuperBlock
{
    int blockSize;
    int numBlocks;
    int numFilenameBlocks;
    int rootDirPosition;
    int fatPosition;
    int dataBlockPosition;
    int bitmapPosition;
    int fileNamesPosition;
};

struct FAT12
{
    unsigned short *blockNum;
};

struct DataBlock
{
    uint8_t *data;
};

struct DirectoryEntry
{
    int filenamePos;
    int filenameLen;
    char attributes[7]; // r and w
    bool isDirectory;
    long lm_time;
    long fc_time;
    unsigned short firstBlockNumber;
    unsigned short fileSize;
    char password[10];
};
struct Bitmap
{
    bool* blocks;
};
struct FileNames
{
    char* fileNames;


};
struct FileSystem
{
    struct SuperBlock superBlock;
    struct FAT12 fat;
    struct DataBlock *dataBlocks;
    struct DirectoryEntry rootDirEntry;
    struct Bitmap bitmap;
    char* filenames;
};

#endif