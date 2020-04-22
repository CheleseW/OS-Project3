#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "instruction.h"

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F

struct DIRENTRY {

    unsigned char DIR_Name[13];
    unsigned char DIR_Attr;
    unsigned char DIR_NTRes;
    unsigned char DIR_CrtTimeTenth;
    unsigned char DIR_CrtTime[2];
    unsigned char DIR_CrtDate[2];
    unsigned char DIR_LstAccDate[2];
    unsigned char DIR_FstClusHI[2];
    unsigned char DIR_WrtTime[2];
    unsigned char DIR_WrtDate[2];
    unsigned char DIR_FstClusLO[2];
    unsigned char DIR_FileSize[4];
    int entryOffset;

}__attribute__((packed));

typedef struct
{

    char *path;
    int offset;
    int mode;

} fileStruct;

typedef struct
{

    unsigned short BPB_BytsPerSec;
    unsigned char BPB_SecPerClus;
    unsigned short BPB_RsvdSecCnt;
    unsigned char BPB_NumFATs;
    unsigned int BPB_TotSec32;
    unsigned int BPB_FATSz32;
    unsigned int BPB_RootClus;
    int rootDirectoryOffset;
    int currentDirectoryOffset;
    int currentFAToffset;
    char *fat32pwd;
    struct DIRENTRY *currentDirectories;
    int numDirectories;
    fileStruct *openFiles;
    int numOpenFiles;

} varStruct;

void fat32info(varStruct *fat32vars);
void fat32size(FILE *image, varStruct *fat32vars, char *filename);
void fat32ls(FILE *image, varStruct *fat32vars, instruction* instr_ptr);
void fat32cd(FILE *image, varStruct *fat32vars, instruction* instr_ptr);	
void fat32creat(FILE *image, varStruct *fat32vars, instruction* instr_ptr);
void fat32mkdir(FILE *image, varStruct *fat32vars, char *directoryName);
void fat32mv(FILE *image, varStruct *fat32vars, char *filename1, char *filename2);
void fat32open(varStruct *fat32vars, char* filename, char *mode);
void fat32close(varStruct *fat32vars, char* filename);
void fat32read(FILE *image, varStruct *fat32vars, char* filename, char *offsetString, char *sizeString);
void fat32write(FILE *image, varStruct *fat32vars, char* filename, char* offsetString, char* sizeString, char* inputString);
void fat32rm(FILE *image, varStruct *fat32vars, char* filename);
void fat32cp();

void fat32initVars(FILE *image, varStruct *fat32vars);
void fillDirectoryEntry(FILE* image, struct DIRENTRY *dir);
void fillDirectories(FILE *image, varStruct *fat32vars);
void jumpToDirectory(varStruct *fat32vars, struct DIRENTRY *dir);
int getFATentryOffset (varStruct *fat32vars, int cluster);
int getNextClusterNumber(FILE *image, varStruct *fat32vars, int cluster);
int getNextClusterOffset(FILE *image, varStruct *fat32vars, int offset);
int compareFilenames(char *filename1, char *filename2);
void clearFATentries (FILE *image, varStruct *fat32vars, struct DIRENTRY *dir);
int clusterToOffset(varStruct *fat32vars, int cluster);
int offsetToCluster(varStruct *fat32vars, int offset);
int getDirectoryFirstCluster (varStruct *fat32vars, struct DIRENTRY *dir);
int getDirectoryFirstOffset (varStruct *fat32vars, struct DIRENTRY *dir);
int littleToBigEndian(unsigned char *address);
int fileIsOpen(varStruct *fat32vars, char *filename);
int getFirstFreeCluster(FILE *image, varStruct *fat32vars);
void markFATentry(FILE *image, varStruct *fat32vars, int cluster, int nextCluster);
void expandCluster(FILE *image, varStruct *fat32vars, int cluster);
char* directoryToString(struct DIRENTRY *dir);
char *makeDirEntryString(varStruct *fat32vars, char *dirname, int firstFreeClus);
char *makeFileEntryString(varStruct *fat32vars, char *filename, int firstClusterNum);
int bitExtracted(int number, int k, int p);

