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
    genoconv.c:	input/output converters seed
	v1.90	Newly written from unic*de.c parsing
    $Id: genoconv.c,v 1.143 2017/01/05 15:05:48 seiji Exp seiji $
**/

#include <stdio.h>
#include "skf.h"
#include "skf_fileio.h"
#include "convert.h"
#include "oconv.h"

#define  SKF@gen@SW	TRUE

#if defined(SKFEUCcSW) || defined(SKFJIScSW) || defined(SKFSJIScSW) \
  || defined(SKFKEIScSW) || defined(SKFBGcSW)
  	/* code counter */
#define SKF_CODECOUNT
#endif

/* *****************************************************************
** GENERIC Converters
**
** Code converter set:
**      s_oconv, j_oconv, e_oconv, u_oconv, z_oconv, k_oconv
**
*******************************************************************
** Conversion Note
**      Codes are converted into JIS code with mode at the first
**      hand, and then converted into appropriate and/or specified
**      code. Unlike nkf, this filter intends to convert text to
**      human readable form, and transparancy of conversion isn't
**      guaranteed. If you want to find out when this kind of 
**      modifications occur, specify -I option.
****************************************************************** */
void @gen@_cjk_oconv P_((skf_ucode));

#ifndef	SKF_CODECOUNT
/* --- string output utilities ----------------------------------- */
void SKF@gen@STROUT(st)
const char *st;
{
    int len;
    for (len=0; (len<30) && (*st != '\0') ; len++,st++) {
	SKF@gen@1OUT(*st);
    };
}
#endif
/* --- incantinate all macros ... -------------------------------- */

/* --- ascii output ---------------------------------------------- */
/* handle 0x00 - 0x7f -------------------------------------------- */
/* ch < 0 do not come here --------------------------------------- */
void @gen@_ascii_oconv (c1)
    skf_ucode    c1;
{
    skf_ucode	c3;

/* memo: this routine output extended EUC code when necessary	   */
    c3 = uni_o_ascii[c1];
#ifdef SKFDEBUG
    if (is_vv_debug) {
    	fprintf(stderr," @gen@_ascii:%02x,%02x(%02x)",
	    ((c1 >> 8) & 0xff),(c1 & 0xff),c3);
	debugcharout((int)c3);
    } else;
#endif
#if	defined(SKFKEISSW)
    if (c3 >= 0x100) {
	SKFKEISOUT(c3);
	return;
    } else {
	if (c3 != 0) {
	    SKFKEIS1OUT(c3);
	    return;
	} else {
	    if (c1 < A_SP) {
		SKFKEIS1OUT(c1); 
		return;
	    } else;	/* DUNNO				   */
	};
    };
#else	/* !KEIS */
    if (o_encode) {
    	out_@gen@_encode(c1,c3);
    } else;
    if (c3 < 0x8000) {
	if ((c3 > 0) && (c3 < 0x80)) {
#if	defined(SKFJISSW) || defined(SKFEUCSW)
	    r_SKF@gen@1OUT(c3);	/* macro variants		   */
#else
	    SKF@gen@1OUT(c3);
#endif
	    return;
#if	defined(SKFJISSW) && !defined(UNIFY_ASCII_JIS)
	/* note that this process is not true-1.92 compliant, but  */
	/* OK, because skf permit only legistimate codeset setting */
	} else if ((c1 == 0x5c) || (c1 == 0x7e)) {
	    if ((g0_char != 'B') && (!force_jis_pri)) {
					/* not ascii		   */
		SKFJIS1ASCOUT(c1);
	    } else {
		r_SKFJIS1OUT(c1);
	    };
	    return;
#endif
	} else if (c3 >= 0x100) {
	    SKF@gen@OUT(c3);
	    return;
	} else if (c3 <= 0) {
	    if (c3 < 0) {
	    	o_c_encode(c3);
		return;
	    } else if (c1 < A_SP) {
		SKF@gen@1OUT(c1); 
		return;
	    } else;		/* DUNNO			   */
#if	defined(SKFJISSW)
	} else if (use_iso8859) { /* read as pass thru flg */
	    SKFJIS8859OUT(c3);
	    return;
#endif
#if	defined(SKFBGSW)
	} else if (use_iso8859) { /* read as pass thru flg */
	    SKFBG1OUT(c3);
	    return;
#endif
	};
#if	defined(SKFJISSW)
    } else if (is_2ln(c3)) {
    	SKF@gen@8859OUT(c3);
	return;
#endif
#if	defined(SKFBGSW)
    } else if (is_johab(conv_cap) || is_big5fam(conv_cap)) {
#ifdef	FOLD_SUPPORT
	fold_count++;
#endif
	SKF@gen@OUT(c3); /* NON QUADRANT CASE: output as it is  */
	return;
#else /* !defined(SKFBGSW) */
    } else if (is_2pl(c3) && use_x0212) {
#ifdef	FOLD_SUPPORT
	fold_count++;
#endif
	SKF@gen@G3OUT(c3);
	return;
    } else if (is_3pl(c3)) {
#ifdef	FOLD_SUPPORT
	fold_count++;
#endif
	SKF@gen@G4OUT(c3);
	return;
#endif /* !defined(SKFBGSW) */
    } else;

#endif	/* !KEIS */

    skf_lastresort(c1);
}

