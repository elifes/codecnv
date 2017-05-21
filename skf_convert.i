%module skf

%{
/* -------------------------------------------- */
/* SWIG interface C definitions			*/
/* -------------------------------------------- */
/* ********************************************************************
**
** skf_convert.i: skf extension interfaces
**
** Copyright (c) 2005-2015 Seiji Kaneko. All rights reserved.
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
**********************************************************************
** RUBY EXTENSION
** Usage: Skf.guess(option_string, string_to_convert) -- returns states
**	  Skf.convert(option_string, string_to_convert) -- conversion
**	  Skf.init(option_string, string_to_convert) -- initialize
**	  Skf.destruct(option_string, string_to_convert) -- free buffers
**
** skf options except -b,-u works as shown in documents. Environment
** variables skfenv and SKFENV is ignored regardless uid.
** conversion refers previous state hold inside skf. To suppress this
** behavior, use Skf.init explicitly.
******************************************************************* */
/* $Id: skf_convert.i,v 1.14 2017/01/05 15:05:48 seiji Exp seiji $ */

/* invocation for some ghosts */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

/* --- perl swig --- */
#ifdef SWIGPERL
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#endif

/* --- ruby swig --- */
#ifdef SWIGRUBY

#if defined(SKF_RUBY20)
#define SKF_RUBY2
#endif

/* 1.9-handling: HAS_RUBY_RUBY_H may be broken on Mac OS X */
#if	defined(SKF_RUBY19) || defined(SKF_RUBY20) || defined(SKF_RUBY21) || defined(SKF_RUBY22) || defined(SKF_RUBY23) || defined(SKF_RUBY24) || defined(SKF_RUBY25)
#include "ruby/ruby.h"
#include "ruby/encoding.h"
#else
#include "ruby.h"
#endif
#endif

/* --- php swig --- */
#ifdef SWIGPHP
#include "php.h"
#endif

/* --- python swig --- */
#ifdef SWIGPYTHON
#include "Python.h"
#define LWLINTSTRING 
const char *skf_notstring = "skf: not string\n";
#endif

#if defined(SKF_PYTHON3) && defined(SWIGPYTHON)
#include "unicodeobject.h"
#include "bytearrayobject.h"
#define	Py_SKFSTR	PyObject
#else	/* PYTHON2.x */
#define	Py_SKFSTR	char
#endif

/* --- others --- */

/* --- skf -------- */
#include "skf.h"
#include "skf_fileio.h"
#include "convert.h"
#include "oconv.h"

#include "skf_convert.h"

/* Note:
   maximum output size is ((input_string_length * 5) + 1) for skf (non-unic*de).
   maximum output size is ((input_string_length * 4) + 6) for skf (unic*de).
   maximum output size is ((input_string_length * 10) + 6) for skf (unic*de w. decomp).
   See skf_fileio.h and .c
*/
#define INCSIZE		256
#define GUESSSIZE	128
#define OPTSTRLEN	1024

#define NO_INIT		0
#define FORCE_INIT	1
#define GUESS_INIT	2

#define SKFNOERR	0
#define SKFINTERR	2

#ifdef __cplusplus
}
#endif

int	swig_state = 0;
int	skf_swig_result = 0;

int	errorcode = 0;		/* return error result condition   */
int	in_saved_codeset;
int	p_out_binary = 0;	/* currently python3 only variable */
struct Skf_localestring	lwlstr_b;
struct Skf_localestring	lwlopt_b;

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
int	ruby_out_locale_index = 0;
int	ruby_out_ascii_index = 0;

/* cygwin64 may need dummy main function */
#if	defined(__CYGWIN__)
int	main() 
{
}
#endif
#endif

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2) || (defined(SKF_PYTHON3) && defined(SWIGPYTHON))
#define SKFSTRUCT_RETURN
#endif

jmp_buf		skf_errbuf;

/* --------------------------------------------------------------- */
/* misc utilities						   */
/* --------------------------------------------------------------- */
void skf_exit(int eval)
{
    errorcode = skf_swig_result;
#if defined(SWIGRUBY)
    if (eval != EXIT_SUCCESS) rb_raise(rb_eSignal,"skf detected fatal error");
#endif
#if defined(SWIGPERL)
    if (eval != EXIT_SUCCESS) croak("skf detected fatal error");
#endif
    longjmp(skf_errbuf,SKFINTERR);
}

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
int get_rstr_enc(cname)
char *cname;
{
    return(skf_search_chname(cname));
}
#endif

#if	defined(SWIGPYTHON) && defined(SKF_PYTHON3)
void setsstrdummies(sstrdef)
struct Skf_localestring	*sstrdef;
{
    sstrdef->length = 0;	/* return dummies  */
    sstrdef->sstr = NULL;
    sstrdef->codeset = codeset_utf16be;
    sstrdef->lwl_codeset = -1;
}
#endif

