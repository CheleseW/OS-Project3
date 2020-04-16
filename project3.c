#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>

#define Swap2Bytes(val) \
    ( (((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00) )
#define ATTR_READ_ONLY 1        //dirEntry attribute definitions
#define ATTR_HIDDEN 2
#define ATTR_SYSTEM 4
#define ATTR_VOLUME_ID 8
#define ATTR_DIRECTORY 16
#define ATTR_ARCHIVE 32
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

typedef struct
{
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint32_t BPB_RootClus;
} varStruct;

struct DIRENTRY {
    unsigned char DIR_name[11]; //0-10
    unsigned char DIR_Attributes; //11
    unsigned char DIR_NTRes; //12
    unsigned char DIR_CrtTimeTenth; //13
    unsigned char DIR_CrtTime[2]; //14-15
    unsigned char DIR_CrtDate[2]; //16-17
    unsigned char DIR_LstAccDate[2]; //18-19
    unsigned char DIR_FstClusHI[2]; //20-21
    unsigned char DIR_WrtTime[2]; //22-23
    unsigned char DIR_WrtDate[2]; //24-25
    unsigned char DIR_FstClusLO[2]; //26-27
    unsigned char DIR_FileSize[4]; //28-31
}   __attribute__((packed));

void parseInput(instruction* instr_ptr);
void addToken(instruction* instr_ptr, char* tok);
void addNull(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void expandVariables(instruction* instr_ptr);

void fat32info(varStruct fat32vars);
void fat32size(instruction* instr_ptr);
void fat32ls(instruction* instr_ptr, varStruct bs);
void fat32cd(instruction* instr_ptr);
void fat32create(instruction* instr_ptr);
void fat32mkdir(instruction* instr_ptr);
void fat32mv(instruction* instr_ptr);
void fat32open(instruction* instr_ptr);
void fat32close(instruction* instr_ptr);
void fat32read(instruction* instr_ptr);
void fat32write(instruction* instr_ptr);
void fat32rm(instruction* instr_ptr);
void fat32cp(instruction* instr_ptr);

int littleToBigEndian(uint8_t *address, int bytes);
void fat32initVars(FILE *image, varStruct *fat32vars);
int getFirstDataSector(varStruct bs);
int getSectorOffset(varStruct bs, int clusterNum);
int getByteOffset(varStruct bs, int clusterNum);
void populateDirEntry(struct DIRENTRY * entry, unsigned long byteNum);
unsigned char* swap2Bytes(unsigned char* val);
unsigned char* swap4Bytes(unsigned char* val);

int main (int argc, char* argv[]) {
    //Variables
    FILE *input;
    int offset;
    char *first;
    uint8_t inputByte;
    varStruct fat32vars;

	char *fat32PWD = malloc(100);

    instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

    //Check arguments
    if(argc != 2) {
        printf("Invalid number of arguments\n");
        return 1;
    }

    //Open file passed in through command line
    //"rb" opens file for reading as binary
    if((input = fopen(argv[1], "rb")) == NULL) {
        printf("File could not be found\n");
        return 1;
    }

    //Initialize fat32 vars
    fat32initVars(input, &fat32vars);

	while(1) {
		printf("%s/%s> ", basename(argv[1]), fat32PWD);	//PROMPT

		//Parse instruction
		parseInput(&instr);
			
		//Check for command
		first = instr.tokens[0];
        if(strcmp(first, "exit")==0) {
            printf("Exiting now!\n");
            break;
        }
        else if(strcmp(first, "info")==0) {
            fat32info(fat32vars);
        }
        else if(strcmp(first, "size")==0) {
            fat32size(&instr);
        }
        else if(strcmp(first, "ls")==0) {
            fat32ls(&instr, fat32vars);
        }
        else if(strcmp(first, "cd")==0) {
            fat32cd(&instr);
        }
        else if(strcmp(first, "create")==0) {
            fat32create(&instr);
        }
        else if(strcmp(first, "mkdir")==0) {
            fat32mkdir(&instr);
        }
        else if(strcmp(first, "mv")==0) {
            fat32mv(&instr);
        }
        else if(strcmp(first, "open")==0) {
            fat32open(&instr);
        }
        else if(strcmp(first, "close")==0) {
            fat32close(&instr);
        }
        else if(strcmp(first, "read")==0) {
            fat32read(&instr);
        }
        else if(strcmp(first, "write")==0) {
            fat32write(&instr);
        }
        else if(strcmp(first, "rm")==0) {
            fat32rm(&instr);
        }
        else if(strcmp(first, "cp")==0) {
            fat32cp(&instr);
        }
        else {
            printf("Unknown command\n");
        }

		//Clear instruction
		clearInstruction(&instr);
        
	}
    

    //Close file
    fclose(input);

    return 0;
}

void parseInput(instruction* instr_ptr) {

	char* token = NULL;
	char* temp = NULL;

	// loop reads character sequences separated by whitespace
	do {
		//scans for next token and allocates token var to size of scanned token
		scanf("%ms", &token);
		temp = malloc(strlen(token)+1);

		int i;
		int start = 0;
		for (i = 0; i < strlen(token); i++) {
			//pull out special characters and make them into a separate token in the instruction
			if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
				if (i-start > 0) {
					memcpy(temp, token + start, i - start);
					temp[i-start] = '\0';
					addToken(instr_ptr, temp);
				}

				char specialChar[2];
				specialChar[0] = token[i];
				specialChar[1] = '\0';
				addToken(instr_ptr,specialChar);
				start = i + 1;
			}
		}

		if (start < strlen(token)) {
			memcpy(temp, token + start, strlen(token) - start);
			temp[i-start] = '\0';
			addToken(instr_ptr, temp);
		}

		//free and reset variables
		free(token);
		free(temp);
		token = NULL;
		temp = NULL;

	} while ('\n' != getchar());    //until end of line is reached

	addNull(instr_ptr);
}

void addToken(instruction* instr_ptr, char* tok){
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr){
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;

	//instr_ptr->numTokens++; <-----Why increment the number of tokens for a null token
}

void clearInstruction(instruction* instr_ptr){
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}

void fat32info(varStruct fat32vars) {

    printf("Bytes per sector: %d\n", fat32vars.BPB_BytsPerSec);
    printf("Sectors per cluster: %d\n", fat32vars.BPB_SecPerClus);
    printf("Reserved sector count: %d\n", fat32vars.BPB_RsvdSecCnt);
    printf("Number of FATs: %d\n", fat32vars.BPB_NumFATs);
    printf("Total Sectors: %d\n", fat32vars.BPB_TotSec32);
    printf("FAT size: %d\n", fat32vars.BPB_FATSz32);
    printf("Root Cluster: %d\n", fat32vars.BPB_RootClus);

}

void fat32size(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 2) {
		printf("size : Invalid number of arguments\n");
		return;
	}
    printf("Size of %s: %d\n", instr_ptr->tokens[1], 6969);
    return;
}

void fat32ls(instruction* instr_ptr, varStruct bs) {
    if(instr_ptr->numTokens > 2) {
		printf("ls : Invalid number of arguments\n");
		return;
	}
    else {
        struct DIRENTRY dir;
        unsigned long rootLoc = getByteOffset(bs, bs.BPB_RootClus);
        unsigned long dirLoc = rootLoc + 32;
        unsigned long increment = 64;
        if (instr_ptr->numTokens == 1){
            //read current directory
        }
        else{
            char* directoryName = instr_ptr->tokens[1];
            do {
                populateDirEntry(&dir, dirLoc);
                if (!memcmp(dir.DIR_name, directoryName, 11)){
                    //enter directory and print all files/subdirectories
                    dirLoc = getByteOffset(bs, littleToBigEndian(dir.DIR_FstClusLO, 4));
                    populateDirEntry(&dir, dirLoc);
                    while (dir.DIR_name[0] != 0x0){
                        printf("%s\t", dir.DIR_name);
                        dirLoc += increment;
                        populateDirEntry(&dir, dirLoc);
                    }
                    printf("\n");
                    return;
                }
                else{
                    dirLoc += increment;
                }
            }
            while (dir.DIR_name[0] != 0x0);
            printf("Directory '%s' not found\n", directoryName);
        }
    }
    
    printf("Files in directory\n");
}

void fat32cd(instruction* instr_ptr) {		
	if(instr_ptr->numTokens != 2) {
		printf("cd : Invalid number of arguments\n");
		return;
	}
    printf("Changing directory to %s\n", instr_ptr->tokens[1]);
}

void fat32create(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 2) {
		printf("create : Invalid number of arguments\n");
		return;
	}
    printf("Created file %s\n", instr_ptr->tokens[1]);
}

