
#include "solis_core.h"

#include <float.h>

#include <stdio.h>
#include <stdlib.h>

char* read_file_into_cstring(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET); // Go back to the beginning of the file

    if (filesize < 0) {
        perror("Failed to determine file size");
        fclose(file);
        return NULL;
    }

    // Allocate memory for the file content plus the null terminator
    char* buffer = (char*)malloc(filesize + 1);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read the file content into the buffer
    size_t read_size = fread(buffer, 1, filesize, file);
    if (read_size != filesize) {
        perror("Failed to read file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Null-terminate the string
    buffer[filesize] = '\0';

    // Close the file
    fclose(file);

    return buffer;
}


void core_printf(VM* vm)
{

    printf("%s\n", SOLIS_AS_CSTRING(solisGetArgument(vm, 0)));

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());
}



void num_toString(VM* vm)
{
    double num = SOLIS_AS_NUMBER(solisGetSelf(vm));

    char buffer[24];
    int length = sprintf(buffer, "%.14g", num);

    Value ret = SOLIS_OBJECT_VALUE(solisCopyString(vm, buffer, length));

    solisSetReturnValue(vm, ret);
}


void string_length(VM* vm)
{
    solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE((double)(SOLIS_AS_STRING(solisGetSelf(vm))->length)));
}


void solisInitialiseCore(VM* vm)
{
    const char* str = read_file_into_cstring("F:/Dev/Solis/Solis/core.solis");

    solisPushGlobalCFunction(vm, "__c_printf", core_printf, 1);


    InterpretResult result = solisInterpret(vm, str);

    if (result == INTERPRET_RUNTIME_ERROR || result == INTERPRET_COMPILE_ERROR)
    {
        printf("Error running core module\n");
        return;
    }

    vm->numberClass = SOLIS_AS_CLASS(solisGetGlobal(vm, "Number"));

    solisAddClassField(vm, SOLIS_OBJECT_VALUE(vm->numberClass), "MIN", true, SOLIS_NUMERIC_VALUE(DBL_MIN));
    solisAddClassField(vm, SOLIS_OBJECT_VALUE(vm->numberClass), "MAX", true, SOLIS_NUMERIC_VALUE(DBL_MAX));
    solisAddClassField(vm, SOLIS_OBJECT_VALUE(vm->numberClass), "PI", true, SOLIS_NUMERIC_VALUE(3.14159265358979323846));
    solisAddClassField(vm, SOLIS_OBJECT_VALUE(vm->numberClass), "TAU", true, SOLIS_NUMERIC_VALUE(6.283185307179586));

    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->numberClass), "toString", num_toString, 0);

    vm->stringClass = SOLIS_AS_CLASS(solisGetGlobal(vm, "String"));

    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->stringClass), "length", string_length, 0);

    vm->boolClass = SOLIS_AS_CLASS(solisGetGlobal(vm, "Bool"));
}