/* --------------------------------------------------------------- */
/* we need function to calculate string length			   */
/* --------------------------------------------------------------- */
#if	defined(SWIGRUBY) && (defined(SKF_RUBY19) || defined(SKF_RUBY2))
size_t  skf_swig_strlen(str,maxlen)
@SKFSTRINGS@ *str;
int maxlen;
{
    int len;
    
    len = str->length;
    if (len > maxlen) len = maxlen;

    return(len);
}
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
size_t  skf_swig_strlen(str,maxlen)
@SKFSTRINGS@ *str;
int maxlen;
{
    size_t len;

    if (PyUnicode_Check(str)) {	/* if SANE object	   */
	len = PyUnicode_GetSize(str);
	if (len > maxlen) len = maxlen;
	return len;
    } else if (PyByteArray_Check(str)) {	/* if SANE object	   */
	len = PyByteArray_Size(str);
	if (len > maxlen) len = maxlen;
	return len;
    } else {
    	return (1);
    };

}
#else
size_t  skf_swig_strlen(str,maxlen)
@SKFSTRINGS@ *str;
int maxlen;
{
    int i;
    size_t len;

    for (i=0,len=0;((i<maxlen) && (*str!='\0')); i++,str++,len++);

    return(len);
}
#endif

/* --------------------------------------------------------------- */
/* input/output string conversion				   */
/* --------------------------------------------------------------- */
#if !defined(SKF_PYTHON3) || !defined(SWIGPYTHON)
static struct Skf_localestring	*istrdef;
#endif
static struct Skf_localestring	*ostrdef;
#if	!defined(SKF_RUBY19) && !defined(SKF_RUBY2) && (!defined(SKF_PYTHON3) || !defined(SWIGPYTHON))
static int optstr_len = 0;
#endif
static int iencode = -1;
/* --------------------------------------------------------------- */
/* output control for LWL interface				   */
/* --------------------------------------------------------------- */
int	skf_olimit = 0;
unsigned char	*skfobuf = NULL;

/* initialize */
void	skf_ioinit(skfoFILE *fout,int mode)
{

    skf_swig_result = 0;
    errorcode = 0;

/* --- output-side buffer prepare -------------------------------- */
    if (ostrdef == NULL) {
    	ostrdef = (struct Skf_localestring *)
			malloc(sizeof(struct Skf_localestring));
	if (ostrdef == NULL) {
	    skferr(SKF_OBUFERR,0,skf_olimit);
	    	/* should be break at this point */
	};
    } else;

    if (skfobuf == NULL) {
#ifdef SKFDEBUG
	if (is_v_debug) fprintf(stderr,"buffer allocation\n");
#endif
	skf_olimit = SKF_STRBUFLEN;
    	skfobuf = (unsigned char *)malloc(skf_olimit * sizeof(unsigned char));
	if (skfobuf == NULL) {
	    skferr(SKF_OBUFERR,0,skf_olimit);
	};
    };
    ostrdef->sstr = skfobuf;
    ostrdef->length = 0;
    ostrdef->codeset = out_codeset;

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    if ((mode == GUESS_INIT) || is_o_encode) {
    		/* return ascii for guess and ACE/MIMEs		   */
	ostrdef->lwl_codeset =
		rb_enc_find_index("US_ASCII");
    } else if (mode == FORCE_INIT) {
	ostrdef->lwl_codeset =
		rb_enc_find_index(i_codeset[out_codeset].cname);
    } else;	/* continuing.. do not initialize		   */
#else
    ostrdef->lwl_codeset = -1;
#endif
    if (o_add_bom) show_endian_out();
    if (add_annon) print_announce(out_codeset);
    show_lang_tag();

    return;
}

/* dummy initialize */
void	skf_dmyinit()
{

    skf_swig_result = 0;
    errorcode = 0;
/* --- output-side buffer prepare -------------------------------- */
    if (ostrdef == NULL) {
    	ostrdef = (struct Skf_localestring *)
			malloc(sizeof(struct Skf_localestring));
	if (ostrdef == NULL) {
	    skferr(SKF_OBUFERR,0,skf_olimit);
	    	/* should be break at this point */
	};
    } else;

    if (skfobuf == NULL) {
#ifdef SKFDEBUG
	if (is_v_debug) fprintf(stderr,"buffer allocation\n");
#endif
	skf_olimit = SKF_STRBUFLEN;
    	skfobuf = (unsigned char *)malloc(4 * sizeof(unsigned char));
	if (skfobuf == NULL) {
	    skferr(SKF_OBUFERR,0,skf_olimit);
	};
    };
    skfobuf[0] = ' ';
    skfobuf[1] = '\0';
    ostrdef->sstr = skfobuf;
    ostrdef->length = 1;
    ostrdef->codeset = out_codeset;

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    ostrdef->lwl_codeset =
		rb_enc_find_index("US_ASCII");
#else
    ostrdef->lwl_codeset = -1;
#endif

    return;
}

