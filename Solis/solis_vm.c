
#include "solis_vm.h"

#include <stdio.h>
#include "solis_compiler.h"

#include <string.h>
#include <math.h>

#include "solis_core.h"

#include "terminal.h"
#include <stdarg.h>

// #define SOLIS_DEBUG_TRACE_EXECUTION

static bool callValue(VM* vm, Value callee, int argCount);
static bool callClosure(VM* vm, ObjClosure* closure, int argCount);
static bool callNativeFunction(VM* vm, SolisNativeSignature func, int numArgs);
static bool callOperator(VM* vm, int op, int numArgs);


static int __openVMs = 0;



void solisInitVM(VM* vm, bool sandboxed)
{

	if (__openVMs == 0)
		terminalInit();

	__openVMs++;

	vm->sandboxed = sandboxed;

	vm->sp = vm->stack;
	vm->objects = NULL;

	vm->moduleName = NULL;
	vm->source = NULL;

	vm->openUpvalues = NULL;
	vm->frameCount = 0;
	vm->frames = NULL;
	vm->frameCapacity = 0;

	vm->apiStack = NULL;

	vm->allocatedBytes = 0;
	vm->nextGC = 1024 * 1024;

	vm->greyCapacity = 0;
	vm->greyCount = 0;
	vm->greyStack = NULL;
	vm->errorRaised = false;

	vm->boolClass = NULL;
	vm->stringClass = NULL;
	vm->numberClass = NULL;
	vm->listClass = NULL;


	solisInitHashTable(&vm->strings, vm);
	/*solisInitHashTable(&vm->globalMap, vm);
	solisValueBufferInit(vm, &vm->globals);*/

	vm->currentModule = solisNewModule(vm);

	// Initialise the operator strings 
	vm->operatorStrings[OPERATOR_ADD] = solisCopyString(vm, "+", 1);
	vm->operatorStrings[OPERATOR_MINUS] = solisCopyString(vm, "-", 1);
	vm->operatorStrings[OPERATOR_STAR] = solisCopyString(vm, "*", 1);
	vm->operatorStrings[OPERATOR_SLASH] = solisCopyString(vm, "/", 1);
	vm->operatorStrings[OPERATOR_SLASH_SLASH] = solisCopyString(vm, "//", 2);
	vm->operatorStrings[OPERATOR_POWER] = solisCopyString(vm, "**", 2);
	vm->operatorStrings[OPERATOR_SUBSCRIPT_GET] = solisCopyString(vm, "[]", 2);
	vm->operatorStrings[OPERATOR_DOTDOT] = solisCopyString(vm, "..", 2);
	vm->operatorStrings[OPERATOR_SUBSCRIPT_SET] = solisCopyString(vm, "[]=", 3);



	solisInitialiseCore(vm, sandboxed);

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
	/*solisFreeHashTable(&vm->globalMap);
	solisValueBufferClear(vm, &vm->globals);*/
	freeObjects(vm);

	__openVMs--;

	// Shut the terminal down
	if (__openVMs == 0)
		terminalShutdown();
	
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
	while (vm->openUpvalues != NULL && vm->openUpvalues->location >= last) 
	{
		ObjUpvalue* upvalue = vm->openUpvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm->openUpvalues = upvalue->next;
	}
}

static bool invokeFromClass(VM* vm, ObjClass* klass, ObjString* name, int argCount, bool isStatic) {
	Value method;

	if (isStatic)
	{
		if (!solisHashTableGet(&klass->statics, name, &method)) 
		{
			return false;
		}
	}
	else
	{
		if (!solisHashTableGet(&klass->methods, name, &method)) 
		{
			return false;
		}
	}

	if (SOLIS_IS_CLOSURE(method))
		return callClosure(vm, SOLIS_AS_CLOSURE(method), argCount);
	else
	{
		if (argCount != SOLIS_AS_NATIVE(method)->arity)
			return false;

		return callNativeFunction(vm, SOLIS_AS_NATIVE(method)->nativeFunction, argCount);
	}
}

static bool invoke(VM* vm, ObjString* name, int argCount) 
{
	Value receiver = solisPeek(vm, argCount);

	ObjClass* klass = NULL;
	if (!SOLIS_IS_INSTANCE(receiver))
		klass = solisGetClassForValue(vm, receiver);

	bool isStatic = SOLIS_IS_CLASS(receiver);

	if (klass == NULL)
		klass = SOLIS_AS_INSTANCE(receiver)->klass;
	
		
	return invokeFromClass(vm, klass, name, argCount, isStatic);
	
}

