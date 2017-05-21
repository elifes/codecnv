/* *******************************************************************
** Copyright (c) 1999-2015 Seiji Kaneko.  All rights reserved.
** Everyone is permitted to use this program in source and binary
** form, with or without modification if and only if the following
** conditions are met:
** 1. Redistributions of source code must retain the above copyright
**   notice, copyright notices written in source code, additional list
**   of conditions and the following disclaimer.
** 2. Redistributions in machine readable form must reproduce the 
**   above copyright notice and the following disclaimer in the
**   documentation or other material provided with the distribution.
** 3. Neither the name of the copyright holders nor the names of its 
**   contributors may be used to endorse or promote products derived from 
**   this software without specific prior written permission.
**********************************************************************
** Disclaimer: This software is provided and distributed AS iS, 
**	without any implicit or explicit warranties, and not
**	guaranteed to be error-free. In no event shall the author be
**	liable for any direct, indirect or incidental damages,
**	including, but not limited to, loss of data, use or profits
**	responsibility for any direct or indirect damages or results
**	arising by using whole or a part of this software.
**********************************************************************
    ucodoconv.c: output converters for Unicode(TM) variants
	v1.90	Newly written from unic*de.c parsing
    $Id: ucodoconv.c,v 1.84 2017/01/05 15:05:48 seiji Exp seiji $
**/

#include <stdio.h>
#include "skf.h"
#include "skf_fileio.h"
#include "convert.h"
#include "oconv.h"
#include "in_code_table.h"

#define  SKFUCODSW	TRUE

static unsigned char utf7_base[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};

/* --- utf7 specific parameters ---------------------------------- */
static int	utf7_res = 0;	/* utf-7 amari			   */
       int	utf7_res_bit = 0; /* amari bit rength		   */

static void SKFUTF7ENCODE P_((skf_ucode));
static void encode_pslenset P_((int *,int *,skf_ucode));

static char ucod_cbuf[32];	/* uri conversion buffer	   */

#ifdef ACE_SUPPORT
static int  is_prohibit P_((skf_ucode));
#endif

/* --- macros ---------------------------------------------------- */
#define utf8put4(x) SKFputc((int)(ZF4VAL + ((x & ZL4MSK) >> 18))); \
    SKFputc((int)(ZF2VAL + ((x & ZL43MSK) >> 12)));\
    SKFputc(ZF2VAL + ((x & ZL2MSK) >> 6)); \
    SKFputc((x & Z1MSK) | ZCVAL);

#define utf8put3(x) SKFputc(ZF3VAL + ((x & ZL3MSK) >> 12));\
    SKFputc(ZF2VAL + ((x & ZL2MSK) >> 6)); \
    SKFputc((x & Z1MSK) | ZCVAL);

#define utf8put2(x) SKFputc(ZF23VAL + ((x & ZL23MSK) >> 6)); \
	SKFputc((x & Z1MSK) | ZCVAL);

#define UCS2PUT2(x,y) {if (out_endian(conv_cap)) \
	    { SKFputc(x); SKFputc(y); \
	    } else { SKFputc(y); SKFputc(x); }; }

#define UCS4PUT4(x,y,p,q) {if (out_endian(conv_cap)) \
	    { SKFputc(x); SKFputc(y); SKFputc(p); SKFputc(q); \
	    } else { SKFputc(q); SKFputc(p); SKFputc(y); SKFputc(x);}; }

/* *****************************************************************
** Unified Unicode output Converters
**
*******************************************************************
** Conversion Note
**      Codes are converted into unicode with mode at the first
**      hand, and then converted into appropriate and/or specified
**      code. Unlike nkf, this filter intends to convert text to
**      human readable form, and transparancy of conversion isn't
**      guaranteed. If you want to find out when this kind of 
**      modifications occur, specify -I option.
****************************************************************** */
/* --- unified output 1 ------------------------------------------ */
/* --- c1 must be 0 <= c1 < 0x10000 ------------------------------ */
static void SKFUNI1OUT(c3)
skf_ucode	c3;
{
    int		c1,c2;

    if (is_ucs_utf8(conv_cap)) {
	if (c3 < 0x80) {
	    SKFputc(c3); 
	} else if (c3 < 0x800) {
	    utf8put2(c3);
	} else {
	    utf8put3(c3);
	};
    } else if (is_ucs_utf16(conv_cap)) {
    	if (is_ucs_utf32(conv_cap)) {
	    c1 = c3 & 0xff;
	    c2 = (c3 >> 8) & 0xff;
	    UCS4PUT4(0,0,c2,c1);
	} else {
	    c1 = c3 & 0xff;
	    c2 = (c3 >> 8) & 0xff;
	    UCS2PUT2(c2,c1);
	};
#ifdef ACE_SUPPORT
    } else if (is_ucs_puny(conv_cap)) {
    	if (!is_prohibit(c3) || is_puny_strdelim(c3)) o_p_encode(c3);
	else out_undefined(c3,SKF_OUT_PROHIBIT);
		/* idn prohibit table (rfc3454) is discarded	   */
#endif
    } else if (	/* UTF7: this routine loops due to 0x26	*/
	   ((c3 >= 0x21) && (c3 <= 0x26))
	|| ((c3 >= 0x2a) && (c3 <= 0x2b))
	|| ((c3 >= 0x3b) && (c3 <= 0x3e))
	|| ((c3 >= 0x5b) && (c3 <= 0x60))
	|| (c3 >= 0x7b)) {	/* must be encoded		*/
	if (!is_utf7_shift) {
	    set_utf7_shift; SKFputc('+');
	    utf7_res_bit = 0;
	};
	SKFUTF7ENCODE(c3);
    } else {
	if (is_utf7_shift) {
	    if (utf7_res_bit != 0) {
		SKFputc(utf7_base[utf7_res]);
	    };
	    utf7_res_bit = 0;
	    reset_kanji_shift; SKFputc('-');
	};
	SKFputc(c3);
    };
}
/* --- unified output multiple ----------------------------------- */
/* --- this routine is limited to ucs2 range for performance ----- */
/* --- string output utilities ----------------------------------- */
void SKFUNISTROUT(st)
const char *st;
{
    int len;
    for (len=0; (len<30) && (*st != '\0') ; len++,st++) {
	SKFUNI1OUT((skf_ucode)(*st));
    };
}

/* --- ascii output ---------------------------------------------- */
void UNI_ascii_oconv(ch)
    skf_ucode	ch;
{
    int		c1,c3;

    c1 = ch & 0xff;	/* guarding				   */
    c3 = uni_o_ascii[c1];
#ifdef SKFDEBUG
    if (is_vv_debug) {
    	fprintf(stderr," uni_ascii:%02x",ch);
	debugcharout((int)c3);
    };
#endif
    if (o_encode) {
    	out_UNI_encode(ch,c3);
	if (is_ucs_utf16(conv_cap) && is_lineend(ch)) {
	    SKFrputc(ch & 0xff);
	    return;
	} else;
    } else;

    if ((c1 == A_SI) || (c1 == A_SO)) {
	return;			/* just discard them		   */
    } else if (c3 == 0) {
	if (c1 < A_SP) {
	    c3 = c1; 
	} else;		/* DUNNO			   */
    } else;
    if ((c3 != 0) || (c1 == 0)) {
	SKFUNI1OUT(c3);
    } else {
    	skf_lastresort(c1);
    };
}

/* --- latin output ---------------------------------------------- */
/*  handles 0xa0-0x2fff						   */
void UNI_latin_oconv (ch)
    skf_ucode    ch;
{
    int c1,c2;

    c1 = (ch & 0xff);
    c2 = (ch >> 8) & 0xff;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," uni_latin:%04x",ch);
#endif
    if (o_encode) out_UNI_encode(ch,ch);

    if (is_ucs_utf16(conv_cap)) {
    	if (is_ucs_utf32(conv_cap)) {
	    UCS4PUT4(0,0,c2,c1);
	} else {
	    UCS2PUT2(c2,c1);
	};
    } else if (is_ucs_utf8(conv_cap)) {
	if (ch < 0x80) {
	    SKFputc(ch); 
	} else if (ch < 0x800) {
	    utf8put2(ch);
	} else {
	    utf8put3(ch);
	};
#ifdef ACE_SUPPORT
    } else if (is_ucs_puny(conv_cap)) {
    	if (!is_prohibit(ch)) o_p_encode(ch);
	else out_undefined(ch,SKF_OUT_PROHIBIT);
		/* idn prohibit table (rfch454) is discarded	   */
#endif
    } else if (is_ucs_utf7(conv_cap)) {
	if (!is_utf7_shift) {
	    set_utf7_shift; SKFputc('+');
	};
	SKFUTF7ENCODE(ch);
    };
}

/* --- CJK kana plane -------------------------------------------- */
/* 0x3000 - 0x4dff */
void UNI_cjkkana_oconv (ch)
    skf_ucode    ch;
{
    int	c2,c3;
    unsigned long nc;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," uni_cjkkana:%04x",ch);
