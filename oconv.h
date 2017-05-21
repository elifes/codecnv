/* *******************************************************************
** Copyright (c) 1998-2015 Seiji Kaneko. All rights reserved.
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
    oconv.h:	Code conversion table related definitions
    $Id: oconv.h,v 1.113 2017/01/05 15:05:48 seiji Exp seiji $
**/
#ifndef	OCONV_H_DEFINED
#define OCONV_H_DEFINED

#define	U_GETA1		0x30	/* geta code */
#define	U_GETA2		0x13

#define ETENCHAR	0x42	/* ETEN pseudo-iso-2022 char	   */
#define HKUCHAR		0x47	/* HKU pseudo-iso-2022 char	   */
#define CP950CH		'3'	/* cp950 pseudo-iso-2022 char	   */
#define X213NEW		'Q'	/* JIS X-0213(2004) plane 1	   */
				/* Note: plane 2 is not updated	   */

#if 0
extern char	ascii_mask[];		/* ascii treatment mask	   */
#endif
extern int	fold_count;

extern int	o_encode_stat;

extern skf_ucode pref_subst_char;

#define ENCODE_LEN_LIMIT 76	/* MIME encode line length limit   */
				/* cf. rfc2045			   */

#define OOBUFSIZE	256

/* --------------------------------------------------------------- */
/* output side table for kana region and kanji region		   */
/*  note: 2e80-2fff is unsupported (shouldn't appear in text)	   */
/* --------------------------------------------------------------- */
#define ASCII_TABLE_LEN	256		/* 0000 - 00ff */
#define LATIN_TABLE_LEN	8032		/* 00a0 - 1fff */
#define GLYPH_TABLE_LEN	4096		/* 2000 - 2fff */
#define KANA_TABLE_LEN	1024		/* 3000 - 33ff */
#define CJK_A_TABLE_LEN	6656		/* 3400 - 4dff */
#define CJK_TABLE_LEN	20992		/* 4e00 - 9fff */
#define Y_TABLE_LEN	3072		/* a000 - abff */
#define HNGL_TABLE_LEN	11264		/* ac00 - d7ff */
#define CPT_TABLE_LEN	1792		/* f900 - ffff */
#define CJK_B_TABLE_LEN	49152		/* 20000 - 2bfff */
#define CJK_C_TABLE_LEN 768		/* 2f800 - 2faff */
#define PRV_TABLE_LEN	6400		/* e000 - f8ff */
#define HIST_TABLE_LEN	16384		/* 10000 - 13fff */
#define UPMISC_TABLE_LEN 8192		/* 16000 - 17fff */
#define UPKANA_TABLE_LEN 4096		/* 1b000 - 1bfff */
#define NOTE_TABLE_LEN	12288		/* 1d000 - 1ffff */

#define ASCII_TBL_START 0x00000000
#define LATIN_TBL_START 0x000000a0
#define GLYPH_TBL_START 0x00002000
#define KANA_TBL_START	0x00003000
#define CJK_A_TBL_START	0x00003400
#define CJK_TBL_START	0x00004e00
#define Y_TBL_START	0x0000a000
#define HNGL_TBL_START	0x0000ac00
#define HNGL_TBL_END	0x0000d7ff
#define PRV_TBL_START	0x0000e000
#define CPT_TBL_START	0x0000f900
#define CPT_TBL_END	0x0000ffff
#define HIST_TBL_START	0x00010000
#define HIST_TBL_END	0x00013fff
#define UPMISC_TBL_START	0x00016000 /* Bamum suppliment	  */
#define UPMISC_TBL_END	0x00017fff
#define UPKANA_TBL_START	0x0001b000 /* kana suppliment	  */
#define UPKANA_TBL_END	0x0001bfff
#define NOTE_TBL_START	0x0001d000
#define NOTE_TBL_END	0x0001ffff
#define FITZPAT_TBL_START	0x0001f3fb
#define FITZPAT_TBL_END	0x0001f3ff
#define CJK_B_TBL_START	0x00020000	/* CJK UI Extension B/C/D */
#define CJK_B_TBL_END	0x0002bfff
#define CJK_C_TBL_START	0x0002f800	/* CJK UI compat suppl	  */
#define CJK_C_TBL_END	0x0002faff
#define TAG_TBL_START	0x000e0000
#define TAG_TBL_END	0x000e007f
#define SUP_PRIV_START	0x000f0000
#define SUP_PRIV_END	0x0010ffff
#define VARSEL_SUP_TBL_START 0x000e0100
#define VARSEL_SUP_TBL_END 0x000e01ef
#define UNI_HIGH_LIMIT	0x0010ffff

