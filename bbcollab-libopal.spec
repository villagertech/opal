%global version_major  3
%global version_minor  19
%global version_patch  5
%global version_oem    11

%global ffmpeg_ver_el7 2.6.3-10.4.el7
%global ptlib_ver_el7 2.19.4.10-2.3.el7
%global srtp_ver_el7 2.1.0-4.3.el7

# Branch ID should be 0 for local builds/PRs
# Jenkins builds should use 1 for develop, 2 for master (release builds)
%{!?branch_id: %global branch_id 0}
%{!?opal_stage: %global opal_stage -alpha}

# Disable the separate debug package and automatic stripping, as detailed here:
# http://fedoraproject.org/wiki/How_to_create_an_RPM_package
%global _enable_debug_package 0
%global debug_package %{nil}
%global __os_install_post /usr/lib/rpm/brp-compress %{nil}

# Fudge the auto-requires script to avoid a dependency on OpenH264
# See https://fedoraproject.org/wiki/PackagingDrafts/FilteringAutomaticDependencies
%define _use_internal_dependency_generator 0
%define __find_requires %{SOURCE1}

Name:           bbcollab-libopal
Version:        %{version_major}.%{version_minor}.%{version_patch}.%{version_oem}
Release:        %{branch_id}%{?jenkins_release}%{?dist}
Summary:        OpalVOIP library

Group:          System Environment/Libraries
License:        MPL 1.0
URL:            http://www.opalvoip.org/
Source0:        zsdk-opal.src.tgz
Source1:        bbcollab-filter-requires.sh

BuildRequires:  devtoolset-9-gcc-c++
BuildRequires:  bbcollab-ptlib-devel = %{ptlib_ver_el7}
BuildRequires:  bbcollab-ffmpeg-devel = %{ffmpeg_ver_el7}
BuildRequires:  libsrtp2-devel = %{srtp_ver_el7}

BuildRequires:  opus-devel
BuildRequires:  speex-devel
BuildRequires:  libtheora-devel
BuildRequires:  libvpx-devel
BuildRequires:  which
BuildRequires:  x264-devel

%description
OpalVOIP library

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       bbcollab-ptlib-devel = %{ptlib_ver_el7}
Requires:       libsrtp2-devel = %{srtp_ver_el7}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%package        static
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       %{name}-devel = %{version}-%{release}

%description    static
The %{name}-static package contains static libraries for
developing applications that use %{name}.


%prep
%setup -q -c -n zsdk-opal


%build
source /opt/rh/devtoolset-9/enable
%configure --enable-localspeexdsp \
        --disable-h323 \
        --disable-iax2 \
        --disable-skinny \
        --disable-lid \
        --disable-t38 \
        --disable-sipim \
        --disable-script \
        --disable-mixer \
        --disable-presence \
        --disable-g.722.1 \
        --enable-cpp14 \
        OPAL_MAJOR=%{version_major} \
        OPAL_MINOR=%{version_minor} \
        OPAL_STAGE=%{opal_stage} \
        OPAL_PATCH=%{version_patch} \
        OPAL_OEM=%{version_oem}
make %{?_smp_mflags} REVISION_FILE= OPAL_FILE_VERSION=%{version} all


%install
source /opt/rh/devtoolset-9/enable
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT OPAL_FILE_VERSION=%{version}
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%clean
rm -rf $RPM_BUILD_ROOT


%post
%{_datadir}/opal/install_openh264.sh %{_libdir}
/sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%{_libdir}/opal-%{version_major}.%{version_minor}.%{version_patch}-%{version_oem}
%{_datadir}/opal
%{_bindir}/conopal

%files devel
%defattr(-,root,root,-)
%{_includedir}/opal
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc
%{_datadir}/opal

%files static
%defattr(-,root,root,-)
%{_libdir}/*.a


%changelog
* Fri Dec 02 2016 Gavin Llewellyn <gavin.llewellyn@blackboard.com> - 3.17.1-1
- Initial RPM release

