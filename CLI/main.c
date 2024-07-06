
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <solis.h>

#define SOLIS_CLI_VERSION 1


char* readFileIntoString(const char* filename) 
{
    // Open the file in binary mode
    FILE* file = fopen(filename, "rb");
    if (file == NULL) 
    {
        perror("Error opening file");
        return NULL;
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize == -1) 
    {
        perror("Error determining file size");
        fclose(file);
        return NULL;
    }

    // Allocate memory for the string, including the null terminator
    char* fileContent = (char*)malloc(fileSize + 1);
    if (fileContent == NULL) 
    {
        perror("Error allocating memory");
        fclose(file);
        return NULL;
    }

    // Seek back to the beginning of the file and read its contents
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(fileContent, 1, fileSize, file);
    if (bytesRead != fileSize) 
    {
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



typedef void(*CommandSig)(void);

typedef struct Command {

	const char* cmd;
	int length;
	CommandSig func;

} Command;

static void help()
{
	printf(" -- Solis CLI Help -- \n\n");

    printf("- help -> prints the help message.\n");
    printf("- version -> prints the version message.\n\n");

    printf("- repl -> starts the repl for running code.\n");
    printf("          :help can be typed to get repl help while within the repl.\n\n");

    printf("- solis {filepath} -> executes a file immediately with a clean VM.\n");

}

static void version()
{
	printf(" Solis v%d.%d %s -- CLI build %d\n", SOLIS_MAJOR_VERSION, SOLIS_MINOR_VERSION, SOLIS_RELEASE_STRING, SOLIS_CLI_VERSION);
}

static void repl()
{
    // Assume our line can't be more than 256
    char line[256];

    VM vm;
    solisInitVM(&vm);

    char* last_filepath = NULL;

    for (;;)
    {
        printf("> ");
        fgets(line, 256, stdin);

        if (strcmp(":quit\n", line) == 0)
        {
            break;
        }
        else if (strncmp(":load", line, 5) == 0)
        {

            int commandLength = 6;

            // Load a file

            if (last_filepath != NULL)
                free(last_filepath);

            int len = strlen(line);

            if (len <= commandLength)
            {
                printf("Please input a file path\n");
                continue;
            }

            char* filepath = &line[commandLength];

            // Put a null terminator in the string
            filepath[strlen(filepath) - 1] = '\0';

            int pathlen = strlen(filepath);
            last_filepath = (char*)malloc((pathlen + 1) * sizeof(char));

            strncpy(last_filepath, filepath, pathlen);

            last_filepath[pathlen] = '\0';

            const char* file = readFileIntoString(filepath);

            if (file == NULL)
            {
                continue;
            }

            InterpretResult result = solisInterpret(&vm, file);

            if (result == INTERPRET_COMPILE_ERROR)
            {
                printf("Compile Error\n");
            }
            else if (result == INTERPRET_RUNTIME_ERROR)
            {
                printf("Runtime Error\n");
            }

        }
        else if (strncmp(":r", line, 2) == 0)
        {
            if (last_filepath == NULL)
            {
                printf("Can't reload a file if it doesn't exist\n");
                continue;
            }

            const char* file = readFileIntoString(last_filepath);

            if (file == NULL)
            {
                continue;
            }

            InterpretResult result = solisInterpret(&vm, file);

            if (result == INTERPRET_COMPILE_ERROR)
            {
                printf("Compile Error\n");
            }
            else if (result == INTERPRET_RUNTIME_ERROR)
            {
                printf("Runtime Error\n");
            }
        }
        else if (strncmp(":help", line, 5) == 0)
        {
            printf("-- Repl Commands --\n\n");

            printf(":load -> Loads and executes a file in the repl.\n");
            printf(":r -> reloads the previous file and executes it.\n");

            printf(":quit -> quits the repl.\n");

            printf("\n Code can be directly typed into the repl to be executed.\n");

            printf("\n");
        }
        else
        {

            InterpretResult result = solisInterpret(&vm, line);

            if (result == INTERPRET_COMPILE_ERROR)
            {
                printf("Compile Error\n");
            }
            else if (result == INTERPRET_RUNTIME_ERROR)
            {
                printf("Runtime Error\n");
            }
        }
    }

    if (last_filepath != NULL)
        free(last_filepath);

    solisFreeVM(&vm);

}


Command commands[] = {
	{ "help", 4, help },
	{ "version", 7, version },
    { "repl", 4, repl },
	{ "null", 4, NULL }
 
};


int main(int argc, char* argv[])
{

	// No arguments
	if (argc <= 1)
		return 0;

    const char* file = NULL;

	if (argc == 2)
	{
		// Get the arg
		const char* arg = argv[1];

        if (strlen(arg) == 0)
            return 1;

		int i = 0; 
		for (;;)
		{
			if (commands[i].func == NULL)
				break;

			if (strcmp(commands[i].cmd, arg) == 0)
			{
				commands[i].func();
				return 0;
			}

			i++;
		}

		// If we reach this point we should try and run the argument
		// We shall assume its a path

        file = readFileIntoString(arg);


	}

    // At this point the file buffer should be filled from various arguments
    // If it isn't it should've returned at an earlier point. 

    // if the file is null return
    if (file == NULL)
    {
        return 1;
    }

    // Initialise and run the VM with the code
    VM vm;

    solisInitVM(&vm);

    InterpretResult result = solisInterpret(&vm, file);

    if (result == INTERPRET_COMPILE_ERROR)
    {
        printf("Compile Error\n");
    }
    else if (result == INTERPRET_RUNTIME_ERROR)
    {
        printf("Runtime Error\n");
    }

    solisFreeVM(&vm);




	return 0;
}