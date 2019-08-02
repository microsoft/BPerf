#pragma once

#ifndef EVENT_DESCRIPTOR_DEF
#define EVENT_DESCRIPTOR_DEF

/*
EVENT_DESCRIPTOR describes and categorizes an event.
Note that for TraceLogging events, the Id and Version fields are not
meaningful and should be ignored.
*/
typedef struct _EVENT_DESCRIPTOR {

    USHORT Id; /*
        For manifest-based events, the Provider.Guid + Event.Id + Event.Version
        should uniquely identify an event. Once a manifest with a particular
        event Id+Version has been made public, the definition of that event
        (the types, ordering, and semantics of the fields) should never be
        changed. If an event needs to be changed, it must be given a new
        identity (usually by incrementing the Version), and the original event
        must remain in the manifest (so that older versions of the event can
        still be decoded with the new manifest). To change an event (e.g. to
        add/remove a field or to change a field type): duplicate the event in
        the manifest, then increment the event Version and make changes in the
        new copy.
        For manifest-free events (i.e. TraceLogging), Event.Id and
        Event.Version are not useful and should be ignored. Use Event name,
        level, keyword, and opcode for event filtering and identification. */
    UCHAR Version; /*
        For manifest-based events, the Provider.Guid + Event.Id + Event.Version
        should uniquely identify an event. The Id+Version constitute a 24-bit
        identifier. Generally, events with the same Id are semantically
        related, and the Version is incremented as the event is refined over
        time. */
    UCHAR Channel; /*
        The meaning of the Channel field depends on the event consumer.
        This field is most commonly used with events that will be consumed by
        the Windows Event Log. Note that Event Log does not listen to all ETW
        events, so setting a channel is not enough to make the event appear in
        the Event Log. For an ETW event to be routed to Event Log, the
        following must be configured:
        - The provider and its channels must be defined in a manifest.
        - The manifest must be compiled with the mc.exe tool, and the resulting
          BIN files must be included into the resources of an EXE or DLL.
        - The EXE or DLL containing the BIN data must be installed on the
          machine where the provider will run.
        - The manifest must be registered (using wevtutil.exe) on the machine
          where the provider will run. The manifest registration process must
          record the location of the EXE or DLL with the BIN data.
        - The channel must be enabled in Event Log configuration.
        - The provider must log an event with the channel and keyword set
          correctly as defined in the manifest. (Note that the mc.exe code
          generator will automatically define an implicit keyword for each
          channel, and will automatically add the channel's implicit keyword to
          each event that references a channel.) */
    UCHAR Level; /*
        The event level defines the event's severity or importance and is a
        primary means for filtering events. Microsoft-defined levels (in
        evntrace.h and  winmeta.h) are 1 (critical/fatal), 2 (error),
        3 (warning), 4 (information), and 5 (verbose). Levels 6-9 are reserved.
        Level 0 means the event is always-on (will not be filtered by level).
        For a provider, a lower level means the event is more important. An
        event with level 0 will always pass any level-based filtering.
        For a consumer, a lower level means the session's filter is more
        restrictive. However, setting a session's level to 0 disables level
        filtering (i.e. session level 0 is the same as session level 255). */
    UCHAR Opcode; /*
        The event opcode is used to mark events with special semantics that
        may be used by event decoders to organize and correlate events.
        Globally-recognized opcode values are defined in winmeta.h. A provider
        can define its own opcodes. Most events use opcode 0 (information).
        The opcodes 1 (start) and 2 (stop) are used to indicate the beginning
        and end of an activity as follows:
        - Generate a new activity Id (UuidCreate or EventActivityIdControl).
        - Write an event with opcode = start, activity ID = (the generated
          activity ID), and related activity ID = (the parent activity if any).
        - Write any number of informational events with opcode = info, activity
          ID = (the generated activity ID).
        - Write a stop event with opcode = stop, activity ID = (the generated
          activity ID).
        Each thread has an implicit activity ID (in thread-local storage) that
        will be applied to any event that does not explicitly specify an
        activity ID. The implicit activity ID can be accessed using
        EventActivityIdControl. It is intended that the thread-local activity
        will be used to implement scope-based activities: on entry to a scope
        (i.e. at the start of a function), a user will record the existing
        value of the implicit activity ID, generate and set a new value, and
        write a start event; on exit from the scope, the user will write a stop
        event and restore the previous activity ID. Note that there is no enforcement
        of this pattern, and an application must be aware that other code may
        potentially overwrite the activity ID without restoring it. In
        addition, the implicit activity ID does not work well with cross-thread
        activities. For these reasons, it may be more appropriate to use
        explicit activity IDs (explicitly pass a GUID to EventWriteTransfer)
        instead of relying on the implicity activity ID. */
    USHORT      Task; /*
        The event task code can be used for any purpose as defined by the
        provider. The task code 0 is the default, used to indicate that no
        special task code has been assigned to the event. The ETW manifest
        supports assigning localizable strings for each task code. The task
        code might be used to group events into categories, or to simply
        associate a task name with each event. */
    ULONGLONG   Keyword; /*
        The event keyword defines membership in various categories and is an
        important means for filtering events. The event's keyword is a set of
        64 bits indicating the categories to which an event belongs. The
        provider manifest may provide definitions for up to 48 keyword values,
        each value defining the meaning of a single keyword bit (the upper 16
        bits are reserved by Microsoft for special purposes). For example, if
        the provider manifest defines keyword 0x0010 as "Networking", and
        defines keyword 0x0020 as "Threading", an event with keyword 0x0030
        would be in both "Networking" and "Threading" categories, while an
        event with keyword 0x0001 would be in neither category. An event with
        keyword 0 is treated as uncategorized.
        Event consumers can use keyword masks to determine which events should
        be included in the log. A session can define a KeywordAny mask and
        a KeywordAll mask. An event will pass the session's keyword filtering
        if the following expression is true:
            event.Keyword == 0 || (
            (event.Keyword & session.KeywordAny) != 0 &&
            (event.Keyword & session.KeywordAll) == session.KeywordAll).
        In other words, uncategorized events (events with no keywords set)
        always pass keyword filtering, and categorized events pass if they
        match any keywords in KeywordAny and match all keywords in KeywordAll.
        */

} EVENT_DESCRIPTOR, * PEVENT_DESCRIPTOR;

