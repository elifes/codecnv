/* *******************************************************************
** Copyright (c) 2000-2014 Seiji Kaneko. All rights reserved.
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
    convert.h	input side converter common header
    $Id: convert.h,v 1.112 2017/01/05 15:05:48 seiji Exp seiji $
*/
#ifndef CONVERT_H_DEFINED
#define CONVERT_H_DEFINED

#define	E_K_AKH		(0x80UL | K_AKIH)
#define	E_K_AKL		(0x80UL | K_AKIL)
#define	E_A_MAX		(0x80UL | A_MAX) /* Alpha area end for euc  */
#define E_K_ST		(0x80UL | K_ST)  /* kanji area start for euc */

/* spec(rfc2045) limit is 72 + header + margin			   */
/* this must be smaller than decode_hook_value array size	   */
#define MIME_CHAR_LIMIT 108 
/* MIME CHARSET IDENTIFIER LENGTH LIMIT				   */
/* Note: this must be longer than MIME code name AND skf options   */
#define MIME_CSET_LEN	32

#define is_lineend(x)	((x == A_LF) || (x == A_CR))
#define is_true_lineend(x) \
	((ch == A_CR) ? \
	  ((!detect_lf || (detect_cr && first_detect_cr)) ? 1 : 0) : \
	  ((!detect_cr || (detect_lf && !first_detect_cr)) ? 1 : 0))
#define is_hex_char(x)	(((x >= '0') && (x <= '9')) || \
			 ((x >= 'A') && (x <= 'F')) || \
			 ((x >= 'a') && (x <= 'f')))
				/* equivalant to GNU isxdigit	   */
#define is_white(x)  ((x == ' ')||(x == '\t'))
				/* equivalant to GNU isblank	   */
#define is_control(x)  ((x < 0x20) && ((x >= 0x10) || (x < A_BEL)))
				/* should escape for hexes.	   */
#define is_latinterm(x) ((x == A_DOT) || (x == A_COMMA) || (x == '?')\
		|| (x == 0xbf) || (x == '!'))
#define is_space(x)	((x == ' ') || (x == '\v') || (x == '\n') \
			  || (x == '\t') || (x == '\r') || (x == '\f'))
#define is_digit(x)	((x >= '0') && (x <= '9'))
#define is_alpha(x)	(((x >= 'A') && (x <= 'Z')) \
			  || ((x >= 'a') && (x <= 'z')))
#define is_lower(x)	((x >= 'a') && (x <= 'z'))
#define is_upper(x)	((x >= 'A') && (x <= 'Z'))
#define is_cname_char(x) ((x == '-') || ((x >= '0') && (x <= '9')) \
			|| ((x >= 'A') && (x <= 'Z')) || (x == '_') \
			|| ((x >= 'a') && (x <= 'z')))
#define is_ascii(x)	((x >= 0x21) && (x < 0x7f))

#define SKFtolower(x)	(((x>='A')&&(x<='Z'))? (x+0x20) : x)
#define SKFtoupper(x)	(((x>='a')&&(x<='z'))? (x-0x20) : x)

#define skf_hex(x)	(x -'0'-((x >= 'A') ? ((x >= 'a') ? 39:7) : 0))

/* internals							   */
extern int	s_in P_((skfFILE *));
extern int	e_in P_((skfFILE *));
extern int	ks_in P_((skfFILE *));
extern int	u_in P_((skfFILE *));
extern int	z_in P_((skfFILE *));
extern int	y_in P_((skfFILE *));
extern int	b_in P_((skfFILE *));
extern int	t_in P_((skfFILE *));
extern int	be_in P_((skfFILE *));
extern int	esc_process P_((skfFILE *));
extern int	c1_process P_((skfFILE *, int));
extern int	seq_sweep P_((skfFILE *, int));

extern int	cns11643_defined_code;

#ifdef FUTURESUPPORT
extern int	is_idnspace P_((skf_ucode));
#endif

/* --------------------------------------------------------------- */
extern int preConvert P_((skfFILE *));

extern void init_all_stats();
extern void clear_all_stats();

extern void g0table2low();	/* psuedo macro			   */
extern void g1table2low();
extern void g2table2low();
extern void g3table2low();
extern void g0table2up();
extern void g1table2up();
extern void g2table2up();
extern void g3table2up();

