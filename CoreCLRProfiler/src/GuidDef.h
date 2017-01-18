#ifndef GUID_DEFINED
#define GUID_DEFINED
#if defined(__midl)
typedef struct
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    byte Data4[8];
} GUID;
#else
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif
#endif