int main (int argc, char* argv[]) {
	FILE *input;
    varStruct fat32vars;
    instruction instr;
    char *first;

	//Check arguments
    if(argc != 2) {
        printf("Invalid number of arguments\n");
        return 1;
    }

    //Open file passed in through command line
    //"rb" opens file for reading as binary
    if((input = fopen(argv[1], "r+")) == NULL) {
        printf("File could not be found\n");
        return 1;
    }

    //Initialize fat32 vars
    fat32initVars(input, &fat32vars);
    
    
    //Initialize instruction
    instr.tokens = NULL;
	instr.numTokens = 0;

	while(1) {
		printf("%s%s> ", basename(argv[1]), fat32vars.fat32pwd);	//PROMPT

		//Parse instruction
		parseInput(&instr);
			
		//Check for command
		first = instr.tokens[0];
        if(strcmp(first, "exit")==0) {
            printf("Exiting now!\n");
            break;
        }

        else if(strcmp(first, "info")==0) {
            if(instr.numTokens == 1) {            
                fat32info(&fat32vars);
            }
            else printf("info : Invalid number of arguments\n");
        }

        else if(strcmp(first, "size")==0) {
            if(instr.numTokens == 2) {
                fat32size(input, &fat32vars, instr.tokens[1]);
            }
            else printf("size : Invalid number of arguments\n");
        }

        else if(strcmp(first, "ls")==0) {
            if(instr.numTokens < 3) {
                fat32ls(input, &fat32vars, &instr);
            }
            else printf("ls : Invalid number of arguments\n");
        }

        else if(strcmp(first, "cd")==0) {
            if(instr.numTokens < 3) {
                fat32cd(input, &fat32vars, &instr);
	        }
            else printf("cd : Invalid number of arguments\n");
        }

        else if(strcmp(first, "creat")==0) {
            if(instr.numTokens == 2) {
                fat32creat(input, &fat32vars, &instr);
                fclose(input);
                input = fopen(argv[1], "r+");
            }
            else printf("create : Invalid number of arguments\n");
        }

        else if(strcmp(first, "mkdir")==0) {
            if(instr.numTokens == 2) {
                fat32mkdir(input, &fat32vars, instr.tokens[1]);
                fclose(input);
                input = fopen(argv[1], "r+");
            }
            else printf("mkdir : Invalid number of arguments\n");
        }

        else if(strcmp(first, "mv")==0) {
            if(instr.numTokens == 3) {
                fat32mv(input, &fat32vars, instr.tokens[1], instr.tokens[2]);
                fclose(input);
                input = fopen(argv[1], "r+");
            }
            else printf("mv : Invalid number of arguments\n");
        }

        else if(strcmp(first, "open")==0) {
            if(instr.numTokens == 3) {
                fat32open(&fat32vars, instr.tokens[1], instr.tokens[2]);
            }
            else printf("open : Invalid number of arguments\n");
        }

        else if(strcmp(first, "close")==0) {
            if(instr.numTokens == 2) {
                fat32close(&fat32vars, instr.tokens[1]);
            }
            else printf("close : Invalid number of arguments\n");
        }

        else if(strcmp(first, "read")==0) {
            if(instr.numTokens == 4) {
                fat32read(input, &fat32vars, instr.tokens[1], instr.tokens[2], instr.tokens[3]);                
            }
            else printf("read : Invalid number of arguments\n");
        }

        else if(strcmp(first, "write")==0) {
            //ensure input string is enclosed by "" and store it in char* inputString to be passed to function
            if (instr.numTokens >= 5){
                if (instr.tokens[4][0] == 34){      //if input string starts with "
                    int lastTokenSize = strlen(instr.tokens[instr.numTokens-1]);
                    if (instr.tokens[instr.numTokens-1][lastTokenSize-1] == 34){    //if input string ends with "
                        char* inputString = malloc(100);
                        strcpy(inputString, instr.tokens[4]);
                        int loop;
                        for (loop = 5; loop < instr.numTokens; loop++){
                            strcat(inputString, " ");
                            strcat(inputString, instr.tokens[loop]);
                        }
                        fat32write(input, &fat32vars, instr.tokens[1], instr.tokens[2], instr.tokens[3], inputString);
                        fclose(input);
                        input = fopen(argv[1], "r+");
                    }
                    else{
                        printf("write : End of Input string formatted incorrectly\n");
                    }
                }
                else{
                        printf("write : Start of Input string formatted incorrectly\n");
                }
                
            }
            else printf("write : Invalid number of arguments\n");
        }

        else if(strcmp(first, "rm")==0) {
            if(instr.numTokens == 2) {
                fat32rm(input, &fat32vars, instr.tokens[1]);
                fclose(input);
                input = fopen(argv[1], "r+");
            }
            else printf("rm : Invalid number of arguments\n");
        }

        else if(strcmp(first, "cp")==0) {
            if(instr.numTokens == 3) {
                fat32cp();
                fclose(input);
                input = fopen(argv[1], "r+");
            }
            else printf("cp : Invalid number of arguments\n");
        }
        
        else {
            printf("Unknown command\n");
        }

		//Clear instruction
		clearInstruction(&instr);
	}

    free(fat32vars.fat32pwd);
    free(fat32vars.currentDirectories);
    free(fat32vars.openFiles);

    //Close file
    fclose(input);

    return 0;
}

void fat32info(varStruct *fat32vars) {

    printf("Bytes per sector: %d\n", fat32vars->BPB_BytsPerSec);
    printf("Sectors per cluster: %d\n", fat32vars->BPB_SecPerClus);
    printf("Reserved sector count: %d\n", fat32vars->BPB_RsvdSecCnt);
    printf("Number of FATs: %d\n", fat32vars->BPB_NumFATs);
    printf("Total Sectors: %d\n", fat32vars->BPB_TotSec32);
    printf("FAT size: %d\n", fat32vars->BPB_FATSz32);
    printf("Root Cluster: %d\n", fat32vars->BPB_RootClus);

}

void fat32size(FILE *image, varStruct *fat32vars, char *filename) {
    struct DIRENTRY *dir;

    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {
        if(!strcmp(filename, ".") || !strcmp(filename, "..")) {
            printf("Invalid file\n");
            return;
        }
    }

    int loop;
    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {

        dir = &fat32vars->currentDirectories[loop];

        if(compareFilenames(dir->DIR_Name, filename) == 0) {

            if(dir->DIR_Attr == 16) {
                printf("File is directory.\n");
                return;
            }

            else {
                printf("Size of %s: %d bytes\n", dir->DIR_Name, littleToBigEndian(dir->DIR_FileSize));
                return;
            }

        }
    }

    printf("File not found\n");

}

void fat32ls(FILE *image, varStruct *fat32vars, instruction* instr_ptr) {

    int loop;

    if(instr_ptr->numTokens == 2) {
        struct DIRENTRY *dir;
        for(loop = 0; loop < fat32vars->numDirectories; ++loop) {

            dir = &fat32vars->currentDirectories[loop];

            if(compareFilenames(dir->DIR_Name, instr_ptr->tokens[1]) == 0) {

                if(dir->DIR_Attr == 16) {
                    int originalOffset = fat32vars->currentDirectoryOffset;
                    fat32vars->currentDirectoryOffset = getDirectoryFirstOffset(fat32vars, dir);
                    fillDirectories(image, fat32vars);
                    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {
                        if(fat32vars->currentDirectories[loop].DIR_Attr == 15 || 
                        fat32vars->currentDirectories[loop].DIR_Name[0] == 229) continue;
                        if(fat32vars->currentDirectories[loop].DIR_Attr == 16) printf("\033[0;34m");
                        printf("%s\n", fat32vars->currentDirectories[loop].DIR_Name);
                        printf("\033[0m");
                    }
                    fat32vars->currentDirectoryOffset = originalOffset;
                    fillDirectories(image, fat32vars);
                    return;
                }

                else {
                    printf("Entry is a file.\n");
                    return;
                }

            }
        }
        printf("Directory not found.\n");
        return;
    }

    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {
        if(fat32vars->currentDirectories[loop].DIR_Attr == 15 || 
        fat32vars->currentDirectories[loop].DIR_Name[0] == 229) continue;
        if(fat32vars->currentDirectories[loop].DIR_Attr == 16) printf("\033[0;34m");
        printf("%s\n", fat32vars->currentDirectories[loop].DIR_Name);
        printf("\033[0m");
    }

}

