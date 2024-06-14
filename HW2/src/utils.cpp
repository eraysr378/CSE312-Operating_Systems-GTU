#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <string.h>
#include <sys/stat.h>
#include <cstdlib>

#include "../include/fs.h"
#include "../include/utils.h"

using namespace std;

void PrintDir(DirectoryEntry dir)
{
    cout << "dir printing: " << endl;
    cout << dir.attributes << endl;
    cout << dir.fc_time << endl;
    cout << "is dir:" << dir.isDirectory << endl;
    // cout << dir.filename << endl;
    cout << dir.fileSize << endl;
    cout << dir.firstBlockNumber << endl;
    cout << dir.lm_time << endl;
    cout << dir.password << endl;
}
void PrintBitmap(FileSystem fs)
{

    int bitmapSize = fs.superBlock.numBlocks;
    for (int i = 0; i < bitmapSize; i++)
    {
        cout << fs.bitmap.blocks[i];
    }
}

// Function to find the first free block using the bitmap
int FindFirstFreeBlock(FileSystem fs)
{
    for (int i = 0; i < fs.superBlock.numBlocks; i++)
    {
        if (fs.bitmap.blocks[i] == FREE)
        {
            return i;
        }
    }
    return -1; // No free blocks available
}

// Function to allocate a block and update FAT and bitmap, newly allocated blocks num is saved to blockNum parameter
bool AllocateBlock(FileSystem fs, int &blockNum)
{
    blockNum = FindFirstFreeBlock(fs);
    if (blockNum == -1)
    {
        cerr << "No free blocks available.\n";
        return false;
    }

    fs.bitmap.blocks[blockNum] = FULL; // Mark the block as used in the bitmap
    fs.fat.blockNum[blockNum] = 0xFFF; // Mark the new block as the end of the chain
    memset(fs.dataBlocks[blockNum].data, 0, fs.superBlock.blockSize);

    return true;
}
// Load sub directory entries of given dirEntry by traversing its data blocks
vector<DirectoryEntry> LoadSubdirEntries(FileSystem fs, DirectoryEntry dirEntry)
{
    vector<DirectoryEntry> entries;
    if (dirEntry.isDirectory == false)
    {
        return entries;
    }
    const int entrySize = sizeof(DirectoryEntry);
    const int blockSize = fs.superBlock.blockSize;

    vector<int> blocks;
    blocks.push_back(dirEntry.firstBlockNumber);
    int blockNum = dirEntry.firstBlockNumber;
    while (fs.fat.blockNum[blockNum] != 0xfff)
    {
        blockNum = fs.fat.blockNum[blockNum];
        blocks.push_back(blockNum);
    }
    for (int block : blocks)
    {
        uint8_t *rawData = fs.dataBlocks[block].data;

        int numEntries = 0;

        for (int i = 0; i < blockSize; i += entrySize)
        {
            DirectoryEntry *entry = reinterpret_cast<DirectoryEntry *>(rawData + i);
            if (entry->attributes[0] == '\0')
            { // Assuming empty attributes indicates a free entry
                numEntries = i / entrySize;
                break;
            }
        }
        for (int i = 0; i < numEntries; ++i)
        {
            const uint8_t *entryData = rawData + (i * entrySize);
            DirectoryEntry entry;
            memcpy(&entry, entryData, entrySize);
            entries.push_back(entry);
        }
    }
    // Root dir entry is stored at the start of root dir entry, so that remove it from the list
    if (dirEntry.filenamePos == fs.rootDirEntry.filenamePos)
    {
        entries.erase(entries.begin());
    }
    return entries;
}
// Parse the path and store the directory names in a vector
vector<char *> ExtractPath(char *path)
{

    if (path == nullptr || path[0] != '/')
    {
        cerr << "Invalid path\n";
        exit(EXIT_FAILURE);
    }
    vector<char *> dirs;
    dirs.push_back("/");

    const char delimiter[] = "/";
    char *token = strtok(path, delimiter);
    while (token != nullptr)
    {
        dirs.push_back(token);
        token = strtok(nullptr, delimiter);
    }
    return dirs;
}
// Add given subDir entry to the directory entries of the parentDir
bool AddDirEntry(FileSystem fs, DirectoryEntry &parentDir, const DirectoryEntry &subDir)
{

    int lastBlockNum = parentDir.firstBlockNumber;
    const int entrySize = sizeof(DirectoryEntry);
    int blockSize = fs.superBlock.blockSize;

    // Find the last block of the parent directory
    while (fs.fat.blockNum[lastBlockNum] != 0xfff)
    {
        lastBlockNum = fs.fat.blockNum[lastBlockNum];
    }

    uint8_t *rawData = fs.dataBlocks[lastBlockNum].data;

    // Find available space in the last block
    int availableSpace = 0;
    int index = -1;
    int entryCount = 0;
    for (int i = 0; i < blockSize; i += entrySize)
    {
        DirectoryEntry *entry = reinterpret_cast<DirectoryEntry *>(rawData + i);
        if (entry->attributes[0] == '\0')
        { // Assuming empty attributes indicates a free entry
            availableSpace = blockSize - i;
            index = i;
            entryCount++;
            break;
        }
    }
    if (availableSpace >= entrySize)
    {
        memcpy(rawData + index, &subDir, entrySize);
        return true;
    }
    else
    {
        // Allocate a new block
        int newBlockNum;
        if (AllocateBlock(fs, newBlockNum))
        {
            fs.fat.blockNum[lastBlockNum] = newBlockNum; // Link the new block to the chain
            memcpy(fs.dataBlocks[newBlockNum].data, &subDir, sizeof(DirectoryEntry));
        }
        else
        {
            cerr << "Failed to allocate a new block for directory entry.\n";
            return false;
        }

        // Update FAT
        fs.fat.blockNum[lastBlockNum] = newBlockNum;
        fs.fat.blockNum[newBlockNum] = 0xfff;

        return true;
    }
}
// Add given file name to filenames is filesystem and return the position of the added filename
int AddFilename(FileSystem fs, char *filename, int size)
{
    int filenamesBufferSize = fs.superBlock.numFilenameBlocks * fs.superBlock.blockSize;
    for (int i = 0; i < filenamesBufferSize; i++)
    {
        // make sure the chosen place is empty
        if (fs.filenames[i] == '\0' && fs.filenames[i + 1] == '\0')
        {
            bool isEnoughSpace = true;
            // Make sure you are not overwriting another filename
            for (int j = 0; j < size; j++)
            {
                if (fs.filenames[i + 1 + j] != '\0')
                {
                    isEnoughSpace = false;
                    break;
                }
            }
            if (isEnoughSpace)
            {
                memcpy(fs.filenames + (i + 1), filename, size);
                return i + 1;
            }
        }
    }
    cerr << "File name couldnt added" << endl;
    return -1;
}
void FreeDataBlock(FileSystem &fs, int blockNum)
{
    memset(fs.dataBlocks[blockNum].data, 0, fs.superBlock.blockSize);
    fs.fat.blockNum[blockNum] = 0xFFF;
    fs.bitmap.blocks[blockNum] = FREE;
}
// Clear the data blocks that belong to the removed directory
int FreeDirBlocks(FileSystem &fs, int firstBlockNum)
{
    int deletedBlockCount = 0;
    int curBlock = firstBlockNum;
    while (curBlock != 0XFFF)
    {
        int blockToDelete = curBlock;
        curBlock = fs.fat.blockNum[curBlock];

        FreeDataBlock(fs, blockToDelete);

        deletedBlockCount++;
    }
    cout << deletedBlockCount << " blocks deleted" << endl;
    return deletedBlockCount;
}
// remove filename from the filesystem buffer
void FreeFilename(FileSystem &fs, int filenamePos, int filenameLen)
{
    cout << "Filename deleted:" << fs.filenames + filenamePos << endl;

    memset(fs.filenames + filenamePos, 0, filenameLen);
}
// Remove dir entry from parentDirEntry data blocks
void FreeDirEntry(FileSystem &fs, DirectoryEntry parentDirEntry, DirectoryEntry dirEntryToRemove)
{
    int entrySize = sizeof(DirectoryEntry);
    int blockSize = fs.superBlock.blockSize;
    int currentBlockNum = parentDirEntry.firstBlockNumber;

    while (currentBlockNum != 0xFFF)
    {
        for (int i = 0; i < blockSize; i += entrySize)
        {
            DirectoryEntry *entry = reinterpret_cast<DirectoryEntry *>(fs.dataBlocks[currentBlockNum].data + i);
            if (entry->filenamePos == dirEntryToRemove.filenamePos) // compare filenames
            {
                memset((fs.dataBlocks[currentBlockNum].data + i), 0, entrySize);
                CompactDirEntries(fs, parentDirEntry);
                cout << "Dir entry removed" << endl;
                return;
            }
        }
        currentBlockNum = fs.fat.blockNum[currentBlockNum];
    }
}

