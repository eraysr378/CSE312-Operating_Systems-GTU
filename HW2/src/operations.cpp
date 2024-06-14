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

void dir(FileSystem fs, char *path,char* password)
{
    char fullPath[strlen(path)+1];
    strcpy(fullPath,path);
    vector<char *> dirs = ExtractPath(path);
    vector<DirectoryEntry> dirEntries = LoadSubdirEntries(fs, fs.rootDirEntry);
    DirectoryEntry parentDirEntry = fs.rootDirEntry;
    DirectoryEntry currentDirEntry = fs.rootDirEntry;
    // Skip root because it is already loaded above
    if (dirs.size() != 1)
    {
        for (int i = 1; i < dirs.size(); i++)
        {

            bool isDirFound = false;
            for (DirectoryEntry dirEntry : dirEntries)
            {
                char *filename = fs.filenames + dirEntry.filenamePos;
                if (strcmp(dirs.at(i), filename) == 0)
                {
                    if (i != dirs.size() - 1 && !dirEntry.isDirectory)
                    {
                        cerr << "Cannot access " << fullPath << ": Not a directory" << endl;
                        return;
                    }
                    // if this is file, then only its information will be printed
                    if (i == dirs.size() - 1 && !dirEntry.isDirectory)
                    {
                        dirEntries.clear();
                        dirEntries.push_back(dirEntry);
                        isDirFound = true;
                        break;
                    }
                    dirEntries = LoadSubdirEntries(fs, dirEntry);

                    parentDirEntry = dirEntry;
                    currentDirEntry = dirEntry;
                    isDirFound = true;
                    break;
                }
            }
            if (!isDirFound)
            {
                cerr << "Invalid directory!" << endl;
                return;
            }
        }
    }
    // check if file has password
    if (currentDirEntry.password[0] != '\0')
    {
        if (!(password != nullptr && strcmp(currentDirEntry.password, password) == 0))
        {
            cerr << "Incorrect password!" << endl;
            return;
        }
    }
    // check read permission of directory
    if (currentDirEntry.attributes[0] != 'r')
    {
        cerr << "Permission denied!" << endl;
        return;
    }
    // print information about each file
    for (DirectoryEntry dirEntry : dirEntries)
    {
        char *filename = fs.filenames + dirEntry.filenamePos;
        struct tm *timeinfo;
        char *lm_date = asctime(localtime(&dirEntry.lm_time));
        for (int i = 0; lm_date[i] != '\0'; i++)
        {
            if (lm_date[i] == '\n')
            {
                lm_date[i] = '\0';
                break;
            }
        }
        cout << filename << " | " << dirEntry.attributes << " | " << lm_date << " | " << dirEntry.fileSize << " | ";
        char *fc_date = asctime(localtime(&dirEntry.fc_time));
        for (int i = 0; fc_date[i] != '\0'; i++)
        {
            if (fc_date[i] == '\n')
            {
                fc_date[i] = '\0';
                break;
            }
        }
        cout << fc_date;
        if (dirEntry.password[0] != '\0')
        {
            cout << " | protected";
        }
        cout << endl;
    }
}

