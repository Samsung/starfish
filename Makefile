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
else ifneq (,$(findstring tizen3_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen3_wearable_emulator
  ARCH=x86
  TIZEN_ARCH=i386
  TIZEN_VERSION=3.0
  TIZEN_PROFILE=wearable
  TIZEN_DEVICE=emulator
else ifneq (,$(findstring tizen_obs_arm,$(MAKECMDGOALS)))
  HOST=tizen_obs
  TIZEN_VERSION=3.0
else ifneq (,$(findstring tizen_obs_emulator,$(MAKECMDGOALS)))
  HOST=tizen_obs
  ARCH=x86
  TIZEN_VERSION=3.0
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
  LTO=1
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
CXXFLAGS += -std=c++11
CXXFLAGS += -fno-rtti -fno-math-errno -Isrc/ -Iinc/
CXXFLAGS += -fdata-sections -ffunction-sections
CXXFLAGS += -frounding-math -fsignaling-nans
CXXFLAGS += -Wno-invalid-offsetof -fvisibility=hidden
CXXFLAGS += -DSTARFISH_ENABLE_TEST

LDFLAGS += -lpthread -lcurl

# fixme
# this causes
# /home/ksh8281/tizen-sdk-2.4.r2/tools/arm-linux-gnueabi-gcc-4.9/bin/../lib/gcc/arm-linux-gnueabi/4.9.2/../../../../arm-linux-gnueabi/bin/ld: BFD (GNU Binutils) 2.22 assertion fail ../../bfd/elf32-arm.c:12049
# LDFLAGS += -Wl,--gc-sections

ifeq ($(ARCH), x86)
  CXXFLAGS += -m32 -mfpmath=sse -msse2
  LDFLAGS += -m32
else ifeq ($(ARCH), arm)
  CXXFLAGS += -march=armv7-a -mthumb
endif

ifeq ($(MODE), debug)
  CXXFLAGS += $(CXXFLAGS_DEBUG)
else ifeq ($(MODE), release)
  CXXFLAGS += $(CXXFLAGS_RELEASE)
endif

# flags for debug/release
CXXFLAGS_DEBUG = -O0 -g3 -D_GLIBCXX_DEBUG -fno-omit-frame-pointer -Wall -Wextra -Werror
CXXFLAGS_DEBUG += -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-unused-parameter -Wno-unused-result
CXXFLAGS_RELEASE = -O2 -g3 -DNDEBUG -fomit-frame-pointer -fno-stack-protector -funswitch-loops -Wno-deprecated-declarations

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
  CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE

  CXXFLAGS_DEBUG += -Wno-literal-suffix
  CXXFLAGS_RELEASE += -USTARFISH_ENABLE_TEST

  ifeq ($(TYPE), lib)
    CXXFLAGS += -DSTARFISH_TIZEN_WEARABLE_LIB
  endif

  ifeq ($(TIZEN_VERSION), 3.0)
    CXXFLAGS += -DSTARFISH_TIZEN_3_0
  endif

  ifeq ($(HOST),tizen_obs)
    CXXFLAGS_DEBUG += -O1 # _FORTIFY_SOURCE requires compiling with optimization
  endif
endif

# for printing TC coverage log
ifeq ($(TC), 1)
  CXXFLAGS += -DSTARFISH_TC_COVERAGE
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

################################################################################
################################################################################
# SRCS & OBJS
################################################################################
################################################################################

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

ifeq ($(TYPE), lib)
  SRC += $(foreach dir, src/public , $(wildcard $(dir)/*.cpp))
else
  SRC += $(foreach dir, src/shell , $(wildcard $(dir)/*.cpp))
endif

# escargot
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/bdwgc/include/
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/src
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/double_conversion/
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/rapidjson/include/
CXXFLAGS += -I$(ESCARGOT_SRC_ROOT)/third_party/yarr/

ifeq ($(HOST), linux)
  JSLIBS = $(ESCARGOT_LIB_ROOT)/out/$(HOST)/$(ARCH)/interpreter/$(MODE)/libescargot.a
  GCLIBS = $(ESCARGOT_LIB_ROOT)/third_party/bdwgc/out/$(HOST)/$(ARCH)/$(MODE).shared/.libs/libgc.a
else ifeq ($(HOST), tizen_obs)
  JSLIBS = $(ESCARGOT_LIB_ROOT)/$(MODE)/libescargot.a
  GCLIBS = $(ESCARGOT_LIB_ROOT)/$(MODE)/libgc.a
else ifneq (,$(findstring tizen,$(HOST)))
  JSLIBS = $(ESCARGOT_LIB_ROOT)/out/tizen_$(TIZEN_VERSION)_$(TIZEN_PROFILE)/$(TIZEN_ARCH)/interpreter/$(MODE)/libescargot.a
  GCLIBS = $(ESCARGOT_LIB_ROOT)/third_party/bdwgc/out/tizen_$(TIZEN_VERSION)_$(TIZEN_PROFILE)/$(TIZEN_ARCH)/$(MODE).shared/.libs/libgc.a
endif

# deviceapi
ifeq ($(TIZEN_DEVICE_API), true)
  SRC += $(foreach dir, third_party/deviceapi/src , $(wildcard $(dir)/*.cpp))
endif

# skia_matrix
SRC += third_party/skia_matrix/SkMath.cpp
SRC += third_party/skia_matrix/SkPoint.cpp
SRC += third_party/skia_matrix/SkRect.cpp
SRC += third_party/skia_matrix/SkMatrix.cpp
SRC += third_party/skia_matrix/SkDebug.cpp

# clipper
SRC += third_party/clipper/cpp/clipper.cpp


# OBJS
OBJS := $(SRC:%.cpp= $(OUTDIR)/%.o)
THIRD_PARTY_OBJS := $(JSLIBS) $(GCLIBS)

################################################################################
################################################################################
# Toolchain
################################################################################
################################################################################

ifeq ($(HOST), linux)
  CC           = gcc
  CXX          = g++
  STRIP        = strip
  CXXFLAGS += $(shell pkg-config --cflags elementary ecore ecore-x libpng cairo freetype2 fontconfig icu-uc icu-i18n)
  LDFLAGS += $(shell pkg-config --libs elementary ecore ecore-x ecore-imf-evas libpng cairo freetype2 fontconfig icu-uc icu-i18n)
else ifeq ($(HOST), tizen_obs)
  CC           = gcc
  CXX          = g++
  STRIP        = strip
  TIZEN_DEPS = dlog elementary ecore libpng cairo freetype2 fontconfig icu-uc icu-i18n \
               ecore-imf-evas efl-extension libpng capi-network-connection capi-media-player
  CXXFLAGS    += $(shell pkg-config --cflags $(TIZEN_DEPS))
  LDFLAGS     += $(shell pkg-config --libs $(TIZEN_DEPS))
  LIB = libWebWidgetEngine.so
else ifneq (,$(findstring tizen,$(HOST)))

  # Toolchain & Platform
  COMPILER_PREFIX=$(TIZEN_ARCH)-linux-gnueabi
  TIZEN_SYSROOT=$(TIZEN_SDK_HOME)/platforms/tizen-$(TIZEN_VERSION)/$(TIZEN_PROFILE)/rootstraps/$(TIZEN_PROFILE)-$(TIZEN_VERSION)-$(TIZEN_DEVICE).core
  TIZEN_TOOLCHAIN=$(TIZEN_SDK_HOME)/tools/$(COMPILER_PREFIX)-gcc-4.9

  CC    = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-gcc
  CXX   = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-g++
  LINK  = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-g++
  LD    = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-ld
  AR    = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-ar
  STRIP = $(TIZEN_TOOLCHAIN)/bin/$(COMPILER_PREFIX)-strip

  TIZEN_INCLUDE = dlog elementary-1 elocation-1 efl-1 ecore-x-1 eina-1 eina-1/eina eet-1 evas-1 ecore-1 ecore-evas-1 ecore-file-1 \
                  ecore-input-1 edje-1 eo-1 emotion-1 ecore-imf-1 ecore-con-1 eio-1 eldbus-1 efl-extension \
                  efreet-1 ecore-input-evas-1 ecore-audio-1 embryo-1 ecore-imf-evas-1 ethumb-1 eeze-1 eeze-1 e_dbus-1 dbus-1.0 freetype2 media cairo network
  ifeq ($(TIZEN_VERSION), 3.0)
    TIZEN_INCLUDE += emile-1 ethumb-client-1
  endif
  TIZEN_LIB = ecore evas rt efl-extension freetype capi-media-player elementary fontconfig ecore_evas ecore_input cairo capi-network-connection dlog icui18n icuuc icudata

  CXXFLAGS += --sysroot=$(TIZEN_SYSROOT)
  CXXFLAGS +=  $(addprefix -I$(TIZEN_SYSROOT)/usr/include/, $(TIZEN_INCLUDE))
  CXXFLAGS += -Ideps/tizen/include
  CXXFLAGS += -I$(TIZEN_SYSROOT)/usr/lib/dbus-1.0/include

  LDFLAGS += --sysroot=$(TIZEN_SYSROOT)
  LDFLAGS +=  $(addprefix -l, $(TIZEN_LIB))
  LDFLAGS += -Ldeps/tizen/lib/tizen-$(TIZEN_PROFILE)-$(TIZEN_VERSION)-$(TIZEN_ARCH)


  # Workaround for platforms using gcc 4.6
  ifneq ($(TIZEN_VERSION), 3.0)
    ifeq ($(TYPE), lib)
      LDFLAGS += -static-libstdc++
    else
      LDFLAGS += $(TIZEN_TOOLCHAIN)/$(COMPILER_PREFIX)/lib/libstdc++.a
    endif
  endif

  LIB = libWebWidgetEngine.so
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
tizen_obs_arm.exe.debug: $(OUTDIR)/$(BIN)
tizen_obs_emulator.exe.debug: $(OUTDIR)/$(BIN)

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

install_git_prepush:
	cp -rf tool/reftest/pre-push .git/hooks/

install_pixel_test_dep:
	$(CXX) -O3 -g3 --std=c++11 -o tool/imgdiff/imgdiff tool/imgdiff/imgdiff.cpp $(shell pkg-config --cflags libpng) $(shell pkg-config --libs libpng)
	javac StarFishTester.java
	mkdir -p ~/.fonts
	cp tool/pixel_test/bin/AHEM____.TTF ~/.fonts/
#	cp tool/pixel_test/bin/CooHew-Roman.ttf ~/.fonts/
#	cp tool/pixel_test/bin/CooHew-Bold.ttf ~/.fonts/
	cp tool/pixel_test/bin/SamsungOne-300C_v1.0.ttf ~/.fonts/
	cp tool/pixel_test/bin/SamsungOne-600C_v1.0.ttf ~/.fonts/
	fc-cache -fv
	./set_nodewebkit_env.sh
	#@echo ""
	#@echo "============ [ Install Arial Font ] ============"
	#sudo apt-get install ttf-mscorefonts-installer
	#sudo fc-cache
	#fc-match Arial

pixel_test:
	./tool/pixel_test/pixel_test.sh $(tc) $(screen)
pixel_test_css:
	java StarFishTester $(tc) $(engine)
pixel_test_css1:
	make pixel_test_css tc=css1 engine=nw
pixel_test_css21:
	make pixel_test_css tc=css21 engine=nw
pixel_test_css3_color:
	make pixel_test_css tc=css3_color engine=nw
pixel_test_css3_backgrounds:
	make pixel_test_css tc=css3_backgrounds engine=nw
pixel_test_css3_transforms:
	make pixel_test_css tc=css3_transforms engine=nw
pixel_test_css_all:
	make pixel_test_css1 2> out/pixel_test_css_all.log
	make pixel_test_css21 2>> out/pixel_test_css_all.log
	make pixel_test_css3_color 2>> out/pixel_test_css_all.log
	make pixel_test_css3_transforms 2>> out/pixel_test_css_all.log
	make pixel_test_css3_backgrounds 2>> out/pixel_test_css_all.log
	@echo '========== Show out/pixel_test_css_all.log =========='
	@cat out/pixel_test_css_all.log | grep "% passed" | cut -d' ' -f1 | sort -d > out/wpt_css_passed.res
	@cat out/pixel_test_css_all.log | grep "====total" | awk 'BEGIN {s=0}{ s += $$6 } END {print "Total "s" Passed (CSS1/21/3-color/3-backgrounds/3-transforms)"}'
	mv tool/reftest/wpt_css_passed.res out/wpt_css_passed.orig
	cp out/wpt_css_passed.res tool/reftest/
	@diff out/wpt_css_passed.res out/wpt_css_passed.orig

internal_test:
	./test/reftest/internaltest.sh test/internal-test/test.res

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
	#./run_wpt_serve.sh
	./tool/reftest/reftest.sh tool/reftest/wpt_xhr.res true
	#-./kill_wpt_serve.sh

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
regression_test_webkit_fast_css:
	./tool/reftest/reftest.sh tool/reftest/webkit_fast_css.res true

regression_test_bidi:
	./tool/reftest/reftest.sh tool/reftest/bidi.res true

regression_test_css1:
	cat tool/reftest/wpt_css_passed.res tool/reftest/tclist/csswg_manual.res | grep css1 | sort -d > out/csswg_css1.res
	./tool/reftest/reftest.sh out/csswg_css1.res true
regression_test_css21:
	cat tool/reftest/wpt_css_passed.res tool/reftest/tclist/csswg_manual.res | grep css21 | sort -d > out/csswg_css21.res
	./tool/reftest/reftest.sh out/csswg_css21.res true
regression_test_css3_color:
	cat tool/reftest/wpt_css_passed.res tool/reftest/tclist/csswg_manual.res | grep css-color-3 | sort -d > out/csswg_css3_color.res
	./tool/reftest/reftest.sh out/csswg_css3_color.res true
regression_test_css3_backgrounds:
	cat tool/reftest/wpt_css_passed.res tool/reftest/tclist/csswg_manual.res | grep css-backgrounds | sort -d > out/csswg_css3_backgrounds.res
	./tool/reftest/reftest.sh out/csswg_css3_backgrounds.res true
regression_test_css3_transforms:
	cat tool/reftest/wpt_css_passed.res tool/reftest/tclist/csswg_manual.res | grep css-transforms | sort -d > out/csswg_css3_transforms.res
	./tool/reftest/reftest.sh out/csswg_css3_transforms.res true
regression_test_css:
	make regression_test_css1
	make regression_test_css21
	make regression_test_css3_color
	make regression_test_css3_backgrounds
	make regression_test_css3_transforms

font_dependent_test_css:
	./tool/reftest/reftest.sh tool/reftest/tclist/csswg_manual.res update

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
	make regression_test_webkit_fast_css
	make regression_test_bidi
	make regression_test_demo
	make regression_test_css1
	make regression_test_css21
	make regression_test_css3_color
	make regression_test_css3_backgrounds
	make regression_test_css3_transforms
	make internal_test

tidy:
	./tool/tidy/check-webkit-style `find src/ -name "*.cpp" -o -name "*.h"`> error_report 2>& 1

.PHONY: clean
