
#include "solis_interface.h"

#include "solis_vm.h"
#include <string.h>

Value solisGetArgument(VM* vm, int argIndex)
{
	return *(vm->apiStack + (argIndex + 1));
}

double solisCheckNumber(VM* vm, int argIndex)
{
	Value v = solisGetArgument(vm, argIndex);

	if (SOLIS_IS_NUMERIC(v))
	{
		return SOLIS_AS_NUMBER(v);
	}

	// TODO: Error check

	return 0.0;

}

const char* solisCheckString(VM* vm, int argIndex)
{
	Value v = solisGetArgument(vm, argIndex);

	if (SOLIS_IS_STRING(v))
	{
		return SOLIS_AS_CSTRING(v);
	}

	// TODO: Error check

	return NULL;
}

void solisSetReturnValue(VM* vm, Value value)
{
	// the return is the root of the api stack 
	*vm->apiStack = value;
}

Value solisGetSelf(VM* vm)
{
	return *vm->apiStack;
}

Value solisCreateEnumObject(VM* vm, const char* name)
{
	ObjEnum* _enum = solisNewEnum(vm);
	
	// Push as a global into the VM
	solisPushGlobal(vm, name, SOLIS_OBJECT_VALUE(_enum));

	// just return a new value with the ptr 
	return SOLIS_OBJECT_VALUE(_enum);

}

void solisBindEnumEntry(VM* vm, Value enumObj, const char* name)
{
	// return if we aren't an enum
	if (!SOLIS_IS_ENUM(enumObj))
		return;

	ObjEnum* _enum = SOLIS_AS_ENUM(enumObj);

	ObjString* nameStr = solisCopyString(vm, name, strlen(name));

	// Push onto stack so it exists somewhere the gc can see
	// As insert could cause a realloc 
	solisPush(vm, SOLIS_OBJECT_VALUE(name));

	solisHashTableInsert(&_enum->fields, nameStr, SOLIS_NUMERIC_VALUE((double)_enum->fieldCount));

	solisPop(vm);

	_enum->fieldCount++;
	
}

Value solisCreateClass(VM* vm, const char* name)
{
	ObjString* str = solisCopyString(vm, name, strlen(name));

	solisPush(vm, SOLIS_OBJECT_VALUE(str));
	ObjClass* klass = solisNewClass(vm, str);
	solisPop(vm);

	solisPushGlobal(vm, name, SOLIS_OBJECT_VALUE(klass));

	return SOLIS_OBJECT_VALUE(klass);
}

Value solisCreateClassInstance(VM* vm, const char* name, Value klass)
{

	ObjInstance* instance = solisNewInstance(vm, SOLIS_AS_CLASS(klass));

	solisPushGlobal(vm, name, SOLIS_OBJECT_VALUE(instance));

	return SOLIS_OBJECT_VALUE(instance);
}

void solisAddClassField(VM* vm, Value klassValue, const char* name, bool isStatic, Value defaultValue)
{
	ObjClass* klass = SOLIS_AS_CLASS(klassValue);

	ObjString* str = solisCopyString(vm, name, strlen(name));

	HashTable* table = &klass->fields;

	if (isStatic)
		table = &klass->statics;

	solisPush(vm, SOLIS_OBJECT_VALUE(str));
	solisHashTableInsert(table, str, defaultValue);
	solisPop(vm);

}

void solisSetStaticField(VM* vm, Value klassValue, const char* name, Value value)
{
	ObjClass* klass = NULL;

	if (SOLIS_IS_INSTANCE(klassValue))
		klass = SOLIS_AS_INSTANCE(klassValue)->klass;
	else if (SOLIS_IS_CLASS(klassValue))
		klass = SOLIS_AS_CLASS(klassValue);
	else
	{
		// TODO: Raise an error
		return;
	}

	ObjString* str = solisCopyString(vm, name, strlen(name));

	solisPush(vm, SOLIS_OBJECT_VALUE(str));

	if (solisHashTableInsert(&klass->statics, str, value))
	{
		// Delete it since we don't want to add on a set call
		solisHashTableDelete(&klass->statics, str);

		// TODO: We should raise an error here
	}

	solisPop(vm);

}

