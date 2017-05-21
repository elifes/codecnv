/* *******************************************************************
** Copyright (c) 1997-2015 Seiji Kaneko. All rights reserved.
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
** Disclaimer: This software is provided and distributed AS iS, 
**	without any implicit or explicit warranties, and not
**	guaranteed to be error-free. In no event shall the author be
**	liable for any direct, indirect or incidental damages,
**	including, but not limited to, loss of data, use or profits
**	responsibility for any direct or indirect damages or results
**	arising by using whole or a part of this software.
**********************************************************************
** Note: do not detect arib-jis here, because arib uses hold_buf for
**       different purposes.
**********************************************************************
    preconvert.c	code detector and converter
    $Id: preconvert.c,v 1.123 2017/01/05 15:05:48 seiji Exp seiji $
*/

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include "skf.h"
#include "skf_fileio.h"
#include "convert.h"
#include "oconv.h"

#ifdef SKFDEBUG
#define set_rescase(x)	rescase = x
#define reset_rescase() rescase = 0
#define debug_fault(x,y)  ___debug_fault(x,y)
#else
#define set_rescase(x)	;
#define reset_rescase() ;
#define debug_fault(x,y)  ;
#endif

#define	is_det_jis  is_jis(i_codeset[in_codeset].encode)
#define	is_det_ms   is_msfam(i_codeset[in_codeset].encode)
#define	is_det_euc  is_euc(i_codeset[in_codeset].encode)
#define	is_det_euc7 is_euc7(i_codeset[in_codeset].encode)
#define	is_det_ucod is_ucs_utf16(i_codeset[in_codeset].encode)
#define	is_det_utf8 is_ucs_utf8(i_codeset[in_codeset].encode)
#define	is_det_utf7 is_ucs_utf7(i_codeset[in_codeset].encode)
#define	is_det_keis is_keis(i_codeset[in_codeset].encode)
#define	is_det_latin is_jis(i_codeset[in_codeset].encode)
#define	is_det_brgt is_ucs_brgt(i_codeset[in_codeset].encode)
#define	is_det_transp is_ucs_transp(i_codeset[in_codeset].encode)

#define is_code_undefine (in_codeset < 0) 
#define is_lt_undefine (le_detect == 0)

#define is_lang_japanese ((skf_input_lang == L_NU) || \
	(skf_input_lang == L_JP) || (skf_input_lang == M_JP) || \
	(skf_input_lang == L_UNI) || (skf_input_lang == 0))
#define is_lang_korian ((skf_input_lang == L_NU) || \
	(skf_input_lang == L_KO) || (skf_input_lang == M_KO) || \
	(skf_input_lang == L_UNI) || (skf_input_lang == 0))
#define is_lang_chinese ((skf_input_lang == L_NU) || \
	(skf_input_lang == L_ZH) || (skf_input_lang == M_ZH) || \
	(skf_input_lang == L_UNI) || (skf_input_lang == 0))
#define is_lang_unicode ((skf_input_lang == L_NU) || \
	(skf_input_lang != L_NUN) || (skf_input_lang == 0))
#define is_lang_vietnamese ((skf_input_lang == L_NU) || \
	(skf_input_lang == L_VI) || (skf_input_lang == 0))
#define is_lang_latin ((skf_input_lang == L_NU) || \
	(skf_input_lang == L_EN) || (skf_input_lang == L_FR) || \
	(skf_input_lang == L_DE) || (skf_input_lang == 0))
#define in_var_shift	{c4 =c3; c3=c2; c2=c1;}
#define in_var_unshift	{c1 =c2; c2=c3; c3=c4;}
#define in_var_clear	{c4 =sEOF; c3 =sEOF; c2 =sEOF; c1 =sEOF;}

#define may_euc		((eucfault == 0) || (krfault == 0) \
			 || (gbfault == 0) )

/* Unicode table area paraphrase */
#define is_diaupalpha(x) ((x >= 0xc0) && (x <= 0xdd) && (x != 0xd7))
#define is_dialwalpha(x) ((x >= 0xdf) && (x <= 0xff) && (x != 0xf7))
#define is_diaalpha(x) ((x >= 0xc0) && (x <= 0xff) && (x != 0xf7) \
			&& (x != 0xd7) && (x != 0xde))
#define is_latglyph(x)	((x >= 0xa1) && (x <= 0xbf))
#define is_igreek(x)	((x >= 0x0386) && (x <= 0x03d6))
#define is_icyrl(x)	((x >= 0x0401) && (x <= 0x045f))
#define is_ihebrw(x)	((x >= 0x05d0) && (x <= 0x05ea))
#define is_iarab(x)	((x >= 0x0621) && (x <= 0x06f9))
#define is_ithai(x)	((x >= 0x0e01) && (x <= 0x0e74))
#define is_ihirak(x)	((x >= 0x3041) && (x <= 0x3094))
#define is_ikatak(x)	((x >= 0x30a1) && (x <= 0x30f6))
#define is_ikdak(x)	((x == 0x3099) || (x == 0x309b))
#define is_ikhdak(x)	((x == 0x309a) || (x == 0x309c))
#define is_itouten(x)	(x == 0x3001)
#define is_ikuten(x)	(x == 0x3002)
#define	is_ikanji(x)	((x >= 0x4e00) && (x < 0xa000))
#define is_ihangul(x)	((x >= 0xac00) && (x <= 0xd7a3))
#define is_icjkcomp(x)	((x >= 0xf900) && (x <= 0xfa7f))
#define is_ifwlatin(x)	((x >= 0xff01) && (x <= 0xff5e))
#define is_ihkana(x)	((x >= 0xff66) && (x <= 0xff9e))
#define is_kanabra(x)	(((x & 0xfff9) == 0x3008) || (x == 0x3010) \
 || (x == 0x3014) || (x == 0x3016) || (x == 0x3018) || (x == 0x301a))
#define is_kanaket(x)	(((x & 0xfff9) == 0x3009) || (x == 0x3011) \
 || (x == 0x3015) || (x == 0x3017) || (x == 0x3019) || (x == 0x301b))
#define is_kanarep(x) (((x >= 0x3031) && (x <= 0x3035)) || (x == 0x303d))
#define is_smallkana(x) ((x >= 0x30f0) && (x <= 0x30ff))
#define is_kanakuten(x) (x == 0x3002)
#define is_kanatoten(x) (x == 0x3001)
#define is_leader(x)	((x == 0x2024) || (x == 0x2025) || (x == 0x2026))
#define is_middot(x)	(x == 0x00b7)
#define is_h_kbra(x)	(x == 0xff62)
#define is_h_kket(x)	(x == 0xff63)
#define is_h_mdot(x)	(x == 0xff65)
#define is_h_kuten(x)	(x == 0xff61)
#define is_h_toten(x)	(x == 0xff64)
#define is_h_dakon(x)	(x == 0xff9e)
#define is_h_haton(x)	(x == 0xff9f)
#define is_h_kutot(x)	((x >= 0xff61) && (x <= 0xff65))
#define is_h_dot(x)	(x == 0xff0e)
#define is_h_comma(x)	(x == 0xff0c)
#define is_gb_olang(x)	((x >= 282) && (x < 658)) /* kana & cyl  */
#define is_kr_olang(x)  (((x >= 564) && (x < 658)) \
	((x >= 846) && (x < 1128)))

#define is_viqr_3rd(x)	((x == 0x27) || (x == 0x60) || (x == 0x3f) \
			 || (x == 0x7e) || (x == 0x2e))
/* -------------------------------------------------------------- */
/* determine likely decision limit (should be trimmed)		  */
/* -------------------------------------------------------------- */
/* --- code detect persistent parameter ------------------------- */
#define DET_LIMIT	3		/* determine threshold	  */
	/* 3 suspicious action is enough to determine		  */
	/* do not set this constant below 2.			  */

/* --- likely side ---------------------------------------------- */
/* Note: these variables should be trimmed by REAL texts.	  */
/* -------------------------------------------------------------- */
#define UTF8_LK_LIMIT	16
#define UTF7_LK_LIMIT	12
#define UCS2_LK_LIMIT	12
#define JIS_LK_LIMIT	2
#define SJIS_LK_LIMIT	10
#define EUC_LK_LIMIT	12
#define LATIN_LK_LIMIT	24
#define B5_LK_LIMIT	10
#define GB_LK_LIMIT	13
#define HZ_LK_LIMIT	12
#define KR_LK_LIMIT	15
#define KEIS_LK_LIMIT	24
#define E7_LK_LIMIT	16
#define VS_LK_LIMIT	24

/* --------------------------------------------------------------- */
/* internal functions prototypes				   */
/* --------------------------------------------------------------- */
static void	dump_name_of_code P_((int, int, int));
static int single_possible P_((int,int,int,int,int,int,int,
	    int,int,int,int,int,int));
static int	eval_encoding P_((long,long));
static long	is_valid_utf8_seq P_((int,int,int,int));
static void	set_le_parse P_((int,int,int));
#ifdef SKFDEBUG
static void	___debug_fault P_((char *,int));
#endif
static int	itext_statchange P_((int,int));

/* --------------------------------------------------------------- */
/* Input side packed shift condition. see convert.h		   */
/* --------------------------------------------------------------- */
unsigned long	shift_condition;	
unsigned long	sshift_condition;	

#ifdef SKFDEBUG
static int		rescase = 0;
#endif

static int	preconvert_counts = 0;
#ifndef SWIG_EXT
static char	*dcode_name = "DEFAULT_CODE";
static char	*utf16be_name = "(UTF-16(BE))";
static char	*utf16le_name = "(UTF-16(LE))";
#endif

/* --------------------------------------------------------------- */
/* table of tables						   */
/* --------------------------------------------------------------- */
/* --- misc control codes which may appear in input stream	   */
static int control_may_appear[32] = {
  0,0,0,0,0,0,0,0, 1,1,1,0,0,1,0,0, 0,0,0,0,0,0,0,0, 1,0,0,0,0,0,0,0
};
/* --- control codes designated to viscii codes			   */
static int viscii_v[32] = {
  0,0,1,0,0,0,1,0, 0,0,0,0,0,0,0,0, 0,0,0,0,1,0,0,0, 0,1,0,0,0,0,1,0
};

static int pure_ascii_tbl[94] = {
    0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,1, 1,1,0,0,0,0,0,0,
  0,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,0,0,0,0,0,
  0,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,0,0,0,0
};

/* --------------------------------------------------------------- */
/* SCOREBOARDING detecter					   */
/* --------------------------------------------------------------- */
static int 
single_possible(jis,sjis,euc,utf8,utf7,ucs2,latin,b5,gb,kr,hz,jl,sl)
int jis,sjis,euc,utf8,utf7,ucs2,latin,b5,gb,kr,hz,jl,sl;
{
    if (sjis && euc && utf8 && utf7 && ucs2 && latin
	&& b5 && gb && kr && hz ) {
	set_rescase(300);
	in_codeset = codeset_x0208;
	return(1);
    } else if (jis && euc && utf8 && utf7 && ucs2 && latin
	&& b5 && gb && kr && hz) {
	set_rescase(301);
	in_codeset = codeset_sjis;
	return(1);
    } else if (jis && sjis && utf8 && utf7 && ucs2 && latin
	&& b5 && gb && kr && hz) {
	set_rescase(302);
	in_codeset = codeset_eucjp;
	return(1);
    } else if (jis && sjis && euc && utf7 && ucs2 && latin
	&& b5 && gb && kr && hz) {
	set_rescase(303);
	in_codeset = codeset_utf8;
	return(1);
    } else if (jis && sjis && euc && utf8 && ucs2 && latin
	&& b5 && gb && kr && hz) {
	set_rescase(304);
	in_codeset = codeset_utf7;
	return(1);
    } else if (jis && sjis && euc && utf8 && utf7 && latin
	&& b5 && gb && kr && hz ) {
	set_rescase(305);
	in_codeset = codeset_utf16le;
	return(1);
    } else if (jis && sjis && euc && utf8 && utf7 && ucs2 
	&& b5 && gb && kr && hz) {
	set_rescase(307);
	in_codeset = codeset_8859_1;
	return(1);
    } else if (jis && sjis && euc && utf8 && utf7 && ucs2
	&& latin && gb && kr && hz) {
	set_rescase(308);
	in_codeset = codeset_big5;
	return(1);
    } else if (jis && sjis && euc && utf8 && utf7 && ucs2
	&& latin && b5 && kr && hz) {
	set_rescase(309);
	in_codeset = codeset_euccn;
	return(1);
    } else if (jis && sjis && euc && utf8 && utf7 && ucs2
	&& latin && gb && b5 && hz) {
	set_rescase(310);
	in_codeset = codeset_euckr;
	return(1);
    } else if (jis && sjis && euc && utf8 && utf7 && ucs2
	&& latin && gb && b5 && kr) {
	set_rescase(311);
	in_codeset = codeset_cnhz;
	return(1);
    } else if (euc && utf8 && utf7 && ucs2 && latin
	&& b5 && gb && kr && hz &&
	skf_is_ja(skf_input_lang)) {	/* JIS or SJIS -> SJIS	  */
	set_rescase(312);
	if (jl > sl) in_codeset = codeset_jis;
	else in_codeset = codeset_sjis;
	return(1);
    } else;
    return(0);
}

#ifdef SKFDEBUG
/* --------------------------------------------------------------- */
/* --------------------------------------------------------------- */
void ___debug_fault(x,y)
char *x;
int   y;
{
    if (is_vv_debug) 
	fprintf(stderr,"\ncode %s fault by %d",x,y);
    rescase = y;
    return;
}
#endif
/* --------------------------------------------------------------- */
/* utf8 test: return -	0 	code is not read yet		   */
/*			>0	return decoded (valid) code.	   */
/*			-1	code is not utf8		   */
/*   sequence:  before c4 c3 c2 c1=current			   */
/* --------------------------------------------------------------- */
#define ZF23VAV		0xc2
#define ZF3VAV		0xe1
#define ZF3V2V		0xa0
#define ZF4V2V		0x90

static long 
is_valid_utf8_seq(c1,c2,c3,c4)
int c1,c2,c3,c4;
{
    long i;

    if (c1 < 0) return(0);
    if (c1 < ZCVAL) return(c1);

    if ((c1 >= ZF23VAV) && (c1 < ZF3VAL)) return(0);
    if ((c2 >= ZF23VAV) && (c2 < ZF3VAL) && (c1 >= ZCVAL) 
		&& (c1 < ZF23VAL)) {
	i = ((c2 & Z23MSK) << 6) + (c1 & Z1MSK);
	return(i);
    };

    if ((c1 >= ZF3VAL) && (c1 < ZF4VAL)) return(0);
    if ((c2 >= ZF3VAV) && (c2 < ZF4VAL) && (c1 >= ZCVAL) 
		&& (c1 < ZF23VAL)) return(0);
    if ((c2 == ZF3VAL) && (c1 >= ZF3V2V) && (c1 < ZF23VAL)) return(0);
    if ((c3 >= ZF3VAL) && (c3 < ZF4VAL) && (c2 >= ZCVAL) 
		&& (c2 < ZF23VAL) && (c1 >= ZCVAL) && (c1 < ZF23VAL)) {
	i = ((c3 & Z3MSK) << 12) + ((c2 & Z1MSK) << 6) + (c1 & Z1MSK);
	if ((c1 < U_SUR) || (c1 > U_COMP)) return(i);
	else { 			/* these areas should never appear */
	    return(-1);
	};
    };

    if ((c1 >= ZF4VAL) && (c1 < ZF4VAV)) return(0);
    if ((c2 >= ZF4VAL) && (c2 < ZF4VAV) && (c1 >= ZCVAL) 
	&& (c1 < ZF23VAL)) return(0);
    if ((c2 == ZF4VAL) && (c2 < ZF4VAV) && (c1 >= ZF4V2V) 
	&& (c1 < ZF23VAL)) return(0);
    if ((c3 >= ZF4VAL) && (c3 < ZF4VAV) && (c2 >= ZCVAL) 
	&& (c2 < ZF23VAL) && (c1 >= ZCVAL) && (c1 < ZF23VAL)) return(0);
    if ((c4 == 0xf0) && (c3 >= 0x90) && (c3 < 0xc0) && (c2 >= 0x80) &&
	(c2 < 0xc0) && (c1 >= ZF2VAL) && (c1 < 0xc0)) return(1);
    if ((c4 >= ZF4VAL) && (c4 < ZF4VAV) && (c3 >= ZCVAL) && (c3 < ZF23VAL)
	&& (c2 >= ZCVAL) && (c2 < ZF23VAL) 
	&& (c1 >= ZCVAL) && (c1 < ZF23VAL)) {
	i = ((c4 & Z4MSK) << 18) + ((c3 & Z43MSK) << 12)
		+ ((c2 & Z1MSK) << 6) + (c1 & Z1MSK);
	if (i <= 0x2ffff) return(i);
	else {
	    return(-1);
	};
    };
    return(-1);
}

/* --------------------------------------------------------------- */
/* eval_encoding: statistical based encoding-likely test	   */
/* both input characters shall be utf-32.			   */
/*	input		w1	current character		   */
/*			w2	one character before w1		   */
/*	return		>0	add this value to xxlikely. 	   */
/*			-1	add this value to xxunlikely.	   */
/*			-2	add this value to xxfault.	   */
/*			0	neutral				   */
/* --------------------------------------------------------------- */
static int
eval_encoding(w1,w2)
long w1,w2;
{
    int score = 0;

    if ((w1 < 0) || (w2 < 0)) return(0);

/* ascii */
    if (is_digit(w1) && is_digit(w2)) return(2); /* numberp	   */
    if (is_digit(w1) && is_alpha(w2)) return(1); /* alpha + number */
    if (is_space(w1) && is_alpha(w2)) return(1);
    if (is_space(w1) && is_latinterm(w2)) return(2); /* ". " or ", " */
    if (is_upper(w1) && is_upper(w2)) return(1);
    if (is_upper(w1) && is_lower(w2)) return(-1);
    if (is_lower(w1) && is_lower(w2)) return(2);
    if (is_lower(w1) && is_upper(w2)) return(2);
/* continuing toten/kuten is unlikely */
    if (is_h_kuten(w1) && is_h_kuten(w2)) return(-1);
    if (is_h_toten(w1) && is_h_kuten(w2)) return(-2);
    if (is_h_kuten(w1) && is_h_toten(w2)) return(-2);
    if (is_h_toten(w1) && is_h_toten(w2)) return(1);
    if (is_h_dot(w1) && is_h_comma(w2)) return(-2);
    if (is_h_comma(w1) && is_h_dot(w2)) return(-2);
/* iso8859 */
    if (is_diaupalpha(w1) && is_space(w2)) return(0);
    if (is_dialwalpha(w1) && is_space(w2)) return(0);
    if (is_dialwalpha(w1) && is_alpha(w2)) return(1);
    if (is_diaalpha(w2) && is_lower(w1)) return(1);
    if (is_diaalpha(w2) && is_dialwalpha(w1)) return(1);
    if (is_dialwalpha(w2) && is_diaupalpha(w1)) return(-2);
    if (is_diaalpha(w2) && is_diaupalpha(w1)) return(-1);
/* JIS x0201 kana */
    if ((w1 == U_CDAK) && (w2 >= U_CKAN) && (w2 <= U_CKANE)) {
	if (dakuten[w2-0xff60] == 0) return(-1);
	score = 1;
    } else if ((w1 == U_CHDK) && (w2 >= U_CKAN) && (w2 <= U_CKANE)) {
	if (dakuten[w2-0xff60] != 3) return(-1);
	score = 2;
    };
    if (is_ihkana(w1) && is_ihkana(w2)) {
	score ++;
    };
    if (is_ihkana(w1) && (is_ikanji(w2) || is_ikatak(w2))) {
	score--;
    };
    if (is_ihkana(w1) && is_ihirak(w2)) {
	score -= 2;
    };
    if ((w1 == U_HKTN) && !is_ihkana(w2)) return(-2);
    if ((is_h_kbra(w2) || is_h_mdot(w2) || is_h_toten(w2) || is_h_haton(w2))
  && (is_h_kuten(w1) || is_h_dakon(w1) || is_h_haton(w1) || is_h_kbra(w1)))
	return(-2);	/* hankaku burasage break */
    if ((is_h_kket(w2) || (w2 == ')'))
  && (is_h_haton(w1) || is_h_dakon(w1) || is_h_mdot(w1))) return(-2);
		/* hw kana kuten with !half_width kana = FAULT	   */
    if (is_h_kuten(w2) && (is_h_kuten(w1) || is_h_toten(w1))) return(-2);
    if ((is_kanabra(w2) || is_middot(w2) || is_kanakuten(w2))
	&& (is_kanakuten(w1) || is_kanabra(w1))) return(-2);
    if ((is_kanaket(w2) || (w2 == ')'))
	&& (is_middot(w1) || is_smallkana(w1) || is_leader(w1)))
	return(-2);
    if (is_kanakuten(w2) && (is_kanakuten(w1) || is_kanatoten(w1)))
	return(-2);
/* Kanji's */
    if (is_ikanji(w2) && is_h_kutot(w1)) return(-4);
    if (is_ikanji(w1) && is_h_kutot(w2)) return(-4);

    if (is_ikanji(w1) && (is_ikanji(w2) || is_ihirak(w2))) {
	score++;
    };

/* latin */
    if (is_latglyph(w2) && is_dialwalpha(w1)) return(-4);

/* Hangul */
    if (is_ihangul(w1) && is_ihangul(w2)) {
    	score++;
    };
    return(score);
}

/* --------------------------------------------------------------- */
/* input text detector						   */
/* --------------------------------------------------------------- */
#define ITEXT_ASC	1
#define ITEXT_ASCCOL	2
#define ITEXT_ASCCOLSP	3

#define ITEXT_BRA	8
#define ITEXT_BRAASC	9
#define ITEXT_BRASGMLC	10
#define ITEXT_KET	11
#define ITEXT_BRABAN	12
#define ITEXT_BRBNMI	13	/* <!- */
#define ITEXT_COMST	14	/* SGML comment */

#define ITEXT_PLAIN	256
#define ITEXT_FTEXT	257

#define is_itext_asc(x)	(((x >= '0') && (x <= '9')) \
	|| ((x >= '@') && (x <= 'Z')) || ((x >= 'a') && (x <= 'z'))\
	|| (x == A_USCORE) || (x == '-'))
#define is_itext_sgml(x) (is_itext_asc(x) || (x == '=') || (x == 0x22) \
	|| (x == 0x27) || (x == '!') || (x == '[') || (x == '_') \
	|| ((x >= 0x2b) && (x <= 0x2f)) || (x == ']') || (x == '%'))


static int	itext_statchange(o_itext_mode,ch)
int	ch;
int	o_itext_mode;
{
    int	itext_mode = o_itext_mode;

    if (is_white(ch)) {
    	if (o_itext_mode == ITEXT_BRAASC) {
	    itext_mode = ITEXT_BRASGMLC;
	    set_intext_sgml;
	} else;
    	return(itext_mode);
    } else if (is_lineend(ch) && ((o_itext_mode == ITEXT_ASC) 
       || (o_itext_mode == ITEXT_BRAASC) || (o_itext_mode == ITEXT_BRABAN))){
	return(0);	/* new_itext_mode will be 0 */
    } else if (!is_ascii(ch)) {	/* other controls and uppers.	  */
    	itext_mode = ITEXT_PLAIN;
	set_intext_plain;
    } else;

    switch(o_itext_mode) {
    	case 0:	/* clean */
	    if (ch == A_BRA) { itext_mode = ITEXT_BRA;
	    } else if (is_itext_asc(ch)) { itext_mode = ITEXT_ASC;
	    } else if (is_lineend(ch)) { ;
	    } else { itext_mode = ITEXT_PLAIN;
	    }; break;
	case ITEXT_ASC:
	    if (ch == A_BRA) { itext_mode = ITEXT_BRA;
	    } else if (ch == A_COL) { itext_mode = ITEXT_ASCCOL;
	    } else { itext_mode = ITEXT_PLAIN;
	    	set_intext_plain;
	    }; break;
	case ITEXT_ASCCOL:
	    if (is_lineend(ch)) {
	    	itext_mode = ITEXT_ASCCOLSP;
		set_intext_mail;
	    } else {
	    	itext_mode = ITEXT_PLAIN;
	    	set_intext_plain;
	    };
	    break;
	case ITEXT_BRA:
	    if (is_itext_asc(ch)) {
	    	itext_mode = ITEXT_BRAASC;
	    } else if (ch == A_BANG) {
	    	itext_mode = ITEXT_BRABAN;
	    } else;
	    break;
	case ITEXT_BRABAN:
	    if (is_itext_asc(ch)) {
	    	itext_mode = ITEXT_BRASGMLC;	/* looks like xml */
		set_intext_sgml;
	    } else if (ch == '-') {
	    	itext_mode = ITEXT_BRBNMI;
	    } else;
	    break;
	case ITEXT_BRBNMI:
	    if (ch == '-') {
	    	itext_mode = ITEXT_COMST;
		set_intext_sgml;
	    } else {
	    	itext_mode = ITEXT_PLAIN; /* like sgml, but not	   */
	    };
	    break;
	case ITEXT_BRAASC:
	    if (is_itext_asc(ch)) {
	    	return(itext_mode);
	    } else {
	    	itext_mode = ITEXT_PLAIN;
	    	set_intext_plain;
	    };
	    break;
	case ITEXT_PLAIN: break;
	case ITEXT_FTEXT: break;
    	default: break;
    };
    return(itext_mode);
}

/* --------------------------------------------------------------- */
/* Note: convert routine may not push EOF char, no matter what	   */
/* input is specified. EOF should be generated in main() when	   */
/* fclose is called. Violating this restriction may cause terminal */
/* screw up when used as transparent terminal kanji converter.     */
/* To meet this condition, extra treatment may be required.	   */
/* --------------------------------------------------------------- */
/* Note: We do detect KEIS83/JEF, but these codeset may require	   */
/*  semantic analysis.						   */
/* --------------------------------------------------------------- */
/* Following codeset is tested and detected.			   */
/*  iso-2022-jp, euc-jp, shift_jis, euc-cn (euc7), euc-kr(euc7)	   */
/*  big5, hz-gb2312, iso-8859-1, utf-8, utf-16le, utf-16be	   */
/* --------------------------------------------------------------- */