void mkdir(FileSystem fs, char *path,char* password)
{
    char fullPath[strlen(path)+1];
    strcpy(fullPath,path);
    vector<char *> dirs = ExtractPath(path);
    vector<DirectoryEntry> dirEntries = LoadSubdirEntries(fs, fs.rootDirEntry);
    DirectoryEntry parentDirEntry = fs.rootDirEntry;
    DirectoryEntry parentsParentDirEntry = fs.rootDirEntry; // this is needed to change lm_time of the parent

    for (int i = 1; i < dirs.size(); i++)
    {
        // The last file, this is the directory to be created
        if (i == dirs.size() - 1)
        {
            // check if parent has password because we are adding to parent dir
            if (parentDirEntry.password[0] != '\0')
            {
                // check if password is correct or not
                if (!(password != nullptr && strcmp(parentDirEntry.password, password) == 0))
                {
                    cerr << "Incorrect password!" << endl;
                    return;
                }
            }
            // check if parent has write permission
            if (parentDirEntry.attributes[1] != 'w')
            {
                cerr << "Permission denied!" << endl;
                return;
            }
            // Check if a file with the same name exists
            for (DirectoryEntry dir : dirEntries)
            {
                char *filename = fs.filenames + dir.filenamePos;
                if (strcmp(filename, dirs.at(i)) == 0)
                {
                    cerr << "File exists!" << endl;
                    return;
                }
            }
            DirectoryEntry subDirEntry;
            int newBlockNum;
            // allocate block, block num will be saved to newBlockNum variable
            if (!AllocateBlock(fs, newBlockNum))
            {
                cerr << "Failed to allocate a new block for directory entry.\n";
                return;
            }

            subDirEntry.filenamePos = AddFilename(fs, dirs.at(i), strlen(dirs.at(i)) + 1);
            subDirEntry.filenameLen = strlen(dirs.at(i)) + 1;

            char *subDirFilename = fs.filenames + subDirEntry.filenamePos;
            char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;
            // memcpy(subDirEntry.filename, dirs.at(i), strlen(dirs.at(i)) + 1);
            strncpy(subDirEntry.attributes, "rwrwrw", 7);
            subDirEntry.isDirectory = true;
            time_t rawtime;
            time(&rawtime);
            subDirEntry.fc_time = rawtime;
            subDirEntry.fileSize = 0;
            subDirEntry.firstBlockNumber = newBlockNum;
            subDirEntry.lm_time = rawtime;
            strncpy(subDirEntry.password, "\0", 2);
            // update parent's last modification time
            parentDirEntry.lm_time = rawtime;
            if (AddDirEntry(fs, parentDirEntry, subDirEntry))
            {
                cout << subDirFilename << " added to " << parentDirFilename << endl;
                ModifyDirEntry(fs, parentsParentDirEntry, parentDirEntry);
                return;
            }
            else
            {
                cerr << subDirFilename << " could not added to " << parentDirFilename << endl;
                FreeFilename(fs, subDirEntry.filenamePos, subDirEntry.filenameLen);
                return;
            }
        }
        bool isDirFound = false;
        char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;

        for (DirectoryEntry dirEntry : dirEntries)
        {
            char *dirFilename = fs.filenames + dirEntry.filenamePos;

            if (strcmp(dirs.at(i), dirFilename) == 0)
            {
                // If it is not directory give error message
                if (!dirEntry.isDirectory)
                {
                    cerr << "Cannot create " << fullPath << ": Not a directory" << endl;
                    return;
                }
                dirEntries = LoadSubdirEntries(fs, dirEntry);
                parentDirEntry = dirEntry;
                if (i == dirs.size() - 3)
                {
                    parentsParentDirEntry = dirEntry;
                }
                isDirFound = true;
                break;
            }
        }
        if (isDirFound == false)
        {
            cerr << "Invalid path! Directory cannot be created" << endl;
            return;
        }
    }
}

