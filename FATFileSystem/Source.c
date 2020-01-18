#include "FAT.h"

int main() {

	if (createDisc() == ERROR_FILE_CREATING) {
		printf("Couldnt create a disc");
		return 0;
	}
	FILE* file = fopen(DISC_NAME, "rb");
	if (copyOnDisc("example_file.txt") < 0)
		printf("cant open\n");

//	int number;
//	fread(&number,sizeof(number),1,file);
	//printf("%d", number);
	fclose(file);
	system("pause");
	return 1;
}