int preConvert (f)
    skfFILE  *f;
{
    int    	c1 = sEOF, 	/* current processing character	   */
		c2 = sEOF, 	/* one character before c1	   */
		c3 = sEOF, 	/* two character before c1	   */ 
		c4 = sEOF; 	/* three character before c1	   */
    int		rc1;		/* raw c1 (c1 reflect iso2022 sft) */

    long	ich;		/* character temporal buffer	   */
    long	ic1,ic2;

    long	euccurch = sEOF; /* decoded processing char	   */
    long	eucprech = sEOF; /* decoded pre-processing char	   */
	/* these two variables are in UCS4. Since some codeset	   */
	/* doesn't have table pre-loaded, some areas are represented */
	/* by the first code point of this areas(in Unic*de table) */
    long	utf8curch = sEOF; /* decoded processing char	   */
    long	utf8prech = sEOF; /* decoded pre-processing char   */
    long	b5curch = sEOF;	  /* decoded processing char	   */
    long	b5prech = sEOF;   /* decoded pre-processing char   */
    long	gbcurch = sEOF;	  /* decoded processing char	   */
    long	gbprech = sEOF;	  /* decoded processing char	   */
    long	krcurch = sEOF;   /* decoded pre-processing char   */
    long	krprech = sEOF;   /* decoded pre-processing char   */
    long	sjiscurch = sEOF; /* decoded pre-processing char   */
    long	sjisprech = sEOF; /* decoded pre-processing char   */
    long	jiscurch = sEOF;  /* decoded pre-processing char   */
    long	jisprech = sEOF;  /* decoded pre-processing char   */

   /* detect encoding status */
    int		utf7fault = 0, utf7likely = 0, utf7unlikely = 0;
    int		utf8fault = 0, utf8likely = 0, utf8unlikely = 0;
#if !defined(SWIG_EXT) || (defined(SWIGPYTHON) && defined(SKF_PYTHON3)) || defined(SWIGRUBY) || defined(HAVE_FAST_LWLSTRLEN)
    int		ucs2fault = 0, ucs2likely = 0, ucs2unlikely = 0;
#else
    int		ucs2fault = 1, ucs2likely = 0, ucs2unlikely = 1;
#endif
    int		sjisfault = 0, sjislikely = 0, sjisunlikely = 0;
    int		jisfault = 0, jislikely = 0, jisunlikely = 0;
    int		latinfault = 0, latinlikely = 0, latinunlikely = 0;
    int		b5fault = 0, b5likely = 0, b5unlikely = 0;
		/* BIG5 (ETen and cp950) */
    int		gbfault = 0, gblikely = 0, gbunlikely = 0;
		/* euc7-gb2312 */
    int		hzfault = 0, hzlikely = 0, hzunlikely = 0;
		/* hz-gb2312 */
    int		krfault = 0, krlikely = 0, krunlikely = 0;
		/* euc7-ksx1001 */
    int		e7upper = 0;
    int		eucfault = 0, euclikely = 0, eucunlikely = 0;
    int		vscfault = 0, vsclikely = 0, vscunlikely = 0;
				/* viscii (iso-2022-vi)		   */
    int		vsqfault = 0, vsqlikely = 0, vsqunlikely = 0;
				/* viqr				   */

    int		jisccnt = 0;	/* JIS character mark. See below.  */
    int		eucccnt = 0;	/* EUC character mark. See below.  */
    int		sjisccnt = 0;	/* SJIS in-character counter	   */
    int		hzccnt = 0;	/* euc-hz in-character counter	   */
    int		b5ccnt = 0;	/* BIG5 in-character counter	   */
    int		utf8ccnt = 0;	/* UTF8 in-character counter	   */
    int		utf7ccnt = 0;	/* UTF7 in-character counter	   */
    int		ucs2ccnt = 0;	/* UCS2 in-character counter	   */
    int		vsqccnt = 0;	/* VIQR in-character counter	   */

    int		kanaunlikely = 0;
    int		in_utf7_encode = 0;
    int		kanatouten = 0, kanakuten = 0, kanachar = 0;
    int		zeroappear = 0, normalcode = 0;
    int		was_ascii = 0;
    int		pure_ascii = TRUE;	/* alphabet+digit+space only */
    int		det_limit;
    int		ascii_code_only = TRUE;
    int		lk_power;
    int		res;

    int		kr_trim = 0;	/* detection trimmer by language   */
    int		ja_trim = 0;	/* see below			   */
    int		zh_trim = 0;

/* -- line delimiter character detections ------------------------ */
    long	ic_count = 0;
    long	icc,icw;
    int		le_u_cr_detect = 0;
    int		le_n_cr_detect = 0;
    int		le_u_lf_detect = 0;
    int		le_n_lf_detect = 0;
    int		le_u_crf_detect = 0;
    int		le_n_crf_detect = 0;
    int		ask1more = FALSE; /* detect needs 1 more character */
    int		lf_should_test = TRUE;

    int		itext_mode = 0;
    int		has_bom = 0;

#ifdef SKFDEBUG
    char 	ln0,ln1;

    if (is_vv_debug) {
	ln0 = (char)(skf_get_langcode(skf_input_lang) >> 8);
	ln1 = (char)(skf_get_langcode(skf_input_lang) & 0x7fU);
	if ((ln0 == 0) || (ln1 == 0)) {
	    ln0 = ' '; ln1 = '-';
	};
    	fprintf(stderr,"\npreconvert i:%c%c(%s)",ln0,ln1,
		(is_code_undefine ? "UN":"CD"));
	ln0 = (char)(skf_get_langcode(skf_output_lang) >> 8);
	ln1 = (char)(skf_get_langcode(skf_output_lang) & 0x7fU);
	if ((ln0 == 0) || (ln1 == 0)) {
	    ln0 = ' '; ln1 = '-';
	};
    	fprintf(stderr," o:%c%c opt: ",ln0,ln1);
    	if (input_inquiry) fprintf(stderr,"INQ ");
    } else;
#endif

    if (preconvert_counts == 0) preconvert_counts = 1;

/* some preparation: reset all status, shifts and clear estab.	   */
    if (is_code_undefine) {
	clear_after_mime();
	reset_rescase();
    };

/* whether need to detect lineend or not			   */
#ifdef FOLD_SUPPORT
    if ((detect_cr || detect_lf || (fold_fclap == 0))
    	&& !input_hard_inquiry && !o_encode) {
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"skip-le-det ");
#endif
	lf_should_test = FALSE;
    };
#else
    if (!input_hard_inquiry && !o_encode) {
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"skip-le-det ");
#endif
	lf_should_test = FALSE;
    };
#endif

    if (kuni_opt) {	/* limit detection to JIS/EUC/SJIS/UTF-8   */
	utf7fault = TRUE; 
	latinfault = TRUE; det_limit = 2; lk_power = 1;
    } else if (fuzzy_detect) {
	lk_power = 2;
	det_limit = DET_LIMIT * lk_power;
    } else {
	lk_power = 1;
	det_limit = DET_LIMIT * lk_power;
    };
    if (no_utf7) {
	utf7fault = TRUE;
    };

/* language detection trimming					   */
    if ((skf_input_lang == L_JA) || (skf_input_lang == M_JP)) {
	ja_trim = 4;
    };
    if ((skf_input_lang == L_ZH) || (skf_input_lang == M_ZH)) {
	zh_trim = 4;
    };
    if ((skf_input_lang == L_KO) || (skf_input_lang == M_KO)) {
	kr_trim = 4;
    };

/* test only japanese in nkf-compat mode */
    if (input_jp_limit) {
    	b5fault += 10;
    	krfault += 10;
    	gbfault += 10;
    	hzfault += 10;
    	vscfault += 10;
    	vsqfault += 10;
    } else;

