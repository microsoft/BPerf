// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

//*****************************************************************************
// This code supports formatting a method and it's signature in a friendly
// and consistent format.
//
//*****************************************************************************

#include "profiler_pal.h"
#include "corhlpr.h"
#include "CQuickBytes.h"
#include "PortableString.h"
#define CONTRACTL
#define CONTRACTL_END
#define NOTHROW
#define INJECT_FAULT(x)
#define PREFIX_ASSUME(x)
#define NAMESPACE_SEPARATOR_STR "."
#define LPCUTF8 LPCSTR
#define IMDInternalImport IMetaDataImport
#ifndef NumItems
// Number of elements in a fixed-size array
#define NumItems(s) (sizeof(s) / sizeof(s[0]))
#endif

const mdToken g_tkCorEncodeToken[4] ={mdtTypeDef, mdtTypeRef, mdtTypeSpec, mdtBaseType};

//////////////////////////////////////////////////////////////////////////////
// enum CorElementTypeZapSig defines some additional internal ELEMENT_TYPE's
// values that are only used by ZapSig signatures.
//////////////////////////////////////////////////////////////////////////////
typedef enum CorElementTypeZapSig {
    // ZapSig encoding for ELEMENT_TYPE_VAR and ELEMENT_TYPE_MVAR. It is always followed
    // by the RID of a GenericParam token, encoded as a compressed integer.
    ELEMENT_TYPE_VAR_ZAPSIG = 0x3b,

    // ZapSig encoding for an array MethodTable to allow it to remain such after decoding
    // (rather than being transformed into the TypeHandle representing that array)
    //
    // The element is always followed by ELEMENT_TYPE_SZARRAY or ELEMENT_TYPE_ARRAY
    ELEMENT_TYPE_NATIVE_ARRAY_TEMPLATE_ZAPSIG = 0x3c,

    // ZapSig encoding for native value types in IL stubs. IL stub signatures may contain
    // ELEMENT_TYPE_INTERNAL followed by ParamTypeDesc with ELEMENT_TYPE_VALUETYPE element
    // type. It acts like a modifier to the underlying structure making it look like its
    // unmanaged view (size determined by unmanaged layout, blittable, no GC pointers).
    //
    // ELEMENT_TYPE_NATIVE_VALUETYPE_ZAPSIG is used when encoding such types to NGEN images.
    // The signature looks like this: ET_NATIVE_VALUETYPE_ZAPSIG ET_VALUETYPE <token>.
    // See code:ZapSig.GetSignatureForTypeHandle and code:SigPointer.GetTypeHandleThrowing
    // where the encoding/decoding takes place.
    ELEMENT_TYPE_NATIVE_VALUETYPE_ZAPSIG = 0x3d,

    ELEMENT_TYPE_CANON_ZAPSIG = 0x3e,  // zapsig encoding for [mscorlib]System.__Canon
    ELEMENT_TYPE_MODULE_ZAPSIG = 0x3f, // zapsig encoding for external module id#

} CorElementTypeZapSig;

HRESULT GetNameOfTypeDefOrRef(IMetaDataTables *metadataTablesImport, ULONG ixTbl, mdTypeRef tk, LPCSTR *psznamespace, LPCSTR *pszname)
{
    HRESULT hr;
    ULONG pVal;

    *psznamespace = nullptr;
    *pszname = nullptr;

    auto rid = RidFromToken(tk);

    pVal = 0;
    IfFailRet(metadataTablesImport->GetColumn(ixTbl, 1, rid, &pVal));

    const char *namePtr;
    IfFailRet(metadataTablesImport->GetString(pVal, &namePtr));

    *pszname = namePtr;

    pVal = 0;
    IfFailRet(metadataTablesImport->GetColumn(ixTbl, 2, rid, &pVal));

    const char *namespacePtr;
    IfFailRet(metadataTablesImport->GetString(pVal, &namespacePtr));

    *psznamespace = namespacePtr;

    return hr;
}

