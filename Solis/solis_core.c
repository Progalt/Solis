
#include "solis_core.h"
#include "core.solis.inc"

#include <float.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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


bool core_printf(VM* vm)
{

    printf("%s\n", SOLIS_AS_CSTRING(solisGetArgument(vm, 0)));

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());

    return true;
}



bool num_toString(VM* vm)
{
    double num = SOLIS_AS_NUMBER(solisGetSelf(vm));

    char buffer[24];
    int length = sprintf(buffer, "%.14g", num);

    Value ret = SOLIS_OBJECT_VALUE(solisCopyString(vm, buffer, length));

    solisSetReturnValue(vm, ret);

    return true;
}

bool num_add(VM* vm)
{
    if (SOLIS_IS_NUMERIC(vm->apiStack[1]))
    {
        vm->apiStack[0] = SOLIS_NUMERIC_VALUE(SOLIS_AS_NUMBER(vm->apiStack[0]) + SOLIS_AS_NUMBER(vm->apiStack[1]));
    }

    return true;
}

bool num_minus(VM* vm)
{
    if (SOLIS_IS_NUMERIC(vm->apiStack[1]))
    {
        vm->apiStack[0] = SOLIS_NUMERIC_VALUE(SOLIS_AS_NUMBER(vm->apiStack[0]) - SOLIS_AS_NUMBER(vm->apiStack[1]));
    }

    return true;
}

bool num_mul(VM* vm)
{

    if (SOLIS_IS_NUMERIC(vm->apiStack[1]))
    {
        vm->apiStack[0] = SOLIS_NUMERIC_VALUE(SOLIS_AS_NUMBER(vm->apiStack[0]) * SOLIS_AS_NUMBER(vm->apiStack[1]));
    }

    return true;
}

bool num_div(VM* vm)
{

    if (SOLIS_IS_NUMERIC(vm->apiStack[1]))
    {
        vm->apiStack[0] = SOLIS_NUMERIC_VALUE(SOLIS_AS_NUMBER(vm->apiStack[0]) / SOLIS_AS_NUMBER(vm->apiStack[1]));
    }

    return true;
}

bool num_dotdot(VM* vm)
{
    Value obj2 = solisGetArgument(vm, 0);

    if (SOLIS_IS_NUMERIC(obj2))
    {
        Value rangeClass = solisGetGlobal(vm, "Range");

        ObjInstance* inst = solisNewInstance(vm, SOLIS_AS_CLASS(rangeClass));

        solisSetInstanceField(vm, SOLIS_OBJECT_VALUE(inst), "min", solisGetSelf(vm));
        solisSetInstanceField(vm, SOLIS_OBJECT_VALUE(inst), "max", obj2);

        solisSetReturnValue(vm, SOLIS_OBJECT_VALUE(inst));
    }

    return true;
}

bool num_pow(VM* vm)
{
    Value obj2 = vm->apiStack[1];

    if (SOLIS_IS_NUMERIC(obj2))
    {
        vm->apiStack[0] = SOLIS_NUMERIC_VALUE(pow(SOLIS_AS_NUMBER(vm->apiStack[0]), SOLIS_AS_NUMBER(obj2)));
    }

    return true;
}

bool num_int_divide(VM* vm)
{
    Value obj2 = vm->apiStack[1];

    if (SOLIS_IS_NUMERIC(obj2))
    {
        vm->apiStack[0] = SOLIS_NUMERIC_VALUE(floor(SOLIS_AS_NUMBER(vm->apiStack[0]) / SOLIS_AS_NUMBER(obj2)));
    }

    return true;
}

bool num_truncate(VM* vm)
{
    vm->apiStack[0] = SOLIS_NUMERIC_VALUE(trunc(SOLIS_AS_NUMBER(vm->apiStack[0])));

    return true;
}

bool string_length(VM* vm)
{
    solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE((double)(SOLIS_AS_STRING(solisGetSelf(vm))->length)));

    return true;
}

bool string_add(VM* vm)
{
    
    ObjString* bstr = SOLIS_AS_STRING(vm->apiStack[0]);
    if (SOLIS_IS_STRING(vm->apiStack[1]))
    {
        ObjString* astr = SOLIS_AS_STRING(vm->apiStack[1]);
        vm->apiStack[0] = SOLIS_OBJECT_VALUE(solisConcatenateStrings(vm, bstr, astr));
    }

    return true;
}

