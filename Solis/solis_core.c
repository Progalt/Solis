
#include "solis_core.h"

#include <float.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void num_add(VM* vm)
{
    Value obj2 = vm->apiStack[1];

    if (SOLIS_IS_NUMERIC(obj2))
    {
        //solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE(num1 + SOLIS_AS_NUMBER(obj2)));
        vm->apiStack[0] = SOLIS_NUMERIC_VALUE(SOLIS_AS_NUMBER(vm->apiStack[0]) + SOLIS_AS_NUMBER(obj2));
    }
}

void num_minus(VM* vm)
{
    Value obj2 = vm->apiStack[1];

    if (SOLIS_IS_NUMERIC(obj2))
    {
        // solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE(num1 - SOLIS_AS_NUMBER(obj2)));
        vm->apiStack[0] = SOLIS_NUMERIC_VALUE(SOLIS_AS_NUMBER(vm->apiStack[0]) - SOLIS_AS_NUMBER(obj2));
    }
}

void num_mul(VM* vm)
{
    Value obj2 = solisGetArgument(vm, 0);

    if (SOLIS_IS_NUMERIC(obj2))
    {
        solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE(SOLIS_AS_NUMBER(vm->apiStack[0]) * SOLIS_AS_NUMBER(obj2)));
        return;
    }
}

void num_div(VM* vm)
{
    Value obj2 = solisGetArgument(vm, 0);

    if (SOLIS_IS_NUMERIC(obj2))
    {
        solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE(SOLIS_AS_NUMBER(vm->apiStack[0]) / SOLIS_AS_NUMBER(obj2)));
        return;
    }
}

void num_dotdot(VM* vm)
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
}

void string_length(VM* vm)
{
    solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE((double)(SOLIS_AS_STRING(solisGetSelf(vm))->length)));
}

void string_add(VM* vm)
{
    
    ObjString* bstr = SOLIS_AS_STRING(vm->apiStack[0]);
    if (SOLIS_IS_STRING(vm->apiStack[1]))
    {
        ObjString* astr = SOLIS_AS_STRING(vm->apiStack[1]);
        vm->apiStack[0] = SOLIS_OBJECT_VALUE(solisConcatenateStrings(vm, bstr, astr));
    }


}

void list_at(VM* vm)
{
    int idx = (int)SOLIS_AS_NUMBER(solisGetArgument(vm, 0));

    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));

    
    solisSetReturnValue(vm, list->values.data[idx]);
}

void list_length(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));
    solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE((double)list->values.count));
}

void list_append(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));

    solisValueBufferWrite(vm, &list->values, solisGetArgument(vm, 0));

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());
}

void list_insert(VM* vm)
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
}

void list_removeAt(VM* vm)
{

}

void list_operator_subscriptGet(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));
    int idx = (int)SOLIS_AS_NUMBER(solisGetArgument(vm, 0));

    // TODO: Safety checks

    solisSetReturnValue(vm, list->values.data[idx]);
}

void list_operator_subscriptSet(VM* vm)
{
    ObjList* list = SOLIS_AS_LIST(solisGetSelf(vm));
    int idx = (int)SOLIS_AS_NUMBER(solisGetArgument(vm, 0));
    Value val = solisGetArgument(vm, 1);

    // TODO: Safety checks

    list->values.data[idx] = val;

    solisSetReturnValue(vm, SOLIS_NULL_VALUE());
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

    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_ADD, num_add);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_MINUS, num_minus);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_STAR, num_mul);
    solisAddClassNativeOperator(vm, SOLIS_OBJECT_VALUE(vm->numberClass), OPERATOR_SLASH, num_div);
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

}