
#ifndef SOLIS_OBJECT_H
#define SOLIS_OBJECT_H

#include "solis_common.h"

#include "solis_chunk.h"

struct Object
{
	ObjectType type;

	// is the object marked by the gc
	bool isMarked;

	// Next object in the allocated linked list
	Object* next;
};

/*
	Object that represent
*/
struct ObjString
{
	Object obj;

	int length;
	char* chars;

	// We store the hash in the string
	// TODO: Could this be moved out of string since other objects might want hashing
	uint32_t hash;
};

#define SOLIS_IS_STRING(value)	solisIsObjType(value, OBJ_STRING)

#define SOLIS_AS_STRING(value) ((ObjString*)SOLIS_AS_OBJECT(value))
#define SOLIS_AS_CSTRING(value) (((ObjString*)SOLIS_AS_OBJECT(value))->chars)


struct ObjFunction
{
	Object obj;

	int arity;
	Chunk chunk;
	ObjString* name;
};

#define SOLIS_IS_FUNCTION(value) solisIsObjType(value, OBJ_FUNCTION)
#define SOLIS_AS_FUNCTION(value) ((ObjFunction*)SOLIS_AS_OBJECT(value))

/*
	Returns the specified value is equal to the type
	If the value is not an object it returns false.
*/
static inline bool solisIsObjType(Value value, ObjectType type)
{
	return SOLIS_IS_OBJECT(value) && SOLIS_AS_OBJECT(value)->type == type;
}


/*
	Copies a cstring into a String object and terminates it
*/
ObjString* solisCopyString(VM* vm, const char* chars, int length);

/*
	Takes owner ship of the supplied values and combines them into a String object
*/
ObjString* solisTakeString(VM* vm, char* chars, int length);

/*
	Concatenates two strings into a new string object
*/
ObjString* solisConcatenateStrings(VM* vm, ObjString* a, ObjString* b);


ObjFunction* solisNewFunction(VM* vm);

#endif // SOLIS_OBJECT_H