// Modify the DirectoryEntry in data block, directory entries stored in parent directory's data block, therefore we should give parent directory as parameter
void ModifyDirEntry(FileSystem &fs, DirectoryEntry parentDirEntry, DirectoryEntry dirEntryToModify)
{
    int entrySize = sizeof(DirectoryEntry);
    int blockSize = fs.superBlock.blockSize;
    int currentBlockNum = parentDirEntry.firstBlockNumber;
    // Find the entry in parent's data blocks
    while (currentBlockNum != 0xFFF)
    {
        for (int i = 0; i < blockSize; i += entrySize)
        {
            DirectoryEntry *entry = reinterpret_cast<DirectoryEntry *>(fs.dataBlocks[currentBlockNum].data + i);
            if (entry->filenamePos == dirEntryToModify.filenamePos) // compare filenames
            {
                // Update the entry in data block
                memcpy(entry->attributes, dirEntryToModify.attributes, 7);
                memcpy(entry->password, dirEntryToModify.password, strlen(dirEntryToModify.password) + 1);

                entry->lm_time = dirEntryToModify.lm_time;
                entry->fileSize = dirEntryToModify.fileSize;

                cout << "Dir entry modified" << endl;
                return;
            }
        }
        currentBlockNum = fs.fat.blockNum[currentBlockNum];
    }
}
// when an entry removed, its place left empty, move other entries to fill empty space
void CompactDirEntries(FileSystem fs, DirectoryEntry dirEntry)
{
    int entrySize = sizeof(DirectoryEntry);
    int blockSize = fs.superBlock.blockSize;
    int currentBlockNum = dirEntry.firstBlockNumber;

    while (currentBlockNum != 0xFFF)
    {
        int lastEntry = -1;
        for (int i = 0; i < blockSize - 2 * entrySize; i += entrySize)
        {
            DirectoryEntry *entry = reinterpret_cast<DirectoryEntry *>(fs.dataBlocks[currentBlockNum].data + i);
            if (entry->attributes[0] == '\0') // if attributes null, then there is empty place
            {
                // move the next entry to the empty place
                memcpy(fs.dataBlocks[currentBlockNum].data + i, fs.dataBlocks[currentBlockNum].data + i + entrySize, entrySize);
                // entry moved, its place is empty
                memset(fs.dataBlocks[currentBlockNum].data + i + entrySize, 0, entrySize);
            }
            lastEntry = i + entrySize;
        }
        // Previous block's last entry is empty, we should move the next entry from next block if it exists
        int prevBlockNum = currentBlockNum;
        currentBlockNum = fs.fat.blockNum[currentBlockNum];
        if (currentBlockNum != 0XFFF)
        {
            memcpy(fs.dataBlocks[prevBlockNum].data + lastEntry, fs.dataBlocks[currentBlockNum].data, entrySize);
            memset(fs.dataBlocks[currentBlockNum].data, 0, entrySize);
            DirectoryEntry *entry = reinterpret_cast<DirectoryEntry *>(fs.dataBlocks[currentBlockNum].data + entrySize);
            // if attributes null, then there is empty place which means the block is empty
            if (entry->attributes[0] == '\0')
            {
                FreeDataBlock(fs, currentBlockNum);
                break;
            }
        }
        else
        {
            break;
        }
    }
}
// Get all directories and files in the filesystem
void GetAllDirectoriesAndFiles(FileSystem fs, DirectoryEntry dir, vector<DirectoryEntry> &allDirs, vector<DirectoryEntry> &allFiles)
{

    vector<DirectoryEntry> subdirs = LoadSubdirEntries(fs, dir);
    if (subdirs.size() == 0)
    {
        return;
    }
    for (const auto &subdir : subdirs)
    {
        if (subdir.filenamePos == fs.rootDirEntry.filenamePos)
        {
            continue;
        }
        if (subdir.isDirectory)
        {
            allDirs.push_back(subdir);
            GetAllDirectoriesAndFiles(fs, subdir, allDirs, allFiles);
        }
        else
        {
            allFiles.push_back(subdir);
        }
    }
}
// To copy attributes from linux file to DirectoryEntry, we should convert it because DirectoryEntr stores attributes in char*
void CopyFileAttributes(struct stat fileStat, char *attributes)
{
    ((fileStat.st_mode & S_IRUSR) ? attributes[0] = 'r' : attributes[0] = '-');
    ((fileStat.st_mode & S_IWUSR) ? attributes[1] = 'w' : attributes[1] = '-');
    ((fileStat.st_mode & S_IRGRP) ? attributes[2] = 'r' : attributes[2] = '-');
    ((fileStat.st_mode & S_IWGRP) ? attributes[3] = 'w' : attributes[3] = '-');
    ((fileStat.st_mode & S_IROTH) ? attributes[4] = 'r' : attributes[4] = '-');
    ((fileStat.st_mode & S_IWOTH) ? attributes[5] = 'w' : attributes[5] = '-');
    attributes[6] = '\0';
}
// DirectoryEntry stores permissions in char* so we should convert it to change linux file's permissions
vector<int> ConvertDirEntryAttributes(char *attributes)
{
    vector<int> permissions;

    permissions.push_back(attributes[0] == 'r' ? S_IRUSR : 0);

    permissions.push_back(attributes[1] == 'w' ? S_IWUSR : 0);

    permissions.push_back(attributes[2] == 'r' ? S_IRGRP : 0);

    permissions.push_back(attributes[3] == 'w' ? S_IWGRP : 0);

    permissions.push_back(attributes[4] == 'r' ? S_IROTH : 0);

    permissions.push_back(attributes[5] == 'w' ? S_IWOTH : 0);

    return permissions;
}