/* --------------------------------------------------------------- */
/* putchar for LWL interface					   */
/* --------------------------------------------------------------- */
int	lwl_putchar(c)
int	c;
 {
    unsigned char	*newbuf;

    if (ostrdef->length >= skf_olimit) {
#ifdef SKFDEBUG
	if (is_v_debug) fprintf(stderr,"buffer re-allocation\n");
#endif
	skf_olimit += BUFINCSIZE;
	newbuf = realloc(skfobuf, (skf_olimit * sizeof(unsigned char)));
	if (newbuf == NULL) {
	    skferr(SKF_OBUFREERR,0,skf_olimit);
	};
	skfobuf = newbuf;
	ostrdef->sstr = newbuf;
    };
    skfobuf[ostrdef->length] = c;
    ostrdef->length = ostrdef->length + 1;

    return(0);
}

#ifdef LWLINTSTRING
#if defined(SWIGPYTHON) && defined(SKF_PYTHON3)
/* --------------------------------------------------------------- */
/* this routine handles only optstr in a python extension	   */
/* --------------------------------------------------------------- */
char	*skfstrstrconv(PyObject *robj,size_t len)
{
    char	*dstr;
    Py_UNICODE	*srcstr;
    char	*bsrcstr;
    Py_ssize_t	slen;
    int		i;

#ifdef Py_UNICODE_WIDE
    dstr = calloc((len+2)*4,sizeof(char));
#else
    dstr = calloc((len+2)*2,sizeof(char));
#endif
    if (dstr == NULL) {
    	skferr(SKF_MALLOCERR,24,0);
    } else;
    srcstr = NULL; bsrcstr = NULL;

    if (PyUnicode_Check(robj)) {	/* if SANE object	   */
        if ((slen = PyUnicode_GetSize(robj)) <= 0) {
		/* may be just nullstring, not error.		   */
	    return(NULL);
	} else;
	if ((srcstr = PyUnicode_AsUnicode(robj)) == NULL) {
		/* LIMITED_API: which should I use?		   */
	    skferr(SKF_IBUFERR,0,0);
	} else;
	if (slen > len) slen = len;
		/* do not process too long string.		   */
	for (i=0;i<slen;i++) {
	    if (srcstr[i] >= 0x7f) break;
	    dstr[i] = srcstr[i];
	};
    } else if (PyByteArray_Check(robj)) { /* if SANE object	   */
        if ((slen = PyByteArray_Size(robj)) <= 0) {
	    return(NULL);
	} else;
	if ((bsrcstr = PyByteArray_AsString(robj)) == NULL) {
	    skferr(SKF_IBUFERR,0,0);
	} else;
	if (slen > len) slen = len;
	for (i=0;i<(slen);i++) {
	    dstr[i] = bsrcstr[i];
	};

        dstr[slen] = '\0';

    };
    return(dstr);
}
#else
/* --------------------------------------------------------------- */
char	*skfstrstrconv(@SKFSTRINGS@ *robj,int len)
{
    char	*dstr;
    int		i;

    dstr = calloc(len+1,sizeof(char));
    if (dstr == NULL) {
    	skferr(SKF_MALLOCERR,24,1);
    } else;
    for (i=0;i<len;i++) dstr[i] = (char) robj[i];
    robj[len] = '\0';

    return(dstr);
}
#endif
#endif

#if	defined(SWIGRUBY) && (defined(SKF_RUBY19) || defined(SKF_RUBY2))
/* --------------------------------------------------------------- */
struct Skf_localestring *skf_rbstring2skfstring(VALUE rstr)
{
    struct Skf_localestring	*sstrdef;


    if ((sstrdef = calloc(1,sizeof(struct Skf_localestring))) == NULL) {
    	skferr(SKF_MALLOCERR,24,2);
    } else {
	sstrdef->sstr = (unsigned char *)RSTRING_PTR(rstr);
	sstrdef->length = RSTRING_LEN(rstr);
	sstrdef->codeset = 
	    skf_search_cname((char *)rb_enc_name(rb_enc_get(rstr)));
	sstrdef->lwl_codeset = -1;	/* do not assume input locale */
    };

    istrdef = sstrdef;
    return sstrdef;
}

#elif	defined(SWIGPYTHON) && defined(SKF_PYTHON3)
	/* Note: Python string comes with UCS2(BMP only) or UCS4.
	   The width depends on compile option, which is described below. */
/* --------------------------------------------------------------- */
struct Skf_localestring *skf_pystring2skfstring(PyObject *robj)
{
    struct Skf_localestring	*sstrdef;
    Py_ssize_t len;
    Py_UNICODE	*instr = NULL;
    char	*instr_c = NULL;
    unsigned char	*istr;
    int		i;
    int		pseudecode = 1;

