
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
	TYPE_CONSTRUCTOR, 
	TYPE_METHOD, 
	TYPE_SCRIPT, 
} FunctionType;

bool solisCompile(VM* vm, const char* source, ObjModule* mdl, const char* sourceName);

void solisMarkCompilerRoots(VM* vm);

#endif // SOLIS_COMPILER_H