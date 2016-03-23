BUILDDIR=./build
HOST=linux

BIN=StarFish
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
else ifneq (,$(findstring tizen_arm,$(MAKECMDGOALS)))
  HOST=tizen_arm
else ifneq (,$(findstring tizen_wearable_emulator, $(MAKECMDGOALS)))
  HOST=tizen_wearable_emulator
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
  CXXFLAGS += $(shell pkg-config --cflags elementary ecore ecore-x libpng cairo freetype2 fontconfig)
  LDFLAGS += $(shell pkg-config --libs elementary ecore ecore-x ecore-imf-evas libpng cairo freetype2 fontconfig)
else ifeq ($(HOST), tizen_arm)
  OUTDIR=out/tizen_$(ARCH)/$(TYPE)/$(MODE)
  	TIZEN_INCLUDE = elementary-1 elocation-1 efl-1 ecore-x-1 eina-1 eina-1/eina eet-1 evas-1 ecore-1 ecore-evas-1 ecore-file-1 \
			ecore-input-1 edje-1 eo-1 ethumb-client-1 emotion-1 ecore-imf-1 ecore-con-1 eio-1 eldbus-1 \
			efreet-1 ecore-input-evas-1 ecore-audio-1 embryo-1 ecore-imf-evas-1 ethumb-1 eeze-1 eeze-1 e_dbus-1 e_dbus-1 dbus-1.0

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
else ifeq ($(HOST), tizen_wearable_arm)
  OUTDIR=out/tizen_$(ARCH)/$(TYPE)/$(MODE)
  	TIZEN_INCLUDE = dlog elementary-1 elocation-1 efl-1 ecore-x-1 eina-1 eina-1/eina eet-1 evas-1 ecore-1 ecore-evas-1 ecore-file-1 \
			ecore-input-1 edje-1 eo-1 emotion-1 ecore-imf-1 ecore-con-1 eio-1 eldbus-1 efl-extension \
			efreet-1 ecore-input-evas-1 ecore-audio-1 embryo-1 ecore-imf-evas-1 ethumb-1 eeze-1 eeze-1 e_dbus-1 e_dbus-1 dbus-1.0 freetype2 media

	TIZEN_LIB = ecore evas rt efl-extension freetype capi-media-player

	DEPENDENCY_INCLUDE =

	CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE
	CXXFLAGS += --sysroot=$(TIZEN_SYSROOT) -std=c++11
	CXXFLAGS +=  $(addprefix -I$(TIZEN_SYSROOT)/usr/include/, $(TIZEN_INCLUDE))
	CXXFLAGS +=  $(addprefix -I$(DEPENDENCY_ROOT_DIR)/include/, $(DEPENDENCY_INCLUDE))
	CXXFLAGS += -I$(TIZEN_SYSROOT)/usr/lib/dbus-1.0/include

	LDFLAGS += --sysroot=$(TIZEN_SYSROOT) -L$(DEPENDENCY_ROOT_DIR)/lib
	LDFLAGS += -Wl,--start-group ${ICU_LIB_PATH} ${DEPENDENCY_LIB_PATH} -Wl,--end-group
	LDFLAGS +=  $(addprefix -l, $(TIZEN_LIB))
else ifeq ($(HOST), tizen_wearable_emulator)
  OUTDIR=out/tizen_$(ARCH)/$(TYPE)/$(MODE)
	TIZEN_INCLUDE = dlog elementary-1 elocation-1 efl-1 ecore-x-1 eina-1 eina-1/eina eet-1 evas-1 ecore-1 ecore-evas-1 ecore-file-1 \
			ecore-input-1 edje-1 eo-1 emotion-1 ecore-imf-1 ecore-con-1 eio-1 eldbus-1 efl-extension \
			efreet-1 ecore-input-evas-1 ecore-audio-1 embryo-1 ecore-imf-evas-1 ethumb-1 eeze-1 eeze-1 e_dbus-1 e_dbus-1 dbus-1.0 freetype2 media

	TIZEN_LIB = ecore evas rt efl-extension freetype capi-media-player elementary fontconfig ecore_evas ecore_input

	DEPENDENCY_INCLUDE =
 
	CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE
	CXXFLAGS += --sysroot=$(TIZEN_SYSROOT) -std=c++11
	CXXFLAGS +=  $(addprefix -I$(TIZEN_SYSROOT)/usr/include/, $(TIZEN_INCLUDE))
	CXXFLAGS +=  $(addprefix -I$(DEPENDENCY_ROOT_DIR)/include/, $(DEPENDENCY_INCLUDE))
	CXXFLAGS += -I$(TIZEN_SYSROOT)/usr/lib/dbus-1.0/include
 
	LDFLAGS += --sysroot=$(TIZEN_SYSROOT) -L$(DEPENDENCY_ROOT_DIR)/lib
	LDFLAGS += -Wl,--start-group ${ICU_LIB_PATH} ${DEPENDENCY_LIB_PATH} -Wl,--end-group
	LDFLAGS +=  $(addprefix -l, $(TIZEN_LIB))