static InterpretResult run(VM* vm)
{
	CallFrame* frame = &vm->frames[vm->frameCount - 1];


	// store the ip here
	// This helps with pointer indirection
	uint8_t* ip = frame->ip;
	ObjClosure* closure = frame->closure;

	Value* globals = vm->currentModule->globals.data;


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
#define PEEK_OFF(offset) (*(vm->sp - 1 - offset))
#define PEEK_PTR() (vm->sp - 1)
#define DROP() (--vm->sp)

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

#define DISPATCH()                                                          \
      do                                                                    \
      {																		\
        STACK_TRACE();														\
        goto *dispatchTable[instruction = READ_BYTE()];						\
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
	CASE_CODE(NEGATE) :
	{
		// Just negate the value on the stack
		// No need to pop and push 
		// TODO: Improve this
		if (SOLIS_IS_NUMERIC(PEEK()))
			PUSH(SOLIS_NUMERIC_VALUE(-SOLIS_AS_NUMBER(POP())));
		else
		{
			solisVMRaiseError( vm, "Negate error\n");
			return INTERPRET_RUNTIME_ERROR;
		}
		DISPATCH();
	}
	CASE_CODE(NOT) :
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
		Value val = POP();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(val) && SOLIS_IS_NUMERIC(a))
			PUSH(SOLIS_BOOL_VALUE(SOLIS_AS_NUMBER(val) > SOLIS_AS_NUMBER(a)));

		DISPATCH();
	}
	CASE_CODE(LESS) :
	{

		Value a = POP();
		Value val = POP();

		// Check if the values are numeric values
		if (SOLIS_IS_NUMERIC(val) && SOLIS_IS_NUMERIC(a))
			PUSH(SOLIS_BOOL_VALUE(SOLIS_AS_NUMBER(val) < SOLIS_AS_NUMBER(a)));

		DISPATCH();
	}
	{
		// These need to 8 bit for speed
		uint8_t op = 0;
		uint8_t argCount = 0;

	CASE_CODE(SUBSCRIPT_SET) :

		op = OPERATOR_SUBSCRIPT_SET;
		argCount = 2;

		goto completeOpCall;

	CASE_CODE(ADD) :
	CASE_CODE(SUBTRACT) :
	CASE_CODE(MULTIPLY) :
	CASE_CODE(DIVIDE) :
	CASE_CODE(FLOOR_DIVIDE) :
	CASE_CODE(POWER) :
	CASE_CODE(SUBSCRIPT_GET) :
	CASE_CODE(DOTDOT):
		

		op = instruction - OP_ADD;
		argCount = 1;

	completeOpCall:

		Value val = PEEK_OFF(argCount);

		ObjClass* klass = solisGetClassForValue(vm, val);

		Object* obj = klass->operators[op];

		if (obj == NULL)
		{
			solisVMRaiseError(vm, "Object does not contain operator: %s\n", vm->operatorStrings[op]->chars);
			return INTERPRET_RUNTIME_ERROR;
		}

		if (obj->type == OBJ_NATIVE_FUNCTION)
		{
			if (!callNativeFunction(vm, ((ObjNative*)obj)->nativeFunction, argCount))
			{
				return INTERPRET_RUNTIME_ERROR;
			}
		}
		else
		{
			if (!callClosure(vm, ((ObjClosure*)obj), argCount))
			{
				return INTERPRET_RUNTIME_ERROR;
			}
		}

		DISPATCH();
	}

	CASE_CODE(POP) :
	{
		DROP();
		DISPATCH();
	}
	CASE_CODE(CREATE_LIST) :
	{
		ObjList* list = solisNewList(vm);
		PUSH(SOLIS_OBJECT_VALUE(list));
		DISPATCH();
	}
	CASE_CODE(APPEND_LIST) :
	{
		// Peek the value on the stack and append it to the list. 
		// This op is only used during list creation. 
		Value list = PEEK_OFF(1);
		Value val = PEEK();

		ObjList* l = SOLIS_AS_LIST(list);
		solisValueBufferWrite(vm, &l->values, val);

		DROP();

		DISPATCH();
	}
	CASE_CODE(DEFINE_GLOBAL) :
	{
		DISPATCH();
	}
	CASE_CODE(SET_GLOBAL) :
	{
		globals[READ_SHORT()] = PEEK();
		DISPATCH();
	}
	CASE_CODE(GET_GLOBAL) :
	{
		PUSH(globals[READ_SHORT()]);
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
		PUSH(*frame->closure->upvalues[READ_SHORT()]->location);
		DISPATCH();
	}
	CASE_CODE(SET_UPVALUE) :
	{
		*frame->closure->upvalues[READ_SHORT()]->location = PEEK();
		DISPATCH();
	}
	CASE_CODE(CLOSE_UPVALUE) :
	{

		closeUpvalues(vm, vm->sp - 1);
		DROP();

		DISPATCH();
	}
	CASE_CODE(JUMP_IF_FALSE) :
	{
		{
			uint16_t offset = READ_SHORT();
			if (solisIsFalsy(PEEK()))
				ip += offset;
		}

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
				closure->upvalues[i] = captureUpvalue(vm, frame->slots + index);
			}
			else
			{
				closure->upvalues[i] = frame->closure->upvalues[index];
			}
		}

		LOAD_FRAME();

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
			solisVMRaiseError(vm, "Failed to call function\n");
			return INTERPRET_RUNTIME_ERROR;
		}

		LOAD_FRAME();

		DISPATCH();
	}
	CASE_CODE(IS) :
	{
		
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());

		Value* obj = PEEK_PTR();

		ObjClass* classObj = solisGetClassForValue(vm, *obj);

		if (strcmp(name->chars, classObj->name->chars) == 0)
		{
			 *obj = SOLIS_BOOL_VALUE(true);
		}
		else
		{
			*obj = SOLIS_BOOL_VALUE(false);
		}

		DISPATCH();
	}
	CASE_CODE(CLASS) :
	{
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());

		PUSH(SOLIS_OBJECT_VALUE(solisNewClass(vm, name)));

		DISPATCH();
	}
	CASE_CODE(INHERIT) :
	{

		Value superclass = solisPeek(vm, 1);

		ObjClass* subclass = SOLIS_AS_CLASS(PEEK());

		solisHashTableCopy(&SOLIS_AS_CLASS(superclass)->methods, &subclass->methods);
		solisHashTableCopy(&SOLIS_AS_CLASS(superclass)->fields, &subclass->fields);

		DROP();
		DISPATCH();
	}
	CASE_CODE(DEFINE_FIELD) :
	{
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());

		Value val = PEEK();
		ObjClass* klass = SOLIS_AS_CLASS(solisPeek(vm, 1));

		solisHashTableInsert(&klass->fields, name, val);

		DROP();
		DISPATCH();
	}
	CASE_CODE(DEFINE_STATIC) :
	CASE_CODE(DEFINE_METHOD) :
	{
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());

		Value val = PEEK();
		ObjClass* klass = SOLIS_AS_CLASS(solisPeek(vm, 1));

		solisHashTableInsert(instruction == OP_DEFINE_METHOD ? &klass->methods : &klass->statics, name, val);

		DROP();

		DISPATCH();
	}
	CASE_CODE(DEFINE_CONSTRUCTOR) :
	{

		// ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());
		READ_SHORT();

		Value val = PEEK();
		ObjClass* klass = SOLIS_AS_CLASS(solisPeek(vm, 1));

		klass->constructor = SOLIS_AS_CLOSURE(val);

		DROP();

		DISPATCH();
	}
	CASE_CODE(GET_FIELD) :
	{
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());

		// TODO: This could be improved
		// Maybe an invoke? 

		ObjClass* objectClass = solisGetClassForValue(vm, PEEK());

		if (objectClass && !SOLIS_IS_INSTANCE(PEEK()))
		{
			Value value;
			if (solisHashTableGet(&objectClass->statics, name, &value))
			{
				DROP();
				PUSH(value);

				DISPATCH();
			}

			if (solisHashTableGet(&objectClass->methods, name, &value))
			{
				ObjBoundMethod* bound = NULL;

				if (SOLIS_IS_CLOSURE(value))
					bound = solisNewBoundMethod(vm, PEEK(), SOLIS_AS_CLOSURE(value));
				else
					bound = solisNewNativeBoundMethod(vm, PEEK(), SOLIS_AS_NATIVE(value));


				DROP();
				PUSH(SOLIS_OBJECT_VALUE(bound));

				DISPATCH();

			}

			solisVMRaiseError(vm, "Can't get field from class: '%s'\n", name);
			return INTERPRET_RUNTIME_ERROR;
			
		}
		else 
		{
			// TODO: Maybe remake Enum as a class With static fields 
			Object* object = SOLIS_AS_OBJECT(PEEK());
			switch (object->type)
			{
			case OBJ_ENUM:
			{
				ObjEnum* enumObj = (ObjEnum*)object;
				Value val;
				if (!solisHashTableGet(&enumObj->fields, name, &val))
				{
					solisVMRaiseError( vm, "Failed to get field from enum\n");
					return INTERPRET_RUNTIME_ERROR;
				}

				// Pop the enum value off the stack
				DROP();
				PUSH(val);

				break;
			}
			case OBJ_INSTANCE:
			{
				ObjInstance* instance = (ObjInstance*)object;

				Value value;
				if (solisHashTableGet(&instance->fields, name, &value))
				{
					DROP();
					PUSH(value);
				}
				else if (solisHashTableGet(&instance->klass->methods, name, &value))
				{
					ObjBoundMethod* bound = NULL;
					
					if (SOLIS_IS_CLOSURE(value))
						bound = solisNewBoundMethod(vm, SOLIS_OBJECT_VALUE(instance), SOLIS_AS_CLOSURE(value));
					else 
						bound = solisNewNativeBoundMethod(vm, SOLIS_OBJECT_VALUE(instance), SOLIS_AS_NATIVE(value));

					DROP();
					PUSH(SOLIS_OBJECT_VALUE(bound));
				}
				else if (solisHashTableGet(&instance->klass->statics, name, &value))
				{
					DROP();
					PUSH(value);
				}
				else
				{
					solisVMRaiseError(vm, "Can't get field from instance.\n");
					return INTERPRET_RUNTIME_ERROR;
				}

				break;
			}
			default:
				// Return an error
				// We can't access the fields
				solisVMRaiseError(vm, "Object does not have fields\n");
				return INTERPRET_RUNTIME_ERROR;
				break;
			}
		}



		DISPATCH();
	}
	CASE_CODE(SET_FIELD) : 
	{
		ObjString* name = SOLIS_AS_STRING(READ_CONSTANT_LONG());


		if (SOLIS_IS_OBJECT(solisPeek(vm, 1)))
		{
			Object* object = SOLIS_AS_OBJECT(solisPeek(vm, 1));
			switch (object->type)
			{
			case OBJ_INSTANCE:
			{
				ObjInstance* instance = (ObjInstance*)object;

				// This is a really bad way of doing it... lol 
				int error = 0;
				if (solisHashTableInsert(&instance->fields, name, PEEK()))
				{
					// Delete it from the hash table
					solisHashTableDelete(&instance->fields, name);
					error++;
				}

				if (solisHashTableInsert(&instance->klass->statics, name, PEEK()))
				{
					solisHashTableDelete(&instance->klass->statics, name);
					error++;
				}
				

				if (error == 2)
				{
					solisVMRaiseError(vm, "Cannot set field that does not exist in class\n");
					return INTERPRET_RUNTIME_ERROR;
				}

				Value value = POP();
				DROP();
				PUSH(value);

				break;
			}
			case OBJ_CLASS:
			{
				ObjClass* klass = (ObjClass*)object;

				if (solisHashTableInsert(&klass->statics, name, PEEK()))
				{
					// Delete it from the hash table
					solisHashTableDelete(&klass->statics, name);

					solisVMRaiseError(vm, "Can't set a static field that doesn't exist in class\n");
					return INTERPRET_RUNTIME_ERROR;
				}

				Value value = POP();
				DROP();
				PUSH(value);

				break;
			}
			default:
				// Return an error
				// We can't access the fields
				solisVMRaiseError(vm, "Can't get field\n");
				return INTERPRET_RUNTIME_ERROR;
				break;
			}
		}

		DISPATCH();
	}
	CASE_CODE(INVOKE) :
	{
		STORE_FRAME();

		ObjString* method = SOLIS_AS_STRING(READ_CONSTANT_LONG());
		int argCount = READ_BYTE();

		if (!invoke(vm, method, argCount))
		{
			solisVMRaiseError(vm, "Can't invoke method '%s'\n", method->chars);
			return INTERPRET_RUNTIME_ERROR;
		}

		LOAD_FRAME();
		DISPATCH();
	}
	CASE_CODE(RETURN) :
	{
		Value result = POP();
		closeUpvalues(vm, frame->slots);
		vm->frameCount--;

		if (vm->frameCount == 0)
		{
			DROP();
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


static inline bool callClosure(VM* vm, ObjClosure* closure, int argCount)
{
	if (argCount != closure->function->arity)
	{
		// TODO: Better errors
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

static inline bool callNativeFunction(VM* vm, SolisNativeSignature func, int numArgs)
{

	if (vm->apiStack != NULL)
	{
		return false;
	}

	// we want to add the caller into it as well
	vm->apiStack = vm->sp - (numArgs + 1);

	bool success = func(vm);

	vm->sp = vm->apiStack + 1;

	vm->apiStack = NULL;

	return success;
}

static bool callValue(VM* vm, Value callee, int argCount) 
{
	if (!SOLIS_IS_OBJECT(callee))
		return false;

	Object* obj = SOLIS_AS_OBJECT(callee);

	switch (obj->type) {
	case OBJ_CLOSURE:
		return callClosure(vm, (ObjClosure*)obj, argCount);
	case OBJ_NATIVE_FUNCTION:
		return callNativeFunction(vm, ((ObjNative*)obj)->nativeFunction, argCount);
	case OBJ_CLASS:
	{
		ObjClass* klass = (ObjClass*)obj;
		vm->sp[-argCount - 1] = SOLIS_OBJECT_VALUE(solisNewInstance(vm, klass));

		// Call the constructor if we have one 
		if (klass->constructor)
			return callClosure(vm, klass->constructor, argCount);

		return true;
	}
	case OBJ_BOUND_METHOD:
	{
		ObjBoundMethod* bound = (ObjBoundMethod*)obj;
		vm->sp[-argCount - 1] = bound->receiver;
		if (bound->nativeFunction)
		{
			return callNativeFunction(vm, bound->native->nativeFunction, argCount);
		}
		else
		{
			return callClosure(vm, bound->method, argCount);
		}
	}
	default:
		break; // Non-callable object type.
	}
	
	return false;
}

static bool callOperator(VM* vm, int op, int numArgs)
{
	ObjClass* klass = solisGetClassForValue(vm, solisPeek(vm, numArgs));

	Value val;
	if (!solisHashTableGet(&klass->methods, vm->operatorStrings[op], &val))
	{
		return false;
	}

	if (SOLIS_IS_NATIVE(val))
	{
		callNativeFunction(vm, SOLIS_AS_NATIVE(val)->nativeFunction, numArgs);
	}
	else
	{
		callClosure(vm, SOLIS_AS_CLOSURE(val), numArgs);
	}

	return true;
}

InterpretResult solisInterpret(VM* vm, const char* source, const char* sourceName)
{
	//solisFreeChunk(vm->currentModule->)

	bool ret = solisCompile(vm, source, vm->currentModule, sourceName);

	if (ret == false)
	{
		return INTERPRET_COMPILE_ERROR;
	}

	// solisDisassembleChunk(&function->chunk, "Script");
	
	vm->moduleName = sourceName;
	vm->source = source;

	ObjClosure* closure = vm->currentModule->closure;

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

InterpretResult solisCallFunction(VM* vm, Value function, Value* args, int argCount)
{

	if (!SOLIS_IS_CLOSURE(function))
		return INTERPRET_COMPILE_ERROR;

	ObjClosure* closure = SOLIS_AS_CLOSURE(function);

	for (int i = 0; i < argCount; i++)
		solisPush(vm, args[i]);

	callClosure(vm, closure, argCount);

	// TODO: Check if its running

	return run(vm);

}

InterpretResult solisCallInstanceMethod(VM* vm, Value instance, const char* methodName, Value* args, int argCount)
{
	if (!SOLIS_IS_INSTANCE(instance))
		return INTERPRET_COMPILE_ERROR;

	ObjString* str = solisCopyString(vm, methodName, strlen(methodName));
	solisPush(vm, SOLIS_OBJECT_VALUE(str));

	ObjInstance* inst = SOLIS_AS_INSTANCE(instance);

	Value method;
	if (!solisHashTableGet(&inst->klass->methods, str, &method))
	{
		solisPop(vm);
		return INTERPRET_COMPILE_ERROR;
	}

	solisPush(vm, instance);

	return solisCallFunction(vm, method, args, argCount);

}

void solisPushGlobal(VM* vm, const char* name, Value value)
{

	ObjString* str = solisCopyString(vm, name, strlen(name));

	// Do this for gc later on 
	solisPush(vm, SOLIS_OBJECT_VALUE(str));

	// We want to check if the value already exists and overwrite it at that position
	Value val;
	if (solisHashTableGet(&vm->currentModule->globalMap, str, &val))
	{
		// We have the value already

		int idx = (int)SOLIS_AS_NUMBER(val);

		vm->currentModule->globals.data[idx] = value;

	}
	else
	{

		solisValueBufferWrite(vm, &vm->currentModule->globals, value);

		int idx = vm->currentModule->globals.count - 1;

		solisHashTableInsert(&vm->currentModule->globalMap, SOLIS_AS_STRING(solisPeek(vm, 0)), SOLIS_NUMERIC_VALUE((double)idx));
	}

	solisPop(vm);
}

Value solisGetGlobal(VM* vm, const char* name)
{
	Value val;
	if (solisHashTableGet(&vm->currentModule->globalMap, solisCopyString(vm, name, strlen(name)), &val))
	{
		// We have a value
		int idx = (int)SOLIS_AS_NUMBER(val);

		return vm->currentModule->globals.data[idx];
	}

	return SOLIS_NULL_VALUE();
}

bool solisGlobalExists(VM* vm, const char* name)
{
	Value val;
	if (solisHashTableGet(&vm->currentModule->globalMap, solisCopyString(vm, name, strlen(name)), &val))
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

void solisDumpGlobals(VM* vm)
{
	for (int i = 0; i < vm->currentModule->globals.count; i++)
	{
		printf("Global %d: ", i);
		solisPrintValue(vm->currentModule->globals.data[i]);
		printf("\n");
		
	}
}

// TODO: Duplicate functions
// Probably should remove these 

int findLineIndicesVm(const char* str, int lineNumber, int* startIndex, int* endIndex) {
	int currentLine = 0;
	int i = 0;

	*startIndex = -1;
	*endIndex = -1;

	while (str[i] != '\0') {
		if (currentLine == lineNumber) {
			if (*startIndex == -1) {
				*startIndex = i;
			}
			if (str[i] == '\n' || str[i + 1] == '\0') {
				*endIndex = (str[i] == '\n') ? i : i + 1;
				break;
			}
		}

		if (str[i] == '\n') {
			currentLine++;
		}

		i++;
	}

	if (*startIndex == -1 || *endIndex == -1) {
		return -1;
	}

	return 0;
}


static void printSourceLineVm( const char* source, int line)
{
	int start = 0, end = 0;
	if (findLineIndicesVm(source, line - 1, &start, &end) == -1)
	{
		terminalPrintf("\n");
		return;
	}

	terminalPrintf( "%.*s\n", end - start, source + start);


}

void solisVMRaiseError(VM* vm, const char* message, ...)
{
	CallFrame* currentFrame = &vm->frames[vm->frameCount - 1];

	vm->currentInstruction = (int)(currentFrame->ip - currentFrame->closure->function->chunk.code);

	int instOffset = vm->currentInstruction;
	Chunk* chunk = &currentFrame->closure->function->chunk;

	// Get the line
	int line = currentFrame->closure->function->chunk.lines.data[instOffset];


	terminalPushForeground(TERMINAL_FG_RED);
	terminalPrintf("runtime error");
	terminalPopStyle();
	terminalPrintf(": ");

	va_list args;
	va_start(args, message);

	terminal_vPrintf(message, args);

	terminalPrintf("\n");

	if (vm->moduleName)
		terminalPrintf("--> %s:%d\n", vm->moduleName, line);

	if (vm->source)
	{
		terminalPushForeground(TERMINAL_FG_BLUE);

		terminalPrintf("%4d | ", line - 1);
		terminalPushForeground(TERMINAL_FG_WHITE);
		printSourceLineVm(vm->source, line - 1);
		terminalPopStyle();

		terminalPrintf("     |\n");

		terminalPrintf("%4d | ", line);
		terminalPushForeground(TERMINAL_FG_RED);
		printSourceLineVm( vm->source, line);
		terminalPopStyle();

		terminalPrintf("     |\n");
		
		terminalPrintf("%4d | ", line + 1);
		terminalPushForeground(TERMINAL_FG_WHITE);
		printSourceLineVm(vm->source, line + 1);
		terminalPopStyle();

		terminalPopStyle();
	}

	va_end(args);

	vm->errorRaised = true;
	
}