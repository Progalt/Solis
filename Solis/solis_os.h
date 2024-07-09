
#ifndef SOLIS_OS_H
#define SOLIS_OS_H

#ifdef _WIN32
#define SOLIS_WINDOWS
#define SOLIS_PLATFORM_STRING "Windows"
#endif

#ifdef __linux__
#define SOLIS_LINUX
#define SOLIS_PLATFORM_STRING "Linux"
#endif

#ifdef __APPLE__
#define SOLIS_APPLE
#define SOLIS_PLATFORM_STRING "Apple"
#endif

typedef void* LibraryHandle;

LibraryHandle solisOpenLibrary(const char* path);

void solisCloseLibrary(LibraryHandle handle);

void* solisGetProcAddress(LibraryHandle handle, const char* func);

#endif // SOLIS_OS_H