/* first loop   : code is not determined yet.                      */

    while ((is_code_undefine) || (is_lt_undefine && lf_should_test)) { 
/* ---------------------------------------------------- */
/* FIRST MAJOR LOOP					*/
/* examine current state				*/
/* ---------------------------------------------------- */
	if ((utf8unlikely >= det_limit) && (utf8fault == 0)) {
	    utf8unlikely = 0;debug_fault("utf8",800);utf8fault = TRUE; };
	if ((utf7unlikely >= det_limit) && (utf7fault == 0)) {
	    utf7unlikely = 0;debug_fault("utf7",800); utf7fault = TRUE; };
	if ((eucunlikely >= det_limit) && (eucfault == 0)) {
	    eucunlikely = 0;debug_fault("euc",800); eucfault = TRUE; };
	if ((jisunlikely >= det_limit) && (jisfault == 0)) {
	    jisunlikely = 0;debug_fault("jis",800); jisfault = TRUE; };
	if ((sjisunlikely >= det_limit) && (sjisfault == 0)) {
	    sjisunlikely = 0;debug_fault("sjis",800); sjisfault = TRUE; };
	if ((ucs2unlikely >= det_limit) && (ucs2fault == 0)) {
	    ucs2unlikely = 0;debug_fault("ucs2",800); ucs2fault = TRUE; };
	if ((latinunlikely >= det_limit) && (latinfault == 0)) {
	    latinunlikely = 0;debug_fault("latin",800); latinfault = TRUE; };
	if ((b5unlikely >= det_limit) && (b5fault == 0)) {
	    b5unlikely = 0;debug_fault("b5",800); b5fault = TRUE; };
	if ((gbunlikely >= det_limit) && (gbfault == 0)) {
	    gbunlikely = 0;debug_fault("gb",800); gbfault = TRUE; };
	if ((krunlikely >= det_limit) && (krfault == 0)) {
	    krunlikely = 0;debug_fault("kr",800); krfault = TRUE; };
	if ((hzunlikely >= det_limit) && (hzfault == 0)) {
	    hzunlikely = 0;debug_fault("hz",800); hzfault = TRUE; };
	if ((vscunlikely >= det_limit) && (vscfault == 0)) {
	    vscunlikely = 0;debug_fault("vsc",800); vscfault = TRUE; };
	if ((vsqunlikely >= det_limit) && (vsqfault == 0)) {
	    vsqunlikely = 0;debug_fault("vsq",800); vsqfault = TRUE; };

	if ((utf8likely > (UTF8_LK_LIMIT * lk_power)) && !utf8fault) {
	    in_codeset = codeset_utf8; debug_fault("utf8",101); };
	if ((utf7likely > (UTF7_LK_LIMIT * lk_power)) && !utf7fault) { 
	    in_codeset = codeset_utf7; debug_fault("utf7",102); };
	if ((euclikely > ((EUC_LK_LIMIT - ja_trim) * lk_power)) && !eucfault) { 
	    in_codeset = codeset_eucjp; debug_fault("eucJP",103); };
	if ((sjislikely > (SJIS_LK_LIMIT * lk_power)) && !sjisfault) { 
	    in_codeset = codeset_sjis; debug_fault("sjis",105); };
	if ((ucs2likely > (UCS2_LK_LIMIT * lk_power)) && !ucs2fault) { 
	    in_codeset = codeset_utf16le; debug_fault("ucs2",106); };
	if ((jislikely > (JIS_LK_LIMIT * lk_power)) && !jisfault) { 
	    in_codeset = codeset_jis; debug_fault("jis",104); };
	if ((latinlikely > (LATIN_LK_LIMIT * lk_power)) && !latinfault) { 
	    in_codeset = codeset_8859_1; debug_fault("8859_1",107); };
	if ((b5likely > (B5_LK_LIMIT * lk_power)) && !b5fault) { 
	    in_codeset = codeset_big5; debug_fault("big5",108); };
	if ((gblikely > ((GB_LK_LIMIT - zh_trim) * lk_power)) && !gbfault) { 
	    in_codeset = codeset_euccn; debug_fault("eucCN",109); };
	if ((krlikely > ((KR_LK_LIMIT - kr_trim) * lk_power)) && !krfault) { 
	    in_codeset = codeset_euckr; debug_fault("eucKR",110); };
	if ((hzlikely > (HZ_LK_LIMIT * lk_power)) && !hzfault){ 
	    in_codeset = codeset_cnhz; debug_fault("hz",112); };
	if ((vsclikely > (VS_LK_LIMIT * lk_power))&& !vsqfault) { 
	    in_codeset = codeset_viscii; debug_fault("viscii",114); };
	if ((vsqlikely > (VS_LK_LIMIT * lk_power))&& !vsqfault) { 
	    in_codeset = codeset_viqr; debug_fault("viqr",115); };

	if ((jisfault > 0) && (sjisfault > 0) && (eucfault > 0) &&
	    (utf8fault > 0) && (utf7fault > 0) && (ucs2fault > 0) &&
	    (latinfault > 0) && (b5fault > 0) &&
	    (gbfault > 0) && (krfault > 0) && 
	    (hzfault > 0)) {	/* all fail condition recovery	   */
	    jisfault--; sjisfault--; eucfault--; utf8fault--;
	    ucs2fault--; latinfault--;
	    b5fault--; gbfault--; krfault--; hzfault--;
	    /* --- Note: utf7 has no reason to salvage ----------- */
	};
	(void)single_possible(jisfault,sjisfault,eucfault,utf8fault,
		utf7fault,ucs2fault,latinfault,b5fault,
		gbfault,krfault,hzfault,
		jislikely-jisunlikely,sjislikely-sjisunlikely);

	/* check again */
	if ((!is_code_undefine) && (!is_lt_undefine || !lf_should_test)) 
		break;

	/* prepare to old input character			   */
	in_var_shift;

	/* --- skf 1.9 decides code when queue reaches full ------ */
	/* Note: we need not count '>del' code, because these 	   */
	/* codes always be put into queue, and we can check queue  */
	/* afterward if necessary.				   */

	if (Qfull & ascii_code_only & !ask1more) {
	    utf7fault++;	/* not likely, but continue anyway */
	    hzlikely++;
	    was_ascii = TRUE;	/* not euc anyway.		   */
	    if (input_inquiry) Qflush();
	    else for (;!Qempty;) oconv(deque());  /* is ascii only */
	} else if (Qfull & !(ascii_code_only) && !ask1more) {
	    break;		/* give up!			   */
	};

	ask1more = FALSE;
/* ---------------------------------------------------- */
/* get one code						*/
/* ---------------------------------------------------- */
	c1 = rvGETC(f);
	ic_count++;

#ifdef SKFDEBUG
	if (is_vv_debug) {
	    fprintf(stderr,"\n1st-Conv:%x - %x %x %x ",
	    	(unsigned int)c1, (unsigned int)c2,
		(unsigned int)c3, (unsigned int)c4);
	    if (e7upper == 1) fprintf(stderr,"R ");
	};
#endif
/* ---------------------------------------------------- */
/* code preparation					*/
/* ---------------------------------------------------- */
	rc1 = c1;
	/* if euc7 shifted, set bit8 (right plane)		   */
	if ((e7upper == 1) && (c1 > A_SP) && (c1 < A_DEL)) { 
	    c1 |= 0x80;
	};

/* ----------------------------------------------------- */
/* entering test	 */
/* exit if code is determined, queue is full or met EOF  */
/* ----------------------------------------------------- */

	if ((rc1 < 0) || (is_lineend(c1) && kuni_opt)) {
				/* sEOF or sOCD			  */
	    set_rescase(200);
	    if (eucccnt != 0) {	/* inconsistent if EUC		  */
		eucfault++; krfault++; gbfault++;
	    };
	    if (utf8ccnt != 0) {utf8fault++; };
	    if (jisccnt != 0) { jisfault++; };
	    if (hzccnt != 0) { hzfault++; };
	    if (b5ccnt != 0) { b5fault++; };
	    if (ucs2ccnt != 0) { ucs2fault++; };
	    if (vsqccnt != 0) { vsqfault++; };
	    if (sjisccnt != 0) { sjisfault++; };
	    if ((rc1 == sEOF) || is_lineend(c1)) enque(rc1); 
	    if (!detect_cr && !detect_lf) {
		if (c2 == A_CR) { set_detect_cr;
		} else if (c2 == A_LF) set_detect_lf;
	    };
	    break;	/* pass to handler(same as Qfull)  */
	} else;
/* ---------------------------------------------------- */
/* Entering REAL test: (1) lineend character tests	*/
/* ---------------------------------------------------- */
/* assumed no Gurmurki, Gujarati, Malayalam appears.	*/
/* ---------------------------------------------------- */
	if (le_detect == 0) {
	  if ((is_code_undefine || (is_lt_undefine && lf_should_test)
		|| is_ucs_utf16(i_codeset[in_codeset].encode)) 
		&& !ucs2fault) {
/* ---------------------------------------------------- */
/* utf-16s.						*/
/* ---------------------------------------------------- */
	    icw = (ic_count & 0x03UL);
	    icc = (ic_count & 0x01UL);
	/* code detection by 0x00 */
	    if (is_code_undefine) {
		if ((rc1 == 0) && (c2 <= 0x01) && (icw == 3)) {
			/* can be BOM */
		    in_codeset = codeset_utf32;
		    reset_in_endian;
		    set_rescase(133); 
		} else if ((rc1 <= 0x01) && (c2 == 0) && (icw == 1)) {
			/* can be BOM */
		    in_codeset = codeset_utf32;
		    set_in_endian;
		    set_rescase(134); 
		} else if ((rc1 == 0) && (c1 >= 0x04) && (icc == 1)) {
			/* U-40000 or above not likely appear	*/
		    in_codeset = codeset_utf16le; /* no BOM	*/
		    reset_in_endian;
		    set_rescase(135); 
		} else if ((rc1 >= 0x04) && (c1 == 0) && (icc == 1)) {
		    in_codeset = codeset_utf16be; /* no BOM	*/
		    set_in_endian;
		    set_rescase(136); 
		} else;		/* not confirmed		*/
	    } else;		/* already know codes		*/

	    if (is_lt_undefine && lf_should_test) {
	    	if ((rc1 == 0) && (c2 == 0) && 
		    (c3 == A_CR) && (c4 == 0) &&
			(!in_big_endian || (icw == 3))) {
		    le_u_cr_detect = TRUE;
		    if (!le_u_lf_detect) le_u_crf_detect = TRUE;
	    	} else if ((rc1 == A_CR) && (c2 == 0) && 
		    (c3 == 0) && (c4 == 0) &&
			(!in_big_endian || (icw == 3))) {
		    le_u_cr_detect = TRUE;
		    if (!le_u_lf_detect) le_u_crf_detect = TRUE;
	    	} else if ((rc1 == 0) && (c2 == A_CR) && 
			(!in_big_endian || (icc == 1))) {
		    le_u_cr_detect = TRUE;
		    if (!le_u_lf_detect) le_u_crf_detect = TRUE;
	    	} else if ((rc1 == A_CR) && (c2 == 0) && 
			(in_big_endian || (icc == 1))) {
		    le_u_cr_detect = TRUE;
		    if (!le_u_lf_detect) le_u_crf_detect = TRUE;
	    	} else if ((rc1 == 0) && (c2 == 0) && 
		    (c3 == A_LF) && (c4 == 0) &&
			(!in_big_endian || (icw == 3))) {
		    le_u_lf_detect = TRUE;
		    if (!le_u_cr_detect) le_u_crf_detect = TRUE;
	    	} else if ((rc1 == A_LF) && (c2 == 0) && 
		    (c3 == 0) && (c4 == 0) &&
			(!in_big_endian || (icw == 3))) {
		    le_u_lf_detect = TRUE;
		    if (!le_u_cr_detect) le_u_crf_detect = TRUE;
	    	} else if ((rc1 == 0) && (c2 == A_LF) && 
			(!in_big_endian || (icc == 1))) {
		    le_u_lf_detect = TRUE;
		    if (!le_u_cr_detect) le_u_crf_detect = TRUE;
	    	} else if ((rc1 == A_LF) && (c2 == 0) && 
			(in_big_endian || (icc == 1))) {
		    le_u_lf_detect = TRUE;
		    if (!le_u_cr_detect) le_u_crf_detect = TRUE;
		} else {
		};
	    } else ;
	    if ((rc1 == A_CR) && (c2 == 0)
		&& (in_big_endian || ((ic_count & 0x01UL) == 0))) {
		 /* if we don't know code, or UCS2B		   */
		le_u_cr_detect = TRUE;
		     /* I know it is too aggressive, but UCS2 wo BOM   */
		     /* is hard to determine anyway.		   */
		if (!le_u_lf_detect) le_u_crf_detect = TRUE;
		if (((c3 == A_CR) || (c3 == A_LF)) && (c4 == 0)) {
		    set_le_parse(
			le_u_cr_detect,le_u_lf_detect,le_u_crf_detect);
		    in_codeset = codeset_utf16; set_in_endian;
		    enque(rc1);
		    set_rescase(133); 
		    if (is_code_undefine) continue;
		    else break;
		};
	    } else if ((rc1 == 0) && (c2 == A_CR) 
		    && (!in_big_endian || ((ic_count & 0x01UL) == 1))) {
		le_u_cr_detect = TRUE; /* see comment above.	   */
		if (!le_u_lf_detect) le_u_crf_detect = TRUE;
		if (((c4 == A_CR) || (c4 == A_LF)) && (c3 == 0)) {
		    set_le_parse(
			le_u_cr_detect,le_u_lf_detect,le_u_crf_detect);
		    in_codeset = codeset_utf16; reset_in_endian;
		    enque(rc1);
		    set_rescase(134);
		    if (is_code_undefine) continue;
		    else break;
		};
	    } else if ((rc1 == A_LF) && (c2 == 0)
		    && (in_big_endian || ((ic_count & 0x01UL) == 0))) {
		le_u_lf_detect = TRUE;
		if (((c3 == A_CR) || (c3 == A_LF)) && (c4 == 0)) {
		    set_le_parse(
			le_u_cr_detect,le_u_lf_detect,le_u_crf_detect);
		    in_codeset = codeset_utf16; set_in_endian;
		    enque(rc1);
		    set_rescase(135); 
		    if (is_code_undefine) continue;
		    else break;
		};
	    } else if ((rc1 == 0) && (c2 == A_LF)
		    && (!in_big_endian || ((ic_count & 0x01UL) == 1))) {
		le_u_lf_detect = TRUE;
		if (((c4 == A_CR) || (c4 == A_LF)) && (c3 == 0)) {
		    set_le_parse(
			le_u_cr_detect,le_u_lf_detect,le_u_crf_detect);
		    in_codeset = codeset_utf16; reset_in_endian;
		    enque(rc1);
		    set_rescase(136);
		    if (is_code_undefine) continue;
		    else break;
		};
	    } else if ((rc1 == 0) && pure_ascii) {
	    	in_codeset = codeset_utf16; set_in_endian;
	    } else if ((rc1 == 0) && (c2 > 0) && pure_ascii) {
	    	in_codeset = codeset_utf16; reset_in_endian;
	    } else {
		if (((c3 == A_CR) || (c3 == A_LF)) && (c2 == 0)) {
		    if (c3 == A_CR) le_u_cr_detect = TRUE;
		    if (c3 == A_LF) le_u_lf_detect = TRUE;
		    set_le_parse(
			le_u_cr_detect,le_u_lf_detect,le_u_crf_detect);
		    in_codeset = codeset_utf16le; reset_in_endian;
		    enque(rc1);
		    set_rescase(137);
		    if (is_code_undefine) continue;
		    else break;
		} else if (((c2 == A_CR) || (c2 == A_LF))
				    && (c3 == 0)) {
		    set_le_parse(
			le_u_cr_detect,le_u_lf_detect,le_u_crf_detect);
		    in_codeset = codeset_utf16le; set_in_endian;
		    enque(rc1);
		    set_rescase(138); 
		    if (is_code_undefine) continue;
		    else break;
		} else;
		set_le_parse(
			le_u_cr_detect,le_u_lf_detect,le_u_crf_detect);
		if (rc1 == 0) pure_ascii = FALSE;
#ifdef SKFDEBUG
		if (is_vv_debug) {
		  if (detect_cr) fprintf(stderr," CR_n trap ");
		  if (detect_lf) fprintf(stderr," LF_n trap ");
		};
#endif
	    };
	  } else;	/* not UTF-16 */
/* ---------------------------------------------------- */
/* text types						*/
/* ---------------------------------------------------- */
	  if (is_intext_undet) {
	      itext_mode = itext_statchange(itext_mode,rc1);
	  } else;
/* ---------------------------------------------------- */
/* !utf-16						*/
/* ---------------------------------------------------- */
	  if (is_code_undefine
		|| ((in_codeset != codeset_utf16le) &&
		    (in_codeset != codeset_utf16be) &&
		    (in_codeset != codeset_utf16) &&
		    (in_codeset != codeset_utf32be) &&
		    (in_codeset != codeset_utf32le) &&
		    (in_codeset != codeset_utf32))) {
	    if (rc1 == A_CR) {
		le_n_cr_detect = TRUE;
		if (!le_n_lf_detect) le_n_crf_detect = TRUE;
		if ((c2 == A_CR) || (c2 == A_LF)) {
		    set_le_parse(
			le_n_cr_detect,le_n_lf_detect,le_n_crf_detect);
		    ucs2unlikely += 2; 
		} else ask1more = TRUE;
	    } else if (rc1 == A_LF) {
		le_n_lf_detect = TRUE;
		if ((c2 == A_CR) || (c2 == A_LF)) {
		    set_le_parse(
			le_n_cr_detect,le_n_lf_detect,le_n_crf_detect);
			ucs2unlikely += 2; 
		} else ask1more = TRUE;
	    } else {		/* trap. 			   */
		if (le_n_cr_detect || le_n_lf_detect ||
		     (c2 == A_CR) || (c2 == A_LF)) {
		    set_le_parse(
			le_n_cr_detect,le_n_lf_detect,le_n_crf_detect);
#ifdef SKFDEBUG
		    if (is_vv_debug) {
		      if (detect_cr) fprintf(stderr," CR_n trap ");
		      if (detect_lf) fprintf(stderr," LF_n trap ");
		    };
#endif
		};
	    };
	  };
	};
	if (!is_code_undefine) { /* if we already know codeset	  */
		enque(rc1); continue;
	};
/* ---------------------------------------------------- */
/* Entering REAL test: (2) iso-2022 charset call	*/
/* ---------------------------------------------------- */
	/* high range trap */
	if (rc1 >= 0x80) {
	    normalcode++; hzfault++; vsqfault++; utf7fault++;
	    hzccnt = 0; vsqccnt = 0; 
	    ascii_code_only = FALSE; normalcode = 0;
	    if ((pure_ascii == TRUE) && (rc1 >= 0xa1)) {
	    	if ((c2 != A_SP) && (c2 < 0xe0) && (c2 >= 0)) {
		    sjisunlikely++;	/* may be unlikely	   */
		 /*   eucunlikely++; */
		} else if (rc1 <= 0xa0) {  /* BIG5 doesn't use KSP */
		    b5fault++; latinfault++;
		} else if (rc1 >= 0xc0) {
		    latinlikely++; /* ascii+upper */
		    if (c3 != A_SP) latinlikely++;
		} else if ((rc1 != 0xa1) && (c1 != 0xa9) && 
			(c1 != 0xbf)) {
			;
		} else;
	    } else;
	    pure_ascii = FALSE;
	} else {
	    if (rc1 < 0x20) { pure_ascii = FALSE;
	    } else if (rc1 == 0x7f) { pure_ascii = FALSE;
	    } else if (rc1 >= 0x21) {
	        if (pure_ascii_tbl[rc1 - 0x21] == 0) 
			pure_ascii = FALSE;
	    } else;
	};
	if ((jisccnt == 0) && (jisfault == 0)) {
		/* jisccnt : 0 - single byte head 		  */
		/*	     1 - Multibyte secound		  */
		/*	     2 - 3/4 byte seq second		  */
		/*	     3 - 3/4 byte seq third		  */
		/*	     4 - 4 byte seq fourth		  */
	    if (c1 == A_ESC) {	/* meet esc	                  */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"esc ");
#endif
/*
Since terminal control codes also include some ESC-started seqence,
ESC code in the input stream does not always mean it is some kind
of ISO-2022 variants. This routine makes some try to filter this
kind of non-ISO2022 sequence, and input sequence would be language
sensed if it fits in ISO-2022 codeset-call sequence.
*/
		jisccnt = 2;
		ascii_code_only = FALSE; in_utf7_encode = 0;
		pure_ascii = FALSE;
		utf7fault++; ucs2fault++; 
		utf8fault++; hzfault++; vscfault++; vsqfault++;
		/* though unlikely, expects utf8 with term. seq.   */
/*	 Note: Since EUC code is 8-bit coded, EUC can't use SI/SO  */
/*		but 7-bit EUC (in euc-kr) do use SI/SO		   */
	    } else if (c1 == A_SI) {
		jisccnt = 0;
		ascii_code_only = FALSE; in_utf7_encode = 0;
		pure_ascii = FALSE;
		utf8fault++; hzfault++; latinfault++;
		utf7fault++; ucs2fault++; sjisfault++; eucfault++;
		vscfault++; vsqfault++; b5fault++;
		e7upper = 0; gblikely++; krlikely++;
	    } else if (c1 == A_SO) {
		jisccnt = 0;
		ascii_code_only = FALSE; in_utf7_encode = 0;
		utf8fault++; hzfault++; latinfault++;
		utf7fault++; ucs2fault++; sjisfault++; eucfault++;
		vscfault++; vsqfault++; b5fault++;
		pure_ascii = FALSE;
		e7upper = 1; gblikely++; krlikely++;
		eucccnt = 0;
	    } else if (c1 == A_SS2) { /* euc-jp			   */
			    /* designated to JIS X-0201 kana	   */
		krfault++; gbfault++; latinfault++; 
		ascii_code_only = FALSE; in_utf7_encode = 0;
		pure_ascii = FALSE;
	    } else if (c1 == A_SS3) { /* euc-jp  		   */
			    /* X-0212 or X-0213-2000/2004	   */
		krfault++; gbfault++; latinfault++; 
		ascii_code_only = FALSE; in_utf7_encode = 0;
		pure_ascii = FALSE;
	    } else if (c1 == 0) {
		jisccnt = 0;
		zeroappear++;  hzfault++; sjisfault++; gbfault++;
		vscfault++; vsqfault++; krfault++;
		latinfault++; utf7fault++; utf8fault++;
		if (zeroappear >= det_limit) {/* 4th zero appears  */
		    set_rescase(152);
		    in_codeset = codeset_utf16le;
		    if (normalcode <= 1) { /* explicitly UCS-4	   */
			set_in_endian; 	/* seems big	   */
		    } else if (normalcode < 3) {
				/* not 100% sure, but assume UCS2  */
			reset_in_endian; 
		    } else {
			; 	/* UCS2, endian is not clear	   */
		    };
		};
#if 0
	    } else if (c1 == A_BS) {  /* hack for groff-output	   */
		enque(rc1);    /* have no information on encoding   */
		set_rescase(279);
		if ((c2 = rvGETC(f)) < 0) break;
		enque(c2);
#endif
/* --- Other controls --- */
	    } else if (c1 < A_SP){ /* other control sequences	   */
/* --- VISCII --- */
		if (viscii_v[c1] != 0) {
		    vsclikely++;
		} else if (control_may_appear[c1] == 0) {
		    ucs2likely++;   /* should not appear	   */
		    vscfault++;
		};
	    } else if ((c1 > A_DEL) && (c1 < A_KSP)) { /* 0x80-0x9f */
		jisfault++; krfault++; gbfault++; eucfault++;
		latinfault++; hzfault++;
		if (c2 > A_DEL) vscunlikely++;
		if (c3 > A_DEL) vscunlikely++;
	    } else if ((c1 >= A_KSP) && (c1 <= KANA_END)) { /* kana */
		krfault++; gbfault++; utf7fault++; hzfault++;
		/* these area is for EUC, UTF-8 or X0201 kana.	   */
		/* Assumed there are no X0201 kana unless	   */
		/* explicitly specified. A first byte of UTF-8	   */
		/* is unlikely, because these fall in latins.	   */
		/* Note: 0xa0 is regarded as kana		   */
		jisprech = jiscurch; 
		jiscurch = c1 - 0xa0 + 0xff60;
		if (!input_x201_kana) {
		    jisfault++; 
		} else if (jisfault == 0) {
		    kanachar++;
		    if (c1 == 0xa1) {
			kanakuten++;
			if ((c2 == 0xa3) || (c2 == 0x29)) { jislikely++; }
			else if (c2 <= 0xb1) { jisfault++;}
			else ;
		    };
		    if (c2 == 0xa1) kanakuten++;
		    if (c1 == 0xa2) {
			if ((c2 == 0xa2) ||((c2 > 0xa4) && (c2 < 0xb0))) 
			    jisfault++;
		    };
		    if (c1 == 0xa3) {
			if ((c2 == 0xa3) ||((c2 > 0xa1) && (c2 < 0xb0))) 
			    jisfault++;
		    };
		    if (c1 == 0xa4) {
			kanatouten++;
			if ((c2 == 0xa3) || (c2 == 0x29)) { jislikely++;}
			else if (c2 <= 0xa6) { jisfault++;}
			else ;
		    };
		    if (c2 == 0xa4) kanatouten++;
		    if ((kanachar > 30) && (kanakuten == 0)
			&& (kanatouten == 0)) {
			kanaunlikely++;
			kanachar = 0; kanakuten = 0; kanatouten = 0;
		    };
		} else {	/* assume no kana		   */
		    jisfault++;
		};
		if ((res = eval_encoding(jiscurch,jisprech)) < 0) {
		    jisunlikely -= res;
		};
	    } else if (c1 > KANA_END) {
		jisfault++; 
		if (c1 > MS_END) jisfault++;
	    } else {		/* pure ascii			   */
		;
	    };
	} else if (jisccnt >= 2) {
	    if ((c1 < 0x20) || (c1 >= 0x7f)) {
		jisfault++;
	    } else if (jisccnt == 2) {
	    	if (c1 == 0x24) {	/* '$' : multibyte seq	   */
		    jisccnt = 3; jislikely++;
		} else if (c1 == '-') {	/* 96 single		   */
		    in_codeset = codeset_8859_1; enque(rc1);
		    set_rescase(260); continue;
		} else if ((c1 >= 0x20) && (c1 <= 0x2f)) {
		    in_codeset = codeset_x0208; /* this won't harm */
		    enque(rc1); set_rescase(270); continue;
		} else if ((c1 == 'N') || (c1 == 'O') || (c1 == 'n')
			|| (c1 == 'o') || (c1 == '~') || (c1 == '|')
			|| (c1 == '}')) {  /* shifts		   */
		    in_codeset = codeset_x0208; /* this won't harm */
		    enque(rc1); set_rescase(273); continue;
		} else {/* out of I seq. doesn't break code detect */
		    jisccnt = 0;
		};
	    } else if (jisccnt == 3) {
		if ((c1 == '@') || (c1 == 'B') || (c1 == 'I')) {
	    	    in_codeset = codeset_x0208; /* is ISO2022-jp   */
		    enque(rc1); set_rescase(251); continue;
		} else if (c1 == 'A') {
	    	    in_codeset = codeset_cn;	/* is ISO2022-cn   */
		    enque(rc1); set_rescase(253); continue;
		} else if ((c1 >= 0x28) && (c1 <= 0x2f)) {
		    jisccnt = 4;
		} else {	/* discard unknown MB sets.	   */
		    jisccnt = 0;
		};		/* discard blisssymbol		   */
	    } else if (jisccnt == 4) {
		if ((c1 == '@') || (c1 == 'B') || (c1 == 'D')) {
	    	    in_codeset = codeset_x0208; /* is ISO2022-jp   */
		    enque(rc1); set_rescase(251); continue;
		} else if ((c1 == 'P') || (c1 == 'Q')
			|| (c1 == 'O')) {
	    	    in_codeset = codeset_x0213; /* is ISO2022-jp   */
		    enque(rc1); set_rescase(252); continue;
		} else if (c1 == 'A') {
	    	    in_codeset = codeset_euccn;	/* is ISO2022-cn   */
		    enque(rc1); set_rescase(253); continue;
		} else if ((c1 == 'C') || (c1 == 'E')) {
	    	    in_codeset = codeset_euckr;	/* is ISO2022-kr   */
		    enque(rc1); set_rescase(254); continue;
		} else if ((c1 >= 'G') &&  (c1 <= 'M')) {
	    	    in_codeset = codeset_euctw;	/* is ISO2022-tw   */
		    enque(rc1); set_rescase(255); continue;
		} else {	/* discard unknown MB sets.	   */
		    jisccnt = 0; jislikely++;
		};		/* discard blisssymbol		   */
	    } else {
		jisccnt = 0;
	    };
	};

