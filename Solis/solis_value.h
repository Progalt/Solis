
#ifndef SOLIS_VALUE_H
#define SOLIS_VALUE_H

#include "solis_common.h"
#include <stdbool.h>

#define SOLIS_NUMBER_VALUE(value) ((Value){ VALUE_NUMERIC, { .number = value } })
#define SOLIS_BOOL_VALUE(value) ((Value){ value ? VALUE_TRUE : VALUE_FALSE })
#define SOLIS_NULL_VALUE() ((Value){ VALUE_NULL })

#define SOLIS_AS_NUMBER(value) ((value).as.number)
#define SOLIS_AS_BOOL(value) ((value).type == VALUE_TRUE ? true : false)

#define SOLIS_IS_NULL(value) ((value).type == VALUE_NULL)
#define SOLIS_IS_BOOL(value) (((value).type == VALUE_TRUE) || ((value).type == VALUE_FALSE))
#define SOLIS_IS_NUMERIC(val) ((val).type == VALUE_NUMERIC)


typedef struct Object Object;
typedef struct ObjFiber ObjFiber;
typedef struct ObjFunction ObjFunction;

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
	OBJ_FIBER
} ObjectType;



Object* solisAllocateObject(VM* vm, size_t size, ObjectType type);

#define ALLOCATE_OBJ(type, objectType) \
    (type*)solisAllocateObject(sizeof(type), objectType)



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



bool solisIsNumeric(Value* value);


void solisPrintValueType(Value value);

void solisPrintValue(Value value);

static inline Value solisValueFromNumber(double number)
{
	return SOLIS_NUMBER_VALUE(number);
}

static inline Value solisValueFromBool(bool b)
{
	return SOLIS_BOOL_VALUE(b);
} 

/*
	Checks if the values are strictly the same. Is true if both objects pointers are equal not their contents.
*/
static inline bool solisValuesSame(Value a, Value b)
{
	if (a.type != b.type) return false;
	if (a.type == VALUE_NUMERIC) return a.as.number == b.as.number;
	return a.as.obj == b.as.obj;
}

bool solisValuesEqual(Value a, Value b);

#endif // SOLIS_VALUE_H