#endif
    if (o_encode) out_UNI_encode(ch,ch);

    if ((ch == 0x3000) && (!(sup_space_conv))) {
	UNI_ascii_oconv((skf_ucode)(A_SP));
	if (!(is_spconv_1)) UNI_ascii_oconv((skf_ucode)(A_SP));
	return;
    } else if (ch >= CJK_A_TBL_START) {
	if (((out_codeset == codeset_nyukan_utf16) ||
		(out_codeset == codeset_nyukan_utf8)) 
		&& (uni_o_cjk_a != NULL)) {
	    nc = uni_o_cjk_a[ch - CJK_A_TBL_START];
	    if (nc == 0) {
		out_undefined(ch,SKF_OUNDEF);
		return;
	    } else {
	    	ch = (skf_ucode)nc;
	    };
	} else if (sup_cjk_ext_a) {
	    out_undefined(ch,SKF_OUNDEF);
	    return;
	};
    } else if (use_ms_compat && ((ch == 0x3099) || (ch == 0x309a))) {
	ch += 2;
    };
    if (is_ucs_utf16(conv_cap)) {
	c2 = ch & 0xff;
	c3 = (ch >> 8) & 0xff;
    	if (is_ucs_utf32(conv_cap)) {
	    UCS4PUT4(0,0,c3,c2);
	} else {
	    UCS2PUT2(c3,c2);
	};
    } else if (is_ucs_utf8(conv_cap)) {
	utf8put3(ch);
    } else if (is_ucs_utf7(conv_cap)) {
	if (!is_utf7_shift) {
	    set_utf7_shift; SKFputc('+');
	};
	SKFUTF7ENCODE(ch);
#ifdef ACE_SUPPORT
    } else if (is_ucs_puny(conv_cap)) {
    	if (!is_prohibit(ch)) o_p_encode(ch);
	else out_undefined(ch,SKF_OUT_PROHIBIT);
		/* idn prohibit table (rfc3454) is discarded	   */
#endif
    } else;
}

/* --- CJK plane ------------------------------------------------- */
void UNI_cjk_oconv(c1)
skf_ucode c1;
{
    int	c3,c4;
    unsigned long nc;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," uni_cjk:%04x",c1);
#endif
    if (o_encode) out_UNI_encode(c1,c1);

    if (is_ucs_utf16(conv_cap)) {
	if ((out_codeset == codeset_nyukan_utf16) 
		&& (uni_o_kanji != NULL)) {
	    nc = uni_o_kanji[c1 - 0x4e00];
	    if (nc == 0) {
		out_undefined(c1,SKF_OUNDEF);
		return;
	    } else {
	    	c1 = (skf_ucode)nc;
	    };
	} else;

	c3 = (c1 >> 8) & 0xff;
	c4 = c1 & 0xff;

    	if (is_ucs_utf32(conv_cap)) {
	    UCS4PUT4(0,0,c3,c4);
	} else {
	    UCS2PUT2(c3,c4);
	};
    } else if (is_ucs_utf8(conv_cap)) {
	if ((out_codeset == codeset_nyukan_utf8) 
		&& (uni_o_kanji != NULL)) {
	    nc = uni_o_kanji[c1 - 0x4e00];
	    if (nc == 0) {
		out_undefined(c1,SKF_OUNDEF);
		return;
	    } else {
	    	c1 = (skf_ucode)nc;
	    };
	} else;

	utf8put3(c1);
    } else if (is_ucs_utf7(conv_cap)) {
	if (!is_utf7_shift) {
	    set_utf7_shift; SKFputc('+');
	};
	SKFUTF7ENCODE(c1);
#ifdef ACE_SUPPORT
    } else if (is_ucs_puny(conv_cap)) {
    	if (!is_prohibit(c1)) o_p_encode(c1);
	else out_undefined(c1,SKF_OUT_PROHIBIT);
		/* idn prohibit table (rfc3454) is discarded	   */
#endif
    };
}

/* --- ozone and non-BMP ----------------------------------------- */
void UNI_ozone_oconv(ch)
skf_ucode ch;
{
    long c3;
    int c1,c2,c4;
    unsigned long nc;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," uni_ozone:%06x",ch);