/* --------------------------------------------------------------- */
/* Unicode processing						   */
/* --------------------------------------------------------------- */
/* parameter definitions					   */
/* --------------------------------------------------------------- */
#define DEC_HOOK_UTF16	0
#define DEC_HOOK_UTF8	1
#define DEC_HOOK_UTF7	2
#define DEC_HOOK_UTF32	3

#define is_cod_utf16(x)	(x == DEC_HOOK_UTF16)
#define is_cod_utf32(x)	(x == DEC_HOOK_UTF32)
#define is_cod_utf8(x)	(x == DEC_HOOK_UTF8)
#define is_cod_utf7(x)	(x == DEC_HOOK_UTF7)

extern int	uni_in	P_((skfFILE *, int));
extern skf_ucode u_dec_hook P_((skfFILE *, int));
extern int	u_parse	P_((skfFILE *, skf_ucode, int));
extern void clear_after_mime();

/* --------------------------------------------------------------- */
/* table of tables						   */
/* 	defines table to convert codeset to unic*de.		   */
/*	definition is in in_code_table.c			   */
/* --------------------------------------------------------------- */
/* table len must not set below 128 (no check in e_in for speed)   */
/*  charwidth:	1	unibyte table and table entry is short	   */
/*  		2	multibyte table and table entry is short   */
/*  		3	unibyte table and table entry is long      */
/*  		4	multibyte table and table entry is long	   */
/* --------------------------------------------------------------- */
struct iso_byte_defs {		/* coded charset definitions	   */
	char	defschar;	/* final-byte-char in iso2022	   */
	short	char_width;	/* unicode table size + MB	   */
	int	table_len;	/* unicode table entry limit	   */
			/* this is for checking purpose.	   */
	unsigned short	*unitbl; /* points table iff char_width < 3*/
	unsigned long 	is_kana; /* misc flags. see below	   */
	skf_ucode 	*uniltbl;/* points table iff char_width = 4*/
	unsigned short	lang;	/* language definition for codeset */
	void	(*hook)();	/* hook function for some fix.	   */
	char	*desc;		/* description for this charset	   */
	char	*cname;		/* canonical name for this charset */
};

extern struct iso_byte_defs iso_unibyte_defs[];
extern struct iso_byte_defs iso_im2byte_defs[];
extern struct iso_byte_defs iso_iso8859_defs[];
extern struct iso_byte_defs iso_3_dblbyte_defs[];
extern struct iso_byte_defs iso_4_dblbyte_defs[];
extern struct iso_byte_defs misc_byte_defs[];
extern struct iso_byte_defs miscmul_byte_defs[];
extern struct iso_byte_defs cp_byte_defs[];
extern struct iso_byte_defs ovlay_byte_defs[];
extern struct iso_byte_defs priv_dblbyte_defs[];
extern struct iso_byte_defs priv_byte_defs[];
extern struct iso_byte_defs priv_drcs_byte_defs[];
extern struct iso_byte_defs priv_drcs_dblbyte_defs[];

/* --------------------------------------------------------------- */
/* is_kana flags 						   */
/* --------------------------------------------------------------- */
/* Note that 0x00000300 is used as COD_xx in skf.h		   */
/*  DO NOT USE COD_99 and COD_78_83 with kana_flag		   */
/*  upper 4B: table entry and misc control			   */
/*  lower 4B: execution control (set to low/high kana)		   */
#define COD_IS_KANA	0x00000001UL	/* is kana feature	   */
#define COD_KANJI	0x00000006UL	/* is jis-kanji feature	   */
#define COD_X0208	0x00000002UL	/* is x-0208		   */
#define COD_X0212	0x00000004UL	/* is x-0212		   */
#define COD_USS		0x00000004UL	/* Has SS-feature(non GR)  */
					/* !MB & KANJI = USS	   */
#define COD_NISO	0x00008000UL	/* CODE is not ISO-2022    */
#define COD_MANUAL	0x00004000UL	/* do not use code table   */
#define COD_8b		0x00010000UL	/* GR side settings	   */
#define COD_25		0x00020000UL	/* GR/GL double set	   */
#define COD_MB		0x00002000UL	/* Multibyte		   */
#define COD_MB_4	0x00040000UL	/* ISO MB 4octet code	   */
#define COD_PRIV	0x00080000UL	/* ISO private area	   */
#define COD_X213_1	0x00000020UL	/* jis x-0213 1st plane	   */
#define COD_X213_2	0x00000010UL	/* jis x-0213 2nd plane	   */
#define COD_X213_MSK	0x0000203fUL	/* jis x-0213 		   */
#define COD_4B		0x00000040UL	/* 4B table		   */
#define COD_SET96	0x00001000UL	/* 96 character code set   */
#define COD_ISOALT	0x00040000UL	/* second intermediate set */
#define COD_TYP_TMSK	0x000ffc00UL
#define COD_SHTCNT_MSK	0x00002004UL
#define COD_HAS_D8	0x00100000UL	/* includes 0xd800-0xdfff  */
/* Note: COD_X213_1 and ISOALT is determined with character width. */

