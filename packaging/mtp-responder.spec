ExclusiveArch: %arm aarch64
%if "%{?tizen_profile_name}" == "tv"
ExcludeArch: %arm aarch64
%endif

Name:       mtp-responder
Summary:    Media Transfer Protocol daemon (responder)
Version:    0.0.9
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
Requires(post): /usr/bin/vconftool


%description
This package includes a daemon which processes Media Transper Protocol(MTP) commands as MTP responder role.


%prep
%setup -q
cp %{SOURCE1001} .


%build
%cmake .

make %{?jobs:-j%jobs}


%install
%make_install

mkdir -p %{buildroot}%{_libdir}/udev/rules.d
cp packaging/99-mtp-responder.rules %{buildroot}%{_libdir}/udev/rules.d/99-mtp-responder.rules

install -D -m 0644 mtp-responder.service %{buildroot}%{_libdir}/systemd/system/mtp-responder.service

%post
/usr/bin/vconftool set -t string db/private/mtp/serial_number "" -u 5000 -g 5000 -i -f -s tizen::vconf::platform::rw
/usr/bin/vconftool set -t string db/private/mtp/sync_partner "" -u 5000 -g 5000 -i -f -s tizen::vconf::platform::rw
/usr/bin/vconftool set -t int db/private/mtp/sync_time 0 -u 5000 -g 5000 -i -f -s tizen::vconf::platform::rw

mkdir -p %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
ln -sf %{_libdir}/systemd/system/mtp-responder.service %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/

%files
%manifest mtp-responder.manifest
%defattr(-,system,system,-)
%{_bindir}/mtp-responder
%{_libdir}/systemd/system/mtp-responder.service
%{_libdir}/udev/rules.d/99-mtp-responder.rules
/opt/var/lib/misc/mtp-responder.conf
#%license LICENSE.APLv2