#endif
    if (o_encode) out_UNI_encode(ch,ch);

    if ((ch >= VARSEL_SUP_TBL_START) && (ch <= VARSEL_SUP_TBL_END)) {
    	if (!enable_var_cntl) return;	/* do nothing		   */
    } else;

    if ((out_codeset == codeset_nyukan_utf16) ||
	(out_codeset == codeset_nyukan_utf8)) {
	if ((ch >= CJK_B_TBL_START) && (ch <= CJK_B_TBL_END)
		&& (uni_o_cjk_b != NULL)) {
	    nc = uni_o_cjk_b[ch - CJK_B_TBL_START];
	} else if ((ch >= CJK_C_TBL_START) && (ch <= CJK_C_TBL_END)
		&& (uni_o_cjk_c != NULL)) {
	    nc = uni_o_cjk_c[ch - CJK_C_TBL_START];
	} else nc = 0;

	if (nc == 0) {
	    out_undefined(ch,SKF_OUNDEF);
	    return;
	} else ch = (skf_ucode)nc;
    } else;

    if (is_ucs_utf16(conv_cap)) {
    	if (is_ucs_utf32(conv_cap)) {
	    c3 = ch & 0xffU;
	    c2 = (ch >> 8) & 0xffU;
	    c1 = (ch >> 16) & 0xffU;
	    c4 = (ch >> 24) & 0xffU;
	    UCS4PUT4(c4,c1,c2,(int)c3);
	} else {
	    if ((ch <= UNI_HIGH_LIMIT) && (ch >= 0x10000) && !limit_ucs2) {
		/* generate surrogate pair			   */
		c3 = ((ch >> 10) - 0x040) & 0x3ffU;
		c2 = (int) (((c3 >> 8) & 0x3) + 0xd8);
		c4 = (int) (c3 & 0xffU);
		UCS2PUT2(c2,c4);
		c2 = ((ch >> 8) & 0x3) + 0xdc;
		c4 = ch & 0xff;
		UCS2PUT2(c2,c4);
	    } else if (ch > UNI_HIGH_LIMIT) {
		out_undefined(ch,SKF_UNSURG);
	    } else if (limit_ucs2 && (ch >= 0x10000)) {
		out_undefined(ch,SKF_UCOMPAT);
	    } else {	/* hangul anf Yi */
		c2 = (int) ((ch >> 8) & 0xff);
		c4 = (int) (ch & 0xff);
		UCS2PUT2(c2,c4);
	    };
	};
    } else if (is_ucs_utf7(conv_cap)) {
	if ((ch <= UNI_HIGH_LIMIT) && (ch >= 0x10000) && !limit_ucs2) {
	    /* generate surrogate pair			   */
	    c3 = ((ch >> 10) - 0x040) & 0x3ff;
	    c2 = (int) (((c3 >> 8) & 0x3) + 0xd8);
	    c4 = (int) (c3 & 0xff) + (c2 << 8);
	    SKFUTF7ENCODE(c4);
	    c2 = ((ch >> 8) & 0x3) + 0xdc;
	    c4 = (ch & 0xff) + (c2 << 8);
	    SKFUTF7ENCODE(c4);
	} else if (ch > UNI_HIGH_LIMIT) {
	    out_undefined(ch,SKF_UNSURG);
	} else if (limit_ucs2 && (ch >= 0x10000)) {
	    out_undefined(ch,SKF_UCOMPAT);
	} else {	/* hangul anf Yi */
	    if (!is_utf7_shift) {
		set_utf7_shift; SKFputc('+');
	    };
	    SKFUTF7ENCODE(ch);
	};
#ifdef ACE_SUPPORT
    } else if (is_ucs_puny(conv_cap)) {
    	if (!is_prohibit(ch)) o_p_encode(ch);
	else out_undefined(ch,SKF_OUT_PROHIBIT);
		/* idn prohibit table (rfc3454) is discarded	   */
#endif
    } else {
	if ((ch <= UNI_HIGH_LIMIT) && (ch >= 0x10000) && !limit_ucs2) {
	    utf8put4(ch);
	} else if ((ch < 0xd800) && (ch >= 0xa000)) {
	    utf8put3(ch);
	} else 
	    out_undefined(ch,SKF_UCOMPAT);
    };
}


/* --- private plane --------------------------------------------- */
/* - 0xd800 - 0xf8ff --------------------------------------------- */
void UNI_private_oconv(c1,rc2)
skf_ucode c1;
skf_ucode rc2;
{
    int	c3,c4;
    unsigned long nc;
#if defined(ARIBINTSUPPORT)
    skf_ucode *pmap;
    skf_ucode cnvc;
#endif

    c3 = (c1 >> 8) & 0xff;
    c4 = c1 & 0xff;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," uni_priv:%04x",c1);