void fat32mkdir(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 2) {
		printf("mkdir : Invalid number of arguments\n");
		return;
	}
    printf("Created directory %s\n", instr_ptr->tokens[1]);
}

void fat32mv(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 3) {
		printf("mv : Invalid number of arguments\n");
		return;
	}
    printf("Moved %s to %s\n", instr_ptr->tokens[1], instr_ptr->tokens[2]);
}

void fat32open(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 3) {
		printf("open : Invalid number of arguments\n");
		return;
	}
    printf("Opened file %s in mode %s\n", instr_ptr->tokens[1], instr_ptr->tokens[2]);
}

void fat32close(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 2) {
		printf("close : Invalid number of arguments\n");
		return;
	}
    printf("Closed file %s\n", instr_ptr->tokens[1]);
}

void fat32read(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 4) {
		printf("read : Invalid number of arguments\n");
		return;
	}
    printf("Reading file %s\n", instr_ptr->tokens[1]);
}

void fat32write(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 5) {
		printf("write : Invalid number of arguments\n");
		return;
	}
    printf("Writing file %s\n", instr_ptr->tokens[1]);
}

void fat32rm(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 2) {
		printf("rm : Invalid number of arguments\n");
		return;
	}
    printf("Removed file %s\n", instr_ptr->tokens[1]);
}

