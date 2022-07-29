
#include <CoffeLdr.h>
#include <BeaconApi.h>
#include <Utils.h>

#if defined( __x86_64__ ) || defined( _WIN64 )

#define COFF_PREP_SYMBOL        0xec598a48
#define COFF_PREP_SYMBOL_SIZE   6

#define COFF_PREP_BEACON        0x353400b0
#define COFF_PREP_BEACON_SIZE   ( COFF_PREP_SYMBOL_SIZE + 6 )

#endif

PVOID CoffeeProcessSymbol( LPSTR Symbol )
{
    CHAR    Bak[ 1024 ] = { 0 };
    PVOID   FuncAddr    = NULL;
    PCHAR   SymLibrary  = NULL;
    PCHAR   SymFunction = NULL;
    HMODULE hLibrary    = NULL;
    DWORD   SymHash     = HashString( Symbol + COFF_PREP_SYMBOL_SIZE, 0 );
    DWORD   SymBeacon   = HashString( Symbol, COFF_PREP_BEACON_SIZE );

    memcpy( Bak, Symbol, strlen( Symbol ) + 1 );

    if (
        SymBeacon == COFF_PREP_BEACON         || // check if this is a Beacon api
        SymHash   == COFFAPI_TOWIDECHAR       ||
        SymHash   == COFFAPI_GETPROCADDRESS   ||
        SymHash   == COFFAPI_LOADLIBRARYA     ||
        SymHash   == COFFAPI_GETMODULEHANDLE  ||
        SymHash   == COFFAPI_FREELIBRARY
    )
    {
        SymFunction = Symbol + COFF_PREP_SYMBOL_SIZE;

        for ( DWORD i = 0; i < BeaconApiCounter; i++ )
        {
            if ( HashString( SymFunction, 0 ) == BeaconApi[ i ].NameHash )
                return BeaconApi[ i ].Pointer;
        }
    }
    else if ( HashString( Symbol, COFF_PREP_SYMBOL_SIZE ) == COFF_PREP_SYMBOL )
    {
        SymLibrary  = Bak + COFF_PREP_SYMBOL_SIZE;
        SymLibrary  = strtok( SymLibrary, "$" );
        SymFunction = SymLibrary + strlen( SymLibrary ) + 1;

        hLibrary = LoadLibraryA( SymLibrary );
        if ( ! hLibrary )
        {
            printf( "Failed to load library: Lib:[%s] Err:[%d]\n", SymLibrary, GetLastError() );
            return NULL;
        }
        FuncAddr = GetProcAddress( hLibrary, SymFunction );
    }
    else
    {
        puts( "Can't handle this function" );
        return FALSE;
    }

    if ( ! FuncAddr )
    {
        puts( "[!] Function address not found" );
        return NULL;
    }

    return FuncAddr;
}

BOOL CoffeeExecuteFunction( PCOFFEE Coffee, PCHAR Function, PVOID Argument, SIZE_T Size )
{
    typedef VOID ( *COFFEEMAIN ) ( PCHAR , ULONG );

    COFFEEMAIN  CoffeeMain    = NULL;
    DWORD       OldProtection = 0;
    BOOL        Success       = FALSE;

    for ( DWORD SymCounter = 0; SymCounter < Coffee->Header->NumberOfSymbols; SymCounter++ )
    {
        if ( strcmp( Coffee->Symbol[ SymCounter ].First.Name, Function ) == 0 )
        {
            Success = TRUE;

            // set the .text section to RX
            VirtualProtect( Coffee->SecMap[ 0 ].Ptr, Coffee->SecMap[ 0 ].Size, PAGE_EXECUTE_READ, &OldProtection );

            CoffeeMain = ( COFFEEMAIN ) ( Coffee->SecMap[ Coffee->Symbol[ SymCounter ].SectionNumber - 1 ].Ptr + Coffee->Symbol[ SymCounter ].Value );
            CoffeeMain( Argument, Size );
        }
    }

    if ( ! Success )
        printf( "[!] Couldn't find function => %s\n", Function );

    return Success;
}

