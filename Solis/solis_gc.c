
#include "solis_gc.h"

#include <stdio.h>

#include "solis_vm.h"
#include "solis_compiler.h"

#define GC_HEAP_GROW_FACTOR 2

void markObject(VM* vm, Object* object)
{
    if (object == NULL)
        return;

    if (object->isMarked) 
        return;

//#ifdef SOLIS_DEBUG_LOG_GC
//    printf("%p mark ", (void*)object);
//    solisPrintValue(SOLIS_OBJECT_VALUE(object));
//    printf("\n");
//#endif

    object->isMarked = true;

    if (vm->greyCapacity < vm->greyCount + 1) {
        vm->greyCapacity = GROW_CAPACITY(vm->greyCapacity);
        vm->greyStack = (Object**)realloc(vm->greyStack, sizeof(Object*) * vm->greyCapacity);
    }

    if (vm->greyStack == NULL) exit(1);

    vm->greyStack[vm->greyCount++] = object;
}

void markValue(VM* vm, Value value)
{
    if (SOLIS_IS_OBJECT(value))
        markObject(vm, SOLIS_AS_OBJECT(value));
}

void markTable(VM* vm, HashTable* table)
{
    for (int i = 0; i < table->capacity; i++)
    {
        TableEntry* entry = &table->entries[i];
        markObject(vm, (Object*)entry->key);
        markValue(vm, entry->value);
    }
}

void markValueBuffer(VM* vm, ValueBuffer* buffer)
{
    for (int i = 0; i < buffer->count; i++)
    {
        markValue(vm, buffer->data[i]);
    }
}

static void markRoots(VM* vm)
{
    for (Value* slot = vm->stack; slot < vm->sp; slot++)
    {
        markValue(vm, *slot);
    }

    for (int i = 0; i < vm->frameCount; i++)
    {
        markObject(vm, (Object*)vm->frames[i].closure);
    }

    // Mark the two objects associated with globals
    markTable(vm, &vm->globalMap);
    markValueBuffer(vm, &vm->globals);

    for (ObjUpvalue* upvalue = vm->openUpvalues;
        upvalue != NULL;
        upvalue = upvalue->next)
    {
        markObject(vm, (Object*)upvalue);
    }

    solisMarkCompilerRoots(vm);
}


static void blackenObject(VM* vm, Object* object) 
{
//#ifdef SOLIS_DEBUG_LOG_GC
//    printf("%p blacken ", (void*)object);
//    solisPrintValue(SOLIS_OBJECT_VALUE(object));
//    printf("\n");
//#endif

    switch (object->type) {
    case OBJ_UPVALUE:
        markValue(vm, ((ObjUpvalue*)object)->closed);
        break;
    case OBJ_FUNCTION: {
        ObjFunction* function = (ObjFunction*)object;
        markObject(vm, (Object*)function->name);
        markValueBuffer(vm, &function->chunk.constants);
        break;
    }
    case OBJ_CLOSURE: {
        ObjClosure* closure = (ObjClosure*)object;
        markObject(vm, (Object*)closure->function);
        for (int i = 0; i < closure->upvalueCount; i++) {
            markObject(vm, (Object*)closure->upvalues[i]);
        }
        break;
    }
    case OBJ_ENUM: {
        markTable(vm, &((ObjEnum*)object)->fields);
        break;
    }
    case OBJ_CLASS: {
        ObjClass* klass = (ObjClass*)object;
        markObject(vm, (Object*)klass->name);
        markTable(vm, &klass->fields);
        markTable(vm, &klass->methods);
        markTable(vm, &klass->statics);

    }
    case OBJ_INSTANCE: {
        ObjInstance* instance = (ObjInstance*)object;
        markObject(vm, (Object*)instance->klass);
        markTable(vm, &instance->fields);
        break;
    }
    case OBJ_BOUND_METHOD: {

        ObjBoundMethod* bound = (ObjBoundMethod*)object;
        markValue(vm, bound->receiver);
        markObject(vm, (Object*)bound->method);

        break;
    }
    case OBJ_NATIVE_FUNCTION:
    case OBJ_STRING:
        break;
    }
}

static void traceReferences(VM* vm) 
{
    while (vm->greyCount > 0) {
        Object* object = vm->greyStack[--vm->greyCount];
        blackenObject(vm, object);
    }
}

static void sweep(VM* vm) {
    Object* previous = NULL;
    Object* object = vm->objects;
    while (object != NULL) 
    {
        if (object->isMarked) 
        {
            object->isMarked = false;
            previous = object;
            object = object->next;
        }
        else 
        {
            Object* unreached = object;
            object = object->next;
            if (previous != NULL) 
            {
                previous->next = object;
            }
            else {
                vm->objects = object;
            }

            solisFreeObject(vm, unreached);
        }
    }
}

void tableRemoveWhite(VM* vm, HashTable* table)
{
    // NOTE: There is a bug here in somewhere
    // It only happens when stressing the GC atm but it should probably be fixed 
    for (int i = 0; i < table->capacity; i++) 
    {
        TableEntry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked) 
        {
            solisHashTableDelete(table, entry->key);
        }
    }
}

void solisCollectGarbage(VM* vm)
{
#ifdef SOLIS_DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif

    markRoots(vm);

    traceReferences(vm);

    tableRemoveWhite(vm, &vm->strings);

    sweep(vm);

    vm->nextGC = vm->allocatedBytes * GC_HEAP_GROW_FACTOR;

#ifdef SOLIS_DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}