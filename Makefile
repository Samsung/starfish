BUILDDIR=./build
HOST=linux

BIN=StarFish
LIB=libStarFish.so

################################################################################
################################################################################
# Environments
################################################################################
################################################################################

ARCH=
TYPE=
MODE=#debug,release
NPROCS:=1
OS:=$(shell uname -s)
SHELL:=/bin/bash
OUTPUT:=bin
TIZEN_DEVICE_API=
LTO=
BACKEND=EFL
#BACKEND=EFL_CAIRO
#BACKEND=DALI
ifeq ($(OS),Linux)
  NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
  SHELL:=/bin/bash
endif
ifeq ($(OS),Darwin)
  NPROCS:=$(shell sysctl -n machdep.cpu.thread_count)
  SHELL:=/opt/local/bin/bash
endif
TEST_NPROCS?=$(NPROCS)

# Set media flag (only works on x64)
MEDIA_SUPPORT=false

# multipage
MULTIPAGE_SUPPORT=false

# inspector
INSPECTOR_SUPPORT=false

# dom parser
DOMPARSER_SUPPORT=false

# WASU
WASU_SUPPORT=false

# binding generator
BINDING_GENERATOR_SUPPORT=true

$(info goal... $(MAKECMDGOALS))

ifneq (,$(findstring x86,$(MAKECMDGOALS)))
  ARCH=x86
else ifneq (,$(findstring x64,$(MAKECMDGOALS)))
  ARCH=x64
else ifneq (,$(findstring arm,$(MAKECMDGOALS)))
  ARCH=arm
endif

ifneq (,$(findstring tizen_wearable_arm,$(MAKECMDGOALS)))
  HOST=tizen_wearable_arm
  TIZEN_ARCH=arm
  TIZEN_VERSION=2.3.1
  TIZEN_PROFILE=wearable
  TIZEN_DEVICE=device
else ifneq (,$(findstring tizen3_wearable_arm,$(MAKECMDGOALS)))
  HOST=tizen3_wearable_arm
  TIZEN_ARCH=arm
  TIZEN_VERSION=3.0
  TIZEN_PROFILE=wearable
  TIZEN_DEVICE=device
else ifneq (,$(findstring tizen_mobile_arm,$(MAKECMDGOALS)))
  HOST=tizen_mobile_arm
  TIZEN_ARCH=arm
  TIZEN_VERSION=2.3.1
  TIZEN_PROFILE=mobile
  TIZEN_DEVICE=device
else ifneq (,$(findstring tizen3_mobile_arm,$(MAKECMDGOALS)))
  HOST=tizen3_mobile_arm
  TIZEN_ARCH=arm
  TIZEN_VERSION=3.0
  TIZEN_PROFILE=mobile
  TIZEN_DEVICE=device