void fat32cp(instruction* instr_ptr) {
    if(instr_ptr->numTokens != 3) {
		printf("cp : Invalid number of arguments\n");
		return;
	}
    printf("Copying %s to %s\n", instr_ptr->tokens[1], instr_ptr->tokens[2]);
}

int littleToBigEndian(uint8_t *address, int bytes) {
    if(bytes == 2) return address[0] | address[1] << 8;
    if(bytes == 4) return address[0] | address[1] << 8 | address[2] << 16 | address[3] << 24;
}

unsigned char* swap2Bytes(unsigned char* val){      //convert to big endian but return swapped hex values
    unsigned char* res;
    res = malloc(2);
    res[0] = val[1];
    res[1] = val[0];
    return res;
}

unsigned char* swap4Bytes(unsigned char* val){      //convert to big endian but return swapped hex values
    unsigned char* res;
    res = malloc(4);
    res[0] = val[3];
    res[1] = val[2];
    res[2] = val[1];
    res[3] = val[0];
    return res;
}

int getFirstDataSector(varStruct bs){
    return bs.BPB_RsvdSecCnt + (bs.BPB_NumFATs * bs.BPB_FATSz32);
}

int getSectorOffset(varStruct bs, int clusterNum){
    return ((clusterNum - 2) * bs.BPB_SecPerClus) + getFirstDataSector(bs);
}

int getByteOffset(varStruct bs, int clusterNum){                              //use this function to find which byte the specified cluster starts at
    return getSectorOffset(bs, clusterNum) * bs.BPB_BytsPerSec;
}

void fat32initVars(FILE *image, varStruct *fat32vars) {
    uint8_t *temp;
    temp = malloc(4);

    fseek(image, 11, SEEK_SET);
    temp[0] = fgetc(image);
    temp[1] = fgetc(image);
    fat32vars->BPB_BytsPerSec = littleToBigEndian(temp, 4);

    fseek(image, 13, SEEK_SET);
    fat32vars->BPB_SecPerClus = fgetc(image);

    fseek(image, 14, SEEK_SET);
    temp[0] = fgetc(image);
    temp[1] = fgetc(image);
    fat32vars->BPB_RsvdSecCnt = littleToBigEndian(temp, 4);

    fseek(image, 16, SEEK_SET);
    fat32vars->BPB_NumFATs = fgetc(image);

    fseek(image, 32, SEEK_SET);
    temp[0] = fgetc(image);
    temp[1] = fgetc(image);
    temp[2] = fgetc(image);
    temp[3] = fgetc(image);
    fat32vars->BPB_TotSec32 = littleToBigEndian(temp, 4);

    fseek(image, 36, SEEK_SET);
    temp[0] = fgetc(image);
    temp[1] = fgetc(image);
    temp[2] = fgetc(image);
    temp[3] = fgetc(image);
    fat32vars->BPB_FATSz32 = littleToBigEndian(temp, 4);

    fseek(image, 44, SEEK_SET);
    temp[0] = fgetc(image);
    temp[1] = fgetc(image);
    temp[2] = fgetc(image);
    temp[3] = fgetc(image);
    fat32vars->BPB_RootClus = littleToBigEndian(temp, 4);

}