#endif

    if ((out_codeset == codeset_nyukan_utf16) ||
	(out_codeset == codeset_nyukan_utf8)) {
	if ((c1 >= PRV_TBL_START) && (c1 < CPT_TBL_START)
		&& (uni_o_prv != NULL)) {
	    nc = uni_o_prv[c1 - PRV_TBL_START];
	} else nc = c1;

	if (nc == 0) {
	    out_undefined(c1,SKF_OUNDEF);
	    return;
	} else c1 = (skf_ucode)nc;
    } else;

    if (o_encode) out_UNI_encode(c1,c1);

    if (c1 < 0xe000) {
	lig_x0213_out(c1,rc2);
	return;
#if defined(ARIBINTSUPPORT)
    } else if ((out_codeset == codeset_aribb24u)
    	    || (out_codeset == codeset_aribb24z)) {
	if (arib_bmp0_convert) {
	    pmap = ovlay_byte_defs[arib_bmp_index].uniltbl;
	} else if (arib_bmp1_convert) {
	    pmap = ovlay_byte_defs[arib_bmp1_index].uniltbl;
	} else pmap = ovlay_byte_defs[arib_bmp2_index].uniltbl;
	if ((cnvc = pmap[c1 - 0xe000]) != 0) {
	    post_oconv(cnvc);
	    return;
	} else;
#endif
    } else;
    if (is_ucs_utf16(conv_cap)) {
    	if (is_ucs_utf32(conv_cap)) {
	    UCS4PUT4(0,0,c3,c4);
	} else {
	    UCS2PUT2(c3,c4);
	};
    } else if (is_ucs_utf7(conv_cap)) {
	if (!is_utf7_shift) {
	    set_utf7_shift; SKFputc('+');
	};
	SKFUTF7ENCODE(c1);
#ifdef ACE_SUPPORT
    } else if (is_ucs_puny(conv_cap)) {
    	if (!is_prohibit(c1) || force_private_idn_o) o_p_encode(c1);
	else out_undefined(c1,SKF_OUT_PROHIBIT);
		/* idn prohibit table (rfc3454) is discarded	   */
#endif
    } else {
	utf8put3(c1);
    };
}

/* --- compatibility plane --------------------------------------- */
/* --- U-F900 to U-FFFF ------------------------------------------ */
void UNI_compat_oconv (c1)
    skf_ucode    c1;
{ 
    skf_ucode	c3,c4,c5;
    int		u0,u1;
    int		c2;
    unsigned long nc;

    if ((out_codeset == codeset_nyukan_utf16) ||
	(out_codeset == codeset_nyukan_utf8)) {
	if ((c1 >= CPT_TBL_START) && (c1 <= CPT_TBL_END)
		&& (uni_o_compat != NULL)) {
	    nc = uni_o_compat[c1 - CPT_TBL_START];
	} else nc = c1;

	if (nc == 0) {
	    out_undefined(c1,SKF_OUNDEF);
	    return;
	} else c1 = (skf_ucode)nc;
    } else;

    c2 = (c1 & 0xff);
    c3 = (c1 >> 8) & 0xff;
    u0 = (ucode_undef >> 8) & 0xff;
    u1 = ucode_undef & 0xff;
    c4 = u1;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," ucod_compat:%04x",c1);
#endif
    if (o_encode) out_UNI_encode(c1,c1);

    if ((c3 == 0xfe) && (c2 < 0x10)) { /* variation selector	   */
    	if (!enable_var_cntl) return;	/* do nothing		   */
    } else if (nkf_compat && (c1 == 0xffe5)) {
    		/* nkf doesn't use fullwidth Yen sign		   */
    	post_oconv(0xa5);
	return;
    } else if (c1 >= 0xfffe) {
    	out_undefined(c1,SKF_OUNDEF);
	return;
    } else if ((use_compat) && 
	((!sup_cjk_cmp) || (c1 < 0xfa30) || (c1 > 0xfa6a))) {
	    c4 = c2;
    } else if ((c3 >= 0xf9) && (c3 < 0xfb)) { /* CJK compat kanji  */
	cjk_compat_parse(c1);
	return;		/* suppress output			   */
    } else if (c3 != 0xff) {	/* NOT width variants		   */
	c3 = u0; 
/* --- 0xffxx ---- */
    } else if (c2 < 0x5f) {
    	c3 = 0; c4 = c2 + 0x20U;
    } else if ((c2 > 0x61) && (c2 < 0xa0)) {
	c3 = 0x30; c4 = uni_t_x201[c2-0x61];
    } else if ((c2 >= 0xa0) && (c2 <= 0xdf)) {
	if (c2 == 0xa0) { c3 = 0x31; c4 = 0x64; }
	else if (c2 <= 0xbf) { c3 = 0x31; c4 = c2 - 0x70; }
	else if ((c2 >= 0xc2) && (c2 <= 0xc7)) { 
		c3 = 0x31; c4 = c2 - 0x6d; }
	else if ((c2 >= 0xca) && (c2 <= 0xcf)) { 
		c3 = 0x31; c4 = c2 - 0x6b; }
	else if ((c2 >= 0xd2) && (c2 <= 0xd7)) { 
		c3 = 0x31; c4 = c2 - 0x69; }
	else if ((c2 >= 0xda) && (c2 <= 0xdf)) { 
		c3 = 0x31; c4 = c2 - 0x67; }
	else { c3 = u0; };
    } else if ((c2 >= 0xe0) && (c2 <= 0xef)) {
	lig_compat(c1);
	return;
    } else if (c2 == 0xfd) {
	c3 = 0xff; c4 = 0xfd;
    } else {
	c3 = u0; 
    };

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr,"-%02x,%02x",c3,c4);
#endif
    if (is_ucs_utf16(conv_cap)) {
    	if (is_ucs_utf32(conv_cap)) {
	    UCS4PUT4(0,0,c3,c4);
	} else {
	    UCS2PUT2(c3,c4);
	};
    } else if (is_ucs_utf8(conv_cap)) {
	c5 = (c3 << 8) + c4;
	if (c5 < 0x80) {
	    SKFputc(c5); 
	} else if (c3 < 0x8) {
	    utf8put2(c5);
	} else {
	    utf8put3(c5);
	};
    } else if (is_ucs_utf7(conv_cap)) {
	c5 = (c3 << 8) + c4;
	if ((c3 != 0)
	    || ((c4 >= 0x21) && (c4 <= 0x26))
	    || ((c4 >= 0x2a) && (c4 <= 0x2b))
	    || ((c4 >= 0x3b) && (c4 <= 0x3e))
	    || ((c4 >= 0x5b) && (c4 <= 0x60))
	    || (c4 >= 0x7b)) {	/* must be encoded		*/
	    if (!is_utf7_shift) {
		set_utf7_shift; SKFputc('+');
	    };
	    SKFUTF7ENCODE(c5);
	} else {
	    if (is_utf7_shift) {
		if (utf7_res_bit != 0) {
		    SKFputc(utf7_base[utf7_res]);
		};
		utf7_res_bit = 0;
		reset_kanji_shift; SKFputc('-');
	    };
	    SKFputc(c4);
	};
#ifdef ACE_SUPPORT
    } else if (is_ucs_puny(conv_cap)) {
    	if (!is_prohibit(c1)) o_p_encode(c1);
	else out_undefined(c1,SKF_OUT_PROHIBIT);
		/* idn prohibit table (rfc3454) is discarded	   */
#endif
    };
}