endif

$(info host... $(HOST))
$(info arch... $(ARCH))
$(info type... $(TYPE))
$(info mode... $(MODE))
$(info build dir... $(OUTDIR))

CXXFLAGS += -std=c++11

ifeq ($(ARCH), x86)
  CXXFLAGS += -m32 -mfpmath=sse -msse2 -DESCARGOT_32=1
  LDFLAGS += -m32
else ifeq ($(ARCH), arm)
  CXXFLAGS += -DESCARGOT_32=1 -march=armv7-a -mthumb
else
CXXFLAGS += -DESCARGOT_64=1
endif

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

LDFLAGS += -lescargot

ifeq ($(MODE), debug)
    LDFLAGS += -Lthird_party/escargot/out/x64/interpreter/debug
    # TODO : use this option only for HOST=linux
    LDFLAGS += -Wl,-rpath=$(LOCAL_PATH)/third_party/escargot/out/x64/interpreter/debug
else
	ifeq ($(ARCH), x64)
    LDFLAGS += -Lthird_party/escargot/out/x64/interpreter/release
    # TODO : use this option only for HOST=linux
    LDFLAGS += -Wl,-rpath=$(LOCAL_PATH)/third_party/escargot/out/x64/interpreter/release
    else
    LDFLAGS += -Lthird_party/escargot/out/$(ARCH)/interpreter/release
    endif
endif

# tinyxml
CXXFLAGS += -Ithird_party/tinyxml2/

#skia_matrix
CXXFLAGS += -Ithird_party/skia_matrix/


#######################################################
# SRCS & OBJS
#######################################################

