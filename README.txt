skf README
----------

1. Introduction
   skf is an i18n-capable kanji filter. skf is designed for
  reading documents in various languages and codes using kanji
  or unicode capable display devices. Like other kanji filters,
  skf provides basic japanese kanji code conversion features, 
  include to/from JIS, EUC, Shift-JIS, UCS2, KEIS83 and UTF-7/8,
  but also support various international codesets include Korian
  and Chinese standard codesets.
   Unlike nkf, skf does not provide additional fancy features, but
  it has support for ISO-8859's, European domestic sets, CJKV code 
  conversion,   IBM gaiji support and can add other code supports 
  easily.

   skf does not depend on glibc kanji support, iconv or other 
  m17n supports within OS.
  This program provides everything required for conversion in it.

2. Install SKF

  Always grab the latest skf package from http://osdn.jp/projects/skf

   Since skf depends on only basic file I/O and provides configure
  scripts, install should be easy except some switches related to 
  locale features. Here are some supported configure options. Since
  1.96, skf needs perl on compile.

  --enable[disable]-getenv
	Enable/Disable getting user setting from environment
	variables using gettext framework. See man page. Default 
	is enabled, and will be off on platform without gettext.

  --enable[disable]-oldnec
	Enable/Disable old NEC PC Kanji-shift sequence (i.e.
	ESC-K, H). Default is enabled.

  --enable[disable]-debug
	Enable/Disable compiling debugging codes into skf.
	Default is disabled. To gather information for debug,
	use -% option (see man page).

  --enable[disable]-dynamic
	Enable/Disable compiling code tables into skf. Note 
	that disabling this feature results in fairly large
	binary. Default is enabled.

  --enable[disable]-nls
	Enable/Disable locale specific messages. Default is
	enabled, but automatically disabled on OS's without
	gettextized NLS support. Note that skf only contains
	Japanese messages.

  Standard autoconf options, like --prefix and --exec-prefix are
  also supported.
  As shown above, skf implements locale features using GNU gettext
  framework, and the framework is used only to get localized 
  messages. skf does not support X11's catget features.

2.1 Linux, M*cOS X and Un*x-like systems
   If you are using a kind of modern unices system, just type 
  ./configure and then make. You'll get working binary. Ubuntu 8.04
  and NetBSD 5.01 is tested for 1.97. You might need the gettext
  development package installed to compile.Z
  Compilation on M*cOS X system (10.5 and 10.6 for 1.97) is tested and
  supported, but needs gettext package from macport. OS versions under
  10.4 and 10.7 are not supported. Especially, llvm-gcc is not supported.
  If you are using an other kind of Un*x style system and find compile
  failures, just report! Or try to adjust locale features 
  (--disable-nls), external table features (--disable-dynamic) and CFLAGS
  in Makefile for your system. 

2.1.1. Create RPM package
   Use "make srpm" creates a RPM source package in rpm/SRPMS and 
   prepare files to make rpms. Note that ${HOME}/.rpmmacro will
   be overwritten.  You are warned 8-)
   After that, make rpm creates RPM package in rpm/RPMS/${arch}.

2.1.2. Create DEB package
   Debian package creation is well tested and supported.  You can 
   build package using dpkg-buildpackage with debhelper (Yep. You
   need debhelper to make deb. See debhelper's manual). 

2.2 Wind*ws
   For cygwin environment, cygwin-version binaries compilation is
  tested and supported. You will need latest cygwin environment and
  both gettext and gettext-dev packages for nls feature which will
  be automatically enabled by configure.
  Please refer descriptions in Makefile. 
  skf-1.94 and later doesn't support SFU 3.5. Proceeds with cautions :-)

   If you want some detailed or esoteric/cosmetic setting changes,
  see config.h. Common settings can be handled in Makefile.

  Compilation under Msys environment is partially tested and supposed
  to work. Note that it is not fully tested. You are warned.

3. Install SKF plugins (will be enabled in 1.94 or later)
   Prerequisite: SWIG, Development package of the target LWL.
  For perl extension, use "make perlext" in skf-root directory. 
  Binary is generated in skf root directory. Requires gcc, and other
  GNU related stuff besides the script language itself. For ruby, 
  s/perl/ruby/. Extension directory used in installing is system
  default arch. directory.
  Created extension will be skf.so(SKF.so in cygwin environment).
  Note that name of function in perl extension is set to
  skfc::convert in some cygwin-related environment,
  not skf::convert in 1.94 or later built under cygwin.

Happy Hacking!

Seiji Kaneko <efialtes@osdn.jp>, 12 Mar 2007 07:55 +0900