/* otbl_reg_defs index order is related this bit order */
#define		OTBL_KANA	0x4000UL
#define		OTBL_KANJI	0x2000UL
#define		OTBL_COMPAT	0x1000UL
#define		OTBL_LATIN	0x0800UL
#define		OTBL_GLYPHS	0x0400UL
#define		OTBL_HNGL	0x0200UL
#define		OTBL_CJK_A	0x0100UL
#define		OTBL_CJK_B	0x0080UL
#define		OTBL_Y		0x0040UL
#define		OTBL_CJK_CC	0x0020UL
#define		OTBL_PRV	0x0010UL
#define		OTBL_ASCII	0x0008UL
#define		OTBL_HIST	0x0004UL
#define		OTBL_NOTE	0x0002UL
#define		OTBL_UPMISC	0x0001UL
#define		OTBL_UPKANA	0x10000000UL /* Misc. Kana	   */

#define		OTBL_NC		0x10000UL
#define		OTBL_ABSUP	0x20000UL /* GB2312 row10-11 suppr.*/
#define		OTBL_X213C	0x40000UL /* X-0213compat table	   */
#define		OTBL_X213	0x80000UL /* X-0213 table	   */
#define		OTBL_BIG5	0x100000UL /* BIG5 table type	   */
#define		OTBL_GBK	0x200000UL /* GBK table type	   */
#define		OTBL_CP950	0x400000UL /* cp950 variant of BIG5*/
#define		OTBL_HKU	0x800000UL /* HKU variant of BIG5  */
#define		OTBL_MONO	0x1000000UL /* Single byte type	   */
#define		OTBL_JOHAB	0x2000000UL /* Johab		   */
#define		OTBL_EBC	0x4000000UL /* EBCDIC Based	   */
#define		OTBL_JIS	0x40000000UL /* JIS Kanji	   */

/* output table format:						   */
/* Plane packing notation -
iso-2022 style table: Quadrants style
	(plus keis, jef, B/Right, ShiftJIS)
    bit15 = 0, bit8 = 0:	plane 1 (lower side)
    bit15 = 1, bit8 = 1:	plane 2 (upper side)
    bit15 = 1, bit8 = 0:	plane 3 (used in iso-2022-jp-3-strict)
    bit15 = 0, bit8 = 1:	plane 4 (currently not supported)
bg - non quadrants:
  Big5, cp950
    < 0x100			plane 1 (single byte)
    >= 0x100			plane 2 ((BIG5 code) & 0x7f7f)
  BIG5P
    < 0x100			single byte
    > 0x2000 && <= 0x9fff	(ic & 0x7fff) + 2000;
    >= a000			(ic | 0x8000)
  BIG5-UAO
    < 0x100			single byte
    < 0x8000			plane 2 (single byte chain)
    >= 0x8000			plane 1 (as it is)
  GB18030, GBK
    < 0xff			plane 1 (single byte)
    >= 0x100 && < 0x8000	plane 2 ((GB code) & 0x7f7f)
    >= 0x8000			plane 3 (packed. See SKFGB2KAOUT)
*/

struct out_codetbl_defs {	/* output codetable definition	   */
	char	defschar;	/* token to read external table	   */
	long	tbl_len;	/* table size			   */
	unsigned short *otbl;	/* output table			   */
};

#define  otbl_isotype(x)	((x & 0x0ff00000UL) == 0)
/* --------------------------------------------------------------- */
/* output codeset define tables 				   */
/* --------------------------------------------------------------- */
extern struct out_codetbl_defs o_iso_ascii_defs[];
extern struct out_codetbl_defs o_iso_latin_defs[];
extern struct out_codetbl_defs o_iso_symbol_defs[];
extern struct out_codetbl_defs o_iso_kana_defs[];
extern struct out_codetbl_defs o_iso_cjk_a_defs[];
extern struct out_codetbl_defs o_iso_kanji_defs[];
extern struct out_codetbl_defs o_iso_y_defs[];
extern struct out_codetbl_defs o_iso_hngl_defs[];
extern struct out_codetbl_defs o_iso_compat_defs[];
extern struct out_codetbl_defs o_iso_cjk_b_defs[];
extern struct out_codetbl_defs o_iso_cjk_c_defs[];
extern struct out_codetbl_defs o_iso_upkana_defs[];
extern struct out_codetbl_defs o_iso_prv_defs[];
extern struct out_codetbl_defs o_iso_hist_defs[];
extern struct out_codetbl_defs o_iso_upmisc_defs[];

extern struct out_codetbl_defs *uni_o_ascii_defs;
extern struct out_codetbl_defs *uni_o_latin_defs;
extern struct out_codetbl_defs *uni_o_symbol_defs;
extern struct out_codetbl_defs *uni_o_kana_defs;
extern struct out_codetbl_defs *uni_o_cjk_a_defs;
extern struct out_codetbl_defs *uni_o_kanji_defs;
extern struct out_codetbl_defs *uni_o_y_defs;
extern struct out_codetbl_defs *uni_o_hngl_defs;
extern struct out_codetbl_defs *uni_o_compat_defs;
extern struct out_codetbl_defs *uni_o_cjk_b_defs;
extern struct out_codetbl_defs *uni_o_cjk_c_defs;
extern struct out_codetbl_defs *uni_o_upkana_defs;
extern struct out_codetbl_defs *uni_o_prv_defs;
extern struct out_codetbl_defs *uni_o_note_defs;
extern struct out_codetbl_defs *uni_o_hist_defs;
extern struct out_codetbl_defs *uni_o_upmisc_defs;

