/* *******************************************************************
** Copyright (c) 1993-2012 Seiji Kaneko. All rights reserved.
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
** plug_convert.c:	various converter routines for skf
**		to treat something dirty ;-)
** $Id: plug_convert.c,v 1.118 2017/01/09 14:52:42 seiji Exp seiji $
**	x0201conv:	x0201 kana converter
**	new2oldJis:	x0208(1983,1990) to C6226(1978)
**	lig_x0213_out:	X0213/ISCII ligature out 
**	jis90tojis83	X0208(1990) to X0208(1983)
**	keis_conv:	keis converter
**	print_announce:	announcer output
**	show_endian_out:endian display
**	skip_x221_2:	x0221 null character suppressor
**	lig_compat:	compatible area ligature converter
**	enc_alpha_supl_conv:	0x1f100 - 0x1f1ff converter
**
**********************************************************************
** Notice for limitation of these converter programming.
**  Since these routine is called from output converters, these
**  converter may not use ambiguous characters(i.e. not strict
**  ascii nor X0208).
*/

#include <stdio.h>
#include <sys/types.h>
#include "skf.h"
#include "skf_fileio.h"
#include "convert.h"
#include "oconv.h"
#include "in_code_table.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#define	PLUGAGAIJI	conv_nec_gaiji

static void	cjk_number_parse P_((int));
static void	CJK_display_ten P_((skf_ucode));
static void	CJK_display_day P_((skf_ucode));

static void	CJK_circled P_((skf_ucode,int));

static void	kana_force_conv P_((skf_ucode));

/* ---------------------------------------------------------------
 *   SKF_STRPUT():	short binary string output
 */

void  SKF_STRPUT(str)
unsigned short *str;
{
    int	ch;
    while ((ch = *str) < 0x100) {
	SKFputc(ch); str++;
    };
}

/* ---------------------------------------------------------------
    x0201conv
	Convert x0201 kana to x0208 kana and output it.
	input:	c	kana code(raw, unic*de compatibility plane)
			0x??20 <= c1 < 0x??7f ??: don't care
		nxt	next code to modify dakuten
			0x??5e <= c1 < 0x??5f ??: else don't care
	return value: int
		0	normal conversion and nxt used.
		nxt	normal conversion and nxt unused.
*/

/* X 0201 kana part conversion table                              */
static const unsigned char kana_lwt[64] = {  
			       /* second(c1) part of conversion   */
 0x01,0x02,0x0c,0x0d,0x01,0xfb,0xf2,0xa1,0xa3,0xa5,0xa7,0xa9,
 0xe3,0xe5,0xe7,0xc3,0xfc,0xa2,0xa4,0xa6,0xa8,0xaa,0xab,0xad,
 0xaf,0xb1,0xb3,0xb5,0xb7,0xb9,0xbb,0xbd,0xbf,0xc1,0xc4,0xc6,
 0xc8,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd2,0xd5,0xd8,0xdb,0xde,
 0xdf,0xe0,0xe1,0xe2,0xe4,0xe6,0xe8,0xe9,0xea,0xeb,0xec,0xed,
 0xef,0xf3,0x9b,0x9c
};

const char dakuten[64] = { /* determine kana can have dakuten	  */
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,4,4,4,4,4,1,1,1,4,1,
 1,1,4,1,4,0,0,0,0,0,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

int x0201conv(c,nxt)
	int c,nxt;
{
	int c1,c3;
	int unxt;

#ifdef SKFDEBUG
	if (is_vv_debug)
		fprintf(stderr," x0201conv: %x-%x",
			(unsigned int)c,(unsigned int)nxt);
#endif
	c3 = (c & 0x00ff) - 0x20;
	unxt = nxt & 0x7f;		/* equalization		   */
	if ((c3 <= 0) || (c3 > 0x3f)) {	/* out of kana region      */
	    in_undefined(c,SKF_IBROKEN);
	    return(nxt);                /* nxt unused              */
	};
	c1 = kana_lwt[c3]; 		/* kana table refer	   */
	if (dakuten[c3] > 0) {
	    if (unxt == 0x5e) {	/* daku on			   */
		if (c1 == 0x46)	c1 = 0x94; /* u -> vu		   */
		else if (c1 == 0xa6)	c1 = 0xf4; /* u -> vu	   */
		else c1 += 1; 	/* others.			   */
		nxt = 0;
	    } else if ((dakuten[c3] == 3) && (unxt == 0x5f)) {
		c1 += 2;       /* han-dakuon              */ 
		nxt = 0;
	    } else if ((dakuten[c3] == 4) && (unxt == 0x5f)) {
	    		/* X-0213 kana-handakuon hook		   */
	    	switch (c3) {
		    case 22: c1 = 0xd808 - 0x3000; nxt = 0; break;
		    case 23: c1 = 0xd809 - 0x3000; nxt = 0; break;
		    case 24: c1 = 0xd80a - 0x3000; nxt = 0; break;
		    case 25: c1 = 0xd80b - 0x3000; nxt = 0; break;
		    case 26: c1 = 0xd80c - 0x3000; nxt = 0; break;
		    case 30: c1 = 0xd80d - 0x3000; nxt = 0; break;
		    case 34: c1 = 0xd80e - 0x3000; nxt = 0; break;
		    case 36: c1 = 0xd80f - 0x3000; nxt = 0; break;
		    default: break;
		};
	    }; };

	post_oconv(0x3000+c1);
	return (nxt);
}

/* ---------------------------------------------------------------
    x0201rconv
	Convert x0208 kana (Unicode) to x0201 kana and output it.
	input:	ch	kana code(raw, unic*de kana plane)
	return value: unsigned int (2 U-ff**: ** parts packed)
*/
static const unsigned short kana_revcnv[92] = {
  0x67,0x71,0x68, 0x72,0x69,0x73,0x6a,
  0x74,0x6b,0x75,0x76, 0x769e,0x77,0x779e,0x78,
  0x789e,0x79,0x799e,0x7a, 0x7a9e,0x7b,0x7b9e,0x7c,
  0x7c9e,0x7d,0x7d9e,0x7e, 0x7e9e,0x7f,0x7f9e,0x80,

  0x809e,0x81,0x819e,0x6f, 0x82,0x829e,0x83,0x839e,
  0x84,0x849e,0x85,0x86, 0x87,0x88,0x89,0x8a,
  0x8a9e,0x8a9f,0x8b,0x8b9e, 0x8b9f,0x8c,0x8c9e,0x8c9f,
  0x8d,0x8d9e,0x8d9f,0x8e, 0x8e9e,0x8e9f,0x8f,0x90,

  0x91,0x92,0x93,0x6c, 0x94,0x6d,0x95,0x6e,
  0x96,0x97,0x98,0x99, 0x9a,0x9b,0x9c,0x9c,
  0x72,0x74,0x66,0x9d, 0x739e,0x76,0x79,0x9c9e,
  0x729e,0x749e,0x7c9e,0x65, 0x70
};

static unsigned short uni_lig_x0213_24[] = {
    0x0000,0x304b,0x304d,0x304f, 0x3051,0x3053,0x0000,0x31f7,
    0x30ab,0x30ad,0x30af,0x30b1, 0x30b3,0x30bb,0x30c4,0x30c8,
    0x0000,0x0000,0x0000,0x0000, 0x00e6,0x0000,0x0000,0x0000,
    0x0254,0x0254,0x028c,0x028c, 0x0259,0x0259,0x025a,0x025a
};

unsigned short x0201rconv(ch)
skf_ucode ch;
{
    int c1,c2;

#ifdef SKFDEBUG
    if (is_vv_debug)
	fprintf(stderr," x0201rconv:%x",(unsigned int)ch);
#endif
    if ((ch >= 0x3041) && (ch <= 0x3096)) {
    	return(kana_revcnv[ch - 0x3041]);
    } else if ((ch >= 0x30a1) && (ch <= 0x30fc)) {
    	return(kana_revcnv[ch - 0x30a1]);
    } else if ((ch == 0x3099) || (ch == 0x309b)) {
    	return(0x9eU);
    } else if ((ch == 0x309a) || (ch == 0x309c)) {
    	return(0x9fU);
    } else if (ch == 0x3001) {
    	return(0x64);
    } else if (ch == 0x3002) {
    	return(0x61);
    } else if (ch == 0x300c) {
    	return(0x62);
    } else if (ch == 0x300d) {
    	return(0x63);
    } else if ((ch >= 0xd801) && (ch <= 0xd80f)) {
        c1 = uni_lig_x0213_24[ch - 0xd800];
	if (c1 == 0) {
	    return(0);
	} else if (c1 < 0x30a0) {
	    c2 = (kana_revcnv[c1 - 0x3041]) & 0xff;
	    return((unsigned short)((c2 << 8) + 0x9f));
	} else if (c1 < 0x3100) {
	    c2 = (kana_revcnv[c1 - 0x30a1]) & 0xff;
	    return((unsigned short)((c2 << 8) + 0x9f));
	} else {
	    return(0);
	};
    } else {
    	return(0);
    };
}

/* --------------------------------------------------------------- */
/* various ligatures and special treatments			   */
/* --------------------------------------------------------------- */
/* lig_x0213_out: input - 0xd800 - 0xdfff			   */
/*   d800-d87f: x0213 kanas/iscii (66-6f: reserved)		   */
/*   d880-d8ff: tiscii 						   */
/*   d900-d9cf: mosaics						   */
/*   da00-d9cf: mosaics						   */
/* --------------------------------------------------------------- */

static unsigned short uni_lig_emo_f9[16] = {
    0x23,0x30,0x31,0x32, 0x33,0x34,0x35,0x36,
    0x37,0x38,0x39,0x00, 0x00,0x00,0x00,0x00
};

static skf_ucode uni_lig_emo_fa[20] = {
    0x1f1e8,0x1f1f3, 0x1f1e9,0x1f1ea,
    0x1f1ea,0x1f1f8, 0x1f1eb,0x1f1f7,
    0x1f1ec,0x1f1e7, 0x1f1ee,0x1f1f9,
    0x1f1ef,0x1f1f5, 0x1f1f0,0x1f1f3,
    0x1f1f7,0x1f1f3, 0x1f1f7,0x1f1f8
};

static const char *uni_lig_arib_mi[32] = {
  "(vn)","(ob)","(cb)","(ce",  "mb)","(hp)","(br)","(p)",
  "(s)","(ms)","(t)","(bs)",   "(b)","(tb)","(tp)","(ds)",
  "(ag)","(eg)","(vo)","(fl)", "(ke","y)","(sa","x)",
  "(sy","n)","(or","g)",       "(pe","r)"," "," "
};

#ifdef TSCII_SUPPORT
static unsigned short *uni_lig_ti;
#endif

void lig_x0213_out(ch,rch)
skf_ucode ch;
skf_ucode rch;
{
    int	c1,c2;
    int	cn;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," x0213lig(%x:%x)",
    			(unsigned short)ch,(unsigned short)rch);
#endif
    if (ch < 0xd880) {
	c1 = (ch & 0x7f);	/* U+d801 - U+d87f		   */
	if (c1 < 0x12) {    /* katakana/hiragana with han-dakuon   */
	    c2 = uni_lig_x0213_24[c1];	/* 0x24xx, 0x25xx	   */
	    if (c2 != 0) {
		post_oconv(c2);
		if (is_ucs_ufam(conv_cap)) {
#if 0
		    if (((in_codeset == codeset_x0213) ||
		         (in_codeset == codeset_x213a) ||
		         (in_codeset == codeset_x0213_s) ||
		         (in_codeset == codeset_euc_213) ||
		         (in_codeset == codeset_euc_213a) ||
		         (in_codeset == codeset_sj_0213) ||
		         (in_codeset == codeset_sj_213a)) &&
			 (rch == 0)) {
			post_oconv(0x309a);
#else
		    if (((is_x0213(g0_table_mod->is_kana)) ||
		         (is_x0213(g1_table_mod->is_kana)) ||
		         (is_x0213(g2_table_mod->is_kana)) ||
		         (is_x0213(g3_table_mod->is_kana))) &&
			 (rch == 0)) {
			post_oconv(0x309a);
#endif
		    } else if (rch == 0) post_oconv(0x309c);
		    else post_oconv(rch);
		} else if (!is_ms_213c(conv_cap) 
			&& !is_euc_213c(conv_cap)
		    	&& !is_jis_213c(conv_cap) 
			&& !is_nkf_compat
#if 0
		     && (in_codeset != codeset_x0213)
		     && (in_codeset != codeset_x213a)
		     && (in_codeset != codeset_x0213_s)
		     && (in_codeset != codeset_euc_213)
		     && (in_codeset != codeset_euc_213a)
		     && (in_codeset != codeset_sj_0213)
		     && (in_codeset != codeset_sj_213a)
#else
		     && (!is_x0213(g0_table_mod->is_kana))
		     && (!is_x0213(g1_table_mod->is_kana))
		     && (!is_x0213(g2_table_mod->is_kana))
		     && (!is_x0213(g3_table_mod->is_kana))
#endif
			) {
		    post_oconv(0x309c);
		} else {
		    post_oconv(0x309a);
		};
	    } else {
		out_undefined(ch,SKF_IOUTUNI);
	    };
	} else if (c1 < 0x20) {		/* 0x2bxx		   */
	    c2 = uni_lig_x0213_24[c1];
	    if (c2 != 0) {
		post_oconv(c2); 
		if (c1 == 0x14) {
		    post_oconv(0x300);
		} else if ((c1 >= 0x18) && (c1 < 0x20)) {
		    post_oconv((c1 & 0x01) ? 0x0301 : 0x0300);
		};
	    } else {
		out_undefined(ch,SKF_OUNDEF);
	    };
	} else if (c1 < 0x30) {		/* Tone mark addendum	   */
	    if (c1 == 0x20) {
		post_oconv(0x2e9);
		post_oconv(0x2e5);
	    } else if (c1 == 0x21) {
		post_oconv(0x2e5);
		post_oconv(0x2e9);
	    } else if (c1 == 0x28) {	/* machebrew 0xc0	   */
		post_oconv(0xf86a);
		post_oconv(0x05dc);
	    } else if (c1 == 0x29) {	/* machebrew 0xde	   */
		post_oconv(0x05b8);
		post_oconv(0xf87f);
	    } else if (c1 == 0x2a) {	/* macgurmukhi 0xd9	   */
		post_oconv(0xf860);
		post_oconv(0x0a38);
		post_oconv(0x0a3c);
	    } else {
		out_undefined(ch,SKF_IOUTUNI);
	    };
#ifdef FOLD_SUPPORT
	    fold_count += 2;
#endif
		/* note: 0x30-0x3f is bitten for x0213 reserved	   */
	} else if (c1 < 0x50) {		/* jis x0213 reserved area */
	    out_undefined(ch,SKF_UNSUPP);
	} else if (c1 < 0x66) {		/* Emoticon named seq.	   */
	    if (c1 < 0x5c) {		/* range d850 - d865	   */
	    	post_oconv((skf_ucode)uni_lig_emo_f9[c1 - 0x50]);
		post_oconv((skf_ucode)0x20e3);
	    } else {		/* regional indicator		   */
	    	cn = (c1 - 0x5c) * 2;
	    	post_oconv(uni_lig_emo_fa[cn]);
	    	post_oconv(uni_lig_emo_fa[cn+1]);
	    };
#ifdef FOLD_SUPPORT
	    fold_count += 2;
#endif
	} else if (c1 < 0x70) {		/* reserved		   */
	    out_undefined(ch,SKF_IOUTUNI);
#ifdef FOLD_SUPPORT
	    fold_count += 2;
#endif
	} else {	/* 0x70 - 0x7f	iscii controls		   */
	    if ((c1 - 0x40) < ISCII_RMN) {
		    ;			/* just discard		   */
	    } else {
		out_undefined(ch,SKF_UNSUPP);
	    };
	};
#ifdef TSCII_SUPPORT
    } else if (ch < 0xd900) {	/* tscii 1.7			   */
    	if ((uni_lig_ti == NULL) || (uni_lig_tv == NULL)) {
	    if ((uni_lig_ti = load_external_table(
	    	  &(ovlay_table_defs[tscii_i_index])) < 0)) {
		out_undefined(ch,SKF_NOTABLE);
		return;
	    };
	};
	c1 = ch & 0x7f;
	if (uni_lig_tcsii[c1+128]) {
	    post_oconv(uni_lig_tcsii[c1+128]);
	} else {
	    out_undefined(ch,SKF_UNSUPP);
	};
	if (uni_lig_tscii[c1]) {
	    post_oconv(uni_lig_tcsii[c1]);
	    if (c1 == 0x82) {
		post_oconv(0x0bb0);
		post_oconv(0x0bc0);
	    } else if (c1 == 0x87) {
		post_oconv(0x0bb7);
#ifdef FOLD_SUPPORT
	    fold_count--;
#endif
	    } else if (c1 == 0x8c) {
		post_oconv(0x0bb7);
		post_oconv(0x0bcd);
	    };
#ifdef FOLD_SUPPORT
	    fold_count += 3;
#endif
	} else {
	    out_undefined(ch,SKF_UNSUPP);
#ifdef FOLD_SUPPORT
	    fold_count += 3;
#endif
	};
#endif
    } else if (ch < 0xd9d0) {	/* mosaics (0xd900 - 0xd9cf)	   */
    	/* do anything before this point.			   */
	out_undefined(ch,SKF_IOUTUNI);
    } else if (ch < 0xda00) {	/* mosaics (0xd9d0 - 0xd9ff)	   */
	if (ch < 0xd9f0) {
	    SKFSTROUT(uni_lig_arib_mi[ch - 0xd9d0]);
    	} else out_undefined(ch,SKF_IOUTUNI);
    } else {
	out_undefined(ch,SKF_IOUTUNI);
    };
}

/* --------------------------------------------------------------- */
/* various character converters					   */
/* --------------------------------------------------------------- */
/* --------------------------------------------------------------- 
 * CJK_sq_conv
 *  CJK Squared abbreviations converter
 *  Unic*de: 0x3300-33ff area
 *   	input:		c1 - input(output) character
 *	output:		none
 */

static const char *uni_cjk_33_base[224] = {
    "Apt", NULL, "A", "a", "inn", "inch", "won", "escud",
    "acre", "oz", "Ohm", "nam", "car", "cal", "gal", NULL,

    "G", "Gn", "Ci", "Gd", "k", "kg", "km", "kW",
    "g", "gt", NULL, NULL, NULL, NULL, "coop", "cycle",

    NULL, "cyl", "cm", NULL, "dz", "d", "$", "t",
    "n", "knot", NULL, "%", NULL, "barrel", NULL, NULL,

    "p", "Bd.", "F", "Ft", NULL, NULL, "ha", NULL,
    NULL, "Hz", NULL, "p.", NULL, NULL, "V", "Phon",

    NULL, "Hall", "Phon", NULL, "mile", "mach", "DM", NULL,
    "micrn", "m", "mb", "M", "Mt", "m", "yd", NULL,

    NULL, "l", NULL, NULL, NULL, "rem", "R", "W",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

    NULL, "hPa", "da", "AU", "bar", "oV", "pc", "dm",
    "dm2", "dm3", "IU", NULL, NULL, NULL, NULL, NULL, 

/* --- unit name and abbreviations --- */
    "pA", "nA", "uA", "mA", "kA", "KB", "MB", "GB", /* 8x */
    "cal", "kcal", "pF", "nF", "uF", "ug", "mg", "kg",

    "Hz", "kHz", "MHz", "GHz", "THz", "ul", "ml", "dl",
    "kl", "fm", "nm", "um", "mm", "cm", "km", "mm2",

    "cm2", "m2", "km2", "mm3", "cm3", "m3", "km3", "m/s",
    "m/s2", "Pa", "kPa", "MPa", "GPa", "rad", "rad/s", "rad/s2",

    "ps", "ns", "us", "ms", "pV", "nV", "uV", "mV",
    "kV", "MV", "pW", "nW", "uW", "mW", "kW", "MW",

    "kOhm", "MOhm", "a.m.", "Bq", "cc", "cd", "C/kg", "Co.",
    "dB", "Gy", "ha", "HP", "in", "K.K.", "KM", "kt",

    "lm", "ln", "lo", "lx", "mb", "mil", "mol", "pH",
    "p.m.", "ppm", "PR", "sr", "Sv", "Wb", "v/m", "a/m"
};

static const char *lit_gal = "gal";

void CJK_sq_conv(c1)
skf_ucode c1;
{
    int c2;
    const char *sym;

    sym = NULL;
    c2 = (c1 & 0x00ff);
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," ligature at %x",(unsigned short)c1);
#endif
    if ((c2 >= 0x58) && (c2 <= 0x70)) {
	CJK_display_ten(c2);
    } else if ((c2 >= 0xe0) && (c2 <= 0xfe)) {
	CJK_display_day(c2);
    } else if (c2 == 0xff) {
	sym = lit_gal;
    } else {
	sym = uni_cjk_33_base[c2];
	if (sym == NULL) {
	    switch (c2) { /* --- Squared words --- */
		case 0x01: o_latin_conv(0x03b1); break; /* Alpha */
		case 0x0f: o_latin_conv(0x03b3); break; /* Gamma */

		case 0x23: o_latin_conv(0x0a2); break; /* cents */
		case 0x3c: o_latin_conv(0x03b2); break; /* Beta */

		case 0x40: o_latin_conv(0x0a3); break; /* Pound */
		case 0x43: o_latin_conv(0x03bc); break; /* Micro */

		case 0x7b: o_cjk_conv(0x5e73); 
			o_cjk_conv(0x6210); break;
		case 0x7c: o_cjk_conv(0x662d);
			o_cjk_conv(0x548c); break;
		case 0x7d: o_cjk_conv(0x5927);
			o_cjk_conv(0x6b63); break;
		case 0x7e: o_cjk_conv(0x660e);
			o_cjk_conv(0x6cbb); break;
		case 0x7f: CJK_circled(0x682a,8); break;
		default: out_undefined(c1,SKF_OUNDEF);
	    };
	};
    };
    if (sym != NULL) {
	SKFSTROUT(sym);
    };
}