void populateDirEntry(struct DIRENTRY * entry, unsigned long byteNum){
    unsigned char * temp;
    temp = malloc(12);

    //open file
    int filedesc = open("fat32.img", O_RDWR);       //open() takes the filename and read/write permissions and returns a file descriptor (int)
    if (filedesc < 0){
        printf("Error opening file...");
    }

    //get dir name
    lseek(filedesc, byteNum, SEEK_SET);             //lseek() takes the filedescriptor and jumps to the offset (byteNum)
    if (read(filedesc, temp, 11) < 0){              //read() reads from the file and stores the specified number of characters (11, in this case) in temp
        printf("Error reading dirname");
    }
    memcpy(entry->DIR_name, temp, 11);       //find a way to convert hex to ascii

    //get dir attributes
    lseek(filedesc, byteNum+11, SEEK_SET);
    if (read(filedesc, temp, 1) < 0){
        printf("Error reading attributes");
    }
    entry->DIR_Attributes = littleToBigEndian(temp, 4);

    //get dir reserved space
    lseek(filedesc, byteNum+12, SEEK_SET);
    if (read(filedesc, temp, 1) < 0){
        printf("Error reading reserved space");
    }
    entry->DIR_NTRes = littleToBigEndian(temp, 4);

    //get dir creation time (tenths of a second)
    lseek(filedesc, byteNum+13, SEEK_SET);
    if (read(filedesc, temp, 1) < 0){
        printf("Error reading creation time tenths");
    }
    entry->DIR_CrtTimeTenth = littleToBigEndian(temp, 4);

    //get dir creation time
    lseek(filedesc, byteNum+14, SEEK_SET);
    if (read(filedesc, temp, 2) < 0){
        printf("Error reading creation time");
    }
    memcpy(entry->DIR_CrtTime, swap2Bytes(temp), 2);

    //get dir creation date
    lseek(filedesc, byteNum+16, SEEK_SET);
    if (read(filedesc, temp, 2) < 0){
        printf("Error reading creation date");
    }
    memcpy(entry->DIR_CrtDate, swap2Bytes(temp), 2);

    //get dir last access date
    lseek(filedesc, byteNum+18, SEEK_SET);
    if (read(filedesc, temp, 2) < 0){
        printf("Error reading last access date");
    }
    memcpy(entry->DIR_LstAccDate, swap2Bytes(temp), 2);

    //get dir first cluster high word
    lseek(filedesc, byteNum+20, SEEK_SET);
    if (read(filedesc, temp, 2) < 0){
        printf("Error reading first cluster high word");
    }
    memcpy(entry->DIR_FstClusHI, swap2Bytes(temp), 2);

    //get dir last write time
    lseek(filedesc, byteNum+22, SEEK_SET);
    if (read(filedesc, temp, 2) < 0){
        printf("Error reading last write time");
    }
    memcpy(entry->DIR_WrtTime, swap2Bytes(temp), 2);

    //get dir last write date
    lseek(filedesc, byteNum+24, SEEK_SET);
    if (read(filedesc, temp, 2) < 0){
        printf("Error reading last write date");
    }
    memcpy(entry->DIR_WrtDate, swap2Bytes(temp), 2);

    //get dir first cluster low word
    lseek(filedesc, byteNum+26, SEEK_SET);
    if (read(filedesc, temp, 2) < 0){
        printf("Error reading first cluster low word");
    }
    memcpy(entry->DIR_FstClusLO, swap2Bytes(temp), 2);

    //get dir last access date
    lseek(filedesc, byteNum+28, SEEK_SET);
    if (read(filedesc, temp, 4) < 0){
        printf("Error reading file size");
    }
    memcpy(entry->DIR_FileSize, swap4Bytes(temp), 2);
}