#define is_x213_1(x)	(((x) & COD_X213_MSK) == (COD_MB | COD_X213_1))
#define is_x213_2(x)	(((x) & COD_X213_MSK) == (COD_MB | COD_X213_2))
#define is_tbl_kanji(x) ((x & COD_KANJI) != 0)
#define is_tbl_x0208(x) ((x & COD_X0208) != 0)
#define is_tbl_x0212(x) ((x & COD_X0212) != 0)
#define is_tbl_kana(x)  ((x & COD_IS_KANA) != 0)
#define is_tbl_man(x)	((x & COD_MANUAL) != 0)
#define is_tbl_iso(x)	((x & COD_NISO) == 0)
#define is_tbl_b4(x)	((x & COD_MB_4) != 0)
#define is_tbl_mb(x)	((x & COD_MB) != 0)
#define is_tbl_set96(x) ((x & COD_SET96) != 0)
#define is_tbl_isoalt(x) ((x & COD_ISOALT) != 0)
#define is_tbl_has_ss(x) ((x & COD_SHTCNT_MSK) == COD_USS)
#define is_tbl_has_d8(x) ((x & COD_HAS_D8) == COD_HAS_D8)
#define cod_kana_mask(x) (int)(x & 0x0000ffffUL)

#define is_isocode	((in_codeset >= 0) && \
			  is_jiscat(i_codeset[in_codeset].encode))

#define KANJI_TBL_END	9025	/* must set later		   */
#define X0212_TBL_END	9025	/* must set later		   */

#define X0208_KANJI_END 7896	/* After kuten 85-01 is gaiji area */
#define X0208_CODE_END 8836	/* table end (94*94)		   */
#define ISOMB_CODE_END 8836	/* table end (94*94)		   */
#define ISOSB_CODE_END 128	/* table end for single byte	   */

#define MS_PRIV_LIM	0xe758	/* cp932 fam. private def. range   */

#define UNI_UDF_A	0xe000
#define	UNI_UDF_B	0xe3ac

#define JOHAB_HANGLE	0xd3
#define JOHAB_HANJA0	0xd8
#define JOHAB_HANJA0E	0xde
#define JOHAB_HANJA1	0xe0
#define JOHAB_HANJA1E	0xf9

#define is_multibyte(x)	(((x & 0x01) == 0) && (x != 0))
#define is_singlebyte(x) ((x & 0x01) == 1)
#define is_tbllong(x)	(x > 2)
#define is_tblshort(x)	(x < 3)
#define is_tblundef(x)	((x == 0) || (x >= 5))

/* --------------------------------------------------------------- */
/* input codetable predefinitions				   */
/* --------------------------------------------------------------- */
struct skf_codeset_point {
	int	tbl_index;
	int	index;
};

/* --------------------------------------------------------------- */
/* codeset type table						   */
/*  codeset definition. Use for both input and output		   */
/*	definition is in out_code_table.c			   */
/* --------------------------------------------------------------- */
struct in_codeset_defs {
/* input side */
    unsigned long	encode;		/* see below		   */
    unsigned long	alt_encode;
    struct skf_codeset_point g0def;	/* codeset presettings..   */
    struct skf_codeset_point g0adef;
    struct skf_codeset_point g1def;
    struct skf_codeset_point g2def;
    struct skf_codeset_point g3def;

/* output side */
    struct skf_codeset_point ogldef;	/* latin area additions	   */
    unsigned long	omap_typ;	/* see below		   */

    short o_iso_ascii_index;	/* ucs2 -> codeset conversion tbl. */
    short o_iso_latin_index;
    short o_iso_symbol_index;
    short o_iso_kana_index;
    short o_iso_cjk_a_index;
    short o_iso_kanji_index;
    short o_iso_y_index;
    short o_iso_hngl_index;
    short o_iso_compat_index;
    short o_iso_cjk_b_index;
    short o_iso_cjk_c_index;
    short o_iso_prv_index;
    short o_iso_his_index;
    short o_iso_note_index;
    short o_iso_upkana_index;
    short o_iso_upmisc_index;
    short o_iso_alt_index;

