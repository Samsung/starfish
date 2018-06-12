Name:          lightweight-web-engine
Version:       0.0.1
Release:       0
Summary:       Lightweight Web Engine
Source:        %{name}-%{version}.tar.gz
Group:         Development/Libraries
License:       LGPL-2.1+ and Apache-2.0 and BSD-2-Clause and BSD-3-Clause and BSL-1.0 and LGPL-3.0+ and MIT
ExclusiveArch: %arm

# RPM ref: http://backreference.org/2011/09/17/some-tips-on-rpm-conditional-macros/

# [ tv | headless | mobile | wearable ]
%if %{?tizen_profile_name:1}%{!?tizen_profile_name:0}
%define tizen_product %{tizen_profile_name}
%else
%define tizen_product tv
%endif

%if "%{?TIZEN_PRODUCT_TV}" == "1"
%define tizen_product tv
%else
%if "%{?TIZEN_PRODUCT_MOBILE}" == "1"
%define tizen_product gear
%else
%if "%{?TIZEN_PRODUCT_WEARABLE}" == "1"
%define tizen_product gear
%else
%if "%{?TIZEN_PRODUCT_HEADLESS}" == "1"
%define tizen_product speaker
%else
%define tizen_product unified
%endif
%endif
%endif
%endif

%if "%{tizen_profile_name}" == "tv"
%define tizen_product tv
%endif
%if "%{tizen_profile_name}" == "wearable"
%define tizen_product gear
%endif
%if "%{tizen_profile_name}" == "mobile"
%define tizen_product unified
%endif
%if "%{tizen_profile_name}" == "headless"
%define tizen_product speaker
%endif

%if "%{tizen_product}" == "gear"
#ExcludeArch: %{arm} %ix86 x86_64
%endif
%if "%{tizen_product}" == "tv"
#ExcludeArch: %{arm} %ix86 x86_64
%endif

# build requirements
BuildRequires: make
BuildRequires: cmake
BuildRequires: ninja
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
%if "%{tizen_product}" == "tv"
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(vconf-internal-keys-tv)
BuildRequires: pkgconfig(vd-win-util)
%endif
%if "%{tizen_product}" == "gear"
BuildRequires: pkgconfig(bundle)
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

./build_third_party.sh arm

%if "%{tizen_product}" == "tv"
%define target tv
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dcustom=vd %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release starfish.tizen.tv.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dcustom=vd %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release starfish.tizen.tv.release
%endif

%if "%{tizen_product}" == "gear"
%define target gear
CFLAGS+=' -Os '
CXXFLAGS+=' -Os '
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dcustom=im %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release starfish.tizen.gear.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dcustom=im %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release starfish.tizen.gear.release
%endif

%if "%{tizen_product}" == "speaker"
%define target speaker
# '-mthumb' that's automatically appended to the compiler flag causes an
# unknown error in headless mode. To fix this (temporarily until correct flags
# are given by the system), '-marm' is appended to override the '-mthumb' flag.
# With '-marm', compiler emits some warnings.
CFLAGS+=' -marm '
CXXFLAGS+=' -marm '
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dcustom=im -DtouchUi=0 %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release starfish.tizen.speaker.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dcustom=im -DtouchUi=0 %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release starfish.tizen.speaker.release
%endif

%if "%{tizen_product}" == "unified"
%define target unified
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release starfish.tizen.unified.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release starfish.tizen.unified.release
%endif

mv out_tizen/%{target}/release/lib/liblightweight-web-engine.%{target}.so out_tizen/%{target}/release/lib/liblightweight-web-engine.so
mv out_tizen/%{target}/release/lightweight-web-engine.%{target} out_tizen/%{target}/release/lightweight-web-engine


%install
%define bin StarFish
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_bindir}
cp -r out_tizen/%{target}/release/lib/*.so %{buildroot}%{_libdir}
cp -r out_tizen/%{target}/release/lightweight-web-engine %{buildroot}%{_bindir}/%{bin}

mkdir -p %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishPublic.h %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishExport.h %{buildroot}%{_includedir}/%{name}/
cp inc/LWEWebView.h %{buildroot}%{_includedir}/%{name}/
cp inc/PlatformIntegrationData.h %{buildroot}%{_includedir}/%{name}/

mkdir -p %{buildroot}%{_libdir}/pkgconfig/
cp lightweight-web-engine.pc %{buildroot}%{_libdir}/pkgconfig/

%files
%manifest %{name}.manifest
%{_libdir}/*.so
%license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT


%files devel
%{_includedir}
%{_bindir}/%{bin}
%{_libdir}/*.so
%{_libdir}/pkgconfig/lightweight-web-engine.pc

