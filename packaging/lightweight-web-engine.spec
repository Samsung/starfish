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

# [ tv | mobile | wearable ]
# The following syntax's been outdated.
# %if %{?tizen_profile_name:1}%{!?tizen_profile_name:0}
# %define profile %{tizen_profile_name}
# %else
# %define profile undefined
# %endif

# [ tv | mobile | wearable | all ]
# build for all profile
%if 0%{?build_profile:1}
%define rpm %{build_profile}
%else
%define rpm all
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
# We do not have these packages in public Tizen
# %if "%{rpm}" == "tv"
# %BuildRequires: pkgconfig(vconf)
# %BuildRequires: pkgconfig(vconf-internal-keys-tv)
# %endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
BuildRequires: pkgconfig(bundle)
%endif

%description
Implementation of Lightweight Web Engine


##############################################
# Packages for profiles
##############################################
%if "%{rpm}" == "tv" || "%{rpm}" == "all"
%package profile_tv
Summary: lightweight-web-engine for tv
%description profile_tv
lightweight-web-engine for tv
%endif

# %if "%{rpm}" == "mobile" || "%{rpm}" == "all"
# %package profile_mobile
# Summary: lightweight-web-engine for mobile
# %description profile_mobile
# lightweight-web-engine for mobile
# %endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%package profile_wearable
Summary: lightweight-web-engine for wearable
%description profile_wearable
lightweight-web-engine for wearable
%endif


%if "%{rpm}" == "tv" || "%{rpm}" == "all"
%package devel-profile_tv
Summary: Devel files for lightweight-web-engine for tv
%description devel-profile_tv
Devel files for lightweight-web-engine for tv
%endif

# %if "%{rpm}" == "mobile" || "%{rpm}" == "all"
# %package devel-profile_mobile
# Summary: Devel files for lightweight-web-engine for mobile
# %description devel-profile_mobile
# Devel files for lightweight-web-engine for mobile
# %endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%package devel-profile_wearable
Summary: Devel files for lightweight-web-engine for wearable
%description devel-profile_wearable
Devel files for lightweight-web-engine for wearable
%endif


##############################################
# Devel
##############################################
%package devel
Summary:    lightweight-web-engine development headers
Group:      Development/Libraries
Requires:   %{name} = %{version}
%description devel
lightweight-web-engine development headers

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

%if "%{tizen_version_major}" == "4"
CXXFLAGS+=' -DSTARFISH_TIZEN_4_0 '
%endif
%if "%{tizen_version_major}" == "5"
CXXFLAGS+=' -DSTARFISH_TIZEN_5_0 '
%endif


##############################################
## Build rules for each profile
##############################################

%if "%{rpm}" == "tv" || "%{rpm}" == "mobile" || "%{rpm}" == "all"
./build_third_party.sh arm
%endif

%if "%{rpm}" == "tv" || "%{rpm}" == "all"
# For Dali
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/tv --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=tv %{?gyp_addition_command}
ninja -C out_tizen/tv/release lwe.tizen.unified_tv.release
mv out_tizen/tv/release/lib/liblightweight-web-engine.tv.so out_tizen/tv/release/lib/liblightweight-web-engine-dali-plugin.tv.so

# For Cairo
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/tv --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dprofile=tv %{?gyp_addition_command}
ninja -C out_tizen/tv/release lwe.tizen.unified_tv.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/tv --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dprofile=tv %{?gyp_addition_command}
ninja -C out_tizen/tv/release lwe.tizen.unified_tv.release
%endif


# %if "%{rpm}" == "mobile" || "%{rpm}" == "all"
# # For Dali
# GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/mobile --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=mobile %{?gyp_addition_command}
# ninja -C out_tizen/mobile/release lwe.tizen.unified_mobile.release
# mv out_tizen/mobile/release/lib/liblightweight-web-engine.mobile.so out_tizen/mobile/release/lib/liblightweight-web-engine-dali-plugin.mobile.so

# For Cairo
# GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/mobile --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dprofile=mobile %{?gyp_addition_command}
# ninja -C out_tizen/mobile/release lwe.tizen.unified_mobile.release
# GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/mobile --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dprofile=mobile %{?gyp_addition_command}
# ninja -C out_tizen/mobile/release lwe.tizen.unified_mobile.release
# %endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
CFLAGS+=' -Os '
CXXFLAGS+=' -Os '
./build_third_party.sh arm gear

# For Dali
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/wearable --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=wearable %{?gyp_addition_command}
ninja -C out_tizen/wearable/release lwe.tizen.unified_wearable.release
mv out_tizen/wearable/release/lib/liblightweight-web-engine.wearable.so out_tizen/wearable/release/lib/liblightweight-web-engine-dali-plugin.wearable.so

# For Cairo
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/wearable --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dprofile=wearable %{?gyp_addition_command}
ninja -C out_tizen/wearable/release lwe.tizen.unified_wearable.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/wearable --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dprofile=wearable %{?gyp_addition_command}
ninja -C out_tizen/wearable/release lwe.tizen.unified_wearable.release
%endif


##############################################
## Install
##############################################

%install
%define bin StarFish

rm -rf %{buildroot}
mkdir -p %{buildroot}%{_libdir}/lwe
mkdir -p %{buildroot}%{_bindir}

