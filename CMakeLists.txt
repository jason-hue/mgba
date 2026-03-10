cmake_minimum_required(VERSION 3.12)
project(mGBA)

set(BINARY_NAME mgba CACHE STRING "Name of output binaries")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(LIBMGBA_ONLY ON)
set(M_CORE_GBA ON)
set(M_CORE_GB OFF)
set(ENABLE_DEBUGGERS OFF)
set(BUILD_STATIC ON)
set(BUILD_SHARED OFF)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/include)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Core mGBA Sources
set(CORE_SRC
    src/core/bitmap-cache.c
    src/core/cache-set.c
    src/core/cheats.c
    src/core/config.c
    src/core/core.c
    src/core/directories.c
    src/core/interface.c
    src/core/lockstep.c
    src/core/log.c
    src/core/map-cache.c
    src/core/mem-search.c
    src/core/rewind.c
    src/core/serialize.c
    src/core/sync.c
    src/core/thread.c
    src/core/tile-cache.c
    src/core/timing.c
    src/core/library.c
)

# ARM7TDMI Core Sources
set(ARM_SRC
    src/arm/isa-arm.c
    src/arm/decoder.c
    src/arm/decoder-thumb.c
    src/arm/isa-thumb.c
    src/arm/decoder-arm.c
    src/arm/arm.c
)

# GBA Implementation Sources
set(GBA_SRC
    src/gba/audio.c
    src/gba/bios.c
    src/gba/core.c
    src/gba/dma.c
    src/gba/gba.c
    src/gba/hle-bios.c
    src/gba/io.c
    src/gba/memory.c
    src/gba/overrides.c
    src/gba/savedata.c
    src/gba/serialize.c
    src/gba/timer.c
    src/gba/video.c
    src/gba/sio.c
    src/gba/cheats.c
    src/gba/cart/gpio.c
    src/gba/cart/matrix.c
    src/gba/cart/unlicensed.c
    src/gba/cart/vfame.c
    src/gba/cart/ereader.c
    src/gba/renderers/common.c
    src/gba/renderers/video-software.c
    src/gba/renderers/software-bg.c
    src/gba/renderers/software-obj.c
    src/gba/renderers/software-mode0.c
    src/gba/renderers/cache-set.c
    src/gba/sio/lockstep.c
    src/gba/sio/gbp.c
    src/gba/sio/dolphin.c
    src/gba/extra/proxy.c
    src/gba/cheats/gameshark.c
    src/gba/cheats/codebreaker.c
    src/gba/cheats/parv3.c
)

# Utility Sources
set(UTIL_SRC
    src/util/audio-buffer.c
    src/util/audio-resampler.c
    src/util/circle-buffer.c
    src/util/configuration.c
    src/util/crc32.c
    src/util/formatting.c
    src/util/geometry.c
    src/util/hash.c
    src/util/memory.c
    src/util/ring-fifo.c
    src/util/string.c
    src/util/table.c
    src/util/vector.c
    src/util/vfs.c
    src/util/vfs/vfs-mem.c
    src/util/vfs/vfs-fifo.c
    src/util/vfs/vfs-fd.c
    src/util/sha1.c
    src/util/md5.c
    src/util/patch.c
    src/util/gbk-table.c
)

if(UNIX)
    add_definitions(-D_GNU_SOURCE)
    add_definitions(-DHAVE_LOCALE -DHAVE_STRTOF_L -DHAVE_STRDUP -DHAVE_STRNDUP -DHAVE_VASPRINTF)
    set(OS_LIB m pthread)
endif()

set(THIRD_PARTY_SRC src/third-party/inih/ini.c)

include(version.cmake)
configure_file(src/core/version.c.in ${CMAKE_CURRENT_BINARY_DIR}/version.c)

# Include the stubs to satisfy the linker
set(FULL_SRC
    ${CORE_SRC}
    ${ARM_SRC}
    ${GBA_SRC}
    ${UTIL_SRC}
    ${THIRD_PARTY_SRC}
    src/platform/sdl/stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/version.c
)

add_library(${BINARY_NAME} STATIC ${FULL_SRC})
target_compile_definitions(${BINARY_NAME} PRIVATE 
    M_CORE_GBA 
    BUILD_STATIC 
    ENABLE_VFS_FD 
    MINIMAL_CORE=2
)

find_package(SDL2 REQUIRED)
add_subdirectory(src/platform/sdl)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/mgba)
configure_file(src/core/flags.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/mgba/flags.h)