void cjk_number_parse(c1)
int c1;
{
    int c2,c3;
    c3 = c1 / 10;
    c2 = c1 - (c3 * 10);
    if (c3 != 0) post_oconv(c3 + '0');
    post_oconv(c2 + '0');
}

void CJK_display_ten(c1)
skf_ucode c1;
{
    cjk_number_parse(c1 - 0x58);
    o_cjk_conv(0x70b9);
}

void CJK_display_day(c1)
skf_ucode c1;
{
    cjk_number_parse(c1-0xdf);
    o_cjk_conv(0x65e5);
}

/* --------------------------------------------------------------
    CJK_circled:	circled CJK character pseudo converter
	c1:	output character
	c3: bit 3: output as is
	    bit 2: output small letters
	    bit 1: output capital letters
	    bit 3,2,1 = 0: output digits
	    bit 0 = 1: output dot
	    bit 0 = 0 and bit 4 = 0: output bracket
	    bit 0 = 0 and bit 4 = 1: output squared bracket
*/
void CJK_circled(c1,c3)
skf_ucode c1;
int c3;
{
    if ((c3 & 0x01) == 0) {
    	if (c3 & 0x10) post_oconv('[');
	else post_oconv('('); 
    } else;

    if ((c3 & 0x02) != 0) post_oconv(c1+'A');
    else if ((c3 & 0x04) != 0) post_oconv(c1+'a');
    else if ((c3 & 0x08) != 0) post_oconv(c1); /* use with care	   */
    else cjk_number_parse(c1);

    if ((c3 & 0x01) == 0) {
    	if (c3 & 0x10) post_oconv(']');
	else post_oconv(')');
    } else { 
	post_oconv('.');
    };
}

/* ---------------------------------------------------------------
    cjk_cc_conv:	0x3200 - 0x32ff parser
    	input:		c2 - input character
	output:		none
*/
static char *uni_cjk_32_u4[4] = {	/* Unic*de 4.0 added	*/
    "Hg","erg","eV","LTD"
};

void CJK_cc_conv(c2)
skf_ucode c2;
{
    int c1;
    char *sym;

    sym = NULL;
    c1 = c2 & 0xff;
    if ((c1 >= 0x20) && (c1 <= 0x43)) {
#if 0
	c1 -= 0x20; post_oconv(0x28);
	post_oconv(uni_k_enl[c1]); 
	post_oconv(0x29);
#else
	CJK_circled(uni_k_enl[c1 - 0x20],0x8);
#endif
    } else if ((c1 >= 0x48) && (c1 <= 0x4f)) {
	post_oconv(0x5b);
	post_oconv(c1 - 0x47+0x30); post_oconv(0x30);
	post_oconv(0x5d);
    } else if ((c1 >= 0x80) && (c1 <= 0x98)) {
#if 0
	c1 -= 0x80; post_oconv(0x28);
	post_oconv(uni_k_enl[c1]);
	post_oconv(0x29);
#else
	CJK_circled(uni_k_enl[c1 - 0x80],0x8);
#endif
    } else if ((c1 >= 0x99) && (c1 <= 0xb0)) {
#if 0
	c1 -= 0x99; post_oconv(0x28);
	post_oconv(uni_k_cil[c1]);
	post_oconv(0x29);
#else
	CJK_circled(uni_k_cil[c1 - 0x99],0x8);
#endif
    } else if (c1 == 0x44) {		/* toi			*/
    	CJK_circled(0x554f,0x8);
    } else if (c1 == 0x45) {		/* you			*/
    	CJK_circled(0x5e7c,0x8);
    } else if (c1 == 0x46) {		/* bun			*/
    	CJK_circled(0x6587,0x8);
    } else if (c1 == 0x47) {		/* sho			*/
    	CJK_circled(0x7b8f,0x8);
    } else if (c1 == 0x50) {		/* PTE			*/
	sym = "PTE";
    } else if ((c1 >= 0x51) && (c1 <= 0x5f)) { /* Enclosed num */
	CJK_circled(c1-0x51+21,0);
    } else if ((c1 >= 0xb1) && (c1 <= 0xbf)) { /* Enclosed num */
	CJK_circled(c1-0xb1+36,0);
    } else if ((c1 >= 0xcc) && (c1 <= 0xcf)) { /* Unic*de 4.0   */
	sym = uni_cjk_32_u4[c1 - 0xcc];
    } else if ((c1 >= 0xd0) && (c1 <= 0xfb)) { /* Enclosed kana */
	post_oconv(0x28);
#ifdef UNICODE_1
	post_oconv(0x2500 + uni_k_iroha[c1-0xd0]); /* Unic*de 1.0 */
#else
	(void)x0201conv((c1-0x9f),0);
#endif
	post_oconv(0x29);
    } else if ((c1 >= 0xfc) && (c1 <= 0xfe)) { /* Enclosed kana */
	CJK_circled(0x2ff4+c1,8);
    } else {		  /* just undefied 		   */
	out_undefined(c2,SKF_OUNDEF);
    };
    if (sym != NULL) {
	SKFSTROUT(sym);
    };
}

