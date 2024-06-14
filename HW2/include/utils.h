#ifndef UTILS_H
#define UTILS_H


#include "fs.h"
#include <vector>
using namespace std;


void PrintDir(DirectoryEntry dir);
void PrintBitmap(FileSystem fs);

// Function to find the first free block using the bitmap
int FindFirstFreeBlock(FileSystem fs) ;

// Function to allocate a block and update FAT and bitmap
bool AllocateBlock(FileSystem fs,int &blockNum);

void lf();
vector<DirectoryEntry> LoadSubdirEntries(FileSystem fs, DirectoryEntry dirEntry);

vector<char*> ExtractPath(char* path);
bool AddDirEntry(FileSystem fs, DirectoryEntry& parentDir, const DirectoryEntry& subDir) ;



int AddFilename(FileSystem fs, char* filename,int size);
void FreeDataBlock(FileSystem &fs,int blockNum);

int FreeDirBlocks(FileSystem &fs,int firstBlockNum);
void FreeFilename(FileSystem &fs,int filenamePos, int filenameLen);
void FreeDirEntry(FileSystem &fs,DirectoryEntry parentDirEntry,DirectoryEntry dirEntryToRemove);
void ModifyDirEntry(FileSystem &fs, DirectoryEntry parentDirEntry, DirectoryEntry dirEntryToModify);

void CompactDirEntries(FileSystem fs,DirectoryEntry dirEntry);

void GetAllDirectoriesAndFiles(FileSystem fs, DirectoryEntry dir,vector<DirectoryEntry>& allDirs,vector<DirectoryEntry>& allFiles);
void CopyFileAttributes(struct stat fileStat, char *attributes);
int CopyFileContent(FileSystem fs, int startBlock, char* linuxFile);
int CopyDirEntryContent(FileSystem fs, int startBlock,char* linuxFile,char* attributes);
int AppendToFile(FileSystem fs,DirectoryEntry fileEntry,char* linuxFile);

#endif