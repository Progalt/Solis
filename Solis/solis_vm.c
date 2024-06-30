
#include "solis_vm.h"

#include <stdio.h>
#include "solis_compiler.h"

#include <string.h>
#include <math.h>

// #define SOLIS_DEBUG_TRACE_EXECUTION

static bool callValue(VM* vm, Value callee, int argCount);

void solisInitVM(VM* vm)
{
	vm->sp = vm->stack;
	vm->objects = NULL;

	vm->openUpvalues = NULL;
	vm->frameCount = 0;

	vm->apiStack = NULL;

	vm->allocatedBytes = 0;
	vm->nextGC = 1024 * 1024;

	vm->greyCapacity = 0;
	vm->greyCount = 0;
	vm->greyStack = NULL;


	solisInitHashTable(&vm->strings, vm);
	solisInitHashTable(&vm->globalMap, vm);
	solisValueBufferInit(vm, &vm->globals);
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
	free(vm->greyStack);

	SOLIS_FREE_ARRAY(vm, CallFrame, vm->frames, vm->frameCapacity);

	solisFreeHashTable(&vm->strings);
	solisFreeHashTable(&vm->globalMap);
	solisValueBufferClear(vm, &vm->globals);
	freeObjects(vm);
	
}

static ObjUpvalue* captureUpvalue(VM* vm, Value* local) 
{
	ObjUpvalue* prevUpvalue = NULL;
	ObjUpvalue* upvalue = vm->openUpvalues;
	while (upvalue != NULL && upvalue->location > local) 
	{
		prevUpvalue = upvalue;
		upvalue = upvalue->next;
	}

	if (upvalue != NULL && upvalue->location == local) 
	{
		return upvalue;
	}

	ObjUpvalue* createdUpvalue = solisNewUpvalue(vm, local);

	createdUpvalue->next = upvalue;

	if (prevUpvalue == NULL) 
	{
		vm->openUpvalues = createdUpvalue;
	}
	else {
		prevUpvalue->next = createdUpvalue;
	}

	return createdUpvalue;
}

static void closeUpvalues(VM* vm, Value* last) 
{
	while (vm->openUpvalues != NULL &&
		vm->openUpvalues->location >= last) {
		ObjUpvalue* upvalue = vm->openUpvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm->openUpvalues = upvalue->next;
	}
}

static InterpretResult run(VM* vm)
{
	CallFrame* frame = &vm->frames[vm->frameCount - 1];


	// store the ip here
	// This helps with pointer indirection
	uint8_t* ip = frame->ip;
	ObjClosure* closure = frame->closure;


#define STORE_FRAME() frame->ip = ip

#define LOAD_FRAME()			\
	STORE_FRAME();				\
	frame = &vm->frames[vm->frameCount - 1]; \
	ip = frame->ip;				\
	closure = frame->closure;	\

	LOAD_FRAME();

	uint8_t instruction = 0;

	// Helper macros to read instructions or constants
#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip+=2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (closure->function->chunk.constants.data[READ_BYTE()])
#define READ_CONSTANT_LONG() (closure->function->chunk.constants.data[READ_SHORT()])

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
	solisDisassembleInstruction(&closure->function->chunk, (int)(ip - closure->function->chunk.code));	\
} while (false)

#else 
#define STACK_TRACE()
#endif


#if SOLIS_COMPUTED_GOTO

	static void* dispatchTable[] = {
#define OPCODE(name) &&code_##name,
#include "solis_opcode.h"
#undef OPCODE
	};

#define INTERPRET_LOOP DISPATCH();
#define CASE_CODE(name) code_##name

