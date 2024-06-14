#ifndef OPERATIONS_H
#define OPERATIONS_H
#include "fs.h"
#include "utils.h"
using namespace std;

void dir(FileSystem fs, char *path, char *password);

void mkdir(FileSystem fs, char *path, char *password);

void rmdir(FileSystem fs, char *path, char *password);

void dumpe2fs(FileSystem fs);

void write(FileSystem fs, char* path,char* linuxFile, char *password);

void read(FileSystem fs, char *path, char *linuxFile,char* password);

void del(FileSystem fs, char *path,char* password);

void chmod(FileSystem fs, char *path, char * attributes);

void addpw(FileSystem fs, char *path, char *password);

#endif