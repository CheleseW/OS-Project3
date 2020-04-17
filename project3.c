#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

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

void parseInput(instruction* instr_ptr);
void addToken(instruction* instr_ptr, char* tok);
void addNull(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void expandVariables(instruction* instr_ptr);

void fat32info(varStruct fat32vars);
void fat32size(FILE *image, varStruct fat32vars, instruction* instr_ptr);
void fat32ls(instruction* instr_ptr);
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
            fat32size(input, fat32vars, &instr);
        }
        else if(strcmp(first, "ls")==0) {
            fat32ls(&instr);
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

void fat32size(FILE *image, varStruct fat32vars, instruction* instr_ptr) {

		if(instr_ptr->numTokens != 2) {
			printf("size : Invalid number of arguments\n");
			return;
		}

		unsigned char DIR_name[11];

		uint8_t *temp;
		uint8_t *DIR_attr;
		uint8_t DIR_size;
		uint8_t FILE_size;
    temp = malloc(2);
		int offset = fat32vars.BPB_BytsPerSec * (fat32vars.BPB_RsvdSecCnt+ (fat32vars.BPB_NumFATs * fat32vars.BPB_FATSz32));
		printf("Offset: %d\n", offset);

    fseek(image, offset, SEEK_SET);
    DIR_name[0] = fgetc(image);
    DIR_name[1] = fgetc(image);
		DIR_name[2] = fgetc(image);
    DIR_name[3] = fgetc(image);
		DIR_name[4] = fgetc(image);
    DIR_name[5] = fgetc(image);
		DIR_name[6] = fgetc(image);
    DIR_name[7] = fgetc(image);
		DIR_name[8] = fgetc(image);
    DIR_name[9] = fgetc(image);
		DIR_name[10] = fgetc(image);
		temp[0] = fgetc(image);
    //DIR_attr = littleToBigEndian(DIR_attr, 2);

		if(temp[0] == 15){

			printf("Long Directory Name... IGNORE\n");
			fseek(image, offset+32, SEEK_SET);
	    DIR_name[0] = fgetc(image);
	    DIR_name[1] = fgetc(image);
			DIR_name[2] = fgetc(image);
	    DIR_name[3] = fgetc(image);
			DIR_name[4] = fgetc(image);
	    DIR_name[5] = fgetc(image);
			DIR_name[6] = fgetc(image);
	    DIR_name[7] = fgetc(image);
			DIR_name[8] = fgetc(image);
	    DIR_name[9] = fgetc(image);
			DIR_name[10] = fgetc(image);
			temp[0] = fgetc(image);
		}

		fseek(image, offset+28, SEEK_SET);
		temp[0] = fgetc(image);
		temp[1] = fgetc(image);
		temp[2] = fgetc(image);
		temp[3] = fgetc(image);
		DIR_size = littleToBigEndian(temp, 4);


		int directoryOffset = offset + DIR_size;
		offset = offset+32;
		int newoffset = offset;
		unsigned char file_name[11];


		while(newoffset < directoryOffset){
				newoffset = newoffset + 32;

				fseek(image, newoffset, SEEK_SET);
				file_name[0] = fgetc(image);
				file_name[1] = fgetc(image);
				file_name[2] = fgetc(image);
				file_name[3] = fgetc(image);
				file_name[4] = fgetc(image);
				file_name[5] = fgetc(image);
				file_name[6] = fgetc(image);
				file_name[7] = fgetc(image);
				file_name[8] = fgetc(image);
				file_name[9] = fgetc(image);
				file_name[10] = fgetc(image);
				temp[0] = fgetc(image);
				//DIR_attr = littleToBigEndian(DIR_attr, 2);

				if(temp[0] == 15){
					//printf("Long Directory Name... IGNORE\n");
					newoffset = newoffset + 32;
					fseek(image, newoffset, SEEK_SET);
					file_name[0] = fgetc(image);
					file_name[1] = fgetc(image);
					file_name[2] = fgetc(image);
					file_name[3] = fgetc(image);
					file_name[4] = fgetc(image);
					file_name[5] = fgetc(image);
					file_name[6] = fgetc(image);
					file_name[7] = fgetc(image);
					file_name[8] = fgetc(image);
					file_name[9] = fgetc(image);
					file_name[10] = fgetc(image);
				}

				//printf("Filename: |%s|\n", file_name);

				int i;
				int match = 1;
				for(i = 0; i < strlen(instr_ptr->tokens[1]); i++){
					if(instr_ptr->tokens[1][i] != file_name[i])
						match = 0;
				}

				if(match){
					printf("Filename: |%s|\n", file_name);

					fseek(image, newoffset+28, SEEK_SET);
					temp[0] = fgetc(image);
					temp[1] = fgetc(image);
					temp[2] = fgetc(image);
					temp[3] = fgetc(image);
					FILE_size = littleToBigEndian(temp, 4);
				}
		}
		//ATTR_LONG_NAME is defined as follows: (ATTR_READ_ONLY | ATTR_HIDDEN |  ATTR_SYSTEM | ATTR_VOLUME_ID)



		printf("DIR_name: |%s|\n", DIR_name);
		printf("DIR_attr: |%d|\n", DIR_size);



    printf("Size of %s: %d\n", instr_ptr->tokens[1], FILE_size);
    return;
}

void fat32ls(instruction* instr_ptr) {
    if(instr_ptr->numTokens > 2) {
		printf("ls : Invalid number of arguments\n");
		return;
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
