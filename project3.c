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
void fat32create();
void fat32mkdir(FILE *image, char* filename, varStruct *fat32vars, instruction* instr_ptr);
void fat32mv();
void fat32open(varStruct *fat32vars, char* filename, int mode);
void fat32close(varStruct *fat32vars, char* filename);
void fat32read();
void fat32write();
void fat32rm(FILE *image, varStruct *fat32vars, char* filename);
void fat32cp();

int littleToBigEndian(unsigned char *address);
void fat32initVars(FILE *image, varStruct *fat32vars);
void fillDirectoryEntry(FILE* image, struct DIRENTRY *dir);
void jumpToDirectory(varStruct *fat32vars, struct DIRENTRY *dir);
void fillDirectories(FILE *image, varStruct *fat32vars);
int getNextClusterOffset(FILE *image, varStruct *fat32vars, int offset);
int getDirectoryStartOffset(varStruct *fat32vars, struct DIRENTRY *dir);
int compareFilenames(char *filename1, char *filename2);
int getFATentryOffset (varStruct *fat32vars, int offset);
void clearFATentries (FILE *image, varStruct *fat32vars, int FAToffset);
int getFirstFreeCluster(FILE *image, varStruct fat32vars);
void makeDirEntry(varStruct *fat32vars, struct DIRENTRY *dir, char* dirName, int clusterNum);
char* dirEntryText(struct DIRENTRY *dir);
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

        else if(strcmp(first, "create")==0) {
            if(instr.numTokens == 2) {
                fat32create();
            }
            else printf("create : Invalid number of arguments\n");
        }

        else if(strcmp(first, "mkdir")==0) {
            if(instr.numTokens == 2) {
                fat32mkdir(input, argv[1], &fat32vars, &instr);
            }
            else printf("mkdir : Invalid number of arguments\n");
        }

        else if(strcmp(first, "mv")==0) {
            if(instr.numTokens == 3) {
                fat32mv();
            }
            else printf("mv : Invalid number of arguments\n");
        }

        else if(strcmp(first, "open")==0) {
            if(instr.numTokens == 2) {
                fat32open(&fat32vars, instr.tokens[1], 0);
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
            if(instr.numTokens == 2) {
                fat32read();
            }
            else printf("read : Invalid number of arguments\n");
        }

        else if(strcmp(first, "write")==0) {
            if(instr.numTokens == 2) {
                fat32write();
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
                    fat32vars->currentDirectoryOffset = getDirectoryStartOffset(fat32vars, dir);
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

void fat32create() {

}

void fat32mkdir(FILE *image, char* filename, varStruct *fat32vars, instruction* instr_ptr) {
    fclose(image);
    image = fopen(filename, "rb+");                             //reopen file with write permissions
    int firstFreeClus = getFirstFreeCluster(image, *fat32vars);
    int freeClusOffset = (2050 + (firstFreeClus-2)) * 512;
    unsigned char * temp = malloc(4);
    int freeDirOffset = fat32vars->currentDirectoryOffset;

    //find first empty space in current directory, store in freeDirOffset
    do{
        fseek(image, freeDirOffset, SEEK_SET);
        fread(temp, 1, 4, image);
        if (littleToBigEndian(temp) != 0)
            freeDirOffset += 32;
    }
    while(littleToBigEndian(temp) != 0);

    //create new direntry at freeDirOffset
    struct DIRENTRY * dir = malloc(sizeof(struct DIRENTRY));
    makeDirEntry(fat32vars, dir, instr_ptr->tokens[1], firstFreeClus);  //fill in dir values in dir
    char* dirStr = dirEntryText(dir);                                   //get 32-bit dir entry
    fseek(image, freeDirOffset, SEEK_SET);                              //write dir entry at freeDirOffset
    fwrite(dirStr, 1, 32, image);
    free(dir);

    //make direntry for . and write to directory's cluster
    dir = malloc(sizeof(struct DIRENTRY));
    makeDirEntry(fat32vars, dir, ".", firstFreeClus);
    dirStr = dirEntryText(dir);
    fseek(image, freeClusOffset, SEEK_SET);
    fwrite(dirStr, 1, 32, image);
    free(dir);

    //make direntry for .. and write to directory's cluster
    dir = malloc(sizeof(struct DIRENTRY));
    int currCluster = (fat32vars->currentDirectoryOffset / 512) - 2050 + 2; //cluster number of pwd
    makeDirEntry(fat32vars, dir, "..", currCluster);
    dirStr = dirEntryText(dir);
    fseek(image, freeClusOffset+32, SEEK_SET);
    fwrite(dirStr, 1, 32, image);
    free(dir);

    //update currentDirectories with newly created directory
    fat32vars->currentDirectories = malloc(1);
    fillDirectories(image, fat32vars);
}

void fat32mv() {

}

void fat32open(varStruct *fat32vars, char* filename, int mode) {
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

            for(loop = 0; loop < fat32vars->numOpenFiles; ++loop) {
                if(getDirectoryStartOffset(fat32vars, dir) == fat32vars->openFiles[loop].offset &&
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

            file->offset = getDirectoryStartOffset(fat32vars, dir);
            file->mode = mode;
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

void fat32read() {

}

void fat32write() {

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
                //Clear out FAT entries
                int dirStartOffset = getDirectoryStartOffset(fat32vars, dir);
                int dirFATentryOffset = getFATentryOffset(fat32vars, dirStartOffset);
                printf("%d\n", dirFATentryOffset);
                if(dirFATentryOffset > 16392) clearFATentries(image, fat32vars, dirFATentryOffset);
                
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

//This function will segfault if you pass a pointer with less than 4 bytes of memory allocated
int littleToBigEndian(unsigned char *address) {
    return address[0] | address[1] << 8 | address[2] << 16 | address[3] << 24;
}

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

void fillDirectories(FILE *image, varStruct *fat32vars) {
    struct DIRENTRY dir;
    int dirCount = 0;
    int temp;

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

        //if(dir.DIR_Name[0] == 229 || dir.DIR_Attr == 15) continue;

        fat32vars->currentDirectories[dirCount] = dir;
        ++dirCount;

        fat32vars->currentDirectories = 
        realloc(fat32vars->currentDirectories, 
        sizeof(struct DIRENTRY)*(dirCount+1));

        if(dirCount % 16 == 0) {
            temp = getNextClusterOffset(image, fat32vars, fat32vars->currentDirectoryOffset);
            if(temp >= 268435447) break; 
            temp = (temp-2)*fat32vars->BPB_BytsPerSec + fat32vars->rootDirectoryOffset;
            //printf("Next cluster offset: %d\n", temp);
            fat32vars->currentDirectoryOffset = temp;
            fseek(image, fat32vars->currentDirectoryOffset, SEEK_SET);
        };

    }
    fat32vars->numDirectories = dirCount;
    fat32vars->currentDirectoryOffset = originalOffset;
}

void jumpToDirectory(varStruct *fat32vars, struct DIRENTRY *dir) {
    
    fat32vars->currentDirectoryOffset = getDirectoryStartOffset(fat32vars, dir);

    //Reallocate PWD for new directory
    fat32vars->fat32pwd = 
    realloc(fat32vars->fat32pwd, 
    strlen(fat32vars->fat32pwd) + strlen(dir->DIR_Name) + 2);

    strcat(fat32vars->fat32pwd, "/");
    strcat(fat32vars->fat32pwd, dir->DIR_Name);
    
    return;
}

int getFATentryOffset (varStruct *fat32vars, int offset) {
    int FAToffset = 
    fat32vars->BPB_BytsPerSec *
    fat32vars->BPB_SecPerClus *
    fat32vars->BPB_RsvdSecCnt;

    int rootCluster =
    fat32vars->rootDirectoryOffset / fat32vars->BPB_BytsPerSec;

    int currentCluster =
    (offset / fat32vars->BPB_BytsPerSec) - rootCluster;

    return FAToffset+((currentCluster+2)*4);
}

int getNextClusterOffset(FILE *image, varStruct *fat32vars, int offset) {
    
    fseek(image, getFATentryOffset(fat32vars, offset), SEEK_SET);

    unsigned char *temp;
    temp = malloc(4);

    fread(temp, 1, 4, image);

    int nextClusterOffset = littleToBigEndian(temp);

    //printf("Next cluster = %d\n", nextClusterOffset);

    return littleToBigEndian(temp);
}

int getDirectoryStartOffset(varStruct *fat32vars, struct DIRENTRY *dir) {

    unsigned char *temp = malloc(4);
    temp[0] = dir->DIR_FstClusLO[0];
    temp[1] = dir->DIR_FstClusLO[1];
    temp[2] = dir->DIR_FstClusHI[0];
    temp[3] = dir->DIR_FstClusHI[1];

    int dirSector;
    dirSector = littleToBigEndian(temp);

    if(dirSector == 0) 
        return fat32vars->rootDirectoryOffset;
    else
        return fat32vars->rootDirectoryOffset +
        ((dirSector-2) * fat32vars->BPB_BytsPerSec);

}

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

void clearFATentries (FILE *image, varStruct *fat32vars, int FAToffset) {

    fseek(image, FAToffset, SEEK_SET);

    unsigned char *FATentry = malloc(4);
    fread(FATentry, 1, 4, image);
    int FATcluster = littleToBigEndian(FATentry);

    int FATbaseOffset = fat32vars->BPB_RsvdSecCnt * fat32vars->BPB_BytsPerSec;
    int currentCluster = (FAToffset-FATbaseOffset) / 4;

    int clusterOffset =
    (currentCluster - 2) *
    fat32vars->BPB_BytsPerSec +
    fat32vars->rootDirectoryOffset;

    unsigned char *temp = malloc(fat32vars->BPB_BytsPerSec);
    memset(temp, 0, fat32vars->BPB_BytsPerSec);

    if(FATcluster < 268435447) {
        int FATnextEntryOffset = 
        (FATcluster*4) + 
        (fat32vars->BPB_RsvdSecCnt * fat32vars->BPB_BytsPerSec);
        clearFATentries (image, fat32vars, FATnextEntryOffset);
    }

    fseek(image, clusterOffset, SEEK_SET);
    fwrite(temp, 1, fat32vars->BPB_BytsPerSec, image);
    fseek(image, FAToffset, SEEK_SET);
    fwrite(temp, 1, 4, image);
    return;
}

//Traverses FAT and finds the first free cluster
//Writes free cluster in first available slot in FAT
int getFirstFreeCluster(FILE *image, varStruct fat32vars){
    int offset = (fat32vars.BPB_BytsPerSec * fat32vars.BPB_RsvdSecCnt) + (2 * 4);   //byte num of FAT
    unsigned char * temp1;
    temp1 = malloc(4);
    int increment = 4;
    int freeClus;   //stores cluster num of first available cluster

    //search FAT until you come across a 0
    do{
        offset += increment;
        fseek(image, offset, SEEK_SET);
        fread(temp1, 1, 4, image);
    }
    while(littleToBigEndian(temp1) != 0);

    //traverse back until you find the last clusterNum
    do{           
        offset -= increment;
        fseek(image, offset, SEEK_SET);           
        fread(temp1, 1, 4, image);
    }
    while(littleToBigEndian(temp1) == 0x0FFFFFFF);
    
    //calculate first available cluster
    freeClus = littleToBigEndian(temp1) + 9;
    int i;

    //fill in spaces until next cluster with 0FFFFFFF
    unsigned char emptyHex[4];
    emptyHex[0] = 255;
    emptyHex[1] = 255;
    emptyHex[2] = 255;
    emptyHex[3] = 15;
    for (i=0; i < 8; i++){
        offset += increment;
        fseek(image, offset, SEEK_SET);
        fwrite(emptyHex, 1, 4, image);
    }

    //write new clusterNum to the FAT (32 bytes after the last cluster)
    offset += increment;
    unsigned char freeClusHex[4];
    freeClusHex[0] = bitExtracted(freeClus, 8, 1);
    freeClusHex[1] = bitExtracted(freeClus, 8, 9);
    freeClusHex[2] = bitExtracted(freeClus, 8, 17);
    freeClusHex[3] = bitExtracted(freeClus, 8, 25);
    fseek(image, offset, SEEK_SET);
    fwrite(freeClusHex, 1, 4, image);

    //write 0FFFFFFF after clusterNum
    offset += 4;
    fseek(image, offset, SEEK_SET);
    fwrite(emptyHex, 1, 4, image);

    free(temp1);

    return freeClus;
}

//fill in dir with appropriate values (dirName and first cluster, mainly)
void makeDirEntry(varStruct *fat32vars, struct DIRENTRY *dir, char* dirName, int clusterNum){
    strcpy(dir->DIR_Name, dirName);
    dir->DIR_Attr = ATTR_DIRECTORY;
    dir->DIR_NTRes = 0;

    //extract lower bits of first cluster
    unsigned char lowBits[2];
    lowBits[0] = bitExtracted(clusterNum, 8, 1);
    lowBits[1] = bitExtracted(clusterNum, 8, 9);
    strcpy(dir->DIR_FstClusLO, lowBits);

    //extract higher bits of first cluster
    unsigned char highBits[2];
    highBits[0] = bitExtracted(clusterNum, 8, 17);
    highBits[1] = bitExtracted(clusterNum, 8, 25);
    strcpy(dir->DIR_FstClusHI, highBits);

    dir->DIR_CrtTimeTenth = 100;
    dir->DIR_CrtTime[0] = 0;
    dir->DIR_CrtTime[1] = 0;
    dir->DIR_CrtDate[0] = 0;
    dir->DIR_CrtDate[1] = 0;
    dir->DIR_LstAccDate[0] = 0;
    dir->DIR_LstAccDate[1] = 0;
    dir->DIR_WrtTime[0] = 0;
    dir->DIR_WrtTime[1] = 0;
    dir->DIR_WrtDate[0] = 0;
    dir->DIR_WrtDate[1] = 0;
    dir->DIR_FileSize[0] = 0;
    dir->DIR_FileSize[1] = 0;
    dir->DIR_FileSize[2] = 0;
    dir->DIR_FileSize[3] = 0;
}

//translate direntry into 32 bit string to be written to image
char* dirEntryText(struct DIRENTRY *dir){
    char* dirStr = malloc(32);
    int i;
    for (i=0; i < 11; i++){
        if (dir->DIR_Name[i] == 0){
            dirStr[i] = 32;
        }
        else{
            dirStr[i] = dir->DIR_Name[i];
        }
    }
    dirStr[11] = dir->DIR_Attr;
    dirStr[12] = dir->DIR_NTRes;
    dirStr[13] = dir->DIR_CrtTimeTenth;
    for (i=0; i < 2; i++){
        dirStr[i+14] = dir->DIR_CrtTime[i];
    }
    for (i=0; i < 2; i++){
        dirStr[i+16] = dir->DIR_CrtDate[i];
    }
    for (i=0; i < 2; i++){
        dirStr[i+18] = dir->DIR_LstAccDate[i];
    }
    for (i=0; i < 2; i++){
        dirStr[i+20] = dir->DIR_FstClusHI[i];
    }
    for (i=0; i < 2; i++){
        dirStr[i+22] = dir->DIR_WrtTime[i];
    }
    for (i=0; i < 2; i++){
        dirStr[i+24] = dir->DIR_WrtDate[i];
    }
    for (i=0; i < 2; i++){
        dirStr[i+26] = dir->DIR_FstClusLO[i];
    }
    for (i=0; i < 4; i++){
        dirStr[i+28] = dir->DIR_FileSize[i];
    }

    return dirStr;
}

//returns k bits of number at position p
int bitExtracted(int number, int k, int p) 
{ 
    return (((1 << k) - 1) & (number >> (p - 1))); 
} 