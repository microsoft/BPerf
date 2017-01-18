#!/bin/sh

[ -z "${CORECLR_PATH:-}" ] && CORECLR_PATH=~/coreclr
[ -z "${BuildOS:-}"      ] && BuildOS=Linux
[ -z "${BuildArch:-}"    ] && BuildArch=x64
[ -z "${BuildType:-}"    ] && BuildType=Debug
[ -z "${Output:-}"       ] && Output=BPerfCoreCLRProfiler.so

printf '  CORECLR_PATH : %s\n' "$CORECLR_PATH"
printf '  BuildOS      : %s\n' "$BuildOS"
printf '  BuildArch    : %s\n' "$BuildArch"
printf '  BuildType    : %s\n' "$BuildType"

printf '  Building %s ... ' "$Output"

CORECLR_BUILT_PATH=$CORECLR_PATH/bin/Product/$BuildOS.$BuildArch.$BuildType

CXX_FLAGS="$CXX_FLAGS --no-undefined -Wno-invalid-noreturn -fPIC -fms-extensions -DUNICODE -DBIT64 -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DPLATFORM_UNIX -std=c++11"
INCLUDES="-I $CORECLR_PATH/src/pal/inc/rt -I $CORECLR_PATH/src/pal/prebuilt/inc -I $CORECLR_PATH/src/pal/inc -I $CORECLR_PATH/src/inc -I $CORECLR_BUILT_PATH/inc"

clang -c -fPIC src/sqlite3.c
clang++ -shared -ldl -luuid -lunwind-generic -lpthread -o $Output $CXX_FLAGS $INCLUDES src/*.cpp sqlite3.o $CORECLR_BUILT_PATH/lib/libcoreclrpal.a
rm sqlite3.o
printf 'Done.\n'
