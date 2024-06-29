
#ifndef SOLIS_INTERFACE_H
#define SOLIS_INTERFACE_H

#include "solis_common.h"
#include "solis_value.h"

/*
	Function signature for C functions
*/
typedef void(*SolisNativeSignature)(VM*);

/*
	These functions help with binding C code to the Solis VM. 
*/

double solisCheckNumber(VM* vm, int argIndex);

const char* solisCheckString(VM* vm, int argIndex);

/*
	Get an argument from the function argument list. 

	Must be called from within a function bound to the VM. 
*/
Value solisGetArgument(VM* vm, int argIndex);

/*
	Sets the return value from the function and passes it back to the VM. 

	All C functions must call this, if there are no return values pass a null value. 
*/
void solisSetReturnValue(VM* vm, Value value);

#endif // SOLIS_INTERFACE_H