void fat32cd(FILE *image, varStruct *fat32vars, instruction* instr_ptr) {

    struct DIRENTRY *dir;		

    //No arguments, move back to root directory
    if(instr_ptr->numTokens == 1) {

        fat32vars->currentDirectoryOffset =
        fat32vars->rootDirectoryOffset;

        free(fat32vars->fat32pwd);
        fat32vars->fat32pwd = malloc(1);
        fat32vars->fat32pwd[0] = '\0';

        fillDirectories(image, fat32vars);
        
        return;
    }

    //Handle the "." and ".." cases
    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {

        if(strcmp(instr_ptr->tokens[1], ".") == 0) {
            return;
        }

        else if(strcmp(instr_ptr->tokens[1], "..") == 0) {

            jumpToDirectory(fat32vars, &fat32vars->currentDirectories[1]);
            fillDirectories(image, fat32vars);

            //Remove last directory in path
            char *temp = strstr(fat32vars->fat32pwd, "/..");
            --temp;
            while(*temp != '/'){--temp;};
            *temp = '\0';

            return;
        }
    }

    //Navigate to directory in instruction
    int loop;
    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {

        dir = &fat32vars->currentDirectories[loop];

        if(dir->DIR_Attr == 15) continue;

        if(compareFilenames(dir->DIR_Name, instr_ptr->tokens[1]) == 0) {
            
            if(dir->DIR_Attr != 16) {
                printf("File is not directory.\n");
                return;
            }
            else {
                jumpToDirectory(fat32vars, dir);
                fillDirectories(image, fat32vars);
                return;
            }
        }
    }
    printf("Directory not found\n");
}

void fat32creat(FILE *image, varStruct *fat32vars, instruction* instr_ptr) {
    struct DIRENTRY *dir;

    //Check to see if file already exists
    int loop;
    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {

        dir = &fat32vars->currentDirectories[loop];

        if(compareFilenames(dir->DIR_Name, instr_ptr->tokens[1]) == 0) {

            if(dir->DIR_Attr == 16) {
                printf("File is directory.\n");
                return;
            }
            else {
                printf("File already exists.\n");
                return;
            }

        }
    }

    int firstFreeClus = getFirstFreeCluster(image, fat32vars);
    markFATentry(image, fat32vars, firstFreeClus, 0);
    int freeClusOffset = clusterToOffset(fat32vars, firstFreeClus);
    int currDirCluster = offsetToCluster(fat32vars, fat32vars->currentDirectoryOffset);
    if(currDirCluster == fat32vars->BPB_RootClus) currDirCluster = 0;

    //find first empty space in current directory, store in freeDirOffset
    unsigned char * readEntry = malloc(32);
    fseek(image, fat32vars->currentDirectoryOffset, SEEK_SET);
    do{
        fread(readEntry, 1, 32, image);
    } while (readEntry[0] != 0 && readEntry[0] != 229);
    fseek(image, -32, SEEK_CUR);
    int freeDirOffset = ftell(image);

    //Create directory entry in current directory
    char* dirEntry = makeFileEntryString(fat32vars, instr_ptr->tokens[1], firstFreeClus);
    fseek(image, freeDirOffset, SEEK_SET);
    fwrite(dirEntry, 1, 32, image);


    //update currentDirectories with newly created directory
    fillDirectories(image, fat32vars);


}

void fat32mkdir(FILE *image, varStruct *fat32vars, char *directoryName) {

    if(strlen(directoryName) > 8) {
        printf("Directory name too long.\n");
        return;
    }

    //Check if that directory already exists
    int loop;
    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {
        if (!compareFilenames(directoryName, fat32vars->currentDirectories[loop].DIR_Name)) {
            printf("Directory already exists\n");
            return;
        }
    }

    int firstFreeClus = getFirstFreeCluster(image, fat32vars);
    markFATentry(image, fat32vars, firstFreeClus, 0);
    int freeClusOffset = clusterToOffset(fat32vars, firstFreeClus);
    int currDirCluster = offsetToCluster(fat32vars, fat32vars->currentDirectoryOffset);
    if(currDirCluster == fat32vars->BPB_RootClus) currDirCluster = 0;

    //find first empty space in current directory, store in freeDirOffset
    unsigned char * readEntry = malloc(32);
    int currCluster = currDirCluster;
    int nextCluster = 0;
    fseek(image, fat32vars->currentDirectoryOffset, SEEK_SET);
    while(1){
        fread(readEntry, 1, 32, image);
        if(readEntry[0] == 0 || readEntry[0] == 229) break;
        if(ftell(image) % fat32vars->BPB_BytsPerSec == 0) {
            printf("Current read offset = %ld\n", ftell(image));
            nextCluster = getNextClusterNumber(
                    image, 
                    fat32vars, 
                    currCluster);
            printf("Current cluster = %d\n", currCluster);
            printf("Next cluster = %d\n", nextCluster);
            if(nextCluster == currCluster) {
                //Allocate a new cluster in the FAT
                nextCluster = getFirstFreeCluster(image, fat32vars);
                printf("First free cluster = %d\n", nextCluster);
                markFATentry(image, fat32vars, currCluster, nextCluster);
            }
            else currCluster = nextCluster;
            fseek(image, clusterToOffset(fat32vars, currCluster), SEEK_SET);
        }
    }
    fseek(image, -32, SEEK_CUR);
    int freeDirOffset = ftell(image);

    //Create directory entry in current directory
    char* dirEntry = makeDirEntryString(fat32vars, directoryName, firstFreeClus);
    fseek(image, freeDirOffset, SEEK_SET);
    fwrite(dirEntry, 1, 32, image);

    //Create "." and ".." directories in new directory
    dirEntry = makeDirEntryString(fat32vars, ".", firstFreeClus);
    fseek(image, freeClusOffset, SEEK_SET);
    fwrite(dirEntry, 1, 32, image);
    dirEntry = makeDirEntryString(fat32vars, "..", currDirCluster);
    fseek(image, freeClusOffset+32, SEEK_SET);
    fwrite(dirEntry, 1, 32, image);

    //update currentDirectories with newly created directory
    fillDirectories(image, fat32vars);
}

