
#include "solis_common.h"
#include "solis_vm.h"

#include <stdio.h>
#include "solis_object.h"

#include "solis_gc.h"

void* solisReallocate(VM* vm, void* ptr, size_t oldSize, size_t newSize)
{

    vm->allocatedBytes += newSize - oldSize;



    if (newSize > oldSize)
    {
#ifdef SOLIS_DEBUG_STRESS_GC
        solisCollectGarbage(vm);
#endif

        // see if we need to run the gc
        if (vm->allocatedBytes > vm->nextGC)
        {
            solisCollectGarbage(vm);
        }
    }



    if (newSize == 0) {
        SOLIS_FREE_FUNC(ptr);
        return NULL;
    }

    void* result = SOLIS_REALLOC_FUNC(ptr, newSize);

    if (result == NULL) exit(1);


    // printf("Allocated bytes: %d\n", vm->allocatedBytes);

    return result;

}
