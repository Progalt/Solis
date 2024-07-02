
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

Value solisGetSelf(VM* vm);



/*
	Creates a new enum object in the VM.

	These are created globally and can be accessed anywhere
*/
Value solisCreateEnumObject(VM* vm, const char* name);

/*
	Binds a new value to an enum. These go up in by 1 each time. 
*/
void solisBindEnumEntry(VM* vm, Value enumObj, const char* name);

/*
	Creates a new global class
*/
Value solisCreateClass(VM* vm, const char* name);

/*
	Add a class field to a class. 
*/
void solisAddClassField(VM* vm, Value klassValue, const char* name, bool isStatic, Value defaultValue);

/*
	Sets the static field of a class 
*/
void solisSetStaticField(VM* vm, Value klassValue, const char* name, Value value);

Value solisGetStaticField(VM* vm, Value klassValue, const char* name);

void solisSetInstanceField(VM* vm, Value instance, const char* name, Value value);

Value solisGetInstanceField(VM* vm, Value instance, const char* name);

#endif // SOLIS_INTERFACE_H