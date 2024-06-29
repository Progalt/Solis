
#include "solis_interface.h"

#include "solis_vm.h"

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