void fat32mv(FILE *image, varStruct *fat32vars, char *FROM, char *TO) {
    struct DIRENTRY *dir, *dirFROM = NULL, *dirTO = NULL;
    int fromloop, toloop;

    if(strlen(TO) > 8) {
        printf("Filename too long\n");
        return;
    }

    //Handle the "." and ".." cases for FROM
    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {
        if(!strcmp(FROM, ".") || !strcmp(FROM, "..")) {
            printf("Invalid file\n");
            return;
        }
    }

    //Navigate to file in FROM
    for(fromloop = 0; fromloop < fat32vars->numDirectories; ++fromloop) {

        dir = &fat32vars->currentDirectories[fromloop];

        if(dir->DIR_Attr == 15) continue;

        if(!compareFilenames(dir->DIR_Name, FROM)) {
            
            if(dir->DIR_Attr == 16) {
                printf("FROM is a directory.\n");
                return;
            }
            else {
                dirFROM = dir;
                break;
            }
        }
    }

    //FROM was not found, print error
    if(dirFROM == NULL) {
        printf("%s not found\n", FROM);
        return;
    }

    if(fileIsOpen(fat32vars, dirFROM->DIR_Name)) {
        printf("Cannot move opened file\n");
        return;
    }

    //Navigate to directory in TO
    for(toloop = 0; toloop < fat32vars->numDirectories; ++toloop) {

        dir = &fat32vars->currentDirectories[toloop];

        if(dir->DIR_Attr == 15) continue;

        if(!compareFilenames(dir->DIR_Name, TO)) {
            if(dir->DIR_Attr != 16) {
                printf("File already exists\n");
                return;
            }
            else {
                dirTO = dir;
                break;
            }
        }
    }

    if(dirTO != NULL) {
        int currentOffset = getDirectoryFirstOffset(fat32vars, dirTO);
        fseek(image, currentOffset, SEEK_SET);
        struct DIRENTRY tempDir;
        while(1){
            fillDirectoryEntry(image, &tempDir);
            if (tempDir.DIR_Name[0] == 0 || tempDir.DIR_Name[0] == 229) break;
            if(ftell(image) % fat32vars->BPB_BytsPerSec == 0) {
                currentOffset = getNextClusterOffset(
                    image, 
                    fat32vars, 
                    currentOffset);
                fseek(image, currentOffset, SEEK_SET);
            }
        }
        unsigned char *temp = malloc(32);
        temp = directoryToString(dirFROM);
        fseek(image, tempDir.entryOffset, SEEK_SET);
        fwrite(temp, 32, 1, image);
        
        int dirSize = 1;
        dir = dir = &fat32vars->currentDirectories[fromloop];
        if(fromloop > 0) {
            dir = &fat32vars->currentDirectories[fromloop-1];
            if(dir->DIR_Attr == 15) dirSize = 2;
            else dir = &fat32vars->currentDirectories[fromloop];
        }
        fseek(image, dir->entryOffset, SEEK_SET);
        free(temp);
        temp = malloc(32*dirSize);
        memset(temp, 0, 32*dirSize);
        if(fromloop < fat32vars->numDirectories-1) { 
            temp[0] = 229;
            if(dirSize==2) temp[32] = 229;
        }
        fwrite(temp, 1, 32*dirSize, image);
        fillDirectories(image, fat32vars);
        return;
    }

    else {
        char *temp = malloc(11);
        memset(temp, 32, 11);
        int loop;
        for(loop = 0; loop < strlen(TO); ++loop) {
            temp[loop] = toupper(TO[loop]);
        }
        fseek(image, dirFROM->entryOffset, SEEK_SET);
        fwrite(temp, 1, 11, image);
        fillDirectories(image, fat32vars);
        return;
    }

}

void fat32open(varStruct *fat32vars, char* filename, char *mode) {
    struct DIRENTRY *dir;
    fileStruct *file;
    int loop;

    int modeInt;
    if(!strcmp(mode, "r")) modeInt = 1;
    else if(!strcmp(mode, "w")) modeInt = 2;
    else if(!strcmp(mode, "rw") || !strcmp(mode, "wr")) modeInt = 3;
    else {
        printf("Invalid mode\n");
        return;
    }

    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {
        if(!strcmp(filename, ".") || !strcmp(filename, "..")) {
            printf("Invalid file\n");
            return;
        }
    }

    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {

        dir = &fat32vars->currentDirectories[loop];

        if(dir->DIR_Attr == 15) continue;

        if(compareFilenames(dir->DIR_Name, filename) == 0) {

            for(loop = 0; loop < fat32vars->numOpenFiles; ++loop) {
                if(getDirectoryFirstOffset(fat32vars, dir) == fat32vars->openFiles[loop].offset &&
                !compareFilenames(basename(fat32vars->openFiles[loop].path), filename)) {
                    printf("File already open.\n");
                    return;
                }
            }

            file = &fat32vars->openFiles[fat32vars->numOpenFiles];

            char *fullpath = malloc(strlen(fat32vars->fat32pwd) + strlen(filename) + 2);
            strcpy(fullpath, fat32vars->fat32pwd);
            strcat(fullpath, "/");
            strcat(fullpath, filename);
            file->path = fullpath;
            printf("Opened file %s\n", file->path);

            file->offset = getDirectoryFirstOffset(fat32vars, dir);
            file->mode = modeInt;
            ++fat32vars->numOpenFiles;

            fat32vars->openFiles = 
            realloc(fat32vars->openFiles, 
            sizeof(fileStruct)*(fat32vars->numOpenFiles+1));
            return;
        }
    }
    printf("File not found\n");
}

void fat32close(varStruct *fat32vars, char* filename) {
    struct DIRENTRY *dir;
    fileStruct *file;
    int loop;

    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {
        if(!strcmp(filename, ".") || !strcmp(filename, "..")) {
            printf("Invalid file\n");
            return;
        }
    }

    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {

        dir = &fat32vars->currentDirectories[loop];

        if(dir->DIR_Attr == 15) continue;

        if(compareFilenames(dir->DIR_Name, filename) == 0) {

            if(dir->DIR_Attr == 16) {
                printf("File is directory.\n");
                return;
            }
            else {
                for(loop = 0; loop < fat32vars->numOpenFiles; ++loop) {
                    file = &fat32vars->openFiles[loop];
                    if(!strcmp(basename(file->path), filename)) {

                        memmove(
                            file, 
                            &fat32vars->openFiles[loop+1], 
                            sizeof(fileStruct) * (fat32vars->numOpenFiles - loop));

                        --fat32vars->numOpenFiles;
                        fat32vars->openFiles = realloc(fat32vars->openFiles, sizeof(fileStruct)*(fat32vars->numOpenFiles+1));
                        printf("Closed file %s\n", filename);
                        return;
                    }
                }
                printf("File not open\n");
                return;
            }
        }
    }
    printf("File not found\n");
}