    if ((sstrdef = calloc(1,sizeof(struct Skf_localestring))) == NULL) {
    	skferr(SKF_MALLOCERR,24,3);
    } else {
    	/* TODO: distinguish byte array and string(UCS2)	   */
    	if (PyUnicode_Check(robj)) {	/* if SANE object	   */
	/* END TODO */
	/* in case we have string */
	    if (PyUnicode_Check(robj)) {/* if SANE object	   */
		if ((len = PyUnicode_GetSize(robj)) <= 0) {
#ifdef SKFDEBUG
		    if (is_v_debug) in_undefined(SKF_UNEXPEOF,0);
#endif
		    setsstrdummies(sstrdef);	/* return dummy    */
		    return(sstrdef);
		} else;
		if ((instr = PyUnicode_AsUnicode(robj)) == NULL) {
		    skf_openerr("",7);
		    setsstrdummies(sstedef);
		    return(sstrdef);
		} else;
		for (i=0; i< len;i++) {
		    if (*(instr+i) >= 0x100) {
			pseudecode = 0;
			break;
		    } else;
		};
		if (pseudecode == 1) {
		    /* pseude code. packed 8-bit asciis.		   */
		    if ((istr = 
		       calloc((len + 2),sizeof(unsigned char))) == NULL) {
			PyErr_NoMemory();
			return(sstrdef);	  /*NOTREACHED*/
		    } else;
	    
		    for (i=0;i<len;i++) {
			*(istr + i) = (unsigned char) (*(instr + i) & 0x0ffu);
		    };
		    sstrdef->length = len;
			    /* Note: codeset is not yet determined */
		    sstrdef->sstr = istr;
		    sstrdef->codeset = codeset_utf16be;
		    sstrdef->lwl_codeset = -1;	/* do not assume input locale */
		} else {
			/* 2 is for margin. 4 is for BOM */
		    if ((istr = calloc((len)*4 + 4 + 4,
				       sizeof(unsigned char))) == NULL) {
			PyErr_NoMemory();
			return(sstrdef);	  /*NOTREACHED*/
		    } else;
		    *(istr) = 0x00; *(istr + 1) = 0x00; /* generate BOM */
		    *(istr + 2) = 0xfe; *(istr + 3) = 0xff;

		    /* packed 	*/
		    for (i=0;i<len;i++) {
#ifdef Py_UNICODE_WIDE		/* UCS4 is used			   */
			*(istr + i*4 + 4) =
			    (unsigned char) (((*(instr + i)) >> 24) & 0x0ffU);
			*(istr + i*4 + 5) =
			    (unsigned char) (((*(instr + i)) >> 16) & 0x0ffU);
#else
			*(istr + i*4 + 4) = 0;
			*(istr + i*4 + 5) = 0;
#endif
			*(istr + i*4 + 6) =
			    (unsigned char) (((*(instr + i)) >> 8) & 0x0ffU);
			*(istr + i*4 + 7) =
			    (unsigned char) (((*(instr + i)) >> 0) & 0x0ffU);
		    };
		    sstrdef->length = len * 4 + 4;
			    /* codeset is UTF-32BE */
		    in_codeset = codeset_utf32benb;
		    sstrdef->sstr = istr;
		    sstrdef->codeset = codeset_utf32be;
		    sstrdef->lwl_codeset = -1;	/* do not assume input locale */
		};
	    } else;
    	} else if (PyByteArray_Check(robj)) {	/* if SANE object	   */
	    if ((len = PyByteArray_Size(robj)) <= 0) {
		setsstrdummies(sstrdef);	/* return dummy    */
		return(sstrdef);
	    } else;
	    if ((instr_c = PyByteArray_AsString(robj)) == NULL) {
		skferr(SKF_IBUFERR,0,0);
	    } else;
	    /* we have 8-bit asciis.			   */
	    if ((istr = 
	       calloc((len + 2),sizeof(unsigned char))) == NULL) {
		PyErr_NoMemory();
		return(NULL);		/*NOTREACHED*/
	    } else;
    
	    for (i=0;i<len;i++) {
		*(istr + i) = (unsigned char) (*(instr_c+ i) & 0x0ffu);
	    };
	    sstrdef->length = len;
		    /* Note: codeset is not yet determined */
	} else {	/* we got neither string nor byte array		  */
		/* we got a byte array */
	    PyErr_SetString(PyExc_RuntimeError,skf_notstring);
	};
    };

    return sstrdef;
}

PyObject *skf_skfstring2pystring(struct Skf_localestring *robj,
	int out_codeset, int p_out_binary)
{
    long	olen,oolen;
    Py_UNICODE	*uary;
    int	i,j;
    unsigned char *oostr;
    PyObject *presult;

    oolen = (olen = robj->length);
    oostr = robj->sstr;
    if (olen < 0) olen = 8;

#ifdef Py_UNICODE_WIDE
    uary = calloc(olen + 8,sizeof(long));
#else
    uary = calloc(olen + 8,sizeof(int));
#endif
    if (uary == NULL) {
	skferr(SKF_MALLOCERR,24,4);
    } else;

    /* Note: if memory is so short that even 8 byte can't be allocated, */
    /*  skf may crash internally. */
    /* oostr may be leaked. TODO; check. */
    
    if (p_out_binary == 0) {
	if (o_encode == codeset_utf16be) {
	    j = 0;
	    for (i=2;i<oolen;i+=2) {
		uary[j++] = (oostr[i] << 8) + oostr[i+1];
	    };
	    uary[j] = 0;
	    presult = PyUnicode_FromUnicode(uary,j+1);
	} else if (o_encode == codeset_utf16le) {
	    j = 0;
	    for (i=2;i<oolen;i+=2) {
		uary[j++] = (oostr[i+1] << 8) + oostr[i];
	    };
	    uary[j] = 0;
	    presult = PyUnicode_FromUnicode(uary,j+1);
	} else {
	    presult = PyUnicode_FromStringAndSize((const char *)oostr,
	    			(Py_ssize_t)oolen);
	};
    } else {
	presult = PyUnicode_FromStringAndSize((const char *)oostr,
				(Py_ssize_t)oolen);
    };
    free (uary);

    return(presult);
}
#else
/* --------------------------------------------------------------- */
struct Skf_localestring *skf_lwlstring2skfstring(char *rstr)
{
    struct Skf_localestring	*sstrdef;
    int i;
    char *pstr = rstr;
    unsigned char	 *istr;
    int	 buflen = LWL_MAXLEN;

