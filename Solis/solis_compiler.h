
#ifndef SOLIS_COMPILER_H
#define SOLIS_COMPILER_H

#include <stdbool.h>
#include "solis_common.h"
#include "solis_chunk.h"


typedef struct sCompiler Compiler;

typedef enum
{
	TYPE_FUNCTION,
	TYPE_SCRIPT, 
} FunctionType;

ObjFunction* solisCompile(VM* vm, const char* source);

#endif // SOLIS_COMPILER_H