#
# Copyright (c) 2018-present Samsung Electronics Co., Ltd
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

Name:          lightweight-web-engine
Summary:       Lightweight Web Engine for Tizen
Version:       1.5.2
Release:       1
Group:         Development/Libraries
License:       LGPL-2.1+ and BSD-2-Clause and BSD-3-Clause and BSL-1.0 and MIT and ISC and Zlib and BOEHM-GC and ICU
Source:        %{name}-%{version}.tar.gz
#ExclusiveArch: %arm

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

# RPM ref: http://backreference.org/2011/09/17/some-tips-on-rpm-conditional-macros/

# [ tv | mobile | wearable ]
# The following syntax's been outdated.
# %if %{?tizen_profile_name:1}%{!?tizen_profile_name:0}
# %define profile %{tizen_profile_name}
# %else
# %define profile undefined
# %endif

# [ tv | headless | mobile | wearable | all ]
# build for all profile
%if 0%{?build_profile:1}
%define rpm %{build_profile}
%else
%define rpm all
%endif

%if 0%{?use_embedded_image_decoder:1}
%else
%define use_embedded_image_decoder 0
%endif

%if %{?skip_config:0}%{!?skip_config:1}
%define skip_config 0
%endif

%if 0%{?tizen_version_major:1}
%else
%define tizen_version_major 4
%endif

%if 0%{?tizen_version_minor:1}
%else
%define tizen_version_minor 0
%endif

%if %{?_vd_cfg_product_type:1}%{!?_vd_cfg_product_type:0}
  %if "%{_vd_cfg_product_type}" == "AUDIO" || "%{rpm}" == "headless"
%define rpm headless
  %else
    %if "%{_vd_cfg_product_type}" == "TV" || "%{_vd_cfg_product_type}" == "LFD" || "%{_vd_cfg_product_type}" == "IWB" || "%{_vd_cfg_product_type}" == "WALL"
%define rpm prod_tv
    %endif
  %endif
%endif

%if 0%{?sec_product_feature_profile_wearable} == 1
%define rpm wearable
%endif

%if 0%{?rebuild_force:1}
%define force_build 1
%else
%define force_build 0
%endif

%if 0%{?disable_lto:1}
%define using_lto 0
%else
%if (0%{?tizen_version_major} == 5) && (0%{?tizen_version_minor} == 5) && %{?_vd_cfg_product_type:1}%{!?_vd_cfg_product_type:0}
%define using_lto 0
%else
%if 0%{?tizen_version_major} >= 5
%define using_lto 1
%else
%define using_lto 0
%endif
%endif
%endif

# Untested below Tizen 9 -- off there, on (default) everywhere else.
%if 0%{?tizen_version_major} <= 8
%define enable_tls_access_by_pthread_key 0
%else
%define enable_tls_access_by_pthread_key 1
%endif

%if 0%{?enable_codecache:1}
%else
%define enable_codecache 0
%endif

%if 0%{?enable_wasm:1}
%else
%define enable_wasm 0
%endif

%if 0%{?enable_debugger:1}
%else
%define enable_debugger 0
%endif

%if 0%{?enable_test:1}
%else
%define enable_test 0
%endif

%if 0%{?enable_webrtc:1}
%else
%define enable_webrtc 0
%endif

# -DENABLE_ESPLUSPLAYER flag: only turned on for the "mobile" profile on
# Tizen 10 or higher (esplusplayer MSE backend is unsupported before Tizen 10).
# It is fed through the shared features_config, so leaving it off for "all" keeps
# the unified_tv/wearable/etc. sub-builds from linking the esplusplayer backend.
# (The unified_mobile sub-build does not need this flag anyway: config.cmake
# force-enables ENABLE_ESPLUSPLAYER for CUSTOM=unified_mobile on Tizen 10+.)
%if 0%{?enable_esplusplayer:1}
%else
%if "%{rpm}" == "mobile" && 0%{?tizen_version_major} >= 10
%define enable_esplusplayer 1
%else
%define enable_esplusplayer 0
%endif
%endif

# BuildRequires gate is broader than the -D flag: the unified_mobile sub-build
# runs for both "mobile" and "all", and config.cmake force-requires
# pkgconfig(esplusplayer) for that CUSTOM on Tizen 10+. Without this the
# buildroot never pulls the package in and cmake configuration fails for "all".
%if 0%{?need_esplusplayer_pkg:1}
%else
%if ("%{rpm}" == "mobile" || "%{rpm}" == "all") && 0%{?tizen_version_major} >= 10
%define need_esplusplayer_pkg 1
%else
%define need_esplusplayer_pkg 0
%endif
%endif

%if 0%{?asan:1}
%else
%define asan 0
%endif

%if 0%{?enable_sharedworker:1}
%else
%define enable_sharedworker 0
%endif

%if 0%{?enable_serviceworker:1}
%else
%define enable_serviceworker 0
%endif

%if 0%{?disable_shell:1}
%else
%define disable_shell 0
%endif

%if (0%{?tizen_version_major} >= 9)
# The features below are supported by default in Tizen 9 or higher unless explicitly specified.
# To set features explicitly, add a define as shown below.
# Ex) --define 'enable_webgl {0|1}' --define 'enable_dynamic_loader {0|1}'
%define is_dynamic_loader_supported 1
%define is_worker_supported 1

  %if "%{rpm}" == "headless"
%define is_webgl_supported 0
  %else
%define is_webgl_supported 1
  %endif

%else
%define is_dynamic_loader_supported 0
%define is_webgl_supported 0
%define is_worker_supported 0
%endif

%if 0%{?enable_dynamic_loader:1}
%else
%define enable_dynamic_loader %{is_dynamic_loader_supported}
%endif

%if 0%{?enable_webgl:1}
%else
%define enable_webgl %{is_webgl_supported}
%endif

%if 0%{?enable_worker:1}
%else
%define enable_worker %{is_worker_supported}
%endif

