
#ifndef SOLIS_CHUNK_H
#define SOLIS_CHUNK_H

#include "solis_common.h"
#include "solis_value.h"

typedef enum {

#define OPCODE(code) OP_##code,
#include "solis_opcode.h"
#undef OPCODE 

} OpCode;


SOLIS_DECLARE_BUFFER(Value, Value);

SOLIS_DECLARE_BUFFER(Int, int);

struct Chunk
{
	uint8_t* code;
	int count;
	int capacity;

	// We treat each new int as a new line. The number is the last bytecode index that appears on that line. 
	// So technically this should be the same length as the number of lines in the file 
	IntBuffer lines;

	int lastLine;

	ValueBuffer constants;
};

void solisInitChunk(VM* vm, Chunk* chunk);

void solisFreeChunk(VM* vm, Chunk* chunk);

void solisWriteChunk(VM* vm, Chunk* chunk, uint8_t byte, int line);

int solisAddConstant(VM* vm, Chunk* chunk, Value value);


void solisDisassembleChunk(Chunk* chunk, const char* name);
int solisDisassembleInstruction(Chunk* chunk, int offset);

#endif // SOLIS_CHUNK_H