HRESULT GetNameOfTypeRef(IMetaDataTables *metadataTablesImport, mdTypeRef tk, LPCSTR *psznamespace, LPCSTR *pszname)
{
    return GetNameOfTypeDefOrRef(metadataTablesImport, 1, tk, psznamespace, pszname);
}

HRESULT GetNameOfTypeDef(IMetaDataTables *metadataTablesImport, mdTypeRef tk, LPCSTR *psznamespace, LPCSTR *pszname)
{
    return GetNameOfTypeDefOrRef(metadataTablesImport, 2, tk, psznamespace, pszname);
}


/***********************************************************************/
// Null-terminates the string held in "out"

static WCHAR* asStringW(CQuickBytes *out) 
{
    CONTRACTL
    {
        NOTHROW;
        INJECT_FAULT(return NULL;);
    }
    CONTRACTL_END

    SIZE_T oldSize = out->Size();
    if (FAILED(out->ReSizeNoThrow(oldSize + 1)))
        return 0;
    WCHAR * cur = (WCHAR *) ((BYTE *) out->Ptr() + oldSize);
    *cur = 0;
    return((WCHAR*) out->Ptr()); 
} // static WCHAR* asStringW()

// Null-terminates the string held in "out"

/***********************************************************************/
// Appends the str to "out"
// The string held in "out" is not NULL-terminated. asStringW() needs to
// be called for the NULL-termination

static HRESULT appendStrW(CQuickBytes *out, const WCHAR* str) 
{
    CONTRACTL
    {
        NOTHROW;
        INJECT_FAULT(return E_OUTOFMEMORY;);
    }
    CONTRACTL_END

    SIZE_T nChar = 0;
    if (!str)
    {
        nChar = 0;
    }
    else
    {
        while (*str++)
        {
            nChar++;
        }
    }

    SIZE_T len = nChar * sizeof(WCHAR);
    SIZE_T oldSize = out->Size();
    if (FAILED(out->ReSizeNoThrow(oldSize + len)))
        return E_OUTOFMEMORY;
    WCHAR * cur = (WCHAR *) ((BYTE *) out->Ptr() + oldSize);
    memcpy(cur, str, len);
    // Note no trailing null!
    return S_OK;
} // static HRESULT appendStrW()

// Appends the str to "out"
// The string held in "out" is not NULL-terminated. asStringA() needs to
// be called for the NULL-termination

static HRESULT appendStrNumW(CQuickBytes *out, int num) 
{
    CONTRACTL
    {
        NOTHROW;
        INJECT_FAULT(return E_OUTOFMEMORY;);
    }
    CONTRACTL_END

    WCHAR buff[32] = { '\0' };
    // swprintf_s(buff, 32, W("%d"), num); // TODO: muks 2019/07/25 we should fix this (used only for MVAR, VAR, etc.)
    return appendStrW(out, buff);
} // static HRESULT appendStrNumW()

static HRESULT appendStrHexW(CQuickBytes *out, int num) 
{
    CONTRACTL
    {
        NOTHROW;
        INJECT_FAULT(return E_OUTOFMEMORY;);
    }
    CONTRACTL_END

    WCHAR buff[32] = { '\0' };
    // swprintf_s(buff, 32, W("%08X"), num); // TODO: muks 2019/07/25 we should fix this (used only for MVAR, VAR, etc.)
    return appendStrW(out, buff);
} // static HRESULT appendStrHexW()

/***********************************************************************/

LPCWSTR PrettyPrintSigWorker(
               PCCOR_SIGNATURE &  typePtr,      // type to convert,     
               size_t             typeLen,      // length of type
               const WCHAR      * name,         // can be "", the name of the method for this sig       
               CQuickBytes *      out,          // where to put the pretty printed string       
               IMetaDataImport *  pIMDI);       // Import api to use.

//*****************************************************************************
//*****************************************************************************
// pretty prints 'type' to the buffer 'out' returns a pointer to the next type, 
// or 0 on a format failure 