/* *****************************************************************
** UCS2 specific Converters
****************************************************************** */

/* --- end process ----------------------------------------------- */
void ucod_finish_procedure()
{
    oconv_flush();
    return;		/* do nothing				   */
}
/* *****************************************************************
** UTF7 specific Converters
****************************************************************** */
void SKFUTF7ENCODE(c2)
skf_ucode c2;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," code: %x, residue:%x(%d)",
			c2,utf7_res,utf7_res_bit);
#endif
    if (utf7_res_bit == 0) {
	utf7_res_bit = 4;
	SKFputc(utf7_base[(c2 & 0xfc00) >> 10]);
	SKFputc(utf7_base[(c2 & 0x03f0) >> 4]);
	utf7_res = (c2 & 0x000f) << 2;
    } else if (utf7_res_bit == 4) {
	utf7_res_bit = 2;
	SKFputc(utf7_base[((c2 & 0xc000) >> 14) + utf7_res]);
	SKFputc(utf7_base[(c2 & 0x3f00) >> 8]);
	SKFputc(utf7_base[(c2 & 0x00fc) >> 2]);
	utf7_res = (c2 & 0x0003) << 4;
    } else {	/* utf_res_bit == 2 */
	utf7_res_bit = 0;
	SKFputc(utf7_base[((c2 & 0xf000) >> 12) + utf7_res]);
	SKFputc(utf7_base[(c2 & 0x0fc0) >> 6]);
	SKFputc(utf7_base[c2 & 0x003f]);
    };
}

/* --- end process ----------------------------------------------- */
void utf7_finish_procedure()
{
    oconv_flush();
    if (utf7_res_bit != 0) {
	SKFputc(utf7_base[utf7_res]);
    };
    if (is_kanji_shift) {
	reset_kanji_shift;
	SKFputc('-');
    };
}

/* *****************************************************************
** UTF8 specific Converters
**
****************************************************************** */
/* --- end process ----------------------------------------------- */
void utf8_finish_procedure()
{
    oconv_flush();
    return;		/* do nothing				   */
}

/* --- output utilities ------------------------------------------ */
/*@-statictrans@*/
char *utf8_urioutstr(x)
skf_ucode x;
{
    if (x < 0x80) {
    	(void)snprintf(ucod_cbuf,(size_t)1,"%c",x);
    } else if (x < 0x800) {
	(void)snprintf(ucod_cbuf,(size_t)6,"\%02x\%02x",
		(ZF23VAL + ((x & ZL23MSK) >> 6)),
		((x & Z1MSK) | ZCVAL));
    } else if (x < 0x10000) {
	(void)snprintf(ucod_cbuf,(size_t)9,"\%02x\%02x\%02x",
		(ZF3VAL + ((x & ZL3MSK) >> 12)),
		(ZF2VAL + ((x & ZL2MSK) >> 6)),
		((x & Z1MSK) | ZCVAL));
    } else if ((x <= 0x10ffff) && (x >= 0x10000) && !limit_ucs2) {
	(void)snprintf(ucod_cbuf,(size_t)12,"\%02lx\%02lx\%02x\%02x",
		(ZF4VAL + ((x & ZL4MSK) >> 18)),
		(ZF2VAL + ((x & ZL43MSK) >> 12)),
		(ZF2VAL + ((x & ZL2MSK) >> 6)),
		((x & Z1MSK) | ZCVAL));
    } else ;

    return(ucod_cbuf);
}

