Name:          web-widget-engine
Version:       0.0.1
Release:       0
Summary:       Web Widget Engine
Source:        %{name}-%{version}.tar.gz
Group:         Development/Libraries
License:       Apache-2.0 and LGPL-2.1+ and BSD-2.0 and ICU and BSL-1.0 and MIT and MPL-1.1


%if "%{?tizen_profile_name}" == "mobile"
ExcludeArch: %{arm} %ix86 x86_64
%endif
%if "%{?tizen_profile_name}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
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
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-media-player)

%description
Implementation of Web Widget Engine

%package devel
Summary:    web-widget-engine development headers
Group:      Development/Libraries
Requires:   %{name} = %{version}

%description devel
web-widget-engine development headers

%prep
%setup -q

%build

%ifarch %{arm}
make tizen_obs_arm.lib.release %{?tizen_version:TIZEN_VERSION=%tizen_version} %{?jobs:-j%jobs}
%else
make tizen_obs_emulator.lib.release %{?tizen_version:TIZEN_VERSION=%tizen_version} %{?jobs:-j%jobs}
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
%ifarch %{arm}
make tizen_obs_arm.exe.debug %{?jobs:-j%jobs}
%else
make tizen_obs_emulator.exe.debug %{?jobs:-j%jobs}
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
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE %{buildroot}%{_datadir}/license/%{name}

mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_bindir}
cp out/tizen_obs/${STARFISH_ARCH}/lib/release/libWebWidgetEngine.so %{buildroot}%{_libdir}
cp out/tizen_obs/${STARFISH_ARCH}/exe/debug/StarFish %{buildroot}%{_bindir}

mkdir -p %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishPublic.h %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishExport.h %{buildroot}%{_includedir}/%{name}/

%files
%manifest %{name}.manifest
%{_datadir}/license/%{name}
%{_libdir}/*.so

%files devel
%{_includedir}
%{_bindir}/StarFish