else ifneq (,$(findstring tizen_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen_wearable_emulator
  ARCH=x86
  TIZEN_ARCH=i386
  TIZEN_VERSION=2.3.1
  TIZEN_PROFILE=wearable
  TIZEN_DEVICE=emulator
else ifneq (,$(findstring tizen24_mobile_emulator,$(MAKECMDGOALS)))
  HOST=tizen24_mobile_emulator
  ARCH=x86
  TIZEN_ARCH=i386
  TIZEN_VERSION=2.4
  TIZEN_PROFILE=mobile
  TIZEN_DEVICE=emulator
else ifneq (,$(findstring tizen3_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen3_wearable_emulator
  ARCH=x86
  TIZEN_ARCH=i386
  TIZEN_VERSION=3.0
  TIZEN_PROFILE=wearable
  TIZEN_DEVICE=emulator
else ifneq (,$(findstring tizen24_mobile_arm,$(MAKECMDGOALS)))
  HOST=tizen24_mobile_arm
  TIZEN_ARCH=arm
  TIZEN_VERSION=2.4
  TIZEN_PROFILE=mobile
  TIZEN_DEVICE=device
else ifneq (,$(findstring tizen_obs_arm,$(MAKECMDGOALS)))
  HOST=tizen_obs
  TIZEN_ARCH=arm
else ifneq (,$(findstring tizen_obs_emulator,$(MAKECMDGOALS)))
  HOST=tizen_obs
  ARCH=x64
  TIZEN_ARCH=x86_64
endif

ifneq (,$(findstring exe,$(MAKECMDGOALS)))
  TYPE=exe
endif

ifneq (,$(findstring lib,$(MAKECMDGOALS)))
  TYPE=lib
endif

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  MODE=debug
else ifneq (,$(findstring release,$(MAKECMDGOALS)))
  MODE=release
endif

ifeq ($(HOST), linux)
  OUTDIR=out/$(ARCH)/$(TYPE)/$(MODE)
else ifeq ($(HOST), tizen_obs)
  OUTDIR=out/tizen_obs/$(ARCH)/$(TYPE)/$(MODE)
else ifneq (,$(findstring tizen,$(HOST)))
  OUTDIR=out/tizen_$(TIZEN_VERSION)/$(ARCH)/$(TYPE)/$(MODE)
endif

ifneq (,$(findstring tizen,$(HOST)))
  #LTO=1
  ifeq ($(TYPE), lib)
    # At present, this feature is not required.
    TIZEN_DEVICE_API=true
  endif
endif

AUTOGEN_SRC=
AUTOGEN_DEPENDENCY=
AUTOGEN_DIR_TMP=out/binding
AUTOGEN_DIR=src/binding

ifeq ($(BINDING_GENERATOR_SUPPORT), true)
  AUTOGEN_DEPENDENCY+=.git/modules/binding_generator/HEAD
  AUTOGEN_DEPENDENCY+=$(shell find src/ -type f -name *.idl)
  ifneq ($(ARCH),)
    GEN_JSBINGING_RESULTS:=$(shell mkdir -p $(AUTOGEN_DIR_TMP) && ./binding_generator/scripts/starfish_code_generator.py src/ $(AUTOGEN_DIR_TMP))
    AUTOGEN_SRC:=$(shell find $(AUTOGEN_DIR_TMP) -type f -name *.cpp | sed 's/out\//src\//')
  endif
endif

$(info host... $(HOST))
$(info arch... $(ARCH))
$(info type... $(TYPE))
$(info mode... $(MODE))
$(info build dir... $(OUTDIR))

################################################################################
################################################################################
# Global build flags
################################################################################
################################################################################

# common flags
CXXFLAGS += -std=c++0x -g3
CXXFLAGS += -fno-math-errno -Isrc/ -Iinc/
CXXFLAGS += -fdata-sections -ffunction-sections
CXXFLAGS += -frounding-math -fsignaling-nans
CXXFLAGS += -Wno-invalid-offsetof -fvisibility=hidden
CXXFLAGS += -fno-omit-frame-pointer -fstack-protector
CXXFLAGS += -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-unused-parameter -Wno-unused-result
CXXFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations

ifeq ($(BACKEND), EFL)
	CXXFLAGS += -DSTARFISH_EFL
	CXXFLAGS += -fno-rtti
else ifeq ($(BACKEND), EFL_CAIRO)
	CXXFLAGS += -DSTARFISH_EFL_CAIRO
	CXXFLAGS += -fno-rtti
	LDFLAGS += -lturbojpeg -lgif
else ifeq ($(BACKEND), DALI)
	CXXFLAGS += -DSTARFISH_DALI
endif
LDFLAGS += -lpthread -lcurl

# fixme
# this causes
# /home/ksh8281/tizen-sdk-2.4.r2/tools/arm-linux-gnueabi-gcc-4.9/bin/../lib/gcc/arm-linux-gnueabi/4.9.2/../../../../arm-linux-gnueabi/bin/ld: BFD (GNU Binutils) 2.22 assertion fail ../../bfd/elf32-arm.c:12049
# LDFLAGS += -Wl,--gc-sections

ifeq ($(ARCH), x64)
  INSPECTOR_SUPPORT=true
  MEDIA_SUPPORT=true
  DOMPARSER_SUPPORT=true
  MULTIPAGE_SUPPORT=true
  CXXFLAGS += -DSTARFISH_ENABLE_TEST
else ifeq ($(ARCH), x86)
  CXXFLAGS += -m32 -mfpmath=sse -msse2
  LDFLAGS += -m32
else ifeq ($(ARCH), arm)
  ifeq ($(HOST), tizen_obs)
    CXXFLAGS += -mthumb
  else
    CXXFLAGS += -march=armv7-a -mthumb
  endif
  ifeq ($(MODE), debug)
    CXXFLAGS += -DSTARFISH_ENABLE_TEST
  endif
endif

ifeq ($(MODE), debug)
  CXXFLAGS += $(CXXFLAGS_DEBUG)
else ifeq ($(MODE), release)
  CXXFLAGS += $(CXXFLAGS_RELEASE)
endif

# flags for debug/release
CXXFLAGS_DEBUG = -O0 -D_GLIBCXX_DEBUG -Wall -Wextra -Werror
CXXFLAGS_RELEASE = -O2 -DNDEBUG -funswitch-loops

# flags for shared library
ifeq ($(TYPE), lib)
  CXXFLAGS += -fPIC
  CFLAGS += -fPIC
endif

# flags for LTO
ifeq ($(LTO), 1)
  CXXFLAGS += -flto -ffat-lto-objects
endif

# flags for tizen
ifneq (,$(findstring tizen,$(HOST)))
  CXXFLAGS += -Os -finline-limit=64
  CXXFLAGS += -DSTARFISH_TIZEN

  ifeq ($(BACKEND), DALI)
    CXXFLAGS += -DGC_THREADS
  endif

  #CXXFLAGS_DEBUG += -Wno-literal-suffix

  ifeq ($(TIZEN_VERSION), 3.0)
    CXXFLAGS += -DSTARFISH_TIZEN_3_0
  else ifeq ($(TIZEN_VERSION), 2.4)
    CXXFLAGS += -DSTARFISH_TIZEN_2_4
  endif

  ifeq ($(HOST),tizen_obs)
    CXXFLAGS_DEBUG += -O1 # _FORTIFY_SOURCE requires compiling with optimization
    CXXFLAGS += -DSTARFISH_TIZEN_OBS
  endif
  ifeq ($(TIZEN_PROFILE),tv)
    MEDIA_SUPPORT=true
    DOMPARSER_SUPPORT=true
    MULTIPAGE_SUPPORT=true
    CXXFLAGS += -DSTARFISH_TIZEN_TV
    CXXFLAGS += -DSTARFISH_ENABLE_TEST
    CXXFLAGS += -DSTARFISH_ENABLE_BODY_FOCUS_RING
#    CXXFLAGS += -DSTARFISH_ENABLE_VIRTUAL_CURSOR
    CXXFLAGS += -DSTARFISH_FRAME_REPLACED_VIDEO_NEEDS_GRAPHICS_BUFFER=false
    CXXFLAGS += -DSTARFISH_ENABLE_TTS
  endif
  ifeq ($(TIZEN_PROFILE),mobile)
    MEDIA_SUPPORT=true
    MULTIPAGE_SUPPORT=true
    DOMPARSER_SUPPORT=true
    CXXFLAGS += -DSTARFISH_TIZEN_MOBILE
    CXXFLAGS += -DSTARFISH_ENABLE_TEST
  endif
  ifeq ($(TIZEN_PROFILE),wearable)
    CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE
    CXXFLAGS += -DSTARFISH_THREAD_POOL_SIZE=2
    ifeq ($(TYPE), lib)
      CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE_LIB
    endif
  endif
endif

# for printing TC coverage log
ifeq ($(TC), 1)
  CXXFLAGS += -DSTARFISH_TC_COVERAGE
endif

# for media support
ifeq ($(MEDIA_SUPPORT), true)
  CXXFLAGS += -DSTARFISH_ENABLE_MULTIMEDIA
  # CXXFLAGS += -DSTARFISH_ENABLE_AVPLAY
endif

ifeq ($(INSPECTOR_SUPPORT), true)
  CXXFLAGS += -DSTARFISH_ENABLE_INSPECTOR
endif

ifeq ($(MULTIPAGE_SUPPORT), true)
  CXXFLAGS += -DSTARFISH_ENABLE_MULTIPAGE
endif

ifeq ($(DOMPARSER_SUPPORT), true)
  CXXFLAGS += -DSTARFISH_ENABLE_DOMPARSER
endif

ifeq ($(WASU_SUPPORT), true)
  CXXFLAGS += -DSTARFISH_ENABLE_WASU
endif

################################################################################
################################################################################
# Third-party build flags
################################################################################
################################################################################

# escargot
ifneq ($(HOST), tizen_obs)
  ESCARGOT_SRC_ROOT=third_party/escargot
  ESCARGOT_LIB_ROOT=third_party/escargot
else
  ESCARGOT_SRC_ROOT=/usr/include/web-widget-js
  ESCARGOT_LIB_ROOT=/usr/lib/web-widget-js
endif

include $(ESCARGOT_SRC_ROOT)/build/Flags.mk

CXXFLAGS += $(ESCARGOT_CXXFLAGS_COMMON)

ifeq ($(HOST), linux)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_LINUX)
else ifneq (,$(findstring tizen,$(HOST)))
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_TIZEN)
endif

ifeq ($(ARCH), x86)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_X86)
else ifeq ($(ARCH), arm)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_ARM)
else
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_X64)
endif

