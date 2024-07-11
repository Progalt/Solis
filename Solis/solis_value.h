
#ifndef SOLIS_VALUE_H
#define SOLIS_VALUE_H

#include "solis_common.h"
#include <stdbool.h>

#include <string.h>


/*
	Allocates a new object linked to a VM
*/
Object* solisAllocateObject(VM* vm, size_t size, ObjectType type);

void solisFreeObject(VM* vm, Object* object);

#define ALLOCATE_OBJ(vm, type, objectType) \
    (type*)solisAllocateObject(vm, sizeof(type), objectType) 

#define SOLIS_NAN_BOXING

#ifdef SOLIS_NAN_BOXING

// Based on https://craftinginterpreters.com/optimization.html#nan-boxing

#define QNAN     ((uint64_t)0x7ffc000000000000)
#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define MASK_TAG 7

#define TAG_NAN 0 
#define TAG_NIL  1
#define TAG_FALSE 2
#define TAG_TRUE 3 

#define SOLIS_GET_TAG(value) ((int)((value) & MASK_TAG))

typedef uint64_t Value;


static inline Value solisNumToValue(double num) 
{
	Value value;
	memcpy(&value, &num, sizeof(double));
	return value;
}

static inline double solisValueToNum(Value value) 
{
	double num;
	memcpy(&num, &value, sizeof(Value));
	return num;
}

#define SOLIS_NULL_VAL      ((Value)(uint64_t)(QNAN | TAG_NIL))
#define SOLIS_FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define SOLIS_TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))

#define SOLIS_NUMERIC_VALUE(value) solisNumToValue(value)
#define SOLIS_NULL_VALUE() ((Value)(uint64_t)(QNAN | TAG_NIL))
#define SOLIS_BOOL_VALUE(value) ((value) ? SOLIS_TRUE_VAL : SOLIS_FALSE_VAL)
#define SOLIS_OBJECT_VALUE(object) (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(object))

#define SOLIS_AS_NUMBER(value) solisValueToNum(value)
#define SOLIS_AS_BOOL(value) ((value) == SOLIS_TRUE_VAL)
#define SOLIS_AS_OBJECT(value) ((Object*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define SOLIS_IS_NUMERIC(value) (((value) & QNAN) != QNAN)
#define SOLIS_IS_NULL(value) ((value) == SOLIS_NULL_VALUE())
#define SOLIS_IS_BOOL(value)  (((value) | 1) == SOLIS_TRUE_VAL)
#define SOLIS_IS_OBJECT(value)  (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define SOLIS_IS_FALSE(value) ((value) == SOLIS_FALSE_VAL)

#else 

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

#define SOLIS_IS_FALSE(val) ((val).type == VALUE_FALSE)

typedef struct
{

	ValueType type;

	union
	{
		double number;
		Object* obj;
	} as;

} Value;

#endif

static inline ValueType solisGetValueType(Value val)
{
	if (SOLIS_IS_BOOL(val))
		return VALUE_TRUE;
	else if (SOLIS_IS_NUMERIC(val))
		return VALUE_NUMERIC;
	else if (SOLIS_IS_OBJECT(val))
		return VALUE_OBJECT;
	else
		return VALUE_NULL;
}

static inline bool solisIsValueType(Value value, ValueType type)
{
#ifdef SOLIS_NAN_BOXING

	switch (type)
	{
	case VALUE_TRUE:
	case VALUE_FALSE:
		return SOLIS_IS_BOOL(value);
	case VALUE_NUMERIC:
		return SOLIS_IS_NUMERIC(value);
	case VALUE_OBJECT:
		return SOLIS_IS_OBJECT(value);
	case VALUE_NULL:
		return SOLIS_IS_NULL(value);
	}

#else 

	return value.type == type;

#endif
}

/*
	Checks if the values are strictly the same. Is true if both objects pointers are equal not their contents.
*/
static inline bool solisValuesSame(Value a, Value b)
{
#ifdef SOLIS_NAN_BOXING

	if (SOLIS_IS_NUMERIC(a) && SOLIS_IS_NUMERIC(b))
		return SOLIS_AS_NUMBER(a) == SOLIS_AS_NUMBER(b);

	return a == b;

#else 
	if (a.type != b.type) return false;
	if (a.type == VALUE_NUMERIC) return a.as.number == b.as.number;
	return a.as.obj == b.as.obj;
#endif
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