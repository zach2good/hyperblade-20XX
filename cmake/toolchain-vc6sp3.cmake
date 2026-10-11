# Visual C++ 6.0 SP3, the toolchain that built HYPERX.EXE v1.0.0.1.
#
# VC6_ROOT names the tree that holds VC98 and Common. Pass -DVC6_ROOT=<path> or set it in the
# environment. The paths below come from it, not from INCLUDE, LIB or PATH.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

if(NOT VC6_ROOT AND DEFINED ENV{VC6_ROOT})
    file(TO_CMAKE_PATH "$ENV{VC6_ROOT}" VC6_ROOT)
endif()

if(NOT VC6_ROOT)
    message(FATAL_ERROR "VC6_ROOT is not set. Set it in the environment or pass -DVC6_ROOT=<path>.")
endif()

set(VC6_ROOT "${VC6_ROOT}" CACHE PATH "Root of the VC6 SP3 tree")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES VC6_ROOT)

set(VC6_BIN "${VC6_ROOT}/VC98/Bin")
set(VC6_IDE_BIN "${VC6_ROOT}/Common/MSDev98/Bin")

foreach(required
        "${VC6_BIN}/CL.EXE"
        "${VC6_BIN}/C2.DLL"
        "${VC6_BIN}/LINK.EXE"
        "${VC6_BIN}/CVTRES.EXE"
        "${VC6_BIN}/NMAKE.EXE"
        "${VC6_IDE_BIN}/RC.EXE")
    if(NOT EXISTS "${required}")
        message(FATAL_ERROR "VC6 SP3 tree incomplete: ${required} not found.")
    endif()
endforeach()

set(CMAKE_MAKE_PROGRAM "${VC6_BIN}/NMAKE.EXE" CACHE FILEPATH "VC6 NMAKE")

set(CMAKE_C_COMPILER "${VC6_BIN}/CL.EXE")
set(CMAKE_CXX_COMPILER "${VC6_BIN}/CL.EXE")
set(CMAKE_LINKER "${VC6_BIN}/LINK.EXE")
set(CMAKE_RC_COMPILER "${VC6_IDE_BIN}/RC.EXE")
set(CMAKE_AR "${VC6_BIN}/LIB.EXE")
set(CMAKE_MT "")

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${VC6_ROOT}/VC98/Include")
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${VC6_ROOT}/VC98/Include")
set(CMAKE_RC_FLAGS_INIT "/i \"${VC6_ROOT}/VC98/Include\"")

# The flags every C and C++ unit shares. CMakeLists.txt forces them after project(), and sets the
# optimisation per source.
set(VC6_COMPILE_FLAGS "/nologo /MT /GX /GR- /Zp8 /Zi /W3 /DWIN32 /D_WINDOWS /DNDEBUG")

# Link 6.00.8447 defaults FileAlignment to 0x200 and the original has 0x1000.
set(VC6_LINKER_FLAGS
    "/nologo /LIBPATH:\"${VC6_ROOT}/VC98/Lib\" /SUBSYSTEM:WINDOWS,4.0 /FIXED /BASE:0x400000 /FILEALIGN:4096 /INCREMENTAL:NO /OPT:REF /machine:X86")

# The same strings seed CMake's compiler test, which otherwise has no /LIBPATH.
set(CMAKE_C_FLAGS_INIT "${VC6_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${VC6_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${VC6_LINKER_FLAGS}")
set(ENV{INCLUDE} "${VC6_ROOT}/VC98/Include")
set(ENV{LIB} "${VC6_ROOT}/VC98/Lib")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
