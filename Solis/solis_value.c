
#include "solis_value.h"
#include <stdio.h>

#include "solis_vm.h"
#include "solis_common.h"

#include "solis_hashtable.h"
#include "solis_chunk.h"

#include <string.h>


bool solisValuesEqual(Value a, Value b)
{
	if (solisValuesSame(a, b)) return true;

	if (SOLIS_IS_STRING(a))
	{
		ObjString* astr = SOLIS_AS_STRING(a);
		ObjString* bstr = SOLIS_AS_STRING(b);

		if (memcmp(astr->chars, bstr->chars, astr->length) == 0)
			return true;
	}
	

	return false;
}

void solisPrintValueType(Value value)
{
	if (SOLIS_IS_OBJECT(value))
	{
		switch (SOLIS_AS_OBJECT(value)->type)
		{
		case OBJ_STRING:
			printf("String");
			break;
		case OBJ_FUNCTION:
			printf("Function");
			break;
		case OBJ_CLOSURE:
			printf("Closure");
			break;
		case OBJ_UPVALUE:
			printf("Upvalue");
			break;
		case OBJ_NATIVE_FUNCTION:
			printf("Native Function");
			break;
		default:
			printf("Unknown Object type");
			break;
		}

		return;
	}

	switch (solisGetValueType(value))
	{
	case VALUE_NULL:
		printf("Null");
		break;
	case VALUE_TRUE:
		printf("Boolean");
		break;
	case VALUE_FALSE:
		printf("Boolean");
		break;
	case VALUE_NUMERIC:
		printf("Numeric");
		break;
	default:
		printf("Unknown value type");
		break;
	}
}

void solisPrintValue(Value value)
{
	if (SOLIS_IS_OBJECT(value))
	{
		switch (SOLIS_AS_OBJECT(value)->type)
		{
		case OBJ_STRING:
			printf("%s", SOLIS_AS_CSTRING(value));
			break;
		case OBJ_FUNCTION:
			if (SOLIS_AS_FUNCTION(value)->name == NULL)
			{
				printf("<script>");
			}
			else
			{
				printf("<fn %s>", SOLIS_AS_FUNCTION(value)->name->chars);
			}
			break;
		case OBJ_CLOSURE:
			if (SOLIS_AS_CLOSURE(value)->function->name)
			{
				printf("<cl %s>", SOLIS_AS_CLOSURE(value)->function->name->chars);
			}
			else
			{
				printf("<script>");
			}
			break;
		case OBJ_UPVALUE:
			printf("upvalue");
			break;
		case OBJ_NATIVE_FUNCTION:
			printf("native");
			break;
		case OBJ_CLASS:
			printf("%s", SOLIS_AS_CLASS(value)->name->chars);
			break;
		case OBJ_INSTANCE:
			printf("instance of %s", SOLIS_AS_INSTANCE(value)->klass->name->chars);
			break;
		default:
			printf("Unknown Object type");
			break;
		}

		return;
	}

	switch (solisGetValueType(value))
	{
	case VALUE_NULL:
		printf("null");
		break;
	case VALUE_TRUE:
	case VALUE_FALSE:
		if (SOLIS_AS_BOOL(value))
			printf("true");
		else
			printf("false");
		break;
	case VALUE_NUMERIC:
		printf("%g", SOLIS_AS_NUMBER(value));
		break;
	default:
		printf("Unknown value type");
		break;
	}
}

