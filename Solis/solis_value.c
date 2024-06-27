
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
		default:
			printf("Unknown Object type");
			break;
		}

		return;
	}

	switch (value.type)
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
		default:
			printf("Unknown Object type");
			break;
		}

		return;
	}

	switch (value.type)
	{
	case VALUE_NULL:
		printf("null");
		break;
	case VALUE_TRUE:
		printf("true");
		break;
	case VALUE_FALSE:
		printf("false");
		break;
	case VALUE_NUMERIC:
		printf("%g", value.as.number);
		break;
	default:
		printf("Unknown value type");
		break;
	}
}