void fat32read(FILE *image, varStruct *fat32vars, char* filename, char *offsetString, char *sizeString) {
    struct DIRENTRY *dir;
    fileStruct *file;
    int loop, offset, size, readSector;

    offset = atoi(offsetString);
    size = atoi(sizeString);

    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {
        if(!strcmp(filename, ".") || !strcmp(filename, "..")) {
            printf("Invalid file\n");
            return;
        }
    }

    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {

        dir = &fat32vars->currentDirectories[loop];

        if(dir->DIR_Attr == 15) continue;

        if(compareFilenames(dir->DIR_Name, filename) == 0) {

            if(littleToBigEndian(dir->DIR_FileSize) == 0) {
                printf("File is empty.\n");
                return;
            }
            if(dir->DIR_Attr == 16) {
                printf("File is directory.\n");
                return;
            }
            else {
                for(loop = 0; loop < fat32vars->numOpenFiles; ++loop) {
                    file = &fat32vars->openFiles[loop];
                    if(!strcmp(basename(file->path), filename)) {
                        if(file->mode == 2) {
                            printf("File is in write only mode\n");
                            return;
                        }
                        int filesize = littleToBigEndian(dir->DIR_FileSize);
                        if((offset + size) > filesize) {
                            printf("Offset + Size is too large\n");
                            return;
                        }
                        int readCluster = getDirectoryFirstCluster(fat32vars, dir);
                        int readOffset = clusterToOffset(fat32vars, readCluster) + offset;
                        fseek(image, readOffset, SEEK_SET);
                        unsigned char *output = malloc(size+1);
                        for(loop = 0; loop < size; ++loop) {
                            output[loop] = fgetc(image);
                            if((ftell(image)%fat32vars->BPB_BytsPerSec) == 0) {
                                readCluster = getNextClusterNumber(image, fat32vars, readCluster);
                                readOffset = clusterToOffset(fat32vars, readCluster) + offset;
                                fseek(image, readOffset, SEEK_SET);
                            }
                        }
                        output[size] = '\0';
                        printf("%s\n", output);
                        return;
                    }
                }
                printf("File not open\n");
                return;
            }
        }
    }
    printf("File not found\n");
}

void fat32write(FILE *image, varStruct *fat32vars, char* filename, char* offsetString, char* sizeString, char* inputString) {
    struct DIRENTRY *dir;
    fileStruct *file;
    int loop1, loop2, offset, size, readSector, bpc;

    offset = atoi(offsetString);
    size = atoi(sizeString);

    //remove quotes from inputString
    inputString++;
    inputString[strlen(inputString)-1] = 0;

    bpc = fat32vars->BPB_BytsPerSec * fat32vars->BPB_SecPerClus;

    //cannot write to . and ..
    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {
        if(!strcmp(filename, ".") || !strcmp(filename, "..")) {
            printf("Invalid file\n");
            return;
        }
    }

    for(loop1 = 0; loop1 < fat32vars->numDirectories; ++loop1) {

        dir = &fat32vars->currentDirectories[loop1];

        if(dir->DIR_Attr == 15) continue;

        if(compareFilenames(dir->DIR_Name, filename) == 0) {
            if(dir->DIR_Attr == 16) {
                printf("File is directory.\n");
                return;
            }
            else {
                for(loop2 = 0; loop2 < fat32vars->numOpenFiles; ++loop2) {
                    file = &fat32vars->openFiles[loop2];
                    if(!strcmp(basename(file->path), filename)) {
                        if(file->mode == 1) {
                            printf("File is in read only mode\n");
                            return;
                        }
                        int writeCluster = getDirectoryFirstCluster(fat32vars, dir);
                        int filesize = littleToBigEndian(dir->DIR_FileSize);
                        if((offset + size) > filesize) {    //have to make more clusters
                            //calculate number of extra clusters to make
                            int i;
                            int currClusters = filesize / bpc;
                            if (filesize % bpc > 0){
                                currClusters++;
                            }
                            int finalClusters = (offset+size) / bpc;
                            if ((offset+size) % bpc > 0){
                                finalClusters++;
                            }
                            int extraClusters = finalClusters - currClusters;
                            //create new clusters
                            for (i = 0; i < extraClusters; i++){
                                expandCluster(image, fat32vars, writeCluster);
                            }
                        }
                        int writeOffset = clusterToOffset(fat32vars, writeCluster) + offset;
                        fseek(image, writeOffset, SEEK_SET);
                        for(loop2 = 0; loop2 < size; ++loop2) {     //write 1 char at a time, moving to next cluster if needed
                            fputc(inputString[loop2], image);
                            if((ftell(image)%fat32vars->BPB_BytsPerSec) == 0) {
                                int currCluster = offsetToCluster(fat32vars, ftell(image) - fat32vars->BPB_BytsPerSec);
                                offset -= (loop2+1);
                                writeCluster = getNextClusterNumber(image, fat32vars, currCluster);
                                writeOffset = clusterToOffset(fat32vars, writeCluster);
                                fseek(image, writeOffset, SEEK_SET);
                            }
                        }
                        return;
                    }
                }
            }
        }

    }
}

void fat32rm(FILE *image, varStruct *fat32vars, char* filename) {
    struct DIRENTRY *dir;
    int loop;
    int dirSize = 1;

    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {
        if(!strcmp(filename, ".") || !strcmp(filename, "..")) {
            printf("Invalid file\n");
            return;
        }
    }

    for(loop = 0; loop < fat32vars->numDirectories; ++loop) {

        dir = &fat32vars->currentDirectories[loop];

        if(dir->DIR_Attr == 15) continue;

        if(compareFilenames(dir->DIR_Name, filename) == 0) {

            if(dir->DIR_Attr == 16) {
                printf("File is directory.\n");
                return;
            }
            else {
                if(fileIsOpen(fat32vars, filename)) fat32close(fat32vars, filename);

                //Clear out FAT entries
                clearFATentries(image, fat32vars, dir);
                
                //Clear out directory entry
                if(loop > 0) {
                    dir = &fat32vars->currentDirectories[loop-1];
                    if(dir->DIR_Attr == 15) dirSize = 2;
                    else dir = &fat32vars->currentDirectories[loop];
                }
                fseek(image, dir->entryOffset, SEEK_SET);
                unsigned char *temp = malloc(32*dirSize);
                memset(temp, 0, 32*dirSize);
                if(loop < fat32vars->numDirectories-1) { 
                    temp[0] = 229;
                    if(dirSize==2) temp[32] = 229;
                }
                fwrite(temp, 1, 32*dirSize, image);
                fillDirectories(image, fat32vars);
                return;
            }
        }
    }
    printf("File not found\n");
}

void fat32cp() {

}

