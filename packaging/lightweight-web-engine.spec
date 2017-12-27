Name:          lightweight-web-engine
Version:       0.0.1
Release:       0
Summary:       Lightweight Web Engine
Source:        %{name}-%{version}.tar.gz
Group:         Development/Libraries
License:       Apache-2.0 and LGPL-2.1+ and BSD-2.0 and ICU and BSL-1.0 and MIT and MPL-1.1

# RPM ref: http://backreference.org/2011/09/17/some-tips-on-rpm-conditional-macros/

# [ tv | headless | mobile | wearable ]
%if %{?tizen_profile_name:1}%{!?tizen_profile_name:0}
%define tizen_platform %{tizen_profile_name}
%else
%define tizen_platform tv
%endif

%if "%{tizen_platform}" == "mobile"
#ExcludeArch: %{arm} %ix86 x86_64
%endif
%if "%{tizen_platform}" == "tv"
#ExcludeArch: %{arm} %ix86 x86_64
%endif

# build requirements
BuildRequires: make
BuildRequires: ninja
BuildRequires: web-widget-js
BuildRequires: web-widget-js-devel
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(ecore-evas)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(cairo)
BuildRequires: pkgconfig(harfbuzz)
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(icu-uc)
BuildRequires: pkgconfig(libcurl)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(capi-location-manager)
BuildRequires: pkgconfig(dali-core)
BuildRequires: pkgconfig(dali-toolkit)
BuildRequires: pkgconfig(dali-adaptor)
BuildRequires: libjpeg-turbo-devel
BuildRequires: pkgconfig(openssl)
BuildRequires: giflib-devel
%if "%{tizen_platform}" == "tv"
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(vconf-internal-keys-tv)
BuildRequires: pkgconfig(vd-win-util)
%endif

%description
Implementation of Lightweight Web Engine

%package devel
Summary:    lightweight-web-engine development headers
Group:      Development/Libraries
Requires:   %{name} = %{version}

%description devel
lightweight-web-engine development headers

%prep
%setup -q

%build
%if "%{tizen_platform}" == "tv"
mkdir -p tizen_tv_build
cd tizen_tv_build
GYP_GENERATORS=ninja ../tool/gyp/gyp ../build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen_tv -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen_tv.release
GYP_GENERATORS=ninja ../tool/gyp/gyp ../build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen_tv -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen_tv.release
cd ..
%endif
%if "%{tizen_platform}" == "headless"
mkdir -p tizen_headless_build
cd tizen_headless_build
# '-mthumb' that's automatically appended to the compiler flag causes an
# unknown error in headless mode. To fix this (temporarily until correct flags
# are given by the system), '-marm' is appended to override the '-mthumb' flag.
# With '-marm', compiler emits some warnings.
CFLAGS+=' -marm '
CXXFLAGS+=' -marm '
GYP_GENERATORS=ninja ../tool/gyp/gyp ../build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen_headless -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen_headless.release
GYP_GENERATORS=ninja ../tool/gyp/gyp ../build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen_headless -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen_headless.release
cd ..
%endif
%if "%{tizen_platform}" == "mobile"
mkdir -p tizen_mobile_build
cd tizen_mobile_build
GYP_GENERATORS=ninja ../tool/gyp/gyp ../build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen.release
GYP_GENERATORS=ninja ../tool/gyp/gyp ../build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen.release
cd ..
%endif
%if "%{tizen_platform}" == "wearable"
CFLAGS+=' -Os '
CXXFLAGS+=' -Os '
mkdir -p tizen_wearable_build
cd tizen_wearable_build
GYP_GENERATORS=ninja ../tool/gyp/gyp ../build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen_wearable -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen_wearable.release
GYP_GENERATORS=ninja ../tool/gyp/gyp ../build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen_wearable -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen_wearable.release
cd ..
%endif

%install
%define bin StarFish
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_libdir}

%if "%{tizen_platform}" == "tv"
cp -r ./tizen_tv_build/out/release/lib/liblightweight-web-engine.tizen_tv.so %{buildroot}%{_libdir}/liblightweight-web-engine.so
mkdir -p %{buildroot}%{_bindir}
cp -r ./tizen_tv_build/out/release/lightweight-web-engine.tizen_tv %{buildroot}%{_bindir}/%{bin}
%endif
%if "%{tizen_platform}" == "headless"
cp -r ./tizen_headless_build/out/release/lib/liblightweight-web-engine.tizen_headless.so %{buildroot}%{_libdir}/liblightweight-web-engine.so
mkdir -p %{buildroot}%{_bindir}
cp -r ./tizen_headless_build/out/release/lightweight-web-engine.tizen_headless %{buildroot}%{_bindir}/%{bin}
%endif
%if "%{tizen_platform}" == "mobile"
cp -r ./tizen_mobile_build/out/release/lib/liblightweight-web-engine.tizen.so %{buildroot}%{_libdir}/liblightweight-web-engine.so
mkdir -p %{buildroot}%{_bindir}
cp -r ./tizen_mobile_build/out/release/lightweight-web-engine.tizen %{buildroot}%{_bindir}/%{bin}
%endif
%if "%{tizen_platform}" == "wearable"
cp -r ./tizen_wearable_build/out/release/lib/liblightweight-web-engine.tizen_wearable.so %{buildroot}%{_libdir}/libWebWidgetEngine.so
mkdir -p %{buildroot}%{_bindir}
cp -r ./tizen_wearable_build/out/release/lightweight-web-engine.tizen_wearable %{buildroot}%{_bindir}/%{bin}
%endif

mkdir -p %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishPublic.h %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishExport.h %{buildroot}%{_includedir}/%{name}/

mkdir -p %{buildroot}%{_libdir}/pkgconfig/
cp lightweight-web-engine.pc %{buildroot}%{_libdir}/pkgconfig/

%files
%manifest %{name}.manifest
%{_libdir}/*.so
%license LICENSE LICENSE.BSL-1.0 LICENSE.LGPL-2.1+ LICENSE.MPL-1.1 LICENSE.BSD-2.0 LICENSE.ICU LICENSE.MIT

%files devel
%{_includedir}
%{_bindir}/%{bin}
%{_libdir}/pkgconfig/lightweight-web-engine.pc

