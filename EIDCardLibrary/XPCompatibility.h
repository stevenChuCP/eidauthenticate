
#pragma comment(lib, "Delayimp.lib")

// Message from delayimp.h:
// Prior to Visual Studio 2015 Update 3, these hooks were non-const.  They were
// made const to improve security (global, writable function pointers are bad).
// If for backwards compatibility you require the hooks to be writable, define
// the macro DELAYIMP_INSECURE_WRITABLE_HOOKS prior to including this header and
// provide your own non-const definition of the hooks.
#ifndef DELAYIMP_INSECURE_WRITABLE_HOOKS
#define DELAYIMP_INSECURE_WRITABLE_HOOKS
#endif

#include <delayimp.h>
// include this file only once in a project to give the xp compatibity
// note : using delayload hook doesn't work on a library

extern "C"
{
	FARPROC WINAPI delayHookFailureFunc (unsigned dliNotify, PDelayLoadInfo pdli);
	PfnDliHook __pfnDliFailureHook2 = delayHookFailureFunc;
}

