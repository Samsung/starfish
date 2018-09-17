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
Summary:       Lightweight Web Engine for Tizen
Version:       0.8.0
Release:       1
Group:         Development/Libraries
License:       LGPL-2.1+ and Apache-2.0 and BSD-2-Clause and BSD-3-Clause and BSL-1.0 and LGPL-3.0+ and MIT
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
BuildRequires: pkgconfig(bundle)

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
%if "%{rpm}" == "tv" || "%{rpm}" == "all"
%package profile_tv
Summary:     Lightweight Web Engine for tv
Provides:    %{name}-compat = %{version}-%{release}
Conflicts:   %{name}-profile_mobile = %{version}-%{release}
Conflicts:   %{name}-profile_wearable = %{version}-%{release}
%description profile_tv
Lightweight Web Engine for tv
%endif

%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
%package profile_mobile
Summary:     Lightweight Web Engine for mobile
Provides:    %{name}-compat = %{version}-%{release}
Conflicts:   %{name}-profile_tv = %{version}-%{release}
Conflicts:   %{name}-profile_wearable = %{version}-%{release}
%description profile_mobile
Lightweight Web Engine for mobile
%endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%package profile_wearable
Summary:     Lightweight Web Engine for wearable
Provides:    %{name}-compat = %{version}-%{release}
Conflicts:   %{name}-profile_tv = %{version}-%{release}
Conflicts:   %{name}-profile_mobile = %{version}-%{release}
%description profile_wearable
Lightweight Web Engine for wearable
%endif

%package devel
Summary:     Development files for Lightweight Web Engine
Group:       Development/Libraries
Requires:    %{name} = %{version}
%description devel
Development files for Lightweight Web Engine. This package provides
headers and package configs.

%if "%{rpm}" == "tv"
%package shell-profile_tv
Summary:     Development files for Lightweight Web Engine for tv
Requires:    %{name}-profile_tv
Conflicts:   %{name}-shell-profile_mobile = %{version}-%{release}
Conflicts:   %{name}-shell-profile_wearable = %{version}-%{release}
%description shell-profile_tv
Development files for Lightweight Web Engine for tv. This package provides
an standalone executable binary for tv.
%endif

%if "%{rpm}" == "mobile"
%package shell-profile_mobile
Summary:     Development files for Lightweight Web Engine for mobile
Requires:    %{name}-profile_mobile
Conflicts:   %{name}-shell-profile_tv = %{version}-%{release}
Conflicts:   %{name}-shell-profile_wearable = %{version}-%{release}
%description shell-profile_mobile
Development files for Lightweight Web Engine for tv. This package provides
an standalone executable binary for mobile.
%endif

%if "%{rpm}" == "wearable"
%package shell-profile_wearable
Summary:     Development files for Lightweight Web Engine for wearable
Requires:    %{name}-profile_wearable
Conflicts:   %{name}-shell-profile_tv = %{version}-%{release}
Conflicts:   %{name}-shell-profile_mobile = %{version}-%{release}
%description shell-profile_wearable
Development files for Lightweight Web Engine for tv. This package provides
an standalone executable binary for wearable.
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
%ifarch armv7l
./build_third_party.sh arm
%endif
%ifarch aarch64
./build_third_party.sh aarch64
%endif
%ifarch i686
./build_third_party.sh i686
%endif
%ifarch x86_64
./build_third_party.sh x86_64
%endif

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


%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
# For Dali
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/mobile --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=mobile %{?gyp_addition_command}
ninja -C out_tizen/mobile/release lwe.tizen.unified_mobile.release
mv out_tizen/mobile/release/lib/liblightweight-web-engine.mobile.so out_tizen/mobile/release/lib/liblightweight-web-engine-dali-plugin.mobile.so

# For Cairo
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/mobile --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dprofile=mobile %{?gyp_addition_command}
ninja -C out_tizen/mobile/release lwe.tizen.unified_mobile.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/mobile --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dprofile=mobile %{?gyp_addition_command}
ninja -C out_tizen/mobile/release lwe.tizen.unified_mobile.release
%endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
CFLAGS+=' -Os '
CXXFLAGS+=' -Os '

%ifarch armv7l
./build_third_party.sh arm gear
%endif
%ifarch aarch64
./build_third_party.sh aarch64 gear
%endif
%ifarch i686
./build_third_party.sh i686 gear
%endif
%ifarch x86_64
./build_third_party.sh x86_64 gear
%endif

# For Dali
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/wearable --no-parallel --toplevel-dir="." --depth=1 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=dali -Dprofile=wearable %{?gyp_addition_command}
ninja -C out_tizen/wearable/release lwe.tizen.unified_wearable.release
mv out_tizen/wearable/release/lib/liblightweight-web-engine.wearable.so out_tizen/wearable/release/lib/liblightweight-web-engine-dali-plugin.wearable.so