void rmdir(FileSystem fs, char *path,char *password)
{
char fullPath[strlen(path)+1];
    strcpy(fullPath,path);
    vector<char *> dirs = ExtractPath(path);
    if (dirs.size() == 1)
    {
        cerr << "Cannot remove root!" << endl;
    }

    vector<DirectoryEntry> dirEntries = LoadSubdirEntries(fs, fs.rootDirEntry);
    DirectoryEntry parentDirEntry = fs.rootDirEntry;
    DirectoryEntry parentsParentDirEntry = fs.rootDirEntry; // this is needed to change lm_time of the parent
    DirectoryEntry *dirToRemove;
    for (int i = 1; i < dirs.size(); i++)
    {
        bool isDirFound = false;
        char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;

        for (DirectoryEntry dirEntry : dirEntries)
        {
            char *dirFilename = fs.filenames + dirEntry.filenamePos;

            if (strcmp(dirs.at(i), dirFilename) == 0)
            {
                // If it is not directory give error message
                if (!dirEntry.isDirectory)
                {
                    cerr << "Cannot remove " << fullPath << ": Not a directory" << endl;
                    return;
                }
                dirEntries = LoadSubdirEntries(fs, dirEntry);
                // Don't update parent if this is the target directory to remove. We need parent to clear dir entry from data block
                if (i != dirs.size() - 1)
                {
                    parentDirEntry = dirEntry;
                }
                if (i == dirs.size() - 3)
                {
                    parentsParentDirEntry = dirEntry;
                }
                isDirFound = true;
                dirToRemove = &dirEntry;
                break;
            }
        }

        if (i == dirs.size() - 1 && isDirFound)
        {
            // check if parent dir has password
            if (parentDirEntry.password[0] != '\0')
            {
                if (!(password != nullptr && strcmp(parentDirEntry.password, password) == 0))
                {
                    cerr << "Incorrect password!" << endl;
                    return;
                }
            }
            // check if parent dir has write permission
            if(parentDirEntry.attributes[1] != 'w'){
                cerr << "Permission denied!" << endl;
                return;
            }
            // If it is not directory give error message
            if (!dirToRemove->isDirectory)
            {
                cerr << "Cannot remove " << path << ": Not a directory" << endl;
                return;
            }
            dirEntries = LoadSubdirEntries(fs, *dirToRemove);
            if (dirEntries.size() != 0)
            {
                cerr << "Directory is not empty, cannot be removed!" << endl;
                return;
            }
            FreeDirEntry(fs, parentDirEntry, *dirToRemove);
            FreeFilename(fs, dirToRemove->filenamePos, dirToRemove->filenameLen);
            FreeDirBlocks(fs, dirToRemove->firstBlockNumber);
            // update parent's last modification time
            time_t rawtime;
            time(&rawtime);
            parentDirEntry.lm_time = rawtime;
            ModifyDirEntry(fs, parentsParentDirEntry, parentDirEntry);
            cout << "Successfully removed" << endl;
            return;
        }
        if (isDirFound == false)
        {
            cerr << "Invalid path! Directory cannot be removed" << endl;
            return;
        }
    }
}

