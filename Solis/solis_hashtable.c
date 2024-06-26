
#include "solis_hashtable.h"
#include "solis_common.h"

static TableEntry* findEntry(TableEntry* entries, int capacity, ObjString* key)
{
	// Instead of modulus use some bitwise magic 
	// Since its faster
	// Because % and / are slow on CPUs... well not really but slower than this 
	uint32_t index = key->hash & (capacity - 1);

	for (;;) {
		TableEntry* entry = &entries[index];
		if (entry->key == key || entry->key == NULL) {
			return entry;
		}

		index = (index + 1) & (capacity - 1);
	}
}


static void adjustCapacity(HashTable* table, int capacity) {
	TableEntry* entries = SOLIS_ALLOCATE(TableEntry, capacity);
	for (int i = 0; i < capacity; i++) {
		entries[i].key = NULL;
		entries[i].value = SOLIS_NULL_VALUE();
	}

	for (int i = 0; i < table->capacity; i++) {
		TableEntry* entry = &table->entries[i];
		if (entry->key == NULL) continue;

		TableEntry* dest = findEntry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
	}

	// Free the old memory
	SOLIS_FREE_ARRAY(TableEntry, table->entries, table->capacity);

	// Assign the new values
	table->entries = entries;
	table->capacity = capacity;
}


void solisInitHashTable(HashTable* table)
{
	table->capacity = 0;
	table->count = 0;
	table->entries = NULL;
}

void solisFreeHashTable(HashTable* table)
{
	SOLIS_FREE_ARRAY(TableEntry, table->entries, table->capacity);
	solisInitHashTable(table);
}

bool solisHashTableInsert(HashTable* table, ObjString* key, Value value)
{
	if (table->count + 1 > table->capacity * SOLIS_HASH_TABLE_MAX_LOAD) 
	{
		int capacity = GROW_CAPACITY(table->capacity);
		adjustCapacity(table, capacity);
	}

	TableEntry* entry = findEntry(table->entries, table->capacity, key);

	bool isNewKey = entry->key == NULL;
	if (isNewKey) table->count++;

	entry->key = key;
	entry->value = value;

	return isNewKey;
}

bool solisHashTableGet(HashTable* table, ObjString* key, Value* value)
{
	if (table->count == 0) 
		return false;

	TableEntry* entry = findEntry(table->entries, table->capacity, key);

	if (entry->key == NULL) 
		return false;

	*value = entry->value;
	return true;
}

void solisHashTableCopy(HashTable* from, HashTable* to)
{
	for (int i = 0; i < from->capacity; i++) 
	{
		TableEntry* entry = &from->entries[i];
		if (entry->key != NULL) 
		{
			solisHashTableInsert(to, entry->key, entry->value);
		}
	}
}