Summary: Weibo adinf data transfer
Name: aidt
Version:0.1.0
Release:el6.5
Group:Development/Tools
License:BSD
URL: https://github.com/weiboad/aidt
Vendor: nmred  <nmred_2008@126.com>
Packager: nmred  <nmred_2008@126.com>
Distribution: Weibo adinf data transfer
Source:%{name}-%{version}.tar
Buildroot:%{_tmppath}/%{name}-%{version}
Prefix:/usr/local/adinf/aidt
Requires:sudo
%define debug_packages %{nil}
%define debug_package %{nil}
%define _topdir /usr/home/zhongxiu/rpmbuild
%description
-------------------------------------
- Everything in order to facilitate -
-------------------------------------

%prep
%setup -q
%build

%pre
if test -d %{prefix}; then
	echo ""
	echo "Install dir \'%{prefix}\' exists. You need resolve it manually before install aidt. Exit now."
	echo ""
# return !0 for exit.
	test ""
fi
%install
#rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{prefix}
cp -r * $RPM_BUILD_ROOT%{prefix}

%clean
#rm -rf $RPM_BUILD_ROOT

%post
# build install
echo ""
echo "Build: START"
echo -e "Build: [0;32mDONE[0m"
%preun
echo -n "unrpm  ... "
SW_TMP_BAK_PATH=%{prefix}-backup-$(date +%%Y%%m%%d-%%H%%M%%S)
mv %{prefix} $SW_TMP_BAK_PATH
echo "Old version '%{prefix}' backup to '$SW_TMP_BAK_PATH'"
%postun
%files
%{prefix}

%changelog