void dumpe2fs(FileSystem fs)
{
    int blockSize = fs.superBlock.blockSize;

    int blockCount = fs.superBlock.numBlocks;

    int occupiedBlocksCount = 0;
    int freeBlocksCount = 0;

    vector<DirectoryEntry> allDirs;
    allDirs.push_back(fs.rootDirEntry);
    vector<DirectoryEntry> allFiles;
    GetAllDirectoriesAndFiles(fs, fs.rootDirEntry, allDirs, allFiles);
    cout << "Blocks 0 to " << fs.superBlock.dataBlockPosition - 1 << " are reserved by file system." << endl;
    cout << "Superblock, fat, root directory entry, filenames and bitmap." << endl;
    cout << "*****************DIRECTORIES*****************" << endl;
    for (DirectoryEntry dir : allDirs)
    {
        cout << fs.filenames + dir.filenamePos << ": ";
        int curBlock = dir.firstBlockNumber;
        while (curBlock != 0XFFF)
        {
            cout << curBlock << ", ";
            curBlock = fs.fat.blockNum[curBlock];
        }
        cout << endl;
    }
    cout << "*********************************************" << endl
         << endl;
    cout << "********************FILES********************" << endl;
    for (DirectoryEntry file : allFiles)
    {
        cout << fs.filenames + file.filenamePos << ": ";
        int curBlock = file.firstBlockNumber;
        while (curBlock != 0XFFF)
        {
            cout << curBlock << ", ";
            curBlock = fs.fat.blockNum[curBlock];
        }
        cout << endl;
    }
    cout << "*********************************************" << endl;

    // Traverse bitmap
    for (int i = 0; i < blockCount; i++)
    {
        if (fs.bitmap.blocks[i] == FULL)
        {
            occupiedBlocksCount++;
        }
        else
        {
            freeBlocksCount++;
        }
    }

    cout << "Free Blocks: " << freeBlocksCount << endl;

    cout << "Occupied Blocks: " << occupiedBlocksCount << endl;

    cout << "Block Size: " << blockSize << endl;
    cout << "Block Count: " << blockCount << endl;

    cout << "Dir Count: " << allDirs.size() << endl;
    cout << "File Count: " << allFiles.size() << endl;
}
void write(FileSystem fs, char *path, char *linuxFile,char* password)
{
char fullPath[strlen(path)+1];
    strcpy(fullPath,path);
    vector<char *> dirs = ExtractPath(path);
    vector<DirectoryEntry> parentDirs; // when file size changed, parent dirs should be updated as well.
    vector<DirectoryEntry> dirEntries = LoadSubdirEntries(fs, fs.rootDirEntry);
    DirectoryEntry parentDirEntry = fs.rootDirEntry;

    for (int i = 1; i < dirs.size(); i++)
    {
        if (i == dirs.size() - 1)
        {
            // Check if a file with the same name exists
            for (DirectoryEntry dir : dirEntries)
            {
                char *filename = fs.filenames + dir.filenamePos;
                if (strcmp(filename, dirs.at(i)) == 0)
                {
                    // check if file has password
                    if (dir.password[0] != '\0')
                    {
                        if (!(password != nullptr && strcmp(dir.password, password) == 0))
                        {
                            cerr << "Incorrect password!" << endl;
                            return;
                        }
                    }
                    // An existing file will be written, check if file has write permission
                    if(dir.attributes[1] != 'w'){
                        cout << "Permission Denied "<< endl;
                        return;
                    }
                    if (dir.isDirectory)
                    {
                        cerr << "Cannot write to directory" << endl;
                        return;
                    }
                    int bytesWritten = AppendToFile(fs, dir, linuxFile);
                    // update modification time
                    time_t rawtime;
                    time(&rawtime);
                    dir.lm_time = rawtime;
                    dir.fileSize += bytesWritten;
                    parentDirEntry.lm_time = rawtime;        // only the real parent's modified time changes (as in linux)

                    parentDirs.push_back(parentDirEntry);
                    // update all parent directories
                    for (int j = parentDirs.size() - 1; j > 0; j--)
                    {
                        parentDirs.at(j).fileSize += bytesWritten; // real parent is not added to parentDirs
                        ModifyDirEntry(fs, parentDirs.at(j - 1), parentDirs.at(j));
                    }
                    ModifyDirEntry(fs, parentDirEntry, dir);
                    return;
                }
            }
            // check if parent dir has password because new file will be added
            if (parentDirEntry.password[0] != '\0')
            {
                if (!(password != nullptr && strcmp(parentDirEntry.password, password) == 0))
                {
                    cerr << "Incorrect password!" << endl;
                    return;
                }
            }
            // new file will be created, check if parent dir has write permission
            if(parentDirEntry.attributes[1] != 'w'){
                cerr << "Permission Denied" << endl;
                return;
            }
            DirectoryEntry subDirEntry;
            int newBlockNum;
            if (!AllocateBlock(fs, newBlockNum))
            {
                cerr << "Failed to allocate a new block for directory entry.\n";
                return;
            }

            subDirEntry.filenamePos = AddFilename(fs, dirs.at(i), strlen(dirs.at(i)) + 1);
            subDirEntry.filenameLen = strlen(dirs.at(i)) + 1;

            char *subDirFilename = fs.filenames + subDirEntry.filenamePos;
            char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;
            struct stat fileStat;
            if (stat(linuxFile, &fileStat) < 0)
            {
                perror("stat");
                exit(EXIT_FAILURE);
            }
            CopyFileAttributes(fileStat, subDirEntry.attributes);
            subDirEntry.isDirectory = false;
            time_t rawtime;
            time(&rawtime);
            subDirEntry.fc_time = rawtime;
            subDirEntry.fileSize = 0;
            subDirEntry.firstBlockNumber = newBlockNum;
            subDirEntry.lm_time = rawtime;
            strncpy(subDirEntry.password, "\0", 2);

            int bytesWritten = AppendToFile(fs, subDirEntry, linuxFile);
            subDirEntry.fileSize += bytesWritten;
            parentDirEntry.lm_time = rawtime;

            parentDirs.push_back(parentDirEntry);
            // update all parent directories size
            for (int j = parentDirs.size() - 1; j > 0; j--)
            {
                parentDirs.at(j).fileSize += bytesWritten; // real parent is not added to parentDirs
                ModifyDirEntry(fs, parentDirs.at(j - 1), parentDirs.at(j));
            }

            if (AddDirEntry(fs, parentDirEntry, subDirEntry))
            {
                cout << subDirFilename << " added to " << parentDirFilename << endl;
                return;
            }
            else
            {
                cerr << subDirFilename << " could not added to " << parentDirFilename << endl;
                FreeFilename(fs, subDirEntry.filenamePos, subDirEntry.filenameLen);
                return;
            }
        }
        bool isDirFound = false;
        char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;
        parentDirs.push_back(parentDirEntry);
        for (DirectoryEntry dirEntry : dirEntries)
        {
            char *dirFilename = fs.filenames + dirEntry.filenamePos;

            if (strcmp(dirs.at(i), dirFilename) == 0)
            {
                // If it is not directory give error message
                if (!dirEntry.isDirectory)
                {
                    cerr << "Cannot create " << fullPath << ": Not a directory" << endl;
                    return;
                }
                dirEntries = LoadSubdirEntries(fs, dirEntry);
                parentDirEntry = dirEntry;
                isDirFound = true;
                break;
            }
        }
        if (isDirFound == false)
        {
            cerr << "Invalid path! Directory cannot be created" << endl;
            return;
        }
    }
}

