Name:          web-widget-engine
Version:       0.0.1
Release:       0
Summary:       Web Widget Engine
Source:        %{name}-%{version}.tar.gz
Group:         Development/Libraries
License:       Apache License, Version 2.0


%if "%{?tizen_profile_name}" == "mobile"
ExcludeArch: %{arm} %ix86 x86_64
%endif
%if "%{?tizen_profile_name}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

# build requirements
%if "%{gcc_version}" == "49"
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
%endif

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

%if "%{gcc_version}" == "49"
%ifarch %{arm}
make tizen_obs_arm.lib.release TIZEN_DEVICE_API=true %{?jobs:-j%jobs}
make tizen_obs_arm.exe.debug %{?jobs:-j%jobs}
%else
make tizen_obs_emulator.lib.release TIZEN_DEVICE_API=true %{?jobs:-j%jobs}
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
%if "%{gcc_version}" == "49"
cp out/tizen_obs/${STARFISH_ARCH}/lib/release/libWebWidgetEngine.so %{buildroot}%{_libdir}
cp out/tizen_obs/${STARFISH_ARCH}/exe/debug/StarFish %{buildroot}%{_bindir}
%else
cp prebuilt/tizen-wearable-2.3-${TIZEN_ARCH}/libWebWidgetEngine.so %{buildroot}%{_libdir}
touch %{buildroot}%{_bindir}/StarFish
%endif

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
