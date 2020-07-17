// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    internal enum BPFCmd
    {
        BPF_MAP_CREATE,
        BPF_MAP_LOOKUP_ELEM,
        BPF_MAP_UPDATE_ELEM,
        BPF_MAP_DELETE_ELEM,
        BPF_MAP_GET_NEXT_KEY,
        BPF_PROG_LOAD,
        BPF_OBJ_PIN,
        BPF_OBJ_GET,
        BPF_PROG_ATTACH,
        BPF_PROG_DETACH,
        BPF_PROG_TEST_RUN,
        BPF_PROG_GET_NEXT_ID,
        BPF_MAP_GET_NEXT_ID,
        BPF_PROG_GET_FD_BY_ID,
        BPF_MAP_GET_FD_BY_ID,
        BPF_OBJ_GET_INFO_BY_FD,
        BPF_PROG_QUERY,
        BPF_RAW_TRACEPOINT_OPEN,
        BPF_BTF_LOAD,
        BPF_BTF_GET_FD_BY_ID,
        BPF_TASK_FD_QUERY,
        BPF_MAP_LOOKUP_AND_DELETE_ELEM,
        BPF_MAP_FREEZE,
        BPF_BTF_GET_NEXT_ID,
        BPF_MAP_LOOKUP_BATCH,
        BPF_MAP_LOOKUP_AND_DELETE_BATCH,
        BPF_MAP_UPDATE_BATCH,
        BPF_MAP_DELETE_BATCH,
    }
}