# The following syntax's been outdated.
# %if "%{?TIZEN_PRODUCT_TV}" == "1"
# %define profile tv
# %else
# %if "%{?TIZEN_PRODUCT_MOBILE}" == "1"
# %define profile mobile
# %else
# %if "%{?TIZEN_PRODUCT_WEARABLE}" == "1"
# %define profile wearable
# %else
#  default profile
# %define profile undefined
# %endif
# %endif
# %endif

# build requirements
BuildRequires: make
BuildRequires: cmake
BuildRequires: ninja
BuildRequires: patchelf
BuildRequires: python
BuildRequires: python3
BuildRequires: unzip
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libtzplatform-config)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(cairo)
BuildRequires: pkgconfig(harfbuzz)
BuildRequires: pkgconfig(libcurl)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(capi-appfw-app-common)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(capi-media-sound-manager)
BuildRequires: pkgconfig(capi-media-audio-io)
BuildRequires: pkgconfig(capi-location-manager)
BuildRequires: pkgconfig(tts)
BuildRequires: libasound-devel

%if 0%{?tizen_version_major} >= 11
BuildRequires: pkgconfig(tizen-core)
BuildRequires: pkgconfig(tizen-core-wl)
BuildRequires: pkgconfig(tizen-core-imf)
%define lwe_backend glib_cairo_gl
%define lwe_shell_type tcore_wl
%define lwe_headless_backend glib_headless
%define lwe_headless_shell_type tcore_headless
%else
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(ecore-evas)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(efl-extension)
%define lwe_backend glib_cairo_gl
%define lwe_shell_type efl
%define lwe_headless_backend glib_headless
%define lwe_headless_shell_type efl_headless
%endif

%if (0%{?tizen_version_major} >= 6) && ("%{rpm}" != "flutter")
BuildRequires: pkgconfig(libwebp)
%endif

%if "%{?use_embedded_image_decoder}" == "1"
%else
BuildRequires: giflib-devel
BuildRequires: libjpeg-turbo-devel
%endif

%%if (0%{?tizen_version_major} >= 6)
  # Tizen 10.0 and below build against OpenSSL 1.1, Tizen 10.1 and above against
  # OpenSSL 3. libopenssl3-devel conflicts with libopenssl1.1-devel and both own
  # /usr/include/openssl and /usr/lib/lib{ssl,crypto}.so, so this choice also
  # decides what the libwebsockets sub-build links against.
  %if (0%{?tizen_version_major} > 10) || ((0%{?tizen_version_major} == 10) && (0%{?tizen_version_minor} >= 1))
BuildRequires: pkgconfig(openssl3)
  %else
BuildRequires: pkgconfig(openssl1.1)
  %endif
%else
  %if (0%{?tizen_version_major} == 5) && (0%{?tizen_version_minor} == 5)
    %if "%{rpm}" == "prod_tv" || "%{rpm}" == "headless"
BuildRequires: pkgconfig(openssl)
    %else
BuildRequires: pkgconfig(openssl1.1)
    %endif
  %else
BuildRequires: pkgconfig(openssl)
  %endif
%endif

BuildRequires: pkgconfig(libpulse)

%if "%{rpm}" == "tv" || "%{rpm}" == "prod_tv" || "%{rpm}" == "mobile" || "%{rpm}" == "wearable" || "%{rpm}" == "all"
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(capi-system-device)
%endif

%if "%{rpm}" == "prod_tv"
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(vconf-internal-keys-tv)
BuildRequires: pkgconfig(capi-media-tool)
%endif

# Touch-exploration accessibility + AT-SPI2 provider (ENABLE_A11Y_TOUCH)
%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(atk)
BuildRequires: pkgconfig(atk-bridge-2.0)
BuildRequires: pkgconfig(atspi-2)
BuildRequires: pkgconfig(dbus-1)
%endif

%if 0%{?enable_webrtc:1}
BuildRequires: pkgconfig(capi-media-camera)
BuildRequires: pkgconfig(capi-media-tool)
%endif

%if 0%{?need_esplusplayer_pkg} == 1
BuildRequires: pkgconfig(esplusplayer)
%endif

%if 0%{?asan} == 1
BuildRequires: libasan
%endif

BuildRequires: pkgconfig(bundle)

%if "%{?build_shell_tpk}" == "1" || "%{?build_uwe_tpk}" == "1"
BuildRequires: hash-signer, zip
BuildRequires: squashfs
  %if "%{rpm}" == "prod_tv"
BuildRequires: app-signer
BuildRequires: sdk-core
  %endif
%endif

# If you want to speed up the gbs build for devel, please uncomment below block.
#%ifarch armv7l
#BuildRequires: clang-accel-armv7l-cross-arm
#%endif # arm7l
#%ifarch aarch64
#BuildRequires: clang-accel-aarch64-cross-aarch64
#%endif # aarch64

# Supporting multiprofiles
# Use profile_mobile as default, as it is both minimal and
# platform-independent version of LWE at the time of writing
# TODO: Creates a profile_common if this is no longer true.
Requires: %{name}-compat = %{version}-%{release}
Recommends: %{name}-profile_mobile = %{version}-%{release}

%description
This package provides a Tizen specific implementation of Lightweight Web Engine.


##############################################
# Packages for profiles
##############################################
%if "%{rpm}" == "tv" || "%{rpm}" == "prod_tv" || "%{rpm}" == "all"
%package profile_tv
Summary:     Lightweight Web Engine for tv
Provides:    %{name}-compat = %{version}-%{release}
Conflicts:   %{name}-profile_headless = %{version}-%{release}
Conflicts:   %{name}-profile_mobile = %{version}-%{release}
Conflicts:   %{name}-profile_wearable = %{version}-%{release}
%description profile_tv
Lightweight Web Engine for tv
%endif

%if "%{rpm}" == "headless"
%package profile_headless
Summary:     Lightweight Web Engine for headless
Provides:    %{name}-compat = %{version}-%{release}
Conflicts:   %{name}-profile_tv = %{version}-%{release}
Conflicts:   %{name}-profile_mobile = %{version}-%{release}
Conflicts:   %{name}-profile_wearable = %{version}-%{release}
%description profile_headless
Lightweight Web Engine for headless
%endif

%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
%package profile_mobile
Summary:     Lightweight Web Engine for mobile
Provides:    %{name}-compat = %{version}-%{release}
Conflicts:   %{name}-profile_tv = %{version}-%{release}
Conflicts:   %{name}-profile_headless = %{version}-%{release}
Conflicts:   %{name}-profile_wearable = %{version}-%{release}
%description profile_mobile
Lightweight Web Engine for mobile
%endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%package profile_wearable
Summary:     Lightweight Web Engine for wearable
Provides:    %{name}-compat = %{version}-%{release}
Conflicts:   %{name}-profile_tv = %{version}-%{release}
Conflicts:   %{name}-profile_headless = %{version}-%{release}
Conflicts:   %{name}-profile_mobile = %{version}-%{release}
%description profile_wearable
Lightweight Web Engine for wearable
%endif

%if "%{rpm}" == "flutter"
%package profile_flutter
Summary:     Lightweight Web Engine for flutter
Provides:    %{name}-compat = %{version}-%{release}
Conflicts:   %{name}-profile_tv = %{version}-%{release}
Conflicts:   %{name}-profile_headless = %{version}-%{release}
Conflicts:   %{name}-profile_wearable = %{version}-%{release}
Conflicts:   %{name}-profile_mobile = %{version}-%{release}
%description profile_flutter
Lightweight Web Engine for flutter
%endif

%package devel
Summary:     Development files for Lightweight Web Engine
Group:       Development/Libraries
Requires:    %{name} = %{version}
%description devel
Development files for Lightweight Web Engine. This package provides
headers and package configs.

%if "%{?disable_shell}" == "0"

%if "%{rpm}" == "tv" || "%{rpm}" == "prod_tv"
%package shell-profile_tv
Summary:     Development files for Lightweight Web Engine for tv
Requires:    %{name}-profile_tv
Conflicts:   %{name}-shell-profile_headless = %{version}-%{release}
Conflicts:   %{name}-shell-profile_mobile = %{version}-%{release}
Conflicts:   %{name}-shell-profile_wearable = %{version}-%{release}
%description shell-profile_tv
Development files for Lightweight Web Engine for tv. This package provides
a standalone executable binary for tv.
%endif

%if "%{rpm}" == "headless"
%package shell-profile_headless
Summary:     Development files for Lightweight Web Engine for headless
Requires:    %{name}-profile_headless
Conflicts:   %{name}-shell-profile_tv = %{version}-%{release}
Conflicts:   %{name}-shell-profile_mobile = %{version}-%{release}
Conflicts:   %{name}-shell-profile_wearable = %{version}-%{release}
%description shell-profile_headless
Development files for Lightweight Web Engine for headless. This package provides
a standalone executable binary for headless.
%endif

%if "%{rpm}" == "mobile"
%package shell-profile_mobile
Summary:     Development files for Lightweight Web Engine for mobile
Requires:    %{name}-profile_mobile
Conflicts:   %{name}-shell-profile_tv = %{version}-%{release}
Conflicts:   %{name}-shell-profile_headless = %{version}-%{release}
Conflicts:   %{name}-shell-profile_wearable = %{version}-%{release}
%description shell-profile_mobile
Development files for Lightweight Web Engine for mobile. This package provides
a standalone executable binary for mobile.
%endif

%if "%{rpm}" == "wearable"
%package shell-profile_wearable
Summary:     Development files for Lightweight Web Engine for wearable
Requires:    %{name}-profile_wearable
Conflicts:   %{name}-shell-profile_tv = %{version}-%{release}
Conflicts:   %{name}-shell-profile_headless = %{version}-%{release}
Conflicts:   %{name}-shell-profile_mobile = %{version}-%{release}
%description shell-profile_wearable
Development files for Lightweight Web Engine for wearable. This package provides
a standalone executable binary for wearable.
%endif

%if "%{rpm}" == "flutter"
%package shell-profile_flutter
Summary:     Development files for Lightweight Web Engine for flutter
Requires:    %{name}-profile_flutter
Conflicts:   %{name}-shell-profile_tv = %{version}-%{release}
Conflicts:   %{name}-shell-profile_headless = %{version}-%{release}
Conflicts:   %{name}-shell-profile_wearable = %{version}-%{release}
Conflicts:   %{name}-shell-profile_mobile = %{version}-%{release}
%description shell-profile_flutter
Development files for Lightweight Web Engine for flutter. This package provides
a standalone executable binary for flutter.
%endif

%endif

##############################################
# Prep
##############################################
%prep
%setup -q

##############################################
# Build
##############################################
%build
echo "Building for: " %{rpm}
%global gbs_root_id %(ls -di . | awk '{print $1}')
echo "Make gbs build id from file inode({GBS-ROOT}/local/BUILD-ROOTS/scratch.{ARCH}.0/home/abuild): " %{gbs_root_id}

# Define output folder
%define out_folder out_tizen/build_%{gbs_root_id}

# Setup Jinja2, ply
%define binding_src_path binding_generator/pip_archive
%define binding_install_path %{out_folder}/binding_generator_python_packages

if [ ! -f "%{binding_install_path}/DONE" ]; then
 unzip -o %{binding_src_path}/Jinja2-3.1.2-py3-none-any.whl -d %{binding_install_path}
 unzip -o %{binding_src_path}/ply-3.11-py2.py3-none-any.whl -d %{binding_install_path}
 unzip -o %{binding_src_path}/MarkupSafe-2.1.3-cp38-cp38-manylinux_2_17_x86_64.manylinux2014_x86_64.whl -d %{binding_install_path}
 touch %{binding_install_path}/DONE
fi
export PYTHONPATH=$PWD/%{binding_install_path}

CXXFLAGS+=' -DSTARFISH_TIZEN_MAJOR_VERSION=%{tizen_version_major} '
CXXFLAGS+=' -DSTARFISH_TIZEN_VERSION_%{tizen_version_major}_%{tizen_version_minor} '
%if 0%{?build_option:1}
%if "%{build_option}" == "evas_gl_transparent_window"
CXXFLAGS+=' -DSTARFISH_ENABLE_TRANSPARENT_WINDOW '
%endif
%endif

