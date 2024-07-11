

#include "solis_object.h"
#include "solis_hashtable.h"
#include <string.h>
#include "solis_vm.h"

#include <stdio.h>

Object* solisAllocateObject(VM* vm, size_t size, ObjectType type)
{
	Object* object = (Object*)solisReallocate(vm, NULL, 0, size);
	object->type = type;

	// Add the object to the chain
	// Needed for GC
	object->next = vm->objects;
	vm->objects = object;

	object->isMarked = false;

#ifdef SOLIS_DEBUG_LOG_GC
	printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

	return object;
}

void solisFreeObject(VM* vm, Object* object)
{
#ifdef SOLIS_DEBUG_LOG_GC
	printf("%p free type %d\n", (void*)object, object->type);
#endif

	switch (object->type) {
	case OBJ_STRING: {
		ObjString* string = (ObjString*)object;
		SOLIS_FREE_ARRAY(vm, char, string->chars, string->length + 1);
		SOLIS_FREE(vm, ObjString, object);
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction* function = (ObjFunction*)object;
		solisFreeChunk(vm, &function->chunk);
		SOLIS_FREE(vm, ObjFunction, object);
		break;
	}
	case OBJ_CLOSURE:
	{
		ObjClosure* closure = (ObjClosure*)object;
		
		SOLIS_FREE_ARRAY(vm, ObjUpvalue*, closure->upvalues, closure->upvalueCount);
		SOLIS_FREE(vm, ObjClosure, object);
		break;
	}
	case OBJ_NATIVE_FUNCTION:
	{
		SOLIS_FREE(vm, ObjNative, object);
		break;
	}
	case OBJ_ENUM: {
		solisFreeHashTable(&((ObjEnum*)object)->fields);
		SOLIS_FREE(vm, ObjEnum, object);
		break;
	}
	case OBJ_USERDATA:
	{
		ObjUserdata* userdata = (ObjUserdata*)object;

		// Call the cleanup callback if it exists
		if (userdata->cleanupFunc)
			userdata->cleanupFunc(userdata->userdata);

		SOLIS_FREE(vm, ObjUserdata, object);
		break;
	}
	case OBJ_CLASS:
	{
		ObjClass* klass = (ObjClass*)object;
		solisFreeHashTable(&klass->fields);
		solisFreeHashTable(&klass->methods);
		solisFreeHashTable(&klass->statics);
		// SOLIS_FREE(vm, ObjClosure, klass->constructor);
		SOLIS_FREE(vm, ObjClass, object);
		break;
	}
	case OBJ_INSTANCE: {
		ObjInstance* instance = (ObjInstance*)object;
		solisFreeHashTable(&instance->fields);
		SOLIS_FREE(vm, ObjInstance, object);
		break;
	}
	case OBJ_BOUND_METHOD: {
		SOLIS_FREE(vm, ObjBoundMethod, object);
		break;
	}
	case OBJ_UPVALUE: 
	{
		SOLIS_FREE(vm, ObjUpvalue, object);
		break;
	}
	case OBJ_LIST:
	{
		ObjList* list = (ObjList*)object;
		solisValueBufferClear(vm, &list->values);
		SOLIS_FREE(vm, ObjList, object);
		break;
	}
	case OBJ_MODULE:
	{

		ObjModule* mdl = (ObjModule*)object;
		solisValueBufferClear(vm, &mdl->globals);
		solisFreeHashTable(&mdl->globalMap);

		break;
	}
	}
}

static ObjString* allocateString(VM* vm, char* chars, int length, uint32_t hash) {
	ObjString* string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;

	string->obj.classObj = vm->stringClass;

	// Store it in a hash table
	solisPush(vm, SOLIS_OBJECT_VALUE(string));
	solisHashTableInsert(&vm->strings, string, SOLIS_NULL_VALUE());
	solisPop(vm);

	return string;
}