extern unsigned short *uni_o_ascii;
extern unsigned short *uni_o_latin;
extern unsigned short *uni_o_symbol;
extern unsigned short *uni_o_kana;
extern unsigned short *uni_o_cjk_a;
extern unsigned short *uni_o_kanji;
extern unsigned short *uni_o_compat;
extern unsigned short *uni_o_cjk_b;
extern unsigned short *uni_o_cjk_c;
extern unsigned short *uni_o_upkana;
extern unsigned short *uni_o_prv;
extern unsigned short *uni_o_hngl;
extern unsigned short *uni_o_y;
extern unsigned short *uni_o_hist;
extern unsigned short *uni_o_note;
extern unsigned short *uni_o_upmisc;

extern skf_ucode      *uniuni_o_prv;

/* table feature */
#if 0
extern unsigned long	ascii_mod;
extern unsigned long	latin_mod;
extern unsigned long	symbol_mod;
extern unsigned long	kana_mod;
extern unsigned long	uni_cjka_mod;
extern unsigned long	uni_kanji_mod;
extern unsigned long	uni_y_mod;
extern unsigned long	uni_hngl_mod;
extern unsigned long	uni_compat_mod;
extern unsigned long	uni_cjkb_mod;
extern unsigned long	uni_cjkc_mod;
extern unsigned long	uni_upkana_mod;
extern unsigned long	uni_prv_mod;
#endif

/* *****************************************************************
** Converters
**
** Code converter set:
**
*******************************************************************
** conversion tables */
/* --------------------------------------------------------------- */
/* --- table anchor ---------------------------------------------- */
#if 0
extern unsigned short *uni2latin;
extern unsigned short *uni2math;
extern unsigned short *uni2cjk;
extern unsigned short *uni2compat;
#endif
/* --------------------------------------------------------------- */
/* --- Actual table from unic*de to jis, euc and Sjis ------------ */
extern unsigned short uni_f_4e[];	/* from unic*de to x208	   */

extern unsigned short uni_f_latin[]; /* unic*de to x208/X212 */
extern unsigned short uni_f_symbol[];
extern const unsigned short uni_f_math_jef[];
extern unsigned short uni_f_kana[];

extern const	char *uni_f_s_21[];

extern unsigned short uni_f_chart[] ; /* in out_code_table.c */
extern const unsigned short moji_kei[];

extern const unsigned short uni_k_enl[];

extern const unsigned short uni_k_cil[]; 
extern unsigned short uni_f_compat[];

#if UNICODE_1
extern const unsigned short uni_k_iroha[];
#endif
extern const unsigned char uni_t_x201[];

#if 0
extern unsigned short uni_f_nttglyph[]; /* doc*mo keitai glyphs	   */
#endif
extern const unsigned short uni_ibm_nec_excg[]; /* cp932 hooks	   */

/* --- !DYNAMIC LOADING additional includes			   */
#ifdef INCLUDE_KR_X_1001_TABLE
extern unsigned short uni_f_x1001[];
#endif
#ifdef INCLUDE_GB2312_TABLE
extern unsigned short uni_f_gb2312[];
#endif

/* --------------------------------------------------------------- */
/* output plane selector 					   */
/* --------------------------------------------------------------- */
#define	is_3pl(x)	((x & 0x8080U) == 0x8080U)
#define is_4pl(x)	((x & 0x8080U) == 0x0080U)
#define is_2pl(x)	((x & 0x8080U) == 0x8000U)
#define is_1pl(x)	((x & 0x8080U) == 0x0000U)
#define is_2ln(x)   (((x & 0x8080U) == 0x8000U) && ((x & 0x7f00U) == 0))

#define stripchar(x)	(x & 0x7f7f)
/* -- KEIS ------------------------------------------------------- */
extern unsigned short KEISOUT1[];
extern const unsigned char KEISOUT3[];

/* --------------------------------------------------------------- */
/* output shift state conditions				   */
/* --------------------------------------------------------------- */
extern unsigned int	g0_char,g0_mid,g0_midl; 
extern unsigned int	ag0_char,ag0_mid,ag0_midl; 
extern unsigned int	g1_char,g1_mid,g1_midl; 
extern unsigned int	g2_char,g2_mid,g2_midl; 
extern unsigned int	g3_char,g3_mid,g3_midl; 
extern unsigned int	g4_char,g4_mid,g4_midl; 
extern unsigned long	g0_typ,ag0_typ,g1_typ,g2_typ,g3_typ,g4_typ;
#define g0_quad		(g0_typ & COD_MB_4)
#define ag0_quad	(ag0_typ & COD_MB_4)
#define g1_quad		(g1_typ & COD_MB_4)
#define g2_quad		(g2_typ & COD_MB_4)
#define g3_quad		(g3_typ & COD_MB_4)
#define g4_quad		(g4_typ & COD_MB_4)