    if (istrdef == NULL) {
	if ((sstrdef = calloc(1,sizeof(struct Skf_localestring))) == NULL) {
	    skferr(SKF_MALLOCERR,24,5);
	} else;
    } else {
    	sstrdef = istrdef;
    };
    if ((sstrdef->sstr) == NULL) {
	if ((istr = calloc(LWL_MAXLEN,sizeof(unsigned char))) == NULL) {
	/* Note: Maybe I should free istr at this point, but do nothing. */
	    skferr(SKF_MALLOCERR,24,6);
	} else;
    } else {
    	istr = sstrdef->sstr;
    };

    for (i=0;((i<LWL_MAXLEN) && (*pstr!='\0')); i++,pstr++) {
	istr[i] = (*pstr) & 0xff;
	if (i >= (buflen - 2)) {
	    istr = realloc(istr,(size_t)(sizeof(int) *(buflen * 2)));
	    if (istr == NULL) {
		skferr(SKF_MALLOCERR,24,7);
	    } else;
	    break;
	} else;
    };

    istr[i] = sEOF;
    sstrdef->sstr = istr;
    sstrdef->length = i;
    sstrdef->codeset = -1;
    sstrdef->lwl_codeset = -1;	/* do not assume input locale */

    istrdef = sstrdef;
    return sstrdef;
}

#endif
/* ---------------------------------------------------------------
    common output routine.
    output buffer should be prepared before calling.
*/
static void r_skf_convert(struct Skf_localestring *lstr, long ibuflen,
				int mode,int ienc)
{
    int sy;
    int errc = 0;

    errorcode = 0;	/* forget previous errors.		   */
/* --- codeset specific initialization is done in convert -------- */
/* --- preparing output buffers ---------------------------------- */
    skf_ioinit((skfoFILE *) 0,mode);

/* --- preconversion output prepare ------------------------------ */
    if (o_add_bom) show_endian_out();
    if (add_annon) print_announce(out_codeset);

/* --- compatibility various hooks ------------------------------- */
#ifdef SKF196COMPAT
    if (encode_enbl) set_skf196mime;
#endif
    if (is_nkf_compat) {
    	mime_fold_llimit -= 1;
	if (!encode_enbl) set_mimeb_encode;
    };

    reset_kanji_shift;
/* --- codeset specific initialization, include table setting ---- */
    skf_output_table_set();

/* --- reflect in_param into conv_cap -------------------------- */
    if (is_set_ad_ann && !is_ucs_ufam(conv_cap)) set_add_annon;
    else if (is_set_no_ann && !is_ucs_ufam(conv_cap)) res_add_annon;
    else;

    if (is_set_p_iso8859 && !is_ucs_ufam(conv_cap)) set_use_iso8859;
    else if (is_set_p_niso8859 && !is_ucs_ufam(conv_cap)) res_use_iso8859;
    else;

    if (is_par_hk_enbl && !is_ucs_ufam(conv_cap)) {
	set_put_hk_enbl(get_par_hkenbl);
    };

    if (is_ucs_utf16(conv_cap)) {
    if (is_set_out_le && is_ucs_utf16(conv_cap)) set_o_ltl_endian;
    else if (is_set_out_be && is_ucs_utf16(conv_cap)) set_o_big_endian;
    else;
    } else;

    if (is_set_ucs_bom && is_ucs_ufam(conv_cap)) set_o_add_bom;
    else if (is_set_ucs_nobom && is_ucs_ufam(conv_cap)) res_o_add_bom;
    else ;

    if (is_set_mscpt) set_use_ms_compat;
    else if (is_set_no_mscpt) res_use_ms_compat;
    else;

    if (is_set_pux212) set_use_x0212;
    else if (is_set_pnux212) res_use_x0212;
    else;

/* --- output initialize ----------------------------------------- */
    if (oconv_init() < 0) {	/* various initialize		   */
	skferr(SKF_MALLOCERR,(long)24,(long)16);
    } else;

    init_all_stats();

/* pass parameter to ioinit input side	-------------------------- */
    stdibuf = lstr->sstr;
    buf_p = lstr->length;

#ifdef SKFDEBUG
    if (is_vv_debug) {
	fprintf(stderr,"#buf_p:%ld#",buf_p);
    } else;
#endif

    if (mode == FORCE_INIT) {
	if (o_add_bom) show_endian_out();
	show_lang_tag();

/* --- preconversion output prepare ------------------------------ */
	if (add_annon) print_announce(out_codeset);

/* --- fold value fix -------------------------------------------- */
#ifdef FOLD_SUPPORT
	fold_value_setup();
#endif
    };
/* --- debug analyze ---- */
#ifdef SKFDEBUG
    debug_analyze();
#endif
/* --- open provided strings ------------------------------------- */
/* Note: this point messed up int and long. Be warned.		   */

    sy = ibuflen;
    if (sy > ibuflen) {
	errorcode = 1;
	skferr(SKF_DEBUGERR_2,(long)0,(long)0);
    };

    skf_fpntr = 0;
    Qflush();

    in_codeset = ienc;	/* this may be blunder. Go anyway	   */

/* --- conversion loop ------------------------------------------- */
    if ((errc = setjmp(skf_errbuf)) == 0) {
	sy = skf_in_converter((FILE *)0);
    } else {
#if defined(SKF_PYTHON3) && defined(SWIGPYTHON)
	if (errc == 1) PyErr_NoMemory();
	else;
#else
	;
#endif
    };

    in_saved_codeset = in_codeset;

    if (is_jis(conv_cap)) JIS_finish_procedure();
    if (is_euc(conv_cap)) EUC_finish_procedure();
    if (is_msfam(conv_cap)) SJIS_finish_procedure();
    if (is_ucs_utf7(conv_cap)) utf7_finish_procedure();
    if (is_ucs_utf8(conv_cap)) utf8_finish_procedure();
    if (is_ucs_utf16(conv_cap)) ucod_finish_procedure();
    if (out_bg(conv_cap)) BG_finish_procedure();
    if (is_ucs_brgt(conv_cap)) BRGT_finish_procedure();
#ifdef SKFDEBUG
    if (is_v_debug) fprintf(stderr,"\n[EOF]\n");
#endif

#if defined(FOLD_SUPPORT) 
/* folding at the end of file					   */
    if (is_nkf_compat && (fold_count != 0) && fold_fclap && !notrunc_le) {
#ifdef SKFDEBUG
	if (is_vv_debug) { fprintf(stderr,"#LEADD#");
	} else;
#endif
    	SKFCRLF();
    } else;
#endif

/* if any characters hang in encoder, push out these stuffs. 	   */
/* and also put eol if base64 encode.				   */
    oconv(sFLSH);  /* remains should be cleared	   */

    if (o_encode) encoder_tail();

    if (skf_swig_result == 0) skf_swig_result = errc;

    return;
}