%if "%{rpm}" == "tv" || "%{rpm}" == "all"
mkdir -p %{buildroot}/%{_libdir}/lwe/tv
cp -fr out_tizen/tv/release/lib/*.so %{buildroot}%{_libdir}/lwe/tv
cp -fr out_tizen/tv/release/lib/tizen/*.so %{buildroot}%{_libdir}/lwe/tv
cp -fr out_tizen/tv/release/lightweight-web-engine.tv %{buildroot}%{_bindir}
%endif

# %if "%{rpm}" == "mobile" || "%{rpm}" == "all"
# mkdir -p %{buildroot}/%{_libdir}/lwe/mobile
# cp -fr out_tizen/mobile/release/lib/*.so %{buildroot}%{_libdir}/lwe/mobile
# cp -fr out_tizen/mobile/release/lib/tizen/*.so %{buildroot}%{_libdir}/lwe/mobile
# cp -fr out_tizen/mobile/release/lightweight-web-engine.mobile %{buildroot}%{_bindir}
# %endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
mkdir -p %{buildroot}/%{_libdir}/lwe/wearable
cp -fr out_tizen/wearable/release/lib/*.so %{buildroot}%{_libdir}/lwe/wearable
cp -fr out_tizen/wearable/release/lib/tizen/*.so %{buildroot}%{_libdir}/lwe/wearable
cp -fr out_tizen/wearable/release/lightweight-web-engine.wearable %{buildroot}%{_bindir}
%endif

# for devel files
mkdir -p %{buildroot}%{_includedir}/%{name}/
cp inc/LWEWebView.h %{buildroot}%{_includedir}/%{name}/
cp inc/PlatformIntegrationData.h %{buildroot}%{_includedir}/%{name}/

mkdir -p %{buildroot}%{_libdir}/pkgconfig/
cp *.pc %{buildroot}%{_libdir}/pkgconfig/


##############################################
## Scripts
##############################################

# Post Install
%post
/sbin/ldconfig
exit 0

# Post Uninstall
%postun
/sbin/ldconfig
exit 0


#############################################
%if "%{rpm}" == "tv" || "%{rpm}" == "all"
%post profile_tv
pushd %{_libdir}
for FILE in `ls lwe/tv/*.so | grep -v 'tv.so'`; do
    ln -sf "$FILE" .
done
ln -sf lwe/tv/liblightweight-web-engine.tv.so liblightweight-web-engine.so
ln -sf lwe/tv/liblightwegith-web-engine-dali-plugin.tv.so liblightwegith-web-engine-dali-plugin.so
popd

pushd %{_bindir}
ln -sf lightweight-web-engine.tv %{bin}
popd
exit 0
%endif

#############################################
# %if "%{rpm}" == "mobile" || "%{rpm}" == "all"
# %post profile_mobile
# pushd %{_libdir}
# for FILE in `ls lwe/mobile/*.so | grep -v 'mobile.so'`; do
#     ln -sf "$FILE" .
# done
# ln -sf lwe/tv/liblightweight-web-engine.mobile.so liblightweight-web-engine.so
# ln -sf lwe/tv/liblightwegith-web-engine-dali-plugin.mobile.so liblightwegith-web-engine-dali-plugin.so
# popd

# pushd %{_bindir}
# ln -sf lightweight-web-engine.mobile %{bin}
# popd
# exit 0
# %endif

#############################################
%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%post profile_wearable
pushd %{_libdir}
for FILE in `ls lwe/wearable/*.so | grep -v 'wearable.so'`; do
    ln -sf "$FILE" .
done
ln -sf lwe/tv/liblightweight-web-engine.wearable.so liblightweight-web-engine.so
ln -sf lwe/tv/liblightwegith-web-engine-dali-plugin.wearable.so liblightwegith-web-engine-dali-plugin.so
popd

pushd %{_bindir}
ln -sf lightweight-web-engine.wearable %{bin}
popd
exit 0
%endif


##############################################
## Packaging rpms
##############################################

%files
%manifest %{name}.manifest
%license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT

%if "%{rpm}" == "tv" || "%{rpm}" == "all"
%files profile_tv
%manifest %{name}.manifest
%{_libdir}/lwe/tv/*.so
%license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT
%endif

# %if "%{rpm}" == "mobile" || "%{rpm}" == "all"
# %files profile_mobile
# %manifest %{name}.manifest
# %{_libdir}/lwe/mobile/*.so
# %license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT
# %endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%files profile_wearable
%manifest %{name}.manifest
%{_libdir}/lwe/wearable/*.so
%license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT
%endif


%if "%{rpm}" == "tv" || "%{rpm}" == "all"
%files devel-profile_tv
%{_includedir}
%{_bindir}/lightweight-web-engine.tv
%{_libdir}/pkgconfig/*.pc
%endif

# %if "%{rpm}" == "mobile" || "%{rpm}" == "all"
# %files devel-profile_mobile
# %{_includedir}
# %{_bindir}/lightweight-web-engine.mobile
# %{_libdir}/pkgconfig/*.pc
# %endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%files devel-profile_wearable
%{_includedir}
%{_bindir}/lightweight-web-engine.wearable
%{_libdir}/pkgconfig/*.pc
%endif

%files devel
%{_includedir}
%{_libdir}/pkgconfig/*.pc

