#include "FAT.h"

int main() {

	if (createDisc() == ERROR_FILE_CREATING) { printf("Couldnt create a disc"); return 0; }
	
	//FILE* file = fopen(DISC_NAME, "rb");
	if (copyOnDisc("example_file.txt", "example_file.txt") == ERROR_NAME_NOT_UNIQUE) printf("File with that name exist.\n");
	showInsides();
	if (copyOnDisc("example_file.txt", "example_file2.txt") == ERROR_NAME_NOT_UNIQUE) printf("File with that name exist.\n");
	showInsides();
	if (copyOnDisc("example_file.txt", "example_file3.txt") == ERROR_NAME_NOT_UNIQUE) printf("File with that name exist.\n");


	if (deleteFile("example_file.txt") != 1) printf("Nie idziala\n");
	if (copyOnDisc("example3blocks.txt", "example_file4.txt") == ERROR_NAME_NOT_UNIQUE) printf("File with that name exist.\n");
	if (copyFromDisc("example_file4.txt", "copied.txt") != 1) printf("Nie idziala\n");
	//if (deleteFile("example_file4.txt") != 1) printf("Nie idziala\n");
	showFolder();
	resetDisc();
	//if (deleteDisc() != 0) printf("nope\n");
//	int number;
//	fread(&number,sizeof(number),1,file);
	//printf("%d", number);
	//fclose(file);
	system("pause");
	return 1;
}