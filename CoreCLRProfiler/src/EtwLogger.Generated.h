// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once
#include "BPerfProfiler.h"

std::mutex RundownMutex;

BPerfProfiler* ProfilerInstance;

#ifdef _WINDOWS

#include <evntrace.h>
#include <evntcons.h>

//
//  Initial Defs
//
#if !defined(ETW_INLINE)
#define ETW_INLINE DECLSPEC_NOINLINE __inline
#endif

#if defined(__cplusplus)
extern "C" {
#endif

    //
    // Allow Diasabling of code generation
    //
#ifndef MCGEN_DISABLE_PROVIDER_CODE_GENERATION
#if  !defined(McGenDebug)
#define McGenDebug(a,b)
#endif 


#if !defined(MCGEN_TRACE_CONTEXT_DEF)
#define MCGEN_TRACE_CONTEXT_DEF
    typedef struct _MCGEN_TRACE_CONTEXT
    {
        TRACEHANDLE            RegistrationHandle;
        TRACEHANDLE            Logger;
        ULONGLONG              MatchAnyKeyword;
        ULONGLONG              MatchAllKeyword;
        ULONG                  Flags;
        ULONG                  IsEnabled;
        UCHAR                  Level;
        UCHAR                  Reserve;
        USHORT                 EnableBitsCount;
        PULONG                 EnableBitMask;
        const ULONGLONG*       EnableKeyWords;
        const UCHAR*           EnableLevel;
    } MCGEN_TRACE_CONTEXT, *PMCGEN_TRACE_CONTEXT;
#endif

#if !defined(MCGEN_LEVEL_KEYWORD_ENABLED_DEF)
#define MCGEN_LEVEL_KEYWORD_ENABLED_DEF
    FORCEINLINE
        BOOLEAN
        McGenLevelKeywordEnabled(
            _In_ PMCGEN_TRACE_CONTEXT EnableInfo,
            _In_ UCHAR Level,
            _In_ ULONGLONG Keyword
        )
    {
        //
        // Check if the event Level is lower than the level at which
        // the channel is enabled.
        // If the event Level is 0 or the channel is enabled at level 0,
        // all levels are enabled.
        //

        if ((Level <= EnableInfo->Level) || // This also covers the case of Level == 0.
            (EnableInfo->Level == 0)) {

            //
            // Check if Keyword is enabled
            //

            if ((Keyword == (ULONGLONG)0) ||
                ((Keyword & EnableInfo->MatchAnyKeyword) &&
                ((Keyword & EnableInfo->MatchAllKeyword) == EnableInfo->MatchAllKeyword))) {
                return TRUE;
            }
        }

        return FALSE;

    }
#endif

#if !defined(MCGEN_EVENT_ENABLED_DEF)
#define MCGEN_EVENT_ENABLED_DEF
    FORCEINLINE
        BOOLEAN
        McGenEventEnabled(
            _In_ PMCGEN_TRACE_CONTEXT EnableInfo,
            _In_ PCEVENT_DESCRIPTOR EventDescriptor
        )
    {

        return McGenLevelKeywordEnabled(EnableInfo, EventDescriptor->Level, EventDescriptor->Keyword);

    }
#endif


    //
    // EnableCheckMacro
    //
#ifndef MCGEN_ENABLE_CHECK
#define MCGEN_ENABLE_CHECK(Context, Descriptor) (Context.IsEnabled &&  McGenEventEnabled(&Context, &Descriptor))
#endif

#if !defined(MCGEN_CONTROL_CALLBACK)
#define MCGEN_CONTROL_CALLBACK

    DECLSPEC_NOINLINE __inline
        VOID
        __stdcall
        McGenControlCallbackV2(
            _In_ LPCGUID SourceId,
            _In_ ULONG ControlCode,
            _In_ UCHAR Level,
            _In_ ULONGLONG MatchAnyKeyword,
            _In_ ULONGLONG MatchAllKeyword,
            _In_opt_ PEVENT_FILTER_DESCRIPTOR FilterData,
            _Inout_opt_ PVOID CallbackContext
        )
        /*++

        Routine Description:

        This is the notification callback for Vista.

        Arguments:

        SourceId - The GUID that identifies the session that enabled the provider.

        ControlCode - The parameter indicates whether the provider
        is being enabled or disabled.

        Level - The level at which the event is enabled.

        MatchAnyKeyword - The bitmask of keywords that the provider uses to
        determine the category of events that it writes.

        MatchAllKeyword - This bitmask additionally restricts the category
        of events that the provider writes.

        FilterData - The provider-defined data.

        CallbackContext - The context of the callback that is defined when the provider
        called EtwRegister to register itself.

        Remarks:

        ETW calls this function to notify provider of enable/disable

        --*/
    {
        PMCGEN_TRACE_CONTEXT Ctx = (PMCGEN_TRACE_CONTEXT)CallbackContext;
        ULONG Ix;
#ifndef MCGEN_PRIVATE_ENABLE_CALLBACK_V2
        UNREFERENCED_PARAMETER(SourceId);
        UNREFERENCED_PARAMETER(FilterData);
#endif

        if (Ctx == NULL) {
            return;
        }

        switch (ControlCode) {

        case EVENT_CONTROL_CODE_ENABLE_PROVIDER:
            Ctx->Level = Level;
            Ctx->MatchAnyKeyword = MatchAnyKeyword;
            Ctx->MatchAllKeyword = MatchAllKeyword;
            Ctx->IsEnabled = EVENT_CONTROL_CODE_ENABLE_PROVIDER;

            for (Ix = 0; Ix < Ctx->EnableBitsCount; Ix += 1) {
                if (McGenLevelKeywordEnabled(Ctx, Ctx->EnableLevel[Ix], Ctx->EnableKeyWords[Ix]) != FALSE) {
                    Ctx->EnableBitMask[Ix >> 5] |= (1 << (Ix % 32));
                }
                else {
                    Ctx->EnableBitMask[Ix >> 5] &= ~(1 << (Ix % 32));
                }
            }

            {
                std::lock_guard<std::mutex> lock(RundownMutex);
                if (ProfilerInstance != nullptr)
                {
                    ProfilerInstance->Rundown();
                }
            }

            break;
        case EVENT_CONTROL_CODE_DISABLE_PROVIDER:
            Ctx->IsEnabled = EVENT_CONTROL_CODE_DISABLE_PROVIDER;
            Ctx->Level = 0;
            Ctx->MatchAnyKeyword = 0;
            Ctx->MatchAllKeyword = 0;
            if (Ctx->EnableBitsCount > 0) {
                RtlZeroMemory(Ctx->EnableBitMask, (((Ctx->EnableBitsCount - 1) / 32) + 1) * sizeof(ULONG));
            }
            break;

        default:
            break;
        }

#ifdef MCGEN_PRIVATE_ENABLE_CALLBACK_V2
        //
        // Call user defined callback
        //
        MCGEN_PRIVATE_ENABLE_CALLBACK_V2(
            SourceId,
            ControlCode,
            Level,
            MatchAnyKeyword,
            MatchAllKeyword,
            FilterData,
            CallbackContext
        );
#endif

        return;
    }

#endif
#endif // MCGEN_DISABLE_PROVIDER_CODE_GENERATION
    //+
    // Provider Microsoft-Windows-DotNETRuntimeRundown Event Count 45
    //+
    EXTERN_C __declspec(selectany) const GUID MICROSOFT_WINDOWS_DOTNETRUNTIME_RUNDOWN_PROVIDER = { 0xd46e5565, 0xd624, 0x4c4d,{ 0x89, 0xcc, 0x7a, 0x82, 0x88, 0x7d, 0x36, 0x26 } };

    //
    // Opcodes
    //
#define CLR_METHODDC_METHODDCSTART_OPCODE 0x23
#define CLR_METHODDC_METHODDCEND_OPCODE 0x24
#define CLR_METHODDC_METHODDCSTARTVERBOSE_OPCODE 0x27
#define CLR_METHODDC_METHODDCENDVERBOSE_OPCODE 0x28
#define CLR_METHODDC_METHODDCSTARTILTONATIVEMAP_OPCODE 0x29
#define CLR_METHODDC_METHODDCENDILTONATIVEMAP_OPCODE 0x2a
#define CLR_METHODDC_DCSTARTCOMPLETE_OPCODE 0xe
#define CLR_METHODDC_DCENDCOMPLETE_OPCODE 0xf
#define CLR_METHODDC_DCSTARTINIT_OPCODE 0x10
#define CLR_METHODDC_DCENDINIT_OPCODE 0x11
#define CLR_LOADERDC_MODULEDCSTART_OPCODE 0x23
#define CLR_LOADERDC_MODULEDCEND_OPCODE 0x24
#define CLR_LOADERDC_ASSEMBLYDCSTART_OPCODE 0x27
#define CLR_LOADERDC_ASSEMBLYDCEND_OPCODE 0x28
#define CLR_LOADERDC_APPDOMAINDCSTART_OPCODE 0x2b
#define CLR_LOADERDC_APPDOMAINDCEND_OPCODE 0x2c
#define CLR_LOADERDC_DOMAINMODULEDCSTART_OPCODE 0x2e
#define CLR_LOADERDC_DOMAINMODULEDCEND_OPCODE 0x2f
#define CLR_LOADERDC_THREADDC_OPCODE 0x30
#define CLR_RUNDOWNSTACK_STACKWALK_OPCODE 0x52
#define CLR_PERFTRACKRUNDOWN_MODULERANGEDCSTART_OPCODE 0xa
#define CLR_PERFTRACKRUNDOWN_MODULERANGEDCEND_OPCODE 0xb

    //
    // Tasks
    //
#define CLR_METHODRUNDOWN_TASK 0x1
    EXTERN_C __declspec(selectany) const GUID CLRMethodRundownId = { 0x0bcd91db, 0xf943, 0x454a,{ 0xa6, 0x62, 0x6e, 0xdb, 0xcf, 0xbb, 0x76, 0xd2 } };
#define CLR_LOADERRUNDOWN_TASK 0x2
    EXTERN_C __declspec(selectany) const GUID CLRLoaderRundownId = { 0x5a54f4df, 0xd302, 0x4fee,{ 0xa2, 0x11, 0x6c, 0x2c, 0x0c, 0x1d, 0xcb, 0x1a } };
#define CLR_STACKRUNDOWN_TASK 0xb
    EXTERN_C __declspec(selectany) const GUID CLRStackRundownId = { 0xd3363dc0, 0x243a, 0x4620,{ 0xa4, 0xd0, 0x8a, 0x07, 0xd7, 0x72, 0xf5, 0x33 } };
#define CLR_RuntimeInformation_TASK 0x13
    EXTERN_C __declspec(selectany) const GUID CLRRuntimeInformationRundownId = { 0xcd7d3e32, 0x65fe, 0x40cd,{ 0x92, 0x25, 0xa2, 0x57, 0x7d, 0x20, 0x3f, 0xc3 } };
#define CLR_PERFTRACKRUNDOWN_TASK 0x14
    EXTERN_C __declspec(selectany) const GUID CLRPerfTrackRundownId = { 0xeac685f6, 0x2104, 0x4dec,{ 0x88, 0xfd, 0x91, 0xe4, 0x25, 0x42, 0x21, 0xec } };
    //
    // Keyword
    //
#define CLR_RUNDOWNLOADER_KEYWORD 0x8
#define CLR_RUNDOWNJIT_KEYWORD 0x10
#define CLR_RUNDOWNNGEN_KEYWORD 0x20
#define CLR_RUNDOWNSTART_KEYWORD 0x40
#define CLR_RUNDOWNEND_KEYWORD 0x100
#define CLR_RUNDOWNAPPDOMAINRESOURCEMANAGEMENT_KEYWORD 0x800
#define CLR_RUNDOWNTHREADING_KEYWORD 0x10000
#define CLR_RUNDOWNJITTEDMETHODILTONATIVEMAP_KEYWORD 0x20000
#define CLR_RUNDOWNOVERRIDEANDSUPPRESSNGENEVENTS_KEYWORD 0x40000
#define CLR_RUNDOWNPERFTRACK_KEYWORD 0x20000000
#define CLR_RUNDOWNSTACK_KEYWORD 0x40000000

    //
    // Event Descriptors
    //
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR CLRStackWalkDCStart = { 0x0, 0x0, 0x0, 0x0, 0x52, 0xb, 0x40000000 };
#define CLRStackWalkDCStart_value 0x0
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCStart = { 0x8d, 0x0, 0x0, 0x4, 0x23, 0x1, 0x30 };
#define MethodDCStart_value 0x8d
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCStart_V1 = { 0x8d, 0x1, 0x0, 0x4, 0x23, 0x1, 0x30 };
#define MethodDCStart_V1_value 0x8d
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCStart_V2 = { 0x8d, 0x2, 0x0, 0x4, 0x23, 0x1, 0x30 };
#define MethodDCStart_V2_value 0x8d
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCEnd = { 0x8e, 0x0, 0x0, 0x4, 0x24, 0x1, 0x30 };
#define MethodDCEnd_value 0x8e
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCEnd_V1 = { 0x8e, 0x1, 0x0, 0x4, 0x24, 0x1, 0x30 };
#define MethodDCEnd_V1_value 0x8e
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCEnd_V2 = { 0x8e, 0x2, 0x0, 0x4, 0x24, 0x1, 0x30 };
#define MethodDCEnd_V2_value 0x8e
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCStartVerbose = { 0x8f, 0x0, 0x0, 0x4, 0x27, 0x1, 0x30 };
#define MethodDCStartVerbose_value 0x8f
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCStartVerbose_V1 = { 0x8f, 0x1, 0x0, 0x4, 0x27, 0x1, 0x30 };
#define MethodDCStartVerbose_V1_value 0x8f
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCStartVerbose_V2 = { 0x8f, 0x2, 0x0, 0x4, 0x27, 0x1, 0x30 };
#define MethodDCStartVerbose_V2_value 0x8f
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCEndVerbose = { 0x90, 0x0, 0x0, 0x4, 0x28, 0x1, 0x30 };
#define MethodDCEndVerbose_value 0x90
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCEndVerbose_V1 = { 0x90, 0x1, 0x0, 0x4, 0x28, 0x1, 0x30 };
#define MethodDCEndVerbose_V1_value 0x90
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCEndVerbose_V2 = { 0x90, 0x2, 0x0, 0x4, 0x28, 0x1, 0x30 };
#define MethodDCEndVerbose_V2_value 0x90
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DCStartComplete = { 0x91, 0x0, 0x0, 0x4, 0xe, 0x1, 0x20038 };
#define DCStartComplete_value 0x91
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DCStartComplete_V1 = { 0x91, 0x1, 0x0, 0x4, 0xe, 0x1, 0x20038 };
#define DCStartComplete_V1_value 0x91
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DCEndComplete = { 0x92, 0x0, 0x0, 0x4, 0xf, 0x1, 0x20038 };
#define DCEndComplete_value 0x92
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DCEndComplete_V1 = { 0x92, 0x1, 0x0, 0x4, 0xf, 0x1, 0x20038 };
#define DCEndComplete_V1_value 0x92
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DCStartInit = { 0x93, 0x0, 0x0, 0x4, 0x10, 0x1, 0x20038 };
#define DCStartInit_value 0x93
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DCStartInit_V1 = { 0x93, 0x1, 0x0, 0x4, 0x10, 0x1, 0x20038 };
#define DCStartInit_V1_value 0x93
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DCEndInit = { 0x94, 0x0, 0x0, 0x4, 0x11, 0x1, 0x20038 };
#define DCEndInit_value 0x94
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DCEndInit_V1 = { 0x94, 0x1, 0x0, 0x4, 0x11, 0x1, 0x20038 };
#define DCEndInit_V1_value 0x94
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCStartILToNativeMap = { 0x95, 0x0, 0x0, 0x5, 0x29, 0x1, 0x20000 };
#define MethodDCStartILToNativeMap_value 0x95
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR MethodDCEndILToNativeMap = { 0x96, 0x0, 0x0, 0x5, 0x2a, 0x1, 0x20000 };
#define MethodDCEndILToNativeMap_value 0x96
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DomainModuleDCStart = { 0x97, 0x0, 0x0, 0x4, 0x2e, 0x2, 0x8 };
#define DomainModuleDCStart_value 0x97
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DomainModuleDCStart_V1 = { 0x97, 0x1, 0x0, 0x4, 0x2e, 0x2, 0x8 };
#define DomainModuleDCStart_V1_value 0x97
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DomainModuleDCEnd = { 0x98, 0x0, 0x0, 0x4, 0x2f, 0x2, 0x8 };
#define DomainModuleDCEnd_value 0x98
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR DomainModuleDCEnd_V1 = { 0x98, 0x1, 0x0, 0x4, 0x2f, 0x2, 0x8 };
#define DomainModuleDCEnd_V1_value 0x98
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ModuleDCStart = { 0x99, 0x0, 0x0, 0x4, 0x23, 0x2, 0x8 };
#define ModuleDCStart_value 0x99
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ModuleDCStart_V1 = { 0x99, 0x1, 0x0, 0x4, 0x23, 0x2, 0x20000008 };
#define ModuleDCStart_V1_value 0x99
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ModuleDCStart_V2 = { 0x99, 0x2, 0x0, 0x4, 0x23, 0x2, 0x20000008 };
#define ModuleDCStart_V2_value 0x99
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ModuleDCEnd = { 0x9a, 0x0, 0x0, 0x4, 0x24, 0x2, 0x8 };
#define ModuleDCEnd_value 0x9a
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ModuleDCEnd_V1 = { 0x9a, 0x1, 0x0, 0x4, 0x24, 0x2, 0x20000008 };
#define ModuleDCEnd_V1_value 0x9a
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ModuleDCEnd_V2 = { 0x9a, 0x2, 0x0, 0x4, 0x24, 0x2, 0x20000008 };
#define ModuleDCEnd_V2_value 0x9a
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR AssemblyDCStart = { 0x9b, 0x0, 0x0, 0x4, 0x27, 0x2, 0x8 };
#define AssemblyDCStart_value 0x9b
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR AssemblyDCStart_V1 = { 0x9b, 0x1, 0x0, 0x4, 0x27, 0x2, 0x8 };
#define AssemblyDCStart_V1_value 0x9b
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR AssemblyDCEnd = { 0x9c, 0x0, 0x0, 0x4, 0x28, 0x2, 0x8 };
#define AssemblyDCEnd_value 0x9c
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR AssemblyDCEnd_V1 = { 0x9c, 0x1, 0x0, 0x4, 0x28, 0x2, 0x8 };
#define AssemblyDCEnd_V1_value 0x9c
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR AppDomainDCStart = { 0x9d, 0x0, 0x0, 0x4, 0x2b, 0x2, 0x8 };
#define AppDomainDCStart_value 0x9d
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR AppDomainDCStart_V1 = { 0x9d, 0x1, 0x0, 0x4, 0x2b, 0x2, 0x8 };
#define AppDomainDCStart_V1_value 0x9d
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR AppDomainDCEnd = { 0x9e, 0x0, 0x0, 0x4, 0x2c, 0x2, 0x8 };
#define AppDomainDCEnd_value 0x9e
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR AppDomainDCEnd_V1 = { 0x9e, 0x1, 0x0, 0x4, 0x2c, 0x2, 0x8 };
#define AppDomainDCEnd_V1_value 0x9e
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ThreadDC = { 0x9f, 0x0, 0x0, 0x4, 0x30, 0x2, 0x10800 };
#define ThreadDC_value 0x9f
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ModuleRangeDCStart = { 0xa0, 0x0, 0x0, 0x4, 0xa, 0x14, 0x20000000 };
#define ModuleRangeDCStart_value 0xa0
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR ModuleRangeDCEnd = { 0xa1, 0x0, 0x0, 0x4, 0xb, 0x14, 0x20000000 };
#define ModuleRangeDCEnd_value 0xa1
    EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR RuntimeInformationDCStart = { 0xbb, 0x0, 0x0, 0x4, 0x1, 0x13, 0x0 };
#define RuntimeInformationDCStart_value 0xbb

    //
    // Note on Generate Code from Manifest Windows Vista and above
    //
    //Structures :  are handled as a size and pointer pairs. The macro for the event will have an extra 
    //parameter for the size in bytes of the structure. Make sure that your structures have no extra padding.
    //
    //Strings: There are several cases that can be described in the manifest. For array of variable length 
    //strings, the generated code will take the count of characters for the whole array as an input parameter. 
    //
    //SID No support for array of SIDs, the macro will take a pointer to the SID and use appropriate 
    //GetLengthSid function to get the length.
    //

    //
    // Allow Diasabling of code generation
    //
#ifndef MCGEN_DISABLE_PROVIDER_CODE_GENERATION

    //
    // Globals 
    //


    //
    // Event Enablement Bits
    //

    EXTERN_C __declspec(selectany) DECLSPEC_CACHEALIGN ULONG Microsoft_Windows_DotNETRuntimeRundownEnableBits[1];
    EXTERN_C __declspec(selectany) const ULONGLONG Microsoft_Windows_DotNETRuntimeRundownKeywords[9] = { 0x40000000, 0x30, 0x20038, 0x20000, 0x8, 0x20000008, 0x10800, 0x20000000, 0x0 };
    EXTERN_C __declspec(selectany) const UCHAR Microsoft_Windows_DotNETRuntimeRundownLevels[9] = { 0, 4, 4, 5, 4, 4, 4, 4, 4 };
    EXTERN_C __declspec(selectany) MCGEN_TRACE_CONTEXT MICROSOFT_WINDOWS_DOTNETRUNTIME_RUNDOWN_PROVIDER_Context = { 0, 0, 0, 0, 0, 0, 0, 0, 9, Microsoft_Windows_DotNETRuntimeRundownEnableBits, Microsoft_Windows_DotNETRuntimeRundownKeywords, Microsoft_Windows_DotNETRuntimeRundownLevels };

    EXTERN_C __declspec(selectany) REGHANDLE Microsoft_Windows_DotNETRuntimeRundownHandle = (REGHANDLE)0;

#if !defined(McGenEventRegisterUnregister)
#define McGenEventRegisterUnregister
#pragma warning(push)
#pragma warning(disable:6103)
    DECLSPEC_NOINLINE __inline
        ULONG __stdcall
        McGenEventRegister(
            _In_ LPCGUID ProviderId,
            _In_opt_ PENABLECALLBACK EnableCallback,
            _In_opt_ PVOID CallbackContext,
            _Inout_ PREGHANDLE RegHandle
        )
        /*++

        Routine Description:

        This function register the provider with ETW USER mode.

        Arguments:
        ProviderId - Provider Id to be register with ETW.

        EnableCallback - Callback to be used.

        CallbackContext - Context for this provider.

        RegHandle - Pointer to Registration handle.

        Remarks:

        If the handle != NULL will return ERROR_SUCCESS

        --*/
    {
        ULONG Error;


        if (*RegHandle) {
            //
            // already registered
            //
            return ERROR_SUCCESS;
        }

        Error = EventRegister(ProviderId, EnableCallback, CallbackContext, RegHandle);

        return Error;
    }
#pragma warning(pop)


    DECLSPEC_NOINLINE __inline
        ULONG __stdcall
        McGenEventUnregister(_Inout_ PREGHANDLE RegHandle)
        /*++

        Routine Description:

        Unregister from ETW USER mode

        Arguments:
        RegHandle this is the pointer to the provider context
        Remarks:
        If Provider has not register RegHandle = NULL,
        return ERROR_SUCCESS
        --*/
    {
        ULONG Error;


        if (!(*RegHandle)) {
            //
            // Provider has not registerd
            //
            return ERROR_SUCCESS;
        }

        Error = EventUnregister(*RegHandle);
        *RegHandle = (REGHANDLE)0;

        return Error;
    }
#endif
    //
    // Register with ETW Vista +
    //
#ifndef EventRegisterMicrosoft_Windows_DotNETRuntimeRundown
#define EventRegisterMicrosoft_Windows_DotNETRuntimeRundown() McGenEventRegister(&MICROSOFT_WINDOWS_DOTNETRUNTIME_RUNDOWN_PROVIDER, McGenControlCallbackV2, &MICROSOFT_WINDOWS_DOTNETRUNTIME_RUNDOWN_PROVIDER_Context, &Microsoft_Windows_DotNETRuntimeRundownHandle) 
#endif

    //
    // UnRegister with ETW
    //
#ifndef EventUnregisterMicrosoft_Windows_DotNETRuntimeRundown
#define EventUnregisterMicrosoft_Windows_DotNETRuntimeRundown() McGenEventUnregister(&Microsoft_Windows_DotNETRuntimeRundownHandle) 
#endif

    //
    // Enablement check macro for CLRStackWalkDCStart
    //

#define EventEnabledCLRStackWalkDCStart() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000001) != 0)

    //
    // Event Macro for CLRStackWalkDCStart
    //
#define EventWriteCLRStackWalkDCStart(ClrInstanceID, Reserved1, Reserved2, FrameCount, Stack)\
        EventEnabledCLRStackWalkDCStart() ?\
        Template_hccqP2(Microsoft_Windows_DotNETRuntimeRundownHandle, &CLRStackWalkDCStart, ClrInstanceID, Reserved1, Reserved2, FrameCount, Stack)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCStart
    //

#define EventEnabledMethodDCStart() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCStart
    //
#define EventWriteMethodDCStart(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags)\
        EventEnabledMethodDCStart() ?\
        Template_xxxqqq(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCStart, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCStart_V1
    //

#define EventEnabledMethodDCStart_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCStart_V1
    //
#define EventWriteMethodDCStart_V1(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, ClrInstanceID)\
        EventEnabledMethodDCStart_V1() ?\
        Template_xxxqqqh(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCStart_V1, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCStart_V2
    //

#define EventEnabledMethodDCStart_V2() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCStart_V2
    //
#define EventWriteMethodDCStart_V2(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, ClrInstanceID, ReJITID)\
        EventEnabledMethodDCStart_V2() ?\
        Template_xxxqqqhx(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCStart_V2, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, ClrInstanceID, ReJITID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCEnd
    //

#define EventEnabledMethodDCEnd() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCEnd
    //
#define EventWriteMethodDCEnd(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags)\
        EventEnabledMethodDCEnd() ?\
        Template_xxxqqq(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCEnd, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCEnd_V1
    //

#define EventEnabledMethodDCEnd_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCEnd_V1
    //
#define EventWriteMethodDCEnd_V1(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, ClrInstanceID)\
        EventEnabledMethodDCEnd_V1() ?\
        Template_xxxqqqh(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCEnd_V1, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCEnd_V2
    //

#define EventEnabledMethodDCEnd_V2() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCEnd_V2
    //
#define EventWriteMethodDCEnd_V2(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, ClrInstanceID, ReJITID)\
        EventEnabledMethodDCEnd_V2() ?\
        Template_xxxqqqhx(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCEnd_V2, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, ClrInstanceID, ReJITID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCStartVerbose
    //

#define EventEnabledMethodDCStartVerbose() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCStartVerbose
    //
#define EventWriteMethodDCStartVerbose(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature)\
        EventEnabledMethodDCStartVerbose() ?\
        Template_xxxqqqzzz(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCStartVerbose, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCStartVerbose_V1
    //

#define EventEnabledMethodDCStartVerbose_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCStartVerbose_V1
    //
#define EventWriteMethodDCStartVerbose_V1(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature, ClrInstanceID)\
        EventEnabledMethodDCStartVerbose_V1() ?\
        Template_xxxqqqzzzh(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCStartVerbose_V1, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCStartVerbose_V2
    //

#define EventEnabledMethodDCStartVerbose_V2() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCStartVerbose_V2
    //
#define EventWriteMethodDCStartVerbose_V2(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature, ClrInstanceID, ReJITID)\
        EventEnabledMethodDCStartVerbose_V2() ?\
        Template_xxxqqqzzzhx(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCStartVerbose_V2, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature, ClrInstanceID, ReJITID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCEndVerbose
    //

#define EventEnabledMethodDCEndVerbose() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCEndVerbose
    //
#define EventWriteMethodDCEndVerbose(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature)\
        EventEnabledMethodDCEndVerbose() ?\
        Template_xxxqqqzzz(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCEndVerbose, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCEndVerbose_V1
    //

#define EventEnabledMethodDCEndVerbose_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCEndVerbose_V1
    //
#define EventWriteMethodDCEndVerbose_V1(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature, ClrInstanceID)\
        EventEnabledMethodDCEndVerbose_V1() ?\
        Template_xxxqqqzzzh(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCEndVerbose_V1, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCEndVerbose_V2
    //

#define EventEnabledMethodDCEndVerbose_V2() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000002) != 0)

    //
    // Event Macro for MethodDCEndVerbose_V2
    //
#define EventWriteMethodDCEndVerbose_V2(MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature, ClrInstanceID, ReJITID)\
        EventEnabledMethodDCEndVerbose_V2() ?\
        Template_xxxqqqzzzhx(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCEndVerbose_V2, MethodID, ModuleID, MethodStartAddress, MethodSize, MethodToken, MethodFlags, MethodNamespace, MethodName, MethodSignature, ClrInstanceID, ReJITID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DCStartComplete
    //

#define EventEnabledDCStartComplete() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000004) != 0)

    //
    // Event Macro for DCStartComplete
    //
#define EventWriteDCStartComplete()\
        EventEnabledDCStartComplete() ?\
        TemplateEventDescriptor(Microsoft_Windows_DotNETRuntimeRundownHandle, &DCStartComplete)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DCStartComplete_V1
    //

#define EventEnabledDCStartComplete_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000004) != 0)

    //
    // Event Macro for DCStartComplete_V1
    //
#define EventWriteDCStartComplete_V1(ClrInstanceID)\
        EventEnabledDCStartComplete_V1() ?\
        Template_h(Microsoft_Windows_DotNETRuntimeRundownHandle, &DCStartComplete_V1, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DCEndComplete
    //

#define EventEnabledDCEndComplete() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000004) != 0)

    //
    // Event Macro for DCEndComplete
    //
#define EventWriteDCEndComplete()\
        EventEnabledDCEndComplete() ?\
        TemplateEventDescriptor(Microsoft_Windows_DotNETRuntimeRundownHandle, &DCEndComplete)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DCEndComplete_V1
    //

#define EventEnabledDCEndComplete_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000004) != 0)

    //
    // Event Macro for DCEndComplete_V1
    //
#define EventWriteDCEndComplete_V1(ClrInstanceID)\
        EventEnabledDCEndComplete_V1() ?\
        Template_h(Microsoft_Windows_DotNETRuntimeRundownHandle, &DCEndComplete_V1, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DCStartInit
    //

#define EventEnabledDCStartInit() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000004) != 0)

    //
    // Event Macro for DCStartInit
    //
#define EventWriteDCStartInit()\
        EventEnabledDCStartInit() ?\
        TemplateEventDescriptor(Microsoft_Windows_DotNETRuntimeRundownHandle, &DCStartInit)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DCStartInit_V1
    //

#define EventEnabledDCStartInit_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000004) != 0)

    //
    // Event Macro for DCStartInit_V1
    //
#define EventWriteDCStartInit_V1(ClrInstanceID)\
        EventEnabledDCStartInit_V1() ?\
        Template_h(Microsoft_Windows_DotNETRuntimeRundownHandle, &DCStartInit_V1, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DCEndInit
    //

#define EventEnabledDCEndInit() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000004) != 0)

    //
    // Event Macro for DCEndInit
    //
#define EventWriteDCEndInit()\
        EventEnabledDCEndInit() ?\
        TemplateEventDescriptor(Microsoft_Windows_DotNETRuntimeRundownHandle, &DCEndInit)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DCEndInit_V1
    //

#define EventEnabledDCEndInit_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000004) != 0)

    //
    // Event Macro for DCEndInit_V1
    //
#define EventWriteDCEndInit_V1(ClrInstanceID)\
        EventEnabledDCEndInit_V1() ?\
        Template_h(Microsoft_Windows_DotNETRuntimeRundownHandle, &DCEndInit_V1, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCStartILToNativeMap
    //

#define EventEnabledMethodDCStartILToNativeMap() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000008) != 0)

    //
    // Event Macro for MethodDCStartILToNativeMap
    //
#define EventWriteMethodDCStartILToNativeMap(MethodID, ReJITID, MethodExtent, CountOfMapEntries, ILOffsets, NativeOffsets, ClrInstanceID)\
        EventEnabledMethodDCStartILToNativeMap() ?\
        Template_xxchQR3QR3h(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCStartILToNativeMap, MethodID, ReJITID, MethodExtent, CountOfMapEntries, ILOffsets, NativeOffsets, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for MethodDCEndILToNativeMap
    //

#define EventEnabledMethodDCEndILToNativeMap() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000008) != 0)

    //
    // Event Macro for MethodDCEndILToNativeMap
    //
#define EventWriteMethodDCEndILToNativeMap(MethodID, ReJITID, MethodExtent, CountOfMapEntries, ILOffsets, NativeOffsets, ClrInstanceID)\
        EventEnabledMethodDCEndILToNativeMap() ?\
        Template_xxchQR3QR3h(Microsoft_Windows_DotNETRuntimeRundownHandle, &MethodDCEndILToNativeMap, MethodID, ReJITID, MethodExtent, CountOfMapEntries, ILOffsets, NativeOffsets, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DomainModuleDCStart
    //

#define EventEnabledDomainModuleDCStart() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for DomainModuleDCStart
    //
#define EventWriteDomainModuleDCStart(ModuleID, AssemblyID, AppDomainID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath)\
        EventEnabledDomainModuleDCStart() ?\
        Template_xxxqqzz(Microsoft_Windows_DotNETRuntimeRundownHandle, &DomainModuleDCStart, ModuleID, AssemblyID, AppDomainID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DomainModuleDCStart_V1
    //

#define EventEnabledDomainModuleDCStart_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for DomainModuleDCStart_V1
    //
#define EventWriteDomainModuleDCStart_V1(ModuleID, AssemblyID, AppDomainID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID)\
        EventEnabledDomainModuleDCStart_V1() ?\
        Template_xxxqqzzh(Microsoft_Windows_DotNETRuntimeRundownHandle, &DomainModuleDCStart_V1, ModuleID, AssemblyID, AppDomainID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DomainModuleDCEnd
    //

#define EventEnabledDomainModuleDCEnd() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for DomainModuleDCEnd
    //
#define EventWriteDomainModuleDCEnd(ModuleID, AssemblyID, AppDomainID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath)\
        EventEnabledDomainModuleDCEnd() ?\
        Template_xxxqqzz(Microsoft_Windows_DotNETRuntimeRundownHandle, &DomainModuleDCEnd, ModuleID, AssemblyID, AppDomainID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for DomainModuleDCEnd_V1
    //

#define EventEnabledDomainModuleDCEnd_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for DomainModuleDCEnd_V1
    //
#define EventWriteDomainModuleDCEnd_V1(ModuleID, AssemblyID, AppDomainID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID)\
        EventEnabledDomainModuleDCEnd_V1() ?\
        Template_xxxqqzzh(Microsoft_Windows_DotNETRuntimeRundownHandle, &DomainModuleDCEnd_V1, ModuleID, AssemblyID, AppDomainID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ModuleDCStart
    //

#define EventEnabledModuleDCStart() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for ModuleDCStart
    //
#define EventWriteModuleDCStart(ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath)\
        EventEnabledModuleDCStart() ?\
        Template_xxqqzz(Microsoft_Windows_DotNETRuntimeRundownHandle, &ModuleDCStart, ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ModuleDCStart_V1
    //

#define EventEnabledModuleDCStart_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000020) != 0)

    //
    // Event Macro for ModuleDCStart_V1
    //
#define EventWriteModuleDCStart_V1(ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID)\
        EventEnabledModuleDCStart_V1() ?\
        Template_xxqqzzh(Microsoft_Windows_DotNETRuntimeRundownHandle, &ModuleDCStart_V1, ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ModuleDCStart_V2
    //

#define EventEnabledModuleDCStart_V2() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000020) != 0)

    //
    // Event Macro for ModuleDCStart_V2
    //
#define EventWriteModuleDCStart_V2(ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID, ManagedPdbSignature, ManagedPdbAge, ManagedPdbBuildPath, NativePdbSignature, NativePdbAge, NativePdbBuildPath)\
        EventEnabledModuleDCStart_V2() ?\
        Template_xxqqzzhjqzjqz(Microsoft_Windows_DotNETRuntimeRundownHandle, &ModuleDCStart_V2, ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID, ManagedPdbSignature, ManagedPdbAge, ManagedPdbBuildPath, NativePdbSignature, NativePdbAge, NativePdbBuildPath)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ModuleDCEnd
    //

#define EventEnabledModuleDCEnd() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for ModuleDCEnd
    //
#define EventWriteModuleDCEnd(ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath)\
        EventEnabledModuleDCEnd() ?\
        Template_xxqqzz(Microsoft_Windows_DotNETRuntimeRundownHandle, &ModuleDCEnd, ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ModuleDCEnd_V1
    //

#define EventEnabledModuleDCEnd_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000020) != 0)

    //
    // Event Macro for ModuleDCEnd_V1
    //
#define EventWriteModuleDCEnd_V1(ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID)\
        EventEnabledModuleDCEnd_V1() ?\
        Template_xxqqzzh(Microsoft_Windows_DotNETRuntimeRundownHandle, &ModuleDCEnd_V1, ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ModuleDCEnd_V2
    //

#define EventEnabledModuleDCEnd_V2() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000020) != 0)

    //
    // Event Macro for ModuleDCEnd_V2
    //
#define EventWriteModuleDCEnd_V2(ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID, ManagedPdbSignature, ManagedPdbAge, ManagedPdbBuildPath, NativePdbSignature, NativePdbAge, NativePdbBuildPath)\
        EventEnabledModuleDCEnd_V2() ?\
        Template_xxqqzzhjqzjqz(Microsoft_Windows_DotNETRuntimeRundownHandle, &ModuleDCEnd_V2, ModuleID, AssemblyID, ModuleFlags, Reserved1, ModuleILPath, ModuleNativePath, ClrInstanceID, ManagedPdbSignature, ManagedPdbAge, ManagedPdbBuildPath, NativePdbSignature, NativePdbAge, NativePdbBuildPath)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for AssemblyDCStart
    //

#define EventEnabledAssemblyDCStart() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for AssemblyDCStart
    //
#define EventWriteAssemblyDCStart(AssemblyID, AppDomainID, AssemblyFlags, FullyQualifiedAssemblyName)\
        EventEnabledAssemblyDCStart() ?\
        Template_xxqz(Microsoft_Windows_DotNETRuntimeRundownHandle, &AssemblyDCStart, AssemblyID, AppDomainID, AssemblyFlags, FullyQualifiedAssemblyName)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for AssemblyDCStart_V1
    //

#define EventEnabledAssemblyDCStart_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for AssemblyDCStart_V1
    //
#define EventWriteAssemblyDCStart_V1(AssemblyID, AppDomainID, BindingID, AssemblyFlags, FullyQualifiedAssemblyName, ClrInstanceID)\
        EventEnabledAssemblyDCStart_V1() ?\
        Template_xxxqzh(Microsoft_Windows_DotNETRuntimeRundownHandle, &AssemblyDCStart_V1, AssemblyID, AppDomainID, BindingID, AssemblyFlags, FullyQualifiedAssemblyName, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for AssemblyDCEnd
    //

#define EventEnabledAssemblyDCEnd() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for AssemblyDCEnd
    //
#define EventWriteAssemblyDCEnd(AssemblyID, AppDomainID, AssemblyFlags, FullyQualifiedAssemblyName)\
        EventEnabledAssemblyDCEnd() ?\
        Template_xxqz(Microsoft_Windows_DotNETRuntimeRundownHandle, &AssemblyDCEnd, AssemblyID, AppDomainID, AssemblyFlags, FullyQualifiedAssemblyName)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for AssemblyDCEnd_V1
    //

#define EventEnabledAssemblyDCEnd_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for AssemblyDCEnd_V1
    //
#define EventWriteAssemblyDCEnd_V1(AssemblyID, AppDomainID, BindingID, AssemblyFlags, FullyQualifiedAssemblyName, ClrInstanceID)\
        EventEnabledAssemblyDCEnd_V1() ?\
        Template_xxxqzh(Microsoft_Windows_DotNETRuntimeRundownHandle, &AssemblyDCEnd_V1, AssemblyID, AppDomainID, BindingID, AssemblyFlags, FullyQualifiedAssemblyName, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for AppDomainDCStart
    //

#define EventEnabledAppDomainDCStart() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for AppDomainDCStart
    //
#define EventWriteAppDomainDCStart(AppDomainID, AppDomainFlags, AppDomainName)\
        EventEnabledAppDomainDCStart() ?\
        Template_xqz(Microsoft_Windows_DotNETRuntimeRundownHandle, &AppDomainDCStart, AppDomainID, AppDomainFlags, AppDomainName)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for AppDomainDCStart_V1
    //

#define EventEnabledAppDomainDCStart_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for AppDomainDCStart_V1
    //
#define EventWriteAppDomainDCStart_V1(AppDomainID, AppDomainFlags, AppDomainName, AppDomainIndex, ClrInstanceID)\
        EventEnabledAppDomainDCStart_V1() ?\
        Template_xqzqh(Microsoft_Windows_DotNETRuntimeRundownHandle, &AppDomainDCStart_V1, AppDomainID, AppDomainFlags, AppDomainName, AppDomainIndex, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for AppDomainDCEnd
    //

#define EventEnabledAppDomainDCEnd() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for AppDomainDCEnd
    //
#define EventWriteAppDomainDCEnd(AppDomainID, AppDomainFlags, AppDomainName)\
        EventEnabledAppDomainDCEnd() ?\
        Template_xqz(Microsoft_Windows_DotNETRuntimeRundownHandle, &AppDomainDCEnd, AppDomainID, AppDomainFlags, AppDomainName)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for AppDomainDCEnd_V1
    //

#define EventEnabledAppDomainDCEnd_V1() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000010) != 0)

    //
    // Event Macro for AppDomainDCEnd_V1
    //
#define EventWriteAppDomainDCEnd_V1(AppDomainID, AppDomainFlags, AppDomainName, AppDomainIndex, ClrInstanceID)\
        EventEnabledAppDomainDCEnd_V1() ?\
        Template_xqzqh(Microsoft_Windows_DotNETRuntimeRundownHandle, &AppDomainDCEnd_V1, AppDomainID, AppDomainFlags, AppDomainName, AppDomainIndex, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ThreadDC
    //

#define EventEnabledThreadDC() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000040) != 0)

    //
    // Event Macro for ThreadDC
    //
#define EventWriteThreadDC(ManagedThreadID, AppDomainID, Flags, ManagedThreadIndex, OSThreadID, ClrInstanceID)\
        EventEnabledThreadDC() ?\
        Template_xxqqqh(Microsoft_Windows_DotNETRuntimeRundownHandle, &ThreadDC, ManagedThreadID, AppDomainID, Flags, ManagedThreadIndex, OSThreadID, ClrInstanceID)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ModuleRangeDCStart
    //

#define EventEnabledModuleRangeDCStart() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000080) != 0)

    //
    // Event Macro for ModuleRangeDCStart
    //
#define EventWriteModuleRangeDCStart(ClrInstanceID, ModuleID, RangeBegin, RangeSize, RangeType)\
        EventEnabledModuleRangeDCStart() ?\
        Template_hxqqc(Microsoft_Windows_DotNETRuntimeRundownHandle, &ModuleRangeDCStart, ClrInstanceID, ModuleID, RangeBegin, RangeSize, RangeType)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for ModuleRangeDCEnd
    //

#define EventEnabledModuleRangeDCEnd() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000080) != 0)

    //
    // Event Macro for ModuleRangeDCEnd
    //
#define EventWriteModuleRangeDCEnd(ClrInstanceID, ModuleID, RangeBegin, RangeSize, RangeType)\
        EventEnabledModuleRangeDCEnd() ?\
        Template_hxqqc(Microsoft_Windows_DotNETRuntimeRundownHandle, &ModuleRangeDCEnd, ClrInstanceID, ModuleID, RangeBegin, RangeSize, RangeType)\
        : ERROR_SUCCESS\

    //
    // Enablement check macro for RuntimeInformationDCStart
    //

#define EventEnabledRuntimeInformationDCStart() ((Microsoft_Windows_DotNETRuntimeRundownEnableBits[0] & 0x00000100) != 0)

    //
    // Event Macro for RuntimeInformationDCStart
    //
#define EventWriteRuntimeInformationDCStart(ClrInstanceID, Sku, BclMajorVersion, BclMinorVersion, BclBuildNumber, BclQfeNumber, VMMajorVersion, VMMinorVersion, VMBuildNumber, VMQfeNumber, StartupFlags, StartupMode, CommandLine, ComObjectGuid, RuntimeDllPath)\
        EventEnabledRuntimeInformationDCStart() ?\
        Template_hhhhhhhhhhqczjz(Microsoft_Windows_DotNETRuntimeRundownHandle, &RuntimeInformationDCStart, ClrInstanceID, Sku, BclMajorVersion, BclMinorVersion, BclBuildNumber, BclQfeNumber, VMMajorVersion, VMMinorVersion, VMBuildNumber, VMQfeNumber, StartupFlags, StartupMode, CommandLine, ComObjectGuid, RuntimeDllPath)\
        : ERROR_SUCCESS\

#endif // MCGEN_DISABLE_PROVIDER_CODE_GENERATION


    //
    // Allow Diasabling of code generation
    //
#ifndef MCGEN_DISABLE_PROVIDER_CODE_GENERATION

    //
    // Template Functions 
    //
    //
    //Template from manifest : ClrStackWalk
    //
#ifndef Template_hccqP2_def
#define Template_hccqP2_def
    ETW_INLINE
        ULONG
        Template_hccqP2(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ const unsigned short  _Arg0,
            _In_ const UCHAR  _Arg1,
            _In_ const UCHAR  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_reads_(2) const void * *_Arg4
        )
    {
#define ARGUMENT_COUNT_hccqP2 5

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_hccqP2];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(const UCHAR));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const UCHAR));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], _Arg4, sizeof(PVOID) * 2);

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_hccqP2, EventData);
    }
#endif

    //
    //Template from manifest : MethodLoadUnloadRundown
    //
#ifndef Template_xxxqqq_def
#define Template_xxxqqq_def
    ETW_INLINE
        ULONG
        Template_xxxqqq(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_ const unsigned int  _Arg5
        )
    {
#define ARGUMENT_COUNT_xxxqqq 6

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqqq];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned int));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqqq, EventData);
    }
#endif

    //
    //Template from manifest : MethodLoadUnloadRundown_V1
    //
#ifndef Template_xxxqqqh_def
#define Template_xxxqqqh_def
    ETW_INLINE
        ULONG
        Template_xxxqqqh(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_ const unsigned int  _Arg5,
            _In_ const unsigned short  _Arg6
        )
    {
#define ARGUMENT_COUNT_xxxqqqh 7

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqqqh];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqqqh, EventData);
    }
#endif

    //
    //Template from manifest : MethodLoadUnloadRundown_V2
    //
#ifndef Template_xxxqqqhx_def
#define Template_xxxqqqhx_def
    ETW_INLINE
        ULONG
        Template_xxxqqqhx(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_ const unsigned int  _Arg5,
            _In_ const unsigned short  _Arg6,
            _In_ unsigned __int64  _Arg7
        )
    {
#define ARGUMENT_COUNT_xxxqqqhx 8

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqqqhx];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[7], &_Arg7, sizeof(unsigned __int64));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqqqhx, EventData);
    }
#endif

    //
    //Template from manifest : MethodLoadUnloadRundownVerbose
    //
#ifndef Template_xxxqqqzzz_def
#define Template_xxxqqqzzz_def
    ETW_INLINE
        ULONG
        Template_xxxqqqzzz(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_ const unsigned int  _Arg5,
            _In_opt_ PCWSTR  _Arg6,
            _In_opt_ PCWSTR  _Arg7,
            _In_opt_ PCWSTR  _Arg8
        )
    {
#define ARGUMENT_COUNT_xxxqqqzzz 9

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqqqzzz];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[6],
            (_Arg6 != NULL) ? _Arg6 : L"NULL",
            (_Arg6 != NULL) ? (ULONG)((wcslen(_Arg6) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[7],
            (_Arg7 != NULL) ? _Arg7 : L"NULL",
            (_Arg7 != NULL) ? (ULONG)((wcslen(_Arg7) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[8],
            (_Arg8 != NULL) ? _Arg8 : L"NULL",
            (_Arg8 != NULL) ? (ULONG)((wcslen(_Arg8) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqqqzzz, EventData);
    }
#endif

    //
    //Template from manifest : MethodLoadUnloadRundownVerbose_V1
    //
#ifndef Template_xxxqqqzzzh_def
#define Template_xxxqqqzzzh_def
    ETW_INLINE
        ULONG
        Template_xxxqqqzzzh(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_ const unsigned int  _Arg5,
            _In_opt_ PCWSTR  _Arg6,
            _In_opt_ PCWSTR  _Arg7,
            _In_opt_ PCWSTR  _Arg8,
            _In_ const unsigned short  _Arg9
        )
    {
#define ARGUMENT_COUNT_xxxqqqzzzh 10

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqqqzzzh];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[6],
            (_Arg6 != NULL) ? _Arg6 : L"NULL",
            (_Arg6 != NULL) ? (ULONG)((wcslen(_Arg6) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[7],
            (_Arg7 != NULL) ? _Arg7 : L"NULL",
            (_Arg7 != NULL) ? (ULONG)((wcslen(_Arg7) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[8],
            (_Arg8 != NULL) ? _Arg8 : L"NULL",
            (_Arg8 != NULL) ? (ULONG)((wcslen(_Arg8) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[9], &_Arg9, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqqqzzzh, EventData);
    }
#endif

    //
    //Template from manifest : MethodLoadUnloadRundownVerbose_V2
    //
#ifndef Template_xxxqqqzzzhx_def
#define Template_xxxqqqzzzhx_def
    ETW_INLINE
        ULONG
        Template_xxxqqqzzzhx(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_ const unsigned int  _Arg5,
            _In_opt_ PCWSTR  _Arg6,
            _In_opt_ PCWSTR  _Arg7,
            _In_opt_ PCWSTR  _Arg8,
            _In_ const unsigned short  _Arg9,
            _In_ unsigned __int64  _Arg10
        )
    {
#define ARGUMENT_COUNT_xxxqqqzzzhx 11

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqqqzzzhx];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[6],
            (_Arg6 != NULL) ? _Arg6 : L"NULL",
            (_Arg6 != NULL) ? (ULONG)((wcslen(_Arg6) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[7],
            (_Arg7 != NULL) ? _Arg7 : L"NULL",
            (_Arg7 != NULL) ? (ULONG)((wcslen(_Arg7) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[8],
            (_Arg8 != NULL) ? _Arg8 : L"NULL",
            (_Arg8 != NULL) ? (ULONG)((wcslen(_Arg8) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[9], &_Arg9, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[10], &_Arg10, sizeof(unsigned __int64));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqqqzzzhx, EventData);
    }
#endif

    //
    //Template from manifest : (null)
    //
#ifndef TemplateEventDescriptor_def
#define TemplateEventDescriptor_def


    ETW_INLINE
        ULONG
        TemplateEventDescriptor(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor
        )
    {
        return EventWrite(RegHandle, Descriptor, 0, NULL);
    }
#endif

    //
    //Template from manifest : DCStartEnd
    //
#ifndef Template_h_def
#define Template_h_def
    ETW_INLINE
        ULONG
        Template_h(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ const unsigned short  _Arg0
        )
    {
#define ARGUMENT_COUNT_h 1

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_h];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_h, EventData);
    }
#endif

    //
    //Template from manifest : MethodILToNativeMapRundown
    //
#ifndef Template_xxchQR3QR3h_def
#define Template_xxchQR3QR3h_def
    ETW_INLINE
        ULONG
        Template_xxchQR3QR3h(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ const UCHAR  _Arg2,
            _In_ const unsigned short  _Arg3,
            _In_reads_(_Arg3) const unsigned int *_Arg4,
            _In_reads_(_Arg3) const unsigned int *_Arg5,
            _In_ const unsigned short  _Arg6
        )
    {
#define ARGUMENT_COUNT_xxchQR3QR3h 7

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxchQR3QR3h];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const UCHAR));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[4], _Arg4, sizeof(const unsigned int)*_Arg3);

        EventDataDescCreate(&EventData[5], _Arg5, sizeof(const unsigned int)*_Arg3);

        EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxchQR3QR3h, EventData);
    }
#endif

    //
    //Template from manifest : DomainModuleLoadUnloadRundown
    //
#ifndef Template_xxxqqzz_def
#define Template_xxxqqzz_def
    ETW_INLINE
        ULONG
        Template_xxxqqzz(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_opt_ PCWSTR  _Arg5,
            _In_opt_ PCWSTR  _Arg6
        )
    {
#define ARGUMENT_COUNT_xxxqqzz 7

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqqzz];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5],
            (_Arg5 != NULL) ? _Arg5 : L"NULL",
            (_Arg5 != NULL) ? (ULONG)((wcslen(_Arg5) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[6],
            (_Arg6 != NULL) ? _Arg6 : L"NULL",
            (_Arg6 != NULL) ? (ULONG)((wcslen(_Arg6) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqqzz, EventData);
    }
#endif

    //
    //Template from manifest : DomainModuleLoadUnloadRundown_V1
    //
#ifndef Template_xxxqqzzh_def
#define Template_xxxqqzzh_def
    ETW_INLINE
        ULONG
        Template_xxxqqzzh(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_opt_ PCWSTR  _Arg5,
            _In_opt_ PCWSTR  _Arg6,
            _In_ const unsigned short  _Arg7
        )
    {
#define ARGUMENT_COUNT_xxxqqzzh 8

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqqzzh];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5],
            (_Arg5 != NULL) ? _Arg5 : L"NULL",
            (_Arg5 != NULL) ? (ULONG)((wcslen(_Arg5) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[6],
            (_Arg6 != NULL) ? _Arg6 : L"NULL",
            (_Arg6 != NULL) ? (ULONG)((wcslen(_Arg6) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[7], &_Arg7, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqqzzh, EventData);
    }
#endif

    //
    //Template from manifest : ModuleLoadUnloadRundown
    //
#ifndef Template_xxqqzz_def
#define Template_xxqqzz_def
    ETW_INLINE
        ULONG
        Template_xxqqzz(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ const unsigned int  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_opt_ PCWSTR  _Arg4,
            _In_opt_ PCWSTR  _Arg5
        )
    {
#define ARGUMENT_COUNT_xxqqzz 6

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxqqzz];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4],
            (_Arg4 != NULL) ? _Arg4 : L"NULL",
            (_Arg4 != NULL) ? (ULONG)((wcslen(_Arg4) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[5],
            (_Arg5 != NULL) ? _Arg5 : L"NULL",
            (_Arg5 != NULL) ? (ULONG)((wcslen(_Arg5) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxqqzz, EventData);
    }
#endif

    //
    //Template from manifest : ModuleLoadUnloadRundown_V1
    //
#ifndef Template_xxqqzzh_def
#define Template_xxqqzzh_def
    ETW_INLINE
        ULONG
        Template_xxqqzzh(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ const unsigned int  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_opt_ PCWSTR  _Arg4,
            _In_opt_ PCWSTR  _Arg5,
            _In_ const unsigned short  _Arg6
        )
    {
#define ARGUMENT_COUNT_xxqqzzh 7

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxqqzzh];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4],
            (_Arg4 != NULL) ? _Arg4 : L"NULL",
            (_Arg4 != NULL) ? (ULONG)((wcslen(_Arg4) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[5],
            (_Arg5 != NULL) ? _Arg5 : L"NULL",
            (_Arg5 != NULL) ? (ULONG)((wcslen(_Arg5) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxqqzzh, EventData);
    }
#endif

    //
    //Template from manifest : ModuleLoadUnloadRundown_V2
    //
#ifndef Template_xxqqzzhjqzjqz_def
#define Template_xxqqzzhjqzjqz_def
    ETW_INLINE
        ULONG
        Template_xxqqzzhjqzjqz(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ const unsigned int  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_opt_ PCWSTR  _Arg4,
            _In_opt_ PCWSTR  _Arg5,
            _In_ const unsigned short  _Arg6,
            _In_ LPCGUID  _Arg7,
            _In_ const unsigned int  _Arg8,
            _In_opt_ PCWSTR  _Arg9,
            _In_ LPCGUID  _Arg10,
            _In_ const unsigned int  _Arg11,
            _In_opt_ PCWSTR  _Arg12
        )
    {
#define ARGUMENT_COUNT_xxqqzzhjqzjqz 13

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxqqzzhjqzjqz];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4],
            (_Arg4 != NULL) ? _Arg4 : L"NULL",
            (_Arg4 != NULL) ? (ULONG)((wcslen(_Arg4) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[5],
            (_Arg5 != NULL) ? _Arg5 : L"NULL",
            (_Arg5 != NULL) ? (ULONG)((wcslen(_Arg5) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[7], _Arg7, sizeof(GUID));

        EventDataDescCreate(&EventData[8], &_Arg8, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[9],
            (_Arg9 != NULL) ? _Arg9 : L"NULL",
            (_Arg9 != NULL) ? (ULONG)((wcslen(_Arg9) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[10], _Arg10, sizeof(GUID));

        EventDataDescCreate(&EventData[11], &_Arg11, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[12],
            (_Arg12 != NULL) ? _Arg12 : L"NULL",
            (_Arg12 != NULL) ? (ULONG)((wcslen(_Arg12) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxqqzzhjqzjqz, EventData);
    }
#endif

    //
    //Template from manifest : AssemblyLoadUnloadRundown
    //
#ifndef Template_xxqz_def
#define Template_xxqz_def
    ETW_INLINE
        ULONG
        Template_xxqz(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ const unsigned int  _Arg2,
            _In_opt_ PCWSTR  _Arg3
        )
    {
#define ARGUMENT_COUNT_xxqz 4

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxqz];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[3],
            (_Arg3 != NULL) ? _Arg3 : L"NULL",
            (_Arg3 != NULL) ? (ULONG)((wcslen(_Arg3) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxqz, EventData);
    }
#endif

    //
    //Template from manifest : AssemblyLoadUnloadRundown_V1
    //
#ifndef Template_xxxqzh_def
#define Template_xxxqzh_def
    ETW_INLINE
        ULONG
        Template_xxxqzh(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ unsigned __int64  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_opt_ PCWSTR  _Arg4,
            _In_ const unsigned short  _Arg5
        )
    {
#define ARGUMENT_COUNT_xxxqzh 6

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxxqzh];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4],
            (_Arg4 != NULL) ? _Arg4 : L"NULL",
            (_Arg4 != NULL) ? (ULONG)((wcslen(_Arg4) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxxqzh, EventData);
    }
#endif

    //
    //Template from manifest : AppDomainLoadUnloadRundown
    //
#ifndef Template_xqz_def
#define Template_xqz_def
    ETW_INLINE
        ULONG
        Template_xqz(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ const unsigned int  _Arg1,
            _In_opt_ PCWSTR  _Arg2
        )
    {
#define ARGUMENT_COUNT_xqz 3

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xqz];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[2],
            (_Arg2 != NULL) ? _Arg2 : L"NULL",
            (_Arg2 != NULL) ? (ULONG)((wcslen(_Arg2) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xqz, EventData);
    }
#endif

    //
    //Template from manifest : AppDomainLoadUnloadRundown_V1
    //
#ifndef Template_xqzqh_def
#define Template_xqzqh_def
    ETW_INLINE
        ULONG
        Template_xqzqh(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ const unsigned int  _Arg1,
            _In_opt_ PCWSTR  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned short  _Arg4
        )
    {
#define ARGUMENT_COUNT_xqzqh 5

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xqzqh];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[2],
            (_Arg2 != NULL) ? _Arg2 : L"NULL",
            (_Arg2 != NULL) ? (ULONG)((wcslen(_Arg2) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xqzqh, EventData);
    }
#endif

    //
    //Template from manifest : ThreadCreatedRundown
    //
#ifndef Template_xxqqqh_def
#define Template_xxqqqh_def
    ETW_INLINE
        ULONG
        Template_xxqqqh(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ unsigned __int64  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ const unsigned int  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const unsigned int  _Arg4,
            _In_ const unsigned short  _Arg5
        )
    {
#define ARGUMENT_COUNT_xxqqqh 6

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_xxqqqh];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned short));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_xxqqqh, EventData);
    }
#endif

    //
    //Template from manifest : ModuleRangeRundown
    //
#ifndef Template_hxqqc_def
#define Template_hxqqc_def
    ETW_INLINE
        ULONG
        Template_hxqqc(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ const unsigned short  _Arg0,
            _In_ unsigned __int64  _Arg1,
            _In_ const unsigned int  _Arg2,
            _In_ const unsigned int  _Arg3,
            _In_ const UCHAR  _Arg4
        )
    {
#define ARGUMENT_COUNT_hxqqc 5

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_hxqqc];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(unsigned __int64));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const UCHAR));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_hxqqc, EventData);
    }
#endif

    //
    //Template from manifest : RuntimeInformationRundown
    //
#ifndef Template_hhhhhhhhhhqczjz_def
#define Template_hhhhhhhhhhqczjz_def
    ETW_INLINE
        ULONG
        Template_hhhhhhhhhhqczjz(
            _In_ REGHANDLE RegHandle,
            _In_ PCEVENT_DESCRIPTOR Descriptor,
            _In_ const unsigned short  _Arg0,
            _In_ const unsigned short  _Arg1,
            _In_ const unsigned short  _Arg2,
            _In_ const unsigned short  _Arg3,
            _In_ const unsigned short  _Arg4,
            _In_ const unsigned short  _Arg5,
            _In_ const unsigned short  _Arg6,
            _In_ const unsigned short  _Arg7,
            _In_ const unsigned short  _Arg8,
            _In_ const unsigned short  _Arg9,
            _In_ const unsigned int  _Arg10,
            _In_ const UCHAR  _Arg11,
            _In_opt_ PCWSTR  _Arg12,
            _In_ LPCGUID  _Arg13,
            _In_opt_ PCWSTR  _Arg14
        )
    {
#define ARGUMENT_COUNT_hhhhhhhhhhqczjz 15

        EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_hhhhhhhhhhqczjz];

        EventDataDescCreate(&EventData[0], &_Arg0, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[1], &_Arg1, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[3], &_Arg3, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[5], &_Arg5, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[7], &_Arg7, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[8], &_Arg8, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[9], &_Arg9, sizeof(const unsigned short));

        EventDataDescCreate(&EventData[10], &_Arg10, sizeof(const unsigned int));

        EventDataDescCreate(&EventData[11], &_Arg11, sizeof(const UCHAR));

        EventDataDescCreate(&EventData[12],
            (_Arg12 != NULL) ? _Arg12 : L"NULL",
            (_Arg12 != NULL) ? (ULONG)((wcslen(_Arg12) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        EventDataDescCreate(&EventData[13], _Arg13, sizeof(GUID));

        EventDataDescCreate(&EventData[14],
            (_Arg14 != NULL) ? _Arg14 : L"NULL",
            (_Arg14 != NULL) ? (ULONG)((wcslen(_Arg14) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L"NULL"));

        return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_hhhhhhhhhhqczjz, EventData);
    }
#endif

#endif // MCGEN_DISABLE_PROVIDER_CODE_GENERATION

#if defined(__cplusplus)
};
#endif

#define MSG_RundownPublisher_LoaderKeywordMessage 0x10000004L
#define MSG_RundownPublisher_JitKeywordMessage 0x10000005L
#define MSG_RundownPublisher_NGenKeywordMessage 0x10000006L
#define MSG_RundownPublisher_StartRundownKeywordMessage 0x10000007L
#define MSG_RundownPublisher_EndRundownKeywordMessage 0x10000009L
#define MSG_RundownPublisher_ThreadingKeywordMessage 0x10000011L
#define MSG_RundownPublisher_JittedMethodILToNativeMapRundownKeywordMessage 0x10000012L
#define MSG_RundownPublisher_OverrideAndSuppressNGenEventsRundownKeywordMessage 0x10000013L
#define MSG_RundownPublisher_PerfTrackRundownKeywordMessage 0x1000001EL
#define MSG_RundownPublisher_StackKeywordMessage 0x1000001FL
#define MSG_RundownPublisher_DCStartCompleteOpcodeMessage 0x3001000EL
#define MSG_RundownPublisher_DCEndCompleteOpcodeMessage 0x3001000FL
#define MSG_RundownPublisher_DCStartInitOpcodeMessage 0x30010010L
#define MSG_RundownPublisher_DCEndInitOpcodeMessage 0x30010011L
#define MSG_RundownPublisher_MethodDCStartOpcodeMessage 0x30010023L
#define MSG_RundownPublisher_MethodDCEndOpcodeMessage 0x30010024L
#define MSG_RundownPublisher_MethodDCStartVerboseOpcodeMessage 0x30010027L
#define MSG_RundownPublisher_MethodDCEndVerboseOpcodeMessage 0x30010028L
#define MSG_RundownPublisher_MethodDCStartILToNativeMapOpcodeMessage 0x30010029L
#define MSG_RundownPublisher_MethodDCEndILToNativeMapOpcodeMessage 0x3001002AL
#define MSG_RundownPublisher_ModuleDCStartOpcodeMessage 0x30020023L
#define MSG_RundownPublisher_ModuleDCEndOpcodeMessage 0x30020024L
#define MSG_RundownPublisher_AssemblyDCStartOpcodeMessage 0x30020027L
#define MSG_RundownPublisher_AssemblyDCEndOpcodeMessage 0x30020028L
#define MSG_RundownPublisher_AppDomainDCStartOpcodeMessage 0x3002002BL
#define MSG_RundownPublisher_AppDomainDCEndOpcodeMessage 0x3002002CL
#define MSG_RundownPublisher_DomainModuleDCStartOpcodeMessage 0x3002002EL
#define MSG_RundownPublisher_DomainModuleDCEndOpcodeMessage 0x3002002FL
#define MSG_RundownPublisher_ThreadDCOpcodeMessage 0x30020030L
#define MSG_RundownPublisher_CLRStackWalkOpcodeMessage 0x300B0052L
#define MSG_RundownPublisher_ModuleRangeDCStartOpcodeMessage 0x3014000AL
#define MSG_RundownPublisher_ModuleRangeDCEndOpcodeMessage 0x3014000BL
#define MSG_RundownPublisher_MethodTaskMessage 0x70000001L
#define MSG_RundownPublisher_LoaderTaskMessage 0x70000002L
#define MSG_RundownPublisher_StackTaskMessage 0x7000000BL
#define MSG_RundownPublisher_EEStartupTaskMessage 0x70000013L
#define MSG_RundownPublisher_PerfTrackTaskMessage 0x70000014L
#define MSG_RundownPublisher_StackEventMessage 0xB0000000L
#define MSG_RundownPublisher_MethodDCStartEventMessage 0xB000008DL
#define MSG_RundownPublisher_MethodDCEndEventMessage 0xB000008EL
#define MSG_RundownPublisher_MethodDCStartVerboseEventMessage 0xB000008FL
#define MSG_RundownPublisher_MethodDCEndVerboseEventMessage 0xB0000090L
#define MSG_RundownPublisher_MethodDCStartILToNativeMapEventMessage 0xB0000095L
#define MSG_RundownPublisher_MethodDCEndILToNativeMapEventMessage 0xB0000096L
#define MSG_RundownPublisher_DomainModuleDCStartEventMessage 0xB0000097L
#define MSG_RundownPublisher_DomainModuleDCEndEventMessage 0xB0000098L
#define MSG_RundownPublisher_ModuleDCStartEventMessage 0xB0000099L
#define MSG_RundownPublisher_ModuleDCEndEventMessage 0xB000009AL
#define MSG_RundownPublisher_AssemblyDCStartEventMessage 0xB000009BL
#define MSG_RundownPublisher_AssemblyDCEndEventMessage 0xB000009CL
#define MSG_RundownPublisher_AppDomainDCStartEventMessage 0xB000009DL
#define MSG_RundownPublisher_AppDomainDCEndEventMessage 0xB000009EL
#define MSG_RundownPublisher_ThreadCreatedEventMessage 0xB000009FL
#define MSG_RundownPublisher_ModuleRangeDCStartEventMessage 0xB00000A0L
#define MSG_RundownPublisher_ModuleRangeDCEndEventMessage 0xB00000A1L
#define MSG_RundownPublisher_RuntimeInformationEventMessage 0xB00000BBL
#define MSG_RundownPublisher_MethodDCStart_V1EventMessage 0xB001008DL
#define MSG_RundownPublisher_MethodDCEnd_V1EventMessage 0xB001008EL
#define MSG_RundownPublisher_MethodDCStartVerbose_V1EventMessage 0xB001008FL
#define MSG_RundownPublisher_MethodDCEndVerbose_V1EventMessage 0xB0010090L
#define MSG_RundownPublisher_DCStartCompleteEventMessage 0xB0010091L
#define MSG_RundownPublisher_DCEndCompleteEventMessage 0xB0010092L
#define MSG_RundownPublisher_DCStartInitEventMessage 0xB0010093L
#define MSG_RundownPublisher_DCEndInitEventMessage 0xB0010094L
#define MSG_RundownPublisher_DomainModuleDCStart_V1EventMessage 0xB0010097L
#define MSG_RundownPublisher_DomainModuleDCEnd_V1EventMessage 0xB0010098L
#define MSG_RundownPublisher_ModuleDCStart_V1EventMessage 0xB0010099L
#define MSG_RundownPublisher_ModuleDCEnd_V1EventMessage 0xB001009AL
#define MSG_RundownPublisher_AssemblyDCStart_V1EventMessage 0xB001009BL
#define MSG_RundownPublisher_AssemblyDCEnd_V1EventMessage 0xB001009CL
#define MSG_RundownPublisher_AppDomainDCStart_V1EventMessage 0xB001009DL
#define MSG_RundownPublisher_AppDomainDCEnd_V1EventMessage 0xB001009EL
#define MSG_RundownPublisher_MethodDCStart_V2EventMessage 0xB002008DL
#define MSG_RundownPublisher_MethodDCEnd_V2EventMessage 0xB002008EL
#define MSG_RundownPublisher_MethodDCStartVerbose_V2EventMessage 0xB002008FL
#define MSG_RundownPublisher_MethodDCEndVerbose_V2EventMessage 0xB0020090L
#define MSG_RundownPublisher_ModuleDCStart_V2EventMessage 0xB0020099L
#define MSG_RundownPublisher_ModuleDCEnd_V2EventMessage 0xB002009AL
#define MSG_RundownPublisher_ModuleRangeTypeMap_ColdRangeMessage 0xF0000001L
#define MSG_RundownPublisher_AppDomain_DefaultMapMessage 0xF0000002L
#define MSG_RundownPublisher_AppDomain_ExecutableMapMessage 0xF0000003L
#define MSG_RundownPublisher_AppDomain_SharedMapMessage 0xF0000004L
#define MSG_RundownPublisher_Assembly_DomainNeutralMapMessage 0xF0000005L
#define MSG_RundownPublisher_Assembly_DynamicMapMessage 0xF0000006L
#define MSG_RundownPublisher_Assembly_NativeMapMessage 0xF0000007L
#define MSG_RundownPublisher_Assembly_CollectibleMapMessage 0xF0000008L
#define MSG_RundownPublisher_Module_DomainNeutralMapMessage 0xF0000009L
#define MSG_RundownPublisher_Module_NativeMapMessage 0xF000000AL
#define MSG_RundownPublisher_Module_DynamicMapMessage 0xF000000BL
#define MSG_RundownPublisher_Module_ManifestMapMessage 0xF000000CL
#define MSG_RundownPublisher_Method_DynamicMapMessage 0xF000000DL
#define MSG_RundownPublisher_Method_GenericMapMessage 0xF000000EL
#define MSG_RundownPublisher_Method_HasSharedGenericCodeMapMessage 0xF000000FL
#define MSG_RundownPublisher_Method_JittedMapMessage 0xF0000010L
#define MSG_RundownPublisher_StartupMode_ManagedExeMapMessage 0xF0000011L
#define MSG_RundownPublisher_StartupMode_HostedCLRMapMessage 0xF0000012L
#define MSG_RundownPublisher_StartupMode_IjwDllMapMessage 0xF0000013L
#define MSG_RundownPublisher_StartupMode_ComActivatedMapMessage 0xF0000014L
#define MSG_RundownPublisher_StartupMode_OtherMapMessage 0xF0000015L
#define MSG_RundownPublisher_RuntimeSku_DesktopCLRMapMessage 0xF0000016L
#define MSG_RundownPublisher_RuntimeSku_CoreCLRMapMessage 0xF0000017L
#define MSG_RundownPublisher_Startup_CONCURRENT_GCMapMessage 0xF0000018L
#define MSG_RundownPublisher_Startup_LOADER_OPTIMIZATION_SINGLE_DOMAINMapMessage 0xF0000019L
#define MSG_RundownPublisher_Startup_LOADER_OPTIMIZATION_MULTI_DOMAINMapMessage 0xF000001AL
#define MSG_RundownPublisher_Startup_LOADER_SAFEMODEMapMessage 0xF000001BL
#define MSG_RundownPublisher_Startup_LOADER_SETPREFERENCEMapMessage 0xF000001CL
#define MSG_RundownPublisher_Startup_SERVER_GCMapMessage 0xF000001DL
#define MSG_RundownPublisher_Startup_HOARD_GC_VMMapMessage 0xF000001EL
#define MSG_RundownPublisher_Startup_SINGLE_VERSION_HOSTING_INTERFACEMapMessage 0xF000001FL
#define MSG_RundownPublisher_Startup_LEGACY_IMPERSONATIONMapMessage 0xF0000020L
#define MSG_RundownPublisher_Startup_DISABLE_COMMITTHREADSTACKMapMessage 0xF0000021L
#define MSG_RundownPublisher_Startup_ALWAYSFLOW_IMPERSONATIONMapMessage 0xF0000022L
#define MSG_RundownPublisher_Startup_TRIM_GC_COMMITMapMessage 0xF0000023L
#define MSG_RundownPublisher_Startup_ETWMapMessage 0xF0000024L
#define MSG_RundownPublisher_Startup_SERVER_BUILDMapMessage 0xF0000025L
#define MSG_RundownPublisher_Startup_ARMMapMessage 0xF0000026L
#define MSG_RundownPublisher_ThreadFlags_GCSpecial 0xF0000027L
#define MSG_RundownPublisher_ThreadFlags_Finalizer 0xF0000028L
#define MSG_RundownPublisher_ThreadFlags_ThreadPoolWorker 0xF0000029L

#endif