/* ------------------------------------------------------ */
/* Entering REAL test: (3) Unicode/iso10646 specific test */
/* ------------------------------------------------------ */
/* --- UCS2 ------------------ */
	if ((ucs2fault == 0) && (ucs2ccnt == 0)) {
			/* 1st character			   */
	  if ((c1 == 0xfe) || (c1 == 0xff)) {
	  /* In consistent codes, only UCS2/4 use this area.	   */
	  /* 0xfe can appear in broken(not matched) EUC, but	   */
	  /* UCS2 is more likely. 0xff is strictly in UCS2/4	   */
	  /* (can be latin, but not likely).			   */
	      jisfault++; in_utf7_encode = 0; 
	      utf8fault++;	/* utf8 does not use this area	   */
	      ascii_code_only = FALSE; in_utf7_encode = 0;
	      pure_ascii = FALSE;
	   /* Test for Unic*de endian character: 		   */
	   /* This code should not appear in the middle of text.   */
	   /* If it appears, this must be result of char drop.	   */
	  } else;
	  ucs2ccnt = 1;
	} else {		/* ucs2ccnt != 0		  */
	  if (in_big_endian) {
	      ich = (c2 << 8) + c1;
	  } else {
	      ich = (c1 << 8) + c2;
	  };
	  if (((c1 == 0xff) && (c2 == 0xfe)) 
		|| ((c1 == 0xfe) && (c2 == 0xff))) {
	      ascii_code_only = FALSE; in_utf7_encode = 0;
	      pure_ascii = FALSE;
	   /* Test for Unic*de endian character: 		   */
	   /* This code should not appear in the middle of text.   */
	   /* If it appears, this must be result of char drop.	   */
	      if (Qempty && (c1 == 0xfe) && (c2 == 0xff) 
			  && is_lang_unicode) {
		  set_rescase(130);
		  in_codeset = codeset_utf16;
		  reset_in_endian;
		  has_bom = TRUE;
		  if (!(endian_protect) && !in_big_endian)
		  	reset_in_endian ;
		  enque(rc1); continue;
	      } else if (Qempty && (c1 == 0xff) && (c2 == 0xfe)
			  && is_lang_unicode) {
		  set_rescase(131);
		  in_codeset = codeset_utf16;
		  set_in_endian;
		  has_bom = TRUE;
		  if (!(endian_protect) && !in_ltl_endian)
		  	set_in_endian ;
		  enque(rc1); continue;
	      } else if (!Qempty && is_lang_unicode && 
			  (((c1 == 0xff) && (c2 == 0xfe)) 
			      || ((c1 == 0xfe) && (c2 == 0xff)))) {
			      /* c1 must be fragment of code.	   */
		  if (c2 == 0xfe) set_in_endian ;
		  else reset_in_endian ;
		  if (c1 == 0xff) in_codeset = codeset_utf16be;
		  else in_codeset = codeset_utf16le;
		  set_rescase(132); 
		  enque(rc1); continue;
	      } else {		/* just not BOM			   */
		  latinlikely += 2;	/* much likely		   */
		  break;
	      };
	  } else if (((ich >= 0x80) && (ich < A_KSP))
	  	|| (ich < A_BEL) || ((ich >= A_SO) && (ich < A_CAN))
		|| ((ich > A_CAN) && (ich < A_SP)) ||
#if !defined(GEN_SCRIPT_SUPRT)
	      ((ich >= GEN_SCRIPT_LLM) && (ich < 0x1e00)) ||
#endif
	      ((ich >= 0xa500) && (ich < 0xac00)) ||
	      ((ich >= 0xe000) && (ich < 0xf900))) {
	      ucs2fault++;
	  } else if (
#if !defined(GEN_SCRIPT_SUPRT)
	      ((ich >= GEN_SCRIPT_LLM) && (ich < 0x1e00)) ||
#endif
	      ((ich >= 0x3400) && (ich < 0x4e00)) || /* CJK ex-1   */
	      ((ich >= 0xd800) && (ich < 0xe000))) { /* surrogate  */
	      ucs2unlikely += 2;
	  } else if (
	      ((ich >= 0x0500) && (ich < 0x1e00)) || /* various latins */
	      ((ich >= 0x2900) && (ich < 0x3000)) || /* rare symbols */
	      ((ich >= 0x3190) && (ich < 0x31bf)) || /* non-CJKV symbol */
	      ((ich >= 0xa000) && (ich < 0xac00)) || /* Y symbols */
	      ((ich >= 0xe000) && (ich < 0xf900)) || /* private */
	      ((ich >= 0xfb50) && (ich < 0xfe2f))) { /* Arabic etc. */
	      ucs2fault++;
	  };
	  ucs2ccnt = 0;
	};
	/* check intermediate seqs. */