##############################################
# Asan with lto leads internal compiler error
##############################################
%if 0%{?asan} == 1
CFLAGS+=' -fno-lto '
CXXFLAGS+=' -fno-lto '
%endif

##############################################
# Disable lto option
##############################################
%if 0%{?using_lto} == 0
CFLAGS+=' -fno-lto '
CXXFLAGS+=' -fno-lto '
%endif

##############################################
# Disable userfaultfd write-protect VDB on prod_tv
# (Tizen x86_64 build env kernel headers lack UFFDIO_WRITEPROTECT support)
##############################################
%if "%{rpm}" == "prod_tv"
CFLAGS+=' -DNO_UFFDWP_VDB '
%endif

##############################################
## Build rules for each profile
##############################################
%define fp_mode soft
%ifarch armv7l armv7hl
%define tizen_arch arm
%endif
%ifarch armv7hl
%define fp_mode hard
%endif
%ifarch aarch64
%define tizen_arch aarch64
%endif
%ifarch i686
%define tizen_arch i686
%endif
%ifarch x86_64
%define tizen_arch x86_64
%endif
%ifarch riscv64
%define tizen_arch riscv64
%endif

# Variables for build
# This features_config values are used in cmake command.
%define features_config -DWORKER='%{enable_worker}' -DSHARED_WORKER='%{enable_sharedworker}' \\\
  -DSERVICE_WORKER='%{enable_serviceworker}' -DENABLE_TLS_ACCESS_BY_PTHREAD_KEY='%{enable_tls_access_by_pthread_key}' \\\
  -DWEBRTC='%{enable_webrtc}' -DWEBGL='%{enable_webgl}' \\\
  -DENABLE_ESPLUSPLAYER='%{enable_esplusplayer}'

%if "%{rpm}" == "tv" || "%{rpm}" == "all"
%define out_tizen %{out_folder}/unified_tv/release

# For Cairo
cmake CMakeLists.txt -B%{out_tizen} -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
  -DTIZEN_MAJOR_VERSION='%{tizen_version_major}' -DTIZEN_MINOR_VERSION='%{tizen_version_minor}' \
  -DUSE_EMBEDDED_IMAGE_DECODER='%{use_embedded_image_decoder}' -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_SYSTEM_NAME=Tizen \
  -DCMAKE_SYSTEM_PROCESSOR='%{tizen_arch}' -DFP_MODE='%{fp_mode}' -DCUSTOM=unified_tv -DBACKEND='%{lwe_backend}' \
  -DLTO='%{using_lto}' -DENABLE_DEBUGGER='%{enable_debugger}' -DTARGETNAME=lightweight-web-engine.tv \
  -DSHELL='%{lwe_shell_type}' -DENABLE_DYNAMIC_LOADER='%{enable_dynamic_loader}' \
  -DTIZEN_RW_APP_DIR='%{TZ_SYS_RW_APP}' -DTIZEN_DATA_DIR='%{_datadir}' \
  -DASAN='%{asan}' %{features_config} %{?extra_cmake_options} \
  -G Ninja
ninja -C %{out_tizen} starfish.shared_library
ninja -C %{out_tizen} starfish.executable

%if "%{?enable_sharedworker}" == "1"
ninja -C %{out_tizen} starfish_api.sharedworker.shared_library
%endif
%if "%{?enable_serviceworker}" == "1"
ninja -C %{out_tizen} starfish_api.serviceworker.shared_library
%endif

%if "%{?build_uwe_tpk}" == "1"
ninja -C %{out_tizen} starfish.uwe.tpk
%endif

%if "%{?build_shell_tpk}" == "1"
ninja -C %{out_tizen} starfish.executable.tpk
%endif

%endif

%if "%{rpm}" == "prod_tv"
%define out_tizen %{out_folder}/prod_tv/release

# For Cairo
%if "%{?skip_config}" == "0"
%if 0%{?build_option:1}
cmake CMakeLists.txt -B%{out_tizen} -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
  -DTIZEN_MAJOR_VERSION='%{tizen_version_major}' -DTIZEN_MINOR_VERSION='%{tizen_version_minor}' \
  -DUSE_EMBEDDED_IMAGE_DECODER='%{use_embedded_image_decoder}' -DENABLE_WASM='%{enable_wasm}' \
  -DENABLE_CODECACHE='%{enable_codecache}' -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_SYSTEM_NAME=Tizen -DCMAKE_SYSTEM_PROCESSOR='%{tizen_arch}' \
  -DFP_MODE='%{fp_mode}' -DCUSTOM=prod_tv -DBACKEND='%{lwe_backend}' -DLTO='%{using_lto}' \
  -DENABLE_DEBUGGER='%{enable_debugger}' -DENABLE_TEST='%{enable_test}' -DTARGETNAME=lightweight-web-engine.prod.tv \
  -DSHELL='%{lwe_shell_type}' -DENABLE_DYNAMIC_LOADER='%{enable_dynamic_loader}'\
  -DTIZEN_RW_APP_DIR='%{TZ_SYS_RW_APP}' -DTIZEN_DATA_DIR='%{_datadir}' \
  -DASAN='%{asan}' %{features_config} %{?extra_cmake_options} \
  -G Ninja
%else # 0%{?build_option:1}
cmake CMakeLists.txt -B%{out_tizen} -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
  -DTIZEN_MAJOR_VERSION='%{tizen_version_major}' -DTIZEN_MINOR_VERSION='%{tizen_version_minor}' \
  -DUSE_EMBEDDED_IMAGE_DECODER='%{use_embedded_image_decoder}' -DENABLE_WASM='%{enable_wasm}' \
  -DENABLE_CODECACHE='%{enable_codecache}' -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_SYSTEM_NAME=Tizen -DCMAKE_SYSTEM_PROCESSOR='%{tizen_arch}' \
  -DFP_MODE='%{fp_mode}' -DCUSTOM=prod_tv -DBACKEND='%{lwe_backend}' -DLTO='%{using_lto}' \
  -DENABLE_DEBUGGER='%{enable_debugger}' -DENABLE_TEST='%{enable_test}' -DTARGETNAME=lightweight-web-engine.prod.tv \
  -DSHELL='%{lwe_shell_type}' -DENABLE_DYNAMIC_LOADER='%{enable_dynamic_loader}' \
  -DTIZEN_RW_APP_DIR='%{TZ_SYS_RW_APP}' -DTIZEN_DATA_DIR='%{_datadir}' \
  -DASAN='%{asan}' %{features_config} %{?extra_cmake_options} \
  -G Ninja
%endif
%endif

ninja -C %{out_tizen} starfish.shared_library
ninja -C %{out_tizen} starfish_api.shared_library
%if "%{?disable_shell}" == "0"
ninja -C %{out_tizen} starfish.executable
%endif

%if "%{?enable_sharedworker}" == "1"
ninja -C %{out_tizen} starfish_api.sharedworker.shared_library
%endif
%if "%{?enable_serviceworker}" == "1"
ninja -C %{out_tizen} starfish_api.serviceworker.shared_library
%endif

%if "%{?build_uwe_tpk}" == "1"
ninja -C %{out_tizen} starfish.uwe.tpk
%endif

%if "%{?build_shell_tpk}" == "1"
ninja -C %{out_tizen} starfish.executable.tpk
%endif

%if "%{?enable_test}" == "1"
ninja -C %{out_tizen} install_pixel_test_dep
%endif

%endif # "%{rpm}" == "prod_tv"

%if "%{rpm}" == "headless"
%define out_tizen %{out_folder}/headless/release

# For Cairo
#CFLAGS+=' -marm '
#CXXFLAGS+=' -marm '

cmake CMakeLists.txt -B%{out_tizen} -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
  -DTIZEN_MAJOR_VERSION='%{tizen_version_major}' -DTIZEN_MINOR_VERSION='%{tizen_version_minor}' \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_SYSTEM_NAME=Tizen -DCMAKE_SYSTEM_PROCESSOR='%{tizen_arch}' -DFP_MODE='%{fp_mode}' -DCUSTOM=headless \
  -DBACKEND='%{lwe_headless_backend}' -DLTO='%{using_lto}' -DENABLE_DEBUGGER='%{enable_debugger}' \
  -DSHELL='%{lwe_headless_shell_type}' -DTARGETNAME=lightweight-web-engine.headless \
  -DENABLE_DYNAMIC_LOADER='%{enable_dynamic_loader}' \
  -DTIZEN_RW_APP_DIR='%{TZ_SYS_RW_APP}' -DTIZEN_DATA_DIR='%{_datadir}' \
  -DASAN='%{asan}' %{features_config} %{?extra_cmake_options} \
  -G Ninja
ninja -C %{out_tizen} starfish.shared_library
ninja -C %{out_tizen} starfish_api.shared_library
ninja -C %{out_tizen} starfish.executable

%if "%{?enable_sharedworker}" == "1"
ninja -C %{out_tizen} starfish_api.sharedworker.shared_library
%endif
%if "%{?enable_serviceworker}" == "1"
ninja -C %{out_tizen} starfish_api.serviceworker.shared_library
%endif

%if "%{?build_uwe_tpk}" == "1"
ninja -C %{out_tizen} starfish.uwe.tpk
%endif

%if "%{?build_shell_tpk}" == "1"
ninja -C %{out_tizen} starfish.executable.tpk
%endif

%endif


%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
%define out_tizen %{out_folder}/unified_mobile/release

# For Cairo
cmake CMakeLists.txt -B%{out_tizen} -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
  -DTIZEN_MAJOR_VERSION='%{tizen_version_major}' -DTIZEN_MINOR_VERSION='%{tizen_version_minor}' \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_SYSTEM_NAME=Tizen -DCMAKE_SYSTEM_PROCESSOR='%{tizen_arch}' -DFP_MODE='%{fp_mode}' -DCUSTOM=unified_mobile \
  -DBACKEND='%{lwe_backend}' -DLTO='%{using_lto}' -DENABLE_DEBUGGER='%{enable_debugger}' \
  -DSHELL='%{lwe_shell_type}' -DTARGETNAME=lightweight-web-engine.mobile \
  -DTIZEN_RW_APP_DIR='%{TZ_SYS_RW_APP}' -DTIZEN_DATA_DIR='%{_datadir}' \
  -DENABLE_DYNAMIC_LOADER='%{enable_dynamic_loader}' -DENABLE_A11Y_TOUCH=1 \
  -DASAN='%{asan}' %{features_config} %{?extra_cmake_options} \
  -G Ninja
ninja -C %{out_tizen} starfish.shared_library
ninja -C %{out_tizen} starfish_api.shared_library
ninja -C %{out_tizen} starfish.executable

%if "%{?enable_sharedworker}" == "1"
ninja -C %{out_tizen} starfish_api.sharedworker.shared_library
%endif
%if "%{?enable_serviceworker}" == "1"
ninja -C %{out_tizen} starfish_api.serviceworker.shared_library
%endif

%if "%{?build_uwe_tpk}" == "1"
ninja -C %{out_tizen} starfish.uwe.tpk
%endif

%if "%{?build_shell_tpk}" == "1"
ninja -C %{out_tizen} starfish.executable.tpk
%endif

%endif


%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%define out_tizen %{out_folder}/unified_wearable/release

CFLAGS+=' -Os '
CXXFLAGS+=' -Os '

# For Cairo
cmake CMakeLists.txt -B%{out_tizen} -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
  -DTIZEN_MAJOR_VERSION='%{tizen_version_major}' -DTIZEN_MINOR_VERSION='%{tizen_version_minor}' \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_SYSTEM_NAME=Tizen -DCMAKE_SYSTEM_PROCESSOR='%{tizen_arch}' -DFP_MODE='%{fp_mode}' -DCUSTOM=unified_wearable \
  -DBACKEND='%{lwe_backend}' -DLTO='%{using_lto}' -DENABLE_DEBUGGER='%{enable_debugger}' \
  -DSHELL='%{lwe_shell_type}' -DTARGETNAME=lightweight-web-engine.wearable \
  -DENABLE_DYNAMIC_LOADER='%{enable_dynamic_loader}' \
  -DTIZEN_RW_APP_DIR='%{TZ_SYS_RW_APP}' -DTIZEN_DATA_DIR='%{_datadir}' \
  -DASAN='%{asan}' %{features_config} %{?extra_cmake_options} \
  -G Ninja
ninja -C %{out_tizen} starfish.shared_library
ninja -C %{out_tizen} starfish_api.shared_library
ninja -C %{out_tizen} starfish.executable

%if "%{?enable_sharedworker}" == "1"
ninja -C %{out_tizen} starfish_api.sharedworker.shared_library
%endif
%if "%{?enable_serviceworker}" == "1"
ninja -C %{out_tizen} starfish_api.serviceworker.shared_library
%endif

%if "%{?build_uwe_tpk}" == "1"
ninja -C %{out_tizen} starfish.uwe.tpk
%endif

%if "%{?build_shell_tpk}" == "1"
ninja -C %{out_tizen} starfish.executable.tpk
%endif

%endif

%if "%{rpm}" == "flutter"
%define out_tizen %{out_folder}/flutter/release
# For Cairo
cmake CMakeLists.txt -B%{out_tizen} -DTIZEN_MAJOR_VERSION='%{tizen_version_major}' \
  -DTIZEN_MINOR_VERSION='%{tizen_version_minor}' -DUSE_EMBEDDED_IMAGE_DECODER='%{use_embedded_image_decoder}' \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_SYSTEM_NAME=Tizen -DCMAKE_SYSTEM_PROCESSOR='%{tizen_arch}' -DFP_MODE='%{fp_mode}' -DCUSTOM=flutter \
  -DBACKEND=flutter -DLTO='%{using_lto}' -DENABLE_DEBUGGER='%{enable_debugger}' \
  -DTARGETNAME=lightweight-web-engine.flutter -DTIZEN_RW_APP_DIR='%{TZ_SYS_RW_APP}' -DTIZEN_DATA_DIR='%{_datadir}' \
  -DASAN='%{asan}' %{features_config} %{?extra_cmake_options} \
  -G Ninja
ninja -C %{out_tizen} starfish.shared_library
ninja -C %{out_tizen} starfish_api.shared_library
%endif

##############################################
## Install
##############################################

%install
%define bin Starfish

rm -rf %{buildroot}
mkdir -p %{buildroot}%{_libdir}/lwe
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_unitdir}

%if "%{rpm}" == "tv" || "%{rpm}" == "mobile" || "%{rpm}" == "wearable" || "%{rpm}" == "all" || "%{rpm}" == "prod_tv" || "%{rpm}" == "headless"
install -d %{buildroot}%{_datadir}/lwe/update
install -m 0644 %{out_tizen}/lightweight-web-engine-update.service %{buildroot}%{_unitdir}
%install_service multi-user.target.wants lightweight-web-engine-update.service
%endif