/* ---------------------------------------------------------------
    cjk_compat_parse:	0xf900 - 0xfbff parser
    	input:		c2 - input character
	output:		none
*/
static const unsigned short uni_u_compat[364] = {
    0x8c48,0x66f4,0x8eca,0x8cc8, 0x6ed1,0x4e32,0x53e5,0x9f9c,
    0x9f9c,0x5951,0x91d1,0x5587, 0x5948,0x61f6,0x7669,0x7f85,
    0x863f,0x87ba,0x88f8,0x908f, 0x6a02,0x6d1b,0x70d9,0x73de,
    0x843d,0x916a,0x99f1,0x4e82, 0x5375,0x6b04,0x721b,0x862d,
    0x9e1e,0x5d50,0x6feb,0x85cd, 0x8964,0x62c9,0x81d8,0x881f,
    0x5eca,0x6717,0x6d6a,0x72fc, 0x90ce,0x4f86,0x51b7,0x52de,
    0x64c4,0x6ad3,0x7210,0x76e7, 0x8001,0x8606,0x865c,0x8def,
    0x9732,0x9b6f,0x9dfa,0x788c, 0x797f,0x7da0,0x83c9,0x9332,

    0x9e7f,0x8ad6,0x58df,0x5f04, 0x7c60,0x807e,0x7262,0x78ca,
    0x8cc2,0x96f7,0x58d8,0x5c62, 0x6a13,0x6d99,0x6f0f,0x7d2f,
    0x7e37,0x964b,0x52d2,0x808b, 0x51dc,0x51cc,0x7a1c,0x7dbe,
    0x83f1,0x9675,0x8b80,0x62cf, 0x6a02,0x8afe,0x4e39,0x5be7,
    0x6012,0x7387,0x7570,0x5317, 0x78fb,0x4fbf,0x5fa9,0x4e0d,
    0x6ccc,0x6578,0x7d22,0x53c3, 0x585e,0x7701,0x8449,0x8aac,
    0x6bba,0x8fb0,0x6c88,0x62fe, 0x82e5,0x63a0,0x7565,0x4eae,
    0x5169,0x51c9,0x6881,0x7ce7, 0x826f,0x8ad2,0x91cf,0x52f5,

    0x5442,0x5973,0x5eec,0x65c5, 0x7018,0x792a,0x95ad,0x9a6a,
    0x9e97,0x9ece,0x529b,0x66a6, 0x6b74,0x8f62,0x5e74,0x6190,
    0x6200,0x649a,0x6f23,0x7149, 0x7489,0x79ca,0x932c,0x806f,
    0x8f26,0x84ee,0x9023,0x932c, 0x5217,0x52a3,0x54bd,0x70c8,
    0x88c2,0x8aac,0x5ec9,0x5ff5, 0x637b,0x6bae,0x7c3e,0x7375,
    0x4ee4,0x56f9,0x5be7,0x5dba, 0x601c,0x73b2,0x7469,0x7f9a,
    0x8046,0x9234,0x96f6,0x9748, 0x9818,0x4f8b,0x79ae,0x91b4,
    0x96b8,0x60e1,0x4e86,0x50da, 0x5bee,0x5c3f,0x6599,0x6a02,

    0x71ce,0x7642,0x84fc,0x907c, 0x9f8d,0x6688,0x962e,0x5289,
    0x677b,0x67f3,0x6d41,0x6e9c, 0x7409,0x7559,0x786b,0x7d10,
    0x985e,0x516d,0x622e,0x9678, 0x502b,0x5d19,0x6dea,0x8f2a,
    0x7387,0x6144,0x6817,0x7387, 0x9686,0x5229,0x540f,0x5c65,
    0x6613,0x674e,0x68a8,0x6ce5, 0x7406,0x75e2,0x7f79,0x88cf,
    0x88e1,0x91cc,0x96e2,0x533f, 0x6eba,0x541d,0x71d0,0x7498,
    0x85fa,0x96a3,0x9c57,0x9e9f, 0x6797,0x6dcb,0x81e8,0x7acb,
    0x7b20,0x7c92,0x72b6,0x7099, 0x8b58,0x4ec0,0x8336,0x523a,

    /* FA00 - */
    0x5207,0x5ea6,0x62d3,0x7cd6, 0x5b85,0x6d1e,0x66b4,0x8f3b,
    0x884c,0x964d,0x898b,0x5ed3, 0x5140,0x6bbc,0x0000,0x0000,
    0x585a,0x5d0e,0x6674,0x0000, 0x6b05,0x51de,0x732a,0x0000,
    0x793c,0x795e,0x7965,0x798f, 0x9756,0x7cbe,0x7fbd,0x81c8,
    0x0000,0x0000,0x8af8,0x8d73, 0x8fd4,0x9038,0x90fd,0x0000,
    0x0000,0x969d,0x98ef,0x98fc, 0x9928,0x9db4,0x0000,0x0000,
    0x4fae,0x50e7,0x514d,0x52c9, 0x52e4,0x5351,0x559d,0x5606,
    0x5668,0x5840,0x58f8,0x5c64, 0x5c6e,0x6094,0x6168,0x618e,

    0x61f2,0x654f,0x65e2,0x6691, 0x6885,0x6d77,0x6e1a,0x6f22,
    0x716e,0x722b,0x7422,0x7891, 0x793e,0x7948,0x7949,0x7950,
    0x7956,0x795d,0x798d,0x798e, 0x7a40,0x7a81,0x7bc0,0x7df4,
    0x7e09,0x7e41,0x7f72,0x8005, 0x81ed,0x8279,0x8279,0x8457,
    0x8910,0x8996,0x8b01,0x8b39, 0x8cd3,0x8d08,0x8fb6,0x9038,
    0x96e3,0x97ff,0x983b,0x0000
};

void cjk_compat_parse(c2)
skf_ucode	c2;
{
    int		c1,c3;
    skf_ucode	c4;

    c1 = (c2 >> 8) & 0xff;
    c3 = (c2 & 0xff);

    if (c1 == 0xf9) {
	c4 = uni_u_compat[c3];
	post_oconv(c4); return;
    } else if ((c1 == 0xfa) && (c3 < 0x6b)) {
	c4 = uni_u_compat[c3 + 256];
	if (c4 != 0) {
	    post_oconv(c4); return;
	};
    };
    out_undefined(c2,SKF_OUNDEF);
}

/* ---------------------------------------------------------------
    lig_compat:		0xff00 - 0xffef parser
    	input:		c2 - input character
	output:		none
*/
void lig_compat(c2)
skf_ucode c2;
{
    int c1,c4;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr,"(lig)");
#endif
    c1 = c2 & 0xff;
    c4 = (c2 & 0xff00) >> 8;

    if (c4 == 0xff) {
	if (c1 == 0x00) {
	   post_oconv((skf_ucode)(A_SP));
	   post_oconv((skf_ucode)(A_SP));
	} else if (c1 <= 0x60) {
	    out_undefined(c2,SKF_OUNDEF);
	} else switch (c1) {
	    case 0xe0: post_oconv(0x00a2); break;
	    case 0xe1: post_oconv(0x00a3); break;
	    case 0xe2: post_oconv(0x00ac); break;
	    case 0xe3: post_oconv(0x00af); break;
	    case 0xe4: post_oconv(0x00a6); break;
	    case 0xe5: post_oconv(0x00a5); break;
	    case 0xe6: post_oconv(0x20a9); break;
	    default:
		out_undefined(c2,SKF_OUNDEF);
	};
    } else out_undefined(c2,SKF_OUNDEF);
}

/* ---------------------------------------------------------------
    LATIN_lig_conv:	0x2000 - 0x2fff parser
    	input:		c4 - input character
	output:		none
*/
/*@-type@*/
void GRPH_lig_conv(c4)
skf_ucode c4;
{
    skf_ucode	c1,c2;
    const char	*ostr;
    skf_ucode	u0;
    int		mod;

    c1 = (skf_ucode)(c4 & 0xffUL);
    c2 = (skf_ucode)((c4 & 0xff00UL) >> 8);

    if (c2 == 0x20) {
	if (is_keis(conv_cap) &&
		((c1 == 0x20) || (c1 == 0x21))) {
	    SKFKEISEOUT(0x7fcd + c1);
	} else {
	    switch (c1) {
		case 0x36: ox_ascii_conv(0x60);
			ox_ascii_conv(0x60); break;
		case 0x3c: SKFSTROUT("!!"); break;
		case 0x47: SKFSTROUT("??"); break;
		case 0x48: SKFSTROUT("?!"); break;
		case 0x49: SKFSTROUT("!?"); break;
		default: 
		    out_undefined(c4,SKF_OUNDEF);
	    };
	};
    } else if (c2 == 0x21) {
	if (c1 < 0x80) {
	    ostr = uni_f_s_21[c1];
	    if (ostr != NULL) SKFSTROUT(ostr);
	    else out_undefined(c4,SKF_OUNDEF);
	} else if (c1 == 0x89) {
	    SKFSTROUT("0/3");
	} else {
	    out_undefined(c4,SKF_OUNDEF);
	};
    } else if (c2 == 0x22) {  /* Mathmatical operators.	   */
	if (c1 == 0x54) {
	    ox_ascii_conv(':');
	    ox_ascii_conv('=');
	} else if (is_keis(conv_cap)) {
	    if ((c1 < 0xb0) && ((u0 = uni_f_math_jef[c1]) != 0)) {
		SKFKEISEOUT(u0);
	    } else out_undefined(c4,SKF_OUNDEF);
	} else {
	    out_undefined(c4,SKF_OUNDEF);
	};
    } else if (c2 == 0x23) {  /* misc. technicals		*/
	    out_undefined(c4,SKF_OUNDEF);
    } else if (c2 == 0x24) {  /* control picture & enclose ch. */
	if (((c1 >= 0x60) && (c1 <= 0x9b)) 
	    || ((c1 >= 0xeb) && (c1 <= 0xfe))) {
	    u0 = c1 - 0x5fL - ((c1 >= 0x74L) ? 20L : 0L)
			- ((c1 >= 0x88L) ? 20L : 0L)
			- ((c1 >= 0xebL) ? 89L : 0L)
			- ((c1 >= 0xf5L) ? 20L : 0L);
	    if ((c1 < 0x88) || (c1 >= 0xeb)) mod = 0;
	    else mod = 1;
	    CJK_circled(u0,mod);
	} else if ((c1 >= 0x9c) && (c1 <= 0xb5)) {
	    CJK_circled(c1-0x9c,0x04);
	} else if ((c1 >= 0xb6) && (c1 <= 0xcf)) {
	    CJK_circled(c1-0xb6,0x02);
	} else if ((c1 >= 0xd0) && (c1 <= 0xe9)) {
	    CJK_circled(c1-0xd0,0x04);
	} else if (c1 == 0xea) {
	    SKFSTROUT("(0)");
	} else {
	    out_undefined(c4,SKF_OUNDEF);
	};
    } else if (c2 == 0x25) {  /* Form and Chart components.   */
	if (is_keis_jef(conv_cap) &&
	    ((c1 == 0x25) || (c1 == 0x30) || (c1 == 0x1d)
			|| (c1 == 0x38) || (c1 == 0xef))) {
	    if (c1 == 0x38) SKFKEISEOUT(0x7fa1);
	    else if (c1 == 0x1d) SKFKEISEOUT(0x7fa2);
	    else if (c1 == 0x30) SKFKEISEOUT(0x7fa3);
	    else if (c1 == 0x25) SKFKEISEOUT(0x7fa4);
	    else SKFKEISEOUT(0x7ff0);
	} else {
	    if ((c1 <= 0x7f) && (skf_is_ja(output_lang))) { 
		u0 = moji_kei[c1];
		post_oconv(u0); 
	    } else out_undefined(c4,SKF_OUNDEF); 
	};
    } else if (c2 == 0x26) {  /* Misc. dingbats		   */
	if (is_keis_jef(conv_cap) && 
	    ((c1 == 0x6f) || (c1 == 0x6d) || (c1 == 0x6a))) {
	    if (c1 == 0x6a) SKFKEISEOUT(0x7fec);
	    else if (c1 == 0x6d) SKFKEISEOUT(0x7feb);
	    else SKFKEISEOUT(0x7fea);
	} else out_undefined(c4,SKF_OUNDEF);
    } else if (c2 == 0x27) {  /* Zapf dingbats		   */
	if ((c1 >= 0x76) && (c1 <= 0x7f)) {
	    CJK_circled(c1 - 0x75,0);
	} else if ((c1 >= 0x80) && (c1 <= 0x89)) {
	    CJK_circled(c1 - 0x7f,0);
	} else if ((c1 >= 0x8a) && (c1 <= 0x93)) {
	    CJK_circled(c1 - 0x89,0);
	} else if ((c1 >= 0x01) && (c1 <= 0x04)) {
	    ox_ascii_conv('8');
	    ox_ascii_conv('<');
	} else {
		out_undefined(c4,SKF_OUNDEF);
	};
    } else if (c2 == 0x28) {  /* misc dingbats	   	   */
	out_undefined(c4,SKF_OUNDEF);
    } else if (c2 == 0x29) {  /* Supprementary arrow 	   */
	out_undefined(c4,SKF_OUNDEF);
    } else if (c2 == 0x2a) {  /* Suppremental Math Operators   */
	out_undefined(c4,SKF_OUNDEF);
    } else if (c2 == 0x2b) {  /* misc symbols		   */
    	if (c1 == 0x1b) post_oconv((skf_ucode)(0x25a0));
    	else if (c1 == 0x1c) post_oconv((skf_ucode)(0x25a1));
    	else if (c1 == 0x24) post_oconv((skf_ucode)(0x25cf));
    	else if (c1 == 0x25) post_oconv((skf_ucode)(0x25c6));
    	else if (c1 == 0x26) post_oconv((skf_ucode)(0x25c7));
    	else if (c1 == 0x55) post_oconv((skf_ucode)(0x25ef));
    	else if (c1 == 0x58) post_oconv((skf_ucode)(0x25ef));
	else if ((c1 >= 0x60) && (c1 <= 0x69))
		post_oconv((skf_ucode)(0x2190 + (c1 - 0x60)));
	else if ((c1 >= 0x6a) && (c1 <= 0x6d))
		post_oconv((skf_ucode)(0x21e0 + (c1 - 0x6a)));
	else if ((c1 >= 0x84) && (c1 <= 0x87))
		post_oconv((skf_ucode)(0x21c7 + (c1 - 0x84)));
    	else if (c1 == 0xbd) post_oconv((skf_ucode)(0x1f147));
    	else if (c1 == 0xbe) post_oconv((skf_ucode)(0x24e7));
    	else if (c1 == 0xbf) post_oconv((skf_ucode)(0x24e7));
    	else if (c1 == 0xc0) post_oconv((skf_ucode)(0x25a0));
    	else if (c1 == 0xc5) post_oconv((skf_ucode)(0x25b2));
    	else if (c1 == 0xc6) post_oconv((skf_ucode)(0x25bc));
	else out_undefined(c4,SKF_OUNDEF);
    } else {		  /* 0x2b00-0x2dff		   */
	out_undefined(c4,SKF_OUNDEF);
    };
}
/*@+type@*/
/* ---------------------------------------------------------------
    ascii_fract_conv:	convert iso-8859 fractions and glyphs
    	input:		c1 - input character
	output:		none
*/
void ascii_fract_conv(c1)
skf_ucode c1;
{
    if (skf_is_ja(output_lang) && (c1 == 0xa6)) { /* broken bar	   */
	 post_oconv(0x2223);
    } else if (is_keis(conv_cap) && is_keis_jef(conv_cap) 
		&& (c1 == 0xb6)) {
	 SKFKEISEOUT(0x7fef);
    } else if (c1 == 0xa9) {	/* copyright 	   */
	 SKFSTROUT("(c)");
    } else if (c1 == 0xaf) {	/* macron 	   */
	 post_oconv(0x0305);
    } else if (c1 == 0xbc) {	/* fractions	   */
	 SKFSTROUT("1/4");
    } else if (c1 == 0xbd) {	/* fractions	   */
	 SKFSTROUT("1/2");
    } else if (c1 == 0xbe) {	/* fractions	   */
	 SKFSTROUT("3/2");
    } else {
	out_undefined(c1,SKF_OUNDEF);
    };
}

/* ---------------------------------------------------------------
    enclose_suppl_conv:	Enclosed Alphanumeric supplement conv.
    			1f100 - 1f1ff
    	input:		c1 - input character
	output:		none
*/
static char *u_bra_lig_supl[9] = {
  "CL","COOL","FREE","ID","NEW","NG","OK",
  "UP!","VS"
};