    void	(*hook)();	/* hook function for table gen.	   */
    const long	*o_patch;	/* postfix code			   */
    unsigned long	oconv_type;
    unsigned short	oconv_lang;	/* two ASCIIs		   */
    char	*desc;		/* description for this codeset	   */
    char	*cname;		/* canonical name of the codeset   */
};

extern struct in_codeset_defs i_codeset[];

/* --------------------------------------------------------------- */
/* omap_typ: output quadrant table assignment			   */
/* --------------------------------------------------------------- */
#define OMAP_MASK	0x0000ffffUL
#define OMAP_ASG 	0x00000000UL	/* use G1,G2,G3,G4	   */
#define OMAP_NSG	0x00000002UL	/* use G1,AG1,G3,G4	   */
#define OMAP_7BIT	0x00000100UL	/* 7bit based output	   */
#define OMAP_RAW	0x00001000UL	/* raw-based output	   */
#define OMAP_MAP	0x00000010UL	/* use non-Q, use G1,G2	   */
#define OMAP_AMAP	0x00000020UL	/* use non-Q, use G1,G1A   */

#define is_omap_asg8(x)	((x & OMAP_MASK) == OMAP_ASG)
#define is_omap_asg7(x)	((x & OMAP_MASK) == (OMAP_7BIT | OMAP_ASG))
#define is_omap_nsg8(x)	((x & OMAP_MASK) == (OMAP_NSG)
#define is_omap_nsg7(x)	((x & OMAP_MASK) == (OMAP_7BIT | OMAP_NSG))
#define is_omap_map(x)	((x & OMAP_MASK) == (OMAP_RAW | OMAP_MAP))
#define is_omap_amap(x)	((x & OMAP_MASK) == (OMAP_RAW | OMAP_AMAP))
/* --------------------------------------------------------------- */
/* for table scan 						   */
/*	root table for charset					   */
/* --------------------------------------------------------------- */
struct iso_byte_defs_entry {
    struct iso_byte_defs	*ientry;	/* points charset  */
    unsigned short		setcap;
			/* controls commandline predefinition	   */
    int				deflimit; /* charset table entries */
    char			*desc;	/* for support code dump   */
};

extern struct iso_byte_defs_entry iso_ubytedef_table[];
/* --------------------------------------------------------------- */
/* parsing up-kana, low-kana:					   */
/* --------------------------------------------------------------- */
#define is_up_kana	(up_kana & COD_IS_KANA)
#define is_low_kana	(low_kana & COD_IS_KANA)

/* --------------------------------------------------------------- */
/* mime processing						   */
/*	definition is in in_code_table.c			   */
/*	see those entries. will be more informative than comment   */
/* --------------------------------------------------------------- */
struct skf_mimedef {
	char	*defschar;
	unsigned long	det_typ;
	int	set_typ;	/* type packed definitions	   */
	int	index;		/* index for codeset in table	   */
	int	r_index;	/* index for codesets table	   */
	int	c_index;	/* output codeset definitions	   */
};

extern int parse_mime_charset P_((int  *));
extern char *skf_ext_table;

extern int has_mime;		/* true if detect input mime	   */
/* note: this is for mime table. DO NOT mix with COD_xx, for this  */
/*  leads to bloody mess.					   */
#define TBL_TYP_HS	0x0080		/* separate hand support   */

#define	MIME_G0		0
#define	MIME_G0A	2
#define	MIME_G1		3
#define	MIME_G2		4
#define	MIME_G3		5

/* --------------------------------------------------------------- */
/* B-Right/V processing						   */
/* --------------------------------------------------------------- */
#define BV_JIS	0x21		/* System script		   */
#define BV_GT0	0x22		/* GT shotai font		   */
#define BV_GT1	0x23
#define BV_CNS0	0x26		/* CNS-11643 characters		   */
#define BV_CNS1	0x27
#define BV_DK0	0x28		/* DAI-KANWA			   */
#define BV_DK1	0x29
#define BV_TRT0	0x2a		/* Tompa characters		   */
#define BV_UC0	0x30		/* Unic*de script		   */
#define BV_UC1	0x31
#define BV_SST  0xe1
#define BV_SED  0xe2
#define BV_ESC  0xff