Value solisGetStaticField(VM* vm, Value klassValue, const char* name)
{
	ObjClass* klass = NULL;

	if (SOLIS_IS_INSTANCE(klassValue))
		klass = SOLIS_AS_INSTANCE(klassValue)->klass;
	else if (SOLIS_IS_CLASS(klassValue))
		klass = SOLIS_AS_CLASS(klassValue);
	else
	{
		// TODO: Raise an error
		return SOLIS_NULL_VALUE();
	}

	ObjString* str = solisCopyString(vm, name, strlen(name));

	solisPush(vm, SOLIS_OBJECT_VALUE(str));

	Value val;
	if (!solisHashTableGet(&klass->statics, str, &val))
	{
		return SOLIS_NULL_VALUE();
	}

	solisPop(vm);

	return val;
}

void solisSetInstanceField(VM* vm, Value instance, const char* name, Value value)
{
	if (!SOLIS_IS_INSTANCE(instance))
	{
		// TODO: Error 
		return;
	}

	ObjInstance* inst = SOLIS_AS_INSTANCE(instance);

	ObjString* str = solisCopyString(vm, name, strlen(name));

	solisPush(vm, SOLIS_OBJECT_VALUE(str));

	if (solisHashTableInsert(&inst->fields, str, value))
	{
		solisHashTableDelete(&inst->fields, str);
	}

	solisPop(vm);
}

Value solisGetInstanceField(VM* vm, Value instance, const char* name)
{
	if (!SOLIS_IS_INSTANCE(instance))
	{
		// TODO: Error 
		return SOLIS_NULL_VALUE();
	}

	ObjInstance* inst = SOLIS_AS_INSTANCE(instance);

	ObjString* str = solisCopyString(vm, name, strlen(name));

	solisPush(vm, SOLIS_OBJECT_VALUE(str));

	Value val;
	if (!solisHashTableGet(&inst->fields, str, &val))
	{
		return SOLIS_NULL_VALUE();
	}

	solisPop(vm);

	return val;
}

void solisAddClassNativeConstructor(VM* vm, Value klassValue, SolisNativeSignature func)
{
	ObjClass* klass = SOLIS_AS_CLASS(klassValue);
	
	assert(false && "Not implmented");
}

void solisAddClassNativeMethod(VM* vm, Value klassValue, const char* name, SolisNativeSignature func, int arity)
{
	ObjClass* klass = SOLIS_AS_CLASS(klassValue);
	ObjString* str = solisCopyString(vm, name, strlen(name));
	ObjNative* native = solisNewNativeFunction(vm, func);
	native->arity = arity;


	solisPush(vm, SOLIS_OBJECT_VALUE(str));
	solisPush(vm, SOLIS_OBJECT_VALUE(native));

	solisHashTableInsert(&klass->methods, str, SOLIS_OBJECT_VALUE(native));

	solisPop(vm);
	solisPop(vm);

}

void solisAddClassNativeStaticMethod(VM* vm, Value klassValue, const char* name, SolisNativeSignature func, int arity)
{
	ObjClass* klass = SOLIS_AS_CLASS(klassValue);
	ObjString* str = solisCopyString(vm, name, strlen(name));
	ObjNative* native = solisNewNativeFunction(vm, func);
	native->arity = arity;


	solisPush(vm, SOLIS_OBJECT_VALUE(str));
	solisPush(vm, SOLIS_OBJECT_VALUE(native));

	solisHashTableInsert(&klass->statics, str, SOLIS_OBJECT_VALUE(native));

	solisPop(vm);
	solisPop(vm);
}

void solisAddClassNativeOperator(VM* vm, Value klassValue, Operators op, SolisNativeSignature func)
{
	ObjClass* klass = SOLIS_AS_CLASS(klassValue);
	ObjNative* native = solisNewNativeFunction(vm, func);
	native->arity = 1;

	if (op == OPERATOR_SUBSCRIPT_SET)
		native->arity = 2;


	solisPush(vm, SOLIS_OBJECT_VALUE(native));

	//solisHashTableInsert(&klass->methods, vm->operatorStrings[op], SOLIS_OBJECT_VALUE(native));

	klass->operators[op] = (Object*)native;

	solisPop(vm);
}