static PCCOR_SIGNATURE PrettyPrintType(
               PCCOR_SIGNATURE    typePtr,      // type to convert,     
               size_t             typeLen,      // Maximum length of the type
               CQuickBytes *      out,          // where to put the pretty printed string       
               IMetaDataImport *  pIMDI)        // ptr to IMDInternal class with ComSig
{
    mdToken             tk;     
    const WCHAR *       str;    
    WCHAR               rcname[MAX_CLASS_NAME];
    HRESULT             hr;
    unsigned __int8     elt = *typePtr++;
    PCCOR_SIGNATURE     typeEnd = typePtr + typeLen;

    switch(elt) 
    {
    case ELEMENT_TYPE_VOID:
        str = W("void");
        goto APPEND;
        
    case ELEMENT_TYPE_BOOLEAN:
        str = W("bool");
        goto APPEND;
        
    case ELEMENT_TYPE_CHAR:
        str = W("wchar");
        goto APPEND; 
        
    case ELEMENT_TYPE_I1:
        str = W("int8");
        goto APPEND;
        
    case ELEMENT_TYPE_U1:
        str = W("unsigned int8");
        goto APPEND;
        
    case ELEMENT_TYPE_I2:
        str = W("int16");
        goto APPEND;
        
    case ELEMENT_TYPE_U2:
        str = W("unsigned int16");
        goto APPEND;
        
    case ELEMENT_TYPE_I4:
        str = W("int32");
        goto APPEND;
        
    case ELEMENT_TYPE_U4:
        str = W("unsigned int32");
        goto APPEND;
        
    case ELEMENT_TYPE_I8:
        str = W("int64");
        goto APPEND;
        
    case ELEMENT_TYPE_U8:
        str = W("unsigned int64");
        goto APPEND;
        
    case ELEMENT_TYPE_R4:
        str = W("float32");
        goto APPEND;
        
    case ELEMENT_TYPE_R8:
        str = W("float64");
        goto APPEND;
        
    case ELEMENT_TYPE_U:
        str = W("unsigned int");
        goto APPEND;
        
    case ELEMENT_TYPE_I:
        str = W("int");
        goto APPEND;
        
    case ELEMENT_TYPE_OBJECT:
        str = W("class System.Object");
        goto APPEND;
        
    case ELEMENT_TYPE_STRING:
        str = W("class System.String");
        goto APPEND;
        
    case ELEMENT_TYPE_CANON_ZAPSIG:
        str = W("class System.__Canon");
        goto APPEND;
        
    case ELEMENT_TYPE_TYPEDBYREF:
        str = W("refany");
        goto APPEND;
        
    APPEND: 
        appendStrW(out, str);
        break;

    case ELEMENT_TYPE_VALUETYPE:
        str = W("value class ");
        goto DO_CLASS;
        
    case ELEMENT_TYPE_CLASS:
        str = W("class "); 
        goto DO_CLASS;
        
    DO_CLASS:
        typePtr += CorSigUncompressToken(typePtr, &tk);
        appendStrW(out, str);
        rcname[0] = 0;
        str = rcname;
        
        if (TypeFromToken(tk) == mdtTypeRef)
        {
            hr = pIMDI->GetTypeRefProps(tk, 0, rcname, NumItems(rcname), 0);
        }
        else if (TypeFromToken(tk) == mdtTypeDef)
        {
            hr = pIMDI->GetTypeDefProps(tk, rcname, NumItems(rcname), 0, 0, 0);
        }
        else
        {
            _ASSERTE(!"Unknown token type encountered in signature.");
            str = W("<UNKNOWN>");
        }
        
        appendStrW(out, str);
        break;
        
    case ELEMENT_TYPE_SZARRAY:
        typePtr = PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI); 
        appendStrW(out, W("[]"));
        break;
    
    case ELEMENT_TYPE_ARRAY:
        {
            typePtr = PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI);
            unsigned rank = CorSigUncompressData(typePtr);
            PREFIX_ASSUME(rank <= 0xffffff);
            
            // <TODO>TODO what is the syntax for the rank 0 case? </TODO>
            if (rank == 0)
            {
                appendStrW(out, W("[??]"));
            }
            else 
            {
                _ASSERTE(rank != 0);
                int* lowerBounds = (int*) _alloca(sizeof(int)*2*rank);
                int* sizes               = &lowerBounds[rank];
                memset(lowerBounds, 0, sizeof(int)*2*rank); 
                
                unsigned numSizes = CorSigUncompressData(typePtr);
                _ASSERTE(numSizes <= rank);
                unsigned int i;
                for(i =0; i < numSizes; i++)
                    sizes[i] = CorSigUncompressData(typePtr);
                
                unsigned numLowBounds = CorSigUncompressData(typePtr);
                _ASSERTE(numLowBounds <= rank); 
                for(i = 0; i < numLowBounds; i++)       
                    lowerBounds[i] = CorSigUncompressData(typePtr); 
                
                appendStrW(out, W("["));
                for(i = 0; i < rank; i++)       
                {       
                    if (sizes[i] != 0 && lowerBounds[i] != 0)   
                    {   
                        if (lowerBounds[i] == 0)        
                            appendStrNumW(out, sizes[i]);
                        else    
                        {       
                            appendStrNumW(out, lowerBounds[i]);
                            appendStrW(out, W("..."));
                            if (sizes[i] != 0)  
                                appendStrNumW(out, lowerBounds[i] + sizes[i] + 1);
                        }       
                    }   
                    if (i < rank-1) 
                        appendStrW(out, W(","));
                }       
                appendStrW(out, W("]"));  
            }
        }
        break;
        
    case ELEMENT_TYPE_MVAR:
        appendStrW(out, W("!!"));
        appendStrNumW(out, CorSigUncompressData(typePtr));
        break;
        
    case ELEMENT_TYPE_VAR:   
        appendStrW(out, W("!"));  
        appendStrNumW(out, CorSigUncompressData(typePtr));
        break;
        
    case ELEMENT_TYPE_GENERICINST:
        {
            typePtr = PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI); 
            unsigned ntypars = CorSigUncompressData(typePtr);
            appendStrW(out, W("<"));
            for (unsigned i = 0; i < ntypars; i++)
            {
                if (i > 0)
                    appendStrW(out, W(","));
                typePtr = PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI); 
            }
            appendStrW(out, W(">"));
        }
        break;
        
    case ELEMENT_TYPE_MODULE_ZAPSIG:
        appendStrW(out, W("[module#"));
        appendStrNumW(out, CorSigUncompressData(typePtr));
        appendStrW(out, W(", token#"));
        typePtr += CorSigUncompressToken(typePtr, &tk); 
        appendStrHexW(out, tk);
        appendStrW(out, W("]"));
        break;
            
    case ELEMENT_TYPE_FNPTR:
        appendStrW(out, W("fnptr "));
        PrettyPrintSigWorker(typePtr, (typeEnd - typePtr), W(""), out, pIMDI);
        break;

    case ELEMENT_TYPE_NATIVE_ARRAY_TEMPLATE_ZAPSIG:
    case ELEMENT_TYPE_NATIVE_VALUETYPE_ZAPSIG:
        appendStrW(out, W("native "));
        typePtr = PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI); 
        break;

    // Modifiers or depedant types  
    case ELEMENT_TYPE_PINNED:
        str = W(" pinned");
        goto MODIFIER;
        
    case ELEMENT_TYPE_PTR:
        str = W("*");
        goto MODIFIER;
        
    case ELEMENT_TYPE_BYREF:   
        str = W("&");
        goto MODIFIER;

    MODIFIER:
        typePtr = PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI); 
        appendStrW(out, str);
        break;
        
    default:
    case ELEMENT_TYPE_SENTINEL:
    case ELEMENT_TYPE_END:
        _ASSERTE(!"Unknown Type");
        return(typePtr);
        break;
    }
    return(typePtr);
} // static PCCOR_SIGNATURE PrettyPrintType()