void read(FileSystem fs, char *path, char *linuxFile, char *password)
{
    char fullPath[strlen(path)+1];
    strcpy(fullPath,path);
    vector<char *> dirs = ExtractPath(path);

    vector<DirectoryEntry> dirEntries = LoadSubdirEntries(fs, fs.rootDirEntry);
    DirectoryEntry parentDirEntry = fs.rootDirEntry;
    DirectoryEntry fileEntry;

    for (int i = 1; i < dirs.size(); i++)
    {
        bool isDirFound = false;
        char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;

        for (DirectoryEntry dirEntry : dirEntries)
        {
            char *dirFilename = fs.filenames + dirEntry.filenamePos;
            if (strcmp(dirs.at(i), dirFilename) == 0)
            {
                // file to read is found
                if (i == dirs.size() - 1)
                {
                   
                    // check if directory
                    if (dirEntry.isDirectory)
                    {
                        cerr << "Cannot read directory" << endl;
                        return;
                    }
                     // check if file has password
                    if (dirEntry.password[0] != '\0')
                    {
                        if (!(password != nullptr && strcmp(dirEntry.password, password) == 0))
                        {
                            cerr << "Incorrect password!" << endl;
                            return;
                        }
                    }
                    // check if read permission is granted
                    if (dirEntry.attributes[0] != 'r')
                    {
                        cerr << "Permission denied" << endl;
                        return;
                    }
                  
                    if (CopyDirEntryContent(fs, dirEntry.firstBlockNumber, linuxFile, dirEntry.attributes) == 0)
                    {
                        cout << "File read successfully" << endl;
                    }
                    else
                    {

                        cerr << "File couldnt read" << endl;
                        return;
                    }
                    return;
                }
                // If it is not directory give error message
                if (!dirEntry.isDirectory)
                {
                    cerr << "Cannot read " << fullPath << ": Not a directory" << endl;
                    return;
                }
                dirEntries = LoadSubdirEntries(fs, dirEntry);
                parentDirEntry = dirEntry;
                isDirFound = true;
                break;
            }
        }
        if (isDirFound == false)
        {
            cerr << "Invalid path! Directory cannot be created" << endl;
            return;
        }
    }
}