/* --- latin output ---------------------------------------------- */
/*  handles 0x080-0xfff, 0x2000-0x2fff				   */
/* --------------------------------------------------------------- */
/*@-globstate@*/
void @gen@_latin_oconv (c4)
    skf_ucode    c4;
{
    skf_ucode	c2;
    int		c1;
    skf_ucode	c3 = 0;
    int		converted = 0;

    c1 = (c4 & 0xff);
    c2 = (c4 >> 8) & 0xff;
#if !defined(SKFKEISSW)
    if (o_encode) out_@gen@_encode(c4,c4);
#endif
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," @gen@_latin:%02x,%02x",c2,c1);
#endif

    if (c4 < 0x100) {
#if	defined(SKFSJISSW) || defined(UNIFY_ASCII_JIS)
        if (c1 == 0xa5) {	/* yen sign			   */
	    if (!force_jis_pri && !o_encode) {
		SKFJIS1OUT(0x5c);	/* JIS X-0201 convert	   */
		converted = 1;
	    };
	};
#else
#if	defined(SKFJISSW) 
        if (c1 == 0xa5) {	/* yen sign			   */
	    if (!(force_jis_pri) && (g0_char == 'J') && !o_encode) { 
		    SKFJIS1OUT(0x5c);	/* JIS X-0201 convert	   */
		    converted = 1;
	    };
	};
#endif
#endif
	if (converted == 0) {
	    if (uni_o_latin != NULL) {
		c3 = uni_o_latin[c1-0xa0];
	    };
	};
    } else if ((c2 >= 0x01) && (c2 <= 0x1f) /* 0x100 - 0x1f00	   */
	    && (uni_o_latin != NULL)) { /* smaller latin	   */
	c3 = uni_o_latin[c4 - 0xa0];
    } else if ((c2 >= 0x20) && (c2 < 0x30)  /* 0x2000 - 0x2f00	   */
		&& (uni_o_symbol != NULL)) {
	c3 = uni_o_symbol[c4 & 0xfff];	/* table lookup		   */
    };
#if !defined(SKFKEISSW)
    if (o_encode && (converted == 0)) out_@gen@_encode(c4,c3);
#endif
    if (c3 != 0) {
#if defined(SKFBGSW) || defined(SKFKEISSW)
	if (c3 < 0x100) { /* BG handler can eat 0x00-0xff   */
	    SKF@gen@1OUT(c3);
	} else {	
	    SKF@gen@OUT(c3);
	};
	converted = 1;
#else
	if (c3 < 0x8000) {
	    if (c3 >= 0x100) {
		SKF@gen@OUT(c3);
		converted = 1;
	    } else if (c3 < 0x80) {
		SKF@gen@1OUT(c3);
		converted = 1;
	    };
#if !defined(SKFSJISSW)
	} else if (is_2ln(c3)) {  /* always some kind of iso8859    */
	    SKFJIS8859OUT(c3);
	    converted = 1;
	} else if (is_2pl(c3) &&
		(use_x0212 || (!is_jis(conv_cap) && !is_euc(conv_cap)))) {
	    SKF@gen@G3OUT(c3);
	    converted = 1;
	} else if (is_3pl(c3)) {
	    SKF@gen@G4OUT(c3);
	    converted = 1;
#else
	} else if (is_2pl(c3) && 
			(is_ms_213c(conv_cap) || is_ms_cel(conv_cap))) {
	    if (is_vv_debug) {
		fprintf(stderr," 2P");
	    } else;
	    SKF@gen@G3OUT(c3);
	    converted = 1;
#endif	/* SKFSJISSW */
	} else;
#endif
    };
    if (converted == 0) {
#if defined(SKFJISSW) 
	if ((c4 < 0x100) && use_iso8859) { /* use only in iso8859-1 */
	    SKF@gen@8859OUT(c4);
	    converted = 1;
	} else
#endif
	if (use_latin2html) {
		converted = latin2html(c4);
	} else if (use_latin2tex) {
		converted = latin2tex(c4);
	};
    };
#ifdef SKFSJISSW
#endif
    if (converted == 0) {
	if (out_codeset == codeset_cp932w) {
	    out_undefined(c4,SKF_OUNDEF);
	} else if (c4 < 0x100) {
	    ascii_fract_conv(c1);
#if defined (SKFKEISSW)
	} else if (is_keis_jef(conv_cap) && ((c1 == 0xd2) || (c1 == 0xd4))) {
	    if (c1 == 0xd2) SKFKEISEOUT(0x7fda);
	    else SKFKEISEOUT(0x7fdb);
	    converted = 1;
#endif
	} else {
	    GRPH_lig_conv(c4);  
	};
    };
}

/* --- CJK kana plane -------------------------------------------- */
/*  handles 0x3000 - 0x4dff					   */
/* --------------------------------------------------------------- */
/*@-globstate@*/ /* should we have to check uni_o_kana is not NULL? */
void @gen@_cjkkana_oconv (c2)
    skf_ucode    c2;
{
    int c1;
    int	ch = 0;
    int converted = 0;

    c1 = c2 & 0x3ff; 		/* 1024				   */

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," @gen@_kana:%02x,%02x",
					((c2 >> 8) & 0xff),c1);
#endif

    if (c2 == 0x3000) {		/* Ideographic space 		   */
#if !defined(SKFKEISSW)
	if (o_encode) out_@gen@_encode(c2,c2);
#endif
	if (sup_space_conv) {
	    ch = uni_o_kana[c1];
	    SKF@gen@OUT(ch);
	} else {
	    SKF@gen@1OUT((skf_ucode)(A_SP));
	    if (!is_spconv_1) SKF@gen@1OUT((skf_ucode)(A_SP));
	};
	return;
    } else {
	if (c2 < 0x3400) { 	 /* 0x3000-0x3400		   */
	    if (uni_o_kana != NULL) {
		ch = uni_o_kana[c1];
	    };
	} else { 		/* hanguls and cjk extension-a	   */
	    if (uni_o_cjk_a != NULL) {
		ch = uni_o_cjk_a[c2 - 0x3400];
	    };
	};
#if !defined(SKFKEISSW)
	if (o_encode) out_@gen@_encode(c2,ch);
#endif
	if (ch != 0) {	/* has entry in output table	   */
#if defined(SKFBGSW) || defined(SKFKEISSW)
	    if (ch >= 0x100) {
		SKF@gen@OUT(ch);
	    } else {
		SKF@gen@1OUT(ch);
	    };
	    return;
#else
	    if (ch < 0x8000) {
		if (ch >= 0x100) {
#if defined(SKFEUCSW) || defined(SKFJISSW)
		    r_SKF@gen@OUT(ch);
#else
		    SKF@gen@OUT(ch);
#endif
		    return;
		} else if (ch < 0x80) {
		    SKF@gen@1OUT(ch);
		    return;
#if defined(SKFJISSW) 
		} else if (use_iso8859) { /* read as pass thru flg */
		    SKFJIS8859OUT(ch);
		    return;
#endif
#if !defined(SKFSJISSW) && !defined(SKFKEISSW)
		} else {
		    SKF@gen@G2OUT(ch);
		    return;
#endif
		};
#if	defined(SKFJISSW)
	    } else if (is_2ln(ch)) {
		SKF@gen@8859OUT(ch);
		return;
#endif
#if !defined(SKFSJISSW)
	    } else if (is_3pl(ch)) {
		SKF@gen@G4OUT(ch);
		return;
	    } else if (is_2pl(ch) && use_x0212) {
#else
	    } else if (is_2pl(ch) && 
			(is_ms_213c(conv_cap) || is_ms_cel(conv_cap))) {
#endif	/* SKFSJISSW */
#ifdef SKFDEBUG
		if (is_vv_debug) {
		    fprintf(stderr,"2P");
		} else;
#endif
		SKF@gen@G3OUT(ch);
		return;
	    } else;
#endif
	} else;
    };

    if (converted == 0) skf_lastresort(c2);
}