/* --------------------------------------------------------------- */
/* g0 designate code set family					   */
/* --------------------------------------------------------------- */
extern unsigned long	g0_output_shift;
extern unsigned long	g1_output_shift;
#define kanji_shift_bit		0x08000000UL
#define basecjk_shift_bit	0x00008000UL
#define altcjk_shift_bit	0x00004000UL
#define x0201_shift_bit		0x00002000UL /* also braille alp.  */
#define hangul_shift_bit	0x00001000UL /* also braille num.  */
#define x0201_lshift_bit	0x00000800UL
#define utf7_shift_bit		0x00000400UL
#define i8859_shift_bit		0x00000200UL
#define braille_capl_shift_bit	0x00080000UL
#define braille_cap_shift_bit	0x00040000UL
#define i8859x_shift_bit	0x00020000UL
#define keis_shift_bit		0x00010000UL
#define ascii_shift_bit		0x00000100UL
#define g2cjk_shift_bit		0x00000080UL
#define g4cjk_shift_bit		0x00000040UL

#define reset_kanji_shift g0_output_shift = 0
#define set_basecjk_shift g0_output_shift = \
			kanji_shift_bit | basecjk_shift_bit
#define set_altcjk_shift g0_output_shift = \
			altcjk_shift_bit | kanji_shift_bit
#define set_x0201_shift g0_output_shift =\
			x0201_shift_bit | kanji_shift_bit
#define set_x0201_lshift g0_output_shift =\
			x0201_lshift_bit | kanji_shift_bit
#define set_hangul_shift g0_output_shift =\
			hangul_shift_bit | kanji_shift_bit
#define set_utf7_shift g0_output_shift =\
			utf7_shift_bit | kanji_shift_bit
#define set_i8859_x_shift g1_output_shift =\
			i8859x_shift_bit | kanji_shift_bit
#define set_i8859_1_shift g1_output_shift =\
			i8859_shift_bit | kanji_shift_bit
#define set_keis_shift g0_output_shift =\
			keis_shift_bit | kanji_shift_bit
#define set_ascii_shift g0_output_shift =\
			ascii_shift_bit | kanji_shift_bit
#define set_g2cjk_shift g0_output_shift =\
			g2cjk_shift_bit | kanji_shift_bit
#define set_g4cjk_shift g0_output_shift =\
			g4cjk_shift_bit | kanji_shift_bit
#define set_bcapl_shift g0_output_shift |= braille_capl_shift_bit
#define set_bcap_shift g0_output_shift |= braille_cap_shift_bit
#define reset_bcap_shift g0_output_shift &= 0xfff3ffffUL
#define	is_kanji_shift	(g0_output_shift)
#define	is_basecjk_shift (g0_output_shift & basecjk_shift_bit)
#define	is_altcjk_shift	(g0_output_shift & altcjk_shift_bit)
#define	is_hangul_shift	(g0_output_shift & hangul_shift_bit)
#define	is_utf7_shift	(g0_output_shift & utf7_shift_bit)
#define	is_x0201_shift	(g0_output_shift & x0201_shift_bit)
#define	is_x0201_lshift	(g0_output_shift & x0201_lshift_bit)
#define	is_i8859_shift	(g1_output_shift & i8859_shift_bit)
#define	is_i8859x_shift	(g1_output_shift & i8859x_shift_bit)
#define	is_ascii_shift	(g0_output_shift & ascii_shift_bit)
#define	is_keis_shift	(g0_output_shift & keis_shift_bit)
#define	is_g2cjk_shift	(g0_output_shift & g2cjk_shift_bit)
#define	is_g4cjk_shift	(g0_output_shift & g4cjk_shift_bit)
#define	is_bcap_shift	(g0_output_shift & braille_cap_shift_bit)
#define	is_bcapl_shift	(g0_output_shift & braille_capl_shift_bit)

/* --------------------------------------------------------------- */
/* g2 designate family (for iso-2022-jp-2)			   */
/* --------------------------------------------------------------- */
extern unsigned long	g23_output_shift;

#define i8859_g2_shift_bit		0x00000200UL
#define i8859_g3_shift_bit		0x00002000UL
#define reset_g23_shift g23_output_shift = 0

#define is_g2_8859_shift (g23_output_shift & i8859_g2_shift_bit)
#define is_g3_8859_shift (g23_output_shift & i8859_g3_shift_bit)
#define g2_extract_code_set() (g23_output_shift & 0x000000ffUL)
#define g3_extract_code_set() (g23_output_shift & 0x00ff0000UL)
#define set_g2_8859_1_shift g23_output_shift =\
  (g23_output_shift & 0xfffff000UL) | i8859_g2_shift_bit | 0x00000001UL;
#define set_g2_8859_7_shift g23_output_shift =\
  (g23_output_shift & 0xfffff000UL) | i8859_g2_shift_bit | 0x00000007UL;

/* --------------------------------------------------------------- */
/* HZ, zW shift							   */
/* --------------------------------------------------------------- */
extern unsigned long hzzwshift;

