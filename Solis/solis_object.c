

#include "solis_object.h"
#include "solis_hashtable.h"
#include <string.h>
#include "solis_vm.h"

Object* solisAllocateObject(VM* vm, size_t size, ObjectType type)
{
	Object* object = (Object*)solisReallocate(NULL, 0, size);
	object->type = type;

	// Add the object to the chain
	// Needed for GC
	object->next = vm->objects;
	vm->objects = object;

	return object;
}

void solisFreeObject(VM* vm, Object* object)
{
	switch (object->type) {
	case OBJ_STRING: {
		ObjString* string = (ObjString*)object;
		SOLIS_FREE_ARRAY(char, string->chars, string->length + 1);
		SOLIS_FREE(ObjString, object);
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction* function = (ObjFunction*)object;
		solisFreeChunk(&function->chunk);
		SOLIS_FREE(ObjFunction, object);
		break;
	}
	case OBJ_CLOSURE:
	{
		ObjClosure* closure = (ObjClosure*)object;
		
		SOLIS_FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
		SOLIS_FREE(ObjClosure, object);
		break;
	}
	case OBJ_UPVALUE: 
	{
		SOLIS_FREE(ObjUpvalue, object);
		break;
	}
	}
}

static ObjString* allocateString(VM* vm, char* chars, int length, uint32_t hash) {
	ObjString* string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;

	// Store it in a hash table
	solisHashTableInsert(&vm->strings, string, SOLIS_NULL_VALUE());

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
	char* heapChars = SOLIS_ALLOCATE(char, length + 1);
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
		SOLIS_FREE_ARRAY(char, chars, length + 1);
		return interned;
	}

	return allocateString(vm, chars, length, hash);
}

ObjString* solisConcatenateStrings(VM* vm, ObjString* a, ObjString* b)
{
	int length = a->length + b->length;
	char* chars = SOLIS_ALLOCATE(char, length + 1);
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
	solisInitChunk(&function->chunk);
	return function;
}


ObjClosure* solisNewClosure(VM* vm, ObjFunction* function)
{
	ObjClosure* closure = ALLOCATE_OBJ(vm, ObjClosure, OBJ_CLOSURE);
	closure->function = function;

	ObjUpvalue** upvalues = SOLIS_ALLOCATE(ObjUpvalue*,
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