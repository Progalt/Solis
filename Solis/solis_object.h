
#ifndef SOLIS_OBJECT_H
#define SOLIS_OBJECT_H

#include "solis_common.h"

#include "solis_chunk.h"

#include "solis_interface.h"
#include "solis_hashtable.h"

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
	int upvalueCount;
	Chunk chunk;
	ObjString* name;
};

typedef struct ObjUpvalue {

	Object obj;
	Value* location;

	Value closed;
	struct ObjUpvalue* next;


} ObjUpvalue;

#define SOLIS_IS_FUNCTION(value) solisIsObjType(value, OBJ_FUNCTION)
#define SOLIS_AS_FUNCTION(value) ((ObjFunction*)SOLIS_AS_OBJECT(value))

struct ObjClosure
{
	Object obj;

	ObjFunction* function;

	ObjUpvalue** upvalues;
	int upvalueCount;
};

#define SOLIS_IS_CLOSURE(value) solisIsObjType(value, OBJ_CLOSURE)
#define SOLIS_AS_CLOSURE(value) ((ObjClosure*)SOLIS_AS_OBJECT(value))

struct ObjNative
{
	Object obj;
	SolisNativeSignature nativeFunction;
	int arity;
};

#define SOLIS_IS_NATIVE(value) solisIsObjType(value, OBJ_NATIVE_FUNCTION)
#define SOLIS_AS_NATIVE(value) ((ObjNative*)SOLIS_AS_OBJECT(value))

struct ObjEnum
{
	Object obj;

	int fieldCount;
	HashTable fields;
};

#define SOLIS_IS_ENUM(value) solisIsObjType(value, OBJ_ENUM)
#define SOLIS_AS_ENUM(value) ((ObjEnum*)SOLIS_AS_OBJECT(value))

typedef void(*UserdataCleanup)(void*);

/*
	A Userdata object is an object that has no meaning in the language itself
	It is a pointer to a C object and can be useful in binding C/C++ classes or objects to the language 
*/
struct ObjUserdata
{
	Object obj;

	void* userdata;

	UserdataCleanup cleanupFunc;
};

#define SOLIS_IS_USERDATA(value) solisIsObjType(value, OBJ_USERDATA)
#define SOLIS_AS_USERDATA(value) ((ObjUserdata*)SOLIS_AS_OBJECT(value))

struct ObjClass
{
	Object obj;

	ObjString* name;

	// This is all the static variables that belong to the class instead 
	HashTable statics;

	HashTable fields;
	HashTable methods;
};

#define SOLIS_IS_CLASS(value) solisIsObjType(value, OBJ_CLASS)
#define SOLIS_AS_CLASS(value) ((ObjClass*)SOLIS_AS_OBJECT(value))

struct ObjInstance
{
	Object obj;

	ObjClass* klass;

	// These fields are initialised with the fields from the klass object
	HashTable fields;
};

#define SOLIS_IS_INSTANCE(value) solisIsObjType(value, OBJ_INSTANCE)
#define SOLIS_AS_INSTANCE(value) ((ObjInstance*)SOLIS_AS_OBJECT(value))


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

ObjClosure* solisNewClosure(VM* vm, ObjFunction* function);

ObjUpvalue* solisNewUpvalue(VM* vm, Value* slot);

ObjNative* solisNewNativeFunction(VM* vm, SolisNativeSignature nativeFunc);

ObjEnum* solisNewEnum(VM* vm);

ObjUserdata* solisNewUserdata(VM* vm, void* ptr, UserdataCleanup cleanupFunc);

ObjClass* solisNewClass(VM* vm, ObjString* name);

ObjInstance* solisNewInstance(VM* vm, ObjClass* klass);

#endif // SOLIS_OBJECT_H