
#ifndef SOLIS_VALUE_H
#define SOLIS_VALUE_H

#include "solis_common.h"
#include <stdbool.h>

#define SOLIS_NUMERIC_VALUE(value) ((Value){ VALUE_NUMERIC, { .number = value } })
#define SOLIS_BOOL_VALUE(value) ((Value){ value ? VALUE_TRUE : VALUE_FALSE })
#define SOLIS_NULL_VALUE() ((Value){ VALUE_NULL })
#define SOLIS_OBJECT_VALUE(object)   ((Value){VALUE_OBJECT, {.obj = (Object*)object}})

#define SOLIS_AS_NUMBER(value) ((value).as.number)
#define SOLIS_AS_BOOL(value) ((value).type == VALUE_TRUE ? true : false)
#define SOLIS_AS_OBJECT(value) ((value).as.obj)

#define SOLIS_IS_NULL(value) ((value).type == VALUE_NULL)
#define SOLIS_IS_BOOL(value) (((value).type == VALUE_TRUE) || ((value).type == VALUE_FALSE))
#define SOLIS_IS_NUMERIC(val) ((val).type == VALUE_NUMERIC)
#define SOLIS_IS_OBJECT(val) ((val).type == VALUE_OBJECT)


typedef struct Object Object;
typedef struct ObjFiber ObjFiber;
typedef struct ObjFunction ObjFunction;
typedef struct ObjString ObjString; 

typedef enum
{
	VALUE_NULL,
	VALUE_TRUE,
	VALUE_FALSE,
	VALUE_NUMERIC,
	VALUE_OBJECT
} ValueType;

typedef enum
{
	OBJ_FIBER,
	OBJ_STRING
} ObjectType;


/*
	Allocates a new object linked to a VM
*/
Object* solisAllocateObject(VM* vm, size_t size, ObjectType type);

#define ALLOCATE_OBJ(vm, type, objectType) \
    (type*)solisAllocateObject(vm, sizeof(type), objectType)



typedef struct
{

	ValueType type;

	union
	{
		double number;
		Object* obj;
	} as;

} Value;


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
};

#define SOLIS_IS_STRING(value)	solisIsObjType(value, OBJ_STRING)

#define SOLIS_AS_STRING(value) ((ObjString*)SOLIS_AS_OBJECT(value))
#define SOLIS_AS_CSTRING(value) (((ObjString*)SOLIS_AS_OBJECT(value))->chars)


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
	Checks if the values are strictly the same. Is true if both objects pointers are equal not their contents.
*/
static inline bool solisValuesSame(Value a, Value b)
{
	if (a.type != b.type) return false;
	if (a.type == VALUE_NUMERIC) return a.as.number == b.as.number;
	return a.as.obj == b.as.obj;
}

/*
	Returns true if two values are identical in contents
*/
bool solisValuesEqual(Value a, Value b);



// Helper functions for printing

/*
	Prints the value type to stdout
*/
void solisPrintValueType(Value value);

/*
	Prints the value to stdout
*/
void solisPrintValue(Value value);

#endif // SOLIS_VALUE_H