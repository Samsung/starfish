BUILDDIR=./build
HOST=linux

BIN=StarFish
EBIN=StarFish
LIB=libStarFish.so

LOCAL_PATH = $(shell pwd)

#######################################################
# Environments
#######################################################

ARCH=
TYPE=
MODE=#debug,release
NPROCS:=1
OS:=$(shell uname -s)
SHELL:=/bin/bash
OUTPUT:=bin
TIZEN_DEVICE_API=
ifeq ($(OS),Linux)
  NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
  SHELL:=/bin/bash
endif
ifeq ($(OS),Darwin)
  NPROCS:=$(shell sysctl -n machdep.cpu.thread_count)
  SHELL:=/opt/local/bin/bash
endif

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
  VERSION=2.3.1
else ifneq (,$(findstring tizen3_wearable_arm,$(MAKECMDGOALS)))
  HOST=tizen3_wearable_arm
  VERSION=3.0
else ifneq (,$(findstring tizen_arm,$(MAKECMDGOALS)))
  HOST=tizen_arm
  VERSION=2.3.1
else ifneq (,$(findstring tizen3_arm,$(MAKECMDGOALS)))
  HOST=tizen3_arm
  VERSION=3.0
else ifneq (,$(findstring tizen_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen_wearable_emulator
  VERSION=2.3.1
  ARCH=x86
else ifneq (,$(findstring tizen3_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen3_wearable_emulator
  VERSION=3.0
  ARCH=x86
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
else ifeq ($(HOST), tizen_arm)
  OUTDIR=out/tizen_$(VERSION)/$(ARCH)/$(TYPE)/$(MODE)
else ifneq ($(filter $(HOST),tizen_wearable_arm tizen3_wearable_arm), )
  OUTDIR=out/tizen_$(VERSION)/$(ARCH)/$(TYPE)/$(MODE)
else ifeq ($(HOST), tizen_wearable_emulator)
  OUTDIR=out/tizen_$(VERSION)/$(ARCH)/$(TYPE)/$(MODE)
endif

$(info host... $(HOST))
$(info arch... $(ARCH))
$(info type... $(TYPE))
$(info mode... $(MODE))
$(info build dir... $(OUTDIR))

CXXFLAGS += -std=c++11 -fno-rtti
LDFLAGS +=

ifeq ($(ARCH), x86)
  CXXFLAGS += -m32 -mfpmath=sse -msse2 -DESCARGOT_32=1
  LDFLAGS += -m32
else ifeq ($(ARCH), arm)
  CXXFLAGS += -DESCARGOT_32=1 -march=armv7-a -mthumb
else
endif

CXXFLAGS +=  -DUSE_ES6_FEATURE

ifeq ($(MODE), debug)
  CXXFLAGS += $(CXXFLAGS_DEBUG)
else ifeq ($(MODE), release)
  CXXFLAGS += $(CXXFLAGS_RELEASE)
endif


#######################################################
# Global build flags
#######################################################

# common flags
CXXFLAGS += -fno-rtti -fno-math-errno -Isrc/ -Iinc/
CXXFLAGS += -fdata-sections -ffunction-sections
CXXFLAGS += -frounding-math -fsignaling-nans
CXXFLAGS += -Wno-invalid-offsetof -fvisibility=hidden

ifeq ($(HOST), tizen)
  CXXFLAGS += --sysroot=$(TIZEN_SYSROOT)
endif

# fixme
# this causes
# /home/ksh8281/tizen-sdk-2.4.r2/tools/arm-linux-gnueabi-gcc-4.9/bin/../lib/gcc/arm-linux-gnueabi/4.9.2/../../../../arm-linux-gnueabi/bin/ld: BFD (GNU Binutils) 2.22 assertion fail ../../bfd/elf32-arm.c:12049
# LDFLAGS += -Wl,--gc-sections

ifeq ($(HOST), tizen)
  LDFLAGS += --sysroot=$(TIZEN_SYSROOT)
endif

# flags for debug/release
CXXFLAGS_DEBUG = -O0 -g3 -D_GLIBCXX_DEBUG -fno-omit-frame-pointer -Wall -Wextra -Werror
CXXFLAGS_DEBUG += -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-unused-parameter
CXXFLAGS_RELEASE = -O2 -g3 -DNDEBUG -fomit-frame-pointer -fno-stack-protector -funswitch-loops -Wno-deprecated-declarations

#######################################################
# Third-party build flags
#######################################################

# bdwgc
#CXXFLAGS += -Ithird_party/bdwgc/include/
#CXXFLAGS_DEBUG += -DGC_DEBUG

#ifeq ($(OUTPUT), bin)
#  GCLIBS=third_party/bdwgc/out/$(ARCH)/$(MODE)/.libs/libgc.a
#else
#  GCLIBS=third_party/bdwgc/out/$(ARCH)/$(MODE).shared/.libs/libgc.a
#endif

# escargot
CXXFLAGS += -Ithird_party/escargot/third_party/bdwgc/include/
CXXFLAGS += -Ithird_party/escargot/src
CXXFLAGS += -Ithird_party/escargot/third_party/double_conversion/
CXXFLAGS += -Ithird_party/escargot/third_party/rapidjson/include/
CXXFLAGS += -Ithird_party/escargot/third_party/yarr/
CXXFLAGS_DEBUG += -DGC_DEBUG

ifeq ($(TIZEN_DEVICE_API), true)
	include third_party/deviceapi/build/Include.mk
endif

JSLIBS = third_party/escargot/out/$(HOST)/$(ARCH)/interpreter/$(MODE)/libescargot.a
GCLIBS = third_party/escargot/third_party/bdwgc/out/$(HOST)/$(ARCH)/$(MODE).shared/.libs/libgc.a

#skia_matrix
CXXFLAGS += -Ithird_party/skia_matrix/

#clipper
CXXFLAGS += -Ithird_party/clipper/cpp/

#######################################################
# SRCS & OBJS
#######################################################

SRC=
SRC += $(foreach dir, src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/dom , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/extra , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/dom/parser , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/dom/builder , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/dom/builder/html , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/dom/binding , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/layout , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/loader , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/style , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/util , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/threading , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/message_loop , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/network , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/window , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/canvas , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/canvas/image , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/canvas/font , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/file_io , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/public , $(wildcard $(dir)/*.cpp))

ifeq ($(TYPE), lib)
else
  SRC += $(foreach dir, src/shell , $(wildcard $(dir)/*.cpp))
endif


# skia_matrix
SRC += third_party/skia_matrix/SkMath.cpp
SRC += third_party/skia_matrix/SkPoint.cpp
SRC += third_party/skia_matrix/SkRect.cpp
SRC += third_party/skia_matrix/SkMatrix.cpp
SRC += third_party/skia_matrix/SkDebug.cpp

# clipper
SRC += third_party/clipper/cpp/clipper.cpp

ifeq ($(TIZEN_DEVICE_API), true)
  SRC += $(foreach dir, third_party/deviceapi/src , $(wildcard $(dir)/*.cpp))
endif

SRC_CC =

OBJS := $(SRC:%.cpp= $(OUTDIR)/%.o)
OBJS += $(SRC_CC:%.cc= $(OUTDIR)/%.o)
OBJS += $(SRC_C:%.c= $(OUTDIR)/%.o)


LDFLAGS += -lpthread -lcurl
THIRD_PARTY_LIBS= $(JSLIBS) $(GCLIBS)

ifeq ($(HOST), linux)
  CC           = gcc
  CXX          = g++
  STRIP        = strip
  CXXFLAGS += -DSTARFISH_ENABLE_PIXEL_TEST
  CXXFLAGS += $(shell pkg-config --cflags elementary ecore ecore-x libpng cairo freetype2 fontconfig icu-uc icu-i18n)
  LDFLAGS += $(shell pkg-config --libs elementary ecore ecore-x ecore-imf-evas libpng cairo freetype2 fontconfig icu-uc icu-i18n)
  # TC Coverage
  # ifeq ($(MODE), debug)
  #   CXXFLAGS += -DSTARFISH_TC_COVERAGE
  # endif
else ifneq ($(filter $(HOST),tizen_arm tizen3_arm), )
  ifeq ($(HOST), tizen_arm)
    ifndef TIZEN_SDK_HOME
      $(error TIZEN_SDK_HOME must be set)
    endif
    VERSION=2.4
    TIZEN_ROOT=$(TIZEN_SDK_HOME)
  endif
  ifeq ($(HOST), tizen3_arm)
    ifndef TIZEN3_SDK_HOME
      $(error TIZEN3_SDK_HOME must be set)
    endif
    VERSION=3.0
    TIZEN_ROOT=$(TIZEN3_SDK_HOME)
  endif
  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/arm-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/mobile/rootstraps/mobile-$(VERSION)-device.core
  # Setting For Tizen 2.3 SDK
  # TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/arm-linux-gnueabi-gcc-4.6
  # TIZEN_SYSROOT=/home/chokobole/Workspace/Tizen-Sdk/2.3/platforms/mobile-2.3/rootstraps/mobile-2.3-device.core
  CC    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-strip
  CXXFLAGS += -Os -g0
  ifeq ($(MODE), debug)
    CXXFLAGS += -g3 -Wno-literal-suffix
  endif
    TIZEN_INCLUDE = elementary-1 elocation-1 efl-1 ecore-x-1 eina-1 eina-1/eina eet-1 evas-1 ecore-1 ecore-evas-1 ecore-file-1 \
                  ecore-input-1 edje-1 eo-1 ethumb-client-1 emotion-1 ecore-imf-1 ecore-con-1 eio-1 eldbus-1 \
                  efreet-1 ecore-input-evas-1 ecore-audio-1 embryo-1 ecore-imf-evas-1 ethumb-1 eeze-1 eeze-1 e_dbus-1 e_dbus-1 dbus-1.0 network
  ifeq ($(VERSION), 3.0)
    TIZEN_INCLUDE += emile-1
  endif

  TIZEN_LIB = m elementary eina eet ecore ecore_file ecore_input edje ethumb_client ecore_imf ecore_con efreet efreet_mime \
              efreet_trash eio  evas ecore_evas ecore_x ecore_imf_evas

  DEPENDENCY_INCLUDE = zlib png

  CXXFLAGS += --sysroot=$(TIZEN_SYSROOT) -std=c++11
  CXXFLAGS +=  $(addprefix -I$(TIZEN_SYSROOT)/usr/include/, $(TIZEN_INCLUDE))
  CXXFLAGS +=  $(addprefix -I$(DEPENDENCY_ROOT_DIR)/include/, $(DEPENDENCY_INCLUDE))
  CXXFLAGS += -I$(TIZEN_SYSROOT)/usr/lib/dbus-1.0/include

  LDFLAGS += --sysroot=$(TIZEN_SYSROOT) -L$(DEPENDENCY_ROOT_DIR)/lib
  LDFLAGS += -Wl,--start-group ${ICU_LIB_PATH} ${DEPENDENCY_LIB_PATH} -Wl,--end-group
  LDFLAGS +=  $(addprefix -l, $(TIZEN_LIB))
  LIB = libWebWidgetEngine.so
else ifneq ($(filter $(HOST),tizen_wearable_arm tizen3_wearable_arm), )
  ifeq ($(HOST), tizen_wearable_arm)
    ifndef TIZEN_SDK_HOME
      $(error TIZEN_SDK_HOME must be set)
    endif
    VERSION=2.3.1
    TIZEN_ROOT=$(TIZEN_SDK_HOME)
  endif
  ifeq ($(HOST), tizen3_wearable_arm)
    ifndef TIZEN3_SDK_HOME
      $(error TIZEN3_SDK_HOME must be set)
    endif
    VERSION=3.0
    TIZEN_ROOT=$(TIZEN3_SDK_HOME)
  endif
  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/arm-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/wearable/rootstraps/wearable-$(VERSION)-device.core
  CC    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-strip
  CXXFLAGS += -Os -g0 -finline-limit=64
  ifeq ($(MODE), debug)
    CXXFLAGS += -g3 -Wno-literal-suffix -DSTARFISH_ENABLE_PIXEL_TEST
  endif
    TIZEN_INCLUDE = dlog elementary-1 elocation-1 efl-1 ecore-x-1 eina-1 eina-1/eina eet-1 evas-1 ecore-1 ecore-evas-1 ecore-file-1 \
                  ecore-input-1 edje-1 eo-1 emotion-1 ecore-imf-1 ecore-con-1 eio-1 eldbus-1 efl-extension \
                  efreet-1 ecore-input-evas-1 ecore-audio-1 embryo-1 ecore-imf-evas-1 ethumb-1 eeze-1 eeze-1 e_dbus-1 e_dbus-1 dbus-1.0 freetype2 media cairo  network
  ifeq ($(VERSION), 3.0)
    TIZEN_INCLUDE += emile-1 ethumb-client-1
  endif

  TIZEN_LIB = ecore evas rt efl-extension freetype capi-media-player cairo capi-network-connection

  DEPENDENCY_INCLUDE =

  CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE -DESCARGOT_TIZEN
  ifeq ($(VERSION), 3.0)
    CXXFLAGS += -DESCARGOT_TIZEN3
  endif
  CXXFLAGS += --sysroot=$(TIZEN_SYSROOT) -std=c++11
  CXXFLAGS +=  $(addprefix -I$(TIZEN_SYSROOT)/usr/include/, $(TIZEN_INCLUDE))
  CXXFLAGS +=  $(addprefix -I$(DEPENDENCY_ROOT_DIR)/include/, $(DEPENDENCY_INCLUDE))
  CXXFLAGS += -I$(TIZEN_SYSROOT)/usr/lib/dbus-1.0/include
  CXXFLAGS += -Ideps/tizen/include

  LDFLAGS += -Ldeps/tizen/lib/tizen-wearable-$(VERSION)-target-arm
  LDFLAGS += --sysroot=$(TIZEN_SYSROOT) -L$(DEPENDENCY_ROOT_DIR)/lib
  ifneq ($(VERSION), 3.0)
    ifeq ($(TYPE), lib)
      LDFLAGS += -static-libstdc++
    endif
  endif
  ifeq ($(TYPE), lib)
    CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE_APP
  else
    LDFLAGS += -Wl,--start-group ${ICU_LIB_PATH} ${DEPENDENCY_LIB_PATH} -Wl,--end-group
    LDFLAGS +=  $(addprefix -l, $(TIZEN_LIB))
    LDFLAGS +=  -ldlog -licui18n -licuuc -licudata -lecore -lecore_input -lecore_evas -levas -lelementary -lrt -lefl-extension -lfreetype -lcapi-media-player -lcairo -lfontconfig -lcapi-network-connection
  endif
  LIB = libWebWidgetEngine.so
else ifneq ($(filter $(HOST),tizen_wearable_emulator tizen3_wearable_emulator), )
  ifeq ($(HOST), tizen_wearable_emulator)
    ifndef TIZEN_SDK_HOME
      $(error TIZEN_SDK_HOME must be set)
    endif
    VERSION=2.3.1
    TIZEN_ROOT=$(TIZEN_SDK_HOME)
  endif
  ifeq ($(HOST), tizen3_wearable_emulator)
    ifndef TIZEN3_SDK_HOME
      $(error TIZEN3_SDK_HOME must be set)
    endif
    VERSION=3.0
    TIZEN_ROOT=$(TIZEN3_SDK_HOME)
  endif
  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/i386-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-$(VERSION)/wearable/rootstraps/wearable-$(VERSION)-emulator.core
  CC    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-strip
  CXXFLAGS += -Os -g0 -finline-limit=64
  ifeq ($(MODE), debug)
    CXXFLAGS += -g3 -Wno-literal-suffix
  endif
    TIZEN_INCLUDE = dlog elementary-1 elocation-1 efl-1 ecore-x-1 eina-1 eina-1/eina eet-1 evas-1 ecore-1 ecore-evas-1 ecore-file-1 network \
                  ecore-input-1 edje-1 eo-1 emotion-1 ecore-imf-1 ecore-con-1 eio-1 eldbus-1 efl-extension \
                  efreet-1 ecore-input-evas-1 ecore-audio-1 embryo-1 ecore-imf-evas-1 ethumb-1 eeze-1 eeze-1 e_dbus-1 e_dbus-1 dbus-1.0 freetype2 media cairo network

  TIZEN_LIB = ecore evas rt efl-extension freetype capi-media-player elementary fontconfig ecore_evas ecore_input cairo capi-network-connection

  DEPENDENCY_INCLUDE =

  CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE -DESCARGOT_TIZEN
  CXXFLAGS += --sysroot=$(TIZEN_SYSROOT) -std=c++11
  CXXFLAGS +=  $(addprefix -I$(TIZEN_SYSROOT)/usr/include/, $(TIZEN_INCLUDE))
  CXXFLAGS +=  $(addprefix -I$(DEPENDENCY_ROOT_DIR)/include/, $(DEPENDENCY_INCLUDE))
  CXXFLAGS += -I$(TIZEN_SYSROOT)/usr/lib/dbus-1.0/include
  CXXFLAGS += -Ideps/tizen/include

  LDFLAGS += --sysroot=$(TIZEN_SYSROOT) -L$(DEPENDENCY_ROOT_DIR)/lib
  LDFLAGS += -Wl,--start-group ${ICU_LIB_PATH} ${DEPENDENCY_LIB_PATH} -Wl,--end-group
  LDFLAGS += -Ldeps/tizen/lib/tizen-wearable-$(VERSION)-emulator-x86
  LDFLAGS +=  $(addprefix -l, $(TIZEN_LIB))
  ifneq ($(VERSION), 3.0)
    ifeq ($(TYPE), lib)
       LDFLAGS += -static-libstdc++
    else
       LDFLAGS += libstdc++.a
    endif
  endif
  ifeq ($(TYPE), lib)
    CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE_APP
  else
    LDFLAGS += -ldlog -licui18n -licuuc -licudata
  endif
  LIB = libWebWidgetEngine.so
ifeq ($(TYPE), exe)
  CXXFLAGS += -DSTARFISH_EMULATOR_RELEASE
endif


endif

ifeq ($(TYPE), lib)
  CXXFLAGS += -fPIC
  CFLAGS += -fPIC
endif


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

tizen_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_arm.exe.debug: $(OUTDIR)/$(BIN)
	$(STRIP) $<
	cp -f $< .
tizen_wearable_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_arm.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_wearable_arm.lib.release: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_wearable_emulator.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_emulator.exe.release: $(OUTDIR)/$(EBIN)
	cp -f $< .
tizen_wearable_emulator.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_wearable_emulator.lib.release: $(OUTDIR)/$(LIB)
	cp -f $< .

tizen3_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_arm.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_arm.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen3_wearable_arm.lib.release: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen3_wearable_emulator.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_emulator.exe.release: $(OUTDIR)/$(EBIN)
	cp -f $< .
tizen3_wearable_emulator.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen3_wearable_emulator.lib.release: $(OUTDIR)/$(LIB)
	cp -f $< .

$(OUTDIR)/$(EBIN): $(OBJS) $(THIRD_PARTY_LIBS)
	@echo "[LINK] $@"
	ln -sf deps/tizen/lib/tizen-wearable-$(VERSION)-emulator-x86/libstdc++.a.0 libstdc++.a
	$(CXX) -o $@ $(OBJS) $(THIRD_PARTY_LIBS) $(LDFLAGS)
	rm libstdc++.a

$(OUTDIR)/$(LIB): $(OBJS) $(THIRD_PARTY_LIBS)
	@echo "[LINK] $@"
	$(CXX) -shared -Wl,-soname,$(LIB) -o $@ $(OBJS) $(THIRD_PARTY_LIBS) $(LDFLAGS)

$(OUTDIR)/%.o: %.cpp Makefile
	echo "[CXX] $@"
	mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $< -o $@
	$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

$(OUTDIR)/%.o: %.cc Makefile
	echo "[CXX] $@"
	mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $< -o $@
	$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

$(OUTDIR)/%.o: %.c Makefile
	echo "[CC] $@"
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@
	$(CC) -MM $(CFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

clean:
	rm -rf out

strip:
	strip $(BIN)

asm:
	objdump -d        $(BIN) | c++filt > $(BIN).asm
	readelf -a --wide $(BIN) | c++filt > $(BIN).elf
	vi -O $(BIN).asm $(BIN).elf

ifeq (run,$(firstword $(MAKECMDGOALS)))
# use the rest as arguments for "run"
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
# ...and turn them into do-nothing targets
$(eval $(RUN_ARGS):;@:)
endif

run:
	phantomjs --web-security=false --local-to-remote-url-access=true runner.js ${RUN_ARGS}

install_pixel_test_dep:
	$(CXX) -O3 -g3 --std=c++11 -o tool/imgdiff/imgdiff tool/imgdiff/imgdiff.cpp $(shell pkg-config --cflags libpng) $(shell pkg-config --libs libpng)
	javac StarFishTester.java
	mkdir -p ~/.fonts
	cp tool/pixel_test/bin/AHEM____.TTF ~/.fonts/
	fc-cache -fv
	@echo ""
	@echo "============ [ Install Arial Font ] ============"
	sudo apt-get install ttf-mscorefonts-installer
	sudo fc-cache
	fc-match Arial

pixel_test:
	./tool/pixel_test/pixel_test.sh $(tc) $(screen)
pixel_test_css:
	java StarFishTester $(tc)
pixel_test_css1:
	make pixel_test_css tc=css1
pixel_test_css21:
	make pixel_test_css tc=css21
pixel_test_css3_color:
	make pixel_test_css tc=css3_color
pixel_test_css3_backgrounds:
	make pixel_test_css tc=css3_backgrounds
pixel_test_css3_transforms:
	make pixel_test_css tc=css3_transforms
pixel_test_css_all:
	make pixel_test_css1 2> out/pixel_test_css_all.log
	make pixel_test_css21 2>> out/pixel_test_css_all.log
	make pixel_test_css3_color 2>> out/pixel_test_css_all.log
	make pixel_test_css3_transforms 2>> out/pixel_test_css_all.log
	make pixel_test_css3_backgrounds 2>> out/pixel_test_css_all.log
	@echo '========== Show out/pixel_test_css_all.log =========='
	@cat out/pixel_test_css_all.log | grep "====total" | awk 'BEGIN {s=0}{ s += $$6 } END {print "Total "s" Passed (CSS1/21/3-color/3-backgrounds/3-transforms)"}'

reftest:
	./tool/reftest/reftest.sh $(tc) $(regression)

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

regression_test_demo:
	./tool/reftest/css_test.sh demo true

regression_test_dom_conformance_test:
	./tool/reftest/reftest.sh tool/reftest/dom_conformance_test.res true

regression_test_wpt_dom:
	./tool/reftest/reftest.sh tool/reftest/wpt_dom.res true
regression_test_wpt_dom_events:
	./tool/reftest/reftest.sh tool/reftest/wpt_dom_events.res true
regression_test_wpt_html:
	./tool/reftest/reftest.sh tool/reftest/wpt_html.res true
regression_test_wpt_page_visibility:
	./tool/reftest/reftest.sh tool/reftest/wpt_page_visibility.res true
regression_test_wpt_progress_events:
	./tool/reftest/reftest.sh tool/reftest/wpt_progress_events.res true
regression_test_wpt_xhr:
	./tool/reftest/reftest.sh tool/reftest/wpt_xhr.res true

regression_test_blink_dom_conformance_test:
	./tool/reftest/reftest.sh tool/reftest/blink_dom_conformance_test.res true
regression_test_blink_fast_dom:
	./tool/reftest/reftest.sh tool/reftest/blink_fast_dom.res true
regression_test_blink_fast_html:
	./tool/reftest/reftest.sh tool/reftest/blink_fast_html.res true
regression_test_gecko_dom_conformance_test:
	./tool/reftest/reftest.sh tool/reftest/gecko_dom_conformance_test.res true
regression_test_webkit_dom_conformance_test:
	./tool/reftest/reftest.sh tool/reftest/webkit_dom_conformance_test.res true
regression_test_webkit_fast_dom:
	./tool/reftest/reftest.sh tool/reftest/webkit_fast_dom.res true
regression_test_webkit_fast_html:
	./tool/reftest/reftest.sh tool/reftest/webkit_fast_html.res true

regression_test_bidi:
	./tool/reftest/reftest.sh tool/reftest/bidi.res true

regression_test_css1:
	./tool/reftest/css_test.sh css1 true
regression_test_css21:
	./tool/reftest/css_test.sh css21 true
regression_test_css3_color:
	./tool/reftest/css_test.sh css-color-3 true
regression_test_css3_backgrounds:
	./tool/reftest/css_test.sh css-backgrounds-3 true
regression_test_css3_transforms:
	./tool/reftest/css_test.sh css-transforms-1 true

regression_test_bidi.tizen_wearable_arm.debug:
	$(CXX) -O3 -g3 --std=c++11 $(CXXFLAGS) $(LDFLAGS) -o tool/imgdiff/imgdiffEvas.exe tool/imgdiff/imgdiffEvas.cpp
	./tool/reftest/setup_bidi_test.sh true

regression_test:
	make regression_test_dom_conformance_test
	make regression_test_wpt_dom
	make regression_test_wpt_dom_events
	make regression_test_wpt_html
	make regression_test_wpt_page_visibility
	make regression_test_wpt_progress_events
	make regression_test_wpt_xhr
	make regression_test_blink_dom_conformance_test
	make regression_test_blink_fast_dom
	make regression_test_blink_fast_html
	make regression_test_gecko_dom_conformance_test
	make regression_test_webkit_dom_conformance_test
	make regression_test_webkit_fast_dom
	make regression_test_webkit_fast_html
	make regression_test_bidi
	make regression_test_demo
	make regression_test_css1
	make regression_test_css21
	make regression_test_css3_color
	make regression_test_css3_backgrounds
	make regression_test_css3_transforms

tidy:
	./tool/tidy/check-webkit-style `find src/ -name "*.cpp" -o -name "*.h"`> error_report 2>& 1

.PHONY: clean
