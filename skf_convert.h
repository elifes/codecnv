/* ********************************************************************
**
** skf_convert.h: skf extension interfaces header
**
** Copyright (c) 2006-2015 Seiji Kaneko. All rights reserved.
** Everyone is permitted to use this program in source and binary
** form, with or without modification if and only if the following
** conditions are met:
** 1. Redistributions of source code must retain the above copyright
**   notice, copyright notice written in source code, additional list
**   of conditions and the following disclaimer.
** 2. Redistributions in machine readable form must reproduce the 
**   above copyright notice and the following disclaimer in the
**   documentation or other material provided with the distribution.
** 3. Neither the name of the copyright holders nor the names of its 
**   contributors may be used to endorse or promote products derived
**   from this software without specific prior written permission.
**********************************************************************
** Disclaimer: This software is provided and distributed AS IS, 
**	without any implicit or explicit warranties, and not
**	guaranteed to be error-free. In no event shall the author be
**	liable for any direct, indirect or incidental damages,
**	including, but not limited to, loss of data, use or profits
**	responsibility for any direct or indirect damages or results
**	arising by using whole or a part of this software.
******************************************************************* */
/* $Id: skf_convert.h,v 1.10 2015/04/26 15:08:38 seiji Exp seiji $ */

#include <setjmp.h>

/* typed string definitions. */
struct Skf_localestring {
	unsigned char 	*sstr;	/* input string value */
	int	codeset;
	int	lwl_codeset;
	int	length;
};

#define	LWL_MAXLEN	32768	/* FIXME	*/

/* --- perl swig --- */
#ifdef SWIGPERL
#undef	HAVE_FAST_LWLSTRLEN
#define skf_alloca(x,y)	alloca(sizeof(x)*y)
#define SKFSTRINGS	char
#define USE_FILE_OFFSET64
#define USE_LARGEFILE64
#endif

/* --- ruby swig --- */
#ifdef SWIGRUBY
#define SKFSTRING_IS_VALUE
#define	HAVE_FAST_LWLSTRLEN
#if defined(RSTRING_LEN)
#define get_rstr_len(x)	RSTRING_LEN(x)
#define get_rstr_ptr(x) RSTRING_PTR(x)
#define get_rstr_enc(x) skf_ruby_get_enc(x)
#define raw_get_rstr_enc(x) STR_ENC_GET(x)
#else
#define get_rstr_len(x)	RSTRING(x)->len
#define get_rstr_ptr(x) RSTRING(x)->ptr
#define get_rstr_enc(x) codeset_binary
#define raw_get_rstr_enc(x) "binary"
#endif

#define skf_alloca	ALLOCA_N

#define SKFSTRINGS	struct Skf_localestring
#define unwrap_skfstrings(x)	(x.sstr)

#else	/* !SWIGRUBY */
#define get_rstr_len(x)	skf_swig_strlen(x,LWL_MAXLEN)
#define get_rstr_enc(x)	codeset_binary
#define unwrap_skfstrings(x)	(x)

#define get_rstr_ptr(x)	x

#ifndef SKFSTRINGS
#define SKFSTRINGS	struct Skf_localestring
#endif
#endif	/* SWIGRUBY */

#define		skf_sleep	sleep

extern int	p_out_binary;
extern jmp_buf	skf_errbuf;

