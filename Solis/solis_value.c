
#include "solis_value.h"
#include <stdio.h>

#include "solis_vm.h"
#include "solis_common.h"

#include "solis_hashtable.h"

#include <string.h>

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
	}
}

static ObjString* allocateString(VM* vm, char* chars, int length, uint32_t hash) {
	ObjString* string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;
	return string;
}

ObjString* solisCopyString(VM* vm, const char* chars, int length)
{
	// + 1 so we can add the null terminator
	char* heapChars = SOLIS_ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';

	uint32_t hash = solisHashString(chars, length);
	return allocateString(vm, heapChars, length, hash);
}

ObjString* solisTakeString(VM* vm, char* chars, int length)
{
	uint32_t hash = solisHashString(chars, length);
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


bool solisValuesEqual(Value a, Value b)
{
	if (solisValuesSame(a, b)) return true;

	if (SOLIS_IS_STRING(a))
	{
		ObjString* astr = SOLIS_AS_STRING(a);
		ObjString* bstr = SOLIS_AS_STRING(b);

		if (memcmp(astr->chars, bstr->chars, astr->length) == 0)
			return true;
	}
	

	return false;
}

void solisPrintValueType(Value value)
{
	if (SOLIS_IS_OBJECT(value))
	{
		switch (SOLIS_AS_OBJECT(value)->type)
		{
		case OBJ_STRING:
			printf("String");
			break;
		default:
			printf("Unknown Object type");
			break;
		}

		return;
	}

	switch (value.type)
	{
	case VALUE_NULL:
		printf("Null");
		break;
	case VALUE_TRUE:
		printf("Boolean");
		break;
	case VALUE_FALSE:
		printf("Boolean");
		break;
	case VALUE_NUMERIC:
		printf("Numeric");
		break;
	default:
		printf("Unknown value type");
		break;
	}
}

void solisPrintValue(Value value)
{
	if (SOLIS_IS_OBJECT(value))
	{
		switch (SOLIS_AS_OBJECT(value)->type)
		{
		case OBJ_STRING:
			printf("%s", SOLIS_AS_CSTRING(value));
			break;
		default:
			printf("Unknown Object type");
			break;
		}

		return;
	}

	switch (value.type)
	{
	case VALUE_NULL:
		printf("null");
		break;
	case VALUE_TRUE:
		printf("true");
		break;
	case VALUE_FALSE:
		printf("false");
		break;
	case VALUE_NUMERIC:
		printf("%g", value.as.number);
		break;
	default:
		printf("Unknown value type");
		break;
	}
}

