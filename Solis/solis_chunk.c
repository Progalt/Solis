
#include "solis_chunk.h"

#include <stdio.h>
#include "solis_object.h"
#include "solis_vm.h"



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

	return offset + 3;
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
	
	return offset + 3;
}

static int jumpInstruction(const char* name, int sign,
	Chunk* chunk, int offset) {
	uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
	jump |= chunk->code[offset + 2];
	printf("%-16s %4d -> %d\n", name, offset,
		offset + 3 + sign * jump);
	return offset + 3;
}

static int invokeInstruction(const char* name, Chunk* chunk,
	int offset) {
	uint8_t upper = chunk->code[offset + 1];
	uint8_t lower = chunk->code[offset + 2];

	uint16_t constant = upper << 8;
	constant |= lower;
	uint8_t argCount = chunk->code[offset + 3];
	printf("%-16s (%d args) %4d '", name, argCount, constant);
	solisPrintValue(chunk->constants.data[constant]);
	printf("'\n");
	return offset + 4;
}

void solisInitChunk(VM* vm, Chunk* chunk)
{
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	solisValueBufferInit(vm, &chunk->constants);
}

void solisFreeChunk(VM* vm, Chunk* chunk)
{
	solisReallocate(vm, chunk->code, sizeof(uint8_t) * chunk->capacity, 0);
	solisInitChunk(vm, chunk);
	solisValueBufferClear(vm, &chunk->constants);
}


void solisWriteChunk(VM* vm, Chunk* chunk, uint8_t byte)
{
	if(chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		
		chunk->code = (uint8_t*)solisReallocate(vm, chunk->code, sizeof(uint8_t) * oldCapacity, sizeof(uint8_t) * chunk->capacity);
	}

	chunk->code[chunk->count] = byte;
	chunk->count++;
}

int solisAddConstant(VM* vm, Chunk* chunk, Value value)
{
	solisPush(vm, value);
	solisValueBufferWrite(vm, &chunk->constants, value);
	solisPop(vm);
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
	case OP_CALL_0:
		return simpleInstruction("OP_CALL_0", offset);
	case OP_CALL_1:
		return simpleInstruction("OP_CALL_1", offset);
	case OP_CALL_2:
		return simpleInstruction("OP_CALL_2", offset);
	case OP_CALL_3:
		return simpleInstruction("OP_CALL_3", offset);
	case OP_CALL_4:
		return simpleInstruction("OP_CALL_4", offset);
	case OP_CALL_5:
		return simpleInstruction("OP_CALL_5", offset);
	case OP_CALL_6:
		return simpleInstruction("OP_CALL_6", offset);
	case OP_CALL_7:
		return simpleInstruction("OP_CALL_7", offset);
	case OP_CALL_8:
		return simpleInstruction("OP_CALL_8", offset);
	case OP_CALL_9:
		return simpleInstruction("OP_CALL_9", offset);
	case OP_CALL_10:
		return simpleInstruction("OP_CALL_10", offset);
	case OP_CALL_11:
		return simpleInstruction("OP_CALL_11", offset);
	case OP_CALL_12:
		return simpleInstruction("OP_CALL_12", offset);
	case OP_CALL_13:
		return simpleInstruction("OP_CALL_13", offset);
	case OP_CALL_14:
		return simpleInstruction("OP_CALL_14", offset);
	case OP_CALL_15:
		return simpleInstruction("OP_CALL_15", offset);
	case OP_CALL_16:
		return simpleInstruction("OP_CALL_16", offset);
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
	case OP_GET_FIELD:
		return constantInstructionLong("OP_GET_FIELD", chunk, offset);
	case OP_SET_FIELD:
		return constantInstructionLong("OP_SET_FIELD", chunk, offset);
	case OP_CLASS:
		return constantInstructionLong("OP_CLASS", chunk, offset);
	case OP_DEFINE_STATIC:
		return constantInstructionLong("OP_DEFINE_STATIC", chunk, offset);
	case OP_DEFINE_FIELD:
		return constantInstructionLong("OP_DEFINE_FIELD", chunk, offset);
	case OP_DEFINE_METHOD:
		return constantInstructionLong("OP_DEFINE_METHOD", chunk, offset);
	case OP_INHERIT:
		return simpleInstruction("OP_INHERIT", offset);
	case OP_INVOKE:
		return invokeInstruction("OP_INVOKE", chunk, offset);
	default:
		printf("Unknown opcode %d\n", instruction);
		return offset + 1;
	}
}