/* --------------------------------------------------------------- */
#if 0
extern unsigned short uni_t_k21[];
extern unsigned short uni_t_k22[];
#endif

extern unsigned short uni_t_x208[];
#if 0
extern unsigned short uni_t_dia[];
extern unsigned short uni_t_xsign[];
extern unsigned short uni_t_xgreek[];
extern unsigned short uni_t_xrcap[];
extern unsigned short uni_t_xrom[];
#endif
extern unsigned short uni_t_x212[];

extern unsigned short *uni_t_sjisx208;

extern unsigned short ascii_uni_byte[];

/* --------------------------------------------------------------- */
/* input shift state conditions					   */
/* --------------------------------------------------------------- */
extern unsigned long shift_condition;
extern unsigned long sshift_condition;	/* single shift specific   */
/* --------------------------------------------------------------- */
/* merging in_x213 is tempotally hack for v1.9x.   S.Kaneko	   */
/* --------------------------------------------------------------- */
#define	IN_LS1_FLG	0x00000001UL
#define	IN_LS2_FLG	0x00000002UL
#define	IN_LS3_FLG	0x00000004UL
#define	IN_RS1_FLG	0x00000010UL
#define	IN_RS2_FLG	0x00000020UL
#define	IN_RS3_FLG	0x00000040UL
#define	IN_SS1_FLG	0x00000100UL
#define	IN_SS2_FLG	0x00000200UL
#define	IN_SS3_FLG	0x00000400UL
#define	IN_MACROL_FLG	0x00010000UL	/* macro is in GL	   */
#define	IN_MACROR_FLG	0x00020000UL	/* macro is in GR	   */
#define	IN_221_2_FLG	0x01000000UL	/* x-0221: 2 octet	   */
#define	IN_221_4_FLG	0x02000000UL	/* x-0221: 4 octet	   */

#define in_ss1	   (sshift_condition & IN_SS1_FLG)
#define in_ss2	   (sshift_condition & IN_SS2_FLG)
#define in_ss3	   (sshift_condition & IN_SS3_FLG)

#define in_ls1	   (shift_condition & IN_LS1_FLG)
#define in_ls2	   (shift_condition & IN_LS2_FLG)
#define in_ls3	   (shift_condition & IN_LS3_FLG)
#define in_rs1	   (shift_condition & IN_RS1_FLG)
#define in_rs2	   (shift_condition & IN_RS2_FLG)
#define in_rs3	   (shift_condition & IN_RS3_FLG)

#define in_locking_shift	(shift_condition & 0x000000ffUL)
#define in_left_locking_shift	(shift_condition & 0x0000000fUL)
#define in_right_locking_shift	(shift_condition & 0x000000f0UL)
#define in_single_shift		(sshift_condition & 0x00030f00UL)

#define set_ls1	   shift_condition = \
	(shift_condition & 0xffffff0fUL) | IN_LS1_FLG
#define set_ls2	   shift_condition = \
	(shift_condition & 0xffffff0fUL) | IN_LS2_FLG
#define set_ls3	   shift_condition = \
	(shift_condition & 0xffffff0fUL) | IN_LS3_FLG
#define res_ls1	   shift_condition &= 0xfffffff0UL
#define res_ls2	   shift_condition &= 0xfffffff0UL
#define res_ls3	   shift_condition &= 0xfffffff0UL

#define set_rs1	   shift_condition = \
	(shift_condition & 0xffffff0fUL) | IN_RS1_FLG
#define set_rs2	   shift_condition = \
	(shift_condition & 0xffffff0fUL) | IN_RS2_FLG
#define set_rs3	   shift_condition = \
	(shift_condition & 0xffffff0fUL) | IN_RS3_FLG
#define res_rs1	   shift_condition &= 0xffffff0fUL
#define res_rs2	   shift_condition &= 0xffffff0fUL
#define res_rs3	   shift_condition &= 0xffffff0fUL

#define set_ss1	   sshift_condition |= IN_SS1_FLG
#define set_ss2	   sshift_condition |= IN_SS2_FLG
#define set_ss3	   sshift_condition |= IN_SS3_FLG

#define res_ss1	   sshift_condition &= (0xffffffffUL ^ IN_SS1_FLG)
#define res_ss2	   sshift_condition &= (0xffffffffUL ^ IN_SS2_FLG)
#define res_ss3	   sshift_condition &= (0xffffffffUL ^ IN_SS3_FLG)