typedef const EVENT_DESCRIPTOR* PCEVENT_DESCRIPTOR;

#endif

#ifndef EVENT_HEADER_DEF
#define EVENT_HEADER_DEF
typedef struct _EVENT_HEADER {

    USHORT              Size;                   // Event Size
    USHORT              HeaderType;             // Header Type
    USHORT              Flags;                  // Flags
    USHORT              EventProperty;          // User given event property
    ULONG               ThreadId;               // Thread Id
    ULONG               ProcessId;              // Process Id
    LARGE_INTEGER       TimeStamp;              // Event Timestamp
    GUID                ProviderId;             // Provider Id
    EVENT_DESCRIPTOR    EventDescriptor;        // Event Descriptor
    union {
        struct {
            ULONG       KernelTime;             // Kernel Mode CPU ticks
            ULONG       UserTime;               // User mode CPU ticks
        } DUMMYSTRUCTNAME;
        ULONG64         ProcessorTime;          // Processor Clock
                                                // for private session events
    } DUMMYUNIONNAME;
    GUID                ActivityId;             // Activity Id

} EVENT_HEADER, * PEVENT_HEADER;
#endif

#ifndef ETW_BUFFER_CONTEXT_DEF
#define ETW_BUFFER_CONTEXT_DEF
typedef struct _ETW_BUFFER_CONTEXT {
    union {
        struct {
            UCHAR ProcessorNumber;
            UCHAR Alignment;
        } DUMMYSTRUCTNAME;
        USHORT ProcessorIndex;
    } DUMMYUNIONNAME;
    USHORT  LoggerId;
} ETW_BUFFER_CONTEXT, * PETW_BUFFER_CONTEXT;
#endif

#ifndef EVENT_HEADER_EXTENDED_DATA_ITEM_DEF
#define EVENT_HEADER_EXTENDED_DATA_ITEM_DEF
typedef struct _EVENT_HEADER_EXTENDED_DATA_ITEM {

    USHORT      Reserved1;                      // Reserved for internal use
    USHORT      ExtType;                        // Extended info type
    struct {
        USHORT  Linkage : 1;       // Indicates additional extended
                                                // data item
        USHORT  Reserved2 : 15;
    };
    USHORT      DataSize;                       // Size of extended info data
    ULONGLONG   DataPtr;                        // Pointer to extended info data

} EVENT_HEADER_EXTENDED_DATA_ITEM, * PEVENT_HEADER_EXTENDED_DATA_ITEM;
#endif

#ifndef EVENT_RECORD_DEF
#define EVENT_RECORD_DEF
typedef struct _EVENT_RECORD {

    EVENT_HEADER        EventHeader;            // Event header
    ETW_BUFFER_CONTEXT  BufferContext;          // Buffer context
    USHORT              ExtendedDataCount;      // Number of extended
                                                // data items
    USHORT              UserDataLength;         // User data length
    PEVENT_HEADER_EXTENDED_DATA_ITEM            // Pointer to an array of
        ExtendedData;           // extended data items
    PVOID               UserData;               // Pointer to user data
    PVOID               UserContext;            // Context from OpenTrace
} EVENT_RECORD, * PEVENT_RECORD;

#endif

#define EVENT_HEADER_PROPERTY_XML               0x0001
#define EVENT_HEADER_FLAG_EXTENDED_INFO         0x0001
#define EVENT_HEADER_FLAG_32_BIT_HEADER         0x0020
#define EVENT_HEADER_FLAG_64_BIT_HEADER         0x0040
#define EVENT_HEADER_FLAG_PROCESSOR_INDEX       0x0200

struct EVENT_DATA_DESCRIPTOR
{
    ULONGLONG Ptr;
    ULONG Size;
    ULONG Reserved;
};
