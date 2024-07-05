
#ifndef SOLIS_VM_H
#define SOLIS_VM_H

#include "solis_chunk.h"
#include "solis_value.h"
#include "solis_hashtable.h"

#include "solis_object.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef enum
{
	INTERPRET_ALL_GOOD, 
	INTERPRET_RUNTIME_ERROR,
	INTERPRET_COMPILE_ERROR
} InterpretResult;

typedef struct {

	//ObjFunction* function;
	ObjClosure* closure;

	uint8_t* ip;
	Value* slots;
} CallFrame;

struct VM
{

	CallFrame* frames;
	int frameCount;
	int frameCapacity;

	// TODO: Make this dynamic and not a static array
	Value stack[STACK_MAX];

	// Points to the top of the stack 
	Value* sp;

	Object* objects;

	// List of interned strings for the VM
	HashTable strings;

	// So we can map names to global indexes
	HashTable globalMap;

	ValueBuffer globals;

	ObjUpvalue* openUpvalues;

	// This method of function calling is stolen from wren... :) 
	Value* apiStack;

	uint64_t allocatedBytes;
	uint64_t nextGC;


	int greyCount;
	int greyCapacity;
	Object** greyStack;

	ObjClass* numberClass;
	ObjClass* stringClass;
	ObjClass* boolClass;
	ObjClass* listClass;

	ObjString* operatorStrings[OPERATOR_COUNT];
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
	Peek a value on the stack 
*/
Value solisPeek(VM* vm, int offset);

/*
	Adds a global to the VM
*/
void solisPushGlobal(VM* vm, const char* name, Value value);

/*
	Returns global as it is currently within the VM 
*/
Value solisGetGlobal(VM* vm, const char* name);

/*
	Checks if a global of name exists 
*/
bool solisGlobalExists(VM* vm, const char* name);

/*
	Pushes a global C function to the global list
*/
void solisPushGlobalCFunction(VM* vm, const char* name, SolisNativeSignature func, int arity);

/*
	Returns if a value is false -> its false if its null or a bool and false.
*/
static inline bool solisIsFalsy(Value value) 
{
	return SOLIS_IS_NULL(value) || (SOLIS_IS_BOOL(value) && !SOLIS_AS_BOOL(value));
}


void solisDumpGlobals(VM* vm);

static inline ObjClass* solisGetClassForValue(VM* vm, Value value)
{
	if (SOLIS_IS_NUMERIC(value))
		return vm->numberClass;

	if (SOLIS_IS_CLASS(value))
		return SOLIS_AS_CLASS(value);


	switch (value.type)
	{
	case VALUE_FALSE: return vm->boolClass;
	case VALUE_TRUE: return vm->boolClass;
	case VALUE_NULL: return NULL;
	case VALUE_OBJECT: return SOLIS_AS_OBJECT(value)->classObj;
	default:
		break;
	}

	return NULL;
}

#endif // SOLIS_VM_H