//Initializes all the variables for the FAT32 volume
void fat32initVars(FILE *image, varStruct *fat32vars) {
    unsigned char *temp;

    temp = malloc(4);
    fseek(image, 11, SEEK_SET);
    fread(temp, 1, 2, image);
    fat32vars->BPB_BytsPerSec = littleToBigEndian(temp);
    free(temp);

    fseek(image, 13, SEEK_SET);
    fat32vars->BPB_SecPerClus = fgetc(image);

    temp = malloc(4);
    fseek(image, 14, SEEK_SET);
    fread(temp, 1, 2, image);
    fat32vars->BPB_RsvdSecCnt = littleToBigEndian(temp);
    free(temp);

    fseek(image, 16, SEEK_SET);
    fat32vars->BPB_NumFATs = fgetc(image);

    temp = malloc(4);
    fseek(image, 32, SEEK_SET);
    fread(temp, 1, 4, image);
    fat32vars->BPB_TotSec32 = littleToBigEndian(temp);
    free(temp);

    temp = malloc(4);
    fseek(image, 36, SEEK_SET);
    fread(temp, 1, 4, image);
    fat32vars->BPB_FATSz32 = littleToBigEndian(temp);
    free(temp);

    temp = malloc(4);
    fseek(image, 44, SEEK_SET);
    fread(temp, 1, 4, image);
    fat32vars->BPB_RootClus = littleToBigEndian(temp);
    free(temp);

    fat32vars->rootDirectoryOffset = 
    fat32vars->BPB_BytsPerSec * 
    (fat32vars->BPB_RsvdSecCnt + 
    (fat32vars->BPB_NumFATs * fat32vars->BPB_FATSz32));

    fat32vars->currentDirectoryOffset = fat32vars->rootDirectoryOffset;

    fat32vars->fat32pwd = malloc(1);
    fat32vars->fat32pwd[0] = '\0';

    fat32vars->currentDirectories = malloc(1);
    fillDirectories(image, fat32vars);

    fat32vars->openFiles = malloc(sizeof(fileStruct));
    fat32vars->numOpenFiles = 0;
}

//Fills dir pointer with 32 byte directory entry at current seek
void fillDirectoryEntry(FILE* image, struct DIRENTRY *dir) {

    dir->entryOffset = ftell(image);

    //Fill directory name, add null terminator
    fread(dir->DIR_Name, 1, 11, image);
    int pos = 0;
    while(pos < 13) {
        if(dir->DIR_Name[pos] == 32 || pos == 12) {
            dir->DIR_Name[pos] = '\0';
            break;
        }
        else ++pos;
    }

    dir->DIR_Attr = fgetc(image);
    dir->DIR_NTRes = fgetc(image);
    dir->DIR_CrtTimeTenth = fgetc(image);
    fread(dir->DIR_CrtTime, 1, 2, image);
    fread(dir->DIR_CrtDate, 1, 2, image);
    fread(dir->DIR_LstAccDate, 1, 2, image);
    fread(dir->DIR_FstClusHI, 1, 2, image);
    fread(dir->DIR_WrtTime, 1, 2, image);
    fread(dir->DIR_WrtDate, 1, 2, image);
    fread(dir->DIR_FstClusLO, 1, 2, image);
    fread(dir->DIR_FileSize, 1, 4, image);

}

//Fills the currentDirectories array with directory entries at currentDirectoryOffset
//currentDirectories array does not need to be emptied beforehand
void fillDirectories(FILE *image, varStruct *fat32vars) {
    struct DIRENTRY dir;
    int dirCount = 0;

    fseek(image, fat32vars->currentDirectoryOffset, SEEK_SET);
    int originalOffset = fat32vars->currentDirectoryOffset;

    free(fat32vars->currentDirectories);
    fat32vars->currentDirectories = malloc(sizeof(struct DIRENTRY));

    if(fat32vars->currentDirectoryOffset != fat32vars->rootDirectoryOffset) {

        fat32vars->currentDirectories = 
        realloc(fat32vars->currentDirectories, 
        sizeof(struct DIRENTRY)*3);

        fillDirectoryEntry(image, &dir);
        fat32vars->currentDirectories[0] = dir;
        ++dirCount;

        fillDirectoryEntry(image, &dir);
        fat32vars->currentDirectories[1] = dir;
        ++dirCount;
    }

    while(1) {
        fillDirectoryEntry(image, &dir);

        if(dir.DIR_Name[0] == 0) break;

        fat32vars->currentDirectories[dirCount] = dir;
        ++dirCount;

        fat32vars->currentDirectories = 
        realloc(fat32vars->currentDirectories, 
        sizeof(struct DIRENTRY)*(dirCount+1));

        if(dirCount % 16 == 0) {
            int nextOffset = getNextClusterOffset(image, fat32vars, fat32vars->currentDirectoryOffset);
            if(nextOffset == fat32vars->currentDirectoryOffset) break; 
            fat32vars->currentDirectoryOffset = nextOffset;
            fseek(image, fat32vars->currentDirectoryOffset, SEEK_SET);
        };

    }
    fat32vars->numDirectories = dirCount;
    fat32vars->currentDirectoryOffset = originalOffset;
}

//Changes currentDirectoryOffset to first cluster pointed to by dir
//Adds filename to the fat32pwd string
void jumpToDirectory(varStruct *fat32vars, struct DIRENTRY *dir) {
    
    fat32vars->currentDirectoryOffset = getDirectoryFirstOffset(fat32vars, dir);
    //printf("Jump to directory offset = %d\n", getDirectoryFirstOffset(fat32vars, dir));

    //Reallocate PWD for new directory
    fat32vars->fat32pwd = 
    realloc(fat32vars->fat32pwd, 
    strlen(fat32vars->fat32pwd) + strlen(dir->DIR_Name) + 2);

    strcat(fat32vars->fat32pwd, "/");
    strcat(fat32vars->fat32pwd, dir->DIR_Name);
    
    return;
}

//Get the FAT entry offset of the passed in cluster number
int getFATentryOffset (varStruct *fat32vars, int cluster) {
    int FAToffset = 
    fat32vars->BPB_BytsPerSec *
    fat32vars->BPB_SecPerClus *
    fat32vars->BPB_RsvdSecCnt;

    return (cluster * 4) + FAToffset;
}

//Returns the cluster number associated with the FAT entry at the passed in offset
int getFATentryCluster(varStruct *fat32vars, int offset) {
    int FAToffset = 
    fat32vars->BPB_BytsPerSec *
    fat32vars->BPB_SecPerClus *
    fat32vars->BPB_RsvdSecCnt;

    return (offset - FAToffset) / 4;
}