/*
    routines visible from (i.e. provided to) scripting languages
    convert, quickconvert, guess
*/
@SKFOSTRINGS@ *convert(@SKFCSTRINGS@ *optstr, @SKFSTRINGS@ *cstr)
{
    long	ibuflen;
    int		result;
    struct	Skf_localestring *lwlstr = &lwlstr_b;
#if defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    struct	Skf_localestring *lwlopt = &lwlopt_b;
#endif
#if defined(FUTURESUPPORT)
    int		errc = 0;
#endif

    in_saved_codeset = -1;
    p_out_binary = 0;

    if (swig_state == 0) {
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"\nextension initialize\n");
#endif
	skf_script_init(); swig_state = 1;
    };

#if	defined(SWIGRUBY) && (defined(SKF_RUBY19) || defined(SKF_RUBY2))
    lwlstr = cstr;
    ibuflen = get_rstr_len(cstr);
#elif	defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    lwlstr = skf_pystring2skfstring(cstr);
    ibuflen = lwlstr->length;
#else
    lwlstr = skf_lwlstring2skfstring(cstr);
    ibuflen = lwlstr->length;
#endif

#ifdef DEBUG
    if (is_vv_debug) {
    	fprintf(stderr,"inputside-len%d,ptr:%lx\n",cstr->length,cstr->sstr)
    } else;
#endif

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    if (optstr->sstr != NULL) {
	result = skf_script_param_parse((char *)(optstr->sstr),optstr->length);
    } else result = 0;
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    lwlopt = skf_pystring2skfstring(optstr);
    if ((lwlopt != NULL) && (lwlopt->sstr != NULL)) {
	result = skf_script_param_parse((char *)lwlopt->sstr,lwlopt->length);
	free(lwlopt->sstr);
    } else result = 0;
#else
    if (optstr != NULL) result = skf_script_param_parse((char *)optstr,optstr_len);
    else result = 0;
