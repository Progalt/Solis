
#ifndef SOLIS_COMMON_H
#define SOLIS_COMMON_H

#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>

// Some forward declares
typedef struct VM VM;
typedef struct Chunk Chunk;

#ifndef SOLIS_REALLOC
#define SOLIS_REALLOC(ptr, newSize) realloc(ptr, newSize)
#endif

#ifndef SOLIS_FREE
#define SOLIS_FREE(ptr) free(ptr)
#endif

static inline void* solisReallocate(void* ptr, size_t oldSize, size_t newSize)
{

    if (newSize == 0) {
        SOLIS_FREE(ptr);
        return NULL;
    }

    void* result = SOLIS_REALLOC(ptr, newSize);

    if (result == NULL) exit(1);

    return result;

}

#define SOLIS_ALLOCATE(type, count) \
    (type*)solisReallocate(NULL, 0, sizeof(type) * (count))


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