/* --- UTF-8 ------------------ */
	if (is_lang_unicode && (utf8fault == 0)) {
	    if (Qle2 && (c3 == 0xef) && (c2 == 0xbb) && (rc1 == 0xbf)) {
		utf8likely += 3;	/* UTF8 BOM		   */
		utf8prech = sEOF; utf8curch = sEOF;
		has_bom = TRUE;
	    } else if ((c2 == A_ESC) && (rc1 > 0x20) && (rc1 < 0x30)) {
		utf8fault++;
	    } else if ((utf8curch = is_valid_utf8_seq(rc1,c2,c3,c4)) >= 0) {
		if (utf8curch == 0) {
		    utf8ccnt = 1;
		} else if (rc1 < 0x80) {
		    utf8ccnt = 0;	/* get one utf8 char	   */
		} else {
		    utf8ccnt = 0;	/* get one utf8 char	   */
		    utf8likely++;
		    res = eval_encoding(utf8curch,utf8prech);
		    if (res < 0) {	/* examine character chain */
			utf8unlikely -= res;
		    } else {
			utf8likely += res;
		    };
		    utf8prech = utf8curch;    /* character shift   */
		};
	    } else {		/* utf8 uses strict test condition */
	    	utf8fault++;
	    };
	};
/* --- UTF-7 ------------------ */
	if (is_lang_unicode && (utf7fault == 0)) {
	    if (c1 > A_DEL) {
		utf7fault++; in_utf7_encode = 0; 
	    } else if (!utf7fault && (in_utf7_encode == 0)
		&& (c1 == '+') && (Qempty || ascii_code_only)) { 
				/* UTF-7 encode starter		   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"+ ");
#endif
		ascii_code_only = TRUE; utf7ccnt = 0;
		normalcode++; in_utf7_encode = 1; 
	    } else if (!utf7fault && (c1 == '-')
				&& (in_utf7_encode == 1)) { 
				    /* UTF-7 encode terminator	   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"- ");
#endif
		utf7likely += (utf7ccnt >> 2); /* likely level	   */
		normalcode++; in_utf7_encode = 0; /* clear anyway  */
	    } else {		   /* in ascii			   */
		if (in_utf7_encode && (c1 < '/') && (c1 != '+')) {
		    utf7fault++; in_utf7_encode = 0;
		} else if (in_utf7_encode) {
		    utf7ccnt++;
		};
		if (in_utf7_encode != 0) {
		    if ((c1 < 0x2b) || ((c1 >= 0x2c) && (c1 <= 0x2e))
			    || ((c1 >= 0x3a) && (c1 <= A_AT))
			    || ((c1 >= 0x5b) && (c1 <= 0x60))
			    || (c1 >= 0x7b)) {
			    utf7fault++; in_utf7_encode = 0;
		    };
		};
	    };
	};
/* ------------------------------------------------------ */
/* Entering REAL test: (4) Various EUC code test	  */
/* ------------------------------------------------------ */
	    /* locking shift/single shift control is not necessary,*/
	    /* because these features do establish code, and 	   */
	    /* escapes loop immediately just after these.	   */
	if ((eucfault == 0) && (eucccnt == 0)) { /* first byte --- */
	  if ((((c1 >= 0xa1) && (c1 <= 0xfe)) ||
	      (e7upper && (c1 >= 0x21) && (c1 <= 0x7e)))
		&& may_euc) {
		/* within EUC code area				   */
		/* code can't be determined because UTF-8,	   */
		/* SJIS and EUC use this area.			   */
		/* likelyhood is UTF-8 >> EUC > SJIS		   */
	    ascii_code_only = FALSE; 
	    normalcode = 0; 
  /* --- euc-jp --- */
	    if ((c1 >= 0xa9) && (c1 <= 0xaf)) eucfault++;
	    if (c1 >= 0xf5) {
	    	if (c1 < 0xf9) eucfault++;
	    	if (c1 > 0xfc) eucfault++;
	    } else;
	    if ((c1 == 0xa4) || ((c1 >= 0xb0) && (c1 <= 0xcf))) {
		euclikely++;	/* hiragana and 1st-level kanji	   */
	    } else if ((c1 == 0xa2) && (c2 == 0xae)) {euclikely++;
	    } else if ((c1 == 0xa3) && (c3 == 0xa3)) {euclikely++;
	    	/* ZENKAKU alphanumeric sequence suggest likely.   */
	    } else;
  /* --- euc-cn --- */
	    if ((c1 >= 0xac) && (c1 <= 0xaf)) gbfault++;
	    if (c1 >= 0xf8) gbfault++;
	    if ((c1 >= GB_KANA) && (c1 <= GB_CYL)) gbfault++;
	    if ((c1 >= 0xb0) && (c1 <= 0xd8)) {	/* in 1st-kanji	   */
		gblikely++;
	    } else if ((c1 >= 0xc4) && (c1 <= 0xc8)) {
		gbunlikely++;	/* not part of chinese		   */
	    };
  /* --- euc-kr --- */
	    if ((c1 >= KR_KANA) && (c1 <= 0xaf)) krfault++;
	    if ((c1 == 0xa7) || (c1 == 0xc9)) krfault++;
	    if (c1 >= 0xf8) krfault++;
	    if ((c1 >= 0xb0) && (c1 <= 0xc8)) {	/* in hangul	   */
		krlikely++;
	    } else if (c1 >= 0xca) {
		krunlikely--;	/* kanji is seldom used in korian  */
	    };
  /* --- common --- */
	    eucccnt = 1;	/* OK. let's get 2nd byte	   */
	  } else if (c1 == A_SS2) { /* euc-jp			   */
			  /* designated to JIS X-0201 kana	   */
	      if (is_lang_japanese) eucccnt = 4;
	      else eucccnt = 0;
	  } else if (c1 == A_SS3) { /* euc-jp  			   */
			  /* X-0212 or X-0213-2000/2004		   */
	      if (is_lang_japanese) eucccnt = 2;
	      else eucccnt = 0;
	  } else if (((c1 >= 0x80) && (c1 <= 0xa0)) ||
		  (e7upper && ((c1 <= 0x20) || (c1 == 0x7f)))) {
			  /* inconsistent range		  */
	      eucfault++; krfault++; gbfault++;
	  } else if ((c2 == A_ESC) && (c1 <= 0x2f) 
		&& (c1 > 0x20)) {	/* ESC should not appear   */
		eucfault++;
	  } else if (c1 < A_DEL) {  /* pure ascii		   */
	      eucprech = c1; gbprech = c1; krprech = c1;
	      eucccnt = 0; normalcode++; 
	  } else {		/* A_DEL, A_KDEL		   */
	      eucfault++; krfault++; gbfault++;
	  };
	} else if (eucccnt == 1) {	/* 2nd byte of 2	   */
	  if ((c1 < A_KSP) || (c1 == A_KDEL)) { /* inconsistent	   */
	      eucfault++; krfault++; gbfault++;
	      debug_fault("eucjp",831);
	  };
	  ich = (((c2 & 0x7f) - 0x21) * 94) + (c1 & 0x7f) - 0x21;
  /* --- euc-jp --- */
	  if (is_lang_japanese && (eucfault == 0)) {
	      if (ich <= KANJI_TBL_END) {
		euccurch = uni_t_x208[ich];	/* peek x208 tbl   */
		if (euccurch == 0) {
		    eucfault++; debug_fault("eucjp",832);
		} else if ((res = 
			eval_encoding(euccurch,eucprech)) < 0) {
		    eucunlikely -= res;
		} else {
		    euclikely += res ;
		    euclikely++;
		};
	      } else {
		euccurch = 0; eucfault++;
	      };
	      eucprech = euccurch;
	  } else if (eucccnt > 1) {
	      eucccnt = 0;
	  };
  /* --- euc-cn --- */
	  if (is_lang_chinese && (gbfault == 0)) {
	      if (ich <= KANJI_TBL_END) {
#if !defined(DYNAMIC_LOADING) && INCLUDE_GB2312_TABLE
				/* if we have the table		   */
		  gbcurch = uni_t_gb2312[ich];	/* peek x208 tbl   */
		  if (gbcurch == 0) gbfault++;
#else
		  if (c2 == 0xaa) { gbcurch = c1;
		  } else if (c2 == 0xa9) { gbcurch = 0x2500;
		  } else if (c2 < 0xb0) { gbcurch = 0xac00;
		  } else if (c2 < A_KDEL) { gbcurch = 0x4e00;
		  } else gbcurch = 0;
#endif
	      } else gbcurch = 0;
	      if ((res = eval_encoding(gbcurch,gbprech)) < 0) {
		  gbunlikely -= res;
	      } else {
		  gblikely += res;
		  gblikely++;
	      };
	      gbprech = gbcurch;
	  };
  /* --- euc-kr --- */
	  if (is_lang_korian && (krfault == 0)) {
	      if (ich <= KANJI_TBL_END) {
#if !defined(DYNAMIC_LOADING) && INCLUDE_KS_X_1001_TABLE
				/* if we have the table		   */
		  krcurch = uni_t_x1001[ich];	/* peek x1001 tbl  */
		  if (krcurch == 0) krfault++;
#else
		  if (c2 == 0xa6) { krcurch = 0x2500;
		  } else if (c2 < 0xb0) { krcurch = 0x201c;
		  } else if (c2 < 0xc8) { krcurch = 0xac00;
		  } else if (c2 < A_KDEL) { krcurch = 0x4e00;
		  } else krcurch = 0;
#endif
	      } else krcurch = 0;
	      if ((res = eval_encoding(krcurch,krprech)) < 0) {
		  krunlikely -= res;
	      } else {
		  krlikely += res;
		  krlikely++;
	      };
	      krprech = krcurch;
	  };
  /* --- common --- */
	  eucccnt = 0;
	} else if (eucccnt == 2) {	/* 2 of 3		   */
	    eucccnt = 3;		/* state transition	   */
	} else if (eucccnt == 3) {	/* 3 of 3		   */
	    eucccnt = 0;		/* clear single shift	   */
	} else if (eucccnt == 4) {	/* 2 of 2 with SS2	   */
	    if ((c1 < A_KSP) && (c1 > KANA_END)) eucfault++;
	    euclikely += 2;
	    eucccnt = 0;		/* clear single shift	   */
	} else {			/* ???			   */
	    eucfault++; krfault++; gbfault++;
	};
