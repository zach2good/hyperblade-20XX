# Third-party packages, fetched by CPM into one cache outside the build tree, so a new build
# directory downloads nothing. Set CPM_SOURCE_CACHE in the environment to share the cache.

set(CPM_VERSION 0.43.2)
set(CPM_HASH 49a3bef91ceb65bb66d57255e12d1ffd22abc2f6408fa9fe4534c544a2f232aa)

if(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE "$ENV{CPM_SOURCE_CACHE}" CACHE PATH "Where CPM keeps its downloads")
else()
    set(CPM_SOURCE_CACHE "${PROJECT_SOURCE_DIR}/.cpm-cache" CACHE PATH "Where CPM keeps its downloads")
endif()

set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_VERSION}.cmake")

if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
        EXPECTED_HASH SHA256=${CPM_HASH}
        TLS_VERIFY ON)
endif()

include("${CPM_DOWNLOAD_LOCATION}")

set(SDL3_VERSION 3.4.18)

if(WIN32)
    # SDL3's own MSVC build, with its import library, DLL and CMake package files.
    CPMAddPackage(
        NAME SDL3
        VERSION ${SDL3_VERSION}
        URL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-devel-${SDL3_VERSION}-VC.zip"
        URL_HASH SHA256=78d84602ae616cfe26b33a73b7d3b9b6b56e8a6650e53d92c9c6ca938f747acd
        DOWNLOAD_ONLY YES)

    find_package(SDL3 ${SDL3_VERSION} EXACT REQUIRED CONFIG PATHS "${SDL3_SOURCE_DIR}/cmake" NO_DEFAULT_PATH)
else()
    # SDL3 built from source as a shared library.
    CPMAddPackage(
        NAME SDL3
        VERSION ${SDL3_VERSION}
        URL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz"
        URL_HASH SHA256=9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
        OPTIONS "SDL_SHARED ON" "SDL_STATIC OFF" "SDL_TEST_LIBRARY OFF" "SDL_EXAMPLES OFF")
endif()

# miniaudio mixes the game's sounds and decodes its music, and SDL plays the result, so its
# device backends are left out.
set(MINIAUDIO_VERSION 0.11.22)

CPMAddPackage(
    NAME miniaudio
    VERSION ${MINIAUDIO_VERSION}
    URL "https://github.com/mackron/miniaudio/archive/refs/tags/${MINIAUDIO_VERSION}.tar.gz"
    URL_HASH SHA256=bcb07bfb27e6fa94d34da73ba2d5642d4940b208ec2a660dbf4e52e6b7cd492f
    DOWNLOAD_ONLY YES)

add_library(miniaudio STATIC "${miniaudio_SOURCE_DIR}/miniaudio.c")
target_include_directories(miniaudio SYSTEM PUBLIC "${miniaudio_SOURCE_DIR}")
target_compile_definitions(miniaudio PUBLIC MA_NO_DEVICE_IO MA_NO_RESOURCE_MANAGER MA_NO_ENCODING MA_NO_GENERATION)

# DXC compiles the HLSL shaders to DXIL and SPIR-V at build time. The release zip is named by
# date, so the URL is pinned by hand alongside the version.
set(DXC_VERSION 1.9.2607)

if(WIN32)
    set(DXC_ARCHIVE dxc_2026_07_29.zip)
    set(DXC_HASH a1dfb116ba3eeae6a1582291b53a8e7bf65ad760676bd3194685c8f7367cd241)
    set(DXC_BIN bin/x64)
else()
    set(DXC_ARCHIVE linux_dxc_2026_07_29.x86_x64.tar.gz)
    set(DXC_HASH 55665c87824051ed4774ff3280a79ccbbb7d39243b9736ca5e98222134112d54)
    set(DXC_BIN bin)
endif()

CPMAddPackage(
    NAME dxc
    VERSION ${DXC_VERSION}
    URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v${DXC_VERSION}/${DXC_ARCHIVE}"
    URL_HASH SHA256=${DXC_HASH}
    DOWNLOAD_ONLY YES)

find_program(DXC_EXECUTABLE dxc PATHS "${dxc_SOURCE_DIR}/${DXC_BIN}" NO_DEFAULT_PATH REQUIRED)

# Dear ImGui draws the debug menu, through SDL's renderer.
set(IMGUI_VERSION 1.92.9b)

CPMAddPackage(
    NAME imgui
    VERSION ${IMGUI_VERSION}
    URL "https://github.com/ocornut/imgui/archive/refs/tags/v${IMGUI_VERSION}.tar.gz"
    URL_HASH SHA256=21d8a0a565e85dce943e375db00812c2f3f0ab21f3f0f7964e364a63422d7f99
    DOWNLOAD_ONLY YES)

add_library(imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp")
target_include_directories(imgui SYSTEM PUBLIC "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_link_libraries(imgui PUBLIC SDL3::SDL3)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

# stb ships in third_party/, header only.
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/third_party/stb")

# libadlib decompiles and compiles the plans. The fixes in res/plans are edits to its decompiled
# source, so it is pinned by commit. Its own build wants POSIX, so only the compiler and decompiler
# are built, with a patch for the one dirent.h use.
set(LIBADLIB_COMMIT b1d307d7f881ec3bba2933aa69cfdf29603264a0)

CPMAddPackage(
    NAME libadlib
    URL "https://github.com/vomitselfie/libadlib/archive/${LIBADLIB_COMMIT}.tar.gz"
    URL_HASH SHA256=09f639b77aae6d7f564d743449d2ca3c7a9db60f884a6433af453d4f2ba77c73
    PATCHES "${PROJECT_SOURCE_DIR}/cmake/libadlib-msvc.patch"
    DOWNLOAD_ONLY YES)

add_library(adlib STATIC
    "${libadlib_SOURCE_DIR}/src/vocabulary.cpp"
    "${libadlib_SOURCE_DIR}/src/pbn.cpp"
    "${libadlib_SOURCE_DIR}/src/pbn_writer.cpp"
    "${libadlib_SOURCE_DIR}/src/compiler/numbers.cpp"
    "${libadlib_SOURCE_DIR}/src/compiler/symbols.cpp"
    "${libadlib_SOURCE_DIR}/src/compiler/compiler.cpp"
    "${libadlib_SOURCE_DIR}/src/compiler/strict.cpp"
    "${libadlib_SOURCE_DIR}/src/tools/symbols.cpp"
    "${libadlib_SOURCE_DIR}/src/tools/decompile.cpp")
target_include_directories(adlib SYSTEM PUBLIC "${libadlib_SOURCE_DIR}/include")
target_compile_features(adlib PUBLIC cxx_std_17)
target_compile_options(adlib PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>)