void del(FileSystem fs, char *path, char *password)
{
    char fullPath[strlen(path)+1];
    strcpy(fullPath,path);
    vector<char *> dirs = ExtractPath(path);
    if (dirs.size() == 1)
    {
        cerr << "Cannot modify root!" << endl;
    }

    vector<DirectoryEntry> dirEntries = LoadSubdirEntries(fs, fs.rootDirEntry);
    vector<DirectoryEntry> parentDirs;
    DirectoryEntry parentDirEntry = fs.rootDirEntry;
    parentDirs.push_back(parentDirEntry); // push root
    DirectoryEntry *fileToRemove;
    for (int i = 1; i < dirs.size(); i++)
    {

        bool isDirFound = false;
        char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;

        // search for file till you find the matching one
        for (DirectoryEntry dirEntry : dirEntries)
        {
            char *dirFilename = fs.filenames + dirEntry.filenamePos;

            if (strcmp(dirs.at(i), dirFilename) == 0)
            {
                // If filepath includes non directory file other than the one will be deleted, give error message
                if (i != dirs.size() - 1 && !dirEntry.isDirectory)
                {
                    cerr << "Cannot remove " << fullPath << ": Not a directory" << endl;
                    return;
                }
                dirEntries = LoadSubdirEntries(fs, dirEntry);
                // Don't update parent if this is the target file to remove. We need parent to clear dir entry from data block
                if (i != dirs.size() - 1)
                {
                    parentDirEntry = dirEntry;
                    parentDirs.push_back(parentDirEntry);
                }
                isDirFound = true;
                fileToRemove = &dirEntry;
                break;
            }
        }

        if (i == dirs.size() - 1 && isDirFound)
        {
            // If it is directory give error message
            if (fileToRemove->isDirectory)
            {
                cerr << "Cannot remove " << path << ": is a directory" << endl;
                return;
            }
            // if parent directory does not have write permission, then file cannot be deleted
            if(parentDirEntry.attributes[1] != 'w'){
                cerr << "Permission Denied" << endl;
                return;
            }
            // check if file has password
            if (fileToRemove->password[0] != '\0')
            {
                if (!(password != nullptr && strcmp(fileToRemove->password, password) == 0))
                {
                    cerr << "Incorrect password!" << endl;
                    return;
                }
            }
            // free the used memory by the file
            FreeDirEntry(fs, parentDirEntry, *fileToRemove);
            FreeFilename(fs, fileToRemove->filenamePos, fileToRemove->filenameLen);
            FreeDirBlocks(fs, fileToRemove->firstBlockNumber);
            time_t rawtime;
            time(&rawtime);
            parentDirEntry.lm_time = rawtime; // only real parent's modified time changes as in linux
            for (int j = parentDirs.size() - 1; j > 0; j--)
            {
                parentDirs.at(j).fileSize -= fileToRemove->fileSize; // file is removed update file size

                ModifyDirEntry(fs, parentDirs.at(j - 1), parentDirs.at(j));
            }
            cout << fileToRemove->fileSize << " Bytes successfully removed" << endl;
            return;
        }
        if (isDirFound == false)
        {
            cerr << "Invalid path! File cannot be deleted" << endl;
            return;
        }
    }
}