ifeq ($(MODE), debug)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_DEBUG)
else ifeq ($(MODE), release)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_RELEASE)
endif

# bdwgc
CXXFLAGS_DEBUG += -DGC_DEBUG

# deviceapi
ifeq ($(TIZEN_DEVICE_API), true)
  CXXFLAGS += -Ithird_party/deviceapi/src
  CXXFLAGS += -DTIZEN_DEVICE_API
  CXXFLAGS += -DSIZE_MAX=0xffffffff
  LDFLAGS += -ldl
endif

#skia_matrix
CXXFLAGS += -Ithird_party/skia_matrix/

#clipper
CXXFLAGS += -Ithird_party/clipper/cpp/

#rapidxml
CXXFLAGS += -Ithird_party/rapidxml/

#libtuv
ifeq ($(BACKEND), DALI)
  CXXFLAGS += -Ithird_party/libtuv/include
  CFLAGS += -Ithird_party/libtuv/include
  CFLAGS += -Ithird_party/libtuv/src
endif

#webm, libav
ifeq ($(MEDIA_SUPPORT), true)
  CXXFLAGS += -Ithird_party/webm/
  CXXFLAGS += -Ithird_party/libav/
endif
################################################################################
################################################################################
# SRCS & OBJS
################################################################################
################################################################################

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC=
SRC_CC=
SRC += $(AUTOGEN_SRC)
ALLSRC := $(call rwildcard,src/,*.cpp)
ALLSRC := $(filter-out $(AUTOGEN_SRC), $(ALLSRC))
SRC += $(ALLSRC)
ifeq ($(TYPE), lib)
  SRC := $(filter-out src/shell, $(SRC))
endif

# escargot
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/GCutil/bdwgc/include/
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/GCutil/
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/src
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/include
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/checked_arithmetic/
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/double_conversion/
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/rapidjson/include/
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/yarr/

ifeq ($(HOST), linux)
  JSLIBS = $(ESCARGOT_LIB_ROOT)/out/$(HOST)/$(ARCH)/interpreter/$(MODE)/libescargot.a
  GCLIBS = $(ESCARGOT_LIB_ROOT)/third_party/bdwgc/out/$(HOST)/$(ARCH)/$(MODE).shared/.libs/libgc.a
else ifeq ($(HOST), tizen_obs)
  JSLIBS = $(ESCARGOT_LIB_ROOT)/release/libescargot.a
  GCLIBS = $(ESCARGOT_LIB_ROOT)/release/libgc.a
else ifneq (,$(findstring tizen,$(HOST)))
  JSLIBS = $(ESCARGOT_LIB_ROOT)/out/tizen_$(TIZEN_VERSION)_$(TIZEN_PROFILE)/$(TIZEN_ARCH)/interpreter/$(MODE)/libescargot.a
  GCLIBS = $(ESCARGOT_LIB_ROOT)/third_party/bdwgc/out/tizen_$(TIZEN_VERSION)_$(TIZEN_PROFILE)/$(TIZEN_ARCH)/$(MODE).shared/.libs/libgc.a
endif