#endif

    if (result < 0) {
#if	defined(SKFSTRUCT_RETURN)
	skf_dmyinit();	/* generate dmy ostrdef			   */
#endif
    } else {

	iencode = in_codeset;

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
	ruby_out_locale_index = rb_enc_find_index(i_codeset[out_codeset].cname);
	iencode = cstr->codeset;
#endif
    /* --- conversion call ------------------------------------------- */
	r_skf_convert(lwlstr,ibuflen,FORCE_INIT,iencode);
	lwl_putchar(0x00);	/* add terminater			   */
	errorcode = skf_swig_result;

#ifdef DEBUG
	if (is_vv_debug) {
#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
	    fprintf(stderr,"\nswig_result-len:%d,ptr:%lx\n",ostrdef->length,ostrdef->sstr);
#else
	    fprintf(stderr,"\nswig_result-len:%d,ptr:%lx\n",ostrdef->length,cstr);
#endif
	};

#endif
    };

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    if (cstr != NULL) free(cstr);
    return (ostrdef);
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    ostrdef->sstr = (unsigned char *)skfobuf;
    ostrdef->length = ostrdef->length;
    return(skf_skfstring2pystring(ostrdef,out_codeset,p_out_binary));
#else
    return ((char *)skfobuf);
#endif
}

@SKFOSTRINGS@ *quickconvert(@SKFCSTRINGS@ *optstr, @SKFSTRINGS@ *cstr)
{
    long	ibuflen;
    int		result = 0;
    struct	Skf_localestring *lwlstr = &lwlstr_b;
#if defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    struct	Skf_localestring *lwlopt = &lwlopt_b;
#endif

    if (swig_state == 0) {
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"\nextension initialize\n");
#endif
	skf_script_init(); swig_state = 1;
    };
    debug_opt = 0;

#if	defined(SWIGRUBY) && (defined(SKF_RUBY19) || defined(SKF_RUBY2))
    lwlstr = cstr;
    ibuflen = get_rstr_len(cstr);
#elif	defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    lwlstr = skf_pystring2skfstring(cstr);
    ibuflen = lwlstr->length;
#else
    lwlstr = skf_lwlstring2skfstring(cstr);
    ibuflen = lwlstr->length;
#endif

    lwlstr->codeset = in_saved_codeset;

#if	defined(SWIGRUBY) && (defined(SKF_RUBY19) || defined(SKF_RUBY2))
    if (optstr->sstr != NULL) {
	result = skf_script_param_parse((char *)(optstr->sstr),optstr->length);
    } else result = 0;
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    lwlopt = skf_pystring2skfstring(optstr);
    if ((lwlopt != NULL) && (lwlopt->sstr != NULL)) {
	result = skf_script_param_parse((char *)lwlopt->sstr,lwlopt->length);
	free(lwlopt->sstr);
    } else result = 0;
#else
    if (optstr != NULL) result = skf_script_param_parse((char *)optstr,optstr_len);
    else result = 0;
#endif

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    if (optstr->sstr != NULL) {
	result = skf_script_param_parse((char *)(optstr->sstr),optstr->length);
    } else result = 0;
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    lwlopt = skf_pystring2skfstring(optstr);
    if ((lwlopt != NULL) && (lwlopt->sstr != NULL)) {
	result = skf_script_param_parse((char *)lwlopt->sstr,lwlopt->length);
	free(lwlopt->sstr);
    } else result = 0;
#else
    if (optstr != NULL) result = skf_script_param_parse((char *)optstr,optstr_len);
    else result = 0;
#endif

    if (result < 0) {
#if	defined(SKFSTRUCT_RETURN)
	skf_dmyinit();	/* generate dmy ostrdef			   */
#endif
    } else {

	iencode = in_codeset;

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
	ruby_out_locale_index = rb_enc_find_index(i_codeset[out_codeset].cname);
	iencode = cstr->codeset;
#endif
    /* --- conversion call ------------------------------------------- */
	r_skf_convert(lwlstr,ibuflen,FORCE_INIT,iencode);
	lwl_putchar(0x00);	/* add terminater			   */
	errorcode = skf_swig_result;

#ifdef DEBUG
	if (is_vv_debug) {
#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
	    fprintf(stderr,"\nswig_result-len:%d,ptr:%lx\n",ostrdef->length,ostrdef->sstr);
#else
	    fprintf(stderr,"\nswig_result-len:%d,ptr:%lx\n",ostrdef->length,cstr);
#endif
	};

#endif
    };

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    if (cstr != NULL) free(cstr);
    return (ostrdef);
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    ostrdef->sstr = (unsigned char *)skfobuf;
    ostrdef->length = ostrdef->length;
    return(skf_skfstring2pystring(ostrdef,out_codeset,p_out_binary));
#else
    return ((char *)skfobuf);
#endif
}