#define DISPATCH()                                                           \
      do                                                                       \
      {                                                                        \
        STACK_TRACE();															\
        goto *dispatchTable[instruction = READ_BYTE()];                  \
      } while (false)

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
	CASE_CODE(FLOOR_DIVIDE) :
	{
		Value a = POP();
		Value* val = PEEK_PTR();

		if (SOLIS_IS_NUMERIC(*val) && SOLIS_IS_NUMERIC(a))
			val->as.number = floor(val->as.number / a.as.number);

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

		solisValueBufferWrite(vm, &vm->globals, PEEK());

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
		frame->slots[READ_SHORT()] = PEEK();
		DISPATCH();
	}
	CASE_CODE(GET_LOCAL) :
	{
		PUSH(frame->slots[READ_SHORT()]);
		DISPATCH();
	}
	CASE_CODE(GET_UPVALUE) :
	{
		uint16_t slot = READ_SHORT();

		PUSH(*frame->closure->upvalues[slot]->location);

		DISPATCH();
	}
	CASE_CODE(SET_UPVALUE) :
	{
		uint16_t slot = READ_SHORT();
		*frame->closure->upvalues[slot]->location = PEEK();

		DISPATCH();
	}
	CASE_CODE(CLOSE_UPVALUE) :
	{

		closeUpvalues(vm, vm->sp - 1);
		POP();

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
	CASE_CODE(LOOP) :
	{
		uint16_t offset = READ_SHORT();
		ip -= offset;

		DISPATCH();
	}
	CASE_CODE(CLOSURE) :
	{
		Value obj = READ_CONSTANT_LONG();
		ObjFunction* function = SOLIS_AS_FUNCTION(obj);
		ObjClosure* closure = solisNewClosure(vm, function);

		PUSH(SOLIS_OBJECT_VALUE(closure));

		for (int i = 0; i < closure->upvalueCount; i++)
		{
			uint8_t isLocal = READ_BYTE();
			uint8_t index = READ_BYTE();

			if (isLocal) 
			{
				closure->upvalues[i] =
					captureUpvalue(vm, frame->slots + index);
			}
			else 
			{
				closure->upvalues[i] = frame->closure->upvalues[index];
			}
		}

		DISPATCH();
	}
	CASE_CODE(CALL_0) :
	CASE_CODE(CALL_1) :
	CASE_CODE(CALL_2) :
	CASE_CODE(CALL_3) :
	CASE_CODE(CALL_4) :
	CASE_CODE(CALL_5) :
	CASE_CODE(CALL_6) :
	CASE_CODE(CALL_7) :
	CASE_CODE(CALL_8) :
	CASE_CODE(CALL_9) :
	CASE_CODE(CALL_10) :
	CASE_CODE(CALL_11) :
	CASE_CODE(CALL_12) :
	CASE_CODE(CALL_13) :
	CASE_CODE(CALL_14) :
	CASE_CODE(CALL_15) :
	CASE_CODE(CALL_16) :
	{

		int argCount = instruction - OP_CALL_0;
		
		STORE_FRAME();

		if (!callValue(vm, *(vm->sp - 1 - argCount), argCount))
		{
			printf("Failed to call function\n");
			return INTERPRET_RUNTIME_ERROR;
		}

		LOAD_FRAME();

		DISPATCH();
	}
	CASE_CODE(GET_FIELD) :
	{
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());


		if (SOLIS_IS_OBJECT(PEEK()))
		{
			Object* object = SOLIS_AS_OBJECT(PEEK());
			switch (object->type)
			{
			case OBJ_ENUM:
			{
				ObjEnum* enumObj = (ObjEnum*)object;
				Value val;
				if (!solisHashTableGet(&enumObj->fields, name, &val))
				{
					printf("Failed to get field from enum\n");
					return INTERPRET_RUNTIME_ERROR;
				}

				// Pop the enum value off the stack
				POP();
				PUSH(val);

				break;
			}
			default:
				// Return an error
				// We can't access the fields
				printf("Object does not have fields\n");
				return INTERPRET_RUNTIME_ERROR;
				break;
			}
		}
		

		DISPATCH();
	}
	CASE_CODE(SET_FIELD) : 
	{
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());


		if (SOLIS_IS_OBJECT(PEEK()))
		{
			Object* object = SOLIS_AS_OBJECT(PEEK());
			switch (object->type)
			{
			case OBJ_ENUM:
			{
				// Can't set an enum value
				printf("Can't set enum value\n");
				return INTERPRET_RUNTIME_ERROR;

				break;
			}
			default:
				// Return an error
				// We can't access the fields
				return INTERPRET_RUNTIME_ERROR;
				break;
			}
		}

		DISPATCH();
	}
	CASE_CODE(RETURN) :
	{
		Value result = POP();
		closeUpvalues(vm, frame->slots);
		vm->frameCount--;

		if (vm->frameCount == 0)
		{
			POP();
			return INTERPRET_ALL_GOOD;
		}

		vm->sp = frame->slots;
		PUSH(result);

		LOAD_FRAME();

		DISPATCH();
	}
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

static bool call(VM* vm, ObjFunction* function, int argCount) 
{

	assert(false);
	//if (argCount != function->arity)
	//{
	//	// TODO: Better errors
	//	return false;
	//}

	//if (vm->frameCount == FRAMES_MAX)
	//{
	//	// TODO: Same better errors
	//	return false;
	//}

	//CallFrame* frame = &vm->frames[vm->frameCount++];
	//frame->function = function;
	//frame->ip = function->chunk.code;
	//frame->slots = vm->sp - argCount - 1;
	//return true;
}


