
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <string.h>
#include <time.h>

#include "../include/types.h"
#include "../include/fs.h"
using namespace std;

void CreateFileSystem(char *fileSystemName, int blockSize)
{
    FileSystem fs;

    // Define superblock parameters
    fs.superBlock.blockSize = blockSize;
    fs.superBlock.numBlocks = 4096; // 2^12
    fs.superBlock.numFilenameBlocks = 10; // reserve 10 blocks to store filenames

    // Define the number of FAT entries (same as number of blocks)
    int numFatEntries = fs.superBlock.numBlocks;
    // fs.superBlock.blockSize - 1 added to make sure we do not lose block because of rounding
    int fatSizeInBlocks = (numFatEntries * sizeof(unsigned short) + blockSize - 1) / blockSize;
    int bitmapSizeInBlocks = (fs.superBlock.numBlocks * sizeof(bool) + blockSize - 1) / blockSize;
    int bitmapEntriesPerBlock = blockSize / sizeof(bool);

    // Calculate the positions
    fs.superBlock.fatPosition = 1;  // SuperBlock is at position 0, FAT starts at position 1
    fs.superBlock.rootDirPosition = fs.superBlock.fatPosition + fatSizeInBlocks; // Root directory after FAT
    fs.superBlock.fileNamesPosition = fs.superBlock.rootDirPosition + 1;
    fs.superBlock.bitmapPosition = fs.superBlock.fileNamesPosition + fs.superBlock.numFilenameBlocks;
    // Assuming a fixed size for root directory (e.g., 1 block)
    int rootDirSizeInBlocks = 1;
    fs.superBlock.dataBlockPosition = fs.superBlock.bitmapPosition + bitmapSizeInBlocks;

    
    fs.filenames = new char[fs.superBlock.numFilenameBlocks *blockSize];
    fill(fs.filenames, fs.filenames + fs.superBlock.numFilenameBlocks *blockSize, 0); 

    // Initialize FAT
    fs.fat.blockNum = new unsigned short[numFatEntries];
    fill(fs.fat.blockNum, fs.fat.blockNum + numFatEntries, 0xFFF); // Mark all entries as free (0xFFF is the end of chain marker)

    // Initialize Data Blocks
    fs.dataBlocks = new DataBlock[fs.superBlock.numBlocks];
    for (int i = 0; i < fs.superBlock.numBlocks; i++)
    {
        fs.dataBlocks[i].data = new uint8_t[blockSize];
        fill(fs.dataBlocks[i].data, fs.dataBlocks[i].data + blockSize, 0);
    }

    // Store SuperBlock in the first data block
    memcpy(fs.dataBlocks[0].data, &fs.superBlock, sizeof(SuperBlock));

    // Store FAT in the data blocks
    int fatStartBlock = fs.superBlock.fatPosition;
    int fatEntriesPerBlock = blockSize / sizeof(unsigned short);
    for (int i = 0; i < fatSizeInBlocks; i++)
    {
        int offset = i * fatEntriesPerBlock;
        int entriesToCopy = min(fatEntriesPerBlock, numFatEntries - offset);
        memcpy(fs.dataBlocks[fatStartBlock + i].data, fs.fat.blockNum + offset, entriesToCopy * sizeof(unsigned short));
    }
    time_t rawtime;
    time(&rawtime);
    // Initialize root directory entry
    // strncpy(fs.rootDirEntry.filename, "ROOT", 5);
    memcpy(fs.filenames,"ROOT",5);
    fs.rootDirEntry.filenamePos = 0;
    fs.rootDirEntry.filenameLen = 5;
    strncpy(fs.rootDirEntry.attributes, "rwrwrw", 7); // Directory attribute
    fs.rootDirEntry.isDirectory = true;
    fs.rootDirEntry.fc_time = rawtime;
    fs.rootDirEntry.lm_time = rawtime;
    fs.rootDirEntry.firstBlockNumber = fs.superBlock.rootDirPosition;
    fs.rootDirEntry.fileSize = 0;
    strncpy(fs.rootDirEntry.password, "\0", 2);
            
    // Store root directory entry in the corresponding data block
    memcpy(fs.dataBlocks[fs.superBlock.rootDirPosition].data, &fs.rootDirEntry, sizeof(DirectoryEntry));

    memcpy(fs.dataBlocks[fs.superBlock.fatPosition].data, fs.filenames, blockSize);
    // Initialize bitmap
    fs.bitmap.blocks = new bool[fs.superBlock.numBlocks];
    fill(fs.bitmap.blocks, fs.bitmap.blocks + fs.superBlock.numBlocks, FREE); // Mark all blocks as free

    // Mark the blocks used by the superblock, FAT,root directory and bitmap as occupied

    for (int i = 0; i < fs.superBlock.dataBlockPosition; i++)
    {
        fs.bitmap.blocks[i] = FULL;
    }

    for (int i = 0; i < bitmapSizeInBlocks; i++)
    {
        int offset = i * bitmapEntriesPerBlock;
        int entriesToCopy = min(bitmapEntriesPerBlock, fs.superBlock.numBlocks - offset);
        memcpy(fs.dataBlocks[fs.superBlock.bitmapPosition + i].data, fs.bitmap.blocks + offset, entriesToCopy * sizeof(bool));
    }

    SaveFileSystem(fs, fileSystemName);

    // Debug information
    cout << "SuperBlock Position: " << 0 << endl;
    cout << "FAT Position: " << fs.superBlock.fatPosition << endl;
    cout << "Root Directory Position: " << fs.superBlock.rootDirPosition << endl;
    cout << "Filenames Block Position: " << fs.superBlock.fileNamesPosition << endl;
    cout << "Bitmap Position: " << fs.superBlock.bitmapPosition << endl;
    cout << "Data Block Position: " << fs.superBlock.dataBlockPosition << endl;

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

void SaveFileSystem(FileSystem fs, char *fileSystemName)
{
    // cout << "SuperBlock Position: " << 0 << endl;
    // cout << "block size: " << fs.superBlock.blockSize << endl;
    // cout << "FAT Position: " << fs.superBlock.fatPosition << endl;
    // cout << "Root Directory Position: " << fs.superBlock.rootDirPosition << endl;
    // cout << "Data Block Position: " << fs.superBlock.dataBlockPosition << endl;
    //   Attempt to delete the file
    if (std::remove(fileSystemName) != 0)
    {
        std::cerr << "Error deleting file" << std::endl;
    }
    // Write the file system to a file
    ofstream outFile(fileSystemName, ios::binary);
    if (!outFile)
    {
        cerr << "Failed to create file system file!" << endl;
        return;
    }
    // Write super block
    memset(fs.dataBlocks[0].data, 0, fs.superBlock.blockSize);
    memcpy(fs.dataBlocks[0].data, &fs.superBlock, sizeof(SuperBlock));
    // Write FAT
    int fatSizeInBlocks = (fs.superBlock.numBlocks * sizeof(unsigned short) + fs.superBlock.blockSize - 1) / fs.superBlock.blockSize;
    int fatEntriesPerBlock = fs.superBlock.blockSize / sizeof(unsigned short);
    for (int i = 0; i < fatSizeInBlocks; i++)
    {
        int offset = i * fatEntriesPerBlock;
        int entriesToCopy = min(fatEntriesPerBlock, fs.superBlock.numBlocks - offset);
        memset(fs.dataBlocks[fs.superBlock.fatPosition + i].data, 0, fs.superBlock.blockSize);
        memcpy(fs.dataBlocks[fs.superBlock.fatPosition + i].data, fs.fat.blockNum + offset, entriesToCopy * sizeof(unsigned short));
    }

    // No need to write root dir entry it is handled when creating directories

    // Write filenames to data blocks
    for(int i = 0; i < fs.superBlock.numFilenameBlocks;i++){
        memset(fs.dataBlocks[fs.superBlock.fileNamesPosition + i].data, 0, fs.superBlock.blockSize);
        memcpy(fs.dataBlocks[fs.superBlock.fileNamesPosition + i].data, fs.filenames + i * fs.superBlock.blockSize, fs.superBlock.blockSize);
    }

 
   
    // Write bitmap
    int bitmapEntriesPerBlock = fs.superBlock.blockSize / sizeof(bool);
    int bitmapSizeInBlocks = (fs.superBlock.numBlocks * sizeof(bool) + fs.superBlock.blockSize - 1) / fs.superBlock.blockSize;

    for (int i = 0; i < bitmapSizeInBlocks; i++)
    {
        int offset = i * bitmapEntriesPerBlock;
        int entriesToCopy = min(bitmapEntriesPerBlock, fs.superBlock.numBlocks - offset);
        memset(fs.dataBlocks[fs.superBlock.bitmapPosition + i].data, 0, fs.superBlock.blockSize);
        memcpy(fs.dataBlocks[fs.superBlock.bitmapPosition + i].data, fs.bitmap.blocks + offset, entriesToCopy * sizeof(bool));
    }

    // Write each data block to the file
    for (int i = 0; i < fs.superBlock.numBlocks; i++)
    {
        outFile.write(reinterpret_cast<char *>(fs.dataBlocks[i].data), fs.superBlock.blockSize);
    }

    outFile.close();
}

FileSystem LoadFileSystem(char *fileSystemName)
{
    FileSystem fs;
    ifstream inFile(fileSystemName, ios::binary);
    if (!inFile)
    {
        cerr << "Failed to open file system file!" << endl;
        exit(EXIT_FAILURE);
    }

    // Read superblock
    inFile.read(reinterpret_cast<char *>(&fs.superBlock), sizeof(SuperBlock));

    // Read FAT
    int fatStartPos = fs.superBlock.fatPosition * fs.superBlock.blockSize;
    int fatSizeInBlocks = (fs.superBlock.numBlocks * sizeof(unsigned short) + fs.superBlock.blockSize - 1) / fs.superBlock.blockSize;
    int fatStartBlock = fs.superBlock.fatPosition;
    int fatEntriesPerBlock = fs.superBlock.blockSize / sizeof(unsigned short);
    fs.fat.blockNum = new unsigned short[fs.superBlock.numBlocks];

    // Seek to the start position of the FAT block
    inFile.seekg(fatStartPos);
    for (int i = 0; i < fatSizeInBlocks; i++)
    {
        inFile.read(reinterpret_cast<char *>(fs.fat.blockNum + i * fatEntriesPerBlock), fs.superBlock.blockSize);
    }

    int rootDirStart = fs.superBlock.rootDirPosition * fs.superBlock.blockSize;
    inFile.seekg(rootDirStart);
    // Read root directory entry
    inFile.read(reinterpret_cast<char *>(&fs.rootDirEntry), sizeof(DirectoryEntry));
    int filenamesStart = fs.superBlock.fileNamesPosition * fs.superBlock.blockSize;
    
    fs.filenames = new char[fs.superBlock.numFilenameBlocks*fs.superBlock.blockSize];

    inFile.seekg(filenamesStart);
    inFile.read(reinterpret_cast<char *>(fs.filenames),fs.superBlock.numFilenameBlocks*fs.superBlock.blockSize);

    int bitmapStartPos = fs.superBlock.bitmapPosition * fs.superBlock.blockSize;
    int bitmapSizeInBlocks = (fs.superBlock.numBlocks * sizeof(bool) + fs.superBlock.blockSize - 1) / fs.superBlock.blockSize;
    int bitmapStartBlock = fs.superBlock.bitmapPosition;
    int bitmapEntriesPerBlock = fs.superBlock.blockSize / sizeof(bool);
    // Seek to the start position of the bitmap block
    inFile.seekg(bitmapStartPos);

    fs.bitmap.blocks = new bool[fs.superBlock.numBlocks];
    for (int i = 0; i < bitmapSizeInBlocks; i++)
    {
        inFile.read(reinterpret_cast<char *>(fs.bitmap.blocks + i * bitmapEntriesPerBlock), fs.superBlock.blockSize);
    }

    inFile.seekg(0, std::ios::beg);
    fs.dataBlocks = new DataBlock[fs.superBlock.numBlocks];
    for (int i = 0; i < fs.superBlock.numBlocks; i++)
    {
        fs.dataBlocks[i].data = new uint8_t[fs.superBlock.blockSize];
        inFile.read(reinterpret_cast<char *>(fs.dataBlocks[i].data), fs.superBlock.blockSize);
    }

    inFile.close();

    return fs;
}