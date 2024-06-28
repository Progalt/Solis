
#include "solis_interface.h"

#include "solis_vm.h"

double solisCheckNumber(VM* vm, int argIndex)
{
	Value v = vm->apiStack[argIndex];

	if (SOLIS_IS_NUMERIC(v))
	{
		return SOLIS_AS_NUMBER(v);
	}

	// TODO: Error check

	return 0.0;

}

const char* solisCheckString(VM* vm, int argIndex)
{
	Value v = vm->apiStack[argIndex];

	if (SOLIS_IS_STRING(v))
	{
		return SOLIS_AS_CSTRING(v);
	}

	// TODO: Error check

	return NULL;
}

void solisSetReturnValue(VM* vm, Value value)
{
	*vm->apiStack = value;
}