void utf8_uriout(x)
skf_ucode x;
{
    char *sym;

    if ((sym = utf8_urioutstr(x)) != NULL) {
    	while (*sym == '\0') {
	    SKFputc(*sym++);
	};
    } else {
	out_undefined(x,SKF_OUNDEF);
    };
}

#ifdef ACE_SUPPORT
/* --------------------------------------------------------------- */
/* idn prohibition table (rfc3454 stringprep)			   */
/* return 1 if prohibited.					   */
/* --------------------------------------------------------------- */
int is_prohibit(ch)
skf_ucode ch;
{
    /* spaces */
    if (ch <= A_SP) return(1);
    if ((ch == A_KSP) || (ch == 0x1680)) return(1);
    else if (((ch >= 0x2000) && (ch <= 0x200f))
	|| ((ch >= 0x2028) && (ch <= 0x202f))
    	|| (ch == 0x205f) || (ch == 0x3000)) return(1);
    /* controls */
    else if ((ch == A_DEL) || (ch == 0x06dd) || (ch == 0x070f)
	|| (ch == 0x180e) || (ch == 0xfeff) 
	|| ((ch >= 0x2060) && (ch <= 0x2063))
	|| ((ch >= 0x206a) && (ch <= 0x206f))
	|| ((ch >= 0xfff9) && (ch <= 0xfffc))
	|| ((ch >= 0x1d173) && (ch <= 0x1d17a))
	) {
	return(1);
    /* privates */
    } else if (((ch >= 0xe000) && (ch <= 0xf8ff))
	|| ((ch >= 0xf0000) && (ch <= 0xffffd))
	|| ((ch >= 0x100000) && (ch <= 0x10fffd))
	) {
	return(1);
    /* non-character */
    } else if (((ch >= 0xfdd0) && (ch <= 0xfdef))
	|| ((ch & 0xfffe) == 0xfffe)) {
	return(1);
    /* Note; surrogate never comes here */
    /* inappropiate for canonical representation */
    } else if ((ch >= 0x2ff0) && (ch <= 0x2ffb)) {
	return(1);
    /* change display properties */
    } else if (((ch >= 0x0340) && (ch <= 0x0341))
	|| ((ch & 0xfffe) == 0xfffe)) {
	return(0);
    /* language tags never comes here - is eaten in in_encoder	  */
    } else return(0);
}
#endif

/* --------------------------------------------------------------- */
/* out_ucod_encode: MIME folding detection			   */
/*  (1) if position already reached limits, then fold.		   */
/*  (2) if character and position reaches limit, then fold.	   */
/* Note MIME respects character bounds, MIME does not consider	   */
/* about named character sequences that skf doesn't handle as one. */
/* Note: UTF-7 never MIMEs. Just ignore if those.		   */
/* --------------------------------------------------------------- */
void encode_pslenset(plenp,slenp,c)
int *plenp;
int *slenp;
skf_ucode c;
{
    int c1,c2;

    if (is_lineend(c)) {
    	*plenp = 0; *slenp = 0;
    } else {
	if (is_ucs_utf16(conv_cap)) {
	    if (!is_ucs_utf32(conv_cap) && (c >= 0x10000)) {
		*slenp = *slenp + 2;	/* surrogate */
		c1 = ((c >> 10) - 0x40) & 0xff;
		c2 = (c & 0xff);
		if (mime_safe(c1)) *plenp = *plenp + 1;
		else *slenp = *slenp + 1;
		if (mime_safe(c2)) *plenp = *plenp + 1;
		else *slenp = *slenp + 1;
	    } else {
		c1 = (int)(c & 0xffUL);
		c2 = (int)((c >> 8) & 0xffUL);
		if (mime_safe(c1)) *plenp = *plenp + 1;
		else *slenp = *slenp + 1;
		if (mime_safe(c2)) *plenp = *plenp + 1;
		else *slenp = *slenp + 1;
		if (is_ucs_utf32(conv_cap)) *slenp += 2;
	    };
	} else if (is_ucs_utf8(conv_cap)) {
	    if (mime_safe(c)) {*plenp = 1; *slenp = 0;
	    } else if (c < 0x80) {*plenp = 0; *slenp = 1;
	    } else if (c < 0x800) {*plenp = 0; *slenp = 2;
	    } else if (c < 0x10000) {*plenp = 0; *slenp = 3;
	    } else {*plenp = 0; *slenp = 4; };
	};
    };
    return;
}