// I decided not to use this function instead AppendFile used
// int CopyFileContent(FileSystem fs, int startBlock, char *linuxFile)
// {
//     ifstream file(linuxFile, std::ios::binary);
//     if (!file)
//     {
//         cerr << "Error: Could not open file " << linuxFile << endl;
//         return EXIT_FAILURE;
//     }
//     char buffer[fs.superBlock.blockSize];
//     memset(buffer, 0, fs.superBlock.blockSize);
//     int currentBlock = startBlock;
//     while (file.read(buffer, fs.superBlock.blockSize) || file.gcount() > 0)
//     {
//         streamsize bytesRead = file.gcount();
//         memcpy(fs.dataBlocks[currentBlock].data, buffer, bytesRead);
//         if (file.eof())
//         {
//             break;
//         }
//         int nextBlock;
//         AllocateBlock(fs, nextBlock);
//         fs.fat.blockNum[currentBlock] = nextBlock;
//         currentBlock = nextBlock;
//     }
//     if (file.eof())
//     {
//         cout << "\nEnd of file reached." << endl;
//     }
//     else if (file.fail())
//     {
//         cerr << "Error: Reading error occurred." << endl;
//     }
//     file.close();
//     return 0;
// }

// Copy dir entry content to given file,if file does not exist, then create and make its permissions the same as given directory entry 
int CopyDirEntryContent(FileSystem fs, int startBlock, char *linuxFile, char *attributes)
{
    bool isCreated = false;
    ifstream file(linuxFile);
    // Check if file already exists
    if (file.good())
    {
        struct stat fileStat;
        if (stat(linuxFile, &fileStat) < 0)
        {
            perror("stat");
            exit(EXIT_FAILURE);
        }
        // check if owner have write permission
        if (!(fileStat.st_mode & S_IWUSR))
        {
            cerr << "permission denied!" << endl;
            return EXIT_FAILURE;
        }
    }
    else
    {
        isCreated = true;
    }
    ofstream outFile(linuxFile, ios::binary);
    if (!outFile)
    {
        cerr << "Error: Could not open file " << linuxFile << endl;
        return EXIT_FAILURE;
    }
    // if file does not exist already, then it should have the same permissions as the file it is copied from
    if (isCreated)
    {
        vector<int> permissions = ConvertDirEntryAttributes(attributes);
        // add new permissions
        mode_t newPermissions = permissions.at(0) | permissions.at(1) | permissions.at(2) | permissions.at(3) | permissions.at(4) | permissions.at(5);
        if (chmod(linuxFile, newPermissions) == 0)
        {
            cout << "File permissions changed successfully." << endl;
        }
        else
        {
            cerr << "Error: Unable to change file permissions." << endl;
            return EXIT_FAILURE;
        }
    }

    int currentBlock = startBlock;
    // traverse all blocks that belong to the file
    while (currentBlock != 0xFFF)
    {
        int nextBlock = fs.fat.blockNum[currentBlock];
        // if there is next block, then this block is full so copy it directly.
        if (nextBlock != 0xFFF)
        {
            outFile.write((char *)fs.dataBlocks[currentBlock].data, fs.superBlock.blockSize);
        }
        // if this is the last block, then it may not be full, so do not copy everything.
        else
        {
            for (int i = 0; i < fs.superBlock.blockSize; i++)
            {
                if (fs.dataBlocks[currentBlock].data[i] == 0)
                {
                    outFile.write(reinterpret_cast<char *>(fs.dataBlocks[currentBlock].data), i);
                    break;
                }
            }
        }
        currentBlock = nextBlock;
    }

    return 0;
}
// Append the linux file content to fileEntry, return the count of written bytes
int AppendToFile(FileSystem fs, DirectoryEntry fileEntry, char *linuxFile)
{
    ifstream inFile(linuxFile, ios::binary);
    if (!inFile)
    {
        cerr << "Failed to open " << linuxFile << endl;
        exit(EXIT_FAILURE);
    }

    int currentBlock = fileEntry.firstBlockNumber;
    int index = 0;
    // find last block
    while (fs.fat.blockNum[currentBlock] != 0XFFF)
    {
        currentBlock = fs.fat.blockNum[currentBlock];
    }
    // find the start of the empty place in the last block
    for (int i = 0; i < fs.superBlock.blockSize; i++)
    {
        if (fs.dataBlocks[currentBlock].data[i] == '\0')
        {
            index = i;
            break;
        }
    }
    char c[2];
    int bytesRead = 0;
    // Read until the end of file
    while (1)
    {
        inFile.read(c, 1);
         if(inFile.eof()){
            break;
        }
        bytesRead++;
        if (index >= fs.superBlock.blockSize)
        {
            int nextBlock;
            AllocateBlock(fs, nextBlock);
            fs.fat.blockNum[currentBlock] = nextBlock;
            currentBlock = nextBlock;
            index = 0;
        }
        fs.dataBlocks[currentBlock].data[index++] = c[0];
       
    }
    cout << bytesRead << " Bytes written to file succesfully" << endl;
    return bytesRead;
}
// read permission yoksa read operation gerçekleşmemesi lazım .
// read permission yoksa read operation gerçekleşmemesi lazım .
// read permission yoksa read operation gerçekleşmemesi lazım .
// read permission yoksa read operation gerçekleşmemesi lazım .
// file size will be added
// file size will be added
// file size will be added
// file size will be added
// read permission yoksa read operation gerçekleşmemesi lazım .