#define	HZ_SHIFT_BIT	0x00000010UL
#define ZW_SHIFT_BIT	0x00000002UL
#define ZW_BTWN_BIT	0x00000001UL

#define is_hz_shift	(hzzwshift & HZ_SHIFT_BIT)
#define is_zw_shift	(hzzwshift & ZW_SHIFT_BIT)
#define is_zw_btwn	(hzzwshift & ZW_BTWN_BIT)
#define set_hz_shift	hzzwshift = HZ_SHIFT_BIT
#define res_hz_shift	hzzwshift = 0
#define set_zw_shift	hzzwshift = ZW_SHIFT_BIT
#define set_zw_btwn	hzzwshift = ZW_BTWN_BIT
#define res_zw_shift	hzzwshift = 0

#ifdef FOLD_SUPPORT
/* --------------------------------------------------------------- */
/* fmt support							   */
/* --------------------------------------------------------------- */
extern skf_ucode	prev_ch;
extern const unsigned short	o_ascii_entity[];

#define O_IS_ALPHANUM	0x0001U
#define O_IS_DELIM	0x0002U	/* can cut line before this char   */
#define O_IS_ADELIM	0x0004U	/* can cut line after this char	   */
#define O_YAKU_OIDASH	0x0010U
#define O_YAKU_BURASA	0x0020U
#define O_IS_TAIL	0x0100U	/* end of line indicator	   */
#define O_IS_SDELIM	0x0200U	/* can terminate sentence	   */

#define o_fold_isalphanum(x) (o_ascii_entity[x] & O_IS_ALPHANUM)
#define o_fold_delim(x)	     (o_ascii_entity[x] & O_IS_DELIM) 
#define o_fold_adelim(x)     (o_ascii_entity[x] & O_IS_ADELIM)
#define o_fold_tail(x)	     (o_ascii_entity[x] & O_IS_TAIL)
#define o_fold_isoidash(x)   (o_ascii_entity[x] & O_YAKU_OIDASH)
#define o_fold_isburasa(x)   (o_ascii_entity[x] & O_YAKU_BURASA)
#define or_fold_isalphanum(x) (x & O_IS_ALPHANUM)
#define or_fold_delim(x)     ((x & O_IS_DELIM) || \
			      (x & O_IS_TAIL))
#define or_fold_adelim(x)     (x & O_IS_ADELIM)
#define or_fold_sdelim(x)     (x & O_IS_SDELIM)
#define or_fold_tail(x)	     (x & O_IS_TAIL)
#define or_fold_isoidash(x)   (x & O_YAKU_OIDASH)
#define or_fold_isburasa(x)   (x & O_YAKU_BURASA)
#endif	/* FOLD_SUPPORT */

#define tail_prevch(x)	(\
	((!first_detect_cr || !detect_lf) && (x == A_CR)) || \
	((first_detect_cr || !detect_cr) && (x == A_LF)))
#define term_prevch(x)	((x == '.') || (x == '?') || (x == 0x3a) \
	|| (x == '!') || (x == 0x3002) || (x == 0x300d) \
	|| (x == 0x300f) || (x == 0xff61) || (x == 0xff63))
/* --------------------------------------------------------------- */
/* GB18030 packed converters					   */
/* --------------------------------------------------------------- */
/* NOTICE: gb18030-A5 input/output table is packed for compaction  */
/* --------------------------------------------------------------- */
#if defined(SHORTEN_GB2K)
#define gb18030_unpack(x)	\
	(((x & 0x7fffU) >= 0x4abd) ? ((x & 0x7fffU) + 0x3570) : (x & 0x7fffU))
#define gb18030_pack(x)	\
	((x >= 0xa000) ? ((x & 0x7fffU) - 0x3570) : (x & 0x7fffU))
#else
#define gb18030_unpack(x)	\
	(((x & 0x7fffU) >= 0x4abd) ? ((x & 0x7fffU) + 0x1ab8) : (x & 0x7fffU))
#define gb18030_pack(x)	\
	((x >= 0xa000) ? ((x & 0x7fffU) - 0x1ab8) : (x & 0x7fffU))
#endif
/* --------------------------------------------------------------- */
/* Actual converters						   */
/* --------------------------------------------------------------- */
/* EUC */
#define	r_SKFEUC1OUT(c3)	\
	{ if (is_euc7(conv_cap)) {\
	    if (is_kanji_shift) { SKFputc(A_SI); reset_kanji_shift;\
	    } else ; \
	    SKFputc(c3 & 0x7fU);\
	  } else { SKFputc(c3);}; }

/* --------------------------------------------------------------- */
extern void SKFEUC1OUT P_((skf_ucode));
extern void SKFEUCOUT P_((skf_ucode));
extern void SKFEUCG2OUT P_((skf_ucode));
extern void SKFEUCG3OUT P_((skf_ucode));
extern void SKFEUCG4OUT P_((skf_ucode));

extern int SKFEUCcount P_((skf_ucode));

