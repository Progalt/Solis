
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

struct Chunk
{
	uint8_t* code;
	int count;
	int capacity;

	int* lines;

	ValueBuffer constants;
};

void solisInitChunk(Chunk* chunk);

void solisFreeChunk(Chunk* chunk);

void solisWriteChunk(Chunk* chunk, uint8_t byte);

int solisAddConstant(Chunk* chunk, Value value);


void solisDisassembleChunk(Chunk* chunk, const char* name);
int solisDisassembleInstruction(Chunk* chunk, int offset);

#endif // SOLIS_CHUNK_H