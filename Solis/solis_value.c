
#include "solis_value.h"
#include <stdio.h>

#include "solis_vm.h"

Object* solisAllocateObject(VM* vm, size_t size, ObjectType type)
{
	Object* object = (Object*)solisReallocate(NULL, 0, size);
	object->type = type;

	object->next = vm->objects;
	vm->objects = object;

	return object;
}


bool solisIsNumeric(Value* value)
{
	return SOLIS_IS_NUMERIC((*value));
}



void solisPrintValueType(Value value)
{
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


bool solisValuesEqual(Value a, Value b)
{
	if (solisValuesSame(a, b)) return true;


	return false;
}