void chmod(FileSystem fs, char *path, char *attributes)
{
    char fullPath[strlen(path)+1];
    strcpy(fullPath,path);
    vector<char *> dirs = ExtractPath(path);
    if (dirs.size() == 1)
    {
        cerr << "Cannot change root!" << endl;
    }

    vector<DirectoryEntry> dirEntries = LoadSubdirEntries(fs, fs.rootDirEntry);
    DirectoryEntry parentDirEntry = fs.rootDirEntry;
    DirectoryEntry *fileToModify;
    for (int i = 1; i < dirs.size(); i++)
    {

        bool isDirFound = false;
        char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;
        // search for file till you find the matching one
        for (DirectoryEntry dirEntry : dirEntries)
        {
            char *dirFilename = fs.filenames + dirEntry.filenamePos;

            if (strcmp(dirs.at(i), dirFilename) == 0)
            {
                // If filepath includes non directory file other than the one will be deleted, give error message
                if (i != dirs.size() - 1 && !dirEntry.isDirectory)
                {
                    cerr << "Cannot access " << fullPath << ": Not a directory" << endl;
                    return;
                }
                dirEntries = LoadSubdirEntries(fs, dirEntry);
                // Don't update parent if this is the target directory to remove. We need parent to clear dir entry from data block
                if (i != dirs.size() - 1)
                {
                    parentDirEntry = dirEntry;
                }
                isDirFound = true;
                fileToModify = &dirEntry;
                break;
            }
        }

        if (i == dirs.size() - 1 && isDirFound)
        {
            int attributesSize = strlen(attributes) + 1;
            if (attributes[0] == '-')
            {
                if (attributesSize > 2 && attributes[1] == 'r')
                {
                    fileToModify->attributes[0] = '-';
                    fileToModify->attributes[2] = '-';
                    fileToModify->attributes[4] = '-';
                }
                if (attributesSize > 2 && attributes[1] == 'w')
                {
                    fileToModify->attributes[1] = '-';
                    fileToModify->attributes[3] = '-';
                    fileToModify->attributes[5] = '-';
                }
                if (attributesSize > 3 && attributes[2] == 'w')
                {
                    fileToModify->attributes[1] = '-';
                    fileToModify->attributes[3] = '-';
                    fileToModify->attributes[5] = '-';
                }
            }
            else if (attributes[0] == '+')
            {
                if (attributesSize > 2 && attributes[1] == 'r')
                {
                    fileToModify->attributes[0] = 'r';
                    fileToModify->attributes[2] = 'r';
                    fileToModify->attributes[4] = 'r';
                }
                if (attributesSize > 2 && attributes[1] == 'w')
                {
                    fileToModify->attributes[1] = 'w';
                    fileToModify->attributes[3] = 'w';
                    fileToModify->attributes[5] = 'w';
                }
                if (attributesSize > 3 && attributes[2] == 'w')
                {
                    fileToModify->attributes[1] = 'w';
                    fileToModify->attributes[3] = 'w';
                    fileToModify->attributes[5] = 'w';
                }
            }
            ModifyDirEntry(fs, parentDirEntry, *fileToModify);

            cout << "Successfully permissions changed" << endl;
            return;
        }
        if (isDirFound == false)
        {
            cerr << "Invalid path! File cannot be modified" << endl;
            return;
        }
    }
}

void addpw(FileSystem fs, char *path, char *password)
{
    char fullPath[strlen(path)+1];
    strcpy(fullPath,path);
    vector<char *> dirs = ExtractPath(path);
    if (dirs.size() == 1)
    {
        cerr << "Cannot modify root!" << endl;
    }

    vector<DirectoryEntry> dirEntries = LoadSubdirEntries(fs, fs.rootDirEntry);
    DirectoryEntry parentDirEntry = fs.rootDirEntry;
    DirectoryEntry *fileToModify;
    for (int i = 1; i < dirs.size(); i++)
    {

        bool isDirFound = false;
        char *parentDirFilename = fs.filenames + parentDirEntry.filenamePos;
        // search for file till you find the matching one
        for (DirectoryEntry dirEntry : dirEntries)
        {
            char *dirFilename = fs.filenames + dirEntry.filenamePos;

            if (strcmp(dirs.at(i), dirFilename) == 0)
            {
                // If filepath includes non directory file other than the one will be deleted, give error message
                if (i != dirs.size() - 1 && !dirEntry.isDirectory)
                {
                    cerr << "Cannot access " << fullPath << ": Not a directory" << endl;
                    return;
                }
                dirEntries = LoadSubdirEntries(fs, dirEntry);
                // Don't update parent if this is the target directory to remove. We need parent to clear dir entry from data block
                if (i != dirs.size() - 1)
                {
                    parentDirEntry = dirEntry;
                }
                isDirFound = true;
                fileToModify = &dirEntry;
                break;
            }
        }

        if (i == dirs.size() - 1 && isDirFound)
        {
            strcpy(fileToModify->password, password);
            ModifyDirEntry(fs, parentDirEntry, *fileToModify);

            cout << "Successfully permissions changed" << endl;
            return;
        }
        if (isDirFound == false)
        {
            cerr << "Invalid path! File cannot be modified" << endl;
            return;
        }
    }
}