%if "%{rpm}" == "tv" || "%{rpm}" == "all"
mkdir -p %{buildroot}/%{_libdir}/lwe/tv
cp -fr %{out_folder}/unified_tv/release/lib/*.so* %{buildroot}%{_libdir}/lwe/tv
cp -fr %{out_folder}/unified_tv/release/lib/*.tv.so* %{buildroot}%{_libdir}/lwe/tv
cp -fr %{out_folder}/unified_tv/release/lib/VERSION %{buildroot}%{_libdir}/lwe/tv
%endif
%if "%{rpm}" == "tv" && "%{?disable_shell}" == "0"
cp -fr %{out_folder}/unified_tv/release/lightweight-web-engine*.tv %{buildroot}%{_bindir}
%endif

%if "%{rpm}" == "prod_tv"
mkdir -p %{buildroot}/%{_libdir}/lwe/tv
cp -fr %{out_folder}/prod_tv/release/lib/*.so* %{buildroot}%{_libdir}/lwe/tv
cp -fr %{out_folder}/prod_tv/release/lib/*.tv.so* %{buildroot}%{_libdir}/lwe/tv
cp -fr %{out_folder}/prod_tv/release/lib/VERSION %{buildroot}%{_libdir}/lwe/tv
strip -v --strip-all %{buildroot}%{_libdir}/lwe/tv/*.so*
strip -v --strip-all %{buildroot}%{_libdir}/lwe/tv/*.tv.so*
%endif
%if "%{rpm}" == "prod_tv" && "%{?disable_shell}" == "0"
cp -fr %{out_folder}/prod_tv/release/lightweight-web-engine*.tv %{buildroot}%{_bindir}
%if "%{?enable_test}" == "1"
cp -fr tool/imgdiff/imgdiff %{buildroot}%{_bindir}
%endif
%endif

%if "%{rpm}" == "headless"
mkdir -p %{buildroot}/%{_libdir}/lwe/headless
cp -fr %{out_folder}/headless/release/lib/*.so* %{buildroot}%{_libdir}/lwe/headless
cp -fr %{out_folder}/headless/release/lib/*.headless.so* %{buildroot}%{_libdir}/lwe/headless
cp -fr %{out_folder}/headless/release/lib/VERSION %{buildroot}%{_libdir}/lwe/headless
%endif
%if "%{rpm}" == "headless" && "%{?disable_shell}" == "0"
cp -fr %{out_folder}/headless/release/lightweight-web-engine.headless %{buildroot}%{_bindir}
%endif

%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
mkdir -p %{buildroot}/%{_libdir}/lwe/mobile
cp -fr %{out_folder}/unified_mobile/release/lib/*.so* %{buildroot}%{_libdir}/lwe/mobile
cp -fr %{out_folder}/unified_mobile/release/lib/*.mobile.so* %{buildroot}%{_libdir}/lwe/mobile
cp -fr %{out_folder}/unified_mobile/release/lib/VERSION %{buildroot}%{_libdir}/lwe/mobile
%endif
%if "%{rpm}" == "mobile" && "%{?disable_shell}" == "0"
cp -fr %{out_folder}/unified_mobile/release/lightweight-web-engine.mobile %{buildroot}%{_bindir}
%endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
mkdir -p %{buildroot}/%{_libdir}/lwe/wearable
cp -fr %{out_folder}/unified_wearable/release/lib/*.so* %{buildroot}%{_libdir}/lwe/wearable
cp -fr %{out_folder}/unified_wearable/release/lib/*.wearable.so* %{buildroot}%{_libdir}/lwe/wearable
cp -fr %{out_folder}/unified_wearable/release/lib/VERSION %{buildroot}%{_libdir}/lwe/wearable
%endif
%if "%{rpm}" == "wearable" && "%{?disable_shell}" == "0"
cp -fr %{out_folder}/unified_wearable/release/lightweight-web-engine.wearable %{buildroot}%{_bindir}
%endif

%if "%{rpm}" == "flutter"
mkdir -p %{buildroot}/%{_libdir}/lwe/flutter
cp -fr %{out_folder}/flutter/release/lib/*.so* %{buildroot}%{_libdir}/lwe/flutter
cp -fr %{out_folder}/flutter/release/lib/*.flutter.so* %{buildroot}%{_libdir}/lwe/flutter
cp -fr %{out_folder}/flutter/release/lib/VERSION %{buildroot}%{_libdir}/lwe/flutter
%endif

# for devel files
mkdir -p %{buildroot}%{_includedir}/%{name}
cp inc/*.h %{buildroot}%{_includedir}/%{name}/

mkdir -p %{buildroot}%{_libdir}/pkgconfig/
cp %{out_tizen}/lightweight-web-engine.pc %{buildroot}%{_libdir}/pkgconfig/
%if "%{?enable_sharedworker}" == "1"
cp %{out_tizen}/lightweight-web-engine-sharedworker.pc %{buildroot}%{_libdir}/pkgconfig/
%endif
%if "%{?enable_serviceworker}" == "1"
cp %{out_tizen}/lightweight-web-engine-serviceworker.pc %{buildroot}%{_libdir}/pkgconfig/
%endif
mkdir -p %{buildroot}%{_sysconfdir}/ld.so.conf.d/
cp lightweight-web-engine.conf %{buildroot}%{_sysconfdir}/ld.so.conf.d/

# symbolic links
pushd %{buildroot}%{_libdir}/lwe
rm -fr *.so*
ln -s liblightweight-web-engine.so.1 liblightweight-web-engine.so
%if "%{?enable_sharedworker}" == "1"
ln -s liblightweight-web-engine-sharedworker.so.1 liblightweight-web-engine-sharedworker.so
%endif
%if "%{?enable_serviceworker}" == "1"
ln -s liblightweight-web-engine-serviceworker.so.1 liblightweight-web-engine-serviceworker.so
%endif
popd

pushd %{buildroot}%{_libdir}
ln -s lwe/liblightweight-web-engine.so liblightweight-web-engine.so
%if "%{?enable_sharedworker}" == "1"
ln -s lwe/liblightweight-web-engine-sharedworker.so liblightweight-web-engine-sharedworker.so
%endif
%if "%{?enable_serviceworker}" == "1"
ln -s lwe/liblightweight-web-engine-serviceworker.so liblightweight-web-engine-serviceworker.so
%endif
popd

##############################################
## Scripts
##############################################

# Post Install
%post
/sbin/ldconfig

%if "%{rpm}" == "tv" || "%{rpm}" == "prod_tv" || "%{rpm}" == "all"
%post profile_tv
pushd %{_libdir}/lwe
for FILE in `ls tv/*.so* | grep -v 'tv.so'`; do
    ln -sf "$FILE" .
done
%if "%{rpm}" == "tv"
ln -sf tv/liblightweight-web-engine.tv.so liblightweight-web-engine.so.1
ln -sf tv/VERSION VERSION
%if "%{?enable_sharedworker}" == "1"
ln -sf tv/liblightweight-web-engine.tv-sharedworker.so liblightweight-web-engine-sharedworker.so.1
%endif
%if "%{?enable_serviceworker}" == "1"
ln -sf tv/liblightweight-web-engine.tv-serviceworker.so liblightweight-web-engine-serviceworker.so.1
%endif
%endif # "%{rpm}" == "tv"
%if "%{rpm}" == "prod_tv"
ln -sf tv/liblightweight-web-engine.prod.tv.so liblightweight-web-engine.so.1
ln -sf tv/VERSION VERSION
%endif # "%{rpm}" == "prod_tv"
popd
%endif
%if "%{rpm}" == "tv"
pushd %{_bindir}
ln -sf lightweight-web-engine.tv %{bin}
popd
exit 0
%endif # "%{rpm}" == "tv"
%if "%{rpm}" == "prod_tv"
pushd %{_bindir}
ln -sf lightweight-web-engine.prod.tv %{bin}
%if "%{?enable_test}" == "1"
ln -sf imgdiff %{bin}
%endif
popd
/sbin/ldconfig
exit 0
%endif # "%{rpm}" == "prod_tv"

#############################################
%if "%{rpm}" == "headless"
%post profile_headless
pushd %{_libdir}/lwe
for FILE in `ls headless/*.so* | grep -v 'headless.so'`; do
   ln -sf "$FILE" .
done
ln -sf headless/liblightweight-web-engine.headless.so liblightweight-web-engine.so.1
ln -sf headless/VERSION VERSION
%if "%{?enable_sharedworker}" == "1"
ln -sf headless/liblightweight-web-engine.headless-sharedworker.so liblightweight-web-engine-sharedworker.so.1
%endif
%if "%{?enable_serviceworker}" == "1"
ln -sf headless/liblightweight-web-engine.headless-serviceworker.so liblightweight-web-engine-serviceworker.so.1
%endif
popd
%endif
%if "%{rpm}" == "headless"
pushd %{_bindir}
ln -sf lightweight-web-engine.headless %{bin}
popd
/sbin/ldconfig
exit 0
%endif

#############################################
%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
%post profile_mobile
pushd %{_libdir}/lwe
for FILE in `ls mobile/*.so* | grep -v 'mobile.so'`; do
   ln -sf "$FILE" .
done
ln -sf mobile/liblightweight-web-engine.mobile.so liblightweight-web-engine.so.1
ln -sf mobile/VERSION VERSION
%if "%{?enable_sharedworker}" == "1"
ln -sf mobile/liblightweight-web-engine.mobile-sharedworker.so liblightweight-web-engine-sharedworker.so.1
%endif
%if "%{?enable_serviceworker}" == "1"
ln -sf mobile/liblightweight-web-engine.mobile-serviceworker.so liblightweight-web-engine-serviceworker.so.1
%endif
popd
%endif
%if "%{rpm}" == "mobile"
pushd %{_bindir}
ln -sf lightweight-web-engine.mobile %{bin}
popd
/sbin/ldconfig
exit 0
%endif

#############################################
%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%post profile_wearable
pushd %{_libdir}/lwe
for FILE in `ls wearable/*.so* | grep -v 'wearable.so'`; do
    ln -sf "$FILE" .
done
ln -sf wearable/liblightweight-web-engine.wearable.so liblightweight-web-engine.so.1
ln -sf wearable/VERSION VERSION
%if "%{?enable_sharedworker}" == "1"
ln -sf wearable/liblightweight-web-engine.wearable-sharedworker.so liblightweight-web-engine-sharedworker.so.1
%endif
%if "%{?enable_serviceworker}" == "1"
ln -sf wearable/liblightweight-web-engine.wearable-serviceworker.so liblightweight-web-engine-serviceworker.so.1
%endif
popd
%endif
%if "%{rpm}" == "wearable"
pushd %{_bindir}
ln -sf lightweight-web-engine.wearable %{bin}
popd
/sbin/ldconfig
exit 0
%endif

#############################################
%if "%{rpm}" == "flutter"
%post profile_flutter
pushd %{_libdir}/lwe
for FILE in `ls flutter/*.so* | grep -v 'flutter.so'`; do
   ln -sf "$FILE" .
done
ln -sf flutter/liblightweight-web-engine.flutter.so liblightweight-web-engine.so.1
ln -sf flutter/VERSION VERSION
popd
%endif
%if "%{rpm}" == "flutter"
/sbin/ldconfig
exit 0
%endif

# Post Uninstall
%postun
/sbin/ldconfig

##############################################
## Packaging rpms
##############################################

%files
%manifest %{name}.manifest

%if "%{rpm}" == "tv" || "%{rpm}" == "prod_tv" || "%{rpm}" == "all"
%files profile_tv
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/lwe/*.so*
%{_libdir}/lwe/tv/*.so*
%{_libdir}/lwe/tv/VERSION
%{_sysconfdir}/ld.so.conf.d/*.conf
%{_datadir}/lwe/update
%{_unitdir}/lightweight-web-engine-update.service
%{_unitdir}/multi-user.target.wants/lightweight-web-engine-update.service
%license LICENSE.LGPL-2.1+ LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.MIT LICENSE.ISC LICENSE.Zlib LICENSE.BOEHM-GC LICENSE.ICU
%endif

%if "%{rpm}" == "headless"
%files profile_headless
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/lwe/*.so*
%{_libdir}/lwe/headless/*.so*
%{_libdir}/lwe/headless/VERSION
%{_sysconfdir}/ld.so.conf.d/*.conf
%{_unitdir}/lightweight-web-engine-update.service
%{_unitdir}/multi-user.target.wants/lightweight-web-engine-update.service
%{_datadir}/lwe/update
%license LICENSE.LGPL-2.1+ LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.MIT LICENSE.ISC LICENSE.Zlib LICENSE.BOEHM-GC LICENSE.ICU
%endif

%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
%files profile_mobile
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/lwe/*.so*
%{_libdir}/lwe/mobile/*.so*
%{_libdir}/lwe/mobile/VERSION
%{_sysconfdir}/ld.so.conf.d/*.conf
%{_unitdir}/lightweight-web-engine-update.service
%{_unitdir}/multi-user.target.wants/lightweight-web-engine-update.service
%{_datadir}/lwe/update
%license LICENSE.LGPL-2.1+ LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.MIT LICENSE.ISC LICENSE.Zlib LICENSE.BOEHM-GC LICENSE.ICU
%endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%files profile_wearable
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/lwe/*.so*
%{_libdir}/lwe/wearable/*.so*
%{_libdir}/lwe/wearable/VERSION
%{_sysconfdir}/ld.so.conf.d/*.conf
%{_unitdir}/lightweight-web-engine-update.service
%{_unitdir}/multi-user.target.wants/lightweight-web-engine-update.service
%{_datadir}/lwe/update
%license LICENSE.LGPL-2.1+ LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.MIT LICENSE.ISC LICENSE.Zlib LICENSE.BOEHM-GC LICENSE.ICU
%endif

%if "%{rpm}" == "flutter"
%files profile_flutter
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/lwe/*.so*
%{_libdir}/lwe/flutter/*.so*
%{_libdir}/lwe/flutter/VERSION
%{_sysconfdir}/ld.so.conf.d/*.conf
%license LICENSE.LGPL-2.1+ LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.MIT LICENSE.ISC LICENSE.Zlib LICENSE.BOEHM-GC LICENSE.ICU
%endif

%files devel
%manifest %{name}.manifest
%{_includedir}
%{_libdir}/pkgconfig/*.pc

%if "%{?disable_shell}" == "0"
%if "%{rpm}" == "tv"
%files shell-profile_tv
%manifest %{name}.manifest
%{_bindir}/lightweight-web-engine.tv
%endif

%if "%{rpm}" == "prod_tv"
%files shell-profile_tv
%manifest %{name}.manifest
%{_bindir}/*
%endif

%if "%{rpm}" == "headless"
%files shell-profile_headless
%manifest %{name}.manifest
%{_bindir}/lightweight-web-engine.headless
%endif

%if "%{rpm}" == "mobile"
%files shell-profile_mobile
%manifest %{name}.manifest
%{_bindir}/lightweight-web-engine.mobile
%endif

%if "%{rpm}" == "wearable"
%files shell-profile_wearable
%manifest %{name}.manifest
%{_bindir}/lightweight-web-engine.wearable
%endif

%if "%{rpm}" == "flutter"
%files shell-profile_flutter
%manifest %{name}.manifest
%endif
%endif