SRC=
SRC += $(foreach dir, src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/dom , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/extra , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/dom/builder , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/dom/binding , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/layout , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/style , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/util , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/message_loop , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/window , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/canvas , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/canvas/image , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/platform/canvas/font , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/public , $(wildcard $(dir)/*.cpp))

ifeq ($(TYPE), lib)
else
  SRC += $(foreach dir, src/shell , $(wildcard $(dir)/*.cpp))
endif

# tinyxml
SRC += third_party/tinyxml2/tinyxml2.cpp

# skia_matrix
SRC += third_party/skia_matrix/SkMath.cpp
SRC += third_party/skia_matrix/SkPoint.cpp
SRC += third_party/skia_matrix/SkRect.cpp
SRC += third_party/skia_matrix/SkMatrix.cpp
SRC += third_party/skia_matrix/SkDebug.cpp

ifeq ($(TIZEN_DEVICE_API), true)
	SRC += third_party/deviceapi/src/TizenDeviceAPILoaderForEscargot.cpp
endif

SRC_CC =

OBJS := $(SRC:%.cpp= $(OUTDIR)/%.o)
OBJS += $(SRC_CC:%.cc= $(OUTDIR)/%.o)
OBJS += $(SRC_C:%.c= $(OUTDIR)/%.o)


LDFLAGS += -lpthread -lcurl
THIRD_PARTY_LIBS= $(GCLIBS)

ifeq ($(HOST), linux)
  CC           = gcc
  CXX          = g++
  STRIP        = strip
  CXXFLAGS += -DSTARFISH_ENABLE_PIXEL_TEST
else ifeq ($(HOST), tizen_arm)
  ifndef TIZEN_SDK_HOME
    $(error TIZEN_SDK_HOME must be set)
  endif
  TIZEN_ROOT=$(TIZEN_SDK_HOME)
  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/arm-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-2.4/mobile/rootstraps/mobile-2.4-device.core
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
else ifeq ($(HOST), tizen_wearable_arm)
  ifndef TIZEN_SDK_HOME
    $(error TIZEN_SDK_HOME must be set)
  endif
  TIZEN_ROOT=$(TIZEN_SDK_HOME)
  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/arm-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-device.core
  CC    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/arm-linux-gnueabi-strip
  CXXFLAGS += -Os -g0 -finline-limit=64 -s
else ifeq ($(HOST), tizen_wearable_emulator)
  ifndef TIZEN_SDK_HOME
    $(error TIZEN_SDK_HOME must be set)
  endif
  TIZEN_ROOT=$(TIZEN_SDK_HOME)
  TIZEN_TOOLCHAIN=$(TIZEN_ROOT)/tools/i386-linux-gnueabi-gcc-4.9
  TIZEN_SYSROOT=$(TIZEN_ROOT)/platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-emulator.core
  CC    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/i386-linux-gnueabi-strip
  CXXFLAGS += -Os -g0 -finline-limit=64
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
	cp -f $< .
tizen_wearable_arm.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_arm.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_wearable_arm.lib.release: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_wearable_emulator.exe.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_emulator.exe.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_emulator.lib.debug: $(OUTDIR)/$(LIB)
	cp -f $< .
tizen_wearable_emulator.lib.release: $(OUTDIR)/$(LIB)
	cp -f $< .





$(OUTDIR)/$(BIN): $(OBJS) $(THIRD_PARTY_LIBS)
	@echo "[LINK] $@"
	$(CXX) -o $@ $(OBJS) $(THIRD_PARTY_LIBS) $(LDFLAGS)

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

install_runner_dep:
	sudo apt-get install phantomjs

ifeq (run,$(firstword $(MAKECMDGOALS)))
# use the rest as arguments for "run"
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
# ...and turn them into do-nothing targets
$(eval $(RUN_ARGS):;@:)
endif

run:
	phantomjs --web-security=false --local-to-remote-url-access=true runner.js ${RUN_ARGS}

install_pixel_test_dep:
	mkdir -p ~/.fonts
	cp tool/pixel_test/bin/AHEM____.TTF ~/.fonts/
	fc-cache -fv
	select CONTINUE in nodejs node; \
	do \
		rm tool/pixel_test/bin/nodejs; \
		ln -s `which $$CONTINUE` tool/pixel_test/bin/nodejs; \
		ls -al tool/pixel_test/bin/nodejs; \
		exit 0; \
	done; \
	@echo "..done"

pixel_test:
	if [[ ! -f tool/pixel_test/bin/nodejs ]]; then \
		make install_pixel_test_dep; \
	fi;
	./tool/pixel_test/pixel_test.sh $(tc) $(screen)
pixel_test_css1:
	make pixel_test tc=tool/pixel_test/css1.res screen=pc
pixel_test_css21:
	make pixel_test tc=tool/pixel_test/css21.res screen=pc
pixel_test_css3_color:
	make pixel_test tc=tool/pixel_test/css-color-3.res screen=pc
pixel_test_css3_backgrounds:
	make pixel_test tc=tool/pixel_test/css-backgrounds-3.res screen=pc
pixel_test_css3_transforms:
	make pixel_test tc=tool/pixel_test/css-transforms-1.res screen=pc

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

regression_test:
	if [[ ! -f tool/pixel_test/bin/nodejs ]]; then \
		make install_pixel_test_dep; \
	fi;
	if [[ -f test/reftest/Regression/demo/20160115/*/*.png ]]; then \
		rm test/reftest/Regression/demo/20160115/*/*.png_; \
	fi;
	./tool/pixel_test/regression_test.sh $(tc)

tidy:
	./tool/tidy/check-webkit-style `find src/ -name "*.cpp" -o -name "*.h"`> error_report 2>& 1

.PHONY: clean