/* --- CJK plane ------------------------------------------------- */
/* handles 0x4e00 - 0x9fff					   */
/* --------------------------------------------------------------- */
void @gen@_cjk_oconv (c3)
    skf_ucode    c3;
{
    register skf_ucode	c4 = 0;
    int converted = 0;
#ifdef SKFDEBUG
    int	c1,c2;

    if (is_vv_debug) {
	c2 = (c3 >> 8) & 0xff;
	c1 = c3 & 0xff;
	fprintf(stderr," @gen@_cjk:%02x,%02x",c2,c1);
    };
#endif

/* assumes current codeset characteristics. No codeset has cjk's   */
/* in single-byte plane.					   */
    if (uni_o_kanji != NULL) {
	c4 = uni_o_kanji[c3 - 0x4e00];
#if !defined(SKFKEISSW)
	if (o_encode) out_@gen@_encode(c3,c4);
#endif
#if defined(SKFBGSW) || defined(SKFKEISSW)
	if (c4 >= 0x100) {
	    SKF@gen@OUT(c4); 
	    return;
	} else if (c4 != 0) {
	    SKF@gen@1OUT(c4); 
	    return;
	};
#else
	if (c4 >= 0x100) {
	    if (c4 < 0x8000) {
#if defined(SKFEUCSW) || defined(SKFJISSW)
		r_SKF@gen@OUT(c4);
#else
		SKF@gen@OUT(c4);
#endif
		return;
	    } else if (is_2pl(c4) && use_x0212) {
		SKF@gen@G3OUT(c4); return;
#if !defined(SKFSJISSW) && !defined(SKFKEISSW)
	    } else if (is_3pl(c4)) {
		SKF@gen@G4OUT(c4); return;
#endif
	    } else;
#if	defined(SKFJISSW)
	} else if (is_2ln(c4)) {
		SKF@gen@8859OUT(c4);
		return;
#endif
	} else if ((c4 != 0) && (c4 < 0x80)) {
	    SKF@gen@1OUT(c4); return;
#if !defined(SKFSJISSW) && !defined(SKFKEISSW)
	} else if (c4 > 0x80) {
	    SKF@gen@G2OUT(c4); return;
#endif
	};
#endif
    };

    if (converted == 0) skf_lastresort(c3);
}

