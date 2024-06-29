
#ifndef SOLIS_GC_H
#define SOLIS_GC_H

#include "solis_common.h"
#include "solis_value.h"
#include "solis_hashtable.h"
#include "solis_object.h"

void solisCollectGarbage(VM* vm);

void markObject(VM* vm, Object* object);
void markValue(VM* vm, Value value);
void markTable(VM* vm, HashTable* table);
void markValueBuffer(VM* vm, ValueBuffer* buffer);


void tableRemoveWhite(VM* vm, HashTable* table);

#endif // SOLIS_GC_H