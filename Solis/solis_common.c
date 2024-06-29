
#include "solis_common.h"
#include "solis_vm.h"

#include <stdio.h>
#include "solis_object.h"

#include "solis_gc.h"

void* solisReallocate(VM* vm, void* ptr, size_t oldSize, size_t newSize)
{

    if (newSize == 0) {
        SOLIS_FREE_FUNC(ptr);
        return NULL;
    }

    if (newSize > oldSize)
    {
#ifdef SOLIS_DEBUG_STRESS_GC
        solisCollectGarbage(vm);
#endif
    }

    void* result = SOLIS_REALLOC_FUNC(ptr, newSize);

    if (result == NULL) exit(1);

    vm->allocatedBytes += newSize - oldSize;

    if (vm->allocatedBytes > vm->nextGC)
    {
        solisCollectGarbage(vm);
    }

    // printf("Allocated bytes: %d\n", vm->allocatedBytes);

    return result;

}