/* -------------------------------------------------------------- */
#define r_SKFEUCOUT(c3) \
	{ if (is_euc7(conv_cap)) { \
	    if (!is_kanji_shift) { SKFputc(A_SO); set_basecjk_shift;\
	    } else; \
	    SKFputc((c3 >> 8) & 0x7f);\
	    SKFputc(c3 & 0x7f);\
	  } else { SKFputc((((c3) & 0x7f00) >> 8) | 0x80); \
	      SKFputc(((c3) & 0xff) | 0x80); }; }
/* -------------------------------------------------------------- */

#if 0
extern int  SKFEUCASC	P_(());
extern int  SKFEUCADD1	P_(());
extern int  SKFEUCADD2	P_(());
extern int  SKFEUCADD3	P_(());
#endif

/* SJIS */
#define	SKFSJIS1OUT(c3)	\
		{ SKFputc(c3);}

#define	SKFSJISK1OUT(c3)	\
		{ SKFputc((c3) | 0x80U);}

extern void SKFSJISOUT P_((skf_ucode));
extern void SKFSJISG2OUT P_((skf_ucode));
extern void SKFSJISG3OUT P_((skf_ucode));
extern void SKFSJISG4OUT P_((skf_ucode));

extern int SKFSJIScount P_((skf_ucode));

#if 0
extern int  SKFSJISASC	P_(());
extern int  SKFSJISADD1	P_(());
extern int  SKFSJISADD2	P_(());
extern int  SKFSJISADD3	P_(());
#endif

/* JIS */
extern void SKFJIS1OUT P_((skf_ucode));
extern void SKFJIS1ASCOUT P_((skf_ucode));
extern void SKFJISK1OUT P_((skf_ucode));
extern void SKFJISOUT P_((skf_ucode));
extern void SKFJISG2OUT P_((skf_ucode));
extern void SKFJISG3OUT P_((skf_ucode));
extern void SKFJISG4OUT P_((skf_ucode));
extern void SKFJIS8859OUT P_((skf_ucode));
extern void SKFJIS8859XOUT P_((skf_ucode));

extern int SKFJIScount P_((skf_ucode));

/* -------------------------------------------------------------- */
#define r_SKF_ADD_RENEW \
    if (!sup_jis90 && !is_jis_213c(conv_cap)) { \
	SKFputc(A_ESC); SKFputc('&'); SKFputc('@'); };
/* -------------------------------------------------------------- */
#define r_SKFJIS1OUT(c1) \
    { if (is_kanji_shift) { \
	if (is_x0201_lshift) SKFputc(A_SI); \
	else {SKFputc(A_ESC); SKFputc(g0_mid); SKFputc(g0_char);}; \
	reset_kanji_shift; \
	if (o_encode) SKFputc(mFLSH);\
    }; \
    SKFputc(c1); }
/* -------------------------------------------------------------- */
#define r_SKFJISOUT(c1) \
    { int p,q; \
      p = ((c1) >> 8) & 0x7f; q = (c1) & 0x7f; \
      if (!is_basecjk_shift) { \
        if (add_renew) { r_SKF_ADD_RENEW; } \
	set_basecjk_shift; \
	if (is_euc7(conv_cap)) { SKFputc(A_SO);\
	} else { SKFputc(A_ESC); SKFputc(ag0_mid); \
	    if (ag0_quad) SKFputc(ag0_midl); \
	    SKFputc(ag0_char); }; };\
      SKFputc(p); SKFputc(q); };
/* -------------------------------------------------------------- */
/* --- KEIS -------- */
extern void	SKFKEISOUT P_((skf_ucode));
extern void	SKFKEIS1OUT P_((skf_ucode));
extern void	SKFKEISG2OUT P_((skf_ucode));
extern void	SKFKEISG3OUT P_((skf_ucode));
extern void	SKFKEISG4OUT P_((skf_ucode));
extern void 	SKFKEISK1OUT P_((skf_ucode));
extern void	SKFKEISEOUT P_((skf_ucode));

/* --- BG -------- */
extern void	SKFBGOUT P_((skf_ucode));
extern void	SKFBG1OUT P_((skf_ucode));
extern void	SKFGB2KAOUT P_((skf_ucode));

/* --- BRGT -------- */
extern void	SKFBRGTOUT P_((skf_ucode));
extern void	SKFBRGT1OUT P_((skf_ucode));
extern void	tron_announce ();

/* --- Unicode -------- */
#if 0
extern int  SKFUNIASC	P_(());
extern int  SKFUNIADD1	P_(());
extern int  SKFUNIADD2	P_(());
extern int  SKFUNIADD3	P_(());
#endif

/* ---- string handlers ---------- */
extern void	SKFJISSTROUT P_((const char *));
extern void	SKFEUCSTROUT P_((const char *));
extern void	SKFSJISSTROUT P_((const char *));
extern void	SKFKEISSTROUT P_((const char *));
extern void	SKFUNISTROUT P_((const char *));
extern void	SKFBRGTSTROUT P_((const char *));
extern void	SKFBGSTROUT P_((const char *));

