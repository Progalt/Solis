
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


/*
	hash tables are implemented with the help of crafting interpreters. 
*/

#define SOLIS_HASH_TABLE_MAX_LOAD 0.75

/*
	Each table entry has a string key and a value
	TODO: Make it not just use a string as key. This could be done with using a value and a hash function for each value? 
*/
typedef struct
{
	ObjString* key;
	Value value;
} TableEntry;

/*
	Hash table struct
	Read only. Use the functions to access values
*/
typedef struct
{
	int count;
	int capacity;
	TableEntry* entries;
} HashTable;

/*
	Initialise a hash table
*/
void solisInitHashTable(HashTable* table);

/*
	Frees all memory associated with a hash table. Does not free memory of objects inserted into the table
*/
void solisFreeHashTable(HashTable* table);

/*
	Insert a new value into the hash table by key. If the key already exists its replaced. 
*/
bool solisHashTableInsert(HashTable* table, ObjString* key, Value value);

/*
	Get a value from the tabel via key. Returned in value pointer. 
*/
bool solisHashTableGet(HashTable* table, ObjString* key, Value* value);

/*
	Deletes a value from the hash table by key. 
*/
bool solisHashTableDelete(HashTable* table, ObjString* key);

/*
	Find a string in the table, this string isn't yet created so it compares to other entries. 

	Used for string interning 
*/
ObjString* solisHashTableFindString(HashTable* table, const char* chars, int length, uint32_t hash);

/*
	Copies one hash table to another hash table. 
*/
void solisHashTableCopy(HashTable* from, HashTable* to);

#endif // SOLIS_HASHTABLE_H