@SKFOSTRINGS@ *guess(@SKFCSTRINGS@ *optstr, @SKFSTRINGS@ *cstr)
{
    long	ibuflen;
    int		result;
    struct	Skf_localestring *lwlstr = &lwlstr_b;
#if defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    struct	Skf_localestring *lwlopt = &lwlopt_b;
#endif
    skf_script_init();		/* guess does not use old value	   */
    in_saved_codeset = -1;

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    lwlstr = cstr;
    ibuflen = get_rstr_len(cstr);
#elif	defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    lwlstr = skf_pystring2skfstring(cstr);
    ibuflen = lwlstr->length;
#else
    lwlstr = skf_lwlstring2skfstring(cstr);
    ibuflen = lwlstr->length;
#endif

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    if (optstr->sstr != NULL) {
	result = skf_script_param_parse((char *)(optstr->sstr),optstr->length);
    } else result = 0;
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    lwlopt = skf_pystring2skfstring(optstr);
    if ((lwlopt != NULL) && (lwlopt->sstr != NULL)) {
	result = skf_script_param_parse((char *)lwlopt->sstr,lwlopt->length);
	free(lwlopt->sstr);
    } else result = 0;
#else
    if (optstr != NULL) result = skf_script_param_parse((char *)optstr,optstr_len);
    else result = 0;
#endif

    if (result < 0) {
#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
	skf_dmyinit();	/* generate dmy ostrdef			   */
	return (ostrdef);
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
	skf_dmyinit();	/* generate dmy ostrdef			   */
	return(skf_skfstring2pystring(ostrdef,out_codeset,p_out_binary));
#else
	return ((char *)skfobuf);
#endif
    };
    set_input_inquiry;
    iencode = in_codeset;

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    ruby_out_ascii_index = rb_enc_find_index("US_ASCII");
    iencode = cstr->codeset;
#endif
/* --- conversion call ------------------------------------------- */
    r_skf_convert(lwlstr,ibuflen,FORCE_INIT,iencode);
    lwl_putchar(0x00);	/* add terminater			   */
    errorcode = skf_swig_result;

#ifdef DEBUG
    if (is_vv_debug) {
#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    	fprintf(stderr,"\nswig_result-len:%d,ptr:%lx\n",ostrdef->length,ostrdef->sstr);
#else
	fprintf(stderr,"\nswig_result-len:%d,ptr:%lx\n",ostrdef->length,cstr);
#endif
    };

#endif

#if	defined(SKF_RUBY19) || defined(SKF_RUBY2)
    if (cstr != NULL) free(cstr);
    return (ostrdef);
#elif defined(SWIGPYTHON) && defined(SKF_PYTHON3)
    ostrdef->sstr = (unsigned char *)skfobuf;
    ostrdef->length = ostrdef->length;
    return(skf_skfstring2pystring(ostrdef,out_codeset,p_out_binary));
#else
    return ((char *)skfobuf);
#endif
}

void destruct()
{
    skf_script_init();
    if (skfobuf != NULL) free(skfobuf);
    skfobuf = NULL;
}

char *inputcode()
{
    return(i_codeset[in_codeset].cname);
}

%}

%import "skf_convert.h"
%newobject convert;
%newobject quickconvert;
%newobject guess;
%newobject inputcode;
%immutable cstr;
%immutable optstr;

#if defined(SWIGRUBY)
%typemap (in) (SKFSTRINGS *cstr) {
	$1 = skf_rbstring2skfstring($input);
}

%typemap (in) (SKFSTRINGS *optstr) {
	$1 = skf_rbstring2skfstring($input);
}

%typemap (out) (SKFSTRINGS *) {
    {
	VALUE res;
	int	i;
	char	*resstr;
	unsigned char	*oostr;
	long	olen,oolen;

	oolen = (olen = $1->length);
	if (olen < 0) olen = 8;

	/* olen includes terminate \0. ruby does not expect this value, so */
	/* we have to fix up the return value.				   */
	olen--;

	res = rb_str_new(0, olen + 5);
	rb_str_set_len(res,olen);
	resstr = RSTRING_PTR(res);
	oostr = $1->sstr;
	if (o_encode) {
	    rb_enc_associate(res, rb_usascii_encoding());
	} else {
	    rb_enc_associate(res,
	      rb_enc_from_index(rb_enc_find_index(i_codeset[out_codeset].cname)));
	};
	for (i=0;i<$1->length;i++) {
	    if (oolen < 0) {
	    	*(resstr++) = ' ';
	    } else {
		*(resstr++) = *(oostr++);
	    };
	};
	$result = res;
    };
}
#endif

#if defined(SKF_PYTHON3) && defined(SWIGPYTHON)
%typemap (in) (@SKFSTRINGS@ *cstr) {
	$1 = skf_pystring2skfstring($input);
}

%typemap (in) (@SKFSTRINGS@ *optstr) {
	$1 = skf_pystring2skfstring($input);
}

#endif

/* module definitions */
/* %apply @SKFSTRINGS@ *INPUT { @SKFSTRINGS@ *cstr }; */
@SKFOSTRINGS@ *convert(@SKFCSTRINGS@ *optstr, @SKFSTRINGS@ *cstr);
@SKFOSTRINGS@ *quickconvert(@SKFCSTRINGS@ *optstr, @SKFSTRINGS@ *cstr);
@SKFOSTRINGS@ *guess(@SKFCSTRINGS@ *optstr, @SKFSTRINGS@ *cstr);
@SKFOSTRINGS@ *inputcode();

/* %rename(init) skf_script_init; */
void skf_script_init();
void destruct();

int  in_codeset;
int  out_codeset;
int  errorcode;