extern void	SKFSTROUT P_((const char *));

/* ---- mime pre-process --------------- */
/* Note: BRGT does not handle MIME encoding */
extern void out_JIS_encode P_((skf_ucode,skf_ucode));
extern void out_EUC_encode P_((skf_ucode,skf_ucode));
extern void out_SJIS_encode P_((skf_ucode,skf_ucode));
extern void out_BG_encode P_((skf_ucode,skf_ucode));

extern void out_UNI_encode P_((skf_ucode,skf_ucode));

/* ---- status reset --------------- */
#define SKFJIS1FLSH() \
    { if (is_kanji_shift) { \
	if (is_x0201_lshift) SKFputc(A_SI); \
	else {SKFputc(A_ESC); SKFputc(g0_mid); SKFputc(g0_char);}; \
	if (o_encode) SKFputc(mFLSH);\
	reset_kanji_shift; \
      }; \
    }
#define SKFEUC1FLSH() \
	{ if (is_euc7(conv_cap)) {\
	    if (is_kanji_shift) { SKFputc(A_SI); reset_kanji_shift;\
	    } else ; \
	}; }
#define SKFBRGT1FLSH()	;
#define SKFSJIS1FLSH()	;
/* Note: KEIS and IBM93x are stateful encoding, but not supported.  */
#define SKFKEIS1FLSH()	;
#define SKFBG1FLSH()	;
#ifdef ACE_SUPPORT
/* FIXME: not sure that the code below is correct. */
#define SKFUNI1FLSH() \
	{ if (is_ucs_puny(conv_cap)) {\
	    o_p_encode(sFLSH);\
	  }; \
	};
#else
#define SKFUNI1FLSH() ;
#endif

/* -------------------------------------------------------------- */
/* mime encode line counters */

extern void encode_clipper P_((int, int));
extern void mime_limit_set P_(());
extern void mime_limit_add P_((int));

/* ---- line end common routines --- */
extern void	SKFCRLF();

/* ---- output converters --------- */
#define o_ascii_conv(x)	{if (is_jiscat(conv_cap)) { \
	    if (is_jis(conv_cap)) JIS_ascii_oconv(x); \
	    else EUC_ascii_oconv(x);\
	} else if (is_ucs_ufam(conv_cap)) {\
	    UNI_ascii_oconv(x); \
	} else if (out_ocat) {\
	    if (is_msfam(conv_cap)) SJIS_ascii_oconv(x);\
	    else if (out_bg(conv_cap)) BG_ascii_oconv(x);\
	    else if (is_keis(conv_cap)) KEIS_ascii_oconv(x);\
	    else BRGT_ascii_oconv(x);\
	} else { EUC_ascii_oconv(x); }; }
/* -------------------------------------------------------------- */
#define o_cjkkana_conv(x)	{if (is_jiscat(conv_cap)) { \
	    if (is_jis(conv_cap)) JIS_cjkkana_oconv(x); \
	    else EUC_cjkkana_oconv(x);\
	} else if (is_ucs_ufam(conv_cap)) {\
	    UNI_cjkkana_oconv(x); \
	} else if (out_ocat) {\
	    if (is_msfam(conv_cap)) SJIS_cjkkana_oconv(x);\
	    else if (out_bg(conv_cap)) BG_cjkkana_oconv(x);\
	    else if (is_keis(conv_cap)) KEIS_cjkkana_oconv(x);\
	    else BRGT_cjkkana_oconv(x);\
	} else { EUC_cjkkana_oconv(x); }; }
/* -------------------------------------------------------------- */
#define o_cjk_conv(x)	{if (is_jiscat(conv_cap)) { \
	    if (is_jis(conv_cap)) JIS_cjk_oconv(x); \
	    else EUC_cjk_oconv(x);\
	} else if (is_ucs_ufam(conv_cap)) {\
	    UNI_cjk_oconv(x); \
	} else if (out_ocat) {\
	    if (is_msfam(conv_cap)) SJIS_cjk_oconv(x);\
	    else if (out_bg(conv_cap)) BG_cjk_oconv(x);\
	    else if (is_keis(conv_cap)) KEIS_cjk_oconv(x);\
	    else BRGT_cjk_oconv(x);\
	} else { EUC_cjk_oconv(x); }; }
/* -------------------------------------------------------------- */
#define o_compat_conv(x) {if (is_jiscat(conv_cap)) { \
	    if (is_jis(conv_cap)) JIS_compat_oconv(x); \
	    else EUC_compat_oconv(x); \
	} else if (is_ucs_ufam(conv_cap)) { \
	    UNI_compat_oconv(x); \
	} else if (out_ocat) { \
	    if (is_msfam(conv_cap)) SJIS_compat_oconv(x);\
	    else if (out_bg(conv_cap)) BG_compat_oconv(x); \
	    else if (is_keis(conv_cap)) KEIS_compat_oconv(x); \
	    else BRGT_compat_oconv(x); \
	} else { \
	    EUC_compat_oconv(x);  \
	}; }

