#ifndef FS_H
#define FS_H


#include "types.h"


void CreateFileSystem(char* fileSystemName,int blockSize);

void SaveFileSystem(FileSystem fs,char* fileSystemName);

FileSystem LoadFileSystem(char* fileSystemName);

void HandleOperation(int argc,  char *argv[]);

#endif