#define is_macl    (sshift_condition & IN_MACROL_FLG)
#define is_macr    (sshift_condition & IN_MACROR_FLG)
#define set_macl   sshift_condition |= IN_MACROL_FLG
#define set_macr   sshift_condition |= IN_MACROR_FLG
#define res_macl   sshift_condition &= (0xffffffffUL ^ IN_MACROL_FLG)
#define res_macr   sshift_condition &= (0xffffffffUL ^ IN_MACROR_FLG)

#define	res_single_shift sshift_condition = 0UL
#define	res_right_locking_shift shift_condition &= 0xffffff0fUL
#define	res_all_shift shift_condition &= 0xf0000000UL
#define	res_left_shift shift_condition &= 0xf00f00f0UL
#define	res_right_shift shift_condition &= 0xf00f000fUL
#define	res_locking_shift shift_condition &= 0xf000ff00UL
#define	res_alt_shift shift_condition &= 0xf0000fffUL

void shift_cond_recovery();

/* --------------------------------------------------------------- */
/* various utilities elsewhere 					   */
/* --------------------------------------------------------------- */
extern void	keis_conv P_((int, int));
extern void	jef_conv P_((int, int));
#if 0
extern void	is_sbroken P_((int, int));
#endif

extern int	x0201conv P_((int, int));
extern unsigned short x0201rconv P_((skf_ucode));

extern int	latin2html P_((skf_ucode));
extern int	latin2tex P_((skf_ucode));
#if 0
extern int	latin2htmlcnt P_((skf_ucode));
extern int	latin2texcnt P_((skf_ucode));
#endif

extern int	is_in_encoded P_(());

extern int	arib_macro_process P_((skfFILE *));
extern int	arib_macro_rawproc P_((int *,int,int));
extern int	arib_rpc_process P_((skfFILE *));

extern void nec_g0_kanjiset();	/* set x-0208 kanji to g0	   */
extern void nec_g0_kanaset();  	/* set x-0201 kana to g0	   */

extern short	in_keis;	/* KEIS83 mode in		   */
/* --------------------------------------------------------------- */
/* table of tables						   */
/* --------------------------------------------------------------- */
#ifndef FAST_MULT
extern int   mul94[];
#endif
#if 0
extern int   is_octet;		/* '1' means octet code set	   */
#endif

extern int   low_dbyte;		/* lower side double byte(left)	   */
extern int   up_dbyte;		/* upper side double byte(right)   */
extern unsigned long  low_kana;
extern unsigned long  up_kana;

extern unsigned short *low_table; /* lower side conversion table   */
extern unsigned short *up_table;  /* upper side conversion table   */
extern skf_ucode *low_ltable;	  /* lower side UCS4 table	   */
extern skf_ucode *up_ltable;	  /* upper side UCS4 table	   */

extern int   low_table_limit;	  /* table size			   */
extern int   up_table_limit;

extern struct iso_byte_defs *low_table_mod; /* lower table mode	   */
extern struct iso_byte_defs *up_table_mod;  /* upper table mode	   */

extern struct iso_byte_defs *g0_table_mod;  /* g0 table mode	   */
extern struct iso_byte_defs *g1_table_mod;  /* g1 table mode	   */
extern struct iso_byte_defs *g2_table_mod;  /* g2 table mode	   */
extern struct iso_byte_defs *g3_table_mod;  /* g3 table mode	   */
extern struct iso_byte_defs *gx_table_mod;  /* X-0213 alternate	   */

extern struct iso_byte_defs *c0_table_mod;
extern struct iso_byte_defs *c1_table_mod;

extern struct iso_byte_defs *pre_single_g0_table;
extern struct iso_byte_defs *pre_single_g1_table;
extern struct iso_byte_defs *pre_single_g2_table;
extern struct iso_byte_defs *pre_single_g3_table;

extern unsigned short uni_k_ibm_fa[];

extern const char dakuten[];
extern const int x213_sjis_map[];
extern unsigned short x201_uni_byte[];

extern unsigned short ebcdik40[];	/* EBCDIK Hitachi	   */
extern unsigned short ebcdic40[];	/* EBCDIC IBM		   */
extern unsigned short ebcdicn40[];	/* EBCDIC NEC		   */
extern unsigned short ebcdics40[];	/* EBCDIC latin		   */
extern unsigned short ebcdic41[];	/* EBCDIC IBM933	   */
extern unsigned short ebcdic42[];	/* EBCDIC IBM935	   */
extern unsigned short ebcdic43[];	/* EBCDIC IBM937	   */
extern unsigned short ebcdic44[];	/* EBCDIC IBM037	   */

