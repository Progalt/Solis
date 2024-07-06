
#ifndef SOLIS_COMMON_H
#define SOLIS_COMMON_H

#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>


#define SOLIS_MAJOR_VERSION 1
#define SOLIS_MINOR_VERSION 0
#define SOLIS_RELEASE_STRING "alpha"

// #define SOLIS_DEBUG_STRESS_GC
#define SOLIS_DEBUG_LOG_GC


#define UINT8_COUNT (UINT8_MAX + 1)

#if defined(_MSC_VER) && !defined(__clang__)
#define SOLIS_COMPUTED_GOTO 0
#else 
#define SOLIS_COMPUTED_GOTO 1 
#endif

// Some forward declares
typedef struct VM VM;
typedef struct Chunk Chunk;

typedef struct Object Object;
typedef struct ObjFiber ObjFiber;
typedef struct ObjFunction ObjFunction;
typedef struct ObjString ObjString;
typedef struct ObjClosure ObjClosure;
typedef struct ObjNative ObjNative;

typedef struct ObjClass ObjClass;
typedef struct ObjInstance ObjInstance;
typedef struct ObjEnum ObjEnum;
typedef struct ObjBoundMethod ObjBoundMethod;
typedef struct ObjList ObjList;

typedef struct ObjUserdata ObjUserdata;

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
    OBJ_FUNCTION,
    OBJ_STRING,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_NATIVE_FUNCTION, 
    OBJ_ENUM,
    OBJ_USERDATA,
    OBJ_CLASS, 
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD, 
    OBJ_LIST
} ObjectType;

typedef enum
{
    OPERATOR_ADD,
    OPERATOR_MINUS,
    OPERATOR_STAR,
    OPERATOR_SLASH, 
    OPERATOR_SLASH_SLASH,
    OPERATOR_POWER,
    OPERATOR_SUBSCRIPT_GET,
    OPERATOR_DOTDOT, 
    OPERATOR_SUBSCRIPT_SET,

    OPERATOR_COUNT
} Operators;


#ifndef SOLIS_REALLOC_FUNC
#define SOLIS_REALLOC_FUNC(ptr, newSize) realloc(ptr, newSize)
#endif

#ifndef SOLIS_FREE_FUNC
#define SOLIS_FREE_FUNC(ptr) free(ptr)
#endif

void* solisReallocate(VM* vm, void* ptr, size_t oldSize, size_t newSize);





#define SOLIS_ALLOCATE(vm, type, count) \
    (type*)solisReallocate(vm, NULL, 0, sizeof(type) * (count))

#define SOLIS_FREE(vm, type, ptr) solisReallocate(vm, ptr, sizeof(type), 0)

#define SOLIS_FREE_ARRAY(vm, type, pointer, oldCount) \
    solisReallocate(vm, pointer, sizeof(type) * (oldCount), 0)

#ifndef SOLIS_ASSERT
#define SOLIS_ASSERT(x) assert(x)
#endif


#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)


#define SOLIS_DECLARE_BUFFER(name, type)                                             \
    typedef struct                                                             \
    {                                                                          \
      type* data;                                                              \
      int count;                                                               \
      int capacity;                                                            \
    } name##Buffer;                                                            \
    void solis##name##BufferInit(VM* vm, name##Buffer* buffer);                         \
    void solis##name##BufferClear(VM* vm, name##Buffer* buffer);            \
    void solis##name##BufferFill(VM* vm, name##Buffer* buffer, type data,   \
                                int count);                                    \
    void solis##name##BufferWrite(VM* vm, name##Buffer* buffer, type data)

// This should be used once for each type instantiation, somewhere in a .c file.
#define SOLIS_DEFINE_BUFFER(name, type)                                              \
    void solis##name##BufferInit(VM* vm, name##Buffer* buffer)                          \
    {                                                                          \
      buffer->data = NULL;                                                     \
      buffer->capacity = 0;                                                    \
      buffer->count = 0;                                                       \
    }                                                                          \
                                                                               \
    void solis##name##BufferClear( VM* vm,name##Buffer* buffer)             \
    {                                                                          \
      solisReallocate(vm, buffer->data, 0, 0);                                  \
      solis##name##BufferInit(vm, buffer);                                          \
    }                                                                          \
                                                                               \
    void solis##name##BufferFill(VM* vm,name##Buffer* buffer, type data,   \
                                int count)                                     \
    {                                                                          \
      if (buffer->capacity < buffer->count + count)                            \
      {                                                                        \
        int capacity = buffer->count + count;                \
        buffer->data = (type*)solisReallocate(vm, buffer->data,                 \
            buffer->capacity * sizeof(type), capacity * sizeof(type));         \
        buffer->capacity = capacity;                                           \
      }                                                                        \
                                                                               \
      for (int i = 0; i < count; i++)                                          \
      {                                                                        \
        buffer->data[buffer->count++] = data;                                  \
      }                                                                        \
    }                                                                          \
                                                                               \
    void solis##name##BufferWrite(VM* vm,name##Buffer* buffer, type data)  \
    {                                                                          \
      solis##name##BufferFill(vm, buffer, data, 1);                             \
    }


#endif // SOLIS_COMMON_H