//*****************************************************************************
// Converts a com signature to a text signature.
//
// Note that this function DOES NULL terminate the result signature string.
//*****************************************************************************
LPCWSTR PrettyPrintSigLegacy(
        PCCOR_SIGNATURE   typePtr,      // type to convert,     
        unsigned          typeLen,      // length of type
        const WCHAR     * name,         // can be "", the name of the method for this sig       
        CQuickBytes     * out,          // where to put the pretty printed string       
        IMetaDataImport * pIMDI)        // Import api to use.
{
    return PrettyPrintSigWorker(typePtr, typeLen, name, out, pIMDI);
} // LPCWSTR PrettyPrintSigLegacy()

LPCWSTR PrettyPrintSigWorker(
        PCCOR_SIGNATURE & typePtr,      // type to convert,     
        size_t            typeLen,      // length of type
        const WCHAR     * name,         // can be "", the name of the method for this sig       
        CQuickBytes     * out,          // where to put the pretty printed string       
        IMetaDataImport * pIMDI)        // Import api to use.
{
    out->Shrink(0); 
    unsigned numTyArgs = 0;
    unsigned numArgs;
    PCCOR_SIGNATURE typeEnd = typePtr + typeLen;    // End of the signature.

    if (name != 0)                      // 0 means a local var sig
    {
        // get the calling convention out
        unsigned callConv = CorSigUncompressData(typePtr);
        
        if (isCallConv(callConv, IMAGE_CEE_CS_CALLCONV_FIELD))
        {
            PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI);
            if (name != 0 && *name != 0)
            {
                appendStrW(out, W(" "));
                appendStrW(out, name);
            }
            return(asStringW(out));
        }
        
        if (callConv & IMAGE_CEE_CS_CALLCONV_HASTHIS)
            appendStrW(out, W("instance "));
        
        if (callConv & IMAGE_CEE_CS_CALLCONV_GENERIC)
        {
            appendStrW(out, W("generic "));
            numTyArgs = CorSigUncompressData(typePtr);
        }
        
        static const WCHAR * const callConvNames[IMAGE_CEE_CS_CALLCONV_MAX] = 
        {
            W(""), 
            W("unmanaged cdecl "), 
            W("unmanaged stdcall "),
            W("unmanaged thiscall "),
            W("unmanaged fastcall "),
            W("vararg "),
            W("<error> "),
            W("<error> "),
            W(""),
            W(""),
            W(""),
            W("native vararg ")
        };

        if ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) < IMAGE_CEE_CS_CALLCONV_MAX)
        {
        appendStrW(out, callConvNames[callConv & IMAGE_CEE_CS_CALLCONV_MASK]);
        }

        
        numArgs = CorSigUncompressData(typePtr);
        // do return type
        typePtr = PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI); 
        
    }
    else
    {
        numArgs = CorSigUncompressData(typePtr);
    }
    
    if (name != 0 && *name != 0)
    {
        appendStrW(out, W(" "));
        appendStrW(out, name);
    }
    appendStrW(out, W("("));
    
    bool needComma = false;
    
    while (numArgs)
    {
        if (typePtr >= typeEnd) 
            break;

        if (*typePtr == ELEMENT_TYPE_SENTINEL) 
        {
            if (needComma)
                appendStrW(out, W(","));
            appendStrW(out, W("..."));  
            typePtr++;
        }
        else 
        {
            if (numArgs <= 0)
                break;
            if (needComma)
                appendStrW(out, W(","));
            typePtr = PrettyPrintType(typePtr, (typeEnd - typePtr), out, pIMDI); 
            --numArgs;
        }
        needComma = true;
    }
    appendStrW(out, W(")"));
    return (asStringW(out));
} // LPCWSTR PrettyPrintSigWorker()