extern unsigned short iscii_attrib_table[];

#define up_has_tbl  ((is_tbllong(up_dbyte) && (up_ltable != NULL)) || \
		     (is_tblshort(up_dbyte) && (up_table != NULL)))

extern int is_g0_table_x0201 P_(());

/* --------------------------------------------------------------- */
/* dynamic table loading controls				   */
/* --------------------------------------------------------------- */
extern short DBCS_host_index;
extern short JEF_host_index; 
extern short KEIS_host_index; 

extern int load_external_table P_((struct iso_byte_defs *));
extern int in_codeset_preload();
extern unsigned short *input_get_dummy_table();
extern skf_ucode *input_get_dummy_ltable();
extern int test_primary_codeset();
extern int paraphrase_arib_macro P_((int));
extern int is_charset_macro P_((struct iso_byte_defs *));

extern struct iso_byte_defs *get_jisx0213_1_table();

extern void code_table_fix();

#ifdef DYNAMIC_LOADING
extern char	*skf_table_magic;
extern char	*skf_table_lmagic;
#endif

/* --------------------------------------------------------------- */
/* dynamic table loading size					   */
/* --------------------------------------------------------------- */
#define KEIS_TBL_LEN	3760
#define JEF_TBL_LEN	5734
#define JIS78_TBL_LEN	8836
#define JISX0213_Q_TBL_LEN	9025
#define KRX1001_TBL_LEN	8836
#define GB2312_TBL_LEN	8836
#define GB12345_TBL_LEN	8836
#define CP932932_PATCH_LEN 376	/* 94 * 4	*/

#define BIG5_TBL_LEN	17860	/* 190 * 94 */
#define BIG5P_TBL_LEN	23940	/* BIG5-Plus 190 * 126 */
#define BIG5_OFFSET	6080	/* 0x8140 - 0xa0fe */
#define GBK_TBL_LEN	23940	/* 190 * 126 */
#if defined(SHORTEN_GB2K)
#define GB2K_A_LEN	25940
#else
#define GB2K_A_LEN	32760
#endif
#define IBMDBCS_TBL_LEN	11970	/* IBM DBCS (ibm-930,931,935)	   */
#define IBMDBCSL_TBL_LEN 30970	/* IBM Large DBCS (ibm-933,937)	   */
#define JOHAB_TBL_LEN	15040	/* johab hangul part only	   */
#define HKSCS_PATCH_LEN 3304	/* code in area CJK Extension BC   */

#define EMOJI_TBL_LEN	2444	/* f040 - fcfc (in SJIS)	   */

#define JOHAB_KOFFSET	41	/* Kanji starts at ROW 42	   */

/* --------------------------------------------------------------- */
/* Unic*de decomposition/composition table definitions		   */
/* --------------------------------------------------------------- */
/* #define UNI_NKDL_TBL_LEN 13376 :* 0x00a0-0x33ff,0xf900-0xffff   */
#define UNI_NKDL_TBL_LEN 20480	/* 0x00a0-0x33ff,0xf900-0xffff     */
				/* 0x1d100-0x1d7ff,0x2f800-0x2fa20 */
				/* KAITHI, ARAM, ESCP etc.	   */

#define UNI_NORM_TBL_LEN 20480	/* about (UNI_TAIL_OFF) * 1.1	   */

#define UNI_LAT_OFF	160	/* 0x00a0			   */
#define UNI_UP_OFF	13152	/* 0x33ff - 0x00a0		   */
#define UNI_COM_OFF	14944	/* UP_OFF + (0xf900 - 0xffff)	   */
#define UNI_CJKC_OFF	16736	/* COM_OFF + (0x1d100 - 0x1d7ff)   */
#define UNI_KAITHI_OFF	17280	/* CJKC_OFF + (0x2f800 - 0x2fa1f)  */
#define UNI_ARAM_OFF	18672	/* KAITHI_OFF + (0x11090 - 0x11600)*/
#define UNI_ECSP_OFF	18928	/* ARAM_OFF + (0x1ee00 - 0x1ef00)  */
#define UNI_TAIL_OFF	19696	/* ECSP_OFF + (0x1f100 - 0x1f400)  */
#define UNI_ATTRIB_TBL_LEN 65536

