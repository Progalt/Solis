
#include "solis_vm.h"

#include <stdio.h>
#include "solis_compiler.h"

#include <math.h>

#define SOLIS_DEBUG_TRACE_EXECUTION

void solisInitVM(VM* vm)
{
	vm->sp = vm->stack;
	vm->objects = NULL;

	solisInitHashTable(&vm->strings);
	solisInitHashTable(&vm->globalMap);
	solisValueBufferInit(&vm->globals);
}


static void freeObjects(VM* vm)
{
	Object* object = vm->objects;
	while (object != NULL) {
		Object* next = object->next;
		solisFreeObject(vm, object);
		object = next;
	}

}

void solisFreeVM(VM* vm)
{
	solisFreeHashTable(&vm->strings);
	solisFreeHashTable(&vm->globalMap);
	solisValueBufferClear(&vm->globals);
	freeObjects(vm);
	
}


static InterpretResult run(VM* vm)
{
	// store the ip here
	// This helps with pointer indirection
	uint8_t* ip = vm->chunk->code;

	uint8_t instruction = 0;

	// Helper macros to read instructions or constants
#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip+=2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (vm->chunk->constants.data[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm->chunk->constants.data[READ_SHORT()])

#define PUSH(val) (*vm->sp++ = val)
#define POP() (*(--vm->sp))
#define PEEK() (*(vm->sp - 1))
#define PEEK_PTR() (vm->sp - 1)

#ifdef SOLIS_DEBUG_TRACE_EXECUTION
#define STACK_TRACE()														\
do {																		\
	printf("          ");													\
	for (Value* slot = vm->stack; slot < vm->sp; slot++) {					\
		printf("[ ");														\
		solisPrintValue(*slot);												\
		printf(" ]");														\
	}																		\
	printf("\n");															\
	solisDisassembleInstruction(vm->chunk, (int)(ip - vm->chunk->code));	\
} while (false)

#else 
#define STACK_TRACE()
#endif


#ifdef COMPUTED_GOTO

#else 

#define INTERPRET_LOOP							\
	main_loop:									\
		STACK_TRACE();							\
		switch(instruction = READ_BYTE())		\

#define CASE_CODE(name) case OP_##name

#define DISPATCH() goto main_loop

#endif

	INTERPRET_LOOP
	{
	CASE_CODE(CONSTANT) :
		PUSH(READ_CONSTANT());
		DISPATCH();
	CASE_CODE(CONSTANT_LONG) :
		PUSH(READ_CONSTANT_LONG());
		DISPATCH();
	CASE_CODE(NIL) :
		PUSH(SOLIS_NULL_VALUE());
		DISPATCH();
	CASE_CODE(TRUE) :
		PUSH(SOLIS_BOOL_VALUE(true));
		DISPATCH();
	CASE_CODE(FALSE) :
		PUSH(SOLIS_BOOL_VALUE(false));
		DISPATCH();
	CASE_CODE(NEGATE):
	{
		// Just negate the value on the stack
		// No need to pop and push 
		Value* ptr = PEEK_PTR();
		if (SOLIS_IS_NUMERIC(*ptr))
			ptr->as.number = -ptr->as.number;
		else
		{
			return INTERPRET_RUNTIME_ERROR;
		}
		DISPATCH();
	}
	CASE_CODE(NOT):
	{
		Value* ptr = PEEK_PTR();
		*ptr = SOLIS_BOOL_VALUE(solisIsFalsy(*ptr));

		DISPATCH();
	}
	CASE_CODE(EQUAL) :
	{
		Value a = POP();
		Value* val = PEEK_PTR();

		*val = SOLIS_BOOL_VALUE(solisValuesEqual(a, *val));

		DISPATCH();
	}
	CASE_CODE(GREATER) :
	{

		Value a = POP();
		Value* val = PEEK_PTR();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(*val) && SOLIS_IS_NUMERIC(a))
			*val = SOLIS_BOOL_VALUE(val->as.number > a.as.number);

		DISPATCH();
	}
	CASE_CODE(LESS) :
	{

		Value a = POP();
		Value* val = PEEK_PTR();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(*val) && SOLIS_IS_NUMERIC(a))
			*val = SOLIS_BOOL_VALUE(val->as.number < a.as.number);

		DISPATCH();
	}
	CASE_CODE(ADD):
	{
		// Instead of popping the value lower on the stack we can just assign it to the new value avoiding unneeded operations
		Value a = POP();
		Value* val = PEEK_PTR();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(*val) && SOLIS_IS_NUMERIC(a))
			val->as.number = val->as.number + a.as.number;

		if (SOLIS_IS_STRING(*val) && SOLIS_IS_STRING(a))
		{
			ObjString* bstr = SOLIS_AS_STRING(*val);
			ObjString* astr = SOLIS_AS_STRING(a);
			*val = SOLIS_OBJECT_VALUE(solisConcatenateStrings(vm, bstr, astr));
		}

		DISPATCH();
	}
	CASE_CODE(SUBTRACT):
	{
		Value a = POP();
		Value* val = PEEK_PTR();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(*val) && SOLIS_IS_NUMERIC(a))
			val->as.number = val->as.number - a.as.number;

		DISPATCH();
	}
	CASE_CODE(MULTIPLY) :
	{
		Value a = POP();
		Value* val = PEEK_PTR();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(*val) && SOLIS_IS_NUMERIC(a))
			val->as.number = val->as.number * a.as.number;

		DISPATCH();
	}
	CASE_CODE(DIVIDE) :
	{
		Value a = POP();
		Value* val = PEEK_PTR();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(*val) && SOLIS_IS_NUMERIC(a))
			val->as.number = val->as.number / a.as.number;

		DISPATCH();
	}
	CASE_CODE(POWER):
	{
		Value a = POP();
		Value* val = PEEK_PTR();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(*val) && SOLIS_IS_NUMERIC(a))
			val->as.number = pow(val->as.number,  a.as.number);

		DISPATCH();
	}
	CASE_CODE(POP) :
	{
		POP();
		DISPATCH();
	}
	CASE_CODE(DEFINE_GLOBAL) :
	{
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());

		int idx = vm->globals.count;
		solisHashTableInsert(&vm->globalMap, name, SOLIS_NUMERIC_VALUE((double)idx));

		solisValueBufferWrite(&vm->globals, PEEK());
		POP();

		DISPATCH();
	}
	CASE_CODE(SET_GLOBAL) :
	{
		vm->globals.data[READ_SHORT()] = PEEK();
		DISPATCH();
	}
	CASE_CODE(GET_GLOBAL) :
	{
		PUSH(vm->globals.data[READ_SHORT()]);
		DISPATCH();
	}
	CASE_CODE(SET_LOCAL) :
	{
		vm->stack[READ_SHORT()] = PEEK();
		DISPATCH();
	}
	CASE_CODE(GET_LOCAL) :
	{
		PUSH(vm->stack[READ_SHORT()]);
		DISPATCH();
	}
	CASE_CODE(JUMP_IF_FALSE) :
	{
		uint16_t offset = READ_SHORT();
		if (solisIsFalsy(PEEK()))
			ip += offset;

		DISPATCH();
	}
	CASE_CODE(JUMP) :
	{
		uint16_t offset = READ_SHORT();
		ip += offset;

		DISPATCH();
	}
	CASE_CODE(RETURN) :

		Value result = POP();

		return INTERPRET_ALL_GOOD;
	}



	return INTERPRET_ALL_GOOD;

#undef READ_BYTE
#undef READ_SHORT
#undef INTERPRET_LOOP
#undef CASE_CODE
#undef DISPATCH
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
}


InterpretResult solisInterpret(VM* vm, const char* source)
{
	
	Chunk chunk;
	solisInitChunk(&chunk);

	if (!solisCompile(vm, source, &chunk))
	{
		solisFreeChunk(&chunk);

		return INTERPRET_COMPILE_ERROR;
	}


	vm->chunk = &chunk;
	
	InterpretResult result = run(vm);

	solisFreeChunk(&chunk);
	return result;
}


void solisPush(VM* vm, Value value)
{
	*vm->sp = value;
	vm->sp++;
}

Value solisPop(VM* vm)
{
	vm->sp--;
	return *vm->sp;
}