bool list_at(VM* vm)
{
    int idx = (int)SOLIS_AS_NUMBER(solisGetArgument(vm, 0));

    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));

    
    solisSetReturnValue(vm, list->values.data[idx]);

    return true;
}

bool list_length(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));
    solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE((double)list->values.count));

    return true;
}

bool list_append(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));

    solisValueBufferWrite(vm, &list->values, solisGetArgument(vm, 0));

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());

    return true;
}

bool list_insert(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));

    int idx = (int)SOLIS_AS_NUMBER(solisGetArgument(vm, 0));

    int moveCount = list->values.count - idx;

    // Write a temp value into the end of the buffer 
    solisValueBufferWrite(vm, &list->values, SOLIS_NUMERIC_VALUE(0));

    // memmove because our data overlaps
    memmove(&list->values.data[idx + 1], &list->values.data[idx], sizeof(Value) * moveCount);

    list->values.data[idx] = solisGetArgument(vm, 1);

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());

    return true;
}

bool list_removeAt(VM* vm)
{
    return true;
}

bool list_operator_subscriptGet(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));

    Value getter = solisGetArgument(vm, 0);

    if (SOLIS_IS_NUMERIC(getter))
    {
        int idx = (int)SOLIS_AS_NUMBER(getter);

        solisSetReturnValue(vm, list->values.data[idx]);
    }

    return true;
}

bool list_operator_subscriptSet(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));
    int idx = (int)SOLIS_AS_NUMBER(solisGetArgument(vm, 0));
    Value val = solisGetArgument(vm, 1);

    // TODO: Safety checks

    list->values.data[idx] = val;

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());

    return true;
}


bool os_getPlatformString(VM* vm)
{
    ObjString* str = solisCopyString(vm, SOLIS_PLATFORM_STRING, strlen(SOLIS_PLATFORM_STRING));

    solisSetReturnValue(vm, SOLIS_OBJECT_VALUE(str));

    return true;
}

bool ffi_loadLibrary(VM* vm)
{
    Value path = solisGetArgument(vm, 0);

    LibraryHandle handle = solisOpenLibrary(SOLIS_AS_CSTRING(path));

    if (!handle)
    {
        // Failed to load the library
        // TODO: Error check

        return false;
    }


    // We want to grab solis_openlib

    SolisNativeSignature openlib = (SolisNativeSignature)solisGetProcAddress(handle, "solis_openlib");

    if (!openlib)
    {

        return false;
    }

    ObjNative* nativeFunc = solisNewNativeFunction(vm, openlib);

    // return it
    solisSetReturnValue(vm, SOLIS_OBJECT_VALUE(nativeFunc));

    return true;

}

void solisInitialiseCore(VM* vm, bool sandboxed)
{
    // const char* str = read_file_into_cstring("F:/Dev/Solis/Solis/core.solis");

    solisPushGlobalCFunction(vm, "__c_printf", core_printf, 1);


    InterpretResult result = solisInterpret(vm, coreModuleSource, "Core");

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
    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->numberClass), "truncate", num_truncate, 0);

    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_ADD, num_add);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_MINUS, num_minus);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_STAR, num_mul);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_SLASH, num_div);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_POWER, num_pow);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_SLASH_SLASH, num_int_divide);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_DOTDOT, num_dotdot);

    vm->stringClass = SOLIS_AS_CLASS(solisGetGlobal(vm, "String"));

    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->stringClass), "length", string_length, 0);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->stringClass), OPERATOR_ADD, string_add);

    vm->boolClass = SOLIS_AS_CLASS(solisGetGlobal(vm, "Bool"));

    vm->listClass = SOLIS_AS_CLASS(solisGetGlobal(vm, "List"));

    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->listClass), "at", list_at, 1);
    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->listClass), "length", list_length, 0);
    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->listClass), "append", list_append, 1);
    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->listClass), "insert", list_insert, 2);

    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->listClass), OPERATOR_SUBSCRIPT_GET, list_operator_subscriptGet);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->listClass), OPERATOR_SUBSCRIPT_SET, list_operator_subscriptSet);

    // Only load these functions in if we are sandboxing the VM
    if (!sandboxed)
    {

        Value osClass = solisCreateClass(vm, "OS");

        solisAddClassNativeStaticMethod(vm, osClass, "getPlatformString", os_getPlatformString, 0);


        Value ffiClass = solisCreateClass(vm, "FFI");

        solisAddClassNativeStaticMethod(vm, ffiClass, "loadLibrary", ffi_loadLibrary, 1);
    }
}