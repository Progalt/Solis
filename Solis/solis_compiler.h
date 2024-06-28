
#ifndef SOLIS_COMPILER_H
#define SOLIS_COMPILER_H

#include <stdbool.h>
#include "solis_common.h"
#include "solis_chunk.h"
#include "solis_hashtable.h"


typedef struct sCompiler Compiler;

typedef enum
{
	TYPE_FUNCTION,
	TYPE_SCRIPT, 
} FunctionType;

ObjFunction* solisCompile(VM* vm, const char* source, HashTable* globals, int globalCount);

#endif // SOLIS_COMPILER_H