BOOL CoffeeCleanup( PCOFFEE Coffee )
{
    DWORD OldProtection = 0;

    for ( DWORD SecCnt = 0; SecCnt < Coffee->Header->NumberOfSections; SecCnt++ )
    {
        if ( Coffee->SecMap[ SecCnt ].Ptr )
        {
            if ( ! VirtualProtect( Coffee->SecMap[ SecCnt ].Ptr, Coffee->SecMap[ SecCnt ].Size, PAGE_READWRITE, &OldProtection ) )
            {
                printf( "[!] Failed to change protection to RW" );
                return FALSE;
            }

            memset( Coffee->SecMap[ SecCnt ].Ptr, 0, Coffee->SecMap[ SecCnt ].Size );

            if ( ! VirtualFree( Coffee->SecMap[ SecCnt ].Ptr, 0, MEM_RELEASE ) )
                printf( "[!] Failed to free memory: %p : %d", Coffee->SecMap[ SecCnt ].Ptr, GetLastError() );

            Coffee->SecMap[ SecCnt ].Ptr = NULL;
        }
    }

    if ( Coffee->SecMap )
    {
        memset( Coffee->SecMap, 0, Coffee->Header->NumberOfSections * sizeof( SECTION_MAP ) );
        LocalFree( Coffee->SecMap );
        Coffee->SecMap = NULL;
    }

    if ( Coffee->FunMap )
    {
        memset( Coffee->FunMap, 0, 2048 );
        VirtualFree( Coffee->FunMap, 0, MEM_RELEASE );
        Coffee->FunMap = NULL;
    }
}

// Process sections relocation and symbols
BOOL CoffeeProcessSections( PCOFFEE Coffee )
{
    UINT32 Symbol     = 0;
    PVOID  SymString  = NULL;
    PCHAR  FuncPtr    = NULL;
    DWORD  FuncCount  = 0;
    UINT64 OffsetLong = 0;
    UINT32 Offset     = 0;

    for ( DWORD SectionCnt = 0; SectionCnt < Coffee->Header->NumberOfSections; SectionCnt++ )
    {
        Coffee->Section = U_PTR( Coffee->Data ) + sizeof( COFF_FILE_HEADER ) + U_PTR( sizeof( COFF_SECTION ) * SectionCnt );
        Coffee->Reloc   = U_PTR( Coffee->Data ) + Coffee->Section->PointerToRelocations;

        for ( DWORD RelocCnt = 0; RelocCnt < Coffee->Section->NumberOfRelocations; RelocCnt++ )
        {
            if ( Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].First.Name[ 0 ] != 0 )
            {
                Symbol = C_PTR( Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].First.Value[ 1 ] );

                if ( Coffee->Reloc->Type == IMAGE_REL_AMD64_ADDR64 )
                {
                    memcpy( &OffsetLong, Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, sizeof( UINT64 ) );

                    OffsetLong = ( UINT64 ) ( Coffee->SecMap[ Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].SectionNumber - 1 ].Ptr + ( UINT64 ) OffsetLong );

                    memcpy( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, &OffsetLong, sizeof( UINT64 ) );
                }
                else if ( Coffee->Reloc->Type == IMAGE_REL_AMD64_ADDR32NB )
                {
                    memcpy( &Offset, Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, sizeof( UINT32 ) );

                    if ( ( ( PCHAR ) ( Coffee->SecMap[ Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].SectionNumber - 1 ].Ptr + Offset ) - ( PCHAR ) ( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress + 4 ) ) > 0xffffffff )
                        return FALSE;

                    Offset = ( UINT32 ) ( ( PCHAR ) ( Coffee->SecMap[ Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].SectionNumber - 1 ].Ptr + Offset ) - ( PCHAR ) ( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress + 4 ) );

                    memcpy( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, &Offset, sizeof( UINT32 ) );
                }
                else if ( IMAGE_REL_AMD64_REL32 <= Coffee->Reloc->Type && Coffee->Reloc->Type <= IMAGE_REL_AMD64_REL32_5  )
                {
                    memcpy( &Offset, Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, sizeof( UINT32 ) );

                    if ( ( Coffee->SecMap[ Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].SectionNumber - 1 ].Ptr - ( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress + 4 ) ) > 0xffffffff )
                        return FALSE;

                    Offset += ( UINT32 ) ( Coffee->SecMap[ Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].SectionNumber - 1 ].Ptr - (Coffee->Reloc->Type - 4) - ( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress + 4 ) );

                    memcpy( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, &Offset, sizeof( UINT32 ) );
                }
                else
                    printf( "[!] Relocation type not found: %d\n", Coffee->Reloc->Type );

            }
            else
            {
                Symbol    = Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].First.Value[ 1 ];
                SymString = ( ( PCHAR ) ( Coffee->Symbol + Coffee->Header->NumberOfSymbols ) ) + Symbol;
                FuncPtr   = CoffeeProcessSymbol( SymString );

                if ( ! FuncPtr )
                {
                    puts( "FunctionPtr is empty: couldn't be resolved" );
                    return FALSE;
                }

                if ( Coffee->Reloc->Type == IMAGE_REL_AMD64_REL32 && FuncPtr != NULL )
                {
                    if ( ( ( Coffee->FunMap + ( FuncCount * 8 ) ) - ( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress + 4 ) ) > 0xffffffff )
                        return FALSE;

                    memcpy( Coffee->FunMap + ( FuncCount * 8 ), &FuncPtr, sizeof( UINT64 ) );
                    Offset = ( UINT32 ) ( ( Coffee->FunMap + ( FuncCount * 8 ) ) - ( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress + 4 ) );

                    memcpy( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, &Offset, sizeof( UINT32 ) );
                    FuncCount++;
                }
                else if ( Coffee->Reloc->Type == IMAGE_REL_AMD64_REL32 )
                {
                    memcpy( &Offset, Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, sizeof( UINT32 ) );

                    if ( ( Coffee->SecMap[ Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].SectionNumber - 1 ].Ptr - ( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress + 4 ) ) > 0xffffffff )
                        return FALSE;

                    Offset += ( UINT32 ) ( Coffee->SecMap[ Coffee->Symbol[ Coffee->Reloc->SymbolTableIndex ].SectionNumber - 1 ].Ptr - ( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress + 4 ) );

                    memcpy( Coffee->SecMap[ SectionCnt ].Ptr + Coffee->Reloc->VirtualAddress, &Offset, sizeof( UINT32 ) );
                }
                else
                    printf( "[!] Relocation type not found: %d\n", Coffee->Reloc->Type );

            }

            Coffee->Reloc = U_PTR( Coffee->Reloc ) + sizeof( COFF_RELOC );
        }
    }

    return TRUE;
}

