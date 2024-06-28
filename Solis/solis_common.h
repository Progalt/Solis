
#ifndef SOLIS_COMMON_H
#define SOLIS_COMMON_H

#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>

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
} ObjectType;


#ifndef SOLIS_REALLOC_FUNC
#define SOLIS_REALLOC_FUNC(ptr, newSize) realloc(ptr, newSize)
#endif

#ifndef SOLIS_FREE_FUNC
#define SOLIS_FREE_FUNC(ptr) free(ptr)
#endif

static inline void* solisReallocate(void* ptr, size_t oldSize, size_t newSize)
{

    if (newSize == 0) {
        SOLIS_FREE_FUNC(ptr);
        return NULL;
    }

    void* result = SOLIS_REALLOC_FUNC(ptr, newSize);

    if (result == NULL) exit(1);

    return result;

}

#define SOLIS_ALLOCATE( type, count) \
    (type*)solisReallocate( NULL, 0, sizeof(type) * (count))

#define SOLIS_FREE(type, ptr) solisReallocate(ptr, sizeof(type), 0)

#define SOLIS_FREE_ARRAY(type, pointer, oldCount) \
    solisReallocate(pointer, sizeof(type) * (oldCount), 0)

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
    void solis##name##BufferInit(name##Buffer* buffer);                         \
    void solis##name##BufferClear(name##Buffer* buffer);            \
    void solis##name##BufferFill(name##Buffer* buffer, type data,   \
                                int count);                                    \
    void solis##name##BufferWrite( name##Buffer* buffer, type data)

// This should be used once for each type instantiation, somewhere in a .c file.
#define SOLIS_DEFINE_BUFFER(name, type)                                              \
    void solis##name##BufferInit(name##Buffer* buffer)                          \
    {                                                                          \
      buffer->data = NULL;                                                     \
      buffer->capacity = 0;                                                    \
      buffer->count = 0;                                                       \
    }                                                                          \
                                                                               \
    void solis##name##BufferClear(name##Buffer* buffer)             \
    {                                                                          \
      solisReallocate(buffer->data, 0, 0);                                  \
      solis##name##BufferInit(buffer);                                          \
    }                                                                          \
                                                                               \
    void solis##name##BufferFill(name##Buffer* buffer, type data,   \
                                int count)                                     \
    {                                                                          \
      if (buffer->capacity < buffer->count + count)                            \
      {                                                                        \
        int capacity = buffer->count + count;                \
        buffer->data = (type*)solisReallocate(buffer->data,                 \
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
    void solis##name##BufferWrite(name##Buffer* buffer, type data)  \
    {                                                                          \
      solis##name##BufferFill(buffer, data, 1);                             \
    }


#endif // SOLIS_COMMON_H