void enc_alpha_supl_conv(c1)	/* 1f100 - 1f200 */
skf_ucode c1;
{
    int c2;

#ifdef SKFDEBUG
    if (is_vvv_debug) {
    	fprintf(stderr,"-EnSC:%x ",c1);
    } else;
#endif

    if (c1 < 0x1f110) {
        if (c1 == 0x1f100) {
	    CJK_circled('0',0x09);
	} else if (c1 <= 0x1f10a) {
	    post_oconv(((skf_ucode)(c1 - 0x1f101 + 0x30)));
	    post_oconv(((skf_ucode)(',')));
	} else {
	    out_undefined(c1,SKF_OUNDEF);
	};
    } else if (c1 <= 0x1f190) {
    	if (c1 < 0x1f130) {
	    c2 = c1 - 0x1f110;
	} else if (c1 < 0x1f150) {
	    c2 = c1 - 0x1f130;
	} else if (c1 < 0x1f170) {
	    c2 = c1 - 0x1f150;
	} else c2 = c1 - 0x1f170;
	if (c2 < 0x1a) {
	    CJK_circled((c2 + 0x41),
	    	(((c1<0x1f130) || ((c1>=0x1f150) && (c1<0x1f170)))
			? 0x08 : 0x18));
	} else {
	    switch(c1) {
	    	case 0x1f12a: SKFSTROUT("[S]"); break;
	    	case 0x1f12b: SKFSTROUT("(C)"); break;
	    	case 0x1f12c: SKFSTROUT("(R)"); break;
	    	case 0x1f12d: SKFSTROUT("(CD)"); break;
	    	case 0x1f12e: SKFSTROUT("(Wz)"); break;
	    	case 0x1f14a: SKFSTROUT("[HV]"); break;
	    	case 0x1f14b: SKFSTROUT("[MV]"); break;
	    	case 0x1f14c: SKFSTROUT("[SD]"); break;
	    	case 0x1f14d: SKFSTROUT("[SS]"); break;
	    	case 0x1f14e: SKFSTROUT("[PPV]"); break;
	    	case 0x1f14f: SKFSTROUT("[WC]"); break;
	    	case 0x1f16a: SKFSTROUT("MC"); break;
	    	case 0x1f16b: SKFSTROUT("MD"); break;
	    	case 0x1f18a: SKFSTROUT("[-P-]"); break;
	    	case 0x1f18b: SKFSTROUT("[IC]"); break;
	    	case 0x1f18c: SKFSTROUT("[PA]"); break;
	    	case 0x1f18d: SKFSTROUT("[SA]"); break;
	    	case 0x1f18e: SKFSTROUT("[AB]"); break;
	    	case 0x1f18f: SKFSTROUT("[WC]"); break;
	    	case 0x1f190: SKFSTROUT("DJ"); break;
	    	default: out_undefined(c1,SKF_OUNDEF);
	    };
	};
    } else if (c1 <= 0x1f19a) {
	post_oconv((skf_ucode)('['));
    	SKFSTROUT(u_bra_lig_supl[c1 - 0x1f191]);
	post_oconv((skf_ucode)(']'));
    } else if (c1 >= 0x1f1e6) {
	post_oconv(((skf_ucode)(c1 - 0x1f1e6 + 0x41)));
    } else {
	out_undefined(c1,SKF_OUNDEF);
    };
}

static skf_ucode u_bra_lig_cjk_s[55] = {
    0x30b5,0x624b,0x5b57,0x53cc,0x30c7,0x4e8c,0x591a,0x89e3,
    0x5929,0x4ea4,0x6620,0x7121,0x6599,0x524d,0x5f8c,0x518d,
    0x65b0,0x521d,0x7d42,0x751f,0x8ca9,0x58f0,0x5439,0x6f14,
    0x6295,0x6355,0x4e00,0x4e09,0x904a,0x5de6,0x4e2d,0x53f3,
    0x6307,0x8d70,0x6253,0x7981,0x7a7a,0x5408,0x6e80,0x6709,
    0x6708,0x7533,0x5272,0x55b6,0x672c,0x4e09,0x4e8c,0x5b89,
    0x70b9,0x6253,0x76d7,0x52dd,0x6557,0x5f97,0x53ef
};

/* ---------------------------------------------------------------
    enclose_cjk_suppl_conv:	Enclosed CJK supplement conv.
    			1f200 - 1f2ff
    	input:		c1 - input character
	output:		none
*/
void enc_cjk_supl_conv(c1)	/* 1f200 - 1f2ff */
skf_ucode c1;
{
    int c2;

#ifdef SKFDEBUG
    if (is_vvv_debug) {
    	fprintf(stderr,"-EnSK:%x ",c1);
    } else;
#endif
    c2 = c1 - 0x1f202;
    if (c1 == 0x1f200) {
	post_oconv((skf_ucode)('['));
    	post_oconv((skf_ucode)0x307b);
    	post_oconv((skf_ucode)0x304b);
	post_oconv((skf_ucode)(']'));
	return;
    } else if (c1 == 0x1f201) {
	post_oconv((skf_ucode)('['));
    	post_oconv((skf_ucode)0x30b3);
    	post_oconv((skf_ucode)0x30b3);
	post_oconv((skf_ucode)(']'));
	return;
    } else if ((c1 >= 0x1f210) && (c1 <= 0x1f23a)) {
    	c2 = c1 - 0x1f210 + 1;
    } else if ((c1 >= 0x1f240) && (c1 <= 0x1f248)) {
    	c2 = c1 - 0x1f240 + 1 + 43;
    } else if ((c1 >= 0x1f250) && (c1 <= 0x1f251)) {
    	c2 = c1 - 0x1f250 + 1 + 43 + 9;
    } else {
	out_undefined(c1,SKF_OUNDEF);
	return;
    };
    CJK_circled(u_bra_lig_cjk_s[c2],0x18);
    return;
}
/* ---------------------------------------------------------------
    kana_force_conv:	convert miscellaneous kana part
    			0x3000 - 0x3400 - called from skf_lastresort
    	input:		c2 - input character
	output:		none
*/
void kana_force_conv(c2)
skf_ucode c2;
{
    int	converted = 0;
    if (c2 < 0x3100) {	      /* other 3000-30ff 	   */
	if (c2 == 0x3013) {
	    post_oconv(0x25a0);    /* convert to black square   */
	    converted = 1;
	} else if (c2 == 0x301f) {  /* Low double quote	   */
	    post_oconv((skf_ucode)(','));
	    post_oconv((skf_ucode)(','));
	    converted = 1;
	} else if (c2 == 0x3036){ /* ARIB bracketted post-mark */
	    CJK_circled(0x3012,0x8);
	    converted = 1;
	} else if (c2 == 0x303f){ /* ideographic single space*/
	    post_oconv((skf_ucode)(A_SP));
	    converted = 1;
	} else if ((c2 == 0x3094) && (uni_o_kana != NULL)
		    && (uni_o_kana[0x9b] != 0)) {
			/* hiragana u voiced sound mark	   */
	    post_oconv(0x3046);
	    post_oconv(0x309b);
	    converted = 1;
	} else ;
    } else if (c2 < 0x3200) {	/* other 3100-31ff	   */
	;
    } else if (c2 < 0x3300) {	/* other 3200-32ff	   */
	CJK_cc_conv(c2);	/* this one is i17n'd	   */
	converted = 1;
    } else if (c2 < 0x3400) {	/* other 3300-33ff	   */
	CJK_sq_conv(c2);	/* this one is i17n'd	   */
	converted = 1;
    } else;
    if (converted == 0)
	    out_undefined(c2,SKF_OUNDEF);
}

/* ---------------------------------------------------------------
    print_announce:	print announcer sequence for fj-code
    	input:		o_code - output codeset (by number)
	output:		none
*/
static unsigned short euc_announce_str[] = {
    A_ESC,A_SP,'P', A_ESC,A_SP,'S', A_ESC,A_SP,'Z', A_ESC,A_SP,'[',
    0x100
};
static unsigned short jis_announce_str1[] = {
    A_ESC,A_SP,'B',A_ESC,')','I',0x100
};
static unsigned short jis_announce_str2[] = {
    A_ESC,A_SP,'A',0x100
};

void print_announce(o_code)
int	o_code;
{
    int i;
    unsigned long	ais_kana;
    char		defschar;

		/* G0 = X-0208/X-0201, G2 = kana, G3 = X-0212	   */
		/* if jis, G1 = kana: if euc, G1 = X-0208	   */
    if (input_inquiry) return;
    if (is_jis(conv_cap)) {/* note: we use input side (for simplify) */
	if ((o_code == codeset_jis) || (o_code == codeset_rfc1554)
	  || (o_code == codeset_rfc1554_kr) || (o_code == codeset_x0213)
	  || (o_code == codeset_x0213_s) || (o_code == codeset_x213a)) {
	    if (si_enbl) { SKF_STRPUT(jis_announce_str1);
	    } else { SKF_STRPUT(jis_announce_str2); };
	};
    } else if (is_euc(conv_cap) || is_euc7(conv_cap)) {
      if ((o_code == codeset_eucjp) 	/* euc-jp		   */
	  || (o_code == codeset_euc_213)	/* JIS x0213(2000) */
	  || (o_code == codeset_euc_213a)) {	/* JIS x0213(2004) */
	SKF_STRPUT(euc_announce_str);
      };
      for (i=1;i<4;i++) {
	if (i==1) {
	    defschar = g1_char; ais_kana = g1_typ;
	} else if (i==2) {
	    defschar = g2_char; ais_kana = g2_typ;
	} else {
	    defschar = g3_char; ais_kana = g3_typ;
	};
	if (defschar != 0) {
	  SKFputc(A_ESC);
	  if (is_tbl_mb(ais_kana)) {
	      SKFputc('$'); 
	      SKFputc(0x28+i); 
	  } else if (is_tbl_set96(ais_kana)) {	/* iso-8859 family */
	      SKFputc(0x2c+i); 
	  } else if (is_tbl_b4(ais_kana)) { /* intermediate byte   */
	      SKFputc(0x21); SKFputc(0x28+i); 
	  } else {			/* single 94 char	   */
	      SKFputc(0x28+i); 
	  };
	  SKFputc(defschar);
	};

      };
    } else ;	/* do nothing if output is MS-JIS code		   */
}

/* ---------------------------------------------------------------
    show_lang_tag:	print language tag defined in > Unic*de 3.2
			and B-Right/V
    	input/output:	none
*/

void show_lang_tag()
{
    unsigned long ll;

    if (input_inquiry) return;
    if (is_lang_tag_enbl && !limit_ucs2 && is_ucs_ufam(conv_cap)) { 
				/* Unic*de language tag support    */
	if ((skf_is_strong_lang(skf_output_lang)) || (is_lang_tag_encr)) {
	    ll = skf_get_langcode(skf_output_lang);
	    o_ozone_conv(0xe0001); 
	    SKFputc((int)((ll >> 8) & 0x7f));
	    SKFputc((int)(ll & 0x7f));
	};
    } else if (is_ucs_brgt(conv_cap)) { /* TAD Announcer	   */
	tron_announce();
    };
}

/* ---------------------------------------------------------------
    show_endian_out:	print endian specify sequence
    	input/output:	none
*/

