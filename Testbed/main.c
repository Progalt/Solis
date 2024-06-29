
#include <stdio.h>

#include <solis.h>


char* readFileIntoString(const char* filename) {
    // Open the file in binary mode
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize == -1) {
        perror("Error determining file size");
        fclose(file);
        return NULL;
    }

    // Allocate memory for the string, including the null terminator
    char* fileContent = (char*)malloc(fileSize + 1);
    if (fileContent == NULL) {
        perror("Error allocating memory");
        fclose(file);
        return NULL;
    }

    // Seek back to the beginning of the file and read its contents
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(fileContent, 1, fileSize, file);
    if (bytesRead != fileSize) {
        perror("Error reading file");
        free(fileContent);
        fclose(file);
        return NULL;
    }

    // Null-terminate the string
    fileContent[fileSize] = '\0';

    // Close the file
    fclose(file);

    return fileContent;
}

void printNative(VM* vm)
{
    Value arg1 = vm->apiStack[0];

    solisPrintValue(arg1);

    printf("\n");

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());
}

int main(void) {


    char* fileContent = readFileIntoString("F:/Dev/Solis/Testbed/test.solis");
    if (fileContent == NULL) 
    {
        printf("Failed to read file\n");
        return EXIT_FAILURE;
    }

	VM vm;
	solisInitVM(&vm);

    solisPushGlobalCFunction(&vm, "println", printNative, 1);

	InterpretResult result = solisInterpret(&vm, fileContent);

	solisFreeVM(&vm);


    free(fileContent);


	return 0;
}