
#ifndef SOLIS_INTERFACE_H
#define SOLIS_INTERFACE_H

#include "solis_common.h"
#include "solis_value.h"

typedef void(*SolisNativeSignature)(VM*);


double solisCheckNumber(VM* vm, int argIndex);

const char* solisCheckString(VM* vm, int argIndex);

void solisSetReturnValue(VM* vm, Value value);

#endif // SOLIS_INTERFACE_H