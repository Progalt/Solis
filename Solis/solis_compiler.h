
#ifndef SOLIS_COMPILER_H
#define SOLIS_COMPILER_H

#include <stdbool.h>
#include "solis_common.h"
#include "solis_chunk.h"


typedef struct sCompiler Compiler;

bool solisCompile(const char* source, Chunk* chunk);

#endif // SOLIS_COMPILER_H