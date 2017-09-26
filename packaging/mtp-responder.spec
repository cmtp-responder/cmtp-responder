ExclusiveArch: %arm aarch64
%if "%{?tizen_profile_name}" == "tv"
ExcludeArch: %arm aarch64
%endif

Name:       mtp-responder
Summary:    Media Transfer Protocol daemon (responder)
Version:    0.0.25
Release:    1
Group:      Network & Connectivity/Other
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	%{name}.manifest
BuildRequires: cmake
BuildRequires: libgcrypt-devel
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(tapi)
BuildRequires: pkgconfig(capi-content-media-content)
BuildRequires: pkgconfig(capi-media-metadata-extractor)
BuildRequires: pkgconfig(capi-system-info)
Buildrequires: pkgconfig(storage)
BuildRequires: pkgconfig(libsystemd-daemon)
Requires(post): /usr/bin/vconftool

%description
This package includes a daemon which processes Media Transper Protocol(MTP) commands as MTP responder role.


%prep
%setup -q
cp %{SOURCE1001} .


%build
export CFLAGS="$CFLAGS -Wno-deprecated-declarations"

%cmake .

make %{?jobs:-j%jobs}


%install
%make_install

mkdir -p %{buildroot}/%{_prefix}/lib/udev/rules.d
cp packaging/99-mtp-responder.rules %{buildroot}/%{_prefix}/lib/udev/rules.d/99-mtp-responder.rules

install -D -m 0644 mtp-responder.service %{buildroot}/%{_unitdir}/mtp-responder.service

%post
mkdir -p %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
ln -sf %{_unitdir}/mtp-responder.service %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/

%files
%manifest mtp-responder.manifest
%license LICENSE.APLv2
%defattr(-,root,root,-)
%{_bindir}/mtp-responder
%{_unitdir}/mtp-responder.service
%{_prefix}/lib/udev/rules.d/99-mtp-responder.rules
/opt/var/lib/misc/mtp-responder.conf
