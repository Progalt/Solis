
#include "solis_core.h"


void num_abs(VM* vm)
{
    double num = SOLIS_AS_NUMBER(solisGetSelf(vm));

    num = num < 0 ? -num : num;

    solisSetReturnValue(vm, SOLIS_NUMERIC_VALUE(num));
}

void solisInitialiseCore(VM* vm)
{
    vm->numberClass = SOLIS_AS_CLASS(solisCreateClass(vm, "Number"));
    solisAddClassNativeMethod(vm, SOLIS_OBJECT_VALUE(vm->numberClass), "abs", num_abs, 0);
}