ObjString* solisCopyString(VM* vm, const char* chars, int length)
{
	uint32_t hash = solisHashString(chars, length);

	// See if the string is interned
	ObjString* interned = solisHashTableFindString(&vm->strings, chars, length, hash);

	if (interned != NULL)
	{
		return interned;
	}

	// + 1 so we can add the null terminator
	char* heapChars = SOLIS_ALLOCATE(vm, char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';


	return allocateString(vm, heapChars, length, hash);
}

ObjString* solisTakeString(VM* vm, char* chars, int length)
{
	uint32_t hash = solisHashString(chars, length);

	// See if the string is interned 
	ObjString* interned = solisHashTableFindString(&vm->strings, chars, length, hash);

	if (interned != NULL)
	{
		SOLIS_FREE_ARRAY(vm, char, chars, length + 1);
		return interned;
	}

	return allocateString(vm, chars, length, hash);
}

ObjString* solisConcatenateStrings(VM* vm, ObjString* a, ObjString* b)
{
	int length = a->length + b->length;
	char* chars = SOLIS_ALLOCATE(vm, char, length + 1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	return solisTakeString(vm, chars, length);
}



ObjFunction* solisNewFunction(VM* vm)
{
	ObjFunction* function = ALLOCATE_OBJ(vm, ObjFunction, OBJ_FUNCTION);
	function->arity = 0;
	function->upvalueCount = 0;
	function->name = NULL;
	solisInitChunk(vm, &function->chunk);
	return function;
}


ObjClosure* solisNewClosure(VM* vm, ObjFunction* function)
{
	ObjClosure* closure = ALLOCATE_OBJ(vm, ObjClosure, OBJ_CLOSURE);
	closure->function = function;

	ObjUpvalue** upvalues = SOLIS_ALLOCATE(vm, ObjUpvalue*,
		function->upvalueCount);

	for (int i = 0; i < function->upvalueCount; i++) 
	{
		upvalues[i] = NULL;
	}

	closure->upvalues = upvalues;
	closure->upvalueCount = function->upvalueCount;

	return closure;
}

ObjUpvalue* solisNewUpvalue(VM* vm, Value* slot)
{
	ObjUpvalue* upvalue = ALLOCATE_OBJ(vm, ObjUpvalue, OBJ_UPVALUE);

	upvalue->location = slot;
	upvalue->next = NULL;
	upvalue->closed = SOLIS_NULL_VALUE();

	return upvalue;
}

ObjNative* solisNewNativeFunction(VM* vm, SolisNativeSignature nativeFunc)
{
	ObjNative* native = ALLOCATE_OBJ(vm, ObjNative, OBJ_NATIVE_FUNCTION);

	native->nativeFunction = nativeFunc;
	native->arity = 0;

	return native;
}

ObjEnum* solisNewEnum(VM* vm)
{
	ObjEnum* objEnum = ALLOCATE_OBJ(vm, ObjEnum, OBJ_ENUM);

	objEnum->fieldCount = 0;
	solisInitHashTable(&objEnum->fields, vm);

	return objEnum;
}

ObjUserdata* solisNewUserdata(VM* vm, void* ptr, UserdataCleanup cleanupFunc)
{
	ObjUserdata* userdata = ALLOCATE_OBJ(vm, ObjUserdata, OBJ_USERDATA);

	userdata->userdata = ptr;
	userdata->cleanupFunc = cleanupFunc;

	return userdata;
}

ObjClass* solisNewClass(VM* vm, ObjString* name)
{
	ObjClass* klass = ALLOCATE_OBJ(vm, ObjClass, OBJ_CLASS);
	klass->name = name;

	klass->constructor = NULL;
	klass->obj.classObj = klass;

	solisInitHashTable(&klass->fields, vm);
	solisInitHashTable(&klass->methods, vm);
	solisInitHashTable(&klass->statics, vm);

	memset(klass->operators, 0, sizeof(Object*) * OPERATOR_COUNT);

	return klass;
}

ObjInstance* solisNewInstance(VM* vm, ObjClass* klass)
{
	ObjInstance* instance = ALLOCATE_OBJ(vm, ObjInstance, OBJ_INSTANCE);
	instance->klass = klass;

	instance->obj.classObj = klass;

	solisInitHashTable(&instance->fields, vm);
	solisHashTableCopy(&klass->fields, &instance->fields);
	return instance;
}

ObjBoundMethod* solisNewBoundMethod(VM* vm, Value receiver, ObjClosure* closure)
{
	ObjBoundMethod* bound = ALLOCATE_OBJ(vm, ObjBoundMethod, OBJ_BOUND_METHOD);
	bound->receiver = receiver;
	bound->method = closure;
	bound->nativeFunction = false;
	return bound;
}

ObjBoundMethod* solisNewNativeBoundMethod(VM* vm, Value receiver, ObjNative* closure)
{
	ObjBoundMethod* bound = ALLOCATE_OBJ(vm, ObjBoundMethod, OBJ_BOUND_METHOD);
	bound->receiver = receiver;
	bound->native = closure;
	bound->nativeFunction = true;
	return bound;
}

ObjList* solisNewList(VM* vm)
{
	ObjList* list = ALLOCATE_OBJ(vm, ObjList, OBJ_LIST);

	list->obj.classObj = vm->listClass;

	solisValueBufferInit(vm, &list->values);

	return list;
}

ObjModule* solisNewModule(VM* vm)
{
	ObjModule* mdl = ALLOCATE_OBJ(vm, ObjModule, OBJ_MODULE);

	solisInitHashTable(&mdl->globalMap, vm);
	solisValueBufferInit(vm, &mdl->globals);

	return mdl;
}