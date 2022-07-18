#ifndef COFFEELDR_BEACONAPI_H
#define COFFEELDR_BEACONAPI_H

#include <windows.h>

#define COFFAPI_BEACONDATAPARSER                0xd0d30e22
#define COFFAPI_BEACONDATAINT                   0xff041492
#define COFFAPI_BEACONDATASHORT                 0xd10d2177
#define COFFAPI_BEACONDATALENGTH                0xe2262f89
#define COFFAPI_BEACONDATAEXTRACT               0x38d8c562

#define COFFAPI_BEACONFORMATALLOC               0x67aab721
#define COFFAPI_BEACONFORMATRESET               0x68da9d99
#define COFFAPI_BEACONFORMATFREE                0xf3a32998
#define COFFAPI_BEACONFORMATAPPEND              0x5d4c05ee
#define COFFAPI_BEACONFORMATPRINTF              0x8069e8c9
#define COFFAPI_BEACONFORMATTOSTRING            0x245f03f0
#define COFFAPI_BEACONFORMATINT                 0x2669d741

#define COFFAPI_BEACONPRINTF                    0x89bf3d20
#define COFFAPI_BEACONOUTPUT                    0x87a66ede
#define COFFAPI_BEACONUSETOKEN                  0xd7dbbb5b
#define COFFAPI_BEACONREVERTTOKEN               0xd7421e6
#define COFFAPI_BEACONISADMIN                   0xa88e0392
#define COFFAPI_BEACONGETSPAWNTO                0x32e13a39
#define COFFAPI_BEACONSPAWNTEMPORARYPROCESS     0xad80158
#define COFFAPI_BEACONINJECTPROCESS             0xe8f5bd09
#define COFFAPI_BEACONINJECTTEMPORARYPROCESS    0x96fbf28c
#define COFFAPI_BEACONCLEANUPPROCESS            0xa0dc954

#define COFFAPI_TOWIDECHAR                      0x5cec66cf
#define COFFAPI_LOADLIBRARYA                    0xb7072fdb
#define COFFAPI_GETPROCADDRESS                  0xdecfc1bf
#define COFFAPI_GETMODULEHANDLE                 0xd908e1d8
#define COFFAPI_FREELIBRARY                     0x4ad9b11c

typedef struct
{
    UINT_PTR    NameHash;
    PVOID       Pointer;
} COFFAPIFUNC;

extern COFFAPIFUNC BeaconApi[ ];
extern DWORD       BeaconApiCounter;

typedef struct {
    char * original; /* the original buffer [so we can free it] */
    char * buffer;   /* current pointer into our buffer */
    int    length;   /* remaining length of data */
    int    size;     /* total size of this buffer */
} datap;

typedef struct {
    char * original; /* the original buffer [so we can free it] */
    char * buffer;   /* current pointer into our buffer */
    int    length;   /* remaining length of data */
    int    size;     /* total size of this buffer */
} formatp;

void    BeaconDataParse(datap * parser, char * buffer, int size);
int     BeaconDataInt(datap * parser);
short   BeaconDataShort(datap * parser);
int     BeaconDataLength(datap * parser);
char *  BeaconDataExtract(datap * parser, int * size);

void    BeaconFormatAlloc(formatp * format, int maxsz);
void    BeaconFormatReset(formatp * format);
void    BeaconFormatFree(formatp * format);
void    BeaconFormatAppend(formatp * format, char * text, int len);
void    BeaconFormatPrintf(formatp * format, char * fmt, ...);
char *  BeaconFormatToString(formatp * format, int * size);
void    BeaconFormatInt(formatp * format, int value);

#define CALLBACK_OUTPUT      0x0
#define CALLBACK_OUTPUT_OEM  0x1e
#define CALLBACK_ERROR       0x0d
#define CALLBACK_OUTPUT_UTF8 0x20


void   BeaconPrintf(int type, char * fmt, ...);
void   BeaconOutput(int type, char * data, int len);

/* Token Functions */
BOOL   BeaconUseToken(HANDLE token);
void   BeaconRevertToken();
BOOL   BeaconIsAdmin();

/* Spawn+Inject Functions */
void   BeaconGetSpawnTo(BOOL x86, char * buffer, int length);
BOOL BeaconSpawnTemporaryProcess(BOOL x86, BOOL ignoreToken, STARTUPINFO * sInfo, PROCESS_INFORMATION * pInfo);
void   BeaconInjectProcess(HANDLE hProc, int pid, char * payload, int p_len, int p_offset, char * arg, int a_len);
void   BeaconInjectTemporaryProcess(PROCESS_INFORMATION * pInfo, char * payload, int p_len, int p_offset, char * arg, int a_len);
void   BeaconCleanupProcess(PROCESS_INFORMATION * pInfo);

/* Utility Functions */
BOOL   toWideChar(char * src, wchar_t * dst, int max);
UINT32 swap_endianess(UINT32 indata);

char* BeaconGetOutputData(int *outsize);

#endif
