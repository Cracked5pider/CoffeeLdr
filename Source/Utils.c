#include <Utils.h>

LPVOID LoadFileIntoMemory( LPSTR Path, PDWORD MemorySize )
{
    HANDLE  hFile       = NULL;
    LPVOID  ImageBuffer = NULL;
    DWORD   dwBytesRead = 0;

    hFile = CreateFileA( Path, GENERIC_READ, 0, 0, OPEN_ALWAYS, 0, 0 );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        printf("Error opening %s\r\n", Path);
        return NULL;
    }

    if ( MemorySize )
        *MemorySize = GetFileSize( hFile, 0 );
    ImageBuffer = (PBYTE)LocalAlloc( LPTR, *MemorySize );

    ReadFile( hFile, ImageBuffer, *MemorySize, &dwBytesRead, 0 );
    return ImageBuffer;
}

DWORD HashString( PVOID String, SIZE_T Length )
{
    ULONG  Hash = HASH_KEY;
    PUCHAR Ptr  = String;

    do
    {
        UCHAR character = *Ptr;

        if ( ! Length )
        {
            if ( !*Ptr ) break;
        }
        else
        {
            if ( (ULONG) ( Ptr - (PUCHAR)String ) >= Length ) break;
            if ( !*Ptr ) ++Ptr;
        }

        if ( character >= 'a' )
            character -= 0x20;

        Hash = ( ( Hash << 5 ) + Hash ) + character;
        ++Ptr;
    } while ( TRUE );

    return Hash;
}
