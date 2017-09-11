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
%ifarch %{arm}
export MAKE_TARGET=tizen_obs_arm
%else
export MAKE_TARGET=tizen_obs_emulator
%endif

%if 0%{?only_devel}
%ifarch %{arm}
mkdir -p out/tizen_obs/arm/lib/release
touch    out/tizen_obs/arm/lib/release/liblightweight-web-engine.so
%else
mkdir -p out/tizen_obs/x86/lib/release
touch    out/tizen_obs/x86/lib/release/liblightweight-web-engine.so
%endif
%else
make ${MAKE_TARGET}.lib.release %{?tizen_version:TIZEN_VERSION=%tizen_version} %{?tizen_profile_name:TIZEN_PROFILE=%tizen_profile_name} %{?jobs:-j%jobs}
%endif

%if 0%{?only_release}
%ifarch %{arm}
mkdir -p out/tizen_obs/arm/exe/debug
touch    out/tizen_obs/arm/exe/debug/StarFish
%else
mkdir -p out/tizen_obs/x86/exe/debug
touch    out/tizen_obs/x86/exe/debug/StarFish
%endif
%else

%if "%{mode}" == "release"
make ${MAKE_TARGET}.exe.release %{?tizen_version:TIZEN_VERSION=%tizen_version} %{?tizen_profile_name:TIZEN_PROFILE=%tizen_profile_name} %{?jobs:-j%jobs}
%else
make ${MAKE_TARGET}.exe.release %{?tizen_version:TIZEN_VERSION=%tizen_version} %{?tizen_profile_name:TIZEN_PROFILE=%tizen_profile_name} %{?jobs:-j%jobs}
%endif
%endif

%install
%ifarch %{arm}
export STARFISH_ARCH=arm
export TIZEN_ARCH=armv7l
%else
export STARFISH_ARCH=x86
export TIZEN_ARCH=i586
%endif

rm -rf %{buildroot}

mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_bindir}
cp out/tizen_obs/${STARFISH_ARCH}/lib/release/liblightweight-web-engine.so %{buildroot}%{_libdir}

%if "%{mode}" == "release"
cp out/tizen_obs/${STARFISH_ARCH}/exe/release/StarFish %{buildroot}%{_bindir}
%else
cp out/tizen_obs/${STARFISH_ARCH}/exe/release/StarFish %{buildroot}%{_bindir}
%endif

mkdir -p %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishPublic.h %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishExport.h %{buildroot}%{_includedir}/%{name}/

%files
%manifest %{name}.manifest
%{_libdir}/*.so
%license LICENSE LICENSE.BSL-1.0 LICENSE.LGPL-2.1+ LICENSE.MPL-1.1 LICENSE.BSD-2.0 LICENSE.ICU LICENSE.MIT

%files devel
%{_includedir}
%{_bindir}/StarFish
