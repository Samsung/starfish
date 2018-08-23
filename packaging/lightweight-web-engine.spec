#
# Copyright (c) 2018-present Samsung Electronics Co., Ltd
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
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
Summary:       Lightweight Web Engine
Version:       0.0.1
Release:       0
Group:         Development/Libraries
License:       LGPL-2.1+ and Apache-2.0 and BSD-2-Clause and BSD-3-Clause and BSL-1.0 and LGPL-3.0+ and MIT
Source:        %{name}-%{version}.tar.gz
ExclusiveArch: %arm


# RPM ref: http://backreference.org/2011/09/17/some-tips-on-rpm-conditional-macros/

# [ common | tv | headless | mobile | wearable ]
%if %{?tizen_profile_name:1}%{!?tizen_profile_name:0}
%define tizen_product %{tizen_profile_name}
%else
%define tizen_product tv
%endif

%if "%{?TIZEN_PRODUCT_TV}" == "1"
%define tizen_product tv
%else
%if "%{?TIZEN_PRODUCT_MOBILE}" == "1"
%define tizen_product common
%else
%if "%{?TIZEN_PRODUCT_WEARABLE}" == "1"
%define tizen_product wearable
%else
%if "%{?TIZEN_PRODUCT_HEADLESS}" == "1"
%define tizen_product headless
%else
%define tizen_product common
%endif
%endif
%endif
%endif

%if "%{tizen_profile_name}" == "tv"
%define tizen_product tv
%endif
%if "%{tizen_profile_name}" == "wearable"
%define tizen_product wearable
%endif
%if "%{tizen_profile_name}" == "mobile"
%define tizen_product common
%endif
%if "%{tizen_profile_name}" == "headless"
%define tizen_product headless
%endif

%if "%{tizen_product}" == "wearable"
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
%endif
%if "%{tizen_product}" == "wearable"
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
%if "%{tizen_product}" == "wearable"
CFLAGS+=' -Os '
CXXFLAGS+=' -Os '
./build_third_party.sh arm gear
%else
./build_third_party.sh arm
%endif


%if "%{tizen_product}" == "tv"
%define target tv
%if "%{tizen_version_major}" == "4"
CXXFLAGS+=' -DSTARFISH_TIZEN_4_0 '
%endif
%if "%{tizen_version_major}" == "5"
CXXFLAGS+=' -DSTARFISH_TIZEN_5_0 '
%endif

# For Cairo
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dprofile=tv %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_tv.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dprofile=tv %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_tv.release
mv out_tizen/%{target}/release/lib/liblightweight-web-engine.%{target}.so out_tizen/%{target}/release/lib/liblightweight-web-engine.so

# For Dali
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=tv %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_tv.release
%endif

%if "%{tizen_product}" == "wearable"
%define target wearable
# For Cairo
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dbackend=efl -Dplatform=tizen -Dprofile=wearable %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_wearable.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dbackend=efl -Dplatform=tizen -Dprofile=wearable %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_wearable.release
mv out_tizen/%{target}/release/lib/liblightweight-web-engine.%{target}.so out_tizen/%{target}/release/lib/liblightweight-web-engine.so

# For Dali
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=wearable %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_wearable.release
%endif

%if "%{tizen_product}" == "headless"
%define target headless
# '-mthumb' that's automatically appended to the compiler flag causes an
# unknown error in headless mode. To fix this (temporarily until correct flags
# are given by the system), '-marm' is appended to override the '-mthumb' flag.
# With '-marm', compiler emits some warnings.
CFLAGS+=' -marm '
CXXFLAGS+=' -marm '
# For Cairo
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dprofile=headless -DtouchUi=0 %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_headless.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dprofile=headless -DtouchUi=0 %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_headless.release
mv out_tizen/%{target}/release/lib/liblightweight-web-engine.%{target}.so out_tizen/%{target}/release/lib/liblightweight-web-engine.so

# For Dali
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=headless %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_headless.release
%endif

%if "%{tizen_product}" == "common"
%define target common
# For Cairo
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dprofile=common %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_common.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dprofile=common %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_common.release
mv out_tizen/%{target}/release/lib/liblightweight-web-engine.%{target}.so out_tizen/%{target}/release/lib/liblightweight-web-engine.so

# For Dali
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/%{target} --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=common %{?gyp_addition_command}
ninja -C out_tizen/%{target}/release lwe.tizen.unified_common.release
%endif

mv out_tizen/%{target}/release/lib/liblightweight-web-engine.%{target}.so out_tizen/%{target}/release/lib/liblightweight-web-engine-dali-plugin.so
mv out_tizen/%{target}/release/lightweight-web-engine.%{target} out_tizen/%{target}/release/lightweight-web-engine


%install
%define bin StarFish
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_bindir}
cp -r out_tizen/%{target}/release/lib/*.so %{buildroot}%{_libdir}
cp -r out_tizen/%{target}/release/lib/tizen/*.so %{buildroot}%{_libdir}
cp -r out_tizen/%{target}/release/lightweight-web-engine %{buildroot}%{_bindir}/%{bin}

mkdir -p %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishPublic.h %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishExport.h %{buildroot}%{_includedir}/%{name}/
cp inc/LWEWebView.h %{buildroot}%{_includedir}/%{name}/
cp inc/PlatformIntegrationData.h %{buildroot}%{_includedir}/%{name}/

mkdir -p %{buildroot}%{_libdir}/pkgconfig/
cp *.pc %{buildroot}%{_libdir}/pkgconfig/

%files
%manifest %{name}.manifest
%{_libdir}/*.so
%license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT


%files devel
%{_includedir}
%{_bindir}/%{bin}
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc

