
#ifndef SOLIS_HASHTABLE_H
#define SOLIS_HASHTABLE_H

#include <stdint.h>
#include "solis_value.h"

/*
	Helper function to hash a string. All string objects are hashed at allocation so they don't need to be rehashed. 
	Hashed using the FNV-1a algorithm 
	https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
*/
static inline uint32_t solisHashString(const char* key, int length)
{
	uint32_t hash = 2166136261u;
	for (int i = 0; i < length; i++)
	{
		hash ^= (uint8_t)key[i];
		hash *= 16777619;
	}
	return hash;
}

#define SOLIS_HASH_TABLE_MAX_LOAD 0.75

typedef struct
{
	ObjString* key;
	Value value;
} TableEntry;

typedef struct
{
	int count;
	int capacity;
	TableEntry* entries;
} HashTable;

void solisInitHashTable(HashTable* table);

void solisFreeHashTable(HashTable* table);

bool solisHashTableInsert(HashTable* table, ObjString* key, Value value);

bool solisHashTableGet(HashTable* table, ObjString* key, Value* value);

bool solisHashTableDelete(HashTable* table, ObjString* key);

ObjString* solisHashTableFindString(HashTable* table, const char* chars, int length, uint32_t hash);

void solisHashTableCopy(HashTable* from, HashTable* to);

#endif // SOLIS_HASHTABLE_H