void show_endian_out()
{
    if (input_inquiry) return;
#ifdef ACE_SUPPORT
    if (is_o_encode_ace(o_encode)) return;
#endif
    if (is_ucs_utf16(conv_cap)) {	/* UTF-16		   */
      if (is_ucs_utf32(conv_cap)) {
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ucs4-bom\n");
#endif
	if (out_endian(conv_cap)) {
	    SKFputc(0x00); SKFputc(0x00);
	    SKFputc(BOM_LOW); SKFputc(BOM_HIGH);
	} else {
	    SKFputc(BOM_HIGH); SKFputc(BOM_LOW);
	    SKFputc(0x00); SKFputc(0x00);
	};
      } else {
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ucs2-bom\n");
#endif
	if (out_endian(conv_cap)) {
	    SKFputc(BOM_LOW); SKFputc(BOM_HIGH);
	} else {
	    SKFputc(BOM_HIGH); SKFputc(BOM_LOW);
	};
      };
    } else if (is_ucs_utf8(conv_cap)) { /* UTF-8		   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," utf8-bom\n");
#endif
	SKFputc(0xef); SKFputc(0xbb); SKFputc(0xbf); 
    };				/* else: do nothing		   */
    show_lang_tag();
}

/* ---------------------------------------------------------------
    various x221 support utilities
*/
static int x221_skip2 = 0;

int	skip_x221_2_null()
{
    x221_skip2++; if (x221_skip2 == 2) x221_skip2 = 0;
    return(x221_skip2);
}

/* --------------------------------------------------------------- 
  various KEIS support routines
    keis_conv(): KEIS90 NON-JIS PART conversion
    jef_conv(): JEF NON-JIS PART conversion
    	input:		c2 - upper input character
    			c1 - lower input character
	output:		none
*/

#ifdef KEIS_EXTRA_SUPPORT
static const unsigned short KEIS_9F[56] = { 
    	   0x4f03,0x4f0b,0x5215, 0x5271,0x5786,0x57c7,0x57de,
    0x37e2,0x5d6d,0x625a,0x677b, 0x67c0,0x67f5,0x6935,0x6980,
    0x6992,0x6a73,0x6a9e,0x6ae4, 0x6c97,0x6e57,0x6ebf,0x6f8d,
    0x6f97,0x710f,0x72be,0x73ca, 0x757e,0x7682,0x7823,0x784e,
    0x7a5d,0x7ca0,0x7cd9,0x81fa, 0x828e,0x8330,0x845a,0x85ad,
    0x880b,0x8983,0x92d3,0x93ba, 0x958d,0x9733,0x973b,0x974f,
    0x9b2e,0x9b66,0x9b75,0x9c00, 0x9c63,0x9d96,0x9d1e,0x9d62,
    0x9d70
};

static const unsigned short KEIS_AF[36] = { /* afa1 - afc4		*/
	   0x250c,0x250d,0x250f, 0x2510,0x2511,0x2513,0x2514,
    0x2515,0x2517,0x2518,0x2519, 0x251b,0x251c,0x251d,0x2523,
    0x2524,0x2525,0x252b,0x252c, 0x252d,0x2533,0x2534,0x2535,
    0x253b,0x253c,0x253f,0x254b, 0x2500,0x2500,0x2501,0x2502,
    0x2502,0x2503,0x254e,0x254f, 0x2236
};

static const char *KEIS_6F[64] = {	/* 6fbd - 6ffe */
    "Re",

    NULL, "arg", "exp", "sin", "cos", "tan", "cot", "sec",
    "div", "rot", NULL, NULL, NULL, NULL, NULL, NULL, 

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "mH", "uH", NULL, NULL, "Ci", "mCi", NULL, NULL,

    NULL, NULL, NULL, NULL, NULL, "bit", "keV", "eV",
    NULL, NULL, NULL, "car", NULL, NULL, NULL
};


static const char *KEIS_70[46] = {	/* 70a1 - 70ce */
    NULL, NULL, NULL, NULL, "St", NULL, NULL,
    NULL, NULL, NULL, "knot", NULL, "min", NULL, "kgf",

    NULL, "VA", "Ah", NULL, "erl", "Np", NULL, "dBm",
    NULL, "Bpu", NULL, NULL, "rem", NULL, NULL, "1/C",

    "1/H", "1/m", "1/m2", "1/m3", "s2", "1/s", "1/s2", "1/kg",
    "1/K", "1/Pa", NULL, "1/mol", "/s", "/m", "/h"
};

static const char *KEIS_71[6] = {	/* 71cb - 71d0		   */
    "As", "KVA", "PL", "KWH", "max", "l"
};

static const char *KEIS_72[35] = {	/* 72a1 - 72c3 */
           "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL",
    "AUG", "SEP", "OCT", "NOV", "DEC", "SUN", "MON", "TUE",

    "WED", "THU", "FRI", "SAT", "Co.,", NULL, NULL, "Ltd.",
    NULL, NULL, "AM", "PM", NULL, NULL, NULL, NULL,

    "EOF", NULL, NULL, "L/c"
};

static const unsigned short KEIS_72a[74] = { /* 72b5 - 72fe	   */
					 0x0000, 0x0000,0x0000,
    0x0000,0x0000, 0x0000,0x0000, 0x0000,0x0000, 0x0000,0x0000,
    0x0000,0x0000, 0x0000,0x0000, 0x5e97,0x5e97, 0x4e0a,0x4e0b,
    0x964d,0x524d, 0x5f8c,0x6e80, 0x671f,0x671f, 0x6708,0x671f,

    0x524d,0x3005, 0x540c,0x6bd4, 0x6bd4,0x8a08, 0x4f4d,0x9662,
    0x5b66,0x5927, 0x5c02,0x6821, 0x5b66,0x5b66, 0x884c,0x58f2,
    0x9023,0x793e, 0x8ca1,0x696d, 0x7d44,0x91d1, 0x91d1,0x5de5,
    0x6e08,0x7d44, 0x56e3,0x6cd5, 0x4e8b,0x7d44, 0x9023,0x91d1,
    0x5186,0x5186, 0x0000,0x0000, 0x5186,0x0000, 0x0000,0x0000,
    0x0000,0x0000, 0x0000,0x0000, 0x2116,0x4eba, 0x4eba
};

static const char KEIS_73a[94] = {	 /* 73a1 - 72fe		   */
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,1,0,1,0,0,1,0,1,0,0,1,0,1,0,
    0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0, 0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1, 1,0,0,1,0,1,1,0,1,1,0,0,0,0,0
};
#endif

/* --------------------------------------------------------------- */
/* Note: KEIS has many ligatures, and more than half of them are   */
/*	not within UNIC*DE.					   */
/* --------------------------------------------------------------- */

void keis_conv(c2,c1)
int	c2,c1;
{
#ifdef KEIS_EXTRA_SUPPORT
    skf_ucode k0 = 0, k1;
    int	c_loc;
    const char *sym = NULL;
    skf_ucode *keis_t;
    int		len;

    keis_t = ovlay_byte_defs[keis_host_index].uniltbl;

    if ((c1 <= 0xa0) || (c1 >= 0xff) || (c2 < 0x59)
	|| (keis_t == NULL)) {
	    in_sbroken(c2,c1); return;
    };
/* --- various kanji area ---------------------------------------- */
/* 	supported X-0208 kanji only				   */
    c_loc = (c2 - 0x59) * 94 + ((c1 & 0x7f) - 0x21);
    if (c2 < 0x81) {
	k0 = keis_t[c_loc];
	if (c2 == 0x6f) {		/* unit ligatures	*/
	    if ((k0 == 0) && (c1 >= 0xbf)) {
		sym = KEIS_6F[c1 - 0xbf];
	    } else;
	} else if (c2 == 0x70) {	/* unit ligatures	*/
	    if (c1 <= 0xce) sym = KEIS_70[c1 - 0xa1];
	    if (sym != NULL) k0 = 0;
	} else if (c2 == 0x71) {	/* various ligatures	*/
	    if ((c1 >= 0xcb) && (c1 <= 0xd0)) {
		sym = KEIS_71[c1 - 0xcb];
	    } else if ((c1 >= 0xc1) && (c1 < 0xcb)) {
		oconv('.');
	    };
	} else if (c2 == 0x72) {	/* fukugou kanji */
	    if ((c1 >= 0xa1) && (c1 <= 0xc3)) {
		sym = KEIS_72[c1 - 0xa1];
	    } else ;
	    switch (c1) {
		case 0xf2: /* hyakuman-en */ oconv(0x767e); oconv(0x4e07);
		    break;
		case 0xf3: /* senman-en */ oconv(0x5343); oconv(0x4e07);
		    break;
		case 0xfc: /* kouza no. */ oconv(0x56d7); break;
		case 0xfd: /* zaidanhoujin */ oconv(0x8ca1); oconv(0x56e3);
		    break;
		case 0xfe: /* shadanhoujin */ oconv(0x793e); oconv(0x56e3);
		    break;
		default: break;
	    };
	    if ((c1 >= 0xb5) && (sym == NULL)) {
		k1 = KEIS_72a[c1 - 0xb5]; /* secondary byte	   */
		if (k1 != 0) {
		    oconv(k0); k0 = k1;
		};
	    };
	} else if (c2 == 0x73) {  /* bracked'd kanji and ligatures */
	    if ((c1 >= 0xa7) && (c1 <= 0xac)) {
		oconv('/');
	    } else if (KEIS_73a[c1 - 0xa1] != 0) {
		oconv('(');
	    } else;
	    if (KEIS_73a[c1 - 0xa1] == 1) {
		oconv(k0); k0 = ')';
	    } else;
	    if (c1 == 0xa1) {		/* shuukyou-houjin	   */
		oconv(0x5b97); oconv(0x6559); oconv(0x6cd5);
	    } else if (c1 == 0xa2) {	/* tokushu-houjinn	   */
		oconv(0x7279); oconv(0x6b8a); oconv(0x6cd5);
	    } else if (c1 == 0xa3) {	/* gouben-gaisha	   */
		oconv(0x5408); oconv(0x5f01); oconv(0x4f1a);
	    } else if (c1 == 0xa4) {	/* goushi-gaisha	   */
		oconv(0x5408); oconv(0x8cc7); oconv(0x4f1a);
	    } else if (c1 == 0xa5) {	/* yuugen-gaisha	   */
		oconv(0x6709); oconv(0x9650); oconv(0x4f1a);
	    } else if (c1 == 0xa6) {	/* gakkou-houjin	   */
		oconv(0x5b66); oconv(0x6821); oconv(0x6cd5);
	    } else if (c1 == 0xad) {	/* yori			   */
		oconv(0x3088);
	    } else if (c1 == 0xae) {	/* kara			   */
		oconv(0x304b);
	    } else if (c1 == 0xaf) {	/* ma-de		   */
		oconv(0x307e);
	    } else if (c1 == 0xb0) {	/* hoka			   */
		oconv(0x307b);
	    } else if (c1 == 0xd1) {	/* sen-en */
		oconv(0x5343); oconv(0x5186); 
	    } else ;
	} else if (c2 == 0x74) {	/* enclosed latin digits   */
	    if ((k0 != 0) && (c1 >= 0xd0) && (c1 < 0xf0)) {
		oconv('('); 
		oconv(k0); k0 = ')';
	    };
	} else if (c2 == 0x75) {	/* enclosed alphabets */
	    if ((k0 != 0) && (c1 >= 0xc0) && (c1 < 0xe0)) {
		oconv('('); 
		oconv(k0); k0 = ')';
	    };
	} else if (c2 == 0x77) {	/* enclosed digits ++	   */
	    if ((k0 == 0) && (c1 <= 0xf9)) {
		oconv_flush();
		CJK_circled((c1 - 0x96),0);
		return;
	    } else if ((c1 >= 0x2a) && (c1 <= 0xf9)) {
		oconv_flush();
		CJK_circled((c1 - 0x96),0);
		return;
	    } else;
	} else if (c2 == 0x78) { 	/* enclosed kana   */
	    if (c1 <= 0xd0) {
		oconv('('); 
		oconv(k0); k0 = ')';
	    };
	} else if (c2 == 0x79) { 	/* enclosed kana   */
	    if (c1 == 0xd0) {
		oconv('('); 
		oconv(k0); k0 = ')';
	    };
	} else;
    } else if (c2 == 0x9F) {		
        if ((c1 < 0xd9) && ((k0 = KEIS_9F[c1 - 0xa1]) != 0)){
	   oconv(k0); 
	} else {
	    in_sbroken(c2,c1);
	};
	return;
    } else if (c2 == 0xaf) {		
	if ((c1 >= 0xa1) && (c1 <= 0xc4)) {
	    oconv(KEIS_AF[c1-0xa1]);
	} else in_sbroken(c2,c1);
	return;
    } else {	/* in euc area					   */
	in_sbroken(c2,c1);
	return;
    };
    if (sym != NULL) {
	for (len = 0; (len < 24) && (*sym != '\0') ; len++, sym++) {
	    oconv(*sym);
	};
    } else if (k0 != 0) {
	oconv(k0);
    } else in_sbroken(c2,c1);
#endif
    return;
}

/* --------------------------------------------------------------- */
/* JEF support: similar as KEIS, but code is completely different  */
/* --------------------------------------------------------------- */

void jef_conv(c2,c1)
int	c2,c1;
{
    skf_ucode k0;
    int	c_loc;
    unsigned short *jef_t;

    c1 &= 0x7f;
    if ((c1 <= 0x20) || (c1 == 0x7f) || (c2 < 0x43) || (c2 > 0x7f)) {
	    in_sbroken(c2,c1); return;
    };
/* --- various kanji area ---------------------------------------- */
/* 	supported X-0208 kanji only				   */
    jef_t = (ovlay_byte_defs[jef_host_index].unitbl);

    c_loc = (c2 - 0x43) * 94 + (c1 - 0x21);
    k0 = jef_t[c_loc];
    if (k0 != 0) oconv(k0);
    else in_sbroken(c2,c1);
    return;
}

/* --------------------------------------------------------------- */
/* skf_lastresort: undefined character treatments		   */
/* --------------------------------------------------------------- */
void skf_lastresort(c1)
skf_ucode c1;
{
    int enbl = FALSE;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr,"-LR(%x)",(unsigned short)c1);
#endif
    if (use_latin2html) enbl = latin2html(c1);
    if (use_latin2tex && !enbl) enbl = latin2tex(c1);
    if (use_latin2null || is_no_bfc) return;	/* discard 'em	   */

    if (!enbl) {
	if ((c1 >= 0x3000) && (c1 <= 0x4e00) &&
	    (out_codeset != codeset_cp932w)) {
	    kana_force_conv(c1);
	    enbl = TRUE;
	} else if ((c1 >= 0xf900) && (c1 <= 0x10000) &&
	    (out_codeset != codeset_cp932w)) {
	    lig_compat(c1);
	    enbl = TRUE;
	};
    };
    if (!enbl) {
	out_undefined(c1,SKF_OUNDEF);
    };
}

/* --------------------------------------------------------------- */
int skf_lastcount(c1)
skf_ucode c1;
{
    int val = 0;

    if (use_latin2html) val = 0;
    if (use_latin2tex && (val == 0)) val = 0;

    if (val == 0) {
	if ((c1 >= 0x3000) && (c1 <= 0x4e00) &&
	    (out_codeset != codeset_cp932w)) {
	    val = 0;
	} else if ((c1 >= 0xf900) && (c1 <= 0x10000) &&
	    (out_codeset != codeset_cp932w)) {
	    val = 0;
	};
    };
    return(val);
}

