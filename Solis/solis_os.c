
#include "solis_os.h"


#ifdef SOLIS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

LibraryHandle solisOpenLibrary(const char* path)
{
	HINSTANCE dll = LoadLibrary(path);

	if (!dll)
	{
		return NULL;
	}

	return (LibraryHandle*)dll;
}

void solisCloseLibrary(LibraryHandle handle)
{
	// Windows doesn't need this
}

void* solisGetProcAddress(LibraryHandle handle, const char* func)
{
	return (void*)GetProcAddress((HMODULE)handle, func);
}

#endif