/* --- o-zone and non-BMP area ----------------------------------- */
/* 0xa000 <= c2 < 0xd7ff or c2 >= 0x10000			   */
/* --------------------------------------------------------------- */
void @gen@_ozone_oconv(c2)
    skf_ucode	c2;
{
#if defined(SKFJISSW) || defined(SKFEUCSW) || defined(SKFBGSW) || defined(SKFSJISSW)
    int		converted = 0;
    int		ch = 0;

#ifdef SKFDEBUG
    skf_ucode	c3;
    int		c1;
#endif

    if (c2 == sFLSH) {
    	SKF@gen@1FLSH(); return;
    } else;

#ifdef SKFDEBUG
    c3 = (c2 >> 8) & 0xfff;
    c1 = c2 & 0xff;
    if (is_vv_debug) fprintf(stderr," @gen@_ozone:%03x,%02x",c3,c1);
#endif

    if ((c2 >= HNGL_TBL_START) && (c2 <= HNGL_TBL_END)) {/* Hangles */
	if (uni_o_hngl != NULL) {
	    ch = uni_o_hngl[c2 - HNGL_TBL_START];
#if defined(SKFBGSW)
	} else if (is_gb18030(conv_cap)) {
	    ch = c2 - 0x5543;	/* 0x82 35 98 03 = 01 05 17 03	   */
	    if (o_encode) out_@gen@_encode(c2,-80); /* 0x1b + 0x40*/
	    SKFGB2KAOUT(ch); return;
#endif	/* SKFBGSW */
	} else;
    } else if (c2 < HNGL_TBL_START) {		/* Yi Syllables	   */
	if ((uni_o_y != NULL) && (c2 < 0xa4d0)) {
	    ch = uni_o_y[c2 - Y_TBL_START];
#if defined(SKFBGSW)
	} else if (is_gb18030(conv_cap)) {
	    ch = c2 - 0x5543;
	    SKFGB2KAOUT(ch); return;
#endif
	} else;
/* NOTE: FIXME */
#if defined(SKFBGSW)
    } else if (is_gb18030(conv_cap)) {
	if (o_encode) out_@gen@_encode(c2,-80); /* 0x1b + 0x40*/
	c2 += 123464;		/* 15 * 12600 - 65536		   */
	SKFGB2KAOUT(c2); return;
#endif
/* NOTE: FIXME */
    } else if ((c2 >= HIST_TBL_START) && (c2 <= HIST_TBL_END)) {
    				/* historicals */
	if (uni_o_hist != NULL) {
	    ch = uni_o_hist[c2 - HIST_TBL_START];
	};
    } else if ((c2 >= UPMISC_TBL_START) && (c2 <= UPMISC_TBL_END)) {
    				/* upper miscellanious */
	if (uni_o_upmisc != NULL) {
	    ch = uni_o_upmisc[c2 - UPMISC_TBL_START];
	};
    } else if ((c2 >= UPKANA_TBL_START) && (c2 <= UPKANA_TBL_END)) {
    				/* upper miscellanious */
	if (uni_o_upkana != NULL) {
	    ch = uni_o_upkana[c2 - UPKANA_TBL_START];
	};
    } else if ((c2 >= NOTE_TBL_START) && (c2 <= NOTE_TBL_END)) {
    				/* musical notes and symbols */
	if ((c2 >= FITZPAT_TBL_START) && (c2 <= FITZPAT_TBL_END)) {
	/* emoji modifier fitzpatrick type */
	/* just discard 'em. */
	    return;
	} else if (uni_o_note != NULL) {
	    ch = uni_o_note[c2 - NOTE_TBL_START];
	} else;
	if ((ch == 0) && (c2 >= 0x1f100) && (c2 < 0x1f200)) {
	    enc_alpha_supl_conv(c2);
	    return;
	} else if ((ch == 0) && (c2 >= 0x1f200) && (c2 < 0x1f300)) {
	    enc_cjk_supl_conv(c2);
	    return;
	} else;
    } else if ((c2 >= CJK_B_TBL_START) && (c2 <= CJK_B_TBL_END)) {
    				/* CJK Ext.B/C/D   */
	if (uni_o_cjk_b != NULL) {
	    ch = uni_o_cjk_b[c2 - CJK_B_TBL_START];
	};
    } else if ((c2 >= CJK_C_TBL_START) && (c2 < CJK_C_TBL_END)) {
    				/* CJK cpt.sup  */
	if (uni_o_cjk_c != NULL) {
	    ch = uni_o_cjk_c[c2 - CJK_C_TBL_START];
	};
    } else if ((c2 >= TAG_TBL_START) && (c2 < TAG_TBL_END)) {
    	return;			/* discard TAGS	*/
    } else if ((c2 >= VARSEL_SUP_TBL_START) && (c2 < VARSEL_SUP_TBL_END)) {
    	return;			/* discard VARIATION SELECTORS	*/
    } else {
	out_undefined(c2,SKF_NOTABLE);
	return;
    };
#ifdef SKFDEBUG
    if (is_vv_debug) {
    	fprintf(stderr," ch(%x)",ch);
    } else;
#endif
    if (ch != 0) {
#if !defined(SKFKEISSW)
	if (o_encode) out_@gen@_encode(c2,ch);
#endif
#if defined(SKFBGSW) || defined(SKFKEISSW)
	if (ch >= 0x100) {
	    SKF@gen@OUT(ch);
	} else {
	    SKF@gen@1OUT(ch);
	};
	converted = 1;
#else
	if (ch < 0x8000) {
	    if (ch >= 0x100) {
		SKF@gen@OUT(ch);
		converted = 1;
	    } else if (ch < 0x80) {
		SKF@gen@1OUT(ch);
		converted = 1;
#if !defined(SKFSJISSW) && !defined(SKFKEISSW)
	    } else {
		SKF@gen@G2OUT(ch);
#endif
	    };
#if	defined(SKFJISSW)
	} else if (is_2ln(ch)) {
	    if (is_vv_debug) {
		fprintf(stderr,"2L");
	    } else;
	    SKF@gen@8859OUT(ch);
	    return;
#endif
#if !defined(SKFSJISSW)
	} else if (is_2pl(ch) && use_x0212) {
	    if (is_vv_debug) {
		fprintf(stderr,"2P");
	    } else;
	    SKF@gen@G3OUT(ch);
	    converted = 1;
	} else if (is_3pl(ch)) {
#ifdef SKFDEBUG
	    if (is_vv_debug) {
		fprintf(stderr,"3P");
	    } else;
#endif
	    SKF@gen@G4OUT(ch);
	    converted = 1;
#else	/* SKFSJISSW */
	} else if (is_2pl(ch) && 
			(is_ms_213c(conv_cap) || is_ms_cel(conv_cap))) {
#ifdef SKFDEBUG
	    if (is_vv_debug) {
		fprintf(stderr,"2P");
	    } else;
#endif
	    SKF@gen@G3OUT(ch);
	    converted = 1;
#endif	/* SKFSJISSW */
	} else {
#ifdef SKFDEBUG
	    if (is_vv_debug) {
		fprintf(stderr,"GX");
	    } else;
#endif
	    ;
	};
#endif
    } else {
	out_undefined(c2,SKF_OUNDEF);
	return;
    };

    if (converted == 0) {
#ifndef SKF_CODECOUNT
	skf_lastresort(c2);
#else
	return(skf_lastcount(c2));
#endif
    };
#else
    skf_lastresort(c2);
#endif
}

/* --- compatibility plane --------------------------------------- */
/*  0xf900 <= c2 < 0x10000					   */
/* --------------------------------------------------------------- */

void @gen@_compat_oconv (c2)
    skf_ucode    c2;
{
    skf_ucode	c3;
    int		c1,c4;
    int		converted = 0;

    c1 = (c2 & 0xff);
    c4 = (c2 >> 8) & 0xff;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," @gen@_cmpat:%02x,%02x",c4,c1);
#endif