/* -------------------------------------------------------------- */

extern void	ascii_fract_conv P_((skf_ucode));
extern void	CJK_sq_conv P_((skf_ucode));
extern void	CJK_cc_conv P_((skf_ucode));
extern void	print_announce P_((int));
extern void	cjk_compat_parse P_((skf_ucode));/* compatible plane conv */
extern void	symbol_conv P_((skf_ucode));	/* 0x2000-0x2dff  */
extern void	viqr_convert P_((skf_ucode));
extern void	lig_x0213_out P_((skf_ucode,skf_ucode));
extern void	lig_compat P_((skf_ucode));
extern void	GRPH_lig_conv P_((skf_ucode));
extern void	enc_alpha_supl_conv P_((skf_ucode));
extern void	enc_cjk_supl_conv P_((skf_ucode));

extern int	y_in_dec P_((int));	/* in in_converter.c	  */

extern int 	test_out_char P_((skf_ucode));

extern void	show_lang_tag ();

extern void	SKF_STRPUT P_((unsigned short *));
extern void	utf8_uriout P_((skf_ucode));
extern char	*utf8_urioutstr P_((skf_ucode));
/* ---- language tag controls ----- */
extern void	output_language_tag_trigger P_(());
/* ---- to make code small -------- */
extern void 	ox_ascii_conv P_((skf_ucode));

extern int	utf7_res_bit;	/* used in out_encode.c		  */

/* -------------------------------------------------------------- */
#define ENCODE_MGN_1	6
#define ENCODE_MGN_2	(ENCODE_MGN_1 - 2)
#define ENCODE_MGN_3	2

#define table_susp_tail(x) ((x == '>') || (x == ')'))

#define mime_safe(x) (((x >= A_SP) && (x < A_DEL) \
    && (x != '=') && (x != '?') && (x != '(') && (x != ')') && (x != '_')\
    && (x != 0x22)) || (x == A_LF) || (x == A_CR))
	/* rfc2045 spec. plus extra care */

#define table_mime_tail(x) ( \
    (x == '<') || (x == ',') || (x == '.') || (x == '>') || (x == '('))

extern void	SKF1FLSH P_(());
extern void	utf16_clipper P_((skf_ucode));

#define SKFcputc(x) {SKFrputc(x); o_encode_lm++;o_encode_lc++;}
#define oconv_flush()	oconv(sFLSH)

extern void	encoder_tail();
extern void 	in_sbroken P_((int,int));
extern void	skf_lastresort P_((skf_ucode));
extern int	skf_lastcount P_((skf_ucode));

#ifdef UNI_DECOMPOSE
extern void	unicode_normalize_setup();
extern void	decompose_code P_((int));
extern void	decompose_hangul P_((skf_ucode));
extern int	get_combine_strength P_((skf_ucode));
extern void	debugcharout P_((int));;

#define NKD_STRMAP_STRMASK	0x00ffU

#define is_combining(x)	(get_combine_strength(x) < 255)
#endif

#ifdef UNI_ENCOMPOSE
#define ENCOMPOSE_DEPTH		256	/* max is 64 in 6.0	*/

#define ENCOMPOSE_SDEPTH	2048	/* table length		 */
      /* this value eventually is extracted from Unicodedata.txt */

#define ENCOMPOSE_UPLIMIT	0x20000
#define ENCOMPOSE_MASK_LEN	512	/* ENCOMPOSE_UPLIMIT/256 */		

#if defined(FUTURESUPPORT)
/* encompose: encompose is always enabled because of kana */
struct encompose_ctbl {
    skf_ucode c_identity;	/* candidate char		*/
    int		leaf_id;	/* next leaf			*/
};
#endif

struct encompose_leaf {
    skf_ucode	c_identity;	/* current char			*/
    skf_ucode	c_compose;	/* composed character		*/
    int		rev_p;		/* parent leaf			*/
    /* search depth						*/
    int		f_ch;		/* following character variations */
    /* following integer is pointer to encompose_list		*/
    int		f_pos;
};

extern struct encompose_leaf *encompose_tree;
extern int 	*encompose_start; 

extern int 	*encompose_list; 
	/* points encompose tree leaf */

#endif

#define WEAK_BURASAGE	0x01
#define STRONG_BURASAGE	0x02
#define WEAK_OIDASI	0x10

#define is_weak_burasage(x) (x & WEAK_BURASAGE)
#define is_weak_oidasi(x) (x & WEAK_OIDASI)
#define is_strong_burasage(x) (x & STRONG_BURASAGE)

#if defined(ROT_SUPPORT) && defined(NEW_ROT_CODE) 

extern void SKF_rotoconv P_((int,int));
extern void SKFROTTHRU P_((int,int));

#endif


/* -------------------------------------------------------------- */
/* locale default codeset definition				  */
/* -------------------------------------------------------------- */

struct locale_codeset_name  {
    unsigned long	langnat;	/* language and nation	  */
    int		codename;
};

/* -------------------------------------------------------------- */
#endif