# deviceapi
ifeq ($(TIZEN_DEVICE_API), true)
  SRC += $(foreach dir, third_party/deviceapi/src, $(wildcard $(dir)/*.cpp))
endif

# skia_matrix
SRC += third_party/skia_matrix/SkMath.cpp
SRC += third_party/skia_matrix/SkPoint.cpp
SRC += third_party/skia_matrix/SkRect.cpp
SRC += third_party/skia_matrix/SkMatrix.cpp
SRC += third_party/skia_matrix/SkDebug.cpp

# clipper
SRC += third_party/clipper/cpp/clipper.cpp

# webm, libav, mp4parser
ifeq ($(MEDIA_SUPPORT), true)
    SRC_CC += third_party/webm/webvttparser.cc
    SRC_CC += third_party/webm/mkvparser.cc

    SRC += $(foreach dir, third_party/MP4Parse/source, $(wildcard $(dir)/MP4*.cpp))
    CXXFLAGS += -Ithird_party/MP4Parse/source/include
    ifneq (,$(findstring tizen,$(HOST)))
      ifneq ($(TIZEN_PROFILE),wearable)
        LDFLAGS += -lavformat -lavcodec -lavutil
      else
        LIBAV_CURPATH=third_party/libav/out/tizen_$(TIZEN_VERSION)_$(TIZEN_PROFILE)/$(TIZEN_ARCH)/$(MODE)
        CXXFLAGS += -I$(LIBAV_CURPATH)
        LDFLAGS += -L$(LIBAV_CURPATH)/libavformat/
        LDFLAGS += -L$(LIBAV_CURPATH)/libavcodec/
        LDFLAGS += -L$(LIBAV_CURPATH)/libavutil/
        LDFLAGS += -lavformat -lavcodec -lavutil
        LDFLAGS += -Wl,-rpath $(LIBAV_CURPATH)/libavformat/
        LDFLAGS += -Wl,-rpath $(LIBAV_CURPATH)/libavcodec/
        LDFLAGS += -Wl,-rpath $(LIBAV_CURPATH)/libavutil/
      endif
    else
      LIBAV_CURPATH=third_party/libav/out/$(HOST)/$(ARCH)/release
      CXXFLAGS += -I$(LIBAV_CURPATH)
      LDFLAGS += -L$(LIBAV_CURPATH)/libavformat/
      LDFLAGS += -L$(LIBAV_CURPATH)/libavcodec/
      LDFLAGS += -L$(LIBAV_CURPATH)/libavutil/
      LDFLAGS += -lavformat -lavcodec -lavutil
      LDFLAGS += -Wl,-rpath $(LIBAV_CURPATH)/libavformat/
      LDFLAGS += -Wl,-rpath $(LIBAV_CURPATH)/libavcodec/
      LDFLAGS += -Wl,-rpath $(LIBAV_CURPATH)/libavutil/
    endif

endif

# zeromq
ifeq ($(INSPECTOR_SUPPORT), true)
  CXXFLAGS += -Ithird_party/zeromq/include
  CXXFLAGS += -Ithird_party/cppzmq/
  ifneq (,$(findstring tizen,$(HOST)))
    ZMQLIBS = third_party/zeromq/out/tizen_$(TIZEN_VERSION)_$(TIZEN_PROFILE)/$(TIZEN_ARCH)/$(MODE).shared/.libs/libzmq.a
  else
    ZMQLIBS = third_party/zeromq/out/$(HOST)/$(ARCH)/$(MODE).shared/.libs/libzmq.a
  endif
else
  ZMQLIBS=
endif

# OBJS
OBJS := $(SRC:%.cpp= $(OUTDIR)/%.o)
OBJS += $(SRC_C:%.c= $(OUTDIR)/%.o)
OBJS += $(SRC_CC:%.cc= $(OUTDIR)/%.o)
THIRD_PARTY_OBJS := $(JSLIBS) $(GCLIBS) $(ZMQLIBS)

################################################################################
################################################################################
# Toolchain
################################################################################
################################################################################

ifeq ($(HOST), linux)
  CC           = gcc
  CXX          = g++
  STRIP        = strip
  ifeq ($(BACKEND), EFL)
    CXXFLAGS += $(shell pkg-config --cflags elementary ecore ecore-x libpng cairo freetype2 fontconfig icu-uc icu-i18n )
    LDFLAGS += $(shell pkg-config --libs elementary ecore ecore-x ecore-imf-evas libpng cairo freetype2 fontconfig icu-uc icu-i18n )
  else ifeq ($(BACKEND), DALI)
    CXXFLAGS += $(shell pkg-config --cflags elementary ecore ecore-x libpng cairo freetype2 fontconfig icu-uc icu-i18n dali-core dali-adaptor dali-toolkit)
    LDFLAGS += $(shell pkg-config --libs elementary ecore ecore-x ecore-imf-evas libpng cairo freetype2 fontconfig icu-uc icu-i18n dali-adaptor dali-toolkit)
    endif
  CXXFLAGS += -I/usr/local/include/
  LDFLAGS += -L/usr/local/lib/ -Wl,-rpath /usr/local/lib
else ifeq ($(HOST), tizen_obs)
  CC           = gcc
  CXX          = g++
  STRIP        = strip
  TIZEN_DEPS = dlog elementary ecore libpng cairo freetype2 fontconfig icu-uc icu-i18n \
               ecore-imf-evas efl-extension libpng capi-network-connection capi-media-player vconf vconf-internal-keys-tv
  ifeq ($(BACKEND), DALI)
    TIZEN_DEPS += dali-core dali-adaptor dali-toolkit
  endif
  TIZEN_DEPS += capi-location-manager
  CXXFLAGS += -I/usr/include/location

  CXXFLAGS    += $(shell pkg-config --cflags $(TIZEN_DEPS))
  LDFLAGS     += $(shell pkg-config --libs $(TIZEN_DEPS)) -lssl -lcrypto
  ifeq ($(BACKEND), DALI)
    LDFLAGS += -lturbojpeg -lgif third_party/libtuv/build/arm-tizen/debug/lib/libtuv.a
  endif
  LIB = liblightweight-web-engine.so
  ifeq ($(TYPE), exe)
    LDFLAGS += -lrt
  endif
else ifneq (,$(findstring tizen,$(HOST)))

  # Toolchain & Platform
  COMPILER_PREFIX=$(TIZEN_ARCH)-linux-gnueabi
  COMPILER_VERSION=4.6
  TIZEN_SYSROOT=$(TIZEN_SDK_HOME)/platforms/tizen-$(TIZEN_VERSION)/$(TIZEN_PROFILE)/rootstraps/$(TIZEN_PROFILE)-$(TIZEN_VERSION)-$(TIZEN_DEVICE).core
  TIZEN_TOOLCHAIN=$(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-$(COMPILER_VERSION)

  CC    = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-strip

  TIZEN_INCLUDE = dlog elementary-1 elocation-1 efl-1 ecore-x-1 eina-1 eina-1/eina eet-1 evas-1 ecore-1 ecore-evas-1 ecore-file-1 \
                  ecore-input-1 edje-1 eo-1 emotion-1 ecore-imf-1 ecore-con-1 eio-1 eldbus-1 efl-extension \
                  efreet-1 ecore-input-evas-1 ecore-audio-1 embryo-1 ecore-imf-evas-1 ethumb-1 eeze-1 eeze-1 e_dbus-1 dbus-1.0 freetype2 media cairo network location vconf vconf-internal-keys-tv
  ifneq ($(TIZEN_VERSION), 2.3.1)
    TIZEN_INCLUDE += emile-1 ethumb-client-1
  endif
  TIZEN_LIB = ecore evas rt efl-extension freetype capi-media-player elementary fontconfig ecore_evas ecore_input cairo capi-network-connection dlog icui18n icuuc icudata capi-location-manager vconf vconf-internal-keys-tv

  CXXFLAGS += --sysroot=$(TIZEN_SYSROOT)
  CXXFLAGS +=  $(addprefix -I$(TIZEN_SYSROOT)/usr/include/, $(TIZEN_INCLUDE))
  CXXFLAGS += -Ideps/tizen/include
  CXXFLAGS += -Ideps/tizen/include/tizen-$(TIZEN_PROFILE)-$(TIZEN_VERSION)-$(TIZEN_ARCH)
  CXXFLAGS += -I$(TIZEN_SYSROOT)/usr/lib/dbus-1.0/include

  LDFLAGS += --sysroot=$(TIZEN_SYSROOT)
  LDFLAGS +=  $(addprefix -l, $(TIZEN_LIB))
  LDFLAGS += -Ldeps/tizen/lib/tizen-$(TIZEN_PROFILE)-$(TIZEN_VERSION)-$(TIZEN_ARCH)

  # Workaround for platform gcc 4.6 + toolchain gcc 4.9
  ifneq ($(TIZEN_VERSION), 3.0)
    ifeq ($(COMPILER_VERSION), 4.9)
      ifeq ($(TYPE), lib)
        LDFLAGS += -static-libstdc++
      else
        LDFLAGS += $(TIZEN_TOOLCHAIN)/$(COMPILER_PREFIX)/lib/libstdc++.a
      endif
    endif
  endif

  LIB = liblightweight-web-engine.so
endif

ifeq ($(LTO), 1)
  LDFLAGS += $(CXXFLAGS) # for LTO, CXXFLAGS should be duplicated in LDFLAGS
endif

################################################################################
################################################################################
# Build Targets
################################################################################
################################################################################

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

.DEFAULT_GOAL:=x86.exe.debug

x86.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x86.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
x64.lib.release: $(OUTDIR)/$(LIB)
	cp -f $< .

tizen_mobile_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_mobile_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN)
tizen_wearable_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN) # use stripped binary
tizen_wearable_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN)
tizen_wearable_arm.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_wearable_arm.lib.release: $(OUTDIR)/$(LIB)
	cp -f $<.strip ./$(LIB)
tizen_wearable_emulator.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_emulator.exe.release: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN)
tizen_wearable_emulator.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_wearable_emulator.lib.release: $(OUTDIR)/$(LIB)
	cp -f $<.strip ./$(LIB)

tizen_obs_arm.lib.release: $(OUTDIR)/$(LIB)
tizen_obs_emulator.lib.release: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_obs_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_obs_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_obs_emulator.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_obs_emulator.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .

tizen24_mobile_emulator.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen24_mobile_emulator.lib.release: $(OUTDIR)/$(LIB)
	cp -f $<.strip ./$(LIB)
tizen24_mobile_emulator.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen24_mobile_emulator.exe.release: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN)

tizen24_wearable_arm.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen24_wearable_arm.lib.release: $(OUTDIR)/$(LIB)
	cp -f $<.strip ./$(LIB)
tizen24_mobile_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen24_mobile_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN)

tizen3_mobile_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_mobile_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN)
tizen3_wearable_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN)
tizen3_wearable_arm.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen3_wearable_arm.lib.release: $(OUTDIR)/$(LIB)
	cp -f $<.strip ./$(LIB)
tizen3_wearable_emulator.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_emulator.exe.release: $(OUTDIR)/$(BIN)
	cp -f $<.strip ./$(BIN)
tizen3_wearable_emulator.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen3_wearable_emulator.lib.release: $(OUTDIR)/$(LIB)
	cp -f $<.strip ./$(LIB)

DEPENDENCY_MAKEFILE = Makefile $(ESCARGOT_SRC_ROOT)/build/Flags.mk

$(OUTDIR)/$(BIN): $(OBJS) $(THIRD_PARTY_OBJS) $(DEPENDENCY_MAKEFILE)
	@echo "[LINK] $@"
	$(CXX) -o $@ $(OBJS) $(THIRD_PARTY_OBJS) $(LDFLAGS)
	cp $@ $@.strip
	$(STRIP) $@.strip

$(OUTDIR)/$(LIB): $(OBJS) $(THIRD_PARTY_OBJS) $(DEPENDENCY_MAKEFILE)
	@echo "[LINK] $@"
	$(CXX) -shared -Wl,-soname,$(LIB) -o $@ $(OBJS) $(THIRD_PARTY_OBJS) $(LDFLAGS)
	cp $@ $@.strip
	$(STRIP) $@.strip

$(OUTDIR)/%.o: %.cpp $(DEPENDENCY_MAKEFILE)
	echo "[CXX] $@"
	mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $< -o $@
	$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

$(OUTDIR)/%.o: %.cc $(DEPENDENCY_MAKEFILE)
	echo "[CXX] $@"
	mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $< -o $@
	$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

$(OUTDIR)/%.o: %.c $(DEPENDENCY_MAKEFILE)
	echo "[CC] $@"
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@
	$(CC) -MM $(CFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

clean:
	rm -rf out
	find src/binding -type f -name "*Binding.cpp" ! -name "*CustomBinding.cpp" | xargs -r rm
	find src/binding -type f -name "*Union.h" ! -name "*CustomUnion.h" | xargs -r rm

$(AUTOGEN_SRC): $(AUTOGEN_DEPENDENCY)
	@echo "[GEN] $@"
	@mkdir -p $(AUTOGEN_DIR)
	@mv $(@:src/%=out/%) $@

.SECONDARY: $(AUTOGEN_SRC)

################################################################################
################################################################################
# Test Targets
################################################################################
################################################################################

ifeq (run,$(firstword $(MAKECMDGOALS)))
# use the rest as arguments for "run"
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
# ...and turn them into do-nothing targets
$(eval $(RUN_ARGS):;@:)
endif

run:
	phantomjs --web-security=false --local-to-remote-url-access=true runner.js ${RUN_ARGS}

install_ffmpeg:
	sudo apt-get install libavformat-dev libavcodec-dev libavutil-dev

install_git_prepush:
	cp -rf tool/reftest/pre-push .git/hooks/

install_pixel_test_dep:
	$(CXX) -O3 -g3 --std=c++11 -o tool/imgdiff/imgdiff tool/imgdiff/imgdiff.cpp $(shell pkg-config --cflags libpng) $(shell pkg-config --libs libpng)
	mkdir -p ~/.fonts
	cp tool/fonts/StarFishAhem.ttf ~/.fonts/
	cp tool/fonts/SamsungOne-300C_v1.0.ttf ~/.fonts/
	cp tool/fonts/SamsungOne-600C_v1.0.ttf ~/.fonts/
	fc-cache -fv
	fc-match SamsungOne

install_inspector_nwjs:
	cd inspector ; ./setup_nwjs.sh

run_inspector:
	./inspector/nwjs-v0.17.0-linux-x64/nw ./inspector/ > /dev/null &

csswg_test_css1:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_css1.res efl -p$(TEST_NPROCS)
csswg_test_css21:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_css21.res efl -p$(TEST_NPROCS)
csswg_test_css21_tables:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_css21_tables.res efl -p$(TEST_NPROCS)
csswg_test_css3_color:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_css3_color.res efl -p$(TEST_NPROCS)
csswg_test_css3_backgrounds:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_css3_backgrounds.res efl -p$(TEST_NPROCS)
csswg_test_css3_transforms:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_css3_transforms.res efl -p$(TEST_NPROCS)
csswg_test_css3_selectors:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_css3_selectors.res efl -p$(TEST_NPROCS)
csswg_test_mediaqueries3:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_mediaqueries3.res efl -p$(TEST_NPROCS)
csswg_test_manual:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_manual.res efl --font-dep -p$(TEST_NPROCS)
csswg_test_rtl:
	./tool/drivers/run_test.py csswg tool/reftest/efl/tclist/csswg_rtl.res efl -p$(TEST_NPROCS)
csswg_test_all:
	make csswg_test_css1
	make csswg_test_css21
	make csswg_test_css21_tables
	make csswg_test_css3_color
	make csswg_test_css3_transforms
	make csswg_test_css3_backgrounds
	make csswg_test_css3_selectors
	make csswg_test_mediaqueries3
	make csswg_test_manual

internal_test:
	cat tool/reftest/efl/internal_unsorted.res | sort -nr | cut -d";" -f2 > tool/reftest/efl/internal.res
	./tool/drivers/run_test.py basic tool/reftest/efl/internal.res efl -p$(TEST_NPROCS)
	./tool/drivers/run_test.py basic tool/reftest/efl/internal_manual.res efl --font-dep -p$(TEST_NPROCS)
	rm tool/reftest/efl/internal.res

internal_test_gitlab_prerequisite:
	./tool/reftest/efl/internal.sh $(div)
internal_test_part1:
	./tool/drivers/run_test.py basic tool/reftest/efl/internal_part1.res efl -p$(TEST_NPROCS)
internal_test_part2:
	./tool/drivers/run_test.py basic tool/reftest/efl/internal_part2.res efl -p$(TEST_NPROCS)
internal_test_part3:
	./tool/drivers/run_test.py basic tool/reftest/efl/internal_part3.res efl -p$(TEST_NPROCS)
internal_test_part4:
	./tool/drivers/run_test.py basic tool/reftest/efl/internal_part4.res -p$(TEST_NPROCS)
internal_test_part5:
	./tool/drivers/run_test.py basic tool/reftest/efl/internal_part5.res efl -p$(TEST_NPROCS)
internal_test_manual:
	./tool/drivers/run_test.py basic tool/reftest/efl/internal_manual.res efl --font-dep -p$(TEST_NPROCS)

tct:
	./StarFish test/tct/index.html

wpt_syntax_checker:
	./tool/pixel_test/syntaxChecker.sh css1
	@echo "[wpt_syntax_checker] Updated tool/pixel_test/css1.res"
	./tool/pixel_test/syntaxChecker.sh css21
	@echo "[wpt_syntax_checker] Updated tool/pixel_test/css21.res"
	./tool/pixel_test/syntaxChecker.sh css-backgrounds-3
	@echo "[wpt_syntax_checker] Updated tool/pixel_test/css-backgrounds-3.res"
	./tool/pixel_test/syntaxChecker.sh css-color-3
	@echo "[wpt_syntax_checker] Updated tool/pixel_test/css-color-3.res"
	./tool/pixel_test/syntaxChecker.sh css-transforms-1
	@echo "[wpt_syntax_checker] Updated tool/pixel_test/css-transforms-1.res"
	@echo "[wpt_syntax_checker] COMPLETE.."

dom_conformance_test:
	./tool/drivers/run_test.py dom_conformance tool/reftest/efl/dom_conformance_test.res efl -p$(TEST_NPROCS)
dom_conformance_test_webkit:
	./tool/drivers/run_test.py dom_conformance tool/reftest/efl/webkit_dom_conformance_test.res efl -p$(TEST_NPROCS)
dom_conformance_test_blink:
	./tool/drivers/run_test.py dom_conformance tool/reftest/efl/blink_dom_conformance_test.res efl -p$(TEST_NPROCS)
dom_conformance_test_gecko:
	./tool/drivers/run_test.py dom_conformance tool/reftest/efl/gecko_dom_conformance_test.res efl -p$(TEST_NPROCS)

web_platform_test_dom:
	./tool/drivers/run_test.py web_platform tool/reftest/efl/wpt_dom.res efl -p$(TEST_NPROCS)
web_platform_test_dom_events:
	./tool/drivers/run_test.py web_platform tool/reftest/efl/wpt_dom_events.res efl -p$(TEST_NPROCS)
web_platform_test_html:
	./tool/drivers/run_test.py web_platform tool/reftest/efl/wpt_html.res efl -p$(TEST_NPROCS)
web_platform_test_page_visibility:
	./tool/drivers/run_test.py web_platform tool/reftest/efl/wpt_page_visibility.res efl -p$(TEST_NPROCS)
web_platform_test_progress_events:
	./tool/drivers/run_test.py web_platform tool/reftest/efl/wpt_progress_events.res efl -p$(TEST_NPROCS)
web_platform_test_xhr:
	./tool/drivers/run_test.py web_platform tool/reftest/efl/wpt_xhr.res efl -p$(TEST_NPROCS)

vendor_test_blink_fast_dom:
	./tool/drivers/run_test.py vendor_basic tool/reftest/efl/blink_fast_dom.res efl -p$(TEST_NPROCS)
vendor_test_blink_fast_html:
	./tool/drivers/run_test.py vendor_basic tool/reftest/efl/blink_fast_html.res efl -p$(TEST_NPROCS)
vendor_test_blink_fast_css:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/blink_fast_css.res efl -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/blink_fast_css_manual.res efl --font-dep -p$(TEST_NPROCS)
vendor_test_blink_fast_etc:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/blink_fast_etc.res efl -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/blink_fast_etc_manual.res efl --font-dep -p$(TEST_NPROCS)
vendor_test_gecko_layout:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/gecko_layout.res efl -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/gecko_layout_manual.res efl --font-dep -p$(TEST_NPROCS)
vendor_test_webkit_fast_dom:
	./tool/drivers/run_test.py vendor_basic tool/reftest/efl/webkit_fast_dom.res efl -p$(TEST_NPROCS)
vendor_test_webkit_fast_html:
	./tool/drivers/run_test.py vendor_basic tool/reftest/efl/webkit_fast_html.res efl -p$(TEST_NPROCS)
vendor_test_webkit_fast_css:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/webkit_fast_css.res efl -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/webkit_fast_css_manual.res efl --font-dep -p$(TEST_NPROCS)
vendor_test_webkit_fast_etc:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/webkit_fast_etc.res efl -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/efl/webkit_fast_etc_manual.res efl --font-dep -p$(TEST_NPROCS)

bidi_test:
	./tool/drivers/run_test.py bidi tool/reftest/efl/bidi.res efl --font-dep -p$(TEST_NPROCS)

regression_test_bidi.tizen_wearable_arm.debug:
	$(CXX) -O3 -g3 --std=c++11 $(CXXFLAGS) $(LDFLAGS) -o tool/imgdiff/imgdiffEvas.exe tool/imgdiff/imgdiffEvas.cpp
	./tool/reftest/setup_bidi_test.sh true

################################################################################
# Test Cairo backend
################################################################################

csswg_test_css1_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_css1.res cairo -p$(TEST_NPROCS)
csswg_test_css21_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_css21.res cairo -p$(TEST_NPROCS)
csswg_test_css21_tables_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_css21_tables.res cairo -p$(TEST_NPROCS)
csswg_test_css3_color_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_css3_color.res cairo -p$(TEST_NPROCS)
csswg_test_css3_backgrounds_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_css3_backgrounds.res cairo -p$(TEST_NPROCS)
csswg_test_css3_transforms_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_css3_transforms.res cairo -p$(TEST_NPROCS)
csswg_test_css3_selectors_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_css3_selectors.res cairo -p$(TEST_NPROCS)
csswg_test_mediaqueries3_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_mediaqueries3.res cairo -p$(TEST_NPROCS)
csswg_test_manual_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_manual.res cairo --font-dep -p$(TEST_NPROCS)
csswg_test_rtl_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_rtl.res cairo -p$(TEST_NPROCS)
csswg_test_flex_cairo:
	./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_flex.res cairo -p$(TEST_NPROCS)
csswg_test_all_cairo:
	make csswg_test_css1_cairo
	make csswg_test_css21_cairo
	make csswg_test_css21_tables_cairo
	make csswg_test_css3_color_cairo
	make csswg_test_css3_transforms_cairo
	make csswg_test_css3_backgrounds_cairo
	make csswg_test_css3_selectors_cairo
	make csswg_test_mediaqueries3_cairo
	make csswg_test_flex_cairo
	make csswg_test_manual_cairo

internal_test_cairo:
	cat tool/reftest/cairo/internal_unsorted.res | sort -nr | cut -d";" -f2 > tool/reftest/cairo/internal.res
	./tool/drivers/run_test.py basic tool/reftest/cairo/internal.res cairo -p$(TEST_NPROCS)
	./tool/drivers/run_test.py basic tool/reftest/cairo/internal_manual.res cairo --font-dep -p$(TEST_NPROCS)
	rm tool/reftest/cairo/internal.res

internal_test_gitlab_prerequisite_cairo:
	./tool/reftest/cairo/internal.sh $(div)
internal_test_part1_cairo:
	./tool/drivers/run_test.py basic tool/reftest/cairo/internal_part1.res cairo -p$(TEST_NPROCS)
internal_test_part2_cairo:
	./tool/drivers/run_test.py basic tool/reftest/cairo/internal_part2.res cairo -p$(TEST_NPROCS)
internal_test_part3_cairo:
	./tool/drivers/run_test.py basic tool/reftest/cairo/internal_part3.res cairo -p$(TEST_NPROCS)
internal_test_part4_cairo:
	./tool/drivers/run_test.py basic tool/reftest/cairo/internal_part4.res cairo -p$(TEST_NPROCS)
internal_test_part5_cairo:
	./tool/drivers/run_test.py basic tool/reftest/cairo/internal_part5.res cairo -p$(TEST_NPROCS)
internal_test_manual_cairo:
	./tool/drivers/run_test.py basic tool/reftest/cairo/internal_manual.res cairo --font-dep -p$(TEST_NPROCS)

dom_conformance_test_cairo:
	./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/dom_conformance_test.res cairo -p$(TEST_NPROCS)
dom_conformance_test_webkit_cairo:
	./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/webkit_dom_conformance_test.res cairo -p$(TEST_NPROCS)
dom_conformance_test_blink_cairo:
	./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/blink_dom_conformance_test.res cairo -p$(TEST_NPROCS)
dom_conformance_test_gecko_cairo:
	./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/gecko_dom_conformance_test.res cairo -p$(TEST_NPROCS)

web_platform_test_dom_cairo:
	./tool/drivers/run_test.py web_platform tool/reftest/cairo/wpt_dom.res cairo -p$(TEST_NPROCS)
web_platform_test_dom_events_cairo:
	./tool/drivers/run_test.py web_platform tool/reftest/cairo/wpt_dom_events.res cairo -p$(TEST_NPROCS)
web_platform_test_html_cairo:
	./tool/drivers/run_test.py web_platform tool/reftest/cairo/wpt_html.res cairo -p$(TEST_NPROCS)
web_platform_test_page_visibility_cairo:
	./tool/drivers/run_test.py web_platform tool/reftest/cairo/wpt_page_visibility.res cairo -p$(TEST_NPROCS)
web_platform_test_progress_events_cairo:
	./tool/drivers/run_test.py web_platform tool/reftest/cairo/wpt_progress_events.res cairo -p$(TEST_NPROCS)
web_platform_test_xhr_cairo:
	./tool/drivers/run_test.py web_platform tool/reftest/cairo/wpt_xhr.res cairo -p$(TEST_NPROCS)

vendor_test_blink_fast_dom_cairo:
	./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_dom.res cairo -p$(TEST_NPROCS)
vendor_test_blink_fast_html_cairo:
	./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_html.res cairo -p$(TEST_NPROCS)
vendor_test_blink_fast_css_cairo:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css.res cairo -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css_manual.res cairo --font-dep -p$(TEST_NPROCS)
vendor_test_blink_fast_etc_cairo:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc.res cairo -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc_manual.res cairo --font-dep -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_table.res cairo -p$(TEST_NPROCS)
vendor_test_gecko_layout_cairo:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout.res cairo -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout_manual.res cairo --font-dep -p$(TEST_NPROCS)
vendor_test_webkit_fast_dom_cairo:
	./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_dom.res cairo -p$(TEST_NPROCS)
vendor_test_webkit_fast_html_cairo:
	./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_html.res cairo -p$(TEST_NPROCS)
vendor_test_webkit_fast_css_cairo:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css.res cairo -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css_manual.res cairo --font-dep -p$(TEST_NPROCS)
vendor_test_webkit_fast_etc_cairo:
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc.res cairo -p$(TEST_NPROCS)
	./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc_manual.res cairo --font-dep -p$(TEST_NPROCS)

bidi_test_cairo:
	./tool/drivers/run_test.py bidi tool/reftest/cairo/bidi.res cairo --font-dep -p$(TEST_NPROCS)

################################################################################

react_test:
	cd ./test/cairo/reftest/vendor/react/ && ./generator.py
	./tool/drivers/run_test.py basic tool/reftest/cairo/react.res common -p$(TEST_NPROCS)

test_all:
	make dom_conformance_test
	make dom_conformance_test_webkit
	make dom_conformance_test_blink
	make dom_conformance_test_gecko
	make web_platform_test_dom
	make web_platform_test_dom_events
	make web_platform_test_html
	make web_platform_test_page_visibility
	make web_platform_test_progress_events
	make web_platform_test_xhr
	make vendor_test_blink_fast_dom
	make vendor_test_blink_fast_html
	make vendor_test_blink_fast_css
	make vendor_test_blink_fast_etc
	make vendor_test_gecko_layout
	make vendor_test_webkit_fast_dom
	make vendor_test_webkit_fast_html
	make vendor_test_webkit_fast_css
	make vendor_test_webkit_fast_etc
	make bidi_test
	make csswg_test_all
	make internal_test

test_pr:
	make vendor_test_blink_fast_html
	make vendor_test_blink_fast_dom
	make vendor_test_blink_fast_css
	make vendor_test_blink_fast_etc
	make vendor_test_gecko_layout
	make vendor_test_webkit_fast_html
	make vendor_test_webkit_fast_css
	make vendor_test_webkit_fast_dom
	make web_platform_test_dom
	make web_platform_test_html
	make web_platform_test_dom_events
	make web_platform_test_progress_events
	make web_platform_test_page_visibility
	make web_platform_test_xhr
	make dom_conformance_test
	make dom_conformance_test_webkit
	make dom_conformance_test_blink
	make dom_conformance_test_gecko
	make bidi_test
	make internal_test_gitlab_prerequisite div=5
	make internal_test_part1
	make internal_test_part2
	make internal_test_part3
	make internal_test_part4
	make internal_test_part5
	make internal_test_manual
	make vendor_test_webkit_fast_etc
	make csswg_test_all


reftest_emulator_2.3:
	./tool/reftest/reftest_runner.sh emulator 2.3 all
reftest_emulator_3.0:
	./tool/reftest/reftest_runner.sh emulator 3.0 all
reftest_target_2.3:
	./tool/reftest/reftest_runner.sh target 2.3 all
reftest_target_3.0:
	./tool/reftest/reftest_runner.sh target 3.0 all

tidy:
	./tool/check_tidy.py > error_report

tidy-update:
	./tool/check_tidy.py -up ./src/

.PHONY: clean tct test_pr tidy tidy-update
