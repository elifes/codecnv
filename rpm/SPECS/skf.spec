Summary: Simple Kanji Filter - i18n kanji converter
Name: skf
Version: 2.10
Release: 0
License: BSD
Group: Applications/Text
Source: http://osdn.jp/project/skf/files/skf_%{version}.tar.gz

Buildroot: $HOME/skf-root

%description
 skf is an i18n-capable kanji filter. It works like i18n enhanced
 nkf-1.6. More specifically, skf is intended to read documents in 
 various languages and codes using kanji or unicode capable
 display devices. skf provides basic kanji code conversion features,
 include to/from JIS, EUC, Shift-JIS, Unicode and Vendor specific
 codes. skf also includes decoder for various code appears in the Net.
 Unlike nkf, it does not provide broken jis recovery nor mime 
 encoding, but it has support for ISO-8859's, European domestic set,
 JIS X-0212/X-0213 code, has IBM/MS vendor specific code support and
 can add other code support easily.

%prep
%setup -q

%build
autoconf
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/usr --mandir=/usr/share/man
rm -rf $RPM_BUILD_ROOT
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT exec_prefix=/usr \
	prefix=/usr DOCDIR=/usr/share/doc/skf-%{version} install
make DESTDIR=$RPM_BUILD_ROOT exec_prefix=/usr \
	prefix=/usr DOCDIR=/usr/share/doc/skf-%{version} locale_install
make DESTDIR=$RPM_BUILD_ROOT exec_prefix=/usr \
	prefix=/usr DOCDIR=/usr/share/doc/skf-%{version} rpmchange_install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin/skf
/usr/share/man/man1/skf.1.gz
/usr/share/man/ja_JP.UTF-8/man1/skf.1.gz
/usr/share/man/ja/man1/skf.1.gz
/usr/share/skf
/usr/share/skf/lib/%version/readme.txt
/usr/share/locale/ja/LC_MESSAGES/skf.mo

%doc CHANGES_ja.txt README.txt changelog copyright doc/
/usr/share/doc/skf-%version/CHANGES_ja.txt
/usr/share/man/ja/man1/skf.1.gz

%changelog
* Fri Jan 7 2017 Seiji Kaneko <efialtes@osdn.jp>
- 2.10 See debian/changelog
* Thu May 21 2015 Seiji Kaneko <efialtes@osdn.jp>
- 2.00 See debian/changelog
* Thu Jan 30 2014 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.99.8 See debian/changelog
* Mon Dec 16 2013 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.99.7 See debian/changelog
* Fri Jul 26 2013 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.99.6 See debian/changelog
* Sat Jun 06 2013 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.99.5 See debian/changelog
* Sat Feb 16 2012 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.99.0 See debian/changelog
* Sat Feb 7 2010 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.97.0 See debian/changelog
* Sat Dec 25 2008 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.96.0 See debian/changelog
* Sat Mar 31 2007 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.95 release. See debian/changelog
* Sat Sep 9 2006 Seiji Kaneko <efialtes@sourceforge.jp>
- 1.95 Beta 1. See debian/changelog

