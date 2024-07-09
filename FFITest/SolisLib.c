
#include "SolisLib.h"

#include <stdio.h>

void solis_dothing(VM* vm)
{
	Value arg = solisGetArgument(vm, 0);

	printf("Doing something...\n");

	solisSetReturnValue(vm, arg);

}

void solis_openlib(VM* vm)
{
	printf("Called from DLL\n");

	solisPushGlobalCFunction(vm, "dllDoSomething", solis_dothing, 1);

	solisSetReturnValue(vm, SOLIS_BOOL_VALUE(true));
}