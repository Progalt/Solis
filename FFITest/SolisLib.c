
#include "SolisLib.h"

#include <stdio.h>


void solis_openlib(VM* vm)
{
	printf("Called from DLL\n");

	solisSetReturnValue(vm, SOLIS_BOOL_VALUE(true));
}