/* ------------------------------------------------------ */
/* Entering REAL test: (5) codeset-language specific test */
/* ------------------------------------------------------ */
	if (is_lang_japanese) {
/* --- SJIS --- */
		/* e7upper is ignored in SJIS			   */
	  if (!sjisfault && (sjisccnt == 0)) { /* first byte	   */
	      if (((c1 >= 0x80) && (c1 < A_KSP))
			|| ((c1 >= KANA_END) && (c1 <= MS_END))) {
		sjisccnt = 1; /* get next character		   */
	/* Note: should fully cut UTF-8 case. */
		if (!sjisfault && kuni_opt) {	/* likely SJIS	   */
		    in_codeset = codeset_sjis; set_rescase(161);
		    enque(rc1); continue;
		};
	      } else if ((c2 == A_ESC) && (c1 <= 0x2f) 
		&& (c1 > 0x20)) {	/* ESC should not appear   */
		sjisfault++;
	      } else if (c1 <= A_DEL) {	/* is ascii		   */
		sjiscurch = c1; 
	      } else if (c1 > MS_END) {	/* not in SJIS		   */
	      	if ((c1 >= MS_GAIJI) && (c1 < 0xfa)) {
		    sjisfault++;
		} else if (c1 > 0xfc) {
		    sjisfault++;
		} else;		/* do not kick cp932 out	   */
	      } else if (c1 > KANA_END) { /* 0xe0-0xea		   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"0xe0-0xea ");
#endif
		   ;/* code can't be determined because UTF-8,	   */
		    /* SJIS and EUC use this area.		   */
		    /* likelyhood is UTF-8 >> EUC > SJIS	   */
	      } else {		/* x-0201 kana area		   */
		/* these area is for EUC, UTF-8 or X0201 kana.	   */
		/* Assumed there are no X0201 kana unless	   */
		/* explicitly specified. A first byte of UTF-8	   */
		/* is unlikely, because these fall in latins.	   */
		/* Note: 0xa0 is regarded as kana		   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"x0201-kana area ");
#endif
		sjisprech = sjiscurch; 
		sjiscurch = c1 - 0xa0 + 0xff60;
		if (!input_x201_kana && (sjisfault == 0)) {
		    kanachar++;
#if 1
		    if (c2 == 0xa1) kanakuten++;
		    if (c2 == 0xa4) kanatouten++;
		    if (c1 == 0xa1) {
			kanakuten++;
			if ((c2 == 0xa3) || (c2 == 0x29) || (c2 == 0xa1)) {
			    sjislikely++; }
			else if ((c2 >= 0) && (c2 <= 0xae)) {
				sjisfault++;}
			else ;
		    } else if (c1 == 0xa2) {
			if ((c2 == 0xa2) ||((c2 > 0xa4) && (c2 < 0xb0))) 
			    sjisfault++;
		    } else if (c1 == 0xa3) {
			if ((c2 == 0xa3) ||((c2 > 0xa1) && (c2 < 0xb0))) 
			    sjisfault++;
		    } else if (c1 == 0xa4) {
			kanatouten++;
			if ((c2 == 0xa3) || (c2 == 0x29)) {
			    sjislikely++;}
			else if (c2 <= 0xa6) { 
			sjisfault++;}
			else ;
		    } else if ((c1 == 0xae) && (c2 == 0xa2)) {
		    	sjisunlikely++;
		    };
		    if (c1 == 0xde) {
		    	if ((c1 <= 0xb5) || ((c1 >= 0xc5) && (c1 <= 0xc9))
				|| (c1 >= 0xcf)) sjisfault++;
		    } else if (c1 == 0xdf) {
		    	if ((c1 <= 0xc9) || (c1 >= 0xcf)) sjisfault++;
		    } else;
		    if ((kanachar > 30) && (kanakuten == 0)
			&& (kanatouten == 0)) {
			kanaunlikely++;
			kanachar = 0; kanakuten = 0; kanatouten = 0;
		    };
#else
		    if ((res = eval_encoding(sjiscurch,sjisprech)) < 0) {
		      sjisunlikely -= res;
		    } else {
		      sjislikely += res;
		    };
#endif
		} else {	/* assume no kana		   */
		    sjisfault++;
		};
	      };
	  } else if (sjisfault == 0) {	/* sjisccnt == 1	   */
	    if (((c2 == 0x84) && (c1 >= 0xbf)) ||
		((c2 == 0x83) && (c1 >= 0xd7)) ||
		((c2 == 0x88) && (c1 <= 0x9e)) ||
		((c2 == 0x98) && (c1 >= 0x73) && (c1 <= 0x9e)) ||
		((c2 >= 0x85) && (c2 <= 0x87)) ||
	    	(c1 < A_AT) || (c1 > A_MS2E) || (c1 == A_DEL)) {
		    sjisfault++;
	    } else {
	      if ((c2 == 0x82) || (c2 == 0x83)) { sjislikely++; };
	      
	      ic2 = c2 + c2 - ((c2 <= 0x9f) ? 0xe1 : 0x161);
	      if (c1 < 0x9f) {
		  ic1 = c1 - ((c1 > A_DEL) ? 0x20 : 0x1f);
	      } else { 
		  ic1 = c1 - 0x7e; ic2++;
	      };
	      ich = ((ic2 - 0x21) * 94) + ic1 - 0x21;
	      if ((ich <= KANJI_TBL_END) && (ich >= 0)) {
		  sjiscurch = uni_t_x208[ich];	/* peek x208 tbl   */
		  if (sjiscurch == 0) sjisfault++;
	      } else sjiscurch = 0;
	      if ((res = eval_encoding(sjiscurch,sjisprech)) < 0) {
		  sjisunlikely -= res;
	      } else {
		  sjislikely += res;
	      };
	      sjisprech = sjiscurch;
	      sjisccnt = 0;
	    };
	  };
	};
/* --- HZ --- */
	if (is_lang_chinese) {
	  if (hzfault == 0) {
	      if (hzccnt == 0) {
		if (c1 == '~') {
		    hzccnt = 1;
		} else if (c1 > A_DEL) { /* not-acceptable	   */
		    hzfault++;
		};
	      } else if (hzccnt == 1) {
		if (c1 == '{') {
		    hzccnt = 2; e7upper = 1;
		} else if (c1 > A_DEL) { /* not-acceptable	   */
		    hzfault++; hzccnt = 0;
		} else {		/* it is not HZ escape	   */
		    hzccnt = 0;
		};
	      } else if (hzccnt == 2) { /* hzccnt = 2 (in gb2312 mode) */
		if (is_lineend(c1)) {
		    hzccnt = 0; hzfault++;
		} else if (rc1 > A_DEL) {
		    hzfault++; hzccnt = 0;
		};
	      } else {		/* hzccnt = 3 (in gb2312 mode)	   */
		if (is_lineend(c1)) {
		    hzccnt = 0;
		} else if (rc1 > A_DEL) {
		    hzfault++; hzccnt = 0;
		};
	      };
	  } else; 
/* --- big5-cn --- */
	  if (b5fault == 0) {
	      if (b5ccnt == 0) {	/* first byte		   */
		if ((c1 > A_KSP) && (c1 <= A_B5E)) { /* Big5 MBr   */
		    b5ccnt = 1;
		    if ((c1 > A_B5ML) && (c1 < A_B5MU)) b5fault++;
		    else if ((c1 < A_B5ML) && (c1 > A_B5KS)) b5likely++;
		} else if (c1 < A_DEL) {	/* ASCII	   */
		    ;
		} else b5fault++; /* not ascii and not within range*/
	      } else {		/* second byte			   */
		if (((c1 >= A_AT) && (c1 < A_DEL))
		     || ((c1 > A_KSP) && (c1 <= A_B52E))) {
		     ;
		} else if (c1 <= A_AT) {	/* not in pure-BIG5 */
		    b5fault++; debug_fault("b5",810);
		} else {	
		    /* we don't have BIG5 table handy, so use fakes */
		    if (c2 == 0xa1) {
			b5curch = 0x2200; /* regard as math	    */
		    } else if (c2 == 0xa2) {
			if (c1 >= 0xe9) { b5curch = c1 - 0x88;
			} else if (c1 >= 0xcf) { b5curch = c1 - 0x8e;
			} else if ((c1 >= 0xaf) && (c1 <= 0xb8)) {
			    b5curch = c1 - 0x7f;
			} else b5curch = 0x2015;
					/* represent by hyphen */
		    } else if (c2 == 0xa3) {
			if (c1 <= 0x43) { b5curch = c1 + 0x37;
			} else b5curch = 0x391;
					/* represent by alpha  */
		    } else if ((c2 >= 0xa4) && (c2 <= 0xf9)) {
			b5curch = 0x4e00;
		    };
		    if ((res = eval_encoding(b5curch,b5prech)) < 0) {
			b5unlikely -= res;
		    } else {
			b5likely += res;
		    };
		    b5prech = b5curch; b5likely++;
		};
		b5ccnt = 0;
	      };
	   };
	} else {
	  hzunlikely++; 
	  b5ccnt = 0;
	};

/* --- iso-8859-1, also handle ascii --- */
	if (is_lang_latin && (latinfault == 0)) {
		/* e7upper is ignored in SJIS			   */
	    if (c1 >= 0xc0) {  /* 0xc0-0xff			   */
		ascii_code_only = FALSE; in_utf7_encode = 0;
		if ((res = eval_encoding(c1,c2)) < 0) {
		    latinunlikely -= res;
		} else {
		    latinlikely += res;
		};
	    } else if (c1 >= A_KSP) {  /* 0xa0-0xbf(symbols)	   */
		    /* likelyhood  EUC >> UTF16 >> UTF8 >> SJIS	   */
		ascii_code_only = FALSE; in_utf7_encode = 0;
		if ((c2 == 0xff) || ((c2 <= A_KSP) && (c2 > A_DEL))) {
			;
		} else if (c2 <= A_DEL) {
		    latinlikely++; 
		} else if ((c2 == 0xa1) && (
			((c1 >= 0xa2) && (c1 <= 0xa6)))) {
		    latinfault++;
		    debug_fault("latin",834);
		} else if ((c1 == 0xa1) && (c1 >= 0x40) && (c2 <= 0x5a)) {
		    latinfault++;
		    debug_fault("latin",834);
		};
		if ((res = eval_encoding(c1,c2)) < 0) {
		    latinunlikely -= res;
		} else {
		    latinlikely += res;
		};
	    } else if (c1 > A_DEL) {  /* 0x80-0xa0		   */
		ascii_code_only = FALSE; latinfault++;
	    } else if ((c1 >= A_SP) && (c1 < A_DEL)) {  /* ascii   */
		normalcode++;
		if ((c2 == 0xa1) && (((c1 >= 0x5b) && (c1 <= A_DEL))
		     || ((c1 >= 0x5b) && (c1 <= A_DEL))
		     || (c1 == 0x40))) {
		    latinfault++;
		    debug_fault("latin",835);
		} else;
	    } else if ((c1 < 0) && (c2 > A_KSP)) { /* terminated   */
		latinlikely++;
	    } else if (is_white(c2) && ((c1 == 0xb2) || (c1 == 0xb3))) {
	    	latinunlikely += 2;
	    } else if (is_white(c2) && ((c1 >= 0xb8) && (c1 <= 0xba))) {
	    	latinunlikely += 2;
	    } else if ((c2 >= A_KSP) && (c2 <= 0xbf) &&
	    	       (c1 >= A_KSP) && (c1 <= 0xbf)) {
		latinunlikely++;
	    };
	} else {
	  if (latinfault == 0) {
	    if (c1 > 0xc0) {
	      if ((res = eval_encoding(c1,c2)) < 0) {
		  latinunlikely -= res;
	      } else {
		  latinlikely += res;
	      };
	    } else if ((c1 > A_DEL) && (c1 < A_KSP)) {
	    	latinfault++;
		debug_fault("latin",801);
	    } else;
	  };
	};
/* --- vsc/vsq --- */
	if (is_lang_vietnamese) {
	  if (!vsqfault) {
	      if ((c1 >= A_DEL) || ((c1 > A_CR) && (c1 < A_SP))) {
		  vsqfault++;
	      } else {	/* within ascii range			   */
		  if ((c3 == 'A') || (c3 == 'a')) {
		      if (((c2 == '(') || (c2 == '^'))
				  && is_viqr_3rd(c1)) {
			  vsqlikely++;
		      } else if (c2 == '+') {
			  vsqfault++;
		      } else if (is_viqr_3rd(c1)) {
			  vsqfault++;	/* just unlikely sequence  */
		      };
		  } else if ((c3 == 'E') || (c3 == 'e')) {
		      if ((c2 == '^') && is_viqr_3rd(c1)) {
			  vsqlikely++;
		      } else if ((c2 == '(') || (c2 == '+')) {
			  vsqfault++;
		      } else if (is_viqr_3rd(c1)) {
			  vsqfault++;	/* just unlikely sequence  */
		      };
		  } else if ((c3 == 'I') || (c3 == 'i')) {
		      if ((c2 == '(') || (c2 == '+') || (c2 == '^')) {
			  vsqfault++;
		      };
		  } else if ((c3 == 'O') || (c3 == 'o') || (c3 == 'U')
			  || (c3 == 'u')) {
		      if ((c2 == '+') && is_viqr_3rd(c1)) {
			  vsqlikely++;
		      } else if ((c2 == '(') || (c2 == '^')) {
			  vsqfault++;
		      } else if (is_viqr_3rd(c1)) {
			  vsqfault++;	/* just unlikely sequence  */
		      };
		  } else if ((c3 == 'Y') || (c3 == 'y')) {
		      if ((c2 == '(') || (c2 == '+') || (c2 == '^')) {
			  vsqfault++;
		      };
		  } else if ((c2 == 'D') || (c1 == 'D')) {
			  vsqlikely++;
		  } else ;
	      };
	  };
	  if (!vscfault) { /* Note: viscii use the most broad range. */
	      if ((c1 <= 0x01) || (c1 == 0x03) || (c1 == 0x04) ||
		  (c1 == 0x07) || (c1 == 0x0b) || 
		  ((c1 >= 0x0e) && (c1 <= 0x13)) ||
		  ((c1 >= 0x15) && (c1 <= 0x18)) ||
		  ((c1 >= 0x1a) && (c1 <= 0x1d)) || (c1 == 0x1f)) {
		  vscfault++;
	      };
	      if (is_lang_japanese || is_lang_chinese || is_lang_korian)
	      	vscfault++;
	  };
	} else {
	  if ((vscfault == 0) || (vsqfault == 0)) {
	      debug_fault("vsc/vsq",801);
	      vscfault++; vsqfault++;
	  };
	};
/* ------------------------------------------------------ */
/* detect loop finish procedure				  */
/* ------------------------------------------------------ */
	enque(rc1);
#ifdef SKFDEBUG
	if (is_vv_debug) {
	    fprintf(stderr,"\njseuuulBghkvv ic:%ld",ic_count);
	    fprintf(stderr," | jl:%2d sl:%2d el:%2d ul:%2d 8l:%2d 7l:%2d",
	    	jislikely,sjislikely,euclikely,ucs2likely,utf8likely,
		utf7likely);
	    fprintf(stderr,"\nsju872n5bzriq");
	    fprintf(stderr," | ll:%2d b5:%2d kl:%2d gl:%2d hz:%2d",
	    	latinlikely,b5likely,krlikely,gblikely,hzlikely);
	    fprintf(stderr,"\n%c%c%c%c%c%c%c%c%c%c%c%c%c",
		((jisfault) ? 'x' : ((jislikely) ? 'L' : '-')),
		((sjisfault) ? 'x' : ((sjislikely) ? 'L' : '-')),
		((eucfault) ? 'x' : ((euclikely) ? 'L' : '-')),
		((utf8fault) ? 'x' : ((utf8likely) ? 'L' : '-')),
		((utf7fault) ? 'x' : ((utf7likely) ? 'L' : '-')),
		((ucs2fault) ? 'x' : '-'),
		((latinfault) ? 'x' : ((latinlikely) ? 'L' : '-')),
		((b5fault) ? 'x' : ((b5likely) ? 'L' : '-')),
		((gbfault) ? 'x' : ((gblikely) ? 'L' : '-')),
		((hzfault) ? 'x' : '-'),
		((krfault) ? 'x' : ((krlikely) ? 'L' : '-')),
		((vscfault) ? 'x' : '-'),
		((vsqfault) ? 'x' : '-')
	    );
	    fprintf(stderr," | ec:%2d",eucccnt);
	};
