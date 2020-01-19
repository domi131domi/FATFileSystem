#define _CRT_SECURE_NO_DEPRECATE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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
	char fileName[MAX_FILENAME_SIZE];
	short startingBlock;
	short fileSize;		// -1 -> spot is free
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
		memset(DTF[i].fileName, 0, MAX_FILENAME_SIZE);
		DTF[i].startingBlock = 0;
		DTF[i].fileSize = -1;
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
	fwrite(&superBlock.firstFATBlock, sizeof(short), 8, fptr);
	for (int i = 0; i < DTF_SIZE; i++) {
		fwrite(&DTF[i].fileName, sizeof(char), MAX_FILENAME_SIZE, fptr);
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

void loadFAT(FILE* disc) {		//loaded superblock needed!
	fseek(disc, superBlock.firstFATBlock, SEEK_SET);
	fread(&FAT[0], sizeof(FatRow), FAT_SIZE, disc);
}

int doesFileExist(char* const name) {
	for (int i = 0; i < DTF_SIZE; i++)
		if (strcmp(DTF[i].fileName, name) == 0)
			return i;
	return -1;
}

void actualizeFAT(FILE* disc) {
	fseek(disc, superBlock.firstFATBlock, SEEK_SET);
	fwrite(&FAT[0], sizeof(FatRow), FAT_SIZE, disc);
}

void actualizeDTF(FILE* disc) {
	fseek(disc, superBlock.firstDTFBlock, SEEK_SET);
	fwrite(&DTF[0], sizeof(File), DTF_SIZE, disc);
}

void actualizeSuperBlock(FILE* disc) {
	fseek(disc, 0, SEEK_SET);
	fwrite(&superBlock, sizeof(SuperBlock), 1, disc);
}

int copyOnDisc(char* const name, char* const name_after) {
	FILE* disc = fopen(DISC_NAME, "r+b");
	if (!disc)
		return ERROR_DISC_NOT_FOUND;
	FILE* file = fopen(name, "rb");
	if (!file)
		return ERROR_FILE_NOT_FOUND;

	fseek(file, 0, SEEK_END);
	int fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	int neededBlocks = (int) ceil((double)fileSize / (double)BLOCK_SIZE);
	loadSuperBlock(disc);
	if (neededBlocks > superBlock.numberOfFreeBlocks)
		return ERROR_NOT_ENOUGH_SPACE;

	loadDTF(disc);
	if (doesFileExist(name_after) >= 0)
		return ERROR_NAME_NOT_UNIQUE;

	int firstfree = 0;
	while (DTF[firstfree].fileSize != -1)
		firstfree++;

	int previous = -1;
	int current = 0;
	fseek(file, 0, SEEK_SET);
	loadFAT(disc);
	for (int i = 0; i < neededBlocks; i++) {
		while (FAT[current].state == FULL)
			current++;
		if (i != 0)
			FAT[previous].next = current;
		else {
			strcpy(DTF[firstfree].fileName, name_after);
			DTF[firstfree].fileSize = fileSize;
			DTF[firstfree].startingBlock = current;
		}
		FAT[current].state = FULL;

		char block[BLOCK_SIZE];
		if (fileSize >= BLOCK_SIZE) {				//enough data to fill whole block
			fseek(disc, superBlock.firstDataBlock + current * BLOCK_SIZE, SEEK_SET);
			fread(&block, sizeof(char), BLOCK_SIZE, file);
			fwrite(&block, sizeof(char), BLOCK_SIZE, disc);
			fileSize -= BLOCK_SIZE;
		}
		else {													//not enough data in block, need to fill it!
			fseek(disc, superBlock.firstDataBlock + current * BLOCK_SIZE, SEEK_SET);
			fread(&block, sizeof(char), fileSize, file);
			for (int e = fileSize; e < BLOCK_SIZE; e++)
				block[e] = '\0';
			fwrite(&block, sizeof(char), BLOCK_SIZE, disc);
		}

		if (i == neededBlocks - 1)
			FAT[current].next = -1;
		previous = current;
	}
	superBlock.numberOfFreeBlocks -= neededBlocks;
	actualizeSuperBlock(disc);
	actualizeFAT(disc);
	actualizeDTF(disc);
	fclose(disc);
	fclose(file);
	return 1;
}

int copyFromDisc(char* const name, char* const name_after) {
	FILE* disc = fopen(DISC_NAME, "rb");
	if (!disc)
		return ERROR_DISC_NOT_FOUND;
	FILE* file = fopen(name_after, "wb");
	if (!file)
		return ERROR_FILE_CREATING;
	loadSuperBlock(disc);
	loadDTF(disc);
	int position = doesFileExist(name);
	if (position < 0)
		return ERROR_FILE_NOT_FOUND;
	int block = DTF[position].startingBlock;
	int size = DTF[position].fileSize;
	loadFAT(disc);
	char data[BLOCK_SIZE];
	while (block != -1) {
		if (size >= BLOCK_SIZE) {							//block wasnt filled
			fseek(disc, superBlock.firstDataBlock + block * BLOCK_SIZE, SEEK_SET);
			fread(data, sizeof(char), BLOCK_SIZE, disc);
			fwrite(data, sizeof(char), BLOCK_SIZE, file);
			block = FAT[block].next;
			size -= BLOCK_SIZE;
		}	
		else {												//block was filled
			fseek(disc, superBlock.firstDataBlock + block * BLOCK_SIZE, SEEK_SET);
			fread(data, sizeof(char), size, disc);
			fwrite(data, sizeof(char), size, file);
			block = FAT[block].next;
		}
	}
	fclose(disc);
	fclose(file);
	return 1;
}

void resetConnected(int DTFPosition, FILE* disc) {
	memset(DTF[DTFPosition].fileName, 0, MAX_FILENAME_SIZE);
	superBlock.numberOfFreeBlocks += (int)ceil((double)DTF[DTFPosition].fileSize / (double)BLOCK_SIZE);
	DTF[DTFPosition].fileSize = -1;
	int block = DTF[DTFPosition].startingBlock;
	DTF[DTFPosition].startingBlock = 0;
	char empty = '\0';
	int previous;
	while (block != -1) {
		fseek(disc, superBlock.firstDataBlock + block * BLOCK_SIZE, SEEK_SET);
		for (int i = 0; i < BLOCK_SIZE; i++)
			fwrite(&empty, sizeof(char), 1, disc);
		FAT[block].state = FREE;
		previous = block;
		block = FAT[block].next;
		FAT[previous].next = -1;
	}
}

int deleteFile(char* const name) {
	FILE* disc = fopen(DISC_NAME, "r+b");
	if (!disc)
		return ERROR_DISC_NOT_FOUND;
	loadSuperBlock(disc);
	loadDTF(disc);
	loadFAT(disc);
	int position = doesFileExist(name);
	if (position < 0)
		return ERROR_FILE_NOT_FOUND;
	resetConnected(position, disc);
	actualizeDTF(disc);
	actualizeFAT(disc);
	actualizeSuperBlock(disc);
	fclose(disc);
	return 1;
}

int showFolder() {
	FILE* disc = fopen(DISC_NAME, "r+b");
	if (!disc)
		return ERROR_DISC_NOT_FOUND;
	loadSuperBlock(disc);
	loadDTF(disc);
	for (int i = 0; i < DTF_SIZE; i++) {
		if (DTF[i].fileSize != -1)
			printf("%s ", DTF[i].fileName);
	}
	printf("\n");
	fclose(disc);
	return 1;
}

int resetDisc() {
	FILE* disc = fopen(DISC_NAME, "r+b");
	if (!disc)
		return ERROR_DISC_NOT_FOUND;

	initValues();
	actualizeSuperBlock(disc);
	actualizeDTF(disc);
	actualizeFAT(disc);
	fseek(disc, superBlock.firstDataBlock, SEEK_SET);
	char empty = '\0';
	for (int i = 0; i < superBlock.numberOfFreeBlocks * BLOCK_SIZE; i++) {
		fwrite(&empty, sizeof(char), 1, disc);
	}
	fclose(disc);
	return 1;
}

int deleteDisc() {
	FILE* disc = fopen(DISC_NAME, "rb");
	if (!disc)
		return ERROR_DISC_NOT_FOUND;
	fclose(disc);
	return remove(DISC_NAME);
}

int showInsides() {
	FILE* disc = fopen(DISC_NAME, "rb");
	if (!disc)
		return ERROR_DISC_NOT_FOUND;
	loadSuperBlock(disc);
	loadDTF(disc);
	loadFAT(disc);
	printf("Rozmiar Dysku: %d\n", superBlock.sizeOfDisc);
	printf("Ilosc wolnych blockow: %d\n", superBlock.numberOfFreeBlocks);
	printf("Byte, na ktorym zaczyna sie DTF: %d\n", superBlock.firstDTFBlock);
	printf("Byte, na ktorym zaczyna sie FAT: %d\n", superBlock.firstFATBlock);
	printf("Byte, na ktorym zaczyna sie DATA: %d\n", superBlock.firstDataBlock);
	printf("Directory Table Format\n");
	printf("Nazwa:    Rozmiar:    Pierwszy Blok:\n");
	for (int i = 0; i < DTF_SIZE; i++) {
		//if (DTF[i].fileSize != -1) {
			printf("%s  ", DTF[i].fileName);
			printf("%d  ", DTF[i].fileSize);
			printf("%d  \n", DTF[i].startingBlock);
	}
	printf("File Allocation Table\n");
	printf("Numer:    Status:    Nastepny:\n");
	for (int i = 0; i < FAT_SIZE; i++) {
		printf("%d  ", i);
		printf("%d  ", FAT[i].state);
		printf("%d  \n", FAT[i].next);
	}
	fclose(disc);
	return 1;
}