#if defined(SKFBGSW) || defined(SKFKEISSW)
    if (uni_o_compat != NULL) {
	if ((c3 = uni_o_compat[c2 - 0xf900]) != 0) {
#if !defined(SKFKEISSW)
	    if (o_encode) out_@gen@_encode(c2,c3);
#endif
#if defined(SKFBGSW)
	    if (c3 >= 0x8000) {	/* dim2 & 4			   */
		if (is_gb18030(conv_cap)) {
#if !defined(SKFKEISSW)
		    if (o_encode) out_@gen@_encode(c2,-80);
		    			/* 0x1b + 0x40*/
#endif
		    SKFGB2KAOUT(gb18030_unpack(c3)); return;
		} else {
		    SKF@gen@OUT(c3);
		};
	    } else if (c3 >= 0x100) {
#else
	    if (c3 >= 0x100) {
#endif
		SKF@gen@OUT(c3);
	    } else {
		SKF@gen@1OUT(c3);
	    };
	    converted = 1;
	} else {
		;
	};
    };
#else
    if (uni_o_compat != NULL) {
	if ((c3 = uni_o_compat[c2 - 0xf900]) != 0) {
	    if (o_encode) out_@gen@_encode(c2,c3);
	    if (c3 >= 0x8000) {	/* dim2 & 4		   */
#if	defined(SKFJISSW)
		if (is_2ln(c3)) {
		    SKF@gen@8859OUT(c3);
		    return;
		} else 
#endif
#if !defined(SKFSJISSW)
	      	if (is_2pl(c3) && use_x0212) {
		    SKF@gen@G3OUT(c3);
		    converted = 1;
		} else if (is_3pl(c3)) {
		    SKF@gen@G4OUT(c3);
		    converted = 1;
#else
		if (is_2pl(c3) &&
		  (use_x0212 || (!is_jis(conv_cap) && !is_euc(conv_cap)))) {
#ifdef SKFDEBUG
		    if (is_vv_debug) {
			fprintf(stderr,"2P");
		    } else;
#endif
		    SKF@gen@G3OUT(c3);
		    converted = 1;
#endif	/* SKFSJISSW */
		} else;
	    } else if (c3 >= 0x100) {
		SKF@gen@OUT(c3);
		converted = 1;
	    } else if (c3 < 0x80) {
		SKF@gen@1OUT(c3);
		converted = 1;
#if defined(SKFJISSW) 
	    } else if (use_iso8859) { /* read as pass thru flg */
		SKFJIS8859OUT(c3);
		converted = 1;
#endif
	    } else {
#if defined(SKFJISSW) || defined(SKFSJISSW) || defined(SKFKEISSW)
		    SKF@gen@K1OUT(c1 + 0x40);
#else
		    SKF@gen@G2OUT(c1 + 0x40);
#endif
		converted = 1;
	    };
	};
    };
#endif	/* !SKFBGSW */
    if ((c4 == 0xfe) && (c1 < 0x10)) { /* variation sel. */
    	converted = 1;	/* just discard				   */
    };

    if (converted == 0) {
	skf_lastresort(c2);
    };
}

/* --- private use plane ----------------------------------------- */
/* --------------------------------------------------------------- */
#if defined(SKFSJISSW)
static unsigned short celn_prv_map[22] = {
    0xf040,0xf985,0xf990,0xf987, 0xf988,0xf989,0xf98a,0xf98b,
    0xf98c,0xf98d,0xf98e,0xf98f, 0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000, 0x0000,0x0000
};
static unsigned short cela_prv_map[22] = {
    0xf041,0xf489,0xf7c9,0xf6fb, 0xf6fc,0xf740,0xf741,0xf742,
    0xf743,0xf744,0xf745,0xf746, 0xf3d2,0xf3cf,0xf348,0xf3ce,
    0xf3d1,0xf3d0,0xf3a5,0xf3d3, 0xf349,0xf790
};
static unsigned short cels_prv_map[22] = {
    0xf042,0xf7b0,0xf7c5,0xf7bc, 0xf7bd,0xf7be,0xf7bf,0xf7c0,
    0xf7c1,0xf7c2,0xf7c3,0xf7c4, 0xfbb3,0xfbae,0xfbb1,0xfbad,
    0xfbb0,0xfbaf,0xfbab,0xfbb4, 0xfbb2,0xfbac
};
#endif
#if defined(SKFSJISSW) || defined(SKFEUCSW) || defined(SKFJISSW)
static unsigned short x213_rev_conv[80] = { /* d800 - d84f */
    0x0000,0x2477,0x2478,0x2479, 0x247a,0x247b,0x247c,0x2678,
    0x2577,0x2578,0x2579,0x257a, 0x257b,0x257c,0x257d,0x257e,
    0x247d,0x247e,0x287d,0x287e, 0x2b44,0x285f,0x2860,0x2861,
    0x2b48,0x2b49,0x2b4a,0x2b4b, 0x2b4c,0x2b4d,0x2b4e,0x2b4f,
    0x2b65,0x2b66,0x2862,0x2863, 0x2864,0x2865,0x2866,0x0000,
    0x0000,0x0000,0x0000,0x0000, 0x2c74,0x2c75,0x2c76,0x2c77,
    0x2c78,0x2c79,0x2c7a,0x2c7b, 0x2c7c,0x2d58,0x2d59,0x2d5a,
    0x2d5b,0x2d5c,0x2d5d,0x2d5e, 0x2d70,0x2d71,0x2d72,0x2d74,
    0x2d75,0x2d76,0x2d77,0x2d7a, 0x2d7b,0x2d7c,0x2e21,0x2f7e,
    0x4f54,0x4f7e,0x7427,0x7e7a, 0x7e7b,0x7e7c,0x7e7d,0x7e7e
};
#endif

void @gen@_private_oconv (c2)	/* u+d800 to u+f8ff		   */
    skf_ucode    c2;
{
    int		ch;
#if (!defined(SKFKEISSW) && !defined(SKFBGSW)) || defined(SKFDEBUG)
    int		c1;

    c1 = (c2 & 0xff);
#endif
    ch = (c2 >> 8) & 0xff;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," @gen@_privt:%02x,%02x",
	ch,c1);
#endif

#if !defined(SKFKEISSW) && !defined(SKFBGSW)
    if (o_encode) out_@gen@_encode(c2,c1);
#endif
    if (c2 < 0xe000) {
    		/* some (x-0213, emoticon) has reverse maps */
#if defined(SKFSJISSW) || defined(SKFJISSW) || defined(SKFEUCSW)
#if defined(SKFSJISSW)
	if ((c2 >= 0xd850) && (c2 < 0xd866)) { /* emoticon  */
	    if (out_codeset == codeset_sjiscl) {
	    	ch = celn_prv_map[c2 - 0xd850];
	    } else if (out_codeset == codeset_sjisau) {
	    	ch = cela_prv_map[c2 - 0xd850];
	    } else {
	    	ch = cels_prv_map[c2 - 0xd850];
	    };
	    if (ch != 0) {
		c1 = (ch >> 8) & 0xff;
		SKFputc(c1); SKFputc(ch & 0xff);
		return;
	    } else;
	} else if (is_ms_213c(conv_cap) && (c2 < 0xd850)) {
#endif
#if defined(SKFJISSW)
    	if (is_jis_213c(conv_cap) && (c2 < 0xd850)) {
#endif
#if defined(SKFEUCSW)
	if (is_euc_213c(conv_cap) && (c2 < 0xd850)) {
#endif
	    ch = x213_rev_conv[c2 - 0xd800];
	    if (ch >= 0x8000) {
		SKF@gen@G3OUT(ch);
		return;
	    } else if (ch > 0) {
#if defined(SKFJISSW)
	    	if (out_codeset == codeset_x0213_s) {
		    SKF@gen@G3OUT(ch);
		} else {
		    SKF@gen@OUT(ch);
		};
#else
		SKF@gen@OUT(ch);
#endif
		return;
	    } else;
	} else;
#endif
	/* others will be processed using ligature	  */
	lig_x0213_out(c2,0);
	return;
    } else if (uni_o_prv != NULL) {	/* 0xe000 - 0xf7ff */
	ch = uni_o_prv[c2 - 0xe000];
	if (ch != 0) {
#if defined(SKFBGSW)
	    if (is_gb18030(conv_cap)) {
		SKF@gen@OUT(ch);
	    } else SKF@gen@OUT(ch);
#else
	    if (ch > 0x8000) {
		SKF@gen@G3OUT(ch);
	    } else SKF@gen@OUT(ch);
#endif
	    return;
	} else;
    } else {	/* 0xe000 - 0xf7ff without table */
#if defined(SKFSJISSW)
	if (is_ms_932(conv_cap) && (c2 >= 0xe000)
	    && (c2 < MS_PRIV_LIM)) {
	    c1 = ((c2 - 0xe000) / 188) + 0xf0;
	    ch = ((c2 - 0xe000) % 188) + 0x40;
	    SKFputc(c1); 
	    SKFputc((ch > 0x7e) ? (ch+1) : ch); 
	    return;
	} else if (is_ms_cel(conv_cap)) {/* keitai graphical characters */
	    if (out_codeset == codeset_sjisontt) {
		if (((c2 >= 0xe63e) && (c2 <= 0xe6a5))
		   || ((c2 >= 0xe6ac) && (c2 <= 0xe6ae))
		   || ((c2 >= 0xe6b1) && (c2 <= 0xe6ba))
		   || ((c2 >= 0xe6d0) && (c2 <= 0xe70b))
		   || ((c2 >= 0xe70c) && (c2 <= 0xe757))) {
		    ch = uni_o_prv[c2 - 0xe000];
		    if (ch != 0) {
			c1 = (ch >> 8) & 0xff;
			SKFputc(c1); SKFputc(ch & 0xff);
			return;
		    };
		};
	    };
	} else;
#endif
#if defined(SKFJISSW)
	if (is_ms_cel(conv_cap)) { /* vodaf*ne graphics */
	    if (((c2 >= 0xe001) && (c2 <= 0xe05a))
	       || ((c2 >= 0xe101) && (c2 <= 0xe15a))
	       || ((c2 >= 0xe201) && (c2 <= 0xe25a))
	       || ((c2 >= 0xe301) && (c2 <= 0xe34d))
	       || ((c2 >= 0xe401) && (c2 <= 0xe44c))
	       || ((c2 >= 0xe501) && (c2 <= 0xe539))) {
		SKFputc(A_ESC); SKFputc('$'); 
		if (c2 < 0xe100) { SKFputc('G');
		} else if (c2 < 0xe200) { SKFputc('E');
		} else if (c2 < 0xe300) { SKFputc('F');
		} else if (c2 < 0xe400) { SKFputc('O');
		} else if (c2 < 0xe500) { SKFputc('P');
		} else { SKFputc('Q');
		};
		SKFputc((c2 & 0x7f) + 0x20);
		SKFputc(A_ESC); SKFputc(g0_mid); SKFputc(g0_char);
		if (o_encode) SKFputc(mFLSH);
		return;
	    } else;
	} else if (is_cp5022x(conv_cap)
	    && (c2 < MS_PRIV_LIM)) {
	    c1 = ((c2 - 0xe000) / 94) + 0x7f;
	    ch = ((c2 - 0xe000) % 94) + 0x21;
	    
	    if (!is_basecjk_shift) { 
		set_basecjk_shift; 
		SKFputc(A_ESC); SKFputc(ag0_mid); 
		if (ag0_quad) SKFputc(ag0_midl); 
		SKFputc(ag0_char);
	    };
	    SKFputc(c1); SKFputc(ch); 
	    return;
	} else;
#endif
#ifdef SKFEUCSW
	if (is_euc_mseuc(conv_cap) && (c2 < MS_PRIV_LIM)) {
#ifndef MS_BUG_COMPAT
	    if ((c1 <= 0xe177) ||
		((c1 >= 0xe2f0) && (c1 <= 0xe3ab))) {
		c1 = ((c2 - 0xe000) / 94) + 0xe5;
		ch = ((c2 - 0xe000) % 94) + 0xa1;
		SKFputc(c1); 
		SKFputc(c2); 
	    } else {
		c1 = ((c2 - 0xe000) / 188) + 0xf0;
		ch = ((c2 - 0xe000) % 188) + 0x40;
		SKFputc(c1); 
		SKFputc((ch > 0x7e) ? (ch+1) : ch); 
	    };
#else
	    c1 = ((c2 - 0xe000) / 188) + 0xf0;
	    ch = ((c2 - 0xe000) % 188) + 0x40;
	    SKFputc(c1); 
	    SKFputc((ch > 0x7e) ? (ch+1) : ch); 
#endif
	    return;
	} else;
#endif
	;
    };

    skf_lastresort(c2);		/* default.			   */
}

#ifndef	SKF_CODECOUNT
/* --------------------------------------------------------------- */
/* --- finish process -------------------------------------------- */
void @gen@_finish_procedure()
{
    oconv_flush();	/* wipe pending charcters to out-streams   */
#ifdef SKFJISSW
    if (out_jis && si_enbl && is_x0201_lshift) {
	SKFputc(A_SI); 		/* si locking shift case */
    };				/* reset anyway			   */
    if (out_jis && is_kanji_shift) {
	reset_kanji_shift;
	SKFputc(A_ESC);SKFputc('(');SKFputc(g0_char);
	if (o_encode) SKFputc(mFLSH);
    };
#endif
#ifdef SKFEUCSW
	;
#endif
#ifdef SKFSJISSW
	;
#endif
#ifdef SKFBGSW
	;
#endif
#ifdef SKFKEISSW
    if (is_keis_shift) {
	SKFputc(KEIS_SMM); SKFputc(KEIS_SI); reset_kanji_shift;
    };
#endif
}
#endif

#if !defined(SKFKEISSW)
/* --------------------------------------------------------------- */
/* encoder main(1) - encode presetting				   */
/*  output codeset is drawn from out_codeset			   */
/* called at genoconv and generate header			   */
/* Note: encode capability consistency is not checked. Those stuff */
/*  should be tested in earlier processes.			   */
/* --------------------------------------------------------------- */
/* out_@gen@_encode: MIME folding detection			   */
/*  (1) if position already reached limits, then fold.		   */
/*  (2) if character and position reaches limit, then fold.	   */
/* Note MIME respects character bounds, MIME does not consider	   */
/* about named character sequences that skf doesn't handle as one. */
/* --------------------------------------------------------------- */
/* special convch encoding: 					   */
/*  0: no conversion exist (ignored here)			   */
/*  < -64: chc count len (not shown here)			   */
/* 	   (-convch & 0x7U) -> plen				   */
/* 	   (-convch & 0x38U)>>3 -> slen				   */
/* --------------------------------------------------------------- */
void out_@gen@_encode(ch,convch)
skf_ucode ch;		/* RAW unicode character */
skf_ucode convch;	/* table lookup result */
{
    int plen = 0;	/* character count within alphanum.	   */
    int slen = 0;	/* controls and GR's			   */

#if defined(SKFBGSW) || defined(SKFSJISSW)
    skf_ucode c3;
#if defined(SKFSJISSW)
    skf_ucode c1;
#endif
#if !defined(SKFBGSW)
    skf_ucode p3;
#endif

#if defined(SKFSJISSW)
    c1 = (convch >> 8) & 0xffU;
#endif
    c3 = convch & 0xffU;
#endif

    if (ch >= 0) {	/* has character to output		   */
    	if (is_lineend(ch)) {
	    convch = ch;
	} else;
    	if (convch > 0) {
#ifdef SKFBGSW
	    if (convch < 0x80) {
	    	if (is_lineend(convch)) {
		/* if ch is lineend, mime will be clip anyway.	   */
		    ;
		} else if ((ch == '<') &&
			(is_intext_mail || is_intext_maillike)) {
		    (void)mime_clip_test(1,-12);
		    /* FIXME */
		} else (void)mime_clip_test(1,0); 
				/* if ascii, result should be 1	   */
#if defined(USE_TRUE_MIME_TAIL)
		if ((table_mime_tail(ch) || is_white(ch)) 
			&& o_encode_stat
			&& !no_early_mime_out(nkf_compat)
			&& is_o_encode_mimebq(o_encode)) {
		    o_c_encode(sSUSP);
		} else;
#endif
		return;
	    } else if (is_big5fam(conv_cap)) {
	    	if (is_gb18030(conv_cap) && (convch > 0x8000)) {
		    plen = 2; slen = 2;		/* Area 5	   */
		} else if (is_big5p(conv_cap)) {
		    if (convch >= 0xa0000) {
		    	plen = 1;
		    } else if (convch >= 0x100) {
		    	slen = 1;
		    } else;
		    if (is_ascii(c3)) plen++;
		    else slen++;
		} else {		/* normal BIG5s		   */
		    slen = 1;
		    if (is_ascii(c3)) plen++;
		    else slen++;
		};
	    } else if (is_hzzW(conv_cap)) {
	    	plen = 4; slen = 0;
	    } else if (is_johab(conv_cap) || is_gbk(conv_cap)) {
	    	slen = 1;
		if (is_ascii(c3)) plen++;
		else slen++;
	    } else if (is_uhc(conv_cap)) {
	    	slen = 2;
	    } else plen = 1;
#else			/* !KEIS && !BG				  */
	    if (convch < 0x80) {
#if defined(SKFJISSW) || defined(SKFSJISSW) || defined(SKFEUCSW)
		/* if ch is lineend, mime will be clip anyway.	   */
		if (is_lineend(convch)) return;
		else plen = 1;
#endif
#if SKFJISSW
		if (is_kanji_shift) { 
		    if (is_x0201_lshift) {
		    	slen = 1;
		    } else {
		    	plen += 2; slen = 1;
		    };
		} else {
		    slen = 0;
		};
#endif
#ifdef SKFSJISSW
		slen = 0;
#endif
#ifdef SKFEUCSW
		if (is_euc7(conv_cap)) {
	    	  if (is_kanji_shift) {
		     slen = 1;
		  } else;
		} else {
		    slen = 0;
		};
#endif
			/* if ascii, result should be 1 */
	    } else if (convch < 0x100) {
#ifdef SKFJISSW
		if (use_iso8859) {
		    if ((use_iso8859_1_left) && 
		((!is_g2_8859_shift) || (g2_extract_code_set() != 1))) {
			plen = 2; slen = 1;
		    } else if (!(use_iso8859_1_left) && (!is_i8859_shift)) {
			if (enbl_latin_annon) {
			    plen = 2; slen = 1;
			};
		    };
		    if (use_iso8859_1_left) {
			slen += 1; plen += 2;
		    } else {
			slen += 1;
		    };
		} else {	/* kana case */
		    if (eight_bit) {
		    	slen += 1;
		    } else if (si_enbl) {
		    	slen += 2; plen += 1;
		    } else if (is_x0201_shift) {
		    	slen += 2; plen += 2;
		    } else {	/* must be return to ascii */
		    	slen += 4; plen += 3;
		    };
		};
#endif
#ifdef SKFSJISSW
		slen = 1; plen = 0;
#endif
#ifdef SKFEUCSW
		if (is_euc7(conv_cap)) {
		    if (is_kanji_shift) { 
		    	slen += 1;
		    } else ;
		    slen += 1; plen = 2;
		} else {
		    slen = 2;
		};
#endif
	    } else if (convch < 0x8000) {  /* basic MB character   */
	    	/* Note: see r_SKFJISOUT			   */
#ifdef SKFJISSW
		plen = 2;	/* for MB itself		   */
		if (!is_basecjk_shift) {
		    if (add_renew) {
		    	plen += 2; slen++;
		    } else;
		    if (is_euc7(conv_cap)) {
			slen++; plen += 2;
		    } else {
		    	if (ag0_quad) plen += (4+3);
			else plen += (3+3);
		    };
		} else {
		    if (is_euc7(conv_cap)) {
		    	plen += 1;
		    } else {
			plen += 3;	
		    };
		};
#endif
#ifdef SKFEUCSW
		if (is_euc7(conv_cap)) {
		    if (!is_kanji_shift) slen ++;
		    plen = 2;
		} else {
		    slen = 2;	/* for MB itself		   */
		};
#endif
#ifdef SKFSJISSW
/* Note: SKFSJISSW does not need any addtional shift characters	   */
		if ((convch < 0x7921) || !(is_ms_932(conv_cap))) {
		    p3 = c3 + ((c1 & 1) ? ((c3 < 0x60) ? 0x1f : 0x20)
		    			: 0x7e);
		} else if (c3 < 0x7c7f) {
			/* ms_compat && nec_selected_ibm_gaiji */
		    if (c3 < 0x7c6f) {	/* Kanji area		   */
			p3 = c3 + ((c1 - 0x79) * 94) + 0x1c - 0x21;
			if (p3 >= 376) {
			    p3 -= 376;
			} else if (p3 >= 188) {
			    p3 -= 188;
			};
			p3 = (p3 >= 0x3f) ? (p3 + 0x41) : (p3 + 0x40);
		    } else {
			c1 = uni_ibm_nec_excg[c3 - 0x7c6f];
			p3 = (c1 & 0x00ff); 
		    };
		    slen = 1;
		    if (is_ascii(p3)) plen = 1;
		    else slen += 1;
		};
#endif
	    } else if (is_2pl(convch)) {
#ifdef SKFSJISSW
		plen = 0; slen = 0;
#endif
#ifdef SKFJISSW
		if ((!is_altcjk_shift && (g3_mid < 0x2d))
		    || (!is_i8859x_shift && (g3_mid >= 0x2d))) {
		    slen = 1; plen = 2;
		    if (g3_quad) plen++;
		}; 
		if ((convch & 0xff00UL) != 0) {
		    plen += 2;
		} else {
		    slen += 1;
		};
#endif
#ifdef SKFEUCSW
		if (is_euc7(conv_cap)) {
		    if (is_kanji_shift) { 
		    	slen += 1;
		    } else ;
		    slen += 1; plen += 3;
		} else if (is_euc_gbcn(conv_cap)) {
		    slen = 3;
		} else {
		    if (is_euc7(conv_cap)) {
			if (is_kanji_shift) { 
			    slen += 1;
			} else ;
			slen += 1; plen += 3;
		    } else {
		    	slen = 3;
			if (is_euc_cns(conv_cap)) {
			    slen += 1;
			};
		    };
		};
#endif
	    } else if (is_3pl(convch)) {
#ifdef SKFSJISSW
		plen = 0; slen = 0;
#endif
#ifdef SKFJISSW
		if (!is_g4cjk_shift) {
		    set_g4cjk_shift;
		    slen = 1; plen = 2;
		    if (g4_quad) plen++;
		}; 
		plen += 2;
#endif
#ifdef SKFEUCSW
		/* IS THIS REALLY BE TRUE? */
		if (is_euc_gbcn(conv_cap)) {
		    slen = 2; plen = 6;
		    if (is_euc7(conv_cap)) {
			slen += 2; plen += 2;
		    } else {
			slen += 2;
		    };
		} else ;
#endif
	    } else {
	    	;
	    };
#endif	/* END !BG & !KEIS */
	} else if (convch <= -32) {	/* explicit count given	   */
    	    unsigned int cvcnt;

	    cvcnt = (unsigned int)(0 - convch);
	    plen = (cvcnt & 0x07U);
	    slen = (cvcnt & 0x38)>>3;
	} else {	/* convch = 0 needs special treatments.	   */
			/* will be handled again in plug_converts. */
		;	/* but test with 0,0 anyway		   */
	};
	(void)mime_clip_test(plen,slen);
#if defined(USE_TRUE_MIME_TAIL)
    	if ((table_mime_tail(ch) || is_white(ch)) 
		&& o_encode_stat
		&& !no_early_mime_out(nkf_compat)
		&& is_o_encode_mimebq(o_encode)) {
		/* mime may stop here				   */
	    o_c_encode(sSUSP);
	} else;
#endif
    } else {	/* if c1 < 0, output as it is.			   */
    	;
    };
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," tc");
#endif
    return;
}
#endif