//Get the number of the next cluster pointed to by the FAT entry of passed in cluster nuber
//Returns same cluster number if it is the final cluster
int getNextClusterNumber(FILE *image, varStruct *fat32vars, int cluster) {

    fseek(image, getFATentryOffset(fat32vars, cluster), SEEK_SET);

    unsigned char *temp;
    temp = malloc(4);

    fread(temp, 1, 4, image);

    int nextCluster = littleToBigEndian(temp);

    if(nextCluster > (fat32vars->BPB_TotSec32/fat32vars->BPB_SecPerClus)) {
        return cluster;
    }

    else return nextCluster;
}

//Get the offset of the next cluster pointed to by the FAT entry of the passed in cluster offset
//Returns the same offset if it is the final cluster
int getNextClusterOffset(FILE *image, varStruct *fat32vars, int offset) {

    int cluster = offsetToCluster(fat32vars, offset);

    int nextCluster = getNextClusterNumber(image, fat32vars, cluster);

    if(nextCluster == cluster) { 
        return offset;
    }
    else {
        return clusterToOffset(fat32vars, nextCluster);
    }
}

//Turn both strings into uppercase and return 0 if they are the same.
int compareFilenames(char *filename1, char *filename2) {
    char *temp1, *temp2;
    int loop;

    temp1 = malloc(strlen(filename1)+1);
    strcpy(temp1, filename1);
    temp2 = malloc(strlen(filename2)+1);
    strcpy(temp2, filename2);

    for(loop = 0; loop < strlen(temp1); ++loop) {
        temp1[loop] = toupper(temp1[loop]);
    }

    for(loop = 0; loop < strlen(temp2); ++loop) {
        temp2[loop] = toupper(temp2[loop]);
    }

    return strcmp(temp1, temp2);

}

//Clear out the data the directory pointer is tied to and recursively clear the FAT at each cluster
void clearFATentries (FILE *image, varStruct *fat32vars, struct DIRENTRY *dir) {

    int FAToffset = fat32vars->BPB_RsvdSecCnt * fat32vars->BPB_BytsPerSec;
    static int currentCluster;
    int previousCluster;

    if(dir != NULL) {
        currentCluster = getDirectoryFirstCluster(fat32vars, dir);
        if(currentCluster == 0) return;
    }
    else {
        previousCluster = currentCluster;
        currentCluster = getNextClusterNumber(image, fat32vars, currentCluster);
    }

    int nextCluster = getNextClusterNumber(image, fat32vars, currentCluster);

    if(nextCluster != currentCluster) {
        clearFATentries (image, fat32vars, NULL);
    }

    unsigned char *temp = malloc(fat32vars->BPB_BytsPerSec);
    memset(temp, 0, fat32vars->BPB_BytsPerSec);

    fseek(image, clusterToOffset(fat32vars, currentCluster), SEEK_SET);
    fwrite(temp, 1, fat32vars->BPB_BytsPerSec, image);
    fseek(image, getFATentryOffset(fat32vars, currentCluster), SEEK_SET);
    fwrite(temp, 1, 4, image);
    currentCluster = previousCluster;
    return;
}

//Convert a cluster number into an absolute offset
int clusterToOffset(varStruct *fat32vars, int cluster) {
    return
    (cluster - 2) *
    (fat32vars->BPB_BytsPerSec * fat32vars->BPB_SecPerClus) + 
    fat32vars->rootDirectoryOffset;
}

//Convert a cluster offset into the cluster number
int offsetToCluster(varStruct *fat32vars, int offset) {
    if(offset % fat32vars->BPB_BytsPerSec != 0) 
    printf("OFFSET IS NOT MULTIPLE OF BYTES PER SEC\n");

    return
    ((offset - fat32vars->rootDirectoryOffset) / 
    fat32vars->BPB_BytsPerSec) + 2;
}

//Get the first cluster number pointed to by the directory entry
//Returns 2 if first cluster is less than 2
int getDirectoryFirstCluster (varStruct *fat32vars, struct DIRENTRY *dir) {
    unsigned char *temp = malloc(4);
    temp[0] = dir->DIR_FstClusLO[0];
    temp[1] = dir->DIR_FstClusLO[1];
    temp[2] = dir->DIR_FstClusHI[0];
    temp[3] = dir->DIR_FstClusHI[1];

    int firstCluster = littleToBigEndian(temp);

    if (firstCluster >= 2) return firstCluster;
    else return 2;
}

//Get the offset of the first cluster pointed to by the directory entry
//Returns root directory offset if next cluster is less than 2
int getDirectoryFirstOffset (varStruct *fat32vars, struct DIRENTRY *dir) {
    return clusterToOffset(fat32vars, getDirectoryFirstCluster(fat32vars, dir));
}

//This function will segfault if you pass a pointer with less than 4 bytes of memory allocated
int littleToBigEndian(unsigned char *address) {
    return address[0] | address[1] << 8 | address[2] << 16 | address[3] << 24;
}

//Returns 1 if the file is in the openFiles array
int fileIsOpen(varStruct *fat32vars, char *filename){
    int loop;
    fileStruct *file;
    for(loop = 0; loop < fat32vars->numOpenFiles; ++loop) {
        file = &fat32vars->openFiles[loop];
        if(!compareFilenames(basename(file->path), filename)) return 1;
    }
    return 0;
}

//Traverses FAT and finds the first free cluster
//Writes free cluster in first available slot in FAT
int getFirstFreeCluster(FILE *image, varStruct *fat32vars){

    int FAToffset = 
    fat32vars->BPB_BytsPerSec *
    fat32vars->BPB_SecPerClus *
    fat32vars->BPB_RsvdSecCnt;

    fseek(image, FAToffset, SEEK_SET);

    unsigned char * FATentry;
    FATentry = malloc(4);
    int freeClus;   //stores cluster num of first available cluster

    //search FAT until you come across a 0
    while(1){
        fread(FATentry, 1, 4, image);
        if(littleToBigEndian(FATentry) == 0) break;
    }
    fseek(image, -4, SEEK_CUR);
    
    //calculate first available cluster
    return getFATentryCluster(fat32vars, ftell(image));
}

