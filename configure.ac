dnl Process this file with autoconf to produce a configure script.
dnl ******************************************************************
dnl Copyright (c) 2005-2015 Seiji Kaneko. All rights reserved.
dnl Everyone is permitted to use this program in source and binary
dnl form, with or without modification if and only if the following
dnl conditions are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl   notice, copyright notice written in source code, additional list
dnl   of conditions and the following disclaimer.
dnl 2. Redistributions in machine readable form must reproduce the
dnl   above copyright notice and the following disclaimer in the
dnl   documentation or other material provided with the distribution.
dnl 3. Neither the name of the copyright holders nor the names of its 
dnl   contributors may be used to endorse or promote products derived from 
dnl   this software without specific prior written permission.
dnl *******************************************************************
dnl Disclaimer: This software is provided and distributed AS IS,
dnl without any implicit or explicit warranties, and not
dnl guaranteed to be error-free. In no event shall the author be
dnl liable for any direct, indirect or incidental damages,
dnl including, but not limited to, loss of data, use or profits
dnl responsibility for any direct or indirect damages or results
dnl arising by using whole or a part of this software.
dnl *******************************************************************

AM_MAINTAINER_MODE
AC_INIT
AC_CONFIG_SRCDIR([skf.c])
AC_REVISION($Id: configure.ac,v 1.63 2017/01/20 15:16:21 seiji Exp seiji $)
AC_CONFIG_AUX_DIR([./scripts])

AM_INIT_AUTOMAKE([skf], [2.10], no-define)
ALL_LINGUAS="ja"
SKFADDCFLG=""

AC_CONFIG_HEADERS([config.h])
AC_COPYRIGHT(copyright)
AC_CHECK_PROGS(PERL, perl, false)

dnl User settings to pass to configure
AC_ARG_ENABLE(getenv,
[  --enable-getenv	enable reading user settings from environment variable [(default yes)]])
if test "$enable_getenv" != "no"; then
   CFLAGS="$CFLAGS -DHAVE_GETENV"
fi
AC_ARG_ENABLE(fast_getc,
[  --enable-fast_getc	enable internal getc function [(default yes)]])
if test "$enable_fast_getc" != "no"; then
   CFLAGS="-DFAST_GETC $CFLAGS"
fi
AC_ARG_ENABLE(oldnec,
[  --enable-oldnec	support old NEC ESC code (i.e. ESC-K, ESC-H) [(default yes)]])
if test "$enable_oldnec" != "no"; then
   CFLAGS="-DOLD_NEC_COMPAT $CFLAGS"
fi
debugopt='-DSKFDEBUG -DSKFPDEBUG'
AC_ARG_ENABLE(debug,
[  --enable-debug	enable debugging [(default no)]])
if test "$enable_debug" = "yes"; then
   CFLAGS="$debugopt $CFLAGS"
fi
AC_ARG_ENABLE(keisextra,
[  --enable-keisextra	support KEIS Ext.Area 2,3 [(default yes)]])
if test "$enable_keisextra" != "no"; then
   CFLAGS="-DKEIS_EXTRA_SUPPORT $CFLAGS"
fi
AC_ARG_ENABLE(dynamic,
[  --enable-dynamic	loading external table on demand basis [(default yes)]])
if test "$enable_dynamic" != "no"; then
   CFLAGS="-DDYNAMIC_LOADING $CFLAGS"
   compflag=""
else
   compflag="-t"
fi
AC_ARG_ENABLE(profile,
[  --enable-profile	enable profiling [(default no)]])
if test "$enable_profile" = "yes"; then
   CFLAGS="$CFLAGS -p -pg"
   LDFLAGS="$LDFLAGS -p -pg"
fi
AC_ARG_ENABLE(folding,
[  --enable-folding	support fmt-like folding support [(default yes)]])
if test "$enable_folding" != "no"; then
   CFLAGS="-DFOLD_SUPPORT $CFLAGS"
fi

ENABLE_STRIP=""
AC_ARG_ENABLE(strip,
[  --enable-strip	enable binary-strip on install [(default yes)]])
if test "$enable_strip" != "no"; then
   ENABLE_STRIP="-s"
fi

AC_ARG_ENABLE(decompose,
[  --enable-decompose	support unicode decomposition support [(default yes)]])
if test "$enable_decompose" != "no"; then
   if test -f UnicodeData.txt && test "$PERL" != "false"; then
       CFLAGS="-DUNI_DECOMPOSE $CFLAGS"
       uniparse_script="./scripts/parse_uni.p"
   else
       uniparse_script="./scripts/parse_uni.p"
       compflag = "-n"
   fi
else
       uniparse_script="./scripts/parse_uni.p"
       if test "$enable_dynamic" != "no"; then
       		compflag = ""
	else
       		compflag = "-n"
	fi
fi

