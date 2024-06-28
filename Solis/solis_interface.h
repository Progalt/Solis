
#ifndef SOLIS_INTERFACE_H
#define SOLIS_INTERFACE_H

#include "solis_common.h"
#include "solis_value.h"

typedef void(*SolisNativeSignature)(VM*);


double solisCheckNumber(VM* vm, int argIndex);


#endif // SOLIS_INTERFACE_H