DWORD CoffeeLdr( PCHAR EntryName, PVOID CoffeeData, PVOID ArgData, SIZE_T ArgSize )
{
    COFFEE Coffee = { 0 };

    if ( ! CoffeeData )
    {
        puts( "[!] Coffee data is empty" );
        return 1;
    }

    Coffee.Data   = CoffeeData;
    Coffee.Header = Coffee.Data;

    Coffee.SecMap = LocalAlloc( LPTR, Coffee.Header->NumberOfSections * sizeof( SECTION_MAP ) );
    Coffee.FunMap = VirtualAlloc( NULL, 2048, MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE );

    puts( "[*] Load sections" );
    for ( DWORD SecCnt = 0 ; SecCnt < Coffee.Header->NumberOfSections; SecCnt++ )
    {
        Coffee.Section               = U_PTR( Coffee.Data ) + sizeof( COFF_FILE_HEADER ) + U_PTR( sizeof( COFF_SECTION ) * SecCnt );
        Coffee.SecMap[ SecCnt ].Size = Coffee.Section->SizeOfRawData;
        Coffee.SecMap[ SecCnt ].Ptr  = VirtualAlloc( NULL, Coffee.SecMap[ SecCnt ].Size, MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE );

        memcpy( Coffee.SecMap[ SecCnt ].Ptr, U_PTR( CoffeeData ) + Coffee.Section->PointerToRawData, Coffee.Section->SizeOfRawData );
    }

    puts( "[*] Process sections" );
    Coffee.Symbol = U_PTR( Coffee.Data ) + Coffee.Header->PointerToSymbolTable;
    if ( ! CoffeeProcessSections( &Coffee ) )
    {
        puts( "[*] Failed to process relocation" );
        return 1;
    }

    puts( "[*] Execute coffee main\n" );
    CoffeeExecuteFunction( &Coffee, EntryName, ArgData, ArgSize );

    puts( "[*] Cleanup memory" );
    CoffeeCleanup( &Coffee );

    return 0;
}