//Marks the FAT entry for cluster to point to nextCluster
//If cluster is the final cluster, pass in 0 as nextCluster
void markFATentry(FILE *image, varStruct *fat32vars, int cluster, int nextCluster) {

    int FAToffset = 
    fat32vars->BPB_BytsPerSec *
    fat32vars->BPB_SecPerClus *
    fat32vars->BPB_RsvdSecCnt;

    unsigned char emptyHex[4];
    emptyHex[0] = 255;
    emptyHex[1] = 255;
    emptyHex[2] = 255;
    emptyHex[3] = 15;

    int FATclusterOffset = getFATentryOffset(fat32vars, cluster);
    
    //nextCluster being 0 means it is the final cluster of that file
    if(nextCluster == 0) {
        fseek(image, FATclusterOffset, SEEK_SET);
        fwrite(emptyHex, 1, 4, image);
        return;
    }

    int FATnextClusterOffset = getFATentryOffset(fat32vars, nextCluster);

    //Convert next cluster into little endian
    unsigned char *nextHex = malloc(4);
    nextHex[0] = bitExtracted(nextCluster, 8, 1);
    nextHex[1] = bitExtracted(nextCluster, 8, 9);
    nextHex[2] = bitExtracted(nextCluster, 8, 17);
    nextHex[3] = bitExtracted(nextCluster, 8, 25);

    //Mark cluster to point to next cluster
    fseek(image, FATclusterOffset, SEEK_SET);
    fwrite(nextHex, 1, 4, image);
    return;

    //Mark next cluster as the last cluster
    fseek(image, FATnextClusterOffset, SEEK_SET);
    fwrite(emptyHex, 1, 4, image);
    return;

    //write new clusterNum to the FAT (32 bytes after the last cluster)
    unsigned char freeClusHex[4];
    freeClusHex[0] = bitExtracted(cluster, 8, 1);
    freeClusHex[1] = bitExtracted(cluster, 8, 9);
    freeClusHex[2] = bitExtracted(cluster, 8, 17);
    freeClusHex[3] = bitExtracted(cluster, 8, 25);
    fseek(image, FAToffset, SEEK_SET);
    //fwrite(freeClusHex, 1, 4, image);

    //write 0FFFFFFF after clusterNum
    fseek(image, FAToffset, SEEK_SET);
    //fwrite(emptyHex, 1, 4, image);

    return;
}

//takes cluster in a file and expands the clusters in that file
void expandCluster(FILE *image, varStruct *fat32vars, int cluster){
    int prevClus = cluster;
    while(1){
        int nextClus = getNextClusterNumber(image, fat32vars, prevClus);
        if (prevClus == nextClus){   //finds last cluster in that file
            int freeClus = getFirstFreeCluster(image, fat32vars);   //allocate first free cluster and make it the last cluster of file
            markFATentry(image, fat32vars, nextClus, freeClus);
            markFATentry(image, fat32vars, freeClus, 0);
            break;
        }
        prevClus = nextClus;
    }
   
}

//fill in dir with appropriate values (dirName and first cluster, mainly)
void makeDirEntry(varStruct *fat32vars, struct DIRENTRY *dir, char* dirName, int firstClusterNum){
    
    //Set all bytes to 0 to clean allocated memory
    memset(dir, 0, 32);
    
    //Set first 11 bytes as spaces for the name entry
    memset(dir, 32, 11);
    
    //Copy passed in name
    int loop;
    for(loop = 0; loop < strlen(dirName); ++loop) {
        dir->DIR_Name[loop] = dirName[loop];
    }

    //Set attribute
    dir->DIR_Attr = ATTR_DIRECTORY;

    //extract lower bits of first cluster
    unsigned char lowBits[2];
    dir->DIR_FstClusLO[0] = bitExtracted(firstClusterNum, 8, 1);
    dir->DIR_FstClusLO[1] = bitExtracted(firstClusterNum, 8, 9);

    //extract higher bits of first cluster
    unsigned char highBits[2];
    dir->DIR_FstClusHI[0] = bitExtracted(firstClusterNum, 8, 17);
    dir->DIR_FstClusHI[1] = bitExtracted(firstClusterNum, 8, 25);
}

//Translate direntry into 32 bit string to be written to image
char *directoryToString(struct DIRENTRY *dir){

    char *dirStr = malloc(32);

    //Set all bytes to 0 to clean allocated memory
    memset(dirStr, 0, 32);
    
    //Set first 11 bytes as spaces for the name entry
    memset(dirStr, 32, 11);

    //Copy name
    memcpy(dirStr, dir->DIR_Name, 8);

    //Copy other values
    memcpy(&dirStr[11], &dir->DIR_Attr, 21);

    return dirStr;
}

//Convert dirname and firstClusterNum into a directory entry string
char *makeDirEntryString(varStruct *fat32vars, char *dirname, int firstClusterNum) {
    char *dirString = malloc(32);

    //Set all bytes to 0 to clean allocated memory
    memset(dirString, 0, 32);

    //Set first 11 bytes as spaces for the name entry
    memset(dirString, 32, 11);

    //Copy passed in name
    int loop;
    for(loop = 0; loop < strlen(dirname); ++loop) {
        dirString[loop] = toupper(dirname[loop]);
    }

    //Set attribute
    dirString[11] = ATTR_DIRECTORY;

    //extract lower bits of first cluster
    unsigned char lowBits[2];
    dirString[26] = bitExtracted(firstClusterNum, 8, 1);
    dirString[27] = bitExtracted(firstClusterNum, 8, 9);

    //extract higher bits of first cluster
    unsigned char highBits[2];
    dirString[20] = bitExtracted(firstClusterNum, 8, 17);
    dirString[21] = bitExtracted(firstClusterNum, 8, 25);

    return dirString;
}

//Convert filename and firstClusterNum into a file entry string
char *makeFileEntryString(varStruct *fat32vars, char *filename, int firstClusterNum) {
    char *dirString = malloc(32);

    //Set all bytes to 0 to clean allocated memory
    memset(dirString, 0, 32);

    //Set first 11 bytes as spaces for the name entry
    memset(dirString, 32, 11);

    //Copy passed in name
    int loop;
    for(loop = 0; loop < strlen(filename); ++loop) {
        dirString[loop] = toupper(filename[loop]);
    }

    //Set attribute
    dirString[11] = ATTR_VOLUME_ID | ATTR_ARCHIVE;

    //extract lower bits of first cluster
    unsigned char lowBits[2];
    dirString[26] = bitExtracted(firstClusterNum, 8, 1);
    dirString[27] = bitExtracted(firstClusterNum, 8, 9);

    //extract higher bits of first cluster
    unsigned char highBits[2];
    dirString[20] = bitExtracted(firstClusterNum, 8, 17);
    dirString[21] = bitExtracted(firstClusterNum, 8, 25);

    return dirString;
}

//returns k bits of number at position p
int bitExtracted(int number, int k, int p) { 
    return (((1 << k) - 1) & (number >> (p - 1))); 
} 