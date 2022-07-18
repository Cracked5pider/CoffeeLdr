
#include <windows.h>
#include <stdio.h>

#define HASH_KEY 5381

LPVOID  LoadFileIntoMemory( LPSTR Path, PDWORD MemorySize );
DWORD   HashString( PVOID String, SIZE_T Length );