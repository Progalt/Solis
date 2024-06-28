
#include "solis_chunk.h"

#include <stdio.h>
#include "solis_object.h"




SOLIS_DEFINE_BUFFER(Value, Value);


static int simpleInstruction(const char* name, int offset) {
	printf("%s\n", name);
	return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
	uint8_t constant = chunk->code[offset + 1];
	printf("%-16s %4d '", name, constant);
	solisPrintValue(chunk->constants.data[constant]);
	printf("' -> ");
	solisPrintValueType(chunk->constants.data[constant]);
	printf("\n");

	return offset + 2;
}

static int constantInstructionLong(const char* name, Chunk* chunk, int offset) {
	uint8_t upper = chunk->code[offset + 1];
	uint8_t lower = chunk->code[offset + 2];

	uint16_t constant = upper << 8;
	constant |= lower;

	printf("%-16s %4d '", name, constant);
	solisPrintValue(chunk->constants.data[constant]);
	printf("' -> ");
	solisPrintValueType(chunk->constants.data[constant]);
	printf("\n");

	return offset + 2;
}

static int byteInstruction(const char* name, Chunk* chunk,
	int offset) {
	uint8_t slot = chunk->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
}

static int shortInstruction(const char* name, Chunk* chunk,
	int offset) {
	uint8_t upper = chunk->code[offset + 1];
	uint8_t lower = chunk->code[offset + 2];

	uint16_t slot = upper << 8;
	slot |= lower;
	printf("%-16s %4d\n", name, slot);
	return offset + 3;
}

static int globalInstruction(const char* name, Chunk* chunk, int offset) {
	uint8_t upper = chunk->code[offset + 1];
	uint8_t lower = chunk->code[offset + 2];

	uint16_t constant = upper << 8;
	constant |= lower;

	printf("%-16s %4d ", name, constant);
	printf("\n");
	
	return offset + 2;
}

static int jumpInstruction(const char* name, int sign,
	Chunk* chunk, int offset) {
	uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
	jump |= chunk->code[offset + 2];
	printf("%-16s %4d -> %d\n", name, offset,
		offset + 3 + sign * jump);
	return offset + 3;
}

void solisInitChunk(Chunk* chunk)
{
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	solisValueBufferInit(&chunk->constants);
}

void solisFreeChunk(Chunk* chunk)
{
	solisReallocate(chunk->code, sizeof(uint8_t) * chunk->capacity, 0);
	solisInitChunk(chunk);
	solisValueBufferClear(&chunk->constants);
}


void solisWriteChunk(Chunk* chunk, uint8_t byte)
{
	if(chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		
		chunk->code = (uint8_t*)solisReallocate(chunk->code, sizeof(uint8_t) * oldCapacity, sizeof(uint8_t) * chunk->capacity);
	}

	chunk->code[chunk->count] = byte;
	chunk->count++;
}

int solisAddConstant(Chunk* chunk, Value value)
{
	solisValueBufferWrite(&chunk->constants, value);
	return chunk->constants.count - 1;
}


void solisDisassembleChunk(Chunk* chunk, const char* name)
{
	printf("== %s ==\n", name);

	for (int offset = 0; offset < chunk->count;) {
		offset = solisDisassembleInstruction(chunk, offset);
	}
}

int solisDisassembleInstruction(Chunk* chunk, int offset)
{
	printf("%06d ", offset);

	uint8_t instruction = chunk->code[offset];
	switch (instruction) {
	case OP_CONSTANT:
		return constantInstruction("OP_CONSTANT", chunk, offset);
	case OP_RETURN:
		return simpleInstruction("OP_RETURN", offset);
	case OP_ADD:
		return simpleInstruction("OP_ADD", offset);
	case OP_SUBTRACT:
		return simpleInstruction("OP_SUBTRACT", offset);
	case OP_MULTIPLY:
		return simpleInstruction("OP_MULTIPLY", offset);
	case OP_DIVIDE:
		return simpleInstruction("OP_DIVIDE", offset);
	case OP_POWER:
		return simpleInstruction("OP_POWER", offset);
	case OP_NIL:
		return simpleInstruction("OP_NIL", offset);
	case OP_TRUE:
		return simpleInstruction("OP_TRUE", offset);
	case OP_FALSE:
		return simpleInstruction("OP_FALSE", offset);
	case OP_NOT:
		return simpleInstruction("OP_NOT", offset);
	case OP_EQUAL:
		return simpleInstruction("OP_EQUAL", offset);
	case OP_GREATER:
		return simpleInstruction("OP_GREATER", offset);
	case OP_LESS:
		return simpleInstruction("OP_LESS", offset);
	case OP_POP:
		return simpleInstruction("OP_POP", offset);
	case OP_DEFINE_GLOBAL:
		return constantInstructionLong("OP_DEFINE_GLOBAL", chunk, offset);
	case OP_SET_GLOBAL:
		return globalInstruction("OP_SET_GLOBAL", chunk, offset);
	case OP_GET_GLOBAL:
		return globalInstruction("OP_GET_GLOBAL", chunk, offset);
	case OP_GET_LOCAL:
		return shortInstruction("OP_GET_LOCAL", chunk, offset);
	case OP_SET_LOCAL:
		return shortInstruction("OP_SET_LOCAL", chunk, offset);
	case OP_JUMP:
		return jumpInstruction("OP_JUMP", 1, chunk, offset);
	case OP_JUMP_IF_FALSE:
		return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
	case OP_LOOP:
		return jumpInstruction("OP_LOOP", -1, chunk, offset);
	case OP_CALL:
		return byteInstruction("OP_CALL", chunk, offset);
	case OP_CLOSURE: {
		offset++;
		uint16_t constant = chunk->code[offset++] & 0xFF;
		constant |= chunk->code[offset++] & 0xFF;
		printf("%-16s %4d ", "OP_CLOSURE", constant);
		solisPrintValue(chunk->constants.data[constant]);
		printf("\n");

		ObjFunction* function = SOLIS_AS_FUNCTION(
			chunk->constants.data[constant]);
		for (int j = 0; j < function->upvalueCount; j++) {
			int isLocal = chunk->code[offset++];
			int index = chunk->code[offset++];
			printf("%04d      |                     %s %d\n",
				offset - 2, isLocal ? "local" : "upvalue", index);
		}

		return offset;
	}
	case OP_GET_UPVALUE:
		return byteInstruction("OP_GET_UPVALUE", chunk, offset);
	case OP_SET_UPVALUE:
		return byteInstruction("OP_SET_UPVALUE", chunk, offset);
	case OP_CLOSE_UPVALUE:
		return simpleInstruction("OP_CLOSE_UPVALUE", offset);
	default:
		printf("Unknown opcode %d\n", instruction);
		return offset + 1;
	}
}