extern unsigned short	*nkdc_lowptr;	/* area mapper		   */
extern skf_ucode	*nkdc_lowmap;	/* map char table (in BSP) */
extern unsigned short	*nkd_strmap;	/* map for compose strength*/
extern int 		nkdc_lowsize;	/* map char table length   */

#define UNIHAN_SBASE	0xac00	/* Hangul start point		   */
#define UNIHAN_LBASE	0x1100
#define UNIHAN_VBASE	0x1161
#define UNIHAN_TBASE	0x11a7
#define UNIHAN_LCOUNT	19
#define UNIHAN_VCOUNT	21
#define UNIHAN_TCOUNT	28

#define ARIB_MACRO_LIMIT	128
#define ARIB_RPC_MGN	6

/* --------------------------------------------------------------- */
#if defined(ROT_SUPPORT) && defined(NEW_ROT_CODE)
extern int skf_rot47conv_d P_((int));
extern int skf_rot13conv_d P_((int));
#endif

extern void dump_table_address P_((struct iso_byte_defs *,char *));
/* --------------------------------------------------------------- */
/* internationalized IDN decoders				   */
/* --------------------------------------------------------------- */
#ifdef ACE_SUPPORT
/* for punycode encoder/decoder --- */
#define PUNY_I_BIAS	72
#define PUNY_DELIM	0x2d
#define PUNY_BASE	36
#define PUNY_SKEW	38
#define PUNY_INIT_N	0x80
#define PUNY_DUMP	700
#define PUNY_TMIN	1
#define PUNY_TMAX	26
#define PUNY_MAXINT	0x7fffffff
#define PUNY_BUFLEN	240	/* 256 - 16 for margin		   */

/* Note -- ietf draft ACE prefix definitions ---
   xn--		punycode	supported
   bq--		RACE		recognized but not supported
   lq--		LACE
   bl--		DUNCE
   wq--		UTF-6
*/
#define PUNY_PRFX1	'x'
#define PUNY_PRFX2	'n'
#define RACE_PRFX1	'b'
#define RACE_PRFX2	'q'

#define is_puny_basic(x) ((punycode_uint)(x) < 0x80)
#define is_puny_delim(x) ((x == PUNY_DELIM) || (x == '.') || (x <= 0x20))
#define is_puny_strdelim(x) ((x == '.') || (x <= 0x20))

#define punycode_uint	skf_ucode
#define punycode_success	0
#define punycode_bad_input	-1
#define punycode_big_output	-2
#define punycode_overflow	-3
#define punycode_woverflow	-4
#define punycode_lencheck	-5
#define punycode_lenover	-6
#define punycode_unexpeof	-7

extern skf_ucode puny_adapt P_ ((long, long, int));
extern int	in_ace;
#endif

#define ARIB_PREDEF_MACRO_CNT	16
#define ARIB_PREDEF_MACRO_IDX	0x60
#ifdef ARIB_FULL_MACRO_DEF
#define ARIB_PREDEF_MACRO_CHAR	326
#else
#define ARIB_PREDEF_MACRO_CHAR	71
#endif

/* --------------------------------------------------------------- */
/* Vietnamese							   */
/* --------------------------------------------------------------- */
#define is_viqr_ch3(x,y) (((x == 'a') && (y == '(')) || \
			  ((x == 'A') && (y == '(')) || \
			  ((x == 'a') && (y == '^')) || \
			  ((x == 'A') && (y == '^')) || \
			  ((x == 'e') && (y == '^')) || \
			  ((x == 'E') && (y == '^')) || \
			  ((x == 'o') && (y == '^')) || \
			  ((x == 'O') && (y == '^')) || \
			  ((x == 'o') && (y == '+')) || \
			  ((x == 'O') && (y == '+')) || \
			  ((x == 'u') && (y == '+')) || \
			  ((x == 'U') && (y == '+')))

#define is_vimn_ch3(x,y) (((x == 'a') && (y == '<')) || \
			  ((x == 'A') && (y == '<')) || \
			  ((x == 'a') && (y == '>')) || \
			  ((x == 'A') && (y == '>')) || \
			  ((x == 'e') && (y == '>')) || \
			  ((x == 'E') && (y == '>')) || \
			  ((x == 'o') && (y == '>')) || \
			  ((x == 'O') && (y == '>')) || \
			  ((x == 'o') && (y == '*')) || \
			  ((x == 'O') && (y == '*')) || \
			  ((x == 'u') && (y == '*')) || \
			  ((x == 'U') && (y == '*')))
/* --------------------------------------------------------------- */
#endif
