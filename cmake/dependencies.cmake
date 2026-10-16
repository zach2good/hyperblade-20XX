# Third-party packages, fetched by CPM into one cache outside the build tree, so a new build
# directory downloads nothing. Set CPM_SOURCE_CACHE in the environment to share the cache.

set(CPM_VERSION 0.43.2)
set(CPM_HASH 49a3bef91ceb65bb66d57255e12d1ffd22abc2f6408fa9fe4534c544a2f232aa)

if(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE "$ENV{CPM_SOURCE_CACHE}" CACHE PATH "Where CPM keeps its downloads")
else()
    set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm-cache" CACHE PATH "Where CPM keeps its downloads")
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

# SDL3's own MSVC build, with its import library, DLL and CMake package files.
set(SDL3_VERSION 3.4.18)

CPMAddPackage(
    NAME SDL3
    VERSION ${SDL3_VERSION}
    URL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-devel-${SDL3_VERSION}-VC.zip"
    URL_HASH SHA256=78d84602ae616cfe26b33a73b7d3b9b6b56e8a6650e53d92c9c6ca938f747acd
    DOWNLOAD_ONLY YES)

find_package(SDL3 ${SDL3_VERSION} EXACT REQUIRED CONFIG PATHS "${SDL3_SOURCE_DIR}/cmake" NO_DEFAULT_PATH)

# miniaudio mixes the PCM the game loads and SDL plays it, so its device backends are left out.
set(MINIAUDIO_VERSION 0.11.22)

CPMAddPackage(
    NAME miniaudio
    VERSION ${MINIAUDIO_VERSION}
    URL "https://github.com/mackron/miniaudio/archive/refs/tags/${MINIAUDIO_VERSION}.tar.gz"
    URL_HASH SHA256=bcb07bfb27e6fa94d34da73ba2d5642d4940b208ec2a660dbf4e52e6b7cd492f
    DOWNLOAD_ONLY YES)

add_library(miniaudio STATIC "${miniaudio_SOURCE_DIR}/miniaudio.c")
target_include_directories(miniaudio SYSTEM PUBLIC "${miniaudio_SOURCE_DIR}")
target_compile_definitions(miniaudio PUBLIC MA_NO_DEVICE_IO MA_NO_DECODING MA_NO_ENCODING MA_NO_GENERATION)