#endif

	/* exit if code is determined, queue is full or met EOF.   */
    };			/* **** end of first major loop ********** */

/* code detection last resort */
    if (is_code_undefine) {	/* can't determine case		   */
/* plausible test */
	if (zeroappear > 0) {
	    set_rescase(400);		/* detect as UCS2	   */
	    in_codeset = codeset_utf16le;
	} else if ((!eucfault) && (eucunlikely == 0) 
		&& (euclikely > det_limit)) {
	    set_rescase(401); 		/* detect as EUC	   */
	    in_codeset = codeset_eucjp;
	} else if ((utf8fault == 0) && (utf8likely > det_limit)) {
	    set_rescase(402); 		/* detect as UTF8	   */
	    in_codeset = codeset_utf8;
	} else if ((sjisfault == 0) && (sjislikely > det_limit)) {
	    set_rescase(403);		/* detect as SJIS	   */
	    in_codeset = codeset_sjis;
	} else if ((utf7fault == 0) && (utf7likely > det_limit)) {
	    set_rescase(404); 		/* detect as UTF7	   */
	    in_codeset = codeset_utf7;
	} else if ((latinfault == 0) && (latinlikely > det_limit)) {
	    set_rescase(405); 		/* detect as LATIN	   */
	    in_codeset = codeset_8859_1;
	} else if ((utf8fault == 0) && ((utf8likely * 4) > b5likely)
		&& (utf8likely > 1)) {
	    set_rescase(407);
	    in_codeset = codeset_utf8;
	} else if ((b5fault == 0) && (b5likely > det_limit)) {
	    set_rescase(408);
	    in_codeset = codeset_big5;
	} else if ((gbfault == 0) && (gblikely > det_limit)) {
	    set_rescase(409);
	    in_codeset = codeset_euccn;
	} else if ((krfault == 0) && (krlikely > det_limit)) {
	    set_rescase(410);
	    in_codeset = codeset_euckr;
	} else if (Qempty) {
	    set_rescase(420); 		/* detect as EUC/ASCII	   */
	    in_codeset = codeset_eucjp;
	    if ((ascii_code_only) && (normalcode >= det_limit))
		was_ascii = TRUE;
/* 2nd try: eliminating methods */
	} else if ((!eucfault) && (eucunlikely == 0) && (utf8fault > 0)
			&& (euclikely > 0)) {
	    set_rescase(421); 		/* detect as EUC	   */
	    in_codeset = codeset_eucjp;
	} else if ((eucunlikely > 0) && (utf8fault == 0)
			&& (utf8likely > 0)) {
	    in_codeset = codeset_utf8;
	    set_rescase(422); 		/* detect as UTF8	   */
	} else if ((sjislikely > 0) && (sjisfault == 0)) {
	    set_rescase(423);
	    in_codeset = codeset_sjis;
	} else if ((utf7likely > 0) && (utf7fault == 0)) {
	    set_rescase(424);
	    in_codeset = codeset_utf7;
	} else if ((utf8fault == 0) && (utf8likely > 0)) {
	    set_rescase(425);
	    in_codeset = codeset_utf8;
	} else if ((latinlikely > 0) && (latinfault == 0)) {
	    set_rescase(426);
	    in_codeset = codeset_8859_1;
	} else if ((b5likely > 0) && (b5fault == 0)) {
	    set_rescase(428);
	    in_codeset = codeset_big5;
	} else if ((gblikely > 0) && (gbfault == 0)) {
	    set_rescase(429);
	    in_codeset = codeset_euccn;
	} else if ((krlikely > 0) && (krfault == 0)) {
	    set_rescase(430);
	    in_codeset = codeset_kr;
	} else if (ascii_code_only) {
	    set_rescase(431);
	    in_codeset = ASSUMED_ASCII_CODE;
	} else if ((vsqlikely > 0) && (vsqfault == 0)) {
	    set_rescase(432);
	    in_codeset = codeset_viqr;
	} else if ((vsclikely > 0) && (vscfault == 0)) {
	    set_rescase(431);
	    in_codeset = codeset_viscii;
/* 3rd try: compare likelyhood */
	} else if (((sjislikely > euclikely)
		|| ((sjislikely > 0) && eucfault)) & !sjisfault) {
	    set_rescase(440);
	    in_codeset = codeset_sjis;
	} else if (((utf8likely > euclikely) 
		|| ((utf8likely > 0) && eucfault && sjisfault)) & !utf8fault) {
	    set_rescase(441);
	    in_codeset = codeset_utf8;
	} else if (((euclikely > sjislikely) 
		|| ((euclikely > 0) && utf8fault)) & !eucfault) {
	    set_rescase(442);
	    in_codeset = codeset_eucjp;
/* No clue ... */
	} else if (!sjisfault && eucfault && 
		(utf8fault || latinfault)) {
	    set_rescase(443);
	    in_codeset = codeset_sjis;
	} else if (!sjisfault && eucfault && !utf8fault) {
	    set_rescase(444);
	    in_codeset = codeset_utf8;
	} else if (sjisfault && !eucfault && !utf8fault) {
	    set_rescase(445);
	    in_codeset = codeset_utf8;
	} else if ((DEFAULT_I == codeset_sjis) && sjisfault) {
	    set_rescase(446);
	    in_codeset = codeset_eucjp;
	} else if ((DEFAULT_I == codeset_sjis) && eucfault) {
	    set_rescase(447);
	    in_codeset = codeset_sjis;
	} else if ((DEFAULT_I == codeset_utf8) && utf8fault) {
	    set_rescase(448);
	    in_codeset = codeset_utf8;
	} else if (!utf8fault && !latinfault) {
	    set_rescase(449);
	} else if (eucfault && (DEFAULT_I == codeset_eucjp)) {
	    set_rescase(451);	/* do not fall into wrong code */
	    in_codeset = codeset_sjis;
	} else {
	    set_rescase(450);
	    in_codeset = DEFAULT_I;
	};
    };	/* end of loop */
#ifdef SKFDEBUG
    if (is_v_debug) 
	fprintf(stderr," in_codeset: %d rescase: %d(%d) ",
		in_codeset,rescase,latinlikely);
#endif
/* --- line end detection after treatments ------------------- */
    if (!detect_lf && !detect_cr) {
#ifdef SKFDEBUG
	if (is_v_debug) fprintf(stderr,"LE-UnDet ");
#endif
	if (is_det_ucod) {
	  set_le_parse(le_u_cr_detect,le_u_lf_detect,le_u_crf_detect);
	} else {
	  set_le_parse(le_n_cr_detect,le_n_lf_detect,le_n_crf_detect);
	};
	if (!detect_lf && !detect_cr) {
	    set_dummy_detect;
/* --- lineend default process --- */
	    if (is_nkf_compat) {
	    	set_detect_lf;
	    } else {
        /* no lineend detected. use default one */
#ifdef DEFAULT_EOL_LF
		set_detect_lf;
#else
#ifdef DEFAULT_EOL_CRLF
		set_detect_lf;
#endif
		set_detect_cr;
#endif
	    };
	} else;
#ifdef SKFDEBUG
    } else {
	if (is_v_debug && detect_cr) fprintf(stderr,"CR ");
	if (is_v_debug && detect_lf) fprintf(stderr,"LF ");
#endif
    };

/* --- nkf codeset canonical name bug compatibility ------------ */
    if (is_nkf_compat) {
    	if (in_codeset == codeset_x0213) {
	    in_codeset = codeset_x213a;
	} else if (in_codeset == codeset_euc_213) {
	    in_codeset = codeset_euc_213a;
	} else if (in_codeset == codeset_sj_0213) {
	    in_codeset = codeset_sj_213a;
	} else ;
    } else;

/* --- clear detected endian before reading in_param ----------- */
    if (endian_protect) clear_in_endian;

/* --- reflect in_param into conv_cap -------------------------- */
    if (is_set_in_le && is_ucs_ufam(conv_cap)) reset_in_endian;
    else if (is_set_in_be && is_ucs_ufam(conv_cap)) set_in_endian;
    else;

/* intext type related trimming */
    if ((is_intext_mail || is_intext_maillike) && nkf_compat) {
    	mime_limit_add(16);
    } else ;

/* --- 1.92a5 temporally hook ---- */
#ifdef SKFDEBUG
    if ((is_v_debug) && (in_codeset >= 0) && !input_inquiry) {
	debug_analyze();
	(void)fflush(stderr);
    };
#endif
    if (input_inquiry && test_primary_codeset()) {
#ifndef SWIG_EXT
	if (show_filename) {
		printf("%s: ",in_file_name);
	};
#endif
	dump_name_of_code(was_ascii,0, has_bom);
	if (input_hard_inquiry) {
	    dump_name_of_lineend(le_detect,0);
	};
	printf("\n");

	Qflush();
	return(sEOF);
    };

/* --- nkf-feature: CRLF normalize ---- */
    if (is_nkf_compat) {
      if (is_lineend_normalize && is_o_encode) {
    	if (detect_cr) {
	    if (!detect_lf) set_lineend_cr;
	    else set_lineend_crlf;
	} else if (detect_lf) {
	    set_lineend_lf;
	} else;
      } else;

      if ((in_codeset != codeset_x0213)
	     && (in_codeset != codeset_x213a)
	     && (in_codeset != codeset_x0213_s)
	     && (in_codeset != codeset_euc_213)
	     && (in_codeset != codeset_euc_213a)
	     && (in_codeset != codeset_sj_0213)
	     && (in_codeset != codeset_sj_213a)) {
	 set_uni_hain_concat ;
      } else;
    } else;
/* --- used lineend definition --- */
    le_defs = le_detect;

    /* Now we know the code, and ready for input process. Then set */
    /* everything for input process(tables etc.) at this point.	   */
    if ((in_codeset_preload() < 0) && disp_warn) {
	in_undefined(0,SKF_UNDEFCSET);
    } else;

    /* Now we knows REAL language of streams here. */

    return (is_det_jis ? e_in(f) :		/* jis -> e_in	   */
    	    ((is_det_euc || is_det_euc7) ?  e_in(f) :	/* euc	   */
    	    (is_det_latin ?  e_in(f) :		/* latin	   */
    	    (is_det_utf8 ?  z_in(f) :		/* utf8		   */
    	    (is_det_utf7 ?  y_in(f) :		/* utf7		   */
    	    (is_det_ucod ?  u_in(f) :		/* Unic*de	   */
    	    (is_det_brgt ?  b_in(f) :		/* B-right	   */
    	    (is_det_transp ?  t_in(f) :		/* Transparent	   */
   	    (is_det_keis ? ks_in(f) : s_in(f)))))))))); /* microsoft/keis*/
}

/* --------------------------------------------------------------- */
/*@-paramuse@*/
static void dump_name_of_code(was_ascii,out,has_bom)
int	was_ascii,out;
int	has_bom;
{
    char *codename;

    if (was_ascii) in_codeset = codeset_ascii;

#ifdef SWIG_EXT
    if (in_codeset >= 0) {
    	codename = i_codeset[in_codeset].cname;
	if (codename == NULL) 
	    codename = i_codeset[in_codeset].desc;
    } else codename = "DEFAULT_CODE";
    SKFSTROUT(codename);
#else
    if (in_codeset >= 0) {
	if (input_soft_inquiry) {
	    codename = i_codeset[in_codeset].cname;
	    if (codename == NULL)
		codename = i_codeset[in_codeset].desc;
	} else {
	    codename = i_codeset[in_codeset].desc;
	};
    } else codename = dcode_name;

    if (out == 1) {
    	fprintf(stderr,"%s",codename);
    } else {
    	SKFSTROUT(codename);
    };	
    if (is_det_ucod || is_det_utf8) {
	if (out == 1) {
	    if (has_bom) fprintf(stderr,"-BOM");
	} else {
	    if (has_bom) SKFSTROUT("-BOM");
	};	
    };
    if (is_det_ucod) {
	if (in_endian) codename = utf16be_name;
	else codename = utf16le_name;
	if (out == 1) {
	    fprintf(stderr,"%s",codename);
	} else {
	    SKFSTROUT(codename);
	};	
    };
#endif
}
/* --------------------------------------------------------------- */
void dump_name_of_lineend(le_detect,mode)
int le_detect;
int mode;
{
    FILE *f;

    if (mode == 0) f = stdout;
    	else	   f = stderr;

    if (le_detect) {
    	fprintf(f," (%s%s%s%s)",
	    (detect_cr && first_detect_cr) ? "CR":"",
	    (detect_lf) ? "LF":"",
	    (detect_cr && !first_detect_cr) ? "CR":"",
	    (is_dummy_ledet && !detect_cr && !detect_lf) ? "DMY":"");
    } else {
    	fprintf(f," (--)");
    };
}
/* --------------------------------------------------------------- */
void set_le_parse(cr,lf,crf)
int cr,lf,crf;
{
	if (cr) set_detect_cr;
	if (lf) set_detect_lf;
	if (crf) set_first_detect_cr;
}
/* --------------------------------------------------------------- */
unsigned long skf_get_valid_langcode(ilang,glang)
unsigned long ilang,glang;
{
    unsigned long il,gl;

    il = skf_get_langcode(ilang);
    gl = skf_get_langcode(glang);

    if (skf_neutral_lang(gl)) return(il);
    else return(gl);
}
/* --------------------------------------------------------------- */
