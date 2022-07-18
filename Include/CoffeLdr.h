#ifndef COFFELDR_COFFELDR_H
#define COFFELDR_COFFELDR_H

#include <windows.h>
#include <stdio.h>

#define U_PTR( x ) ( ( UINT_PTR ) x )
#define C_PTR( x ) ( ( PVOID ) x )

typedef struct _COFF_FILE_HEADER 
{
    UINT16  Machine;
    UINT16  NumberOfSections;
    UINT32  TimeDateStamp;
    UINT32  PointerToSymbolTable;
    UINT32  NumberOfSymbols;
    UINT16  SizeOfOptionalHeader;
    UINT16  Characteristics;
} COFF_FILE_HEADER, *PCOFF_FILE_HEADER;

/* AMD64  should always be here */
#define MACHINETYPE_AMD64 0x8664

#pragma pack(push,1)

typedef struct _COFF_SECTION
{
    CHAR    Name[ 8 ];
    UINT32  VirtualSize;
    UINT32  VirtualAddress;
    UINT32  SizeOfRawData;
    UINT32  PointerToRawData;
    UINT32  PointerToRelocations;
    UINT32  PointerToLineNumbers;
    UINT16  NumberOfRelocations;
    UINT16  NumberOfLinenumbers;
    UINT32  Characteristics;
} COFF_SECTION, *PCOFF_SECTION;


typedef struct _COFF_RELOC
{
    UINT32  VirtualAddress;
    UINT32  SymbolTableIndex;
    UINT16  Type;
} COFF_RELOC, *PCOFF_RELOC;

typedef struct _COFF_SYMBOL
{
    union
    {
        CHAR    Name[8];
        UINT32  Value[2];
    } First;

    UINT32 Value;
    UINT16 SectionNumber;
    UINT16 Type;
    UINT8  StorageClass;
    UINT8  NumberOfAuxSymbols;
} COFF_SYMBOL, *PCOFF_SYMBOL;

typedef struct _SECTION_MAP
{
    PCHAR   Ptr;
    SIZE_T  Size;
} SECTION_MAP, *PSECTION_MAP;

typedef struct _COFFEE
{
    PVOID             Data;
    PCOFF_FILE_HEADER Header;
    PCOFF_SECTION     Section;
    PCOFF_RELOC       Reloc;
    PCOFF_SYMBOL      Symbol;

    PSECTION_MAP      SecMap;
    PCHAR             FunMap;
} COFFEE, *PCOFFEE;

DWORD CoffeeLdr( PCHAR EntryName, PVOID CoffeeData, PVOID ArgData, SIZE_T ArgSize );

#endif