/* --------------------------------------------------------------- */
/* encoder main(1) - encode presetting				   */
/*  output codeset is drawn from out_codeset			   */
/* called at genoconv and generate header			   */
/* Note: encode capability consistency is not checked. Those stuff */
/*  should be tested in earlier processes.			   */
/* --------------------------------------------------------------- */
void out_UNI_encode(ch,cc)
skf_ucode ch;		/* RAW unicode character */
skf_ucode cc;		/* table lookup result */
{
    skf_ucode c1,c3;
    int plen = 0;	/* character count within alphanum.	   */
    int slen = 0;	/* controls and GR's			   */

    if (ch >= 0) {	/* has character to output		   */
    	if (cc != 0) {	/* not suppressed character	   */
	    if (is_lineend(cc)) {
	    	;
	    } else if ((cc < 0xd800) 
	    		|| ((cc >= 0xe000) && (cc < 0xf900))) {
	    	/* BSP and not surrogate nor compat plane	   */
		encode_pslenset(&plen,&slen,cc);
		(void)mime_clip_test(plen,slen);
	    } else if (cc >= 0x10000) {
		encode_pslenset(&plen,&slen,cc);
		(void)mime_clip_test(plen,slen);
	    } else if (cc < 0xe000) { /* surrogate - special area  */
		/* X0213 ligature count is processed within parser,*/
		/* we just care about current cursor position.	   */
		(void)mime_clip_test(0,0);
	    } else {	/* 0xf900 - 0xffff			   */
	    	c1 = (skf_ucode)(cc & 0xffL);
	    	c3 = (skf_ucode)((cc >> 8) & 0xffL);
		if ((c3 == 0xfe) && (c1 < 0x10)) {
					/* variation selector	   */
		    if (enable_var_cntl) 
			encode_pslenset(&plen,&slen,cc);
		} else if ((use_compat) && 
		    ((!sup_cjk_cmp) || (cc < 0xfa30) || (cc > 0xfa6a))) {
		    encode_pslenset(&plen,&slen,cc);
		} else if ((c3 >= 0xf9) && (c3 < 0xfb)) {
		/* FIXME */
		    cjk_compat_parse(cc);
		    return;		/* suppress output	   */
		} else if (c3 != 0xff) {/* NOT width variants	   */
		    ;			/* do nothing		   */
	    /* --- 0xffxx ---- */
		} else if (c1 < 0x5f) {	/* convert into plain ascii */
		    encode_pslenset(&plen,&slen,(skf_ucode)(c1 + 0x20));
		} else if ((c1 > 0x61) && (c1 < 0xa0)) {
		    encode_pslenset(&plen,&slen,
		    	(skf_ucode)(0x3000UL + uni_t_x201[c1-0x61]));
		} else if ((c1 >= 0xa0) && (c1 <= 0xdf)) {
		    if (c1 == 0xa0) {
			encode_pslenset(&plen,&slen,0x3164);
		    } else if ((c1 <= 0xbf)
		    	|| ((c1 >= 0xc2) && (c1 <= 0xc7))  
		    	|| ((c1 >= 0xca) && (c1 <= 0xcf))
		    	|| ((c1 >= 0xd2) && (c1 <= 0xd7))
		    	|| ((c1 >= 0xda) && (c1 <= 0xdf))) { 
				/* 31xx. xx is ascii	*/
			encode_pslenset(&plen,&slen,0x3164);
		    } else ;
		} else if ((c1 >= 0xe0) && (c1 <= 0xef)) {
		    /* ligature is processed through post_oconv	   */
		    /* we don't need to count here.		   */
		    /* Strictly it is not corrent, but go anyway   */
		    return;
		} else if (c1 == 0xfd) {
		    encode_pslenset(&plen,&slen,cc);
		} else {
		    encode_pslenset(&plen,&slen,ucode_undef);
		};
	    };
	    (void)mime_clip_test(plen,slen);
	} else {	/* ASCII and suppressed case		   */
			/* Note: cr/lf comes here.		   */
	    if (is_lineend(ch) && is_ucs_utf16(conv_cap)) {
	    	utf16_clipper(ch);
		return;
	    } else if ((ch != A_SI) && (ch != A_SO)) {
	    	if (ch < A_SP) encode_pslenset(&plen,&slen,ch);
		else;		/* undefined. not processed here   */
	    } else;
	    (void)mime_clip_test(plen,slen);
	};
    } else {	/* if c1 < 0, output as it is.			   */
	(void)mime_clip_test(0,0);
    };
    return;
}
