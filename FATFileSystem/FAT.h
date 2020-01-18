#define _CRT_SECURE_NO_DEPRECATE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//init data
#define DISC_NAME "fat_file_system.bin"
#define DATA_SIZE 1024*4
#define BLOCK_SIZE 64
#define FAT_SIZE DATA_SIZE/BLOCK_SIZE		//File Allocation Table
#define DTF_SIZE DATA_SIZE/BLOCK_SIZE		//Directory Table Format
#define MAX_FILENAME_SIZE 20

//errors
#define ERROR_FILE_CREATING -1
#define ERROR_DISC_NOT_FOUND -2
#define ERROR_FILE_NOT_FOUND -3
#define ERROR_NOT_ENOUGH_SPACE -4
#define ERROR_NAME_NOT_UNIQUE -5

//states of blocks
#define FREE 1
#define FULL 0

typedef struct SuperBlock {
	short firstFATBlock;
	short firstDataBlock;
	short sizeOfDisc;
	short numberOfFreeBlocks;
	short firstDTFBlock;
	short a;
	short b;
	short c;
}SuperBlock;

typedef struct File {
	char fileName[20];
	short startingBlock;
	short fileSize;
}File;

typedef struct FatRow {
	short state;
	short next;		//-1 -> end of file
}FatRow;

SuperBlock superBlock;
File DTF[DTF_SIZE];
FatRow FAT[FAT_SIZE];

void initValues() {
	for (int i = 0; i < DTF_SIZE; i++) {
		DTF[i].startingBlock = 0;
		DTF[i].fileSize = 0;
	}
	for (int i = 0; i < FAT_SIZE; i++) {
		FAT[i].state = FREE;
		FAT[i].next = -1;
	}
	superBlock.firstDataBlock = sizeof(SuperBlock) + sizeof(File) * DTF_SIZE + sizeof(FatRow) * FAT_SIZE;
	superBlock.firstFATBlock = sizeof(SuperBlock) + sizeof(File) * DTF_SIZE;
	superBlock.numberOfFreeBlocks = DATA_SIZE / BLOCK_SIZE;
	superBlock.sizeOfDisc = superBlock.firstDataBlock + DATA_SIZE;
	superBlock.firstDTFBlock = sizeof(SuperBlock);
}

int createDisc() {
	FILE* fptr = fopen(DISC_NAME, "wb");
	if (!fptr)
		return ERROR_FILE_CREATING;
	initValues();
	fwrite(&superBlock.firstFATBlock, sizeof(short), 4, fptr);
	for (int i = 0; i < DTF_SIZE; i++) {
		fwrite(&DTF[i].fileName, sizeof(char), 20, fptr);
		fwrite(&DTF[i].startingBlock, sizeof(short), 2, fptr);
	}
	for (int i = 0; i < FAT_SIZE; i++) {
		fwrite(&FAT[i].state, sizeof(short), 2, fptr);
	}
	fseek(fptr, DATA_SIZE - 1, SEEK_SET);
	char end = '\0';
	fwrite(&end, sizeof(char), 1, fptr);
	fclose(fptr);
	return 1;
}

void loadSuperBlock(FILE* disc) {
	fseek(disc, 0, SEEK_SET);
	fread(&superBlock, sizeof(superBlock), 1, disc);
}

void loadDTF(FILE* disc) {		//loaded superblock needed!
	fseek(disc, superBlock.firstDTFBlock, SEEK_SET);
	fread(&DTF[0], sizeof(File), DTF_SIZE, disc);
}

int isNameUnique(char* const name) {
	for (int i = 0; i < DTF_SIZE; i++)
		if (strcmp(DTF[i].fileName, name) == 0)
			return ERROR_NAME_NOT_UNIQUE;
	return 1;
}

int copyOnDisc(char* const name) {
	FILE* disc = fopen(DISC_NAME, "r+b");
	if (!disc)
		return ERROR_DISC_NOT_FOUND;
	FILE* file = fopen(name, "rb");
	if (!file)
		return ERROR_FILE_NOT_FOUND;
	fseek(file, 0, SEEK_END);
	int fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	int neededBlocks = fileSize / BLOCK_SIZE + 1;
	loadSuperBlock(disc);

	if (neededBlocks > superBlock.numberOfFreeBlocks)
		return ERROR_NOT_ENOUGH_SPACE;
	while (!isNameUnique(name)) {
		printf("Plik o tej nazwie juz istnieje. Wprowadz nowa nazwe lub enter by wyjsc: \n");
		scanf("%s", name);
		if (strcmp(name, "") == 0)
			return 1;
	}

	fclose(disc);
	fclose(file);
	return 1;
}

