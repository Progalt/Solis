
#include <stdio.h>

#include <solis.h>

#include <time.h>


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
    Value arg1 = solisGetArgument(vm, 0);

    solisPrintValue(arg1);

    printf("\n");

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());
}

void clockNative(VM* vm)
{
    double time = (double)clock() / CLOCKS_PER_SEC;

    solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE(time));
}



void cleanupList(void* mem)
{

}

void list_add(VM* vm)
{
    Value self = solisGetSelf(vm);
    Value field = solisGetInstanceField(vm, self, "_data");
    Value length = solisGetInstanceField(vm, self, "length");
    Value capacity = solisGetInstanceField(vm, self, "_capacity");

    ObjUserdata* userdata = NULL;

    if (!SOLIS_IS_USERDATA(field))
    {
        int newCapacity = GROW_CAPACITY((int)SOLIS_AS_NUMBER(capacity));

        Value* arr = (Value*)solisReallocate(vm, NULL, 0, sizeof(Value) * newCapacity);

        arr[0] = solisGetArgument(vm, 0);

        userdata = solisNewUserdata(vm, (void*)arr, cleanupList);

        solisSetInstanceField(vm, self, "length", SOLIS_NUMERIC_VALUE(1));
        solisSetInstanceField(vm, self, "_capacity", SOLIS_NUMERIC_VALUE((double)newCapacity));
        solisSetInstanceField(vm, self, "_data", SOLIS_OBJECT_VALUE(userdata));

        solisSetReturnValue(vm, SOLIS_NULL_VALUE());
    }
    else
    {
        userdata = SOLIS_AS_USERDATA(field);
        int cap = (int)SOLIS_AS_NUMBER(capacity);
        int len = (int)SOLIS_AS_NUMBER(length);

        Value* arr = (Value*)userdata->userdata;

        if (len + 1 >= cap)
        {
            int newCap = GROW_CAPACITY(cap);
            arr = (Value*)solisReallocate(vm, arr, sizeof(Value) * cap, sizeof(Value) * newCap);

            cap = newCap;
        }

        arr[len] = solisGetArgument(vm, 0);

        len++;

        solisSetInstanceField(vm, self, "length", SOLIS_NUMERIC_VALUE((double)len));
        solisSetInstanceField(vm, self, "_capacity", SOLIS_NUMERIC_VALUE((double)cap));
        solisSetInstanceField(vm, self, "_data", SOLIS_OBJECT_VALUE(userdata));

        solisSetReturnValue(vm, SOLIS_NULL_VALUE());
    }
}

void list_get(VM* vm)
{
    Value self = solisGetSelf(vm);
    Value field = solisGetInstanceField(vm, self, "_data");
    Value length = solisGetInstanceField(vm, self, "length");
    Value idx = solisGetArgument(vm, 0);

    if (SOLIS_AS_NUMBER(idx) >= SOLIS_AS_NUMBER(length) || !SOLIS_IS_USERDATA(field))
    {
        solisSetReturnValue(vm, SOLIS_NULL_VALUE());
        return;
    }

    Value* arr = SOLIS_AS_USERDATA(field)->userdata;

    solisSetReturnValue(vm, arr[(int)SOLIS_AS_NUMBER(idx)]);
   
}

int main(void) {


    char* fileContent = readFileIntoString("F:/Dev/Solis/Testbed/lists.solis");
    if (fileContent == NULL) 
    {
        printf("Failed to read file\n");
        return EXIT_FAILURE;
    }

	VM vm;
	solisInitVM(&vm);
    
    solisPushGlobalCFunction(&vm, "println", printNative, 1);
    solisPushGlobalCFunction(&vm, "clock", clockNative, 0);

    Value list = solisCreateClass(&vm, "List");

    solisAddClassField(&vm, list, "_data", false, SOLIS_NULL_VALUE());
    solisAddClassField(&vm, list, "_capacity", false, SOLIS_NUMERIC_VALUE(0));
    solisAddClassField(&vm, list, "length", false, SOLIS_NUMERIC_VALUE(0));
    solisAddClassNativeMethod(&vm, list, "add", list_add, 1);
    solisAddClassNativeMethod(&vm, list, "get", list_get, 1);

	InterpretResult result = solisInterpret(&vm, fileContent);

    if (result == INTERPRET_RUNTIME_ERROR)
    {
        printf("Interpreter runtime error. \n");
    }

    // solisDumpGlobals(&vm);

	solisFreeVM(&vm);


    free(fileContent);


	return 0;
}