# For Cairo
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/wearable --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Dbackend=efl_cairo -Dprofile=wearable %{?gyp_addition_command}
ninja -C out_tizen/wearable/release lwe.tizen.unified_wearable.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp -Goutput_dir=out_tizen/wearable --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Dbackend=efl_cairo -Dprofile=wearable %{?gyp_addition_command}
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
cp -fr out_tizen/tv/release/lib/*.tv.so* %{buildroot}%{_libdir}/lwe/tv
cp -fr out_tizen/tv/release/lib/tizen/*.so %{buildroot}%{_libdir}/lwe/tv
%endif
%if "%{rpm}" == "tv"
cp -fr out_tizen/tv/release/lightweight-web-engine.tv %{buildroot}%{_bindir}
%endif

%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
mkdir -p %{buildroot}/%{_libdir}/lwe/mobile
cp -fr out_tizen/mobile/release/lib/*.so %{buildroot}%{_libdir}/lwe/mobile
cp -fr out_tizen/mobile/release/lib/*.mobile.so* %{buildroot}%{_libdir}/lwe/mobile
cp -fr out_tizen/mobile/release/lib/tizen/*.so %{buildroot}%{_libdir}/lwe/mobile
%endif
%if "%{rpm}" == "mobile"
cp -fr out_tizen/mobile/release/lightweight-web-engine.mobile %{buildroot}%{_bindir}
%endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
mkdir -p %{buildroot}/%{_libdir}/lwe/wearable
cp -fr out_tizen/wearable/release/lib/*.so %{buildroot}%{_libdir}/lwe/wearable
cp -fr out_tizen/wearable/release/lib/*.wearable.so* %{buildroot}%{_libdir}/lwe/wearable
cp -fr out_tizen/wearable/release/lib/tizen/*.so %{buildroot}%{_libdir}/lwe/wearable
%endif
%if "%{rpm}" == "wearable"
cp -fr out_tizen/wearable/release/lightweight-web-engine.wearable %{buildroot}%{_bindir}
%endif

# for devel files
mkdir -p %{buildroot}%{_includedir}/%{name}
cp inc/LWEWebView.h %{buildroot}%{_includedir}/%{name}/
cp inc/PlatformIntegrationData.h %{buildroot}%{_includedir}/%{name}/

mkdir -p %{buildroot}%{_libdir}/pkgconfig/
cp *.pc %{buildroot}%{_libdir}/pkgconfig/
mkdir -p %{buildroot}%{_sysconfdir}/ld.so.conf.d/
cp *.conf %{buildroot}%{_sysconfdir}/ld.so.conf.d/

# symbolic links
pushd %{buildroot}%{_libdir}/lwe
rm -fr *.so*
ln -s liblightweight-web-engine.so.1 liblightweight-web-engine.so
ln -s liblightweight-web-engine-dali-plugin.so.1 liblightweight-web-engine-dali-plugin.so
popd

pushd %{buildroot}%{_libdir}
ln -s lwe/liblightweight-web-engine.so liblightweight-web-engine.so
ln -s lwe/liblightweight-web-engine-dali-plugin.so liblightweight-web-engine-dali-plugin.so
popd

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
pushd %{_libdir}/lwe
for FILE in `ls tv/*.so | grep -v 'tv.so'`; do
    ln -sf "$FILE" .
done
ln -sf tv/liblightweight-web-engine.tv.so liblightweight-web-engine.so.1
ln -sf tv/liblightweight-web-engine-dali-plugin.tv.so liblightweight-web-engine-dali-plugin.so.1
popd
%endif
%if "%{rpm}" == "tv"
pushd %{_bindir}
ln -sf lightweight-web-engine.tv %{bin}
popd
exit 0
%endif

#############################################
%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
%post profile_mobile
pushd %{_libdir}/lwe
for FILE in `ls mobile/*.so | grep -v 'mobile.so'`; do
   ln -sf "$FILE" .
done
ln -sf mobile/liblightweight-web-engine.mobile.so liblightweight-web-engine.so.1
ln -sf mobile/liblightweight-web-engine-dali-plugin.mobile.so liblightweight-web-engine-dali-plugin.so.1
popd
%endif
%if "%{rpm}" == "mobile"
pushd %{_bindir}
ln -sf lightweight-web-engine.mobile %{bin}
popd
exit 0
%endif

#############################################
%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%post profile_wearable
pushd %{_libdir}/lwe
for FILE in `ls wearable/*.so | grep -v 'wearable.so'`; do
    ln -sf "$FILE" .
done
ln -sf wearable/liblightweight-web-engine.wearable.so liblightweight-web-engine.so.1
ln -sf wearable/liblightweight-web-engine-dali-plugin.wearable.so liblightweight-web-engine-dali-plugin.so.1
popd
%endif
%if "%{rpm}" == "wearable"
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

%if "%{rpm}" == "tv" || "%{rpm}" == "all"
%files profile_tv
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/lwe/*.so*
%{_libdir}/lwe/tv/*.so*
%{_sysconfdir}/ld.so.conf.d/*.conf
%license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT
%endif

%if "%{rpm}" == "mobile" || "%{rpm}" == "all"
%files profile_mobile
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/lwe/*.so*
%{_libdir}/lwe/mobile/*.so*
%{_sysconfdir}/ld.so.conf.d/*.conf
%license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT
%endif

%if "%{rpm}" == "wearable" || "%{rpm}" == "all"
%files profile_wearable
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/lwe/*.so*
%{_libdir}/lwe/wearable/*.so*
%{_sysconfdir}/ld.so.conf.d/*.conf
%license LICENSE.LGPL-2.1+ LICENSE.Apache-2.0 LICENSE.BSD-3-Clause LICENSE.BSL-1.0 LICENSE.LGPL-3.0+ LICENSE.MIT
%endif

%files devel
%manifest %{name}.manifest
%{_includedir}
%{_libdir}/pkgconfig/*.pc

%if "%{rpm}" == "tv"
%files shell-profile_tv
%manifest %{name}.manifest
%{_bindir}/lightweight-web-engine.tv
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
