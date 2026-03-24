NAME = mgba

ROM_GEN_DIR = roms/gen
AM_PLATFORM_DIR = src/platform/am
ROM_GEN_HDR = $(ROM_GEN_DIR)/roms.h
ROM_GEN_SRC = $(ROM_GEN_DIR)/roms.c
ROM_FILES = $(wildcard roms/*.gba)

INC_PATH += include
INC_PATH += src
INC_PATH += $(ROM_GEN_DIR)

CFLAGS += -DM_CORE_GBA
CFLAGS += -DMINIMAL_CORE=2
CFLAGS += -DBUILD_STATIC
CFLAGS += -DDISABLE_THREADING
CFLAGS += -D_GNU_SOURCE
CFLAGS += -DHAVE_LOCALE
CFLAGS += -DHAVE_STRTOF_L
CFLAGS += -DHAVE_STRDUP
CFLAGS += -DHAVE_STRNDUP
CFLAGS += -DHAVE_VASPRINTF
CFLAGS += -DHAVE_LOCALTIME_R

LDFLAGS_CXX += -lm

ifeq ($(ARCH),native)
CFLAGS += -DENABLE_VFS
CFLAGS += -DENABLE_VFS_FILE
NATIVE_VFS_SRC = src/util/vfs/vfs-file.c
endif

AM_PLATFORM_SRC = $(wildcard $(AM_PLATFORM_DIR)/*.c)

CORE_SRC = \
	src/core/bitmap-cache.c \
	src/core/cache-set.c \
	src/core/cheats.c \
	src/core/config.c \
	src/core/core.c \
	src/core/directories.c \
	src/core/interface.c \
	src/core/lockstep.c \
	src/core/log.c \
	src/core/map-cache.c \
	src/core/mem-search.c \
	src/core/rewind.c \
	src/core/serialize.c \
	src/core/sync.c \
	src/core/thread.c \
	src/core/tile-cache.c \
	src/core/timing.c \
	src/core/library.c

ARM_SRC = \
	src/arm/isa-arm.c \
	src/arm/decoder.c \
	src/arm/decoder-thumb.c \
	src/arm/isa-thumb.c \
	src/arm/decoder-arm.c \
	src/arm/arm.c

GBA_SRC = \
	src/gba/audio.c \
	src/gba/bios.c \
	src/gba/core.c \
	src/gba/dma.c \
	src/gba/gba.c \
	src/gba/hle-bios.c \
	src/gba/io.c \
	src/gba/memory.c \
	src/gba/overrides.c \
	src/gba/savedata.c \
	src/gba/serialize.c \
	src/gba/timer.c \
	src/gba/video.c \
	src/gba/sio.c \
	src/gba/cheats.c \
	src/gba/cart/gpio.c \
	src/gba/cart/matrix.c \
	src/gba/cart/unlicensed.c \
	src/gba/cart/vfame.c \
	src/gba/cart/ereader.c \
	src/gba/renderers/common.c \
	src/gba/renderers/video-software.c \
	src/gba/renderers/software-bg.c \
	src/gba/renderers/software-obj.c \
	src/gba/renderers/software-mode0.c \
	src/gba/renderers/cache-set.c \
	src/gba/sio/lockstep.c \
	src/gba/sio/gbp.c \
	src/gba/sio/dolphin.c \
	src/gba/extra/proxy.c \
	src/gba/cheats/gameshark.c \
	src/gba/cheats/codebreaker.c \
	src/gba/cheats/parv3.c

UTIL_SRC = \
	src/util/audio-buffer.c \
	src/util/audio-resampler.c \
	src/util/circle-buffer.c \
	src/util/configuration.c \
	src/util/crc32.c \
	src/util/formatting.c \
	src/util/geometry.c \
	src/util/hash.c \
	src/util/interpolator.c \
	src/util/memory.c \
	src/util/patch-fast.c \
	src/util/ring-fifo.c \
	src/util/string.c \
	src/util/table.c \
	src/util/vector.c \
	src/util/vfs.c \
	src/util/vfs/vfs-mem.c \
	src/util/sha1.c \
	src/util/md5.c \
	src/util/patch.c \
	src/util/gbk-table.c

THIRD_PARTY_SRC = \
	src/third-party/inih/ini.c \
	src/platform/sdl/stubs.c

SRCS = \
	$(AM_PLATFORM_SRC) \
	$(CORE_SRC) \
	$(ARM_SRC) \
	$(GBA_SRC) \
	$(UTIL_SRC) \
	$(NATIVE_VFS_SRC) \
	$(THIRD_PARTY_SRC) \
	$(ROM_GEN_SRC)

$(ROM_GEN_SRC) $(ROM_GEN_HDR): roms/build-roms.py $(ROM_FILES)
	@python3 roms/build-roms.py

$(AM_PLATFORM_SRC): $(ROM_GEN_HDR)

include $(AM_HOME)/Makefile