/* --------------------------------------------------------------- */
/* latin2html: iso8859 non-ascii to sgml/html quoted expression	   */
/* --------------------------------------------------------------- */
static const char *uni_latin_html_0[96] = {	/* 0x20 - 0x7f	   */
    NULL, "&excl;", "&quot;", "&num;", 
    "&dollar;", "&percnt;", "&amp;", "&apos;",
    "&lpar;", "&rpar;", NULL, NULL, NULL, NULL, NULL, "&sol;",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, "&colon;", "&semi;", "&lt;", NULL, "&gt;", "&quest;",

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, "&sbsol;", NULL, NULL, NULL,

    "&grave;", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static const char *uni_latin_html_1[96] = {	/* 0xa0 - 0xff	   */
	"&nbsp;", "&iexcl;", "&cent;", "&pound;",
	"&curren;", "&yen;", "&brvbar;", "&sect;",
	"&uml;", "&copy;", "&ordf;", "&laquo;",
	"&not;", "&shy;", "&reg;", "&macr;",

	"&deg;", "&plusmn;", "&sup2;", "&sup3;",
	"&acute;", "&micro;", "&para;", "&middot;",
	"&cedil;", "&sup1;", "&ordm;", "&raquo;",
	"&frac14;", "&frac12;", "&frac34;", "&iquest;",

	"&Agrave;", "&Aacute;", "&Acirc;", "&Atilde;",
	"&Auml;", "&Aring;", "&AElig;", "&Ccedil;",
	"&Egrave;", "&Eacute;", "&Ecirc;", "&Euml;",
	"&Igrave;", "&Iacute;", "&Icirc;", "&Iuml;",

	"&ETH;", "&Ntilde;", "&Ograve;", "&Oacute;",
	"&Ocirc;", "&Otilde;", "&Ouml;", "&times;",
	"&Oslash;", "&Ugrave;", "&Uacute;", "&Ucirc;",
	"&Uuml;", "&Yacute;", "&THORN;", "&szlig;",

	"&agrave;", "&aacute;", "&acirc;", "&atilde;",
	"&auml;", "&aring;", "&aelig;", "&ccedil;",
	"&egrave;", "&eacute;", "&ecirc;", "&euml;",
	"&igrave;", "&iacute;", "&icirc;", "&iuml;",

	"&eth;", "&ntilde;", "&ograve;", "&oacute;",
	"&ocirc;", "&otilde;", "&ouml;", "&divide;",
	"&oslash;", "&ugrave;", "&uacute;", "&ucirc;",
	"&uuml;", "&yacute;", "&thorn;", "&yuml;"
};

static const char *uni_latin_html_2[128] = {	/* 0x100 - 0x17f   */
	"&Amacr;", "&amacr;", "&Abreve;", "&abreve;",
	"&Aogon;", "&aogon;", "&Cacute;", "&cacute;",
	"&Ccirc;", "&ccirc;", "&Cdot;", "&cdot;",
	"&Ccaron;", "&ccaron;", "&Dcaron;", "&dcaron;",

	"&Dstrok;", "&dstrok;", "&Emacr;", "&emacr;",
	"&Ebreve;", "&ebreve;", "&Edot;", "&edot;",
	"&Eogon;", "&eogon;", "&Ecaron;", "&ecaron;",
	"&Gcirc;", "&gcirc;", "&Gbreve;", "&gbreve;",

	"&Gdot;", "&gdot;", "&Gcedil;", "&gcedil;",
	"&Hcirc;", "&hcirc;", "&Hstrok;", "&hstrok;",
	"&Itilde;", "&itilde;", "&Imacr;", "&imacr;",
	"&Ibreve;", "&ibreve;", "&Iogon;", "&iogon;",

	"&Idot;", "&inodot;", "&IJlig;", "&ijlig;",
	"&Jcirc;", "&Jcirc;", "&Kcedil;", "&kcedil;",
	"&kgreen;", "&Lacute;", "&lacute;", "&Lcedil;",
	"&lcedil;", "&Lcaron;", "&lcaron;", "&Lmidot;",

	"&lmidot;", "&Lstrok;", "&lstrok;", "&Nacute;",
	"&nacute;", "&Ncedil;", "&ncedil;", "&Ncaron;",
	"&ncaron;", "&napos;", "&ENG;", "&eng;",
	"&Omacr;", "&omacr;", "&Obreve;", "&obreve;",

	"&Odblac;", "&odblac;", "&OElig;", "&oelig;",
	"&Racute;", "&racute;", "&Rcedil;", "&rcedil;",
	"&Rcaron;", "&rcaron;", "&Sacute;", "&sacute;",
	"&Scirc;", "&scirc;", "&Scedil;", "&scedil;",

	"&Scaron;", "&scaron;", "&Tcedil;", "&tcedil;",
	"&Tcaron;", "&tcaron;", "&Tstrok;", "&tstrok;",
	"&Utilde;", "&utilde;", "&Umacr;", "&umacr;",
	"&Ubreve;", "&ubreve;", "&Uring;", "&uring;",

	"&Udblac;", "&udblac;", "&Uogon;", "&uogon;",
	"&Wcirc;", "&wcirc;", "&Ycirc;", "&ycirc;",
	"&Yuml;", "&Zacute;", "&zacute;", "&Zdot;",
	"&zdot;", "&Zcaron;", "&zcaron;", NULL
};

static const char *uni_latin_html_3_1[25] = {  /* 0x391 - 0x3a9	   */
	    "&Alpha;", "&Beta;", "&Gamma;",
	"&Delta;", "&Epsilon;", "&Zeta;", "&Eta;",
	"&Theta;", "&Iota;", "&Kappa;", "&Lambda;",
	"&Mu;", "&Nu;", "&Xi;", "&Omicron;",

	"&Pi;", "&Rho;", NULL, "&Sigma;",
	"&Tau;", "&Upsilon;", "&Phi;", "&Chi;",
	"&Psi;", "&Omega;"
};

static const char *uni_latin_html_3_2[65] = {  /* 0x3b1 - 0x3f1	   */
	    "&alpha;", "&beta;", "&gamma;",
	"&delta;", "&epsilon;", "&zeta;", "&eta;",
	"&theta;", "&iota;", "&kappa;", "&lambda;",
	"&mu;", "&nu;", "&xi;", "&omicron;",

	"&pi;", "&rho;", "&sigmaf;", "&sigma;",
	"&tau;", "&upsilon;", "&phi;", "&chi;",
	"&psi;", "&omega;", NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, "&thetasym;", "&upsih;", NULL,
	NULL, "&phiv;", "&piv;", NULL,
	NULL, NULL, NULL, NULL,
	"&gammad;", NULL, NULL, NULL,

	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

	"&kappav;", "&rhov;"
};

static const char *uni_latin_html_4[96] = {	/* 0x400 - 0x45f   */
	NULL, "&IOcy;", "&DJcy;", "&GJcy;",
	"&Jukcy;", "&DScy;", "&Iukcy;", NULL,
	"&Jsercy;", "&LJcy;", "&NJcy;", "&TSHcy;",
	"&KJcy;", NULL, "&Ubrcy;", "&DZcy;", 

	"&Acy;", "&Bcy;", "&Vcy;", "&Gcy;",
	"&Dcy;", "&IEcy;", "&ZHcy;", "&Zcy;",
	"&Icy;", "&Jcy;", "&Kcy;", "&Lcy;",
	"&Mcy;", "&Ncy;", "&Ocy;", "&Pcy;",

	"&Rcy;", "&Scy;", "&Tcy;", "&Ucy;",
	"&Fcy;", "&KHcy;", "&TScy;", "&CHcy;",
	"&SHcy;", "&SHCHcy;", "&HARDcy;", "&Ycy;",
	"&SOFTcy;", "&Ecy;", "&YUcy;", "&YAcy;",

	"&acy;", "&bcy;", "&vcy;", "&gcy;",
	"&dcy;", "&iecy;", "&zhcy;", "&zcy;",
	"&icy;", "&jcy;", "&kcy;", "&lcy;",
	"&mcy;", "&ncy;", "&ocy;", "&pcy;",

	"&rcy;", "&scy;", "&tcy;", "&ucy;",
	"&fcy;", "&khcy;", "&tscy;", "&chcy;",
	"&shcy;", "&shchcy;", "&hardcy;", "&ycy;",
	"&softcy;", "&ecy;", "&yucy;", "&yacy;",

	NULL, "&iocy;", "&djcy;", "&Egjcy;",
	"&jukcy;", "&dscy;", "&iukcy;", "&yicy;",
	"&jsercy;", "&ljcy;", "&njcy;", "&tshcy;",
	"&kjcy;", NULL, "&ubrcy;", "&dzcy;"
};

static const char *uni_latin_html_20[69] = {	/* 0x2000 - 0x2044   */
	NULL, NULL, "&ensp;", "&emsp;",
	"&emsp13;", "&emsp14;", NULL, "&numsp;",
	NULL, "&thinsp;", "&hairsp;", NULL,
	"&zwnj;", "&zwj;", "&lrm;", "&rlm;",

       "&dash;", NULL, NULL, "&ndash;",
       "&mdash;", "&horbar;", "&Verbar;", NULL,
       "&lsquo;", "&rsquo;", "&sbquo;", NULL,
       "&ldquo;", "&rdquo;", "&bdquo;", NULL,

       "&dagger;", "&Dagger;", "&bull;", NULL,
	NULL, "&nldr;", "&hellip;", NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

       "&permil;", NULL, "&prime;", "&Prime;",
	NULL, NULL, NULL, NULL,
	NULL, "&lsaquo;", "&rsaquo;", NULL,
	NULL, NULL, "&oline;", NULL,

	NULL, "&caret;", NULL, "&hybull;",
       "&frasl;"
};

static const char *uni_latin_html_21[76] = {  /* 0x2190 - 0x21db   */
    "&larr;", "&uarr;", "&rarr;", "&darr;",
    "&harr;", "&varr;", "&nwarr;", "&nearr;",
    "&drarr;", "&dlarr;", "&nlarr;", "&nrarr;",
    NULL, "&rarrw;", "&Larr;", NULL,

    "&Rarr;", NULL, "&larrtl;", "&rarrtl;",
    NULL, NULL, "&map;", NULL,
    NULL, "&larrhk;", "&rarrhk;", "&larrlp;",
    "&larrlp;", "&harrw;", "&nharr;", NULL,

    "&lsh;", "&rsh;", NULL, NULL,
    NULL, "&crarr;", "&cularr;", "&cularr;",
    NULL, NULL, "&cularr;", "&curarr;",
    "&olarr;", "&orarr;", "&lharu;", "&lhard;",

    "&rharu;", "&rhard;", "&dharr;", "&dharl;",
    "&rlarr2;", NULL, "&lrarr2;", "&larr2;",
    "&uarr2;", "&rarr2;", "&darr2;", "&lrhar2;",
    "&rlhar2;", "&nlArr;", "&nhArr;", "&nrArr;",

    "&lArr;", "&uArr;", "&rArr;", "&dArr;",
    "&hArr;", "&vArr;", NULL, NULL,
    NULL, NULL, "&lAarr;", "&rAarr;"
};

static const char *uni_latin_html_22[256] = {  /* 0x2200 - 0x22ff  */
    "&forall;", "&comp;", "&part;", "&exist;",
    "&nexist;", "&empty;", NULL, "&nabla;",
    "&isin;", "&notin;", "&epsis;", "&ni;",
    NULL, "&bepsi;", NULL, "&prod;",

    "&coprod;", "&sum;", "&minus;", "&mnplus;",
    "&plusdo;", NULL, "&setmn;", "&lowast;",
    "&compfn;", NULL, "&radic;", NULL, 
    NULL, "&prop;", "&infin;", "&ang90;",

    "&ang;", "&angmsd;", "&angsph;", "&mid;",
    "&nmid;", "&par;", "&npar;", "&and;",
    "&or;", "&cap;", "&cup;", "&int;", NULL, NULL, "&conint;", NULL,

    NULL, NULL, NULL, NULL, "&there4;", "&becaus;", NULL, NULL,
    NULL, NULL, NULL, NULL, "&sim;", "&bsim;", NULL, NULL,

    "&wreath;", "&nsim;", NULL, "&sime;",	/* 4x */
    "&nsime;", "&cong;", NULL, "&ncong;",
    "&asymp;", "&nap;", NULL, NULL, "&bcong;", NULL, "&bump;", "&bumpe;",

    "&esdot;", "&eDot;", "&efDot;", "&erDot;",
    "&colone;", "&ecolon;", "&ecir;", "&cire;",
    "&wedgeq;", NULL, NULL, NULL, "&trie;", NULL, NULL, NULL,

    "&ne;", "&equiv;", "&nequiv;", NULL, "&le;", "&ge;", "&lE;", "&gE;",
    "&lnE;", "&gnE;", "&Lt;", "&Gt;", "&twixt;", NULL, "&nlt;", "&ngt;",

    "&nle;", "&nge;", "&lsim;", "&gsim;", 
    NULL, NULL, "&lg;", "&gl;",
    NULL, NULL, "&pr;", "&Ssc;", "&pre;", "&sce;", "&prsim;", "&scsim;",

    "&npr;", "&nsc;", "&sub;", "&sup;",		/* 8x */
    "&nsub;", "&nsup;", "&sube;", "&supe;",
    "&nsubE;", "&nsupE;", "&subne;", "&supne;",
    NULL, NULL, "&uplus;", "&sqsub;", 

    "&sqsup;", "&sqsube;", "&sqsupe;", "&sqcap;",
    "&sqcup;", "&oplus;", "&ominus;", "&otimes;",
    "&osol;", "&odot;", "&ocir;", "&oast;",
    NULL, "&odash;", "&plusb;", "&minusb;",

    "&timesb;", "&sdotb;", "&vdash;", "&dashv;",
    "&top;", "&perp;", NULL, "&models;",
    "&vDash;", "&Vdash;", "&vdash;", NULL,
    "&nvdash;", "&nvDash;", "&nVdash;", "&nVDash;",

    NULL, NULL, "&vltri;", "&vrtri;", "&ltrie;", "&rtrie;", NULL, NULL,
    "&mumap;", NULL, "&intcal;", "&veebar;", "&barwed;", NULL, NULL, NULL,

    NULL, NULL, NULL, NULL,  /* cx */
    "&diam;", "&sdot;", "&sstarf;", "&divonx;",
    "&bowtie;", "&ltimes;", "&rtimes;", "&lthree;",
    "&rthree;", "&bsime;", "&cuvee;", "&cuwed;",

    "&Sub;", "&Sup;", "&Cap;", "&Cup;",
    "&fork;", NULL, "&ldot;", "&gsdot;",
    "&Ll;", "&Gg;", "&leg;","&gel;",
    "&els;", "&egs;", "&cuepr;","&cuesc;",

    "&npre;", "&nsce;", NULL, NULL,
    NULL, NULL, "&lnsim;", "&gnsim;",
    "&prnsim;", "&sscnsim;", "&nltri;", "&nrtri;",
    "&nltrie;", "&nrtrie;", "&vellip;", NULL,

    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL
};

static const char *uni_latin_html_23[48] = {  /* 0x2300 - 0x232f   */
    NULL, NULL, NULL, NULL, NULL, NULL, "&Barwed;", NULL,
    "&lceil;", "&rceil;", "&lfloor;", "rfloor;",
    "&drcrop;", "&dlcrop;", "&urcrop;", "&ulcrop;",

    NULL, NULL, NULL, NULL, NULL, "&telrec;", "&target;", NULL,
    NULL, NULL, NULL, NULL, "&ulcorn;", "&urcorn;", "&dlcorn;", "&drcorn;",

    NULL, NULL, "&frown;", "&smile;", NULL, NULL, NULL, NULL,
    NULL, "&lang;", "&rang;", NULL, NULL, NULL, NULL, NULL
};

static const char *uni_latin_html_25[208] = {  /* 0x2500 - 0x25cf   */
    "&boxh;", NULL, "&boxv;", NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, "&boxdr;", NULL, NULL, NULL, 

    "&boxdl;", NULL, NULL, NULL, "&boxur;", NULL, NULL, NULL, 
    "&boxul;", NULL, NULL, NULL, "&boxvr;", NULL, NULL, NULL, 

    NULL, NULL, NULL, NULL, "&boxvl;", NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, "&boxhd;", NULL, NULL, NULL, 

    NULL, NULL, NULL, NULL, "&boxhu;", NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, "&boxvh;", NULL, NULL, NULL, 

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

    "&boxH;", "&boxV;", "&boxdR;", "&boxDr;", 
    "&boxDR;", "&boxdL;", "&boxDl;", "&boxDL;", 
    "&boxuR;", "&boxUr;", "&boxUR;", "&boxuL;", 
    "&boxUl;", "&boxUL;", "&boxvR;", "&boxVr;", 

    "&boxVR;", "&boxvL;", "&boxVl;", "&boxVL;", 
    "&boxHd;", "&boxhD;", "&boxHD;", "&boxHu;", 
    "&boxhU;", "&boxHU;", "&boxvH;", "&boxVh;", 
    "&boxVH;", NULL, NULL, NULL, 

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

    "&uhblk;", NULL, NULL, NULL, "&lhblk;", NULL, NULL, NULL, /* 8x */
    "&block;", NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

    NULL, "&blk14;", "&blk12;", "&blk34;", NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

    NULL, "&square;", NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, "&squf;", NULL, NULL, "&rect;", "&marker;", NULL, 

    NULL, NULL, NULL, "&xutri;", "&utrif;", "&utri;", NULL, NULL, 
    "&rtrif;", "&rtri;", NULL, NULL, NULL, "&xdtri;", "&dtrif;", "&dtri;", 

    NULL, NULL, "&ltrif;", "&ltri;", NULL, NULL, NULL, NULL, 
    NULL, NULL, "&loz;", "&xcirc;", NULL, NULL, NULL, NULL
};

static const char *uni_latin_html_26[16] = {  /* 0x2660 - 0x266f   */
    "&spades;", "&hearts;", "&diams;", "&clubs;",
    "&spades;", "&hearts;", "&diams;", "&clubs;",
    NULL, NULL, "&sung;", NULL,
    NULL, "&flat;", "&natur;", "&sharp;"
};

int latin2html(c1)
skf_ucode c1;
{
    skf_ucode c2,c3;
    const char *sym = NULL;

#ifdef SKFDEBUG
    if (is_vv_debug)
		fprintf(stderr," latin2html: %06lx",(unsigned long)c1);
#endif
    c2 = (c1 >> 8) & 0xff; c3 = (c1 & 0xff);
    if (((c1 < A_DEL) && (!use_htmlsanitize ||
		(uni_latin_html_0[c1 - 0x20] == NULL))) 
		|| (use_latin2htmld)) {
	SKFSTROUT("&#");
	printf("%03d;",c1); 	/* output 3 digit at least	   */
	return(TRUE);
    } else if (use_latin2htmlh) {
	SKFSTROUT("&#x");
	printf("%03x;",c1); 	/* output 3 digit at least	   */
	return(TRUE);
    } else if (use_latin2htmlu) {
	utf8_uriout(c1);
	return(TRUE);
    } else if (c2 == 0) {
	if (c3 < A_DEL) sym = uni_latin_html_0[c3 - 0x20];
	else sym = uni_latin_html_1[c3 - 0xa0];
    } else if (c2 == 0x01) {
	switch (c3) {
	    case 0x92: sym ="&fnof;"; break;
	    case 0xf5: sym ="&gacute;"; break;
	    default: 
		if (c3 <= 0x7e) sym = uni_latin_html_2[c3];
		break;
	};
    } else if (c2 == 0x02) {
	switch (c3) {
#if 0
	    case 0x26: sym ="&Adot;"; break;
	    case 0x27: sym ="&adot;"; break;
	    case 0x2e: sym ="&Odot;"; break;
	    case 0x2f: sym ="&odot;"; break;
	    case 0x32: sym ="&Ymacr;"; break;
	    case 0x33: sym ="&ymacr;"; break;
#endif
	    case 0xbc: sym ="&apos;"; break;
	    case 0xc6: sym ="&circ;"; break;
	    case 0xc7: sym ="&caron;"; break;
	    case 0xd8: sym ="&breve;"; break;
	    case 0xd9: sym ="&dot;"; break;
	    case 0xda: sym ="&ring;"; break;
	    case 0xdb: sym ="&ogon;"; break;
	    case 0xdc: sym ="&tilde;"; break;
	    case 0xdd: sym ="&dblac;"; break;
	    default: break;
	};
    } else if (c2 == 0x03) {	/* Greek alphabet		  */
	if ((c3 >= 0x91) && (c3 <= 0xa9)) {
	    sym = uni_latin_html_3_1[c3 - 0x91];
	} else if ((c3 >= 0xb1) && (c3 <= 0xf1)) {
	    sym = uni_latin_html_3_2[c3 - 0xb1];
	} else ;
    } else if (c2 == 0x04) {	/* Cyrillic alphabet		  */
	if ((c3 >= 0x01) && (c3 <= 0x5f)) {
	    sym = uni_latin_html_4[c3];
	} else ;
    } else if (c2 == 0x20) {
	if (c3 <= 0x44) {
	    sym = uni_latin_html_20[c3];
	} else switch (c3) {
	    case 0xac: sym ="&euro;"; break;
	    case 0xdb: sym ="&tdot;"; break;
	    case 0xdc: sym ="&DotDot;"; break;
	    default: break;
	};
    } else if (c2 == 0x21) {
	if ((c3 >= 0x90) && (c3 <= 0xdb)) {
	    sym = uni_latin_html_21[c3 - 0x90];
	} else switch (c3) {
	    case 0x18: sym ="&weierp;"; break;
	    case 0x11: sym ="&image;"; break;
	    case 0x1c: sym ="&real;"; break;
	    case 0x22: sym ="&trade;"; break;
	    case 0x35: sym ="&alefsym;"; break;
	    default: break;
	};
    } else if (c2 == 0x22) {
	sym = uni_latin_html_22[c3];
    } else if (c2 == 0x23) {
	if (c3 <= 0x2f) {
	    sym = uni_latin_html_23[c3];
	} else ;
    } else if (c3 == 0x25) {
	if (c3 <= 0xcf) {
	    sym = uni_latin_html_25[c3];
	} else ;
    } else if (c2 == 0x26) {
	if ((c3 >= 0x60) && (c3 <= 0x6f)) {
	    sym = uni_latin_html_26[c3 - 0x60];
	} else;
    };

    if (sym != NULL) {
	SKFSTROUT(sym);
    } else {
	SKFSTROUT("&#");
	printf("%03d;",c1); 	/* output 3 digit at least	   */
    };

    return(TRUE);
}

/* --------------------------------------------------------------- */
/* latin2tex: iso8859 non-ascii to latex quoted expression	   */
/*  Note: conversion assumed text is in paragraph mode.		   */
/* --------------------------------------------------------------- */
static const char *uni_latin_tex_0[96] = {	/* 0xa0 - 0xaf	   */
     " ", "!`", "\\cents", "\\pounds",
     NULL, "\\yens", NULL, "\\S",
     NULL, "\\copyright", "\\[^{\\underline{a}}\\]", "\\[\\not\\]",
     "-", NULL, NULL, NULL,

     "\\[^{o}\\]", "\\[\\pm\\]", "\\[^{2}\\]", "\\[^{3}\\]",
     NULL, "\\[\\mu\\]", "\\P", NULL,
     NULL, "\\[^{1}\\]", "\\[^{\\underline{o}}\\]", NULL,
     "\\[\\frac{1}{4}\\]", "\\[\\frac{1}{2}\\]", "\\[\\frac{3}{4}\\]", "?`",

     "\\`{A}", "\\\'{A}", "\\^{A}", "\\~{A}",
     "\\\"{A}", "\\AA", "\\AE", "\\c{C}",
     "\\`{E}", "\\'{E}", "\\^{E}", "\\\"{E}",
     "\\`{I}", "\\'{I}", "\\^{I}", "\\\"{I}",

     NULL, "\\~{N}", "\\`{O}", "\\\'{O}",
     "\\^{O}", "\\~{O}", "\\\"{O}", "\\[\\times\\]",
     "\\O", "\\`{U}", "\\\'{U}", "\\^{U}",
     "\\\"{U}", "\\`{Y}", NULL, "\\ss",

     "\\`{a}", "\\\'{a}", "\\^{a}", "\\~{a}",
     "\\\"{a}", "\\aa", "\\ae", "\\c{c}",
     "\\`{e}", "\\'{e}", "\\^{e}", "\\\"{e}",
     "\\`{\\i}", "\\'{\\i}", "\\^{\\i}", "\\\"{\\i}",


    NULL, "\\~{n}", "\\`{o}", "\\\'{o}",
     "\\^{o}", "\\~{o}", "\\\"{o}", "\\[\\div\\]",
     "\\o", "\\`{u}", "\\\'{u}", "\\^{u}",
     "\\\"{u}", "\\`{y}", NULL, "\\\"{y}"
};

static const char *uni_latin_tex_1_1[128] = {  /* 100 - 17f	   */
    "\\={A}", "\\={a}", "\\u{A}", "\\u{a}",
    "\\c{A}", "\\c{a}", "\\\'{C}", "\\\'{c}",
    "\\^{C}", "\\^{c}", "\\.{C}", "\\.{c}",
    "\\v{C}", "\\v{c}", "\\v{D}", "\\v{d}",

    NULL, NULL, "\\={E}", "\\={e}",
    "\\c{E}", "\\c{e}", "\\.{E}", "\\.{e}",
    "\\c{E}", "\\c{e}", "\\v{E}", "\\v{e}",
    "\\^{G}", "\\^{g}", "\\c{G}", "\\c{g}",

    "\\.{G}", "\\.{g}", "\\c{G}", "\\c{g}",
    "\\^{H}", "\\^{h}", NULL, "\\[\\hbar\\[",
    "\\~{I}", "\\~{\\i}", "\\={I}", "\\={\\i}",
    "\\c{I}", "\\c{\\i}", "\\c{I}", "\\c{i}",

    "\\.{I}", "\\i", NULL, NULL,
    "\\^{J}", "\\^{\\j}", "\\c{K}", "\\c{k}",
    NULL, "\\\'{L}", "\\\'{l}", "\\c{L}",
    "\\c{l}", "\\v{L}", "\\v{l}", NULL,

    NULL, "\\L", "\\l", "\\\'{N}",
    "\\\'{n}", "\\c{N}", "\\c{n}", "\\v{N}",
    "\\v{n}", NULL, NULL, NULL,
    "\\={O}", "\\={o}", "\\u{O}", "\\u{o}",

    "\\H{O}", "\\H{o}", "\\OE",	"\\oe",
    "\\\'{R}", "\\\'{r}", "\\c{R}", "\\c{r}",
    "\\v{R}", "\\v{r}", "\\\'{S}", "\\\'{s}",
    "\\^{S}", "\\^{s}", "\\c{S}", "\\c{s}",

    "\\v{S}", "\\v{s}", "\\c{T}", "\\c{t}",
    "\\v{T}", "\\v{t}", NULL, NULL,
    "\\~{U}", "\\~{u}", "\\={U}", "\\={u}",
    "\\u{U}", "\\u{u}", "\\a{U}", "\\a{u}",

    "\\H{U}", "\\H{u}", "\\c{U}", "\\c{u}",
    "\\^{W}", "\\^{w}", "\\^{Y}", "\\^{y}",
    "\\\"{Y}", "\\\'{Z}", "\\\'{z}", "\\.{Z}",
    "\\.{z}", "\\v{Z}", "\\v{z}", NULL
};

static const char *uni_latin_tex_1_2[64] = {  /* 1c0 - 1ff	   */
    NULL,NULL,NULL, "!",
    "D\\v{Z}", "D\\v{z}", "d\\v{z}", "LJ",
    "Lj", "lj", "NJ", "Nj",
    "nj", "\\v{A}", "\\v{a}", "\\v{I}", 

    "\\v{\\i}", "\\v{O}", "\\v{o}", "\\v{U}",
    "\\v{u}", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,

    NULL, NULL, "\\={\\AE}", "\\={\\ae}",
    NULL, NULL, "\\v{G}", "\\v{g}",
    "\\v{K}", "\\v{k}", "\\c{O}", "\\c{o}",
    NULL, NULL, NULL, NULL,

    "\\v{\\j}", "DZ", "Dz", "dz",
    "\\'{G}", "\\'{g}", NULL, NULL,
    "\\`{N}", "\\`{n}", NULL, NULL,
    "\\'{\\AE}", "\\'{\\ae}", "\\'{\\O}", "\\'{\\o}"
};

static const char *uni_latin_tex_2[52] = {  /* 200 - 233	   */
    NULL, NULL, "\\t{A}", "\\t{a}",
    NULL, NULL, "\\t{E}", "\\t{e}",
    NULL, NULL, "\\t{I}", "\\t{\\i}",
    NULL, NULL, "\\t{O}", "\\t{o}",

    NULL, NULL, "\\t{R}", "\\t{r}",
    NULL, NULL, "\\t{U}", "\\t{u}",
    NULL, NULL, NULL, NULL,
    NULL, NULL, "\\v{H}", "\\v{h}",

    NULL, NULL, NULL, NULL,
    NULL, NULL, "\\.{A}", "\\.{a}",
    "\\c{E}", "\\c{e}", NULL, NULL,
    NULL, NULL, "\\.{O}", "\\.{o}",

    NULL, NULL, "\\={Y}", "\\={y}"
};

static const char *uni_latin_tex_3[68] = {  /* 393 - 3d6	   */
    "\\[\\Gamma\\]",
    "\\[\\Delta\\]", NULL, NULL, NULL,
    "\\[\\Theta\\]", NULL, NULL, "\\[\\Lambda\\]",
    NULL, NULL, "\\[\\Xi\\]", NULL,

    "\\[\\Pi\\]", NULL, NULL, "\\[\\Sigma\\]",
    NULL, "\\[\\Upsilon\\]", "\\[\\Phi\\]", NULL,
    "\\[\\Psi\\]", "\\[\\Omega\\]", NULL, NULL,
    NULL, NULL, NULL, NULL,

    NULL, "\\[\\alpha\\]", "\\[\\beta\\]", "\\[\\gamma\\]",
    "\\[\\delta\\]", "\\[\\epsilon\\]", "\\[\\zeta\\]", "\\[\\eta\\]",
    "\\[\\theta\\]", "\\[\\iota\\]", "\\[\\kappa\\]", "\\[\\lambda\\]",
    "\\[\\mu\\]", "\\[\\nu\\]", "\\[\\xi\\]", "o",

    "\\[\\pi\\]", "\\[\\rho\\]", "\\[\\varsigma\\]", "\\[\\sigma\\]",
    "\\[\\tau\\]", "\\[\\upsilon\\]", "\\[\\phi\\]", "\\[\\chi\\]",
    "\\[\\psi\\]", "\\[\\omega\\]", NULL, NULL, 
    NULL, NULL, NULL, NULL,

    NULL, "\\[\\vartheta\\]", NULL, NULL,
    NULL, "\\[\\varphi\\]", "\\[\\varpi\\]"
};

static const char *uni_latin_tex_20[16] = {  /* 2070 - 207f	   */
 "\\[^{0}\\]", NULL, NULL, NULL,
 "\\[^{4}\\]", "\\[^{5}\\]", "\\[^{6}\\]", "\\[^{7}\\]",
 "\\[^{8}\\]", "\\[^{9}\\]", "\\[^{+}\\]", "\\[^{-}\\]",
 "\\[^{=}\\]", "\\[^{{}\\]", "\\[^{)}\\]", "\\[^{n}\\]"
};

static const char *uni_latin_tex_21[80] = {  /* 2190 - 21df 	   */
 "\\[\\leftarrow\\]", "\\[\\uparrow\\]", "\\[\\rightarrow\\]",
 "\\[\\downarrow\\]", "\\[\\leftrightarrow\\]", "\\[\\updownarrow\\]",
 "\\[\\nwarrow\\]", "\\[\\nearrow\\]", "\\[\\searrow\\]",
 "\\[\\swarrow\\]", "\\[\\not\\leftarrow\\]", "\\[\\not\\rightarrow\\]",
 NULL, "\\[\\leadsto\\]", NULL, NULL,

 NULL, NULL, NULL, NULL,
 NULL, NULL, "\\[\\mapsto\\]", NULL,
 NULL, "\\[\\hookleftarrow\\]", "\\[\\hookrightarrow\\]", NULL,
 NULL, NULL, "\\[\\not\\leftrightarrow\\]", NULL,

 NULL, NULL, NULL, NULL,
 NULL, NULL, NULL, NULL,
 NULL, NULL, NULL, NULL,
 "\\[\\leftharpoonup\\]", "\\[\\leftharpoondown\\]", NULL, NULL,

 "\\[\\rightharpoonup\\]", "\\[\\rightharpoondown\\]", NULL, NULL,
 NULL, NULL, NULL, NULL,
 NULL, NULL, NULL, NULL,
 "\\[\\rightleftharpoons\\]", "\\[\\not\\Leftarrow\\]",
 "\\[\\not\\Leftrightarrow\\]", "\\[\\not\\Rightarrow\\]",

 "\\[\\Leftarrow\\]", "\\[\\Uparrow\\]", "\\[\\Rightarrow\\]",
 "\\[\\Downarrow\\]", "\\[\\Leftrightarrow\\]", NULL, NULL, NULL,
 NULL, NULL, NULL, NULL,
 NULL, NULL, NULL, NULL
};

static const char *uni_latin_tex_22[242] = {  /* 2200 - 22f1 	   */
    "\\[\\forall\\]", NULL, "\\[\\partial\\]", "\\[\\exists\\]",
    "\\[\\not\\exists\\]", "\\[\\emptyset\\]", "\\[\\delta\\]",
    "\\[\\nabla\\]",
    "\\[\\in\\]", "\\[\\not\\in\\]", "\\[\\epsilon\\]", "\\[\\ni\\]",
    "\\[\\not\\ni\\]", NULL, NULL, "\\[\\Pi\\]",

    "\\[\\amalg\\]", "\\[\\Sigma\\]", "-", "\\[\\mp\\]",
    "\\.{+}", "\\[/\\]", "\\[\\backslash\\]", "\\[\\ast\\]",
    "\\[\\circ\\]", "\\[\\bullet\\]", "\\[\\surd\\]", NULL,
    NULL, "\\[\\infto\\]", "\\[\\infty\\]", NULL,

    "\\[\\angle\\]", NULL, NULL, NULL,
    NULL, "\\[\\parallel\\]", "\\[\\not\\parallel\\]", "\\[\\vee\\]",
    "\\[\\wedge\\]", "\\[\\cap\\]", "\\[\\cup\\]", "\\[\\int\\]",
    "\\[\\int\\int\\]", "\\[\\int\\int\\int\\]", "\\[\\oint\\]",
    "\\[\\oint\\oint\\]",

    "\\[\\oint\\oint\\oint\\]", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    "\\[\\sim\\]", NULL, NULL, NULL,

    "\\[\\wr\\]", NULL, NULL, "\\[\\simeq\\]", "\\[\\not\\simeq\\]",
    "\\[\\cong\\]", "\\[\\not\\cong\\]", "\\[\\not\\cong\\]",
    "\\[\\approx\\]", "\\[\\not\\approx\\]", NULL, NULL,
    NULL, "\\[\\asymp\\]", NULL, NULL,

    "\\[\\doteq\\]", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,

    "\\[\\neq\\]", "\\[\\equiv\\]", "\\[\\not\\equiv\\]", NULL,
    "\\[\\leq\\]", "\\[\\geq\\]", "\\[\\leq\\]", "\\[\\geq\\]",
    NULL, NULL, "\\[\\ll\\]", "\\[\\gg\\]",
    NULL, NULL, "\\[\\not<\\]", "\\[\\not>\\]",

    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, "\\[\\prec\\]", "\\[\\succ\\]",
    "\\[\\preceq\\]", "\\[\\succeq\\]", NULL, NULL,

    "\\[\\not\\prec\\]", "\\[\\not\\succ\\]", "\\[\\subset\\]",
    "\\[\\supset\\]", "\\[\\not\\subset\\]", "\\[\\not\\supset\\]",
    "\\[\\subseteq\\]", "\\[\\supseteq\\]",
    "\\[\\not\\subseteq\\]", "\\[\\not\\supseteq\\]", NULL, NULL,
    NULL, "\\[\\udot\\]", "\\[\\uplus\\]", "\\[\\sqsubset\\]",

    "\\[\\sqsupset\\]", "\\[\\sqsubseteq\\]", "\\[\\sqsupseteq\\]",
    "\\[\\sqcap\\]",
    "\\[\\sqcup\\]", "\\[\\oplus\\]", "\\[\\ominus\\]", "\\[\\otimes\\]",
    "\\[\\oslash\\]", "\\[\\odot\\]", NULL, NULL,
    NULL, NULL, NULL, NULL,

    NULL, NULL, "\\[\\vdash\\]", "\\[\\dashv\\]",
    "\\[\\top\\]", "\\[\\perp\\]", NULL, NULL,
    "\\[\\models\\]", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,

    NULL, NULL, "\\[\\lhd\\]", "\\[\\rhd\\]",
    "\\[\\unlhd\\]", "\\[\\unrhd\\]", NULL, NULL,
    NULL, NULL, "\\[\\top\\]", NULL,
    NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL,
    "\\[\\diamond\\]", "\\[\\cdot\\]", "\\[\\star\\]", NULL,
    "\\[\\bowtie\\]", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, "\\[\\not\\lhd\\]", "\\[\\not\\rhd\\]",
    "\\[\\not\\unlhd\\]", "\\[\\not\\unrhd\\]", "\\[\\vdots\\]",
    "\\[\\cdots\\]",

    "\\[\\udots\\]", "\\[\\ddots\\]"
};

static const char *uni_latin_tex_26[16] = {  /* 0x2660 - 0x266f	   */
    "\\[\\spadesuit\\]", "\\[\\heartsuit\\]",
    "\\[\\diamondsuit\\]", "\\[\\clubsuit\\]",
    "\\[\\spadesuit\\]", "\\[\\heartsuit\\]",
    "\\[\\diamondsuit\\]", "\\[\\clubsuit\\]",
    NULL, NULL, NULL, NULL,
    NULL, "\\[\\flat\\]", "\\[\\natural\\]", "\\[\\sharp\\]"
};

/*@-branchstate@*/
int latin2tex(c1)
skf_ucode c1;
{
    skf_ucode c2,c3;
    int	out_enbl = TRUE;
    const char *sym = NULL;

#ifdef SKFDEBUG
	if (is_vv_debug)
		fprintf(stderr," latin2tex: %04x",c1);
#endif
    c2 = (c1 >> 8) & 0xff; c3 = (c1 & 0xff);
    if (c2 == 0) {
	if ((c3 == 0x5c) && !use_htmlsanitize) {
	    sym = "\\\\";
	} else if (c3 < A_DEL) { 
	    post_oconv(c3); out_enbl = TRUE;
	} else if ((c3 >= 0xa0) && (c3 <= 0xff)) {
	    sym = uni_latin_tex_0[c3 - 0xa0];
	} else {
	    out_enbl = FALSE; 
	};
    } else if (c2 == 0x01) {
/* This section regards ogonek as cedilla, but it is doubtful. */
	if (c3 <= 0x7f) {
	    sym = uni_latin_tex_1_1[c3];
	} else if (c3 == 0xb1) {
	    sym = "\\[\\mho\\]";
	} else if (c3 >= 0xc0) {
	    sym = uni_latin_tex_1_2[c3 - 0xc0];
	} else ;
	if (sym == NULL) out_enbl = FALSE; 
    } else if (c2 == 0x02) {
	if (c3 <= 0x33) {
	    sym = uni_latin_tex_2[c3];
	} else ;
	if (sym == NULL) out_enbl = FALSE; 
    } else if (c2 == 0x03) {
	if ((c3 >= 0x93) && (c3 <= 0xd6)) {
	    sym = uni_latin_tex_3[c3 - 0x93];
	} else switch (c3) {
	    case 0xf4: sym = "\\[\\Theta\\]"; break; /* Symbol. 3.1 */
	    case 0xf5: sym = "\\[\\varepsilon\\]"; break; /* 3.1 */
	    default: out_enbl = FALSE; break;
	};
	if (sym == NULL) out_enbl = FALSE; 
    } else if (c2 == 0x20) {
	if ((c3 >= 0x70) && (c3 <= 0x7f)) {
	    sym = uni_latin_tex_20[c3 - 0x70];
	} else switch (c3) {
	    case 0x16: sym = "\\[\\|\\]"; break;
	    case 0x20: sym = "\\dag"; break;
	    case 0x21: sym = "\\ddag"; break;
	    case 0x22: sym = "\\[\\bullet\\]"; break;
	    case 0x24: sym = "\\[\\cdot\\]"; break;
	    case 0x32: sym = "\\[\\prime\\]"; break;

	    default: out_enbl = FALSE; break;
	};
	if (sym == NULL) out_enbl = FALSE; 
    } else if (c2 == 0x21) {
	if ((c3 >= 0x90) && (c3 <= 0xdf)) {
	    sym = uni_latin_tex_21[c3 - 0x90];
	} else switch (c3) {
	    case 0x11: sym = "\\[\\Im\\]"; break;
	    case 0x18: sym = "\\[\\wp\\]"; break;
	    case 0x1f: sym = "\\[\\hbar\\]"; break;
	    case 0x1c: sym = "\\[\\Re\\]"; break;
	    case 0x22: sym = "\\[\\^{TM}\\]"; break;
	    case 0x26: sym = "\\[\\mho\\]"; break;
	    case 0x35: sym = "\\[\\aleph\\]"; break;
	    default: out_enbl = FALSE; break;
	};
	if (sym == NULL) out_enbl = FALSE; 
    } else if (c2 == 0x22) {
	if (c3 <= 0xf1) {
	    sym = uni_latin_tex_22[c3];
	} else ;
	if (sym == NULL) out_enbl = FALSE; 
    } else if (c2 == 0x23) {
	switch (c3) {
	    case 0x07: sym = "\\[\\wr\\]"; break;
	    case 0x08: sym = "\\[\\lceil\\]"; break;
	    case 0x09: sym = "\\[\\rceil\\]"; break;
	    case 0x0a: sym = "\\[\\lfloor\\]"; break;
	    case 0x0b: sym = "\\[\\rfloor\\]"; break;
	    case 0x28: sym = "\\[\\langle\\]"; break;
	    case 0x29: sym = "\\[\\rangle\\]"; break;
	    default: break;
	};
	if (sym == NULL) out_enbl = FALSE; 
    } else if (c2 == 0x25) {
	switch (c3) {
	    case 0xa1: sym = "\\[\\Box\\]"; break;
	    case 0xb3: sym = "\\[\\bigtriangleup\\]"; break;
	    case 0xb5: sym = "\\[\\triangleup\\]"; break;
	    case 0xbd: sym = "\\[\\bigtriangledown\\]"; break;
	    case 0xbf: sym = "\\[\\triangledown\\]"; break;
	    case 0xb9: sym = "\\[\\triangleright\\]"; break;
	    case 0xc3: sym = "\\[\\triangleleft\\]"; break;
	    case 0xc7: sym = "\\[\\diamond\\]"; break;
	    case 0xcb: sym = "\\[\\bigcirc\\]"; break;
	    case 0xe6: sym = "\\[\\circ\\]"; break;
	    default: out_enbl = FALSE; break;
	};
	if (sym == NULL) out_enbl = FALSE; 
    } else if (c2 == 0x26) {
	if ((c3 >= 0x60) && (c3 <= 0x6f)) {
	    sym = uni_latin_tex_26[c3 - 0x60];
	} else ;
	if (sym == NULL) out_enbl = FALSE; 
    } else {
	    out_enbl = FALSE; 
    };
    if (sym != NULL) SKFSTROUT(sym);
    return(out_enbl);
}
/*@+branchstate@*/

/* --------------------------------------------------------------- */
static const unsigned short viqr_map[256] = {
  0x0000,0x0001,0x2141,0x0003, 0x0004,0x3141,0x3241,0x0007,
  0x0008,0x0009,0x000a,0x000b, 0x000c,0x000d,0x000e,0x000f,
  0x0010,0x0011,0x0012,0x0013, 0x2059,0x0015,0x0016,0x0017,
  0x0018,0x3059,0x001a,0x001b, 0x001c,0x001d,0x5059,0x001f,
  0x0020,0x0021,0x0022,0x0023, 0x0024,0x0025,0x0026,0x0027,
  0x0028,0x0029,0x002a,0x002b, 0x002c,0x002d,0x002e,0x002f,
  0x0030,0x0031,0x0032,0x0033, 0x0034,0x0035,0x0036,0x0037,
  0x0038,0x0039,0x003a,0x003b, 0x003c,0x003d,0x003e,0x003f,

  0x0040,0x0041,0x0042,0x0043, 0x0044,0x0045,0x0046,0x0047,
  0x0048,0x0049,0x004a,0x004b, 0x004c,0x004d,0x004e,0x004f,
  0x0050,0x0051,0x0052,0x0053, 0x0054,0x0055,0x0056,0x0057,
  0x0058,0x0059,0x005a,0x005b, 0x005c,0x005d,0x005e,0x005f,
  0x0060,0x0061,0x0062,0x0063, 0x0064,0x0065,0x0066,0x0067,
  0x0068,0x0069,0x006a,0x006b, 0x006c,0x006d,0x006e,0x006f,
  0x0070,0x0071,0x0072,0x0073, 0x0074,0x0075,0x0076,0x0077,
  0x0078,0x0079,0x007a,0x007b, 0x007c,0x007d,0x007e,0x007f,

  0x5041,0x4141,0x1141,0x5141, 0x4241,0x1241,0x2241,0x5241,
  0x3045,0x5045,0x4245,0x1245, 0x2245,0x3245,0x5245,0x424f,
  0x124f,0x224f,0x324f,0x524f, 0x534f,0x434f,0x134f,0x234f,
  0x5049,0x204f,0x504f,0x2049, 0x2055,0x3055,0x5055,0x1059,
  0x304f,0x4161,0x1161,0x5161, 0x4261,0x1261,0x2261,0x5261,
  0x3065,0x5065,0x4265,0x1265, 0x2265,0x3265,0x5265,0x426f,
  0x126f,0x226f,0x326f,0x334f, 0x034f,0x526f,0x136f,0x236f,
  0x5069,0x5355,0x4355,0x1355, 0x2355,0x036f,0x436f,0x0355,

  0x1041,0x4041,0x0241,0x3041, 0x2041,0x0141,0x2161,0x3161,
  0x1045,0x4045,0x0245,0x2045, 0x1049,0x4049,0x3049,0x1079,
  0x0444,0x4375,0x104f,0x404f, 0x024f,0x5061,0x2079,0x1375,
  0x2375,0x1055,0x4055,0x3079, 0x5079,0x4059,0x336f,0x0375,
  0x1061,0x4061,0x0261,0x3061, 0x2061,0x0161,0x3375,0x3261,
  0x1065,0x4065,0x0265,0x2065, 0x1069,0x4069,0x3069,0x2069,
  0x0564,0x5375,0x106f,0x406f, 0x026f,0x306f,0x206f,0x506f,
  0x5075,0x1075,0x4075,0x3075, 0x2075,0x4079,0x536f,0x3355
};

static const int viqr_base[] = {'(','^','+','D','d'};
static const int viqr_tone[] = {0x60,'?','~',0x27,'.'};
static const int vinm_base[] = {'<','>','*','D','d'};
static const int vinm_tone[] = {'!','?',0x22,0x27,'.'};

void viqr_convert(x)
skf_ucode x;
{
    int vqb,vqt;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," - viqr_convert: %x ",(x & 0xff));
#endif

    vqb = (viqr_map[x & 0xff] >> 8) & 0x0f;
    vqt = (viqr_map[x & 0xff] >> 12) & 0x0f;

    SKFputc((viqr_map[x & 0xff]) & 0x7f);
    if (vqb > 0) {
	if (is_viqr(conv_cap)) {
	    SKFputc(viqr_base[vqb - 1]);
	} else {
	    SKFputc(vinm_base[vqb - 1]); };
    };
    if (vqt > 0) { 
	if (is_viqr(conv_cap)) {
	    SKFputc(viqr_tone[vqt - 1]);
	} else {
	    SKFputc(vinm_tone[vqt - 1]); };
    };
}

int viqr_convert_count(x)
skf_ucode x;
{
    int vqb,vqt;
    int val = 2;

    vqb = (viqr_map[x & 0xff] >> 8) & 0x0f;
    vqt = (viqr_map[x & 0xff] >> 12) & 0x0f;
    if (vqb > 0) val += 2;
    if (vqt > 0) val += 2;

    return(val);
}
/* --------------------------------------------------------------- */

