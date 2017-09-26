Name:          lightweight-web-engine
Version:       0.0.1
Release:       0
Summary:       Lightweight Web Engine
Source:        %{name}-%{version}.tar.gz
Group:         Development/Libraries
License:       Apache-2.0 and LGPL-2.1+ and BSD-2.0 and ICU and BSL-1.0 and MIT and MPL-1.1

%if "%{?tizen_profile_name}" == "mobile"
#ExcludeArch: %{arm} %ix86 x86_64
%endif
%if "%{?tizen_profile_name}" == "tv"
#ExcludeArch: %{arm} %ix86 x86_64
%endif

%if %{?profile:1}%{!?profile:0}
%define tizen_profile_name {%profile}
%endif

%if %{?sec_product_feature_profile_wearable:1}%{!?sec_product_feature_profile_wearable:0}
%define tizen_profile_name wearable
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
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(icu-uc)
BuildRequires: pkgconfig(libcurl)
BuildRequires: pkgconfig(libxml-2.0)
%if "%{?tizen_profile_name}" != "wearable"
BuildRequires: pkgconfig(libavcodec)
BuildRequires: pkgconfig(libavutil)
BuildRequires: pkgconfig(libavformat)
%endif
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-media-player)
%if "%{?tizen_profile_name}" != "tv"
BuildRequires: pkgconfig(capi-location-manager)
%endif
BuildRequires: pkgconfig(dali-core)
BuildRequires: pkgconfig(dali-toolkit)
BuildRequires: pkgconfig(dali-adaptor)
BuildRequires: libjpeg-turbo-devel
BuildRequires: pkgconfig(openssl)
BuildRequires: giflib-devel
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(vconf-internal-keys-tv)

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
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=executable -Dplatform=tizen -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen.release
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp --no-parallel --toplevel-dir="." --depth=0 -Dcomponent=shared_library -Dplatform=tizen -Ddeplib=static_library %{?gyp_addition_command}
ninja -C out/release starfish.tizen.release

%install
%define bin StarFish

rm -rf %{buildroot}
mkdir -p %{buildroot}%{_libdir}
cp -r out/release/lib/libStarFish.tizen.release.so %{buildroot}%{_libdir}/liblightweight-web-engine.so
mkdir -p %{buildroot}%{_bindir}
cp -r out/release/StarFish.tizen.release %{buildroot}%{_bindir}/%{bin}

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

