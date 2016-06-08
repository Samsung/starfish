Name:          web-widget-engine
Version:       0.0.1
Release:       0
Summary:       Web Widget Engine
Source:        %{name}-%{version}.tar.gz
Group:         Development/Libraries
License:       Apache License, Version 2.0

# build requirements
#BuildRequires: cmake
#BuildRequires: pkgconfig(bundle)
#BuildRequires: pkgconfig(dlog)

# install requirements


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
%if "%{tizen_profile_name}" == "wearable"
%ifarch %{arm}
#make tizen_wearable_arm %{?jobs:-j%jobs}
%else
#make tizen_wearable_emulator %{?jobs:-j%jobs}
%endif
%endif

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE %{buildroot}%{_datadir}/license/%{name}

mkdir -p %{buildroot}%{_libdir}/
%if "%{tizen_profile_name}" == "wearable"
%ifarch %{arm}
cp prebuilt/tizen-wearable-2.3-armv7l/libWebWidgetEngine.so %{buildroot}%{_libdir}
%else
cp prebuilt/tizen-wearable-2.3-i586/libWebWidgetEngine.so %{buildroot}%{_libdir}
%endif
%else
touch %{buildroot}%{_libdir}/libWebWidgetEngine.so # TBD
%endif

mkdir -p %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishPublic.h %{buildroot}%{_includedir}/%{name}/
cp inc/StarFishExport.h %{buildroot}%{_includedir}/%{name}/

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%{_datadir}/license/%{name}
%{_libdir}/*.so*

%files devel
%{_includedir}
