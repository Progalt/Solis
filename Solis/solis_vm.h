
#ifndef SOLIS_VM_H
#define SOLIS_VM_H

#include "solis_chunk.h"
#include "solis_value.h"

#define STACK_MAX 256

typedef enum
{
	INTERPRET_ALL_GOOD, 
	INTERPRET_RUNTIME_ERROR,
	INTERPRET_COMPILE_ERROR
} InterpretResult;

struct VM
{

	Chunk* chunk;

	// TODO: Make this dynamic and not a static array
	Value stack[STACK_MAX];

	// Points to the top of the stack 
	Value* sp;

	Object* objects;

};


/*
	This interprets a source string with the given VM
*/
InterpretResult solisInterpret(VM* vm, const char* source);

/*
	Initialises a VM ready to be used
*/
void solisInitVM(VM* vm);

/*
	Frees all the memory associated with the VM. 
	Must not call any VM related functions on the VM after this. 
*/
void solisFreeVM(VM* vm);



/*
	Pushes a value onto the VM stack
*/
void solisPush(VM* vm, Value value);

/*
	Pops a value from the VM stack and returns it
*/
Value solisPop(VM* vm);


/*
	Returns if a value is false -> its false if its null or a bool and false.
*/
static inline bool solisIsFalsy(Value value) 
{
	return SOLIS_IS_NULL(value) || (SOLIS_IS_BOOL(value) && !SOLIS_AS_BOOL(value));
}

#endif // SOLIS_VM_H