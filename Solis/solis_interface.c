
#include "solis_interface.h"

#include "solis_vm.h"
#include <string.h>

Value solisGetArgument(VM* vm, int argIndex)
{
	return *(vm->apiStack + (argIndex + 1));
}

double solisCheckNumber(VM* vm, int argIndex)
{
	Value v = solisGetArgument(vm, argIndex);

	if (SOLIS_IS_NUMERIC(v))
	{
		return SOLIS_AS_NUMBER(v);
	}

	// TODO: Error check

	return 0.0;

}

const char* solisCheckString(VM* vm, int argIndex)
{
	Value v = solisGetArgument(vm, argIndex);

	if (SOLIS_IS_STRING(v))
	{
		return SOLIS_AS_CSTRING(v);
	}

	// TODO: Error check

	return NULL;
}

void solisSetReturnValue(VM* vm, Value value)
{
	// the return is the root of the api stack 
	*vm->apiStack = value;
}

Value solisCreateEnumObject(VM* vm, const char* name)
{
	ObjEnum* _enum = solisNewEnum(vm);
	
	// Push as a global into the VM
	solisPushGlobal(vm, name, SOLIS_OBJECT_VALUE(_enum));

	// just return a new value with the ptr 
	return SOLIS_OBJECT_VALUE(_enum);

}

void solisBindEnumEntry(VM* vm, Value enumObj, const char* name)
{
	// return if we aren't an enum
	if (!SOLIS_IS_ENUM(enumObj))
		return;

	ObjEnum* _enum = SOLIS_AS_ENUM(enumObj);

	ObjString* nameStr = solisCopyString(vm, name, strlen(name));

	// Push onto stack so it exists somewhere the gc can see
	// As insert could cause a realloc 
	solisPush(vm, SOLIS_OBJECT_VALUE(name));

	solisHashTableInsert(&_enum->fields, nameStr, SOLIS_NUMERIC_VALUE((double)_enum->fieldCount));

	solisPop(vm);

	_enum->fieldCount++;
	
}