static bool callClosure(VM* vm, ObjClosure* closure, int argCount)
{
	if (argCount != closure->function->arity)
	{
		// TODO: Better errors
		return false;
	}

	if (vm->frameCount == FRAMES_MAX)
	{
		// TODO: Same better errors
		return false;
	}

	// Grow our frames
	if (vm->frameCount + 1 >= vm->frameCapacity)
	{
		int oldCapacity = vm->frameCapacity * sizeof(CallFrame);
		vm->frameCapacity = GROW_CAPACITY(vm->frameCapacity);

		vm->frames = (CallFrame*)solisReallocate(vm, vm->frames, oldCapacity, vm->frameCapacity * sizeof(CallFrame));
	}

	CallFrame* frame = &vm->frames[vm->frameCount++];
	frame->closure = closure;
	frame->ip = closure->function->chunk.code;
	frame->slots = vm->sp - argCount - 1;
	return true;
}

static bool callNativeFunction(VM* vm, SolisNativeSignature func, int numArgs)
{
	if (vm->apiStack != NULL)
	{
		return false;
	}

	// we want to add the caller into it as well
	vm->apiStack = vm->sp - (numArgs + 1);

	func(vm);

	vm->sp = vm->apiStack + 1;

	vm->apiStack = NULL;

	return true;
}

static bool callValue(VM* vm, Value callee, int argCount) {
	if (SOLIS_AS_OBJECT(callee)) {
		switch (SOLIS_AS_OBJECT(callee)->type) {
		case OBJ_FUNCTION:
			return call(vm, SOLIS_AS_FUNCTION(callee), argCount);
		case OBJ_CLOSURE:
			return callClosure(vm, SOLIS_AS_CLOSURE(callee), argCount);
		case OBJ_NATIVE_FUNCTION:
		{
			ObjNative* native = SOLIS_AS_NATIVE(callee);

			return callNativeFunction(vm, native->nativeFunction, argCount);
		}
		default:
			break; // Non-callable object type.
		}
	}
	return false;
}

InterpretResult solisInterpret(VM* vm, const char* source)
{
	

	ObjFunction* function = solisCompile(vm, source, &vm->globalMap, vm->globals.count);

	if (function == NULL)
	{
		return INTERPRET_COMPILE_ERROR;
	}

	// solisDisassembleChunk(&function->chunk, "Script");
	
	solisPush(vm, SOLIS_OBJECT_VALUE(function));

	ObjClosure* closure = solisNewClosure(vm, function);
	solisPop(vm);
	solisPush(vm, SOLIS_OBJECT_VALUE(closure));
	callClosure(vm, closure, 0);
	
	InterpretResult result = run(vm);

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

Value solisPeek(VM* vm, int offset)
{
	return (*(vm->sp - 1 - offset));
}

void solisPushGlobal(VM* vm, const char* name, Value value)
{
	solisValueBufferWrite(vm, &vm->globals, value);

	int idx = vm->globals.count - 1;

	ObjString* str = solisCopyString(vm, name, strlen(name));

	// Do this for gc later on 
	solisPush(vm, SOLIS_OBJECT_VALUE(str));

	solisHashTableInsert(&vm->globalMap, SOLIS_AS_STRING(solisPeek(vm, 0)), SOLIS_NUMERIC_VALUE((double)idx));

	solisPop(vm);
}

Value solisGetGlobal(VM* vm, const char* name)
{
	Value val;
	if (solisHashTableGet(&vm->globalMap, solisCopyString(vm, name, strlen(name)), &val))
	{
		// We have a value
		int idx = (int)SOLIS_AS_NUMBER(val);

		return vm->globals.data[idx];
	}

	return SOLIS_NULL_VALUE();
}

bool solisGlobalExists(VM* vm, const char* name)
{
	Value val;
	if (solisHashTableGet(&vm->globalMap, solisCopyString(vm, name, strlen(name)), &val))
	{
		return true;
	}

	return false;
}

void solisPushGlobalCFunction(VM* vm, const char* name, SolisNativeSignature func, int arity)
{
	ObjNative* nativeFunc = solisNewNativeFunction(vm, func);
	nativeFunc->arity = arity;

	solisPush(vm, SOLIS_OBJECT_VALUE(nativeFunc));
	solisPushGlobal(vm, name, solisPeek(vm, 0));
	solisPop(vm);
}