AC_ARG_ENABLE(python3,
[  --enable-python3	support Python3 specific code support [(default no)]])
AC_ARG_ENABLE(py3feature,
[  --enable-py3feature	support Python3 feature support [(default no)]])
AC_ARG_ENABLE(ruby24,
[  --enable-ruby24	prefer to make Ruby 2.4.x extension [(default no)]])
AC_ARG_ENABLE(ruby23,
[  --enable-ruby23	prefer to make Ruby 2.3.x extension [(default no)]])
AC_ARG_ENABLE(ruby22,
[  --enable-ruby22	prefer to make Ruby 2.2.x extension [(default no)]])
AC_ARG_ENABLE(ruby21,
[  --enable-ruby21	prefer to make Ruby 2.1.x extension [(default no)]])
AC_ARG_ENABLE(ruby20,
[  --enable-ruby20	prefer to make Ruby 2.0.x extension [(default no)]])
AC_ARG_ENABLE(ruby19,
[  --enable-ruby19	prefer to make Ruby 1.9.x extension [(default no)]])
AC_ARG_ENABLE(ruby18,
[  --enable-ruby18	prefer to make Ruby 1.8.x extension [(default no)]])
AC_ARG_ENABLE(mactiger,
[  --enable-mactiger	Enable support for MacOS X 10.4/10.5 [(default no)]])
AC_SUBST(uniparse_script)
AC_SUBST(compflag)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB

dnl Check for sed - We need sed to compile
AC_PATH_PROG([SKFSED], [sed], 
	[AC_MSG_ERROR([skf compilation needs sed])],
	[/usr/local/bin:/usr/bin:/bin])
AC_SUBST(SKFSED)

dnl Check for rpm build
AC_PATH_PROG([RPMBUILD], [rpmbuild],[rpm],[/usr/local/bin:/usr/bin:/bin])
AC_SUBST(RPMBUILD)

dnl Checks for libraries.
dnl Replace `main' with a function in -lintl:
AC_CHECK_LIB(intl, main)

lskfdir='${prefix}/share/skf'

dnl set defaults in LWL extensions
ENV_DARWIN="no"
ENV_CYGWIN="no"
MINGW32="no"

config_pos_indep="-fPIC"
ruby_config_shared="${CC} -shared"
python_config_shared="${CC} -shared"
php_config_shared="${CC} -shared"
lua_config_shared="${CC} -shared"
tcl_config_shared="${CC} -shared"
gnu_s_config_shared="${CC} -shared"
ruby_extlib=""
perl_extlib=""
python_extlib=""
php_extlib=""
tcl_extlib=""
tcl_extlib_add=""
lua_extlib=""
gnu_s_extlib=""
llibruby=""
llibperl=""
llibpython=""
llibphp=""
llibtcl=""
lliblua=""
llibgnu_s=""
darwin_shared=""
ruby_binary="skf.so"
perl_binary="skf.so"
perl_pm="skf.pm"
python_binary="_skf.so"
python_script="skf.py"
php_binary="skf.so"
lua_binary="skf.so"
tcl_binary="skf.so"
gnu_s_binary="skf.so"
python_sitearch_dir=""
lua_sitearch_dir=""
tcl_sitearch_dir=""
gnu_s_sitearch=""
deployment_target=""
ruby_swig_opt=""
tcl_header_search=""

ruby_lib_dir="${libdir}"
perl_lib_dir="${libdir}"
php_lib_dir="${libdir}"
python_lib_dir="${libdir}"
lua_lib_dir="${libdir}"
tcl_lib_dir="${libdir}"
gnu_s_lib_dir="${libdir}"

lineend_default="-DDEFAULT_EOL_LF"

dnl change default include directory
test "$site_include_dir" = NONE && site_include_dir=/usr/include

dnl perl must exist, and test is already performed.
AC_CHECK_PROGS(PHP, php, false)
AC_CHECK_PROGS(LUA, lua, false)
AC_CHECK_PROGS(LUACONFIG, lua-config, false)
AC_CHECK_PROGS(TCL, tclsh, false)
AC_CHECK_PROGS(GNU_S, R, false)
AC_CHECK_PROGS(MSGFMT, msgfmt, false)

dnl Which version is preferred?
dnl check whether we have python3.x or ruby1.9.x

dnl trying to determine target ruby version
ruby_21_preferred="no"
ruby_20_preferred="no"
ruby_19_preferred="no"
ruby_18_preferred="no"

dnl shut up ubuntu additional check
ubuntu_smudge="-fno-delete-null-pointer-checks "

if test "$enable_ruby24" = "yes"; then
    AC_CHECK_PROGS(RUBY,[ruby2.4 ruby2.4.0 ruby2.4.1 ruby2.4.2 ruby2.4.3 ruby2.4.4 ruby2.4.5 ruby2.4.6], false)
elif test "$enable_ruby23" = "yes"; then
    AC_CHECK_PROGS(RUBY,[ruby2.3 ruby2.3.0 ruby2.3.1 ruby2.3.2 ruby2.3.3 ruby2.3.4 ruby2.3.5 ruby2.3.6 ruby2.3.7 ruby2.3.8], false)
elif test "$enable_ruby22" = "yes"; then
    AC_CHECK_PROGS(RUBY,[ruby2.2 ruby2.2.0 ruby2.2.1 ruby2.2.2 ruby2.2.3 ruby2.2.4 ruby2.2.5 ruby2.2.6 ruby2.2.7 ruby2.2.8], false)
    dnl we have ruby 2.1 or higher
elif test "$enable_ruby21" = "yes"; then
    AC_CHECK_PROGS(RUBY,[ruby2.1 ruby2.1.0 ruby2.1.1 ruby2.1.2 ruby2.1.3 ruby2.1.4 ruby2.1.5 ruby2.1.6 ruby2.1.7 ruby2.1.8 ruby2.1.9 ruby2.1.10], false)
    dnl we have ruby 2.1 or higher
elif test "$enable_ruby20" = "yes"; then
    AC_CHECK_PROGS(RUBY,[ruby2.0 ruby2.0.0 ruby2.0.1 ruby2.0.2], false)
elif test "$enable_ruby19" = "yes"; then
    AC_CHECK_PROGS(RUBY,[ruby1.9 ruby1.9.0 ruby1.9.1 ruby1.9.2 ruby1.9.3], false)
elif test "$enable_ruby18" = "yes"; then
    AC_CHECK_PROGS(RUBY,[ruby1.8], false)
else
dnl if no prefernce is given or no version specific ruby found
    AC_CHECK_PROGS(RUBY, ruby, false)
fi

dnl if specified versioned ruby is not found, try generic one
if test "$RUBY" = "false"; then
    RUBY=""
    AC_CHECK_PROGS(RUBY, [ruby], false)
fi

if test "$RUBY" != "false"; then
dnl test ruby version.
dnl 2.0.x and 1.9.x replies with 3 digits. 1.8.x with 2 digits.
dnl we only use major and minor version for extension,
   AC_MSG_CHECKING([ruby version for extension])
   ruby_version=`$RUBY -rrbconfig -e'printf "%.1s%.1s",RbConfig::CONFIG.fetch("MAJOR"),RbConfig::CONFIG.fetch("MINOR")'`
   ruby_maj_version=`$RUBY -rrbconfig -e'printf "%.1s",RbConfig::CONFIG.fetch("MAJOR")'`
   AC_MSG_RESULT($ruby_version)

dnl if no prefernce is given or no version specific ruby found

dnl ruby1.7 or older are not supported
	if test "$ruby_version" = "24"; then
	    ruby_24_preferred="yes"
	    ruby_23_preferred="yes"
	    ruby_22_preferred="yes"
	    ruby_21_preferred="yes"
	    ruby_20_preferred="yes"
	    ruby_19_preferred="yes"
	elif test "$ruby_version" = "23"; then
	    ruby_23_preferred="yes"
	    ruby_22_preferred="yes"
	    ruby_21_preferred="yes"
	    ruby_20_preferred="yes"
	    ruby_19_preferred="yes"
	elif test "$ruby_version" = "22"; then
	    ruby_22_preferred="yes"
	    ruby_21_preferred="yes"
	    ruby_20_preferred="yes"
	    ruby_19_preferred="yes"
	elif test "$ruby_version" = "21"; then
	    ruby_21_preferred="yes"
	    ruby_20_preferred="yes"
	    ruby_19_preferred="yes"
	elif test "$ruby_version" = "20"; then
	    ruby_20_preferred="yes"
	    ruby_19_preferred="yes"
	elif test "$ruby_version" == "19"; then
	    ruby_19_preferred="yes"
	elif test "$ruby_version" == "18"; then
	    ruby_18_preferred="yes"
	else
dnl should be ruby 2.2 or later
	    ruby_21_preferred="yes"
	    ruby_20_preferred="yes"
	    ruby_19_preferred="yes"
	fi
fi

if test "$enable_python3" = "yes"; then
    AC_CHECKING([if we have python3])
    AC_CHECK_PROGS(PYTHON,[python3.5 python3.4 python3.3 python3.2 python3.1 python3 python], false)
    if test "$PYTHON" = "false"; then
    	enable_python3="no"
    fi
fi

if test "$enable_python3" = "yes"; then
    python_swig_opt="-py3"
else
    python_swig_opt=""
fi

if test "$enable_python3" = "yes" && test "$enable_py3feature" = "yes"; then
    python_version="PYTHON3"
    python_define="-DSKF_PYTHON3 -DSKF_PYTHON"
    python_optstr_typ="PyObject"
    python_srcstr_typ="PyObject"
    python_tgtstr_typ="PyObject"
else
    python_version="PYTHON2"
    AC_CHECK_PROGS(PYTHON,[python2.7 python2.6 python2.5 python2.4 python], false)
    python_define="-DSKF_PYTHON2 -DSKF_PYTHON"
    python_optstr_typ="char"
    python_srcstr_typ="char"
    python_tgtstr_typ="char"
fi

dnl Some system related differences treatments
dnl Trying to find hosts name
AC_CANONICAL_HOST
case $host_os in
   *cygwin* ) ENV_CYGWIN=yes;;
   *darwin* ) ENV_DARWIN=yes;;
   *mingw32* ) MINGW32=yes;;
   * ) ENV_CYGWIN=no; ENV_DARWIN=no;
esac

multiarch_header_name=""
dnl MULTIARCH TEST -- this IS dirty hack. I know.
   multiarch_header_n1=${host_cpu}-${host_os}
   multiarch_header_n2=${host_cpu}-${host_vendor}-${host_os}
   AC_CHECK_HEADERS([${multiarch_header_n1}/sys/types.h],
   	multiarch_header_name=${multiarch_header_n1})
   AC_CHECK_HEADERS([${multiarch_header_n2}/sys/types.h],
	multiarch_header_name=${multiarch_header_n2})
AC_MSG_CHECKING([for multiarch headers])
AC_MSG_RESULT(${multiarch_header_name})

dnl Cygwin and Msys. Generic handling.
AC_MSG_CHECKING([if we are building on cygwin])
AC_MSG_RESULT($ENV_CYGWIN)
if test "$ENV_CYGWIN" = "yes"; then
    config_pos_indep=""
    if test "$enable_mingw" = "yes"; then
	SKFADDCFLG="-mno-cygwin $SKFADDCFLG"
    fi
    lineend_default="-DDEFAULT_EOL_CRLF"
else
    AC_MSG_CHECKING([if we are building on mingw])
    AC_MSG_RESULT($MINGW32)
dnl MinGW32 handling: from bugreport by naruse-san.
    if test "$MINGW32" = "yes"; then
	config_pos_indep=""
	lineend_default="-DDEFAULT_EOL_CRLF"
	AC_DEFINE(SKF_MINGW)
	lskfdir=`echo ${EXEPATH}|sed 's/\\\\/\\//'`${lskfdir}
    fi
fi

dnl Mac OS X shared library creation fix
AC_MSG_CHECKING([if we are building on darwin])
AC_MSG_RESULT($ENV_DARWIN)
if test "$ENV_DARWIN" = "yes"; then
    config_pos_indep="-fPIC -fno-common"
    AC_MSG_CHECKING([major version])
dnl    darwin_major_release=`uname -r | sed 's/\([0-9]*\)\.[0-9]*\.[0-9]*/\1/'`
dnl 10.5 has Darwin version 9.*.
dnl 10.6 has Darwin version 10.*.
dnl 10.7 has Darwin version 11.*.
dnl 10.8 has Darwin version 12.*.
dnl 10.9 has Darwin version 13.*.
dnl 10.10 has Darwin version 14.*.
dnl 10.11 has Darwin version 15.*.
dnl 10.12 has Darwin version 16.*.
    darwin_major_release=`uname -r`
    deployment_release=`sw_vers -productVersion | cut -d. -f-2`
dnl should I specify newer version?
    case $darwin_major_release in
    	16* ) darwin_shared="-arch x86_64";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.6;;
dnl		deployment_target=MACOSX_DEPLOYMENT_TARGET=${deployment_release};;
    	15* ) darwin_shared="-arch i386 -arch x86_64";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.6;;
    	14* ) darwin_shared="-arch i386 -arch x86_64";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.6;;
    	13* ) darwin_shared="-arch i386 -arch x86_64";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.6;;
    	13* ) darwin_shared="-arch i386 -arch x86_64";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.6;;
    	11* ) darwin_shared="-arch i386 -arch x86_64";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.6;;
    	10* ) darwin_shared="-arch i386 -arch x86_64";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.6;;
    	9* ) darwin_shared="-arch i386 -arch ppc";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.4;;
	* ) darwin_shared="-arch i386 -arch x86_64";
		deployment_target=MACOSX_DEPLOYMENT_TARGET=10.6;;
    esac
    if test "$mactiger" == "yes"; then
	deployment_target="MACOSX_DEPLOYMENT_TARGET=10.4"
    fi
    lineend_default="-DDEFAULT_EOL_CR"
    ubuntu_smudge=""
fi

dnl Thanks Ubun*u team for nothing.
if test "$GCC" = "yes"; then
    SKFADDCFLG="$SKFADDCFLG -Wall -Wno-format-security ${ubuntu_smudge}"
fi
AC_EXEEXT

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(langinfo.h)
AC_CHECK_FUNCS(nl_langinfo)
AC_CACHE_SAVE

dnl extensions: .. ruby, perl, swig etc. existence
AC_CHECK_PROGS(SWIG, swig, :)

if test "$RUBY" != "false"; then
   AC_CHECKING([ruby headers and libs])

dnl test ruby-site architecture
   ruby_sitearch=`$RUBY -rrbconfig -e'puts RbConfig::CONFIG.fetch("sitearch")'`
   rubycppflags=`$RUBY -rrbconfig -e'puts RbConfig::CONFIG.fetch("CPPFLAGS")'`
   ruby_arch_dir=${ruby_headers_dir}/`$RUBY -rrbconfig -e'puts RbConfig::CONFIG.fetch("archdir")'`
   ruby_arch_headers_dir=""
   ruby_alt_headers_dir=""

dnl checking where ruby.h resides
   AC_MSG_CHECKING([ruby base header])
   ruby_base_headers_dir=`$RUBY -rrbconfig -e'printf "%s",RbConfig::CONFIG.fetch("rubyhdrdir")'`
   if test "${ruby_base_headers_dir}" = "" ; then
      ruby_base_headers_dir=`$RUBY -rrbconfig -e'printf "%s/ruby",RbConfig::CONFIG.fetch("includedir"),RbConfig::CONFIG.fetch("ruby_version")'`
      ruby_altheaders_dir=`$RUBY -rrbconfig -e'printf "%s/ruby",RbConfig::CONFIG.fetch("includedir")'`
      AC_CHECK_FILE("${ruby_headers_dir}/ruby.h",,
	    ruby_base_headers_dir="${ruby_altheaders_dir}"
      )
   fi
   AC_MSG_RESULT(${ruby_base_headers_dir})

dnl Site headers
dnl 2.0 and 2.1 has sitehdrdir
   AC_MSG_CHECKING([ruby site headers])
if test "$ruby_20_preferred" = "yes"; then
       ruby_arch_headers_dir=`$RUBY -rrbconfig -e'printf "%s",RbConfig::CONFIG.fetch("rubyarchhdrdir")'`
else
       ruby_arch_headers_dir="${ruby_base_headers_dir}/${ruby_sitearch}"
fi
   AC_MSG_RESULT($ruby_arch_headers_dir)

dnl testing ruby shared library flags
   AC_MSG_CHECKING([ruby shared flags])
if test "$ruby_21_preferred" = "yes"; then
   ruby_config_shared_flag=`$RUBY -rrbconfig -e'puts RbConfig::CONFIG.fetch("LIBRUBYARG_SHARED")'`
   ruby_config_shared="${CC} -shared ${ruby_config_shared_flag}"
else
   ruby_config_shared=`$RUBY -rrbconfig -e'puts RbConfig::CONFIG.fetch("LIBRUBY_LDSHARED")'`
fi
   AC_MSG_RESULT(${ruby_config_shared})

   AC_MSG_CHECKING([ruby header files])
   ruby_headers_dir="-I${ruby_base_headers_dir} -I${ruby_arch_dir} -I${ruby_arch_headers_dir} -I/usr/include"
   ruby_libbase_dir=\"`$RUBY -rrbconfig -e'printf "%s",RbConfig::CONFIG.fetch("libdir")'`\"
   AC_MSG_RESULT($ruby_headers_dir)

dnl Cygwin/Mingw dll search
   ruby_dll=`$RUBY -rrbconfig -e'printf "%s/%s",RbConfig::CONFIG.fetch("libdir"),RbConfig::CONFIG.fetch("LIBRUBY")'`
   llibruby=`$RUBY -rrbconfig -e'puts RbConfig::CONFIG.fetch("LIBRUBYARG")'`

   if test "$ENV_DARWIN" = "yes"; then
	llibruby="-lruby"
	ruby_binary="skf.bundle"
   elif test "$ENV_CYGWIN" = "yes"; then
	if test "$enable_ruby19" = "yes"; then
	    ruby_extlib=${ruby_libbase_dir}/${ruby_dll}
	else
	    ruby_extlib=${ruby_dll}
	fi
	ruby_binary="skf.so"
   elif test "$MINGW32" = "yes"; then
	ruby_config_shared="$ruby_config_shared -LD"
	ruby_extlib=${ruby_libbase_dir}/${ruby_dll}
   else
     if test "$mingw32" = "yes"; then
	ruby_config_shared="$ruby_config_shared -LD"
     fi
   fi
   AC_ARG_WITH([ruby_sitearch_dir],
      [AS_HELP_STRING([--with-ruby-sitearch-dir],
	[where ruby extension should go (default system defined)])],
	[ruby_sitearch_dir=$withval],[ruby_sitearch_dir=NONE])

   test "$ruby_sitearch_dir" = NONE && ruby_sitearch_dir=`ruby -rrbconfig -e'puts RbConfig::CONFIG.fetch("sitearchdir")'`

   AC_SUBST(llibruby)
   AC_SUBST(ruby_headers_dir)
   AC_SUBST(ruby_binary)
   AC_SUBST(ruby_sitearch_dir)
   AC_SUBST(ruby_lib_dir)
   AC_SUBST(ruby_extlib)
   AC_SUBST(ruby_version)
   AC_SUBST(ruby_maj_version)
   AC_SUBST(ruby_swig_opt)
fi

dnl perl development environment test
dnl perl runtime must exist, but dev env is only for extension.
if test "$PERL" != "false"; then
   AC_CHECKING([perl headers and libs])
   perl_config_shared=`perl -e 'use Config; print $Config{lddlflags};'`
   cppflags=`perl -e 'use Config;print $Config{cppflags};'`
   perl_headers_dir=\"`scripts/find-perl`\"

   AC_CHECK_LIB(perl,main,llibperl="-lperl")

   if test "$ENV_DARWIN" = "yes"; then
	perl_binary="skf.bundle"
	case $darwin_major_release in
	    10* ) llibperl="";;
	    9* ) llibperl="";;
	    * ) llibperl="-lperl";;
	esac
   elif test "$ENV_CYGWIN" = "yes"; then
	perl_config_shared="-shared "
	perl_headers_dir="$perl_headers_dir -I/usr/include"
	perl_extlib=`scripts/find-perl`"/libperl.dll.a"
	perl_binary="Skf.so"
   elif test "$MINGW32" = "yes"; then
	perl_config_shared="-shared "
	perl_extlib=\'`scripts/find-perl`"/"`perl -e'use Config;print $Config{libperl};'`\'
	perl_binary="skf.dll"
   fi

   AC_ARG_WITH([perl_sitearch_dir],
      [AS_HELP_STRING([--with-perl-sitearch-dir],
	[where perl extension should go (default system defined)])],
	[perl_sitearch_dir=$withval],[perl_sitearch_dir=NONE])

   test "$perl_sitearch_dir" = NONE && perl_sitearch_dir="`perl -e 'use Config;print $Config{sitearch};'`"

   AC_SUBST(perl_headers_dir)
   AC_SUBST(perl_sitearch_dir)
   AC_SUBST(llibperl)
   AC_SUBST(perl_extlib)
   AC_SUBST(perl_binary)
   AC_SUBST(perl_lib_dir)
   AC_SUBST(perl_pm)
   AC_SUBST(PERL)
fi

dnl PHP: cygwin and Msys are not supported.
if test "$PHP" != "false"; then
   AC_CHECKING([PHP headers and libs])
   php_headers_dir=`php-config --includes`
   llibphp=`php-config --ldflags`

   AC_ARG_WITH([php_sitearch_dir],
      [AS_HELP_STRING([--with-php-sitearch_dir],
	[where php extension should go (default system defined)])],
	[php_sitearch_dir=$withval],[php_sitearch_dir=NONE])

   test "$php_sitearch_dir" = NONE && php_sitearch_dir=`php-config --extension-dir`

   AC_SUBST(php_headers_dir)
   AC_SUBST(llibphp)
   AC_SUBST(php_binary)
   AC_SUBST(php_sitearch_dir)
   AC_SUBST(php_config_shared)
   AC_SUBST(php_extlib)
   AC_SUBST(php_lib_dir)
fi

dnl PYTHON
if test "$PYTHON" != "false"; then
   AC_CHECKING([python headers and libs])
   AC_MSG_CHECKING([python version for extension])
   dnl DO NOT ASK WHY. I too can't explain.
   python_version=`(LANG=C ${PYTHON} --version 2>&1 | ${PERL} -e '$vin=<>;printf "python%s",substr($vin,7,rindex($vin,"\.",20)-7);')`
   AC_MSG_RESULT($python_version)
   python_config_bin=${python_version}-config
   python_configm_bin=${python_version}m-config
   python_configmu_bin=${python_version}mu-config
   python_configu_bin=${python_version}u-config
   python_configd_bin=${python_version}d-config
   python_configmd_bin=${python_version}md-config
   python_configmud_bin=${python_version}mud-config
   python_configud_bin=${python_version}ud-config
   AC_CHECK_PROGS(PYTHON_CONFIG,[${python_configm_bin} ${python_configmu_bin} ${python_configu_bin} ${python_config_bin} ${python_configmd_bin} ${python_configd_bin}],false)

   if test "$PYTHON_CONFIG" = "false"; then
       AC_MSG_RESULT("Python plugin compilation needs python-config command")
   else
       AC_ARG_WITH([python_sitearch_dir],
	  [AS_HELP_STRING([--with-python-sitearch-dir],
	    [where python extension should go under $libdir (default system defined)])],
	    [python_sitearch_dir=$withval],[python_sitearch_dir=NONE])

       test "$python_sitearch_dir" = NONE && python_sitearch_dir="${libdir}/${python_version}"

       AC_MSG_CHECKING([whether python3 feature enabled])
       if test "$enable_python3" = "yes" && test "$enable_py3feature" = "yes"; then
           AC_MSG_RESULT([yes])
       else
           AC_MSG_RESULT([no])
       fi
    dnl test which directories has headers.
       
       AC_MSG_CHECKING([for python header file])
       python_headers_dir=`${PYTHON_CONFIG} --includes`
       if test "$enable_python3" = "yes"; then
	   python_abiflags=`${PYTHON_CONFIG} --abiflags`
       else
	   python_abiflags=""
       fi
       AC_MSG_RESULT($python_headers_dir)
       python_prefix=`${PYTHON_CONFIG} --prefix`
       python_confname="config"
       if test "$enable_python3" = "yes"; then
	   python_confname=`(${PYTHON} --version 2>&1 | ${PERL} -e 'printf "config-%s",substr(<>,7,-3);')`
       fi

       llibpython=`${PYTHON_CONFIG} --libs`

       if test "$ENV_DARWIN" = "yes"; then
	    llibpython=${llibpython}" -lpython"
       elif test "$ENV_CYGWIN" = "yes"; then
	   python_extlib="${python_prefix}/lib/${python_version}/${python_confname}${python_abiflags}/lib${python_version}${python_abiflags}.dll.a"
	   python_binary="_skf.dll"
       elif test "$MINGW32" = "yes"; then
	   python_extlib="${python_prefix}/lib/${python_version}/${python_confname}${python_abiflags}/lib${python_version}${python_abiflags}.dll.a"
	   python_binary="_skf.dll"
       fi

       AC_SUBST(python_headers_dir)
       AC_SUBST(llibpython)
       AC_SUBST(python_extlib)
       AC_SUBST(python_binary)
       AC_SUBST(python_script)
       AC_SUBST(python_config_shared)
       AC_SUBST(python_sitearch_dir)
       AC_SUBST(python_lib_dir)
       AC_SUBST(python_swig_opt)
       AC_SUBST(python_version)
       AC_SUBST(python_define)
       AC_SUBST(python_optstr_typ)
       AC_SUBST(python_tgtstr_typ)
       AC_SUBST(python_srcstr_typ)
   fi
fi

dnl LUA: cygwin, darwin and Msys are not supported.
if test "$LUA" != "false"; then
   AC_CHECKING([lua headers and libs])
   AC_ARG_WITH([lua_sitearch_dir],
      [AS_HELP_STRING([--with-lua-sitearch_dir],
	[where lua extension should go under $libdir (default system defined)])],
	[lua_sitearch_dir=$withval],[lua_sitearch_dir=NONE])
   test "$lua_sitearch_dir" = NONE && lua_sitearch_dir="${libdir}"`(${LUA} -v 2>&1 | ${PERL} -e 'printf "/lua/%s",substr(<>,4,3);')`

   if test "$LUACONFIG" != "false"; then
       lua_headers_dir=`${LUACONFIG} --include`
       lliblua=`${LUACONFIG} --libs`
   else
       lua_headers_dir=`(${LUA} -v 2>&1 | ${PERL} -e 'printf "-I/usr/include/lua%s",substr(<>,4,3);')`
       if test ! -d "$lua_headers_dir" ; then
	   lua_headers_dir=`(${LUA} -v 2>&1 | ${PERL} -e 'printf "-I/usr/include/lua%s%s",substr(<>,4,1),substr(<>,6,1);')`
	   if test ! -d "$lua_headers_dir" ; then
    dnl no idea if following does not exist.
	       lua_headers_dir=`(${LUA} -v 2>&1 | ${PERL} -e 'printf "-I/usr/include/lua%s0",substr(<>,4,1);')`
	   fi
       fi
       lliblua="-llualib50 -llua50"
   fi

   AC_SUBST(lua_headers_dir)
   AC_SUBST(lliblua)
   AC_SUBST(lua_extlib)
   AC_SUBST(lua_binary)
   AC_SUBST(lua_config_shared)
   AC_SUBST(lua_sitearch_dir)
   AC_SUBST(lua_lib_dir)
fi

dnl TCL
if test "$TCL" != "false"; then
   AC_CHECKING([tcl headers and libs])
   AC_ARG_WITH([tcl_sitearch_dir],
      [AS_HELP_STRING([--with-tcl-sitearch-dir],
	[where tcl extension should go under $libdir (default system defined)])],
	[tcl_sitearch_dir=$withval],[tcl_sitearch_dir=NONE])

   test "$tcl_sitearch_dir" = NONE && tcl_sitearch_dir="${libdir}/tcltk/tcl8.4"

   AC_MSG_CHECKING([tcl name])
   tcl_libname=`echo 'puts -nonewline tcl\$tcl_version' | tclsh`
   AC_MSG_RESULT($tcl_libname)

dnl for standard package, headers should be here.
   AC_CHECK_HEADER("tcl.h",,[tcl_header_search="no"])

dnl alternate path searching ...
   if test "$MINGW32" = "yes"; then
      if test "$tcl_header_search" = "no"; then
      	 tcl_header_search=""
	 AC_CHECK_FILE([/c/Tcl/include/tcl/tcl.h],
	    [tcl_headers_dir="/c/Tcl/include/tcl"],
	    [tcl_header_search="no"])
      fi
      if test "$tcl_header_search" = "no"; then
      	 tcl_header_search=""
	 AC_CHECK_FILE([/c/Tcl/include/${tcl_libname}/tcl.h],
	    [tcl_headers_dir="/c/Tcl/include/${tcl_libname}"],
	    [tcl_header_search="no"])
      fi
      if test "$tcl_header_search" = "no"; then
      	 tcl_header_search=""
	 AC_CHECK_FILE([/d/Tcl/include/tcl/tcl.h],
	    [tcl_headers_dir="/d/Tcl/include/tcl"],
	    [tcl_header_search="no"])
      fi
      if test "$tcl_header_search" = "no"; then
      	 tcl_header_search=""
	 AC_CHECK_FILE([/d/Tcl/include/${tcl_libname}/tcl.h],
	    [tcl_headers_dir="/d/Tcl/include/${tcl_libname}"],
	    [tcl_header_search="no"])
      fi
   fi
   if test "$tcl_header_search" = "no"; then
      tcl_header_search=""
      AC_CHECK_FILE([/usr/include/tcl/tcl.h],
		[tcl_headers_dir="/usr/include/tcl"],
		[tcl_header_search="no"])
   fi
   if test "$tcl_header_search" = "no"; then
      tcl_header_search=""
      AC_CHECK_FILE([/usr/include/${tcl_libname}/tcl.h],
	    [tcl_headers_dir="/usr/include/${tcl_libname}"],
	    [tcl_header_search="no"])
   fi
   if test "$tcl_header_search" = "no"; then
      tcl_header_search=""
      AC_CHECK_FILE([/usr/local/include/tcl/tcl.h],
		[tcl_headers_dir="/usr/local/include/tcl"],
		[tcl_header_search="no"])
   fi
   if test "$tcl_header_search" = "no"; then
      tcl_header_search=""
      AC_CHECK_FILE([/usr/local/include/${tcl_libname}/tcl.h],
	    [tcl_headers_dir="/usr/local/include/${tcl_libname}"],
	    [tcl_header_search="no"])
   fi

   llibtcl=""
   if test "$ENV_CYGWIN" = "yes"; then
      tcl_lib_search=""
      AC_CHECK_FILE([/usr/lib/libtcl.a],
      		[tcl_extlib="/usr/lib/libtcl.a"],
      		[tcl_extlib="/usr/lib/libtclstub.a"])
      AC_CHECK_FILE([/usr/lib/libtcl.dll.a],
      		[tcl_extlib_add="/usr/lib/libtcl.dll.a"])
      tcl_extlib="$tcl_extlib $tcl_extlib_add"
      AC_MSG_CHECKING([tcl include libs in cygwin])
      AC_MSG_RESULT([${tcl_extlib}])
   elif test "$MINGW32" = "yes"; then
	tcl_extlib="/c/Tcl/lib/tcl85.lib"
   fi

   AC_SUBST(tcl_headers_dir)
   AC_SUBST(llibtcl)
   AC_SUBST(tcl_extlib)
   AC_SUBST(tcl_binary)
   AC_SUBST(tcl_config_shared)
   AC_SUBST(tcl_sitearch_dir)
   AC_SUBST(tcl_lib_dir)
fi

dnl GNU S(R): cygwin and Msys are not supported.
if test "$GNU_S" != "false"; then
   AC_CHECKING([gnu_R headers and libs])
   AC_ARG_WITH([gnu_r_sitearch_dir],
      [AS_HELP_STRING([--with-gnu-r-sitearch-dir],
	[where gnu R extension should go under $libdir (default system defined)])],
	[gnu_r_sitearch_dir=$withval],[gnu_r_sitearch_dir=NONE])

   test "$gnu_r_sitearch_dir" = NONE && gnu_r_sitearch_dir="${libdir}/R/site-library"

   gnu_s_headers_dir="/usr/share/R/include"
   llibgnu_s=`R CMD config LDFLAGS`
   gnu_s_extlib=""
   gnu_s_script="skf.R"
   if test "$ENV_CYGWIN" = "yes"; then
   	gnu_s_lib_dir="/usr/lib/R/lib"
   	llibgnu_s="-lR"
   fi

   AC_SUBST(gnu_s_headers_dir)
   AC_SUBST(llibgnu_s)
   AC_SUBST(gnu_s_extlib)
   AC_SUBST(gnu_s_script)
   AC_SUBST(gnu_s_binary)
   AC_SUBST(gnu_s_config_shared)
   AC_SUBST(gnu_r_sitearch_dir)
   AC_SUBST(gnu_s_lib_dir)
fi

AC_SUBST(perl_config_shared)
AC_SUBST(ruby_config_shared)
AC_SUBST(config_pos_indep)
AC_SUBST(deployment_target)

AC_SUBST(manpath_sitearch_dir)
AC_SUBST(darwin_shared)
AC_SUBST(SKFADDCFLG)
AC_SUBST(ENABLE_STRIP)
AC_SUBST(lineend_default)
AC_SUBST(lskfdir)
AC_SUBST(multiarch_header_name)
AC_SUBST(ac_exeext)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_CHECKING([various library environments])
AC_ISC_POSIX
AM_C_PROTOTYPES
AC_C_CONST
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(calloc setvbuf mkstemp mktemp tempnam bcopy setbuffer)
AC_FUNC_MALLOC

AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

dnl Checks for library functions.
AC_FUNC_SETVBUF_REVERSED

dnl for gettext
AM_GNU_GETTEXT
AC_CHECK_FUNC(gettext,,AC_CHECK_LIB(intl, gettext))
AC_CHECK_HEADERS("libintl.h")

msgfmt_skf=$MSGFMT

if test "$MSGFMT" = "false"; then
    msgfmt_skf="cat"
fi
AC_SUBST(msgfmt_skf)

localedir='${prefix}/share/locale'
AC_SUBST(localedir)

AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT
