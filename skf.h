/* ------------------------------------------------------------------
** Copyright (c) 1993-2015 Seiji Kaneko. All rights reserved.
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
********************************************************************

    skf.h:		common definitions
	v1.30		first written for skf v1.30
	v1.90 		written for skf v1.9
    $Id: skf.h,v 1.180 2017/01/05 15:05:48 seiji Exp seiji $
*/

#ifndef		INCLUDE_SKFDEFS

#include	"config.h"
#define		INCLUDE_SKFDEFS
#include	"config.h"

/* -------------------------------------------------------------- */
/* prototype convension 					  */
/* -------------------------------------------------------------- */
#if defined(__STDC__) || defined(WINDOWSNT)
#ifndef P_
#define P_(proto) proto
#endif
#define STATIC static
#else
#ifndef P_
#define P_(proto) ()
#endif
#define STATIC 
#endif
/* -------------------------------------------------------------- */
/* CONSTANTS							  */
/* -------------------------------------------------------------- */

#if !defined(FALSE)
#define         FALSE   0
#define         TRUE    1
#endif

#define		FULLONE 0xffffffffUL

/* Various ascii codepoint */
#define         A_BEL	0x07
#define         A_BS	0x08
#define		A_HT	0x09
#define         A_LF	0x0a
#define         A_FF	0x0c
#define         A_CR	0x0d
#define         A_SO	0x0e
#define         A_SI	0x0f
#define         A_CAN	0x18
#define         A_SUB	0x1a
#define         A_ESC	0x1b
#define         A_SS	0x1c	/* Super Shift for NATS		   */
#define         A_SP	0x20
#define		A_BANG	0x21
#define         A_COMMA	0x2c
#define         A_DOT	0x2e
#define         A_0	0x30
#define         A_BRA	0x3c
#define         A_KET	0x3e
#define         A_AT	0x40
#define         A_CAPA	0x41
#define         A_CAPZ	0x5a
#define		A_USCORE 0x5f
#define         A_SMLA	0x61
#define         A_SMLZ	0x7a
#define         A_KSP	0xa0
#define         A_DEL	0x7f
#define         A_COL	':'
#define         A_YEN	0x5c
#define		A_2SS2  0x4e
#define		A_2SS3	0x4f
#define         A_SS2	0x8e    /* single shift 2 in 8-bit coded   */
#define         A_SS3	0x8f    /* single shift 3 in 8-bit coded   */
#define		A_IATR	0xef	/* ISCII attribute		   */
#define		A_IEXT	0xf0	/* ISCII extension		   */
#define		A_IINV	0xe9	/* ISCII Invert char		   */

#define		A_MS2E	0xfc	/* MS SJIS 2nd char upper limit	   */
#define		A_B5E	0xf9	/* BIG5 1st char upper limit	   */
#define		A_B52E	0xfe	/* BIG5 2nd char upper limit	   */
#define		A_B5KS	0xa4	/* BIG5 1st kanji area start	   */
#define		A_B5ML	0xc6	/* BIG5 1st Level 1 upper limit	   */
#define		A_B5MU	0xc9	/* BIG5 1st Level 2 lower limit	   */
#define		A_DAK	0xde	/* X 0201 dakuten		   */
#define		A_HDK	0xdf	/* X 0201 han-dakuten		   */
#define		KANA_END 0xdf
#define		MS_END	0xea	/* maximum euc's first character   */
#define		ASCII_END 0x80
#define		A_KDEL	0xff
#define		MS_GAIJI 0xf0
#define		BOM_HIGH 0xff
#define		BOM_LOW 0xfe

/* X-0208 assignment						   */
#define		A_MAX	0x28	/* maximum alpha code		   */
#define		K_ST	0x30
#define		K_AKIH	0x4f	/* gap between 1st-2nd suijyun	   */
#define		K_AKIL	0x63

#define		KEIS_3	0x59	/* KEIS83 Exp Set 3 start point	   */
#define		KEIS_3E	0x80	/* KEIS83 Exp Set 3 end point	   */
#define		KEIS_S	0xA1	/* KEIS83 standard character set   */
#define		KEIS_2  0xAF	/* KEIS83 Exp Set 3 tobichi	   */
#define		KEIS_1	0xB0	/* KEIS83 JIS-KANJI		   */
#define		KEIS_SMM 0x0A	/* KEIS83 SMM			   */
#define		KEIS_SI	0x41	/* KEIS83 SHIFTOUT MARK		   */
#define		KEIS_SO	0x42	/* KEIS83 SHIFTIN MARK		   */
#define		KEIS_JEF_SI 0x29 /* JEF,DBCS-HOST Shiftout	   */
#define		KEIS_JEF_SO 0x28 /* JEF,DBCS-HOST Shiftin	   */
#define		KEIS_IBM_SI 0x0f /* IBM-DBCS-HOST Shiftout(to SB)  */
#define		KEIS_IBM_SO 0x0e /* IBM-DBCS-HOST Shiftin(to MB)   */
#define		KEIS_SP 0x40 	/* KEIS SPACE			   */
#define		JEF_E1	0x41	/* JEF low area start		   */
#define		JEF_E1E	0x7F	/* JEF low area end		   */
#define		IBMDBCS_S 0x41	/* IBM DBCS start point		   */
#define		IBMDBCS_E 0xd3	/* IBM DBCS end point		   */

#define		KR_KANA  0xaa
#define		KR_CYL	0xab
#define		GB_KANA  0xa4
#define		GB_CYL	0xa7

/* arib controls */
#define		AR_MAC  0x95
#define		AR_RPC  0x98

#define		X0208_3RD 0x4f	
#define		X0208_4TH 0x50	

#define		NCU	0x22
#define		NCL	0x2e

#define		U_BLANK 0x3000
#define		U_PRIV	0xe000
#define 	U_SUR	0xd800
#define		U_COMP	0xf900

#define		U_SRMSK	0x03ffU

#define		UNC	0x3013
#define		U_HTTN	0xff61
#define		U_HBRA	0xff62
#define		U_HKET	0xff63
#define		U_HKTN	0xff64
#define		U_KTTN  0x3002
#define		U_KKTN  0x3001
#define		U_KBRA  0x300c
#define		U_KDAK  0x3099
#define		U_KHDK  0x309a
#define		U_LDAK  0x309B
#define		U_LHDK  0x309c
#define		U_CDAK	0xff9e
#define		U_CHDK	0xff9f
#define		U_CKAN	0xff71
#define		U_CKANE	0xff9d

/* UTF-8 handling constants -------------------------------------- */
#define		ZL23MSK	0x07c0U
#define		ZL2MSK	0x0fc0U
#define		ZL3MSK	0xf000U
#define		ZL4MSK	0x1c0000UL
#define		ZL43MSK	0x03f000UL
#define		ZF2MSK	0xe0U
#define		ZF2VAL	0x80
#define		ZF3MSK	0xf0U
#define		ZF3VAL	0xe0
#define		ZF4VAL	0xf0
#define		ZF4MSK	0xf8U
#define		ZF4VAV	0xf8
#define		ZF23MSK	0xc0U
#define		ZF23VAL	0xc0
#define		ZCMSK	0xc0U
#define		Z1MSK	0x3fU
#define		Z23MSK	0x1fU
#define		Z3MSK	0x0fU
#define		Z4MSK	0x07U
#define		Z43MSK	0x3fU
#define		ZCVAL	0x80

/* UTF-7 handling constants -------------------------------------- */
#define		Y0MSK	0x3f00U
#define		Y1MSK	0x03f0U
#define		Y2MSK	0x003fU
#define		Y3MSK	0x0003U
#define		Y0AMSK	0xfc00U
#define		Y1AMSK	0x0fc0U
#define		Y2AMSK	0x00fcU
#define		Y3AMSK	0x000fU

/* HZ code handling	-----------------------------------------  */
#define		HZ_ANN	0x7e
#define		HZ_ENT	0x7b
#define		HZ_END	0x7d

#define		ZW_ANN	0x7a
#define		ZW_ENT  0x57
#define		ZW_LF	0x23
#define		ZW_ESC	0x20

/* MIME and encode delimiters -----------------------------------  */
#define CAP_DEL		':'	/* delimiters			   */
#define URI_DEL		'%'
#define PERC_DEL	'%'
#define EQU_DEL		'='
#define QU_DEL		'?'
#define BSL_DEL		0x5c	/* back slash */

/* misc code handling	-----------------------------------------  */
#define		LBMSK	0x0ffU
#define		HBMSK	0xff00U

/* --------------------------------------------------------------  */
#define RECURSION_LIMIT	32
/* --------------------------------------------------------------  */
/* internal types						   */
/* --------------------------------------------------------------  */

/* skf_ucode: can hold at least 20bit signed, must be efficient.   */

typedef int	skf_ucode;
/* typedef long	skf_ucode; 	:* for 16bit int machine	   */

/* --------------------------------------------------------------  */
/* utilities							   */
/* -------------------------------------------------------------- */
/* in_code_table.c */
extern void	uni_table_init();

extern void 	skf_codeset_parser P_((int));
extern void 	skf_charset_parser P_((int));

/* --------------------------------------------------------------  */
/* Global states						   */
/* -------------------------------------------------------------- */
extern char	ucs_tagstr[];	/* Unic*de tag string		   */

extern int	fold_clap;	/* fold line limit		  */
extern int	fold_fclap;	/* fold line limit (force)	  */
#if 0
extern int	fold_nclap;	/* fold line limit (force)	  */
#endif
extern int	fold_omgn;	/* fold line oidashi margin	  */

/* -------------------------------------------------------------- */
/* detected code table						  */
/* -------------------------------------------------------------- */
/* bit assignment for conv_cap, in_code(preconvert.c) and	  */
/*  i_codeset.encode						  */
/* -------------------------------------------------------------- */
extern unsigned long	skf_given_lang;

/* --------------------------------------------------------------- */
/* capability packed word					   */
/*  these two variables control output side main features in skf   */
/* --------------------------------------------------------------- */
extern unsigned long conv_cap;	 /* defined in skf.c		   */
extern unsigned long conv_alt_cap; /* defined in skf.c		   */

/* --------------------------------------------------------------- */
/* codeset features 0x*0000000 (not related to output)		   */
/* --------------------------------------------------------------- */
#define COD_HIDE  0x40000000UL	/* do not show as supported code   */
#define is_codehide(x) (COD_HIDE & x)

#define SKF_COD_MB  0x20000000UL /* derived output table property  */
			/* Note: it is NOT what the codeset is.    */
/* --------------------------------------------------------------- */
/* command parameter not commonly used with codeset		   */
/* --------------------------------------------------------------- */
#define QUAD_CHAR	0x40000000UL
#define set_quad_char	conv_cap |= QUAD_CHAR
#define is_quad_char	(conv_cap & QUAD_CHAR)

extern short	debug_opt;
#define set_vvv_debug	debug_opt = 3
#define set_vv_debug	debug_opt = 2
#define set_v_debug	debug_opt = 1
#define is_vvv_debug	(debug_opt > 2)
#define is_vv_debug	(debug_opt > 1)
#define is_v_debug	(debug_opt > 0)
#define res_debug	debug_opt = 0

/* --------------------------------------------------------------- */
/* codeset features 0x0**00000 					   */
/* --------------------------------------------------------------- */
#define SKF_COD_V_MSK	0x0fffffffUL /* codeset value mask	   */
#define SKF_COD_NV_MSK	0xfff00000UL /* codeset value revert mask  */
#define set_ocode_codeset(x) conv_cap = \
	((conv_cap & SKF_COD_NV_MSK) | (x & SKF_COD_V_MSK))
#define res_out_code  conv_cap &= SKF_COD_NV_MSK
/* --------------------------------------------------------------- */
/* --- kana control features ------------------------------------- */
/* controlled by conv_alt_cap bit 11-8				   */
/* bit 11-8	0: no x-0201 kana feature. do not output them	   */
/*		1: use by ^[(I kana-call			   */
/*		2: use by SI/SO shift-in/out feature		   */
/*		3: use by output 8bit code directly		   */
/*		4: use by output compatibility plane (unic*de)	   */
/*		5: use by SS2 single shift (euc-JP)		   */
/*		6-15: reserved					   */
/* --------------------------------------------------------------- */
#define	HK_ENBL	  0x00c00000UL	/* x0201 kana fac.		   */
#define	hk_enbl	(conv_cap & HK_ENBL)

#define KANA_CALL 0x00400000UL	/* kana-call by ^[(I		   */
#define kana_call ((conv_cap & HK_ENBL) == KANA_CALL)
#define SI_ENBL   0x00800000UL	/* shift in-out capable		   */
#define si_enbl ((conv_cap & HK_ENBL) == SI_ENBL)
#define EIGHTBIT  0x00c00000UL	/* 8-bit mode			   */
#define eight_bit ((conv_cap & HK_ENBL) == EIGHTBIT)
#define USE_SS2   0x00400000UL	/* use single shift		   */
#define ss2_enbl ((conv_cap & HK_ENBL) == USE_SS2)
#define HAVE_HHK  0x00c00000UL	/* code include half-width variant */

#define HK_DSBL_MSK 0xff3fffffUL

#define no_x0201kana ((conv_cap & HK_DSBL_MSK) == 0)

#define set_kana_call conv_cap = (conv_cap & HK_DSBL_MSK) | KANA_CALL
#define set_si_enbl conv_cap = (conv_cap & HK_DSBL_MSK) | SI_ENBL
#define set_eightbit conv_cap = (conv_cap & HK_DSBL_MSK) | EIGHTBIT
#define reset_hk_enbl conv_cap = conv_cap & HK_DSBL_MSK

#define set_put_hk_enbl(x) conv_cap = (HK_DSBL_MSK & conv_cap) | x

/* --------------------------------------------------------------- */
/* common selectors 0x000**000					   */
/* --------------------------------------------------------------- */
#define SKF_OFLG_ENBL	0x00080000UL  /* has flagged features	   */
#define SKF_USE_8BIT	0x00040000UL  /* use upper side table	   */
#define SKF_HAS_GAIJI	0x00020000UL  /* has NEC gaiji area	   */
#define SKF_MS_COMPAT	0x00010000UL  /* has ms-compatible map	   */

#define use_ms_compat (conv_cap & SKF_MS_COMPAT)
#define set_use_ms_compat conv_cap |= SKF_MS_COMPAT
#define res_use_ms_compat conv_cap &= 0xfffbffffUL

#define SKF_HAS_PRIVATE 0x00008000UL /* has map in private area	   */
#define is_cod_has_private(x) (x & SKF_HAS_PRIVATE)
#define set_cod_has_private conv_cap |= SKF_HAS_PRIVATE
#define SKF_NOT_ASCII   0x00004000UL /* not including ascii	   */
#define is_cod_not_ascii(x)   (x & SKF_NOT_ASCII)
/* --------------------------------------------------------------- */
/* various codeset related flags				   */
/* --------------------------------------------------------------- */
/* !UCS */
#define SKF_MS_WCHAR	0x00000800UL /* has ms-compat wchar conv.  */
#define SKF_ADD_ANNON	0x00000200UL /* add announcer seq.	   */
/* X-0212 and others */
#define	SKF_USE_X0212 	0x00200000UL /* X-0212 feature enable	   */
#define	SKF_USE_ISO8859	0x00100000UL /* iso-8859-1 feature enable  */

#define	use_x0212 (conv_cap & SKF_USE_X0212)
#define	set_use_x0212 conv_cap |= SKF_USE_X0212
#define	res_use_x0212 conv_cap &= 0xffffdfffUL

#define	use_iso8859 (conv_cap & SKF_USE_ISO8859)
#define	set_use_iso8859 conv_cap |= SKF_USE_ISO8859
#define	res_use_iso8859 conv_cap &= 0xffffefffUL

#define add_annon (conv_cap & SKF_ADD_ANNON)
#define set_add_annon conv_cap |= SKF_ADD_ANNON
#define res_add_annon conv_cap &= 0xfffffdffUL

/* UCS */
#define SKF_DBL_LATIN	0x00200000UL /* 0x3400- area code switch   */
#define SKF_ADD_BOM	0x00100000UL /* add BYTE ORDER MARK	   */
#define SKF_ENBL_ECMP	0x00000800UL /* Unicode encomposition cntl */
#define SKF_ENBL_DCMP	0x00000400UL /* Unicode decomposition cntl */
#define SKF_BIG_ENDIAN	0x00000200UL /* big endian marker	   */
#define SKF_LTL_ENDIAN	0x00000100UL /* little endian marker	   */
#define SKF_BIG_ENDIAN_MASK 0xfffffdffUL

#define o_enbl_dbl_latin (conv_cap & SKF_DBL_LATIN)
#define o_enbl_encomp	 (conv_cap & SKF_ENBL_ECMP)
#define o_enbl_decomp	 (conv_cap & SKF_ENBL_DCMP)
#define o_add_bom	 (conv_cap & SKF_ADD_BOM)
#define set_o_add_bom conv_cap |=  SKF_ADD_BOM
#define res_o_add_bom conv_cap &= 0xffefffffUL 

/* --------------------------------------------------------------- */
/* codeset designation 0x000000**				   */
/* --------------------------------------------------------------- */
#define SKF_COD_C_MSK	0x000000f0UL /* code categories mask	   */
#define SKF_COD_S_MSK	0x0000000fUL /* code specifics mask	   */
#define SKF_COD_O_MSK	0x000000ffUL /* categories and specifics   */
#define SKF_COD_FLG_MSK 0x00f00f00UL /* code specifics flags	   */

#define SKF_COD_J_MSK   0x000000f7UL /* categories and specifics   */
#define SKF_COD_CAT_MSK 0x000000c0UL /* rough categolize	   */
				/* beware bit assigns of cat_msk   */
#define SKF_COD_UC_MSK  0x000000fcUL /* same categories		   */
#define SKF_COD_U8_MSK  0x000000feUL /* same categories		   */
#define SKF_COD_UD_MSK  0x000000fdUL /* same categories		   */
#define SKF_COD_213_MSK	0x000000efUL /* x0213 common mask	   */
#define SKF_COD_LL_MSK	0xfffffff0UL /* LOW 1BYTE MASK		   */

/* --------------------------------------------------------------- */
/* category selectors 0x000000*0				   */
/* --------------------------------------------------------------- */
#define SKF_EUC7	0x0000UL    /* EUC 7bit style codes	   */
#define SKF_JIS		0x0010UL    /* iso-2022-jp style codes	   */
#define SKF_EUC		0x0020UL    /* EUC style codes		   */
#define SKF_UCS		0x0040UL    /* output is Unic*de	   */
#define SKF_MS		0x0080UL    /* MS jis style codes	   */
#define SKF_BIG5	0x0090UL    /* BIG5 style codes		   */
#define SKF_GBKR	0x00a0UL    /* GBKR-derivative style codes */
#define SKF_NNISO	0x00c0UL    /* Various misc code sets	   */
#define SKF_KEIS 	0x00e0UL    /* output is kind of KEIS	   */

/* --------------------------------------------------------------- */
/* misc selectors 0x0000000*					   */
/* --------------------------------------------------------------- */
/* 1st set: SKF_EUC7						   */
/* --------------------------------------------------------------- */
#define is_euc7(x)  ((x & SKF_COD_C_MSK) == SKF_EUC7)
#define SKF_EUC7_V	0x0001UL /* normal EUC7 (==0 is invalid)   */

#define SKF_COD_INVALID (0x0000UL | SKF_EUC7)
#define is_code_invalid(x) ((x & SKF_COD_O_MSK) == SKF_COD_INVALID)
/* --------------------------------------------------------------- */
/* 2nd set: SKF_EUC						   */
/* --------------------------------------------------------------- */
#define SKF_EUC_CDE	0x0001UL  /* TOG/JVC CDE(eucJP-ms)	   */
#define SKF_EUC_51932	0x0002UL  /* euc with cp932'd		   */
#define SKF_EUC_20932	0x0003UL  /* enh.euc with cp932'd	   */
#define SKF_EUC_213	0x0004UL  /* JIS X-0213(2000)		   */
#define SKF_EUC_213N	0x0005UL  /* JIS X-0213(2004)		   */
#define SKF_EUC_CNS	0x0008UL  /* CNS 11643			   */
#define SKF_EUC_TCN	0x0009UL  /* iso-2022-cn-cns		   */
#define SKF_EUC_GCN	0x000aUL  /* iso-2022-cn-gb2312		   */
#define SKF_EUC_NISO	0x000fUL  /* euc without ISO calling char  */

#define is_euc(x) ((x & SKF_COD_C_MSK) == SKF_EUC)

#define is_euc_eucjp(x) ((x & SKF_COD_O_MSK) == SKF_EUC)
#define is_euc_eucms(x) ((x & SKF_COD_O_MSK)==(SKF_EUC|SKF_EUC_CDE))
#define is_euc_51932(x) ((x & SKF_COD_O_MSK) == \
			  (SKF_EUC | SKF_EUC_51932))
#define is_euc_20932(x) ((x & SKF_COD_O_MSK) == \
			  (SKF_EUC | SKF_EUC_20932))
#define is_euc_mseuc(x) ((x & SKF_COD_U8_MSK) == \
			  (SKF_EUC | SKF_EUC_51932))
#define is_euc_cns(x) ((x & SKF_COD_O_MSK) == (SKF_EUC|SKF_EUC_CNS))
#define is_euc_twcn(x) ((x & SKF_COD_O_MSK)==(SKF_EUC|SKF_EUC_TCN))
#define is_euc_gbcn(x) ((x & SKF_COD_O_MSK)==(SKF_EUC|SKF_EUC_GCN))
#define is_euc_213(x) ((x & SKF_COD_O_MSK) == (SKF_EUC|SKF_EUC_213))
#define is_euc_213n(x) ((x & SKF_COD_O_MSK)==(SKF_EUC|SKF_EUC_213N))
#define is_euc_213c(x) ((x & SKF_COD_U8_MSK)==(SKF_EUC|SKF_EUC_213))
#define is_euc_niso(x) ((x & SKF_COD_O_MSK)==(SKF_EUC|SKF_EUC_NISO))
/* --------------------------------------------------------------- */
/* 3rd set: SKF_JIS						   */
/* --------------------------------------------------------------- */
#define SKF_JIS_932  0x0001UL	/* JIS with cp932 extension	   */
#define SKF_JIS_213  0x0004UL	/* JIS X-0213(2000)		   */
#define SKF_JIS_213N 0x0005UL	/* JIS X-0213(2004)		   */
#define SKF_JIS_8BIT 0x0008UL	/* 8bit iso-2022		   */
#define SKF_JIS_NATS 0x0009UL	/* Fin/Swed/Norsk/Dnmk NATS	   */
#define SKF_JIS_VAR  0x000bUL	/* Variable latin		   */
#define SKF_JIS_220  0x000cUL	/* MS cp50220			   */
#define SKF_JIS_221  0x000dUL	/* MS cp50221			   */
#define SKF_JIS_222  0x000eUL	/* MS cp50222			   */

#define is_jis_jis(x)  ((x & SKF_COD_O_MSK) == SKF_JIS)
#define is_jis_932(x) ((x & SKF_COD_J_MSK)==(SKF_JIS | SKF_JIS_932))
#define is_cp5022x(x) ((x & SKF_COD_UC_MSK)==(SKF_JIS | SKF_JIS_220))
#define is_jis_213c(x) ((x & SKF_COD_U8_MSK) == (SKF_JIS |SKF_JIS_213))

#define is_jis(x)  ((x & SKF_COD_C_MSK) == SKF_JIS)
#define set_jis(x)  x = (SKF_JIS | (x & SKF_COD_NV_MSK))

#define out_jis ((conv_cap & SKF_COD_C_MSK) == SKF_JIS)
#define set_out_jis  conv_cap = (SKF_JIS | (conv_cap & SKF_COD_NV_MSK))

#define out_jiscat(x) ((x & SKF_COD_CAT_MSK) == 0)
#define is_jiscat(x) ((x & SKF_COD_CAT_MSK) == 0)

#define is_jis_var(x)  ((x & SKF_COD_O_MSK)==(SKF_JIS|SKF_JIS_VAR))
/* --------------------------------------------------------------- */
/* 4th set: SKF_UCS						   */
/* --------------------------------------------------------------- */
#define SKF_UCS_UTF16	0x0000UL    /* BASE utf-16 		   */
#define SKF_UCS_UTF32	0x0002UL    /* BASE utf-32 		   */
#define SKF_UCS_UTF8	0x0004UL    /* BASE utf-8		   */
#define SKF_UCS_UTF7	0x0006UL    /* BASE utf-7		   */
#define SKF_UCS_PUNY	0x0008UL    /* BASE IDN Punycode	   */
#define SKF_UCS_URI	0x0009UL    /* BASE IDN URI notation	   */
#define SKF_UCS_BRGT	0x000eUL    /* output is B/Right V	   */	
#define SKF_UCS_TRANS	0x000fUL    /* transparent mode		   */

#define is_ucs_utf16(x) ((x & SKF_COD_UC_MSK)==(SKF_UCS|SKF_UCS_UTF16))
#define is_ucs_utf32(x) ((x & SKF_COD_O_MSK)==(SKF_UCS|SKF_UCS_UTF32))
#define is_ucs_utf8(x) ((x & SKF_COD_O_MSK)==(SKF_UCS|SKF_UCS_UTF8))
#define is_ucs_utf7(x) ((x & SKF_COD_O_MSK)==(SKF_UCS|SKF_UCS_UTF7))
#define is_ucs_brgt(x) ((x & SKF_COD_O_MSK)==(SKF_UCS|SKF_UCS_BRGT))
#define is_ucs_puny(x) ((x & SKF_COD_O_MSK)==(SKF_UCS|SKF_UCS_PUNY))
#define is_ucs_uri(x) ((x & SKF_COD_O_MSK)==(SKF_UCS|SKF_UCS_URI))
#define is_ucs_transp(x) ((x & SKF_COD_O_MSK)==(SKF_UCS|SKF_UCS_TRANS))
#define is_ucs_ufam(x) ((x & SKF_COD_C_MSK) == SKF_UCS)

#define out_endian(x) \
	(((x & SKF_COD_UC_MSK) == SKF_UCS) && (x & SKF_BIG_ENDIAN))
#define set_o_big_endian conv_cap |= SKF_BIG_ENDIAN
#define set_o_ltl_endian conv_cap &= SKF_BIG_ENDIAN_MASK

/* --------------------------------------------------------------- */
#define out_bg(x) (((x & SKF_COD_C_MSK) == SKF_BIG5) || \
		   ((x & SKF_COD_C_MSK) == SKF_NNISO) || \
		   ((x & SKF_COD_C_MSK) == SKF_GBKR))
#define out_ocat   ((conv_cap & 0x00000080UL) != 0)
/* --------------------------------------------------------------- */
/* 5th set: SKF_MS						   */
/* --------------------------------------------------------------- */
#define SKF_MS_932	0x0001UL  /* pure MS cp932 		   */
#define SKF_MS_943	0x0002UL  /* IBM cp943			   */
#define SKF_MS_APL	0x0003UL  /* Appl* K*njitalk 7,8,9	   */
#define SKF_MS_213	0x0004UL  /* JIS X-0213(2000)		   */
#define SKF_MS_213N	0x0005UL  /* JIS X-0213(2004)		   */
#define SKF_MS_NTT	0x000cUL  /* NT* i-m*de glyph mode	   */

#define is_msfam(x)  ((x & SKF_COD_C_MSK)== SKF_MS)
#define is_ms_kanas(x)  ((x & SKF_COD_C_MSK)== SKF_MS)
#define is_ms_932(x) ((x & SKF_COD_O_MSK)==(SKF_MS | SKF_MS_932))
#define is_ms_943(x) ((x & SKF_COD_O_MSK)==(SKF_MS | SKF_MS_943))
#define is_ms_apl(x) ((x & SKF_COD_O_MSK)==(SKF_MS | SKF_MS_APL))
#define is_ms_213(x) ((x & SKF_COD_O_MSK)==(SKF_MS | SKF_MS_213))
#define is_ms_213n(x) ((x & SKF_COD_O_MSK)==(SKF_MS | SKF_MS_213N))
/* 213c: x-0213:2000 or x-0213:2004	*/
#define is_ms_213c(x) ((x & SKF_COD_U8_MSK)==(SKF_MS | SKF_MS_213))
#define is_ms_cel(x) ((x & SKF_COD_O_MSK)==(SKF_MS | SKF_MS_NTT))

#define out_ms   ((conv_cap & SKF_COD_C_MSK) == SKF_MS)
#define set_out_ms  conv_cap = (SKF_MS | (conv_cap & SKF_COD_NV_MSK))
/* --------------------------------------------------------------- */
/* 6th set: SKF_BIG5						   */
/* --------------------------------------------------------------- */
#define SKF_B5_CP950	0x0001UL
#define SKF_B5_HKU	0x0003UL
#define SKF_B5_GCSS	0x0004UL  /* HKSCS & GCSS		   */
#define SKF_B5_BPLS	0x0005UL  /* BIG5 PLUS			   */
#define SKF_B5_2003	0x0006UL  /* BIG5 2003		 	   */
#define SKF_B5_UAO	0x0007UL  /* BIG5 Unicode at on		   */
#define SKF_B5_GBK	0x000cUL  /* GBK			   */
#define SKF_B5_GB8	0x000dUL  /* GB18030			   */

#define is_big5fam(x) ((x & SKF_COD_C_MSK) == SKF_BIG5)
#define is_big5(x)    ((x & SKF_COD_O_MSK) == SKF_BIG5)
#define is_big5n(x)   ((x & SKF_COD_UC_MSK) == SKF_BIG5)
#define is_big5p(x)   (((x & SKF_COD_C_MSK) == SKF_BIG5) && \
		       ((x & SKF_COD_S_MSK) < SKF_B5_GBK) && \
		       ((x & SKF_COD_S_MSK) >= SKF_B5_GCSS))
#define is_big5h(x)   ((x & SKF_COD_O_MSK) == (SKF_BIG5 | SKF_B5_GCSS))
#define is_cp950(x)   ((x & SKF_COD_O_MSK) == (SKF_BIG5 | SKF_B5_CP950))
#define is_hku(x)     ((x & SKF_COD_O_MSK) == (SKF_BIG5 | SKF_B5_HKU))
#define is_big5_2k(x) ((x & SKF_COD_O_MSK) == (SKF_BIG5 | SKF_B5_2003))
#define is_big5_uao(x) ((x & SKF_COD_O_MSK) == (SKF_BIG5 | SKF_B5_UAO))
#define is_b5_gbk(x)  ((x & SKF_COD_O_MSK) == (SKF_BIG5 | SKF_B5_GBK))
#define is_gb18030(x) ((x & SKF_COD_O_MSK) == (SKF_BIG5 | SKF_B5_GB8))

/* GBK and GB18030 */
#define is_gbk(x)     ((x & SKF_COD_U8_MSK) == (SKF_BIG5 | SKF_B5_GBK))
/* --------------------------------------------------------------- */
/* 7th set: SKF_GBKR						   */
/* --------------------------------------------------------------- */
#define SKF_GBKR_JOHAB	0x0001UL
#define SKF_GBKR_UHC	0x0002UL
#define SKF_GBKR_HZ	0x0004UL
#define SKF_GBKR_ZW	0x0005UL
#define SKF_GBKR_HZ8	0x0006UL
#define SKF_GBKR_THRU	0x0008UL  /* THRU			   */
#define SKF_GBKR_IS	0x0009UL  /* ISCII derivative		   */

#define is_gbkrfam(x) ((x & SKF_COD_C_MSK) == SKF_GBKR)
#define is_johab(x) ((x & SKF_COD_O_MSK) == (SKF_GBKR | SKF_GBKR_JOHAB))
#define is_uhc(x)   ((x & SKF_COD_O_MSK) == (SKF_GBKR | SKF_GBKR_UHC))
#define is_hz(x)    ((x & SKF_COD_O_MSK) == (SKF_GBKR | SKF_GBKR_HZ))
#define is_zW(x)    ((x & SKF_COD_O_MSK) == (SKF_GBKR | SKF_GBKR_ZW))
#define is_hz8(x)    ((x & SKF_COD_O_MSK) == (SKF_GBKR | SKF_GBKR_HZ8))
#define is_hzzW(x)  ((x & SKF_COD_UC_MSK) == (SKF_GBKR | SKF_GBKR_HZ))
#define is_iscii(x)  ((x & SKF_COD_O_MSK) == (SKF_GBKR | SKF_GBKR_IS))
#define is_gbthru(x) ((x & SKF_COD_O_MSK) == (SKF_GBKR | SKF_GBKR_THRU))
/* --------------------------------------------------------------- */
/* 8th set: SKF_NNISO						   */
/* --------------------------------------------------------------- */
#define SKF_NISO_GSM	0x0001UL
#define SKF_NISO_VM	0x000aUL  /* VIETNAMISE MISC.		   */
#define SKF_NISO_VNI	0x000cUL  /* VIETNAMISE VNI		   */
#define SKF_NISO_VISC	0x000dUL  /* rfc-VISCII			   */
#define SKF_NISO_VSQR	0x000eUL  /* VSQR			   */
#define SKF_NISO_VIMN	0x000fUL  /* VIMN			   */

#define is_nnisofam(x)	((x & SKF_COD_C_MSK) == SKF_NNISO)
#define is_nniso_gsm(x)	((x & SKF_COD_O_MSK)==(SKF_NNISO|SKF_NISO_GSM))

#define is_vni(x)   ((x & SKF_COD_O_MSK) == (SKF_NNISO | SKF_NISO_VNI))
#define is_viqr(x)  ((x & SKF_COD_O_MSK) == (SKF_NNISO | SKF_NISO_VSQR))
#define is_visc(x)  ((x & SKF_COD_O_MSK) == (SKF_NNISO | SKF_NISO_VISC))
#define is_vimn(x)  ((x & SKF_COD_O_MSK) == (SKF_NNISO | SKF_NISO_VIMN))
#define is_viqr_or_vimn(x) ((x & SKF_COD_U8_MSK)==\
			(SKF_NNISO | SKF_NISO_VSQR))
#define is_vivm(x)  (((x & SKF_COD_C_MSK) == SKF_NNISO) && \
			((x & SKF_COD_S_MSK) >= SKF_NISO_VM))

/* --------------------------------------------------------------- */
/* 9th set: SKF_KEIS						   */
/* --------------------------------------------------------------- */
#define out_keis  ((conv_cap & SKF_COD_C_MSK) == SKF_KEIS)

#define	SKF_JEF		0x00000002UL	/* Fujitsu JEF		   */
#define	SKF_JEF_S	0x00000003UL	/* Fujitsu JEF		   */
#define	SKF_NECH	0x00000004UL	/* ACOS series		   */
#define	SKF_NECI	0x00000005UL	/* ACOS series internal	   */
#define	SKF_MELC	0x00000006UL	/* MELCOM EBCDIC	   */
#define	SKF_DBCS	0x00000008UL	/* IBM MVS encode(jp,gb)   */
#define	SKF_DBCST	0x00000009UL	/* IBM MVS encode(kr,tw)   */

#define is_keis(x)	((x & SKF_COD_C_MSK) == SKF_KEIS)
#define is_keis_jef(x)	(((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_JEF)) ||\
			 ((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_JEF_S)))
#define is_keis_dbcs(x) ((x & SKF_COD_U8_MSK) == (SKF_KEIS | SKF_DBCS))
#define is_keis_dbcsj(x) ((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_DBCS))
#define is_keis_dbcst(x) ((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_DBCST))
#define is_keis_jefk(x) ((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_JEF))
#define is_keis_jefs(x) ((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_JEF_S))
#define is_keis_neci(x) ((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_NECI))
#define is_keis_nech(x) ((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_NECH))
#define is_keis_melc(x) ((x & SKF_COD_O_MSK) == (SKF_KEIS | SKF_MELC))
#define is_keis_keis(x) ((x & SKF_COD_O_MSK) == SKF_KEIS)

#define is_cod_iso(x) ((((x & SKF_COD_CAT_MSK) == 0) || is_msfam(x) \
			|| is_hz(x) || is_zW(x)) \
			&& !is_euc_niso(x))
#define is_cod_mb(x)  ((x & SKF_COD_MB) == SKF_COD_MB)
/* -------------------------------------------------------------- */
/* set of state test						  */
/* -------------------------------------------------------------- */
extern int	skip_x221_2_null();
#if 0
extern int	skip_x221_4_null();
#endif

/* --------------------------------------------------------------- */
/* conv_alt_cap: compatibility/capability features		   */
/* --------------------------------------------------------------- */
/* --- conv_alt_cap controlled features -------------------------- */
/* gaiji conversion features */
/* for latin */
#if 0
#define LATIN2X0212 0x80000000UL
#define use_latin2x0212 (conv_alt_cap & LATIN2X0212)
#define set_latin2x0212 conv_alt_cap |= LATIN2X0212
#endif

#define LATIN2HTML 0x40000000UL
#define use_latin2html (conv_alt_cap & LATIN2HTML)
#define set_latin2html conv_alt_cap |= LATIN2HTML

#define LATIN2TEX 0x20000000UL
#define use_latin2tex (conv_alt_cap & LATIN2TEX)
#define set_latin2tex conv_alt_cap |= LATIN2TEX

#define LATIN2HTML_D 0x10000000UL
#define use_latin2htmld (conv_alt_cap & LATIN2HTML_D)
#define set_latin2htmld conv_alt_cap |= (LATIN2HTML | LATIN2HTML_D)

#define LATIN2HTML_H 0x08000000UL
#define use_latin2htmlh (conv_alt_cap & LATIN2HTML_H)
#define set_latin2htmlh conv_alt_cap |= (LATIN2HTML | LATIN2HTML_H)

#define LATIN2HTML_U 0x04000000UL
#define use_latin2htmlu (conv_alt_cap & LATIN2HTML_U)
#define set_latin2htmlu conv_alt_cap |= (LATIN2HTML | LATIN2HTML_U)

#define HTML_SANITIZE 0x01000000UL	/* 1: sanitize suppress	   */
#define use_htmlsanitize (conv_alt_cap & HTML_SANITIZE)
#define set_htmlsanitize conv_alt_cap |= (HTML_SANITIZE | LATIN2HTML)

/* just discard */
#define LATIN2NULL 0x00800000UL
#define use_latin2null (conv_alt_cap & LATIN2NULL)
#define set_latin2null conv_alt_cap |= LATIN2NULL

/* non-kanji control bits */
#define CHART_DSBL 0x00400000UL      /* chart and form disable	   */
#define chart_dsbl (conv_alt_cap & CHART_DSBL)
#define set_chart_dsbl conv_alt_cap |= CHART_DSBL

/* jis90 suppressor */
#define SUP_JIS90 0x00200000UL      /* chart and form disable	   */
#define sup_jis90 (conv_alt_cap & SUP_JIS90)
#define set_sup_jis90 conv_alt_cap |= SUP_JIS90

/* --------------------------------------------------------------- */
#define	USE_ISO8859_1_L 0x00020000UL 
				/* disable Right plane for latin   */
#define	use_iso8859_1_left (conv_alt_cap & USE_ISO8859_1_L)
#define	set_use_iso8859_1_left conv_alt_cap |= USE_ISO8859_1_L
#define	res_use_iso8859_1_left conv_alt_cap &= 0xfffdffffUL

/* --- alternate features ---------------------------------------- */
/* flavours  0x0000*000						   */
/* --------------------------------------------------------------- */
#define KUNI_OPT	0x00008000UL
#define set_kuni_opt	conv_alt_cap |= KUNI_OPT
#define res_kuni_opt	conv_alt_cap &= 0xffff7fffUL
#define kuni_opt	(conv_alt_cap & KUNI_OPT)

#define ESTAB_REINIT	0x00004000UL
#define set_estab_reinit conv_alt_cap |= ESTAB_REINIT
#define estab_reinit	(conv_alt_cap & ESTAB_REINIT)
#define res_estab_reinit conv_alt_cap &= 0xffffbfffUL

#define	res_jis_flavors conv_alt_cap &= 0xffffc0ffUL

#define USE_G0_ASCII 0x00002000UL /* use ASCII as G0		   */
#define use_g0ascii (conv_alt_cap & USE_G0_ASCII)
#define set_use_g0ascii conv_alt_cap |= USE_G0_ASCII

/* Note: this bit is set for no purposes. Will be fixed in 1.95.   */
#define	USE_JIS8 0x00001000UL	/* iso-2022 8bit enable		   */

/* --------------------------------------------------------------- */
/* should be not needed.					   */
/* --------------------------------------------------------------- */
#define COD_99	  0x00000800UL	/* code is JIS X0213(2000)	   */
#define out_jis3  (conv_alt_cap & COD_99)
#define is_x0213(x) (COD_99 & x)
#define set_out_jis3 conv_alt_cap = \
		((conv_alt_cap & 0xfffff3ffUL) | COD_99)
#define set_new_jis conv_alt_cap = (conv_alt_cap & 0xfffff3ffUL)

#define USE_BG2CC	0x00000100UL
#define use_bg2cc	(conv_alt_cap & USE_BG2CC)
#define set_use_bg2cc	conv_alt_cap |= USE_BG2CC

/* - other miscellaneous controlls ------------------------------- */
#define STRIPINVIS 0x00000080UL    /* suppress invisible sequence  */
#define stripinvis (conv_alt_cap & STRIPINVIS)
#define set_stripinvis conv_alt_cap |= STRIPINVIS

#define JIS_HTML_PRI  0x00000040UL   /* html-jis yen priority	   */
#define force_jis_pri (conv_alt_cap & JIS_HTML_PRI)
#define set_force_jis_pri conv_alt_cap |= JIS_HTML_PRI
#define reset_force_jis_pri conv_alt_cap &= 0xffffffbfUL

#define F_DISPWARN  0x00000020UL   /* display warnings (warn only) */
#define force_disp_warn (conv_alt_cap & F_DISPWARN)
#define set_force_disp_warn conv_alt_cap |= F_DISPWARN
#define reset_force_disp_warn conv_alt_cap &= 0xffffffdfUL

#define DISPWARN  0x00000010UL  /* display warnings		   */
#define disp_warn (conv_alt_cap & DISPWARN)
#define set_disp_warn conv_alt_cap |= DISPWARN

#define FOLD_FLAT 0x00000004UL	/* flattening fold		   */
	/* disable smart line-end detection			   */
#define fold_flat (conv_alt_cap & FOLD_FLAT)
#define set_fold_flat conv_alt_cap |= FOLD_FLAT
#define reset_fold_flat conv_alt_cap &= 0xfffffffbUL

#define FOLD_STRONG 0x00000001UL /* strong kinsoku		   */
#define fold_strong (conv_alt_cap & FOLD_STRONG)
#define set_fold_strong conv_alt_cap |= FOLD_STRONG
#define reset_fold_strong conv_alt_cap &= 0xfffffffdUL

#ifdef NOSPACECONV
#define NO_SUP_SPACE_CONV 0x00000001UL /* ideographic space through mode */
#define sup_space_conv !(conv_alt_cap & NO_SUP_SPACE_CONV)
#define res_sup_space_conv conv_alt_cap |= NO_SUP_SPACE_CONV
#define set_sup_space_conv conv_alt_cap &= 0xfffffffeUL
#else
#define SUP_SPACE_CONV 0x00000001UL /* ideographic space through mode */
#define sup_space_conv (conv_alt_cap & SUP_SPACE_CONV)
#define set_sup_space_conv conv_alt_cap |= SUP_SPACE_CONV
#define res_sup_space_conv conv_alt_cap &= 0xfffffffeUL
#endif

/* --------------------------------------------------------------  */
/* preconvert.c control: 					   */
/* --------------------------------------------------------------  */
extern unsigned long preconv_opt;

#define INPUT_INQUIRY	0x20000000UL	/* inquiry mode		   */
#define input_inquiry  (preconv_opt & INPUT_INQUIRY)
#define set_input_inquiry preconv_opt |= INPUT_INQUIRY
#define reset_input_inquiry preconv_opt &= 0xdfffffffUL

#define SHOW_FILENAME	0x10000000UL	/* inquiry file name show  */
#define show_filename  (preconv_opt & SHOW_FILENAME)
#define set_show_filename preconv_opt |= SHOW_FILENAME

#define INPUT_HARD_INQUIRY 0x04000000UL	/* inquiry mode		   */
#define input_hard_inquiry  ((preconv_opt & INPUT_HARD_INQUIRY) && \
			     (preconv_opt & INPUT_INQUIRY))
#define set_input_hard_inquiry preconv_opt |= \
			(INPUT_HARD_INQUIRY|INPUT_INQUIRY)

#define INPUT_SOFT_INQUIRY 0x02000000UL	/* inquiry mode(short)	   */
#define input_soft_inquiry  (preconv_opt & INPUT_SOFT_INQUIRY)
#define set_input_soft_inquiry preconv_opt |= INPUT_SOFT_INQUIRY

#define MIME_MS_COMPAT 0x00010000UL  /* MIME Japanese fix	   */
#define mime_ms_compat (preconv_opt & MIME_MS_COMPAT)
#define set_mime_ms_compat preconv_opt |= MIME_MS_COMPAT

#define IN_DET_JIS78 0x00008000UL
#define	in_detect_jis78 (preconv_opt & IN_DET_JIS78)
#define set_in_detect_jis78 preconv_opt = \
	((preconv_opt & 0xffff3fffUL) | IN_DET_JIS78)
#define reset_in_detect_jis78 preconv_opt = \
	(preconv_opt & 0xffff3fffUL)

#define IN_NDET_COD_SYS 0x00002000UL
#define coding_system_sense ((preconv_opt & IN_NDET_COD_SYS) == 0)
#define set_suppress_codesys preconv_opt = \
	((preconv_opt & 0xffffcfffUL) | IN_NDET_COD_SYS)

#define KANJI_GO	0x00001000UL	/* Start with kanji mode   */
#define start_kanji  (preconv_opt & KANJI_GO)
#define set_start_kanji preconv_opt |= KANJI_GO

#define OLD_NEC_COMPAT_C  0x00000800UL	/* NEC old codeset detect  */
#define old_nec_compat (preconv_opt & OLD_NEC_COMPAT_C)
#define set_old_nec_compat preconv_opt |= OLD_NEC_COMPAT_C

#define NO_LANG_PRESERVE  0x00000100UL	/* lang preserve in lines  */
#define no_lang_preserve (preconv_opt & NO_LANG_PRESERVE)
#define set_no_lang_preserve preconv_opt |= NO_LANG_PRESERVE

#define INPUT_JAPAN_LIMIT  0x00000040UL	/* detect only Japanese	   */
#define input_jp_limit (preconv_opt & INPUT_JAPAN_LIMIT)
#define set_input_jp_limit preconv_opt |= INPUT_JAPAN_LIMIT

#define INPUT_KANA_UNDET  0x00000020UL	/* kana detect on	   */
#define input_x201_kana (preconv_opt & INPUT_KANA_UNDET)
#define set_input_x201_kana preconv_opt |= INPUT_KANA_UNDET
#define res_input_x201_kana preconv_opt &= 0xfffffffdfUL

#define FUZZY_DET  0x00000010UL	/* fuzzy detect			   */
#define fuzzy_detect (preconv_opt & FUZZY_DET)
#define set_fuzzy_detect preconv_opt |= FUZZY_DET

#if 0
#define NO_KANA	  0x00000008UL	/* suppress x0201 kana detection   */
#define no_kana	 (preconv_opt & NO_KANA)
#define set_no_kana preconv_opt |= NO_KANA
#endif

#define NO_UTF7	  0x00000004UL	/* suppress utf7 detection	   */
#define no_utf7	 (preconv_opt & NO_UTF7)
#define set_no_utf7 preconv_opt |= NO_UTF7

#define LANG_SENSE  0x00000002UL  /* Language sense enable	   */
#define lang_sense  (preconv_opt & LANG_SENSE)
#define set_lang_sense preconv_opt |= LANG_SENSE

/* --------------------------------------------------------------  */
/* in_param: input command-line parameter save for codeset_flavor  */
/* --------------------------------------------------------------  */
extern unsigned long	in_param;

#define SKFPAR_O_U_ENDIAN 0x40000000UL	/* parameter is set	   */
#define SKFPAR_O_U_BE	  0x20000000UL	/* big endian		   */

#define SKFPAR_I_U_ENDIAN 0x04000000UL	/* parameter is set	   */
#define SKFPAR_I_U_BE	  0x02000000UL	/* big endian		   */

#define set_out_ltl_endian in_param = \
    (in_param & 0x0fffffffUL) | SKFPAR_O_U_ENDIAN
#define set_out_big_endian in_param = \
    (in_param & 0x0fffffffUL) | SKFPAR_O_U_BE | SKFPAR_O_U_ENDIAN
#define is_set_out_le \
   ((in_param & (SKFPAR_O_U_ENDIAN | SKFPAR_O_U_BE)) == SKFPAR_O_U_ENDIAN)
#define is_set_out_be \
   ((in_param & (SKFPAR_O_U_ENDIAN | SKFPAR_O_U_BE)) == \
	(SKFPAR_O_U_ENDIAN | SKFPAR_O_U_BE))
#if 0
#define set_in_ltl_endian in_param = \
    (in_param & 0xf0ffffffUL) | SKFPAR_I_U_ENDIAN
#define set_in_big_endian in_param = \
    (in_param & 0xf0ffffffUL) | SKFPAR_I_U_BE | SKFPAR_I_U_ENDIAN
#endif
#define is_set_in_le \
   ((in_param & (SKFPAR_I_U_ENDIAN | SKFPAR_I_U_BE)) == SKFPAR_I_U_ENDIAN)
#define is_set_in_be \
   ((in_param & (SKFPAR_I_U_ENDIAN | SKFPAR_I_U_BE)) == \
	(SKFPAR_I_U_ENDIAN | SKFPAR_I_U_BE))
#define in_ltl_endian \
   ((in_param & (SKFPAR_I_U_ENDIAN | SKFPAR_I_U_BE)) == \
	(SKFPAR_I_U_ENDIAN))
#define in_big_endian \
   ((in_param & (SKFPAR_I_U_ENDIAN | SKFPAR_I_U_BE)) == \
	(SKFPAR_I_U_ENDIAN | SKFPAR_I_U_BE))
/* --------------------------------------------------------------  */
#define SKF_IS_ARIB_FLAVOR	  0x00400000UL
#define set_arib_flavor	in_param |= SKF_IS_ARIB_FLAVOR
#define is_skf_aribmd	 (in_param & SKF_IS_ARIB_FLAVOR)
/* --------------------------------------------------------------  */
#define	SKFPAR_HK_ENBL	  0x00200000UL	/* x0201 kana fac.	   */
#define	is_par_hk_enbl	(in_param & SKFPAR_HK_ENBL)

#define set_par_kana_call in_param =\
	(in_param & 0xff1fffffUL) | KANA_CALL | SKFPAR_HK_ENBL
#define set_par_si_enbl in_param = \
	(in_param & 0xff1fffffUL) | SI_ENBL | SKFPAR_HK_ENBL
#define set_par_eightbit in_param = \
	(in_param & 0xff1fffffUL) | EIGHTBIT | SKFPAR_HK_ENBL
#define get_par_hkenbl (in_param & 0x00c00000UL)

/* --------------------------------------------------------------  */
#define SKFPAR_U_ISO8859  0x00080000UL	/* use_iso8859_1	   */

#define SKFPAR_O_BOM	  0x00020000UL	/* bom ordered		   */
#define SKFPAR_O_EBOM	  0x00010000UL	/* enable bom		   */
#define SKFPAR_BOMMSK	  0xfffcffffUL

#define set_pout_iso8859 in_param |= (SKFPAR_U_ISO8859 | SKF_USE_ISO8859)
#define res_pout_iso8859 in_param = (in_param & 0xffe7ffffUL) \
	| SKFPAR_U_ISO8859
#define is_set_p_iso8859 \
    ((in_param & (SKFPAR_U_ISO8859 | SKF_USE_ISO8859)) \
	== (SKFPAR_U_ISO8859 | SKF_USE_ISO8859))
#define is_set_p_niso8859 \
   ((in_param & (SKFPAR_U_ISO8859 | SKF_USE_ISO8859)) == SKFPAR_U_ISO8859)

#define set_pout_ucs_bom in_param |= (SKFPAR_O_BOM | SKFPAR_O_EBOM)
#define set_pout_ucs_nobom in_param = (in_param & SKFPAR_BOMMSK) \
	| SKFPAR_O_BOM
#define is_set_ucs_bom ((in_param & (SKFPAR_O_BOM | SKFPAR_O_EBOM)) == \
	(SKFPAR_O_BOM | SKFPAR_O_EBOM)) 
#define is_set_ucs_nobom ((in_param & (SKFPAR_O_BOM | SKFPAR_O_EBOM)) == \
	SKFPAR_O_BOM) 
/* --------------------------------------------------------------  */
#define SKFPAR_SX212	0x00008000UL	/* jis x0212 out set	   */

#define set_pux212 in_param |= SKFPAR_SX212
#define res_pux212 in_param = (in_param & 0xffff7fffUL)
#define is_set_pux212 ((in_param & SKFPAR_SX212) == SKFPAR_SX212)
#define is_set_pnux212 ((in_param & SKFPAR_SX212) == 0)
/* --------------------------------------------------------------  */
#define SKFPAR_S_MS	0x00000800UL	/* set ms_compat mode	   */

#define set_par_mscpt	in_param |= SKFPAR_S_MS
#define set_par_no_mscpt in_param = (in_param & 0xfffff3ffUL)
#define is_set_mscpt	((in_param & SKFPAR_S_MS) == SKFPAR_S_MS)
#define is_set_no_mscpt	((in_param &  SKFPAR_S_MS) == 0)
/* --------------------------------------------------------------  */
#define SKFPAR_U_ANN	0x00000100UL	/* add announce		   */

#define set_par_ad_ann	in_param = \
	((in_param & 0xfffffcffUL) | SKFPAR_U_ANN | SKF_ADD_ANNON)
#define set_par_no_ann	in_param = \
	((in_param & 0xfffffcffUL) | SKFPAR_U_ANN)
#define is_set_ad_ann	((in_param & (SKFPAR_U_ANN | SKF_ADD_ANNON)) \
	== (SKFPAR_U_ANN | SKF_ADD_ANNON))
#define is_set_no_ann	((in_param & (SKFPAR_U_ANN | SKF_ADD_ANNON)) \
	== SKFPAR_U_ANN)
/* --------------------------------------------------------------  */
/* Language control: input side					   */
/* --------------------------------------------------------------  */
extern unsigned long skf_input_lang;

/* --------------------------------------------------------------  */
/* Language control: output side				   */
/* --------------------------------------------------------------  */
extern unsigned long skf_output_lang;
extern int	     out_codeset;
extern int	     in_codeset;

extern void skf_output_table_set();

/* --------------------------------------------------------------  */
/* Encoding control: output side				   */
/* --------------------------------------------------------------  */
extern unsigned int 	o_encode; 

#define	O_ENCODE_HEX	0x0001U /* cap (like :9F)		   */
#define	O_ENCODE_ROT	0x0002U	/* not used for output		   */
#define	O_ENCODE_MIME	0x0004U
#define	O_ENCODE_MIMEQ	0x0008U
#define	O_ENCODE_MIMEBS	0x0010U	/* not used for output		   */
#define	O_ENCODE_OCT	0x0020U
#define	O_ENCODE_BASE64	0x0040U
#define	O_ENCODE_RFC2231 0x0080U
#define	O_ENCODE_PERC	0x0100U /* %** notation like in URI	   */
#define	O_ENCODE_URI	0x0200U
#define	O_ENCODE_DCM	0x0400U	/* not used for output		   */
#define	O_ENCODE_Q	0x0800U /* hex but uses "=" 		   */
#define	O_ENCODE_PUNY	0x1000U /* hex but uses "=" 		   */

#define O_ENCODE_MIMES  (O_ENCODE_MIME | O_ENCODE_MIMEQ | O_ENCODE_RFC2231)
#define O_ENCODE_MIMEBQ  (O_ENCODE_MIME | O_ENCODE_MIMEQ)

#define O_ENCODE_MIME_MASK 0x001cU
#define O_ENCODE_HEX_MASK 0x0f03U
#define O_ENCODE_IUCS2	   0x0082U

/*
 * rfc2231, dcm is NOT implemented.
 */
#define is_o_encode_hex(x)	(x & O_ENCODE_HEX)
#define is_o_encode_mimeb(x)	(x & O_ENCODE_MIME)
#define is_o_encode_mimeq(x)	(x & O_ENCODE_MIMEQ)
#define is_o_encode_oct(x)	(x & O_ENCODE_OCT)
#define is_o_encode_b64(x)	(x & O_ENCODE_BASE64)
#define is_o_encode_2231(x)	(x & O_ENCODE_RFC2231)
#define is_o_encode_perc(x)	(x & O_ENCODE_PERC)
#define is_o_encode_uri(x)	(x & O_ENCODE_URI)
#define is_o_encode_dcm(x)	(x & O_ENCODE_DCM)
#define is_o_encode_q(x)	(x & O_ENCODE_Q)
#define is_o_encode_ace(x)	(x & O_ENCODE_PUNY)
#define is_o_encode_cap(x) \
	((x & O_ENCODE_HEX_MASK) == O_ENCODE_HEX)
#define is_o_encode		(o_encode != 0)
#define is_o_encode_mimebq(x)	(x & O_ENCODE_MIMEBQ)
#define is_o_encode_mimes(x)	(x & O_ENCODE_MIMES)

#define set_o_mimeb_encode o_encode = O_ENCODE_MIME
#define set_o_mimeq_encode o_encode = O_ENCODE_MIMEQ
#define set_o_hex_encode o_encode = O_ENCODE_HEX
#define set_o_hex_qencode o_encode = (O_ENCODE_Q | O_ENCODE_HEX)
#define set_o_hex_uri_encode o_encode = O_ENCODE_URI
#define set_o_hex_perc_encode o_encode = (O_ENCODE_HEX | O_ENCODE_PERC)
#define set_o_oct_encode o_encode = O_ENCODE_OCT
#define set_o_base64_encode o_encode = O_ENCODE_BASE64
#define set_o_2231_encode o_encode = O_ENCODE_RFC2231
#define reset_o_encode o_encode = 0

/* --------------------------------------------------------------  */
/* Encoding control: input side					   */
/* --------------------------------------------------------------  */
extern	long		encode_cap; 

#define SKF196MIMECOMPAT 0x00010000UL
	/* does not need space after CR/LF on softwrap		   */

/* --------------------------------------------------------------  */
#define	encode_enbl	(encode_cap != 0)
#define is_ucod_ace	(encode_cap & O_ENCODE_PUNY)

/* --------------------------------------------------------------  */
#define set_hex_encoded	encode_cap = O_ENCODE_HEX
#define reset_encoded encode_cap = 0;
#define decode_set(x) encode_cap = x

#define is_hex_encoded	(encode_cap & O_ENCODE_HEX)
#define is_rot_encoded	(encode_cap & O_ENCODE_ROT)
#define is_oct_encoded	(encode_cap & O_ENCODE_OCT)
#define is_base64_encoded (encode_cap & O_ENCODE_BASE64)
#define is_rfc2231_encoded (encode_cap & O_ENCODE_RFC2231)
#define	is_hex_cap ((encode_cap & O_ENCODE_PERC) == 0)
#define	is_hex_uri (encode_cap & O_ENCODE_URI)
#define	is_hex_uri_docomo ((encode_cap & O_ENCODE_HEX_MASK) == \
		(O_ENCODE_URI | O_ENCODE_DCM))
#define	is_hex_qencode (encode_cap & O_ENCODE_Q)
#define is_puny_encoded (encode_cap & O_ENCODE_PUNY)
#define is_mimeq_encoded ((encode_cap & O_ENCODE_MIME_MASK) == \
		(O_ENCODE_MIME | O_ENCODE_MIMEQ))
#define is_mimeb_encoded ((encode_cap & O_ENCODE_MIME_MASK) == \
		O_ENCODE_MIME)
#define is_mimeb_strict ((encode_cap & O_ENCODE_MIME_MASK) == \
		(O_ENCODE_MIME | O_ENCODE_MIMEBS))
#define set_mimeb_encode encode_cap |= O_ENCODE_MIME
#define set_mimeq_encode encode_cap |= (O_ENCODE_MIME | O_ENCODE_MIMEQ)
#define set_mimeb_strict encode_cap |= (O_ENCODE_MIME | O_ENCODE_MIMEBS)
#define set_rot_encode	 encode_cap |= O_ENCODE_ROT
#define set_hex_encode encode_cap |= O_ENCODE_HEX
#define set_hex_qencode encode_cap |= (O_ENCODE_Q | O_ENCODE_HEX)
#define set_hex_uri_encode encode_cap |= O_ENCODE_URI
#define set_hex_perc_encode encode_cap |= (O_ENCODE_HEX | O_ENCODE_PERC)
#define set_hex_uri_docomo_encode encode_cap |= \
		(O_ENCODE_URI | O_ENCODE_DCM)
#define set_oct_encode encode_cap |= O_ENCODE_OCT
#define set_rfc2231_encode encode_cap |= O_ENCODE_RFC2231
#define set_base64_encode encode_cap |= O_ENCODE_BASE64
#define set_puny_encode encode_cap |= O_ENCODE_PUNY
#define is_encode_incomp_ucs2 (encode_cap & O_ENCODE_IUCS2)

#define set_skf196mime	encode_cap |= SKF196MIMECOMPAT
#define is_skf196mime	(encode_cap & SKF196MIMECOMPAT)

/* --------------------------------------------------------------  */
/* fold and lineend controls					   */
/* --------------------------------------------------------------  */
extern int	le_detect;	/* lineend detect marker	   */
extern int	le_defs;	/* normalized lineend		   */

#define set_detect_cr	le_detect |= 0x02
#define set_detect_lf	le_detect |= 0x04
#define set_detect_crlf	le_detect |= 0x06
#define set_first_detect_cr le_detect |= 0x10
#define set_mix_detect	le_detect = 0x20
#define set_dummy_detect le_detect |= 0x100
#define set_add_lastle le_detect |= 0x1000

#define detect_cr	(le_detect & 0x02)
#define detect_lf	(le_detect & 0x04)
#define first_detect_cr (le_detect & 0x10)
#define mix_le_detect	(le_detect & 0x20)
#define is_dummy_ledet  (le_detect & 0x100)
#define is_add_lastle	(le_detect & 0x1000)

#define use_detect_cr	(le_defs & 0x02)
#define use_detect_lf	(le_defs & 0x04)
#define use_first_detect_cr	(le_defs & 0x10)
#define set_use_detect_cr	le_defs |= 0x02
#define set_use_detect_lf	le_defs |= 0x04
#define set_use_detect_crlf	le_defs |= 0x06
#define set_use_first_detect_cr le_defs |= 0x10

/* --------------------------------------------------------------  */
/* option guarding controls: force respect command line option.	   */
/* --------------------------------------------------------------  */
extern unsigned long	option_guarding;

#define EUC_PROTECT 0x00000010UL
#define euc_protect	(option_guarding & EUC_PROTECT)
#define set_euc_protect  option_guarding |= EUC_PROTECT

#define ENDIAN_PROTECT 0x00000020UL
#define endian_protect	(option_guarding & ENDIAN_PROTECT)
#define set_endian_protect  option_guarding |= ENDIAN_PROTECT

#define UP_BLOCK 0x00020000UL
#define up_block	(option_guarding & UP_BLOCK)
#define set_up_block  option_guarding |= UP_BLOCK

#if 0
#define DISABLE_LOAD 0x00001000UL
#define disable_load	(option_guarding & DISABLE_LOAD)
#define set_disable_loading  option_guarding |= DISABLE_LOAD
#endif

/* --------------------------------------------------------------  */
/* Unic*de misc. output controls				   */
/* --------------------------------------------------------------  */
/* Note: this flag have effect to output side code conversion.	   */
/* --------------------------------------------------------------  */
extern unsigned long	ucod_flavor;
/* --------------------------------------------------------------  */
#define OLD_EMOT_MAP 0x00000008UL   /* old emoticon map compat	   */
#define use_old_cell_map (ucod_flavor & OLD_EMOT_MAP)
#define set_use_old_cell_map ucod_flavor |= OLD_EMOT_MAP

#define UCS2_REPL 0x00000010UL	    /* use unic*de replacement ch. */
#define use_uni_repl (ucod_flavor & UCS2_REPL)
#define set_use_uni_repl ucod_flavor |= UCS2_REPL

/* --------------------------------------------------------------  */
/* enbl_decomp=0,decomp_comp=x:	no decomposition		   */
/* enbl_decomp=1,decomp_comp=0:	perform NFD			   */
/* enbl_decomp=1,decomp_comp=1:	perform NFKD			   */
/* --------------------------------------------------------------  */
#define DECOMP_APPLE 0x00000020UL /* Unicode apple-compat decomp.  */
#define decomp_apple (ucod_flavor & DECOMP_APPLE)
#define set_decomp_apple ucod_flavor |= DECOMP_APPLE
#define res_decomp_apple ucod_flavor &= 0xffffffdfUL

#define DECOMP_COMP 0x00000040UL /* Unicode compatibility decomp.  */
#define decomp_comp (ucod_flavor & DECOMP_COMP)
#define set_decomp_comp ucod_flavor |= DECOMP_COMP
#define res_decomp_comp ucod_flavor &= 0xffffffbfUL

#define ENBL_DECOMP 0x00000080UL /* Unicode de-composition control */
#define enbl_decomp (ucod_flavor & ENBL_DECOMP)
#define set_enbl_decomp ucod_flavor |= ENBL_DECOMP
#define res_enbl_decomp ucod_flavor &= 0xffffff7fUL

/* --------------------------------------------------------------  */
/* various inhibit flags.					   */
/* --------------------------------------------------------------  */
#define LIMIT_UCS2 0x00000100UL	/* suppress unic*de >0x10000 area  */
#define limit_ucs2 (ucod_flavor & LIMIT_UCS2)
#define set_limit_ucs2 ucod_flavor |= LIMIT_UCS2

#define SUP_CJK_EXT_A 0x00000200UL /* suppress UCS CJK extention A */
#define sup_cjk_ext_a (ucod_flavor & SUP_CJK_EXT_A)
#define set_sup_cjk_ext_a ucod_flavor |= SUP_CJK_EXT_A

#define SUP_CJK_EXT_CMP 0x00000400UL /* suppress UCS CJK compat ideo.*/
#define sup_cjk_cmp (ucod_flavor & SUP_CJK_EXT_CMP)
#define set_sup_cjk_cmp ucod_flavor |= SUP_CJK_EXT_CMP

/* --------------------------------------------------------------  */
/* composition side:						   */
/* default: enabled (dsbl_ucode_encomp = 0)			   */
/* if (dsbl_ucod_encomp == 1), all encomposition will be disabled  */
/* dsbl_uni_kana_concat=1:	3040-30ff concat is disabled	   */
/*  enbl_uni_hain_concat=1;	  but x-0213 ainu kana concatted   */
/* dsbl_uni_hkna_concat=1:	f060-f09f concat is disabled	   */
/* --------------------------------------------------------------  */
#define KANA_CONCAT 0x00000800UL    /* use unic*de dakuten-concat  */
#define dsbl_uni_kana_concat (ucod_flavor & KANA_CONCAT)
#define set_uni_kana_concat ucod_flavor |= KANA_CONCAT
#define res_uni_kana_concat ucod_flavor &= 0xfffff7ffUL

#define DSBL_UCOD_ENCOMP 0x00002000UL	/* Unicode KANAcompress	*/
#define dsbl_ucod_encomp (ucod_flavor & DSBL_UCOD_ENCOMP)
#define set_dsbl_ucod_encomp ucod_flavor |= DSBL_UCOD_ENCOMP
#define res_dsbl_ucod_encomp ucod_flavor &= 0xffffdfffUL

#define DSBL_HKANA_ENCOMP 0x00004000UL	/* Unicode Hkanacompress  */
#define dsbl_uni_hkna_concat (ucod_flavor & DSBL_HKANA_ENCOMP)
#define set_uni_hkna_concat ucod_flavor |= DSBL_HKANA_ENCOMP
#define res_uni_hkna_concat ucod_flavor &= 0xffffbfffUL

#define ENBL_HAIN_ENCOMP  0x00008000UL	/* X-0213 AINUkanacompress  */
#define enbl_uni_hain_concat (ucod_flavor & ENBL_HAIN_ENCOMP)
#define set_uni_hain_concat ucod_flavor |= ENBL_HAIN_ENCOMP
#define res_uni_hain_concat ucod_flavor &= 0xffff7fffUL

#define ENBL_UCOD_DENCOMP 0x00010000UL	/* NFD	*/
#define enbl_ucod_dencomp (ucod_flavor & ENBL_UCOD_DENCOMP)
#define set_enbl_ucod_dencomp ucod_flavor |= ENBL_UCOD_DENCOMP
#define res_enbl_ucod_dencomp ucod_flavor &= 0xfffeffffUL

#define ENBL_UCOD_CENCOMP 0x00020000UL	/* NFC	*/
#define enbl_ucod_cencomp (ucod_flavor & ENBL_UCOD_CENCOMP)
#define set_enbl_ucod_cencomp ucod_flavor |= ENBL_UCOD_CENCOMP
#define res_enbl_ucod_cencomp ucod_flavor &= 0xfffdffffUL

/* --------------------------------------------------------------  */
#define ENBL_DBL_LATIN 0x00008000UL
#define enable_dbl_latin (ucod_flavor & ENBL_DBL_LATIN)
#define set_enable_dbl_latin ucod_flavor |= ENBL_DBL_LATIN

#define OLDCELL_TO_EMOT 0x00020000UL
#define enable_cellconvert (ucod_flavor & OLDCELL_TO_EMOT)
#define set_enable_cellconvert ucod_flavor |= OLDCELL_TO_EMOT

#define ENBL_VAR_CNTL 0x00040000UL
#define enable_var_cntl (ucod_flavor & ENBL_VAR_CNTL)
#define set_enable_var_cntl ucod_flavor |= ENBL_VAR_CNTL

#define ENBL_NOUNICODE6 0x00080000UL	/* Unicode 6 feature	   */
/* mainly enables emoji-area conversion				   */
#define set_nounicode6  ucod_flavor |= ENBL_NOUNICODE6
#define is_nounicode6  (ucod_flavor & ENBL_NOUNICODE6)

#define NYUKAN_CONVERT 0x00100000UL
#define nyukan_convert (ucod_flavor & NYUKAN_CONVERT)
#define set_nyukan_convert ucod_flavor |= NYUKAN_CONVERT

#define ARIB_BMP_MASK	  0xff9fffffUL
#define ARIB_BMP0_CONVERT 0x00200000UL
#define arib_bmp0_convert (ucod_flavor & ARIB_BMP_MASK) == ARIB_BMP0_CONVERT
#define set_arib_bmp0_convert ucod_flavor = \
		(ucod_flavor & ARIB_BMP_MASK) | ARIB_BMP0_CONVERT

#define ARIB_BMP1_CONVERT 0x00400000UL
#define arib_bmp1_convert (ucod_flavor & ARIB_BMP_MASK) == ARIB_BMP1_CONVERT
#define set_arib_bmp1_convert ucod_flavor = \
		(ucod_flavor & ARIB_BMP_MASK) | ARIB_BMP1_CONVERT

#define ARIB_BMP2_CONVERT 0x00600000UL
#define arib_bmp2_convert (ucod_flavor & ARIB_BMP_MASK) == ARIB_BMP2_CONVERT
#define set_arib_bmp2_convert ucod_flavor = \
		(ucod_flavor & ARIB_BMP_MASK) | ARIB_BMP2_CONVERT

/* --------------------------------------------------------------  */
/* Note: 0x***0**** - disable language tag completely		   */
/*       0x***1**** - output language tag if strong lang specified */
/*       0x***2**** - disable language tag completely		   */
/*       0x***3**** - output language tag if valid lang specified  */
/*   strong lang: M_** lang is specified as codeset, or unicode	   */
/*		language tag is given as input			   */
/*   valid lang: in_codeset is not 0x0000, L_NU nor L_EM.	   */
/* --------------------------------------------------------------  */
#define LANG_TAG_ENBL 0x00400000UL  /* language tag output enable  */
#define LANG_TAG_ENCR 0x00200000UL  /* language tag output encourage */
#define is_lang_tag_enbl (ucod_flavor & LANG_TAG_ENBL)
#define is_lang_tag_encr ((ucod_flavor & LANG_TAG_ENBL) && \
			  (ucod_flavor & LANG_TAG_ENCR))
#define set_lang_tag_enbl ucod_flavor &= 0xffffc000UL | (LANG_TAG_ENBL)
#define set_lang_tag_encr ucod_flavor &= 0xffffc000UL | \
				(LANG_TAG_ENBL | LANG_TAG_ENCR)

#define FORCE_PRIVATE_IDN_O 0x00100000UL
		/* use private area for idn stringprep		   */
#define force_private_idn_o (ucod_flavor & FORCE_PRIVATE_IDN_O)
#define set_force_private_idn_o ucod_flavor |= FORCE_PRIVATE_IDN_O

/* --------------------------------------------------------------  */
/* switches to control input/output mode.			   */
/* --- internal states ------------------------------------------- */
extern int	unbuf_f;	/* unbuffered mode		   */
extern skf_ucode ucode_undef;	/* unic*de undef char		   */

/* kanji-kana transition character definition			   */
extern char	k_in,k_out;

/* --------------------------------------------------------------  */
/* codeset flavors controls					   */
/* codeset_flavor: codeset minor change control			   */
/* --------------------------------------------------------------  */
extern unsigned long codeset_flavor; 
/* this feature is not absolutely necessary (can change by 	   */
/* defining alternate codeset), but reduces skf code size.	   */

#define IN_ENDIAN    0x00010000UL	/* 1: BIG endian	   */

#define in_endian (codeset_flavor & IN_ENDIAN)
#define set_in_endian \
	codeset_flavor = (codeset_flavor & 0xfffeffffUL) | IN_ENDIAN
/* reset is for set little endian. clear is for clearing state.	   */
#define reset_in_endian codeset_flavor = (codeset_flavor & 0xfffeffffUL)
#define clear_in_endian codeset_flavor = (codeset_flavor & 0xfffeffffUL)

#define UNCHK_UTF32_RNG  0x00008000UL /* uncheck if within utf-32  */
#define set_unchk_utf32_range codeset_flavor |= UNCHK_UTF32_RNG
#define unchk_utf32_range  (codeset_flavor & UNCHK_UTF32_RNG)

#define NON_STRICT_UTF8  0x00004000UL	/* UTF-8 loose check	   */
#define set_non_strict_utf8 codeset_flavor |= NON_STRICT_UTF8
#define non_strict_utf8  (codeset_flavor & NON_STRICT_UTF8)

#define IN_TABLE_DUMP 0x00002000UL
#define set_table_dump codeset_flavor |= IN_TABLE_DUMP
#define table_dump (codeset_flavor & IN_TABLE_DUMP)

#define ENABLE_CESU  0x00001000UL	/* CESU enable		   */
#define set_enable_cesu codeset_flavor |= ENABLE_CESU
#define enable_cesu  (codeset_flavor & ENABLE_CESU)

#define	ENBL_LATIN_ANNON 0x00000200UL  /* enable annon. for latin */
#define	enbl_latin_annon (codeset_flavor & ENBL_LATIN_ANNON)
#define	set_enbl_latin_annon codeset_flavor |= ENBL_LATIN_ANNON
#define	res_enbl_latin_annon codeset_flavor &= 0xfffffdffUL

#define ADD_RENEW 0x00000100UL	/* add jis-0208 renewal seq.	   */
#define add_renew (codeset_flavor & ADD_RENEW)
#define set_add_renew codeset_flavor |= ADD_RENEW

#define UNUSE_COMPAT  0x00000020UL  /* unuse unic*de compatible plane  */
#define use_compat !(codeset_flavor & UNUSE_COMPAT)
#define reset_use_compat codeset_flavor |= UNUSE_COMPAT
#define set_use_compat codeset_flavor &= 0xffffffdfUL

#define USE_CDE_COMPAT 0x00000010UL
#define use_cde_compat (codeset_flavor & USE_CDE_COMPAT)
#define set_use_cde_compat codeset_flavor |= USE_CDE_COMPAT

#define USE_APPLE_GAIJI  0x00000008UL  /* Kanjitalk 7(8,9) gaiji   */
#define use_apple_gaiji (codeset_flavor & USE_APPLE_GAIJI)
#define set_use_apple_gaiji codeset_flavor |= USE_APPLE_GAIJI

#define miti_undef ((codeset_flavor & 0x0000000fUL) != 0)
#define set_miti_undef codeset_flavor &= 0xfffffff0UL

#define res_compat_feature codeset_flavor &= 0xffff0000UL

/* -------------------------------------------------------------- */
/* nkf compatibility mode control 				  */
/* -------------------------------------------------------------- */
extern unsigned long	nkf_compat;

#define NKF_FULLCOMPAT		0x40000000UL
#define O_PREFER_ASC		0x08000000UL
#define NKF_FORCE_NOHK		0x02000000UL /* don't use hankaku */
#define MIME_UNTRUNC_SPACE	0x01000000UL
#define LINEEND_CR   		0x00400000UL
#define LINEEND_LF   		0x00800000UL
#define LINEEND_CRLF		0x00c00000UL
#define LINEEND_TRU 		0x00000000UL
#define LINEEND_MSK 		0x00c00000UL
#define LINEEND_UMK		0xff1fffffUL
#define FOLD_NOTRUNC_LE		0x00200000UL /* no strip lineend    */

#define SPACE_NOADELIM		0x00040000UL
#define KANA_CONV_X0201		0x00080000UL
#define SPACE_CONV_1		0x00020000UL
#define ASCII_CONV_X0208	0x00010000UL
#define NKF_CONVERT_HOOK	0x000b8f00UL

#define LINEEND_NORMALIZE	0x00008000UL
#define NO_EARLY_MIME_OUT	0x00004000UL
#define SENTENCE_CLIP		0x00002000UL
#define MIME_LIMIT_AWARE	0x00001000UL

#define NKF_CP932_EXT		0x00000800UL
#define NKF_NOCP932_EXT		0x00000400UL
#define NKF_NO_BFC		0x00000200UL /* best-fit-chars	   */
#define NKF_NOCP932		0x00000100UL /* no NEC gaiji	   */

#define NKF_OVERWRITE		0x00000080UL /* --overwrite	   */
#define NKF_IN_PLACE		0x00000040UL /* --in-place	   */

#define NKF_HIRAGANA		0x00000020UL /* --hiragana (h1)	   */
#define NKF_KATAKANA		0x00000010UL /* --katakana (h2)	   */
#define NKF_ROT_OUTPUT		0x00000008UL /* NKF style ROT	   */

/* bit 1,2,3: 00 - no recovery bit0: bit1: B1 bit2:B2		   */
#define INPUT_JIS_BROKEN 	0x00000004UL /* JIS Broken support */
#define INPUT_JIS_FBROKEN 	0x00000002UL /* JIS Broken support */
#define INPUT_JIS_LBROKEN 	0x00000001UL /* JIS Broken support */

#define SENTENCE_LIMIT		1024

#define is_lineend_thru ((nkf_compat & LINEEND_MSK) == 0)
#define is_lineend_crlf ((nkf_compat & LINEEND_MSK) == LINEEND_CRLF)
#define is_lineend_cr   ((nkf_compat & LINEEND_MSK) == LINEEND_CR)
#define is_lineend_lf   ((nkf_compat & LINEEND_MSK) == LINEEND_LF)
#define is_lineend_normalize   ((nkf_compat & LINEEND_NORMALIZE) != 0)
#define is_mime_nkfmode	(nkf_compat &\
			    (MIME_UNTRUNC_SPACE | NKF_FULLCOMPAT))

#define set_lineend_thru nkf_compat = (nkf_compat & LINEEND_UMK)
#define set_lineend_crlf nkf_compat = \
	(nkf_compat & LINEEND_UMK) | LINEEND_CRLF
#define set_lineend_lf nkf_compat = (nkf_compat & LINEEND_UMK) | LINEEND_LF
#define set_lineend_cr nkf_compat = (nkf_compat & LINEEND_UMK) | LINEEND_CR
#define set_lineend_normalize nkf_compat = nkf_compat | LINEEND_NORMALIZE
#define set_mime_nkfmode nkf_compat |= MIME_UNTRUNC_SPACE

#define set_nkf_compat	nkf_compat |= (NKF_FULLCOMPAT | LINEEND_NORMALIZE)
#define reset_nkf_compat nkf_compat &= 0x00ffffffUL
#define is_nkf_compat	(nkf_compat & NKF_FULLCOMPAT)

/* fullwidth space to space control */
#define set_spconv_1	nkf_compat |= SPACE_CONV_1
#define is_spconv_1	(nkf_compat & SPACE_CONV_1)

#define set_noadelim	nkf_compat |= SPACE_NOADELIM
#define is_noadelim	(nkf_compat & SPACE_NOADELIM)
#define set_sentence_clip nkf_compat |= SENTENCE_CLIP
#define is_sentence_clip (nkf_compat & SENTENCE_CLIP)
#define set_kanaconv_x0201 nkf_compat |= KANA_CONV_X0201
#define set_ascii_conv nkf_compat |= ASCII_CONV_X0208
#define is_kanaconv_x0201 (nkf_compat & KANA_CONV_X0201)
#define is_ascii_conv (nkf_compat & ASCII_CONV_X0208)

#define set_cp932_ext	nkf_compat |= NKF_CP932_EXT
#define set_nocp932_ext	nkf_compat |= NKF_NOCP932_EXT
#define set_nocp932	nkf_compat |= NKF_NOCP932
#define set_no_bfc	nkf_compat |= NKF_NO_BFC
#define is_cp932_ext	(nkf_compat & NKF_CP932_EXT)
#define is_nocp932_ext	(nkf_compat & NKF_NOCP932_EXT)
#define is_nocp932	(nkf_compat & NKF_NOCP932)
#define is_no_bfc	(nkf_compat & NKF_NO_BFC)
#define set_nkf_rotmode nkf_compat |= NKF_ROT_OUTPUT
#define is_nkf_rotmode	(nkf_compat & NKF_ROT_OUTPUT)
#define is_nkf_jbroken	(nkf_compat & INPUT_JIS_BROKEN)
#define is_nkf_jfbroken (nkf_compat & INPUT_JIS_FBROKEN)
#define is_nkf_jffbroken (nkf_compat & INPUT_JIS_LBROKEN) 
#define set_nkf_jbroken nkf_compat |= INPUT_JIS_BROKEN
#define set_nkf_jfbroken nkf_compat |= INPUT_JIS_FBROKEN
#define set_nkf_jffbroken nkf_compat |= INPUT_JIS_LBROKEN

#define set_notrunc_le	nkf_compat |= FOLD_NOTRUNC_LE
#define notrunc_le	(nkf_compat & FOLD_NOTRUNC_LE)

#define no_early_mime_out(x) (x & NO_EARLY_MIME_OUT)
#define set_no_early_mime_out nkf_compat |= NO_EARLY_MIME_OUT
#define reset_no_early_mime_out nkf_compat &= (FULLONE ^ NO_EARLY_MIME_OUT)
#define mime_limit_aware(x) (x & MIME_LIMIT_AWARE)
#define set_mime_limit_aware nkf_compat |= MIME_LIMIT_AWARE

#define is_nkf_convert_hook (nkf_compat & NKF_CONVERT_HOOK)

#define is_nkf_in_place	(nkf_compat & NKF_IN_PLACE)
#define is_nkf_overwrite (nkf_compat & NKF_OVERWRITE)
#define set_nkf_in_place nkf_compat |= NKF_IN_PLACE
#define set_nkf_overwrite nkf_compat |= NKF_OVERWRITE

#define is_nkf_c_hiragana (nkf_compat & NKF_HIRAGANA)
#define set_nkf_c_hiragana nkf_compat |= NKF_HIRAGANA
#define is_nkf_c_katakana (nkf_compat & NKF_KATAKANA)
#define set_nkf_c_katakana nkf_compat |= NKF_KATAKANA

#define is_nkf_no_hk	(nkf_compat & NKF_FORCE_NOHK)
#define set_nkf_no_hk	nkf_compat = (nkf_compat & 0xffefffffUL) | NKF_FORCE_NOHK
#define reset_nkf_no_hk	nkf_compat = (nkf_compat & 0xffefffffUL) 
#define is_o_prefer_ascii	(nkf_compat & O_PREFER_ASC)
#define set_o_prefer_ascii nkf_compat |= O_PREFER_ASC
/* -------------------------------------------------------------- */
/* error ontion: controls behaviors on errors			  */
/* -------------------------------------------------------------- */
extern unsigned long	error_opt;

#define ABT_CONV_ERR 0x00000002UL /* abort on conversion error	  */
#define abt_conv_err (error_opt & ABT_CONV_ERR)
#define set_abt_conv_err error_opt |= ABT_CONV_ERR
#define res_abt_conv_err error_opt &= 0xfffffffdUL

/* -------------------------------------------------------------- */
/* input type detection						  */
/* -------------------------------------------------------------- */
/* for characteristics of types, see included documents.	  */
/* -------------------------------------------------------------- */
extern unsigned long	skf_in_text_type;

#define INTEXT_SGML	0x00000001UL
#define INTEXT_MAILHEAD 0x00000010UL
#define INTEXT_MAILLIKE 0x00000020UL /* may be mail head	  */
#define INTEXT_FTEXT	0x00000100UL /* formatted text		  */
#define INTEXT_MAN	0x00001000UL /* roff format		  */
#define INTEXT_TEX	0x00002000UL /* tex format		  */
#define INTEXT_PLAIN	0x00010000UL /* plain text		  */

#define is_intext_sgml  (skf_in_text_type & INTEXT_SGML)
#define is_intext_mail  (skf_in_text_type & INTEXT_MAILHEAD)
#define is_intext_maillike  (skf_in_text_type & INTEXT_MAILLIKE)
#define is_intext_ftext  (skf_in_text_type & INTEXT_FTEXT)
#define is_intext_man  (skf_in_text_type & INTEXT_MAN)
#define is_intext_tex  (skf_in_text_type & INTEXT_TEX)
#define is_intext_plain (skf_in_text_type & INTEXT_PLAIN)
#define is_intext_undet (skf_in_text_type == 0)

#define set_intext_sgml skf_in_text_type |= INTEXT_SGML
#define set_intext_mail skf_in_text_type |= INTEXT_MAILHEAD
#define set_intext_maillike skf_in_text_type |= INTEXT_MAILLIKE
#define set_intext_ftext skf_in_text_type |= INTEXT_FTEXT
#define set_intext_man skf_in_text_type |= INTEXT_MAN
#define set_intext_tex skf_in_text_type |= INTEXT_TEX
#define set_intext_plain skf_in_text_type |= INTEXT_PLAIN

/* -------------------------------------------------------------- */
/* other misc. variables					  */
/* -------------------------------------------------------------- */
#define MIME_ENCODE_LLIMIT	75
#define MIME_LINE_LIMIT		77

extern int	mime_fold_llimit;
extern int	viscii_escape;
extern int	o_viscii_escape;

/* -------------------------------------------------------------- */
/* Language information(iso639-1)				  */
/* -------------------------------------------------------------- */
#define M_ZH		0x7a48		/* Chinese		  */
#define	M_JP		0x6a41		/* Japanese		  */
#define M_KO		0x6b4f		/* Korian		  */
#define M_VI		0x7649		/* Vietnamese		  */
#define M_FR		0x6652		/* Vietnamese		  */
#define M_DE		0x6445		/* Vietnamese		  */

#define L_NU		0x0000		/* Neutral		  */
#define L_UNI		0x404e		/* Unicode (@N)		  */
#define L_NUN		0x4055		/* Not Unicode (@U)	  */
#define L_EM		0x404d		/* Europian mix		  */
#define L_US		0x4053		/* US			  */

#define L_AA		0x4141		/* Afar			  */
#define L_AB		0x4142		/* Abkhazian		  */
#define L_AE		0x4145		/* Avestan		  */
#define L_AF		0x4146		/* Afrikaans		  */
#define L_AK		0x414a		/* Akan			  */
#define L_AM		0x414d		/* Amharic		  */
#define L_AN		0x414e		/* Aragonese		  */
#define L_AR		0x4152		/* Arabic		  */
#define L_AS		0x4153		/* Assamese		  */
#define L_AV		0x4156		/* Avar			  */
#define L_AY		0x4159		/* Aymara		  */
#define L_AZ		0x415a		/* Azerbbaijani		  */
#define L_BA		0x4241		/* Bashkir		  */
#define L_BE		0x4245		/* Belarusian		  */
#define L_BG		0x4247		/* Burgarian		  */
#define L_BH		0x4248		/* Bihari		  */
#define L_BI		0x4249		/* Bislama		  */
#define L_BN		0x424e		/* Bengali		  */
#define L_BO		0x424f		/* Tibetan		  */
#define L_BR		0x4252		/* Breton		  */
#define L_BS		0x4253		/* Bosnian		  */
#define L_CA		0x4341		/* Catalan		  */
#define L_CE		0x4345		/* Chechen		  */
#define L_CH		0x4348		/* Chamorro		  */
#define L_CO		0x434f		/* Corsican		  */
#define L_CR		0x4352		/* Cree			  */
#define L_CS		0x4353		/* Czech		  */
#define L_CU		0x4355		/* Church Slavic	  */
#define L_CV		0x4356		/* Chuvash		  */
#define L_CY		0x4359		/* Welsh		  */
#define L_DA		0x4441		/* Danish		  */
#define L_DE		0x4445		/* German		  */
#define L_DK		0x444b		/* Denmark		  */
#define L_DV		0x4456		/* Divehi/Maldivian	  */
#define L_EE		0x4545		/* Ewe			  */
#define L_EL		0x454c		/* Greek		  */
#define L_EN		0x454e		/* English		  */
#define L_EO		0x454f		/* Esperanto		  */
#define L_ES		0x4553		/* Spanish		  */
#define L_ET		0x4554		/* Estonian		  */
#define L_EU		0x4555		/* Basque		  */
#define L_FA		0x4641		/* Persian		  */
#define L_FF		0x4646		/* Fulah		  */
#define L_FI		0x4649		/* Finnish		  */
#define L_FJ		0x464a		/* Fijian		  */
#define L_FO		0x464f		/* Faroese		  */
#define L_FI		0x4649		/* Finnish		  */
#define L_FR		0x4652		/* France		  */
#define L_FY		0x4659		/* Western Frisian	  */
#define L_GA		0x4741		/* Irish 		  */
#define L_GB		0x4742		/* Great Britain	  */
#define L_GD		0x4744		/* Gaelic		  */
#define L_GL		0x474c		/* Galicien		  */
#define L_GN		0x474e		/* Gujarani		  */
#define L_GU		0x4755		/* Gujarati		  */
#define L_GV		0x4756		/* Manx			  */
#define L_HA		0x4841		/* Hausa		  */
#define L_HE		0x4845		/* Hebrew		  */
#define L_HI		0x4849		/* Hindi		  */
#define L_HO		0x484f		/* Hiri Motu		  */
#define L_HR		0x4852		/* Croatian		  */
#define L_HT		0x4854		/* Haitian/Haitian Creole */
#define L_HU		0x4855		/* Hungarian		  */
#define L_HY		0x4859		/* Armenian		  */
#define L_HZ		0x485a		/* Herero		  */
#define L_IA		0x4941		/* Inter lingua		  */
#define L_ID		0x4944		/* Indonesian		  */
#define L_IE		0x4945		/* Interlingue		  */
#define L_IG		0x4948		/* Igbo			  */
#define L_II		0x4949		/* Sichuan Yi		  */
#define L_IK		0x494b		/* Inupiaq		  */
#define L_IO		0x494f		/* Ido			  */
#define L_IS		0x4953		/* Icelandic		  */
#define L_IT		0x4954		/* Itarian		  */
#define L_IU		0x4955		/* Inuktitut		  */
#define	L_JA		0x4a41		/* Japanese		  */
#define	L_JI		0x4a49		/* Jidish		  */
#define	L_JP		0x4a50		/* Japanese		  */
#define	L_JV		0x4a56		/* Javanese		  */
#define L_KA		0x4b41		/* Georgian		  */
#define L_KG		0x4b47		/* Kongo		  */
#define L_KI		0x4b49		/* Kikuyu/Gikuyu	  */
#define L_KJ		0x4b4a		/* Kwanyama		  */
#define L_KK		0x4b4b		/* Kazakh		  */
#define L_KL		0x4b4c		/* Greenlandic		  */
#define L_KM		0x4b4d		/* Khmer		  */
#define L_KN		0x4b4e		/* Kannada		  */
#define L_KO		0x4b4f		/* Korian		  */
#define L_KR		0x4b52		/* Kanuri		  */
#define L_KS		0x4b53		/* Kashimiri		  */
#define L_KU		0x4b55		/* Kurdish		  */
#define L_KV		0x4b56		/* Komi			  */
#define L_KW		0x4b57		/* Cornish		  */
#define L_KY		0x4b59		/* Kirghiz		  */
#define L_LA		0x4c41		/* Latin		  */
#define L_LB		0x4c42		/* Luxembourgish	  */
#define L_LG		0x4c47		/* Ganda		  */
#define L_LI		0x4c49		/* Limburger		  */
#define L_LN		0x4c4e		/* Lingala		  */
#define L_LO		0x4c4f		/* Lao			  */
#define L_LT		0x4c54		/* Lithuanian		  */
#define L_LU		0x4c55		/* Luba-Katanga		  */
#define L_LV		0x4c56		/* Latvian		  */
#define L_MG		0x4d47		/* Malagasy		  */
#define L_MH		0x4d48		/* Marshallese		  */
#define L_MI		0x4d49		/* Maori		  */
#define L_MK		0x4d4b		/* Macedonian		  */
#define L_ML		0x4d4c		/* Malayalam		  */
#define L_MN		0x4d4e		/* Mongorian		  */
#define L_MO		0x4d4f		/* Moldavian		  */
#define L_MR		0x4d52		/* Marathi		  */
#define L_MS		0x4d53		/* Malay		  */
#define L_MT		0x4d54		/* Maltese		  */
#define L_MY		0x4d59		/* Burmese		  */
#define L_NA		0x4e41		/* Nauru		  */
#define L_NB		0x4e42		/* Norwegian bokmal	  */
#define L_ND		0x4e44		/* Ndebele North	  */
#define L_NE		0x4e45		/* Nepali		  */
#define L_NG		0x4e47		/* Ndonga		  */
#define L_NL		0x4e4c		/* Netherland		  */
#define L_NN		0x4e4e		/* Norwegian Nynorsk	  */
#define L_NO		0x4e4f		/* Norwegian		  */
#define L_NR		0x4e52		/* Ndebele South	  */
#define L_NV		0x4e56		/* Navaho		  */
#define L_NY		0x4e59		/* Chichewa/Nyanja	  */
#define L_OC		0x4f43		/* Occitan (post 1500)	  */
#define L_OJ		0x4f4a		/* Ojibwa		  */
#define L_OM		0x4f4d		/* Oromo		  */
#define L_OR		0x4f52		/* Oriya		  */
#define L_OS		0x4f53		/* Ossetian		  */
#define L_PA		0x5041		/* Panjabi		  */
#define L_PI		0x5049		/* Pali			  */
#define L_PL		0x504c		/* Polish		  */
#define L_PS		0x5053		/* Pushto		  */
#define L_PT		0x5054		/* Portuguese		  */
#define L_QU		0x5155		/* Quechua		  */
#define L_RM		0x524d		/* Raeto-Romance	  */
#define L_RN		0x524e		/* Rundi		  */
#define L_RO		0x524f		/* Romanian		  */
#define L_RU		0x5255		/* Russian		  */
#define L_RW		0x5257		/* Kinyarwanda		  */
#define L_SA		0x5341		/* Sanskrit		  */
#define L_SC		0x5343		/* Sardinian		  */
#define L_SD		0x5344		/* Sindhi		  */
#define L_SE		0x5345		/* Northan Sami		  */
#define L_SG		0x5347		/* Sango		  */
#define L_SI		0x5349		/* Sinhalese		  */
#define L_SK		0x534b		/* Slovak		  */
#define L_SL		0x534c		/* Slovenian		  */
#define L_SM		0x534d		/* Samoan		  */
#define L_SN		0x534e		/* Shona		  */
#define L_SO		0x534f		/* Somali		  */
#define L_SQ		0x5351		/* Albanian		  */
#define L_SR		0x5352		/* Serbian		  */
#define L_SS		0x5353		/* Swati		  */
#define L_ST		0x5354		/* Sotho Southern	  */
#define L_SU		0x5355		/* Sundanese		  */
#define L_SV		0x5356		/* Swedish		  */
#define L_SW		0x5357		/* Swahili		  */
#define L_TA		0x5441		/* Tamil		  */
#define L_TE		0x5445		/* Telugu		  */
#define L_TG		0x5447		/* Tajik		  */
#define L_TH		0x5448		/* Thai			  */
#define L_TI		0x5449		/* Tigrinya		  */
#define L_TK		0x544b		/* Turkmen		  */
#define L_TL		0x544c		/* Tagalog		  */
#define L_TN		0x544e		/* Tswana		  */
#define L_TO		0x544f		/* Tonga		  */
#define L_TR		0x5452		/* Turkish		  */
#define L_TS		0x5453		/* Tsonga		  */
#define L_TT		0x5454		/* Tutar		  */
#define L_TW		0x5457		/* Twi			  */
#define L_TY		0x5459		/* Tahitian		  */
#define L_UK		0x554b		/* Ukrainian		  */
#define L_UG		0x5547		/* Uighur		  */
#define L_UR		0x5552		/* Urdu			  */
#define L_UZ		0x555a		/* Uzbek		  */
#define L_VE		0x5645		/* Venda		  */
#define L_VI		0x5649		/* Vietnamese		  */
#define L_VO		0x564f		/* Volapuk		  */
#define L_WA		0x5741		/* Walloon		  */
#define L_WO		0x574f		/* Wolof		  */
#define L_XH		0x5848		/* Xhosa		  */
#define L_YI		0x5949		/* Yiddish		  */
#define L_YO		0x594f		/* Yoruba		  */
#define L_ZA		0x5a41		/* Zhuang		  */
#define L_ZH		0x5a48		/* Chinese		  */
#define L_ZU		0x5a55		/* Zulu			  */

/* -------------------------------------------------------------- */
/* Nation information						  */
/* -------------------------------------------------------------- */
#define N_BR		0x4252		/* Brazil		  */
#define N_CN		0x434e		/* Continental China	  */
#define N_HK		0x484b		/* HongKong		  */
#define N_JA		0x4a41		/* Japan		  */
#define N_JP		0x4a50		/* Japan		  */
#define N_KR		0x4b52		/* Koria		  */
#define N_PT		0x5054		/* Portugal		  */
#define N_RU		0x5255		/* Russia		  */
#define N_TW		0x5457		/* Taiwan		  */
#define N_US		0x5553		/* United States	  */

#define N_JAJP		0x4a414a50	/* ja_JP		  */
#define N_ZHTW		0x5a485458	/* zh_TW		  */
#define N_ZHCN		0x5a48434e	/* zh_CN		  */
#define N_ENUS		0x454e5553	/* en_US		  */
#define N_ENGB		0x454e4742	/* en_GB		  */
#define N_KOKR		0x4b4f4b52	/* ko_KR		  */
#define N_PTPT		0x50545054	/* pt_PT		  */
#define N_PTBR		0x50544252	/* pt_BR		  */
#define N_THTH		0x55485548	/* th_TH		  */
#define N_VIVN		0x5649564f	/* vi_VN		  */
#define N_RURU		0x52555255	/* ru_RU		  */
#define N_RUUA		0x52555541	/* ru_UA		  */
/* -------------------------------------------------------------- */

#define LANGCD_MSK	0xdfdfUL

#define skf_get_langcode(x)	((x) & LANGCD_MSK)
#define skf_is_ja(x)	(((x) & 0xdfdf) == L_JA)
#define skf_is_strong_lang(x)	((x) & 0x2000UL)
#define skf_set_strong_lang(x)	((x) | 0x2000UL)
#define skf_get_wlang(x)	(x & 0xdfff)
#define skf_neutral_lang(x)	(x < 0x4100)
unsigned long skf_get_valid_langcode P_((unsigned long,unsigned long));

#define output_lang		(skf_output_lang & LANGCD_MSK)
#define input_lang		(skf_input_lang & LANGCD_MSK)

/* -------------------------------------------------------------- */
/* name of the tables						  */
/*   ... is corrupted and just ad hoc.				  */
/* Note: NOT all codeset shown below is supported.		  */
/* -------------------------------------------------------------- */
#define	codeset_ascii	1  /* GL:ascii, GR: -			  */
#define codeset_x0208	2  
#define codeset_jis	2  /* alias				  */ 
	/* a.k.a. iso-2022-jp GL:- GR:x0201 G2:iso8859-1 G3:x0212 */
	/* skf regards this as iso-2022-jp-1			  */
#define codeset_rfc1554	3  
	/* a.k.a. iso-2022-jp-2 GL:- GR:x0201 G2:iso8859-1 G3:-	  */
	/* alternate codeset for G0 is limited to GB 2312	  */
#define codeset_rfc1554_kr	4  
	/* a.k.a. iso-2022-jp-2 GL:- GR:x0201 G2:iso8859-1 G3:-	  */
	/* alternate codeset for G0 is limited to KS X 1001 	  */
#define codeset_x0213	5 /* G0:- G1:x0201 G2:iso8859-1 G3:x0213  */
#define codeset_x0213_s 6 /* a.k.a. iso-2022-jp-3-strict	  */
#define codeset_jis78	7 /* JIS X 0208(1978) a.k.a. C 6226(78)   */
#define codeset_x213a	8 /* iso-2022-jp-3(2004)		  */
#define codeset_kr	9 
	/* a.k.a. iso-2022-kr 7-bit-EUC G0:ascii G1:ksx1001 	  */
#define codeset_cn	10 /* iso-2022-jp style GB2312		  */
#define codeset_eucjp	11 
#define codeset_euc	11 /* alias				  */
		/* euc-jp G0:ascii G1:x0208 G2:0201 G3:0212	  */
#define codeset_euc_213 12
	/* euc-jp-0213 G0:ascii G1:x0213-1 G2:x0201 G2:x0213-2	  */
#define codeset_euc_213a 13 /* euc-jp X-0213:2004		  */
#define codeset_euckr	14 /* euc-kr G0:ascii G1:ksx1001 G2:- G3:-*/
#define codeset_euccn	15 /* euc-cn G0:ascii G1:gb2312 G2:- G3:- */
#define codeset_euctw	16 /* EUC CNS11643			  */
#define codeset_cnhz	17 /* euc-cn with HZ encoding		  */
#define codeset_cnzw	18 /* euc-cn with zW encoding		  */
#define codeset_sjis	19 /* Shift-jis				  */
#define codeset_sj_0213 20 /* Shift-jis'd X-0213:2000		  */
#define codeset_sj_213a	21 /* SJIS X-0213(2004)			  */
#define codeset_cp932	22 /* Shift-jis (cp932)			  */
#define codeset_cp943	23 /* IBM cp943(for OS/2)		  */
#define codeset_sjis78	24 /* JIS X 0208(1978) SJIS version	  */
#define codeset_sjiscl	25 /* SJIS for cellular phone		  */
#define	codeset_cp932w	26  /* MS cp932 (WideByteCharConv compat) */
#define	codeset_cp20932	27  /* MS cp20932 (hairy euc-jp)	  */
#define	codeset_cp51932	28  /* MS cp51932 (cp932'd euc-jp)	  */
#define	codeset_cp5022x	29  /* MS cp50220 (cp932'd jis x208)	  */
#define	codeset_cp50221	30  /* MS cp50221 (cp932'd jis x208)	  */
#define	codeset_cp50222	31  /* MS cp50222 (cp932'd jis x208)	  */
#define codeset_utf16	32 /* UTF-16 (dummy entry)		  */
#define codeset_utf16le	33 /* UTF-16LE (dummy entry)		  */
#define codeset_utf16be	34 /* UTF-16BE (dummy entry)		  */
#define codeset_utf8	35 /* UTF8 (dummy entry)		  */
#define codeset_utf7	36 /* UTF7 (dummy entry)		  */
#define codeset_brgt	37 /* B-Right/V R4			  */
#define codeset_big5	38 /* BIG5 (ETen variant)		  */
#define codeset_big5h	39 /* Big5 (HKU variant)		  */
#define codeset_big5m	40 /* Big5 (Microsoft cp950)		  */
#define codeset_big52	41 /* Big5 2003				  */
#define codeset_big5a	42 /* Big5 Unicode-at-on		  */
#define codeset_big5p	43 /* Big5 plus				  */
#define codeset_gbk	44 /* GBK				  */
#define codeset_johab	45 /* Korian ksx1001 Johab		  */
#define codeset_gb18	46 /* GB 18030				  */
#define codeset_gb12	47 /* GB 12345				  */
#define codeset_uhc	48 /* Korian UHC hangul			  */
#define codeset_isocn	49 /* iso-2022-cn (gb+cns)		  */
#define codeset_isocnc	50 /* iso-2022-cn (cns+gb)		  */
#define codeset_koi8r	51 /* GL:ascii, GR:koi-8-Russian	  */
#define codeset_viscii	52 /* VISCII (rfc1456)			  */
#define codeset_viqr	53 /* VISCII (rfc1456) VIQR encode	  */
#define codeset_vimn	54 /* VISCII (rfc1456) MNEM encode	  */
#define	codeset_vni	55 /* Vietnamese VNI (VNI software co.) */
#define	codeset_vps	56  /* Vietnamise VPS			  */
#define	codeset_8859_1	57  /* GL:ascii, GR:iso8859-1		  */
#define	codeset_8859_2	58  /* GL:ascii, GR:iso8859-2		  */
#define	codeset_8859_3	59  /* GL:ascii, GR:iso8859-3		  */
#define	codeset_8859_4	60  /* GL:ascii, GR:iso8859-4		  */
#define	codeset_8859_5	61  /* GL:ascii, GR:iso8859-5		  */
#define	codeset_8859_6	62  /* GL:ascii, GR:iso8859-6		  */
#define	codeset_8859_7	63  /* GL:ascii, GR:iso8859-7		  */
#define	codeset_8859_8	64  /* GL:ascii, GR:iso8859-8		  */
#define	codeset_8859_9	65  /* GL:ascii, GR:iso8859-9		  */
#define	codeset_8859_10	66  /* GL:ascii, GR:iso8859-10		  */
#define	codeset_8859_11	67  /* GL:ascii, GR:iso8859-11		  */
#define	codeset_8859_13	68  /* GL:ascii, GR:iso8859-13		  */
#define	codeset_8859_14	69  /* GL:ascii, GR:iso8859-14		  */
#define	codeset_8859_15	70  /* GL:ascii, GR:iso8859-15		  */
#define	codeset_8859_16	71  /* GL:ascii, GR:iso8859-16		  */
#define codeset_ibm	72  /* IBM MVS DBCS			  */
#define	codeset_ibm931	73  /* IBM codepage 931(Japanese)	  */
#define	codeset_ibm933	74  /* IBM codepage 933(Korian)		  */
#define	codeset_ibm935	75  /* IBM codepage 935(Simpl.Chinese)	  */
#define	codeset_ibm937	76  /* IBM codepage 937(Trad.Chinese)	  */
#define codeset_keis	77 /* KEIS 83/90			  */
#define codeset_jef	78 /* Fujitsu JEF			  */
#define codeset_jefl	79 /* Fujitsu-JEF with small-latin EBCDIC */
#define codeset_nec	80 /* NEC external DBCS			  */
#define	codeset_natsf	81  /* NATS for Finland/Sweden		  */
#define	codeset_natsd	82  /* NATS for Denmark/Norway		  */
#define codeset_cp1250   83 /* GL: ascii GR: cp1250		  */
#define codeset_cp1251   84 /* GL: ascii GR: cp1251		  */
#define	codeset_bs4730	 85  /* GB BS 4730			  */
#define	codeset_nfz62010 86  /* French NF Z 62010		  */
#define	codeset_din66083 87  /* German DIN 66083 		  */
#define	codeset_macroman 88  /* Macintosh < OS9 roman		  */
#define	codeset_macCE	 89  /* Macintosh < OS9 CE		  */
#define	codeset_macdevang 90 /* Macintosh < OS9 devanagari	  */
#define	codeset_maccyrl  91  /* Macintosh < OS9 Cyrillic	  */
#define	codeset_macturka 92  /* Macintosh < OS9 Turkish		  */
#define	codeset_macgreek 93  /* Macintosh < OS9 greek		  */
#define	codeset_maciceln 94  /* Macintosh < OS9 Icelandic	  */
#define	codeset_macgujar 95  /* Macintosh < OS9 gujarati	  */
#define	codeset_macgurmu 96  /* Macintosh < OS9 gurmukhi	  */
#define	codeset_maccroatian 97 /* Macintosh < OS9 croatian	  */
#define	codeset_macromna 98  /* Macintosh < OS9 Romanian	  */
#define	codeset_armscii8 99 /* Armscii-8 (Armenian)		  */
#define	codeset_geostd	 100 /* Geostd-8 (Gursian)		  */
#define	codeset_iscii	 101 /* ISCII Devanagari (base)		  */
#define codeset_transp	 102 /* psuedo code: transparent	  */
#define codeset_var	 103 /* psuedo code: variables		  */
#define codeset_gsm0338	 104 /* Cellular phone GSM 03.38	  */
#define codeset_uri	 105 /* Unicode/URI			  */
#define codeset_puny	 106 /* Unicode/URI ACE(Punycode)	  */
#define codeset_cp437	 107 /* cp 437 (MS/IBM PC US)		  */
#define codeset_cp1252	 108 /* cp1252 (Windows Latin-1)	  */
#define codeset_utf8n	 109 /* UTF-8N (UTF-8 with BOM)		  */
#define codeset_iso2022jp1 110 /* iso-2022-jp-1			  */
#define codeset_utf32	 111 /* UTF-32 (dummy entry)		  */
#define codeset_utf32le  112 /* UTF-32LE (dummy entry)		  */
#define codeset_utf32be  113 /* UTF-32BE (dummy entry)		  */
#define codeset_jisms	 114 /* cp932 based jis x0208		  */
#define codeset_x0208nj	 115 /* new jouyou-kanji		  */
#define codeset_sjisau	 116 /* Shift JIS with AU glyph		  */
#define codeset_sjissb	 117 /* Shift JIS with S*ftBank glyph	  */
#define codeset_sjisontt 118 /* Shift JIS with old-ntt glyph	  */
#define	codeset_nyukan_utf8 119  /* nyukan gaiji utf8		  */
#define	codeset_nyukan_utf16 120  /* nyukan gaiji utf8		  */
#define	codeset_koi8u	 121  /* KOI8 Ukraine			  */
#define	codeset_binary	 122  /* a.k.a. ASCII-8bit		  */
#define codeset_utf16leb 123 /* UTF-16LE with BOM 		  */
#define codeset_utf16beb 124 /* UTF-16BE with BOM		  */
#define codeset_utf32leb 125 /* UTF-32LE with BOM		  */
#define codeset_utf32beb 126 /* UTF-32BE with BOM		  */
#define	codeset_locale	 127  /* no codeset is given		  */
#define	codeset_aribb24s 128  /* ARIB-B24-SI(SJIS)		  */
#define	codeset_aribb24	 129  /* ARIB-B24-SI(JIS)		  */
#define	codeset_aribb24sa 130  /* ARIB-B24-SI-ALT(SJIS)		  */
#define	codeset_aribb24a  131  /* ARIB-B24-SI-ALT(JIS)		  */
#define codeset_cnhz8	 132 /* GB2312 HZ8			  */
#define codeset_end	 133 /* psuedo code: end-point		  */

/* -------------------------------------------------------------- */
struct long_option {
	char *option;
	int	index;
};

#define is_in_ucs_ufam ((in_codeset == codeset_utf16le) || \
	(in_codeset == codeset_utf16be) || \
	(in_codeset == codeset_utf8) || \
	(in_codeset == codeset_transp) || \
	(in_codeset == codeset_utf7) || \
	(in_codeset == codeset_puny))

#define is_in_ucs_ufamnomime ((in_codeset == codeset_utf16le) || \
	(in_codeset == codeset_utf16be) || \
	(in_codeset == codeset_transp) || \
	(in_codeset == codeset_utf7) || \
	(in_codeset == codeset_puny))
/* -------------------------------------------------------------- */
/* iscii character properties					  */
/* -------------------------------------------------------------- */
#define	ISCII_BLD	0xd030	/* Bold */
#define	ISCII_ITA	0xd031	/* Italic */
#define	ISCII_UL	0xd032	/* Underline */
#define	ISCII_EXP	0xd033	/* Expanded */
#define	ISCII_HLT	0xd034	/* Highlight */
#define	ISCII_OTL	0xd035	/* Outline */
#define	ISCII_SHD	0xd036	/* Shadow */
#define	ISCII_TOP	0xd037	/* Double height top */
#define	ISCII_LOW	0xd038	/* Double height bottom */
#define	ISCII_DBL	0xd039	/* Double size row */
#define ISCII_DEF	0xd03a	/* default */
#define ISCII_RMN	0xd03b	/* Roman */

#define ISCII_ARB	0xd03c	/* Arabic */
#define ISCII_PES	0xd03d	/* Persian */
#define ISCII_URD	0xd03e	/* Urdu */
#define ISCII_SND	0xd03f	/* Sindhi */
#define ISCII_KSM	0xd040	/* Kashmiri */
#define ISCII_PST	0xd041	/* Pushto */

/* -------------------------------------------------------------- */
/* canonical name handler					  */
/* -------------------------------------------------------------- */
extern const struct long_option codeset_option_code[];
/* -------------------------------------------------------------- */
extern int	cname_comp	P_ ((char *, char *));
extern int 	skf_search_cname P_((char *));
extern int 	skf_search_chname P_((char *));
extern int	skf_option_parser \
			P_((char *,const struct long_option *));
#define SG_SHIFTLEN	7
#define SG_HIMASK	0x780
#define SG_LOMASK	0x07f

/* -------------------------------------------------------------- */
/* hook for output handling change				  */
/* -------------------------------------------------------------- */
/* generics */
extern	void	oconv P_((skf_ucode));
extern  int	oconv_init();

/* --------------------------------------------------------------- */
/* output side encoder						   */
/* --------------------------------------------------------------- */
extern	void	o_c_encode P_((int));
extern	void	o_p_encode P_((skf_ucode));
extern  int	mime_clip_test P_((int,int));

/* -------------------------------------------------------------- */
/* post oconv side						  */
/* -------------------------------------------------------------- */
extern	void	post_oconv P_((skf_ucode));
/* -------------------------------------------------------------- */
extern void	o_latin_conv P_((skf_ucode));
extern void	o_surrg_conv P_((skf_ucode));
extern void	o_ozone_conv P_((skf_ucode));
extern void	o_private_conv P_((skf_ucode,skf_ucode));

/* -------------------------------------------------------------- */
/* actual oconverter reference definitions			  */
/* -------------------------------------------------------------- */
extern void	JIS_ascii_oconv P_((skf_ucode));
extern void	JIS_latin_oconv P_((skf_ucode));
extern void	JIS_cjkkana_oconv P_((skf_ucode));
extern void	JIS_cjk_oconv P_((skf_ucode));
extern void	JIS_ozone_oconv P_((skf_ucode));
extern void	JIS_compat_oconv P_((skf_ucode));
extern void	JIS_private_oconv P_((skf_ucode));

extern void	SJIS_ascii_oconv P_((skf_ucode));
extern void	SJIS_latin_oconv P_((skf_ucode));
extern void	SJIS_cjkkana_oconv P_((skf_ucode));
extern void	SJIS_cjk_oconv P_((skf_ucode));
extern void	SJIS_ozone_oconv P_((skf_ucode));
extern void	SJIS_compat_oconv P_((skf_ucode));
extern void	SJIS_private_oconv P_((skf_ucode));

extern void	EUC_ascii_oconv P_((skf_ucode));
extern void	EUC_latin_oconv P_((skf_ucode));
extern void	EUC_cjkkana_oconv P_((skf_ucode));
extern void	EUC_cjk_oconv P_((skf_ucode));
extern void	EUC_ozone_oconv P_((skf_ucode));
extern void	EUC_compat_oconv P_((skf_ucode));
extern void	EUC_private_oconv P_((skf_ucode));

extern void	UNI_ascii_oconv P_((skf_ucode));
extern void	UNI_latin_oconv P_((skf_ucode));
extern void	UNI_cjkkana_oconv P_((skf_ucode));
extern void	UNI_cjk_oconv P_((skf_ucode));
extern void	UNI_ozone_oconv P_((skf_ucode));
extern void	UNI_compat_oconv P_((skf_ucode));
extern void	UNI_private_oconv P_((skf_ucode,skf_ucode));

extern void	KEIS_ascii_oconv P_((skf_ucode));
extern void	KEIS_latin_oconv P_((skf_ucode));
extern void	KEIS_cjkkana_oconv P_((skf_ucode));
extern void	KEIS_cjk_oconv P_((skf_ucode));
extern void	KEIS_ozone_oconv P_((skf_ucode));
extern void	KEIS_compat_oconv P_((skf_ucode));
extern void	KEIS_private_oconv P_((skf_ucode));

extern void	BG_ascii_oconv P_((skf_ucode));
extern void	BG_latin_oconv P_((skf_ucode));
extern void	BG_cjkkana_oconv P_((skf_ucode));
extern void	BG_cjk_oconv P_((skf_ucode));
extern void	BG_ozone_oconv P_((skf_ucode));
extern void	BG_compat_oconv P_((skf_ucode));
extern void	BG_private_oconv P_((skf_ucode));

extern void	BRGT_ascii_oconv P_((skf_ucode));
extern void	BRGT_latin_oconv P_((skf_ucode));
extern void	BRGT_cjkkana_oconv P_((skf_ucode));
extern void	BRGT_cjk_oconv P_((skf_ucode));
extern void	BRGT_ozone_oconv P_((skf_ucode));
extern void	BRGT_compat_oconv P_((skf_ucode));
extern void	BRGT_private_oconv P_((skf_ucode));

#if 0
/* ??? */
/* -------------------------------------------------------------- */
/* actual character count reference definitions			  */
/* -------------------------------------------------------------- */
extern void	JISc_ascii_oconv P_((skf_ucode));
extern void	JISc_latin_oconv P_((skf_ucode));
extern void	JISc_cjkkana_oconv P_((skf_ucode));
extern void	JISc_cjk_oconv P_((skf_ucode));
extern void	JISc_ozone_oconv P_((skf_ucode));
extern void	JISc_compat_oconv P_((skf_ucode));
extern void	JISc_private_oconv P_((skf_ucode));

extern void	SJISc_ascii_oconv P_((skf_ucode));
extern void	SJISc_latin_oconv P_((skf_ucode));
extern void	SJISc_cjkkana_oconv P_((skf_ucode));
extern void	SJISc_cjk_oconv P_((skf_ucode));
extern void	SJISc_ozone_oconv P_((skf_ucode));
extern void	SJISc_compat_oconv P_((skf_ucode));
extern void	SJISc_private_oconv P_((skf_ucode));

extern void	EUCc_ascii_oconv P_((skf_ucode));
extern void	EUCc_latin_oconv P_((skf_ucode));
extern void	EUCc_cjkkana_oconv P_((skf_ucode));
extern void	EUCc_cjk_oconv P_((skf_ucode));
extern void	EUCc_ozone_oconv P_((skf_ucode));
extern void	EUCc_compat_oconv P_((skf_ucode));
extern void	EUCc_private_oconv P_((skf_ucode));

extern void	UNIc_ascii_oconv P_((skf_ucode));
extern void	UNIc_latin_oconv P_((skf_ucode));
extern void	UNIc_cjkkana_oconv P_((skf_ucode));
extern void	UNIc_cjk_oconv P_((skf_ucode));
extern void	UNIc_ozone_oconv P_((skf_ucode));
extern void	UNIc_compat_oconv P_((skf_ucode));
extern void	UNIc_private_oconv P_((skf_ucode));

extern void	KEISc_ascii_oconv P_((skf_ucode));
extern void	KEISc_latin_oconv P_((skf_ucode));
extern void	KEISc_cjkkana_oconv P_((skf_ucode));
extern void	KEISc_cjk_oconv P_((skf_ucode));
extern void	KEISc_ozone_oconv P_((skf_ucode));
extern void	KEISc_compat_oconv P_((skf_ucode));
extern void	KEISc_private_oconv P_((skf_ucode));

extern void	BGc_ascii_oconv P_((skf_ucode));
extern void	BGc_latin_oconv P_((skf_ucode));
extern void	BGc_cjkkana_oconv P_((skf_ucode));
extern void	BGc_cjk_oconv P_((skf_ucode));
extern void	BGc_ozone_oconv P_((skf_ucode));
extern void	BGc_compat_oconv P_((skf_ucode));
extern void	BGc_private_oconv P_((skf_ucode));

extern void	BRGTc_ascii_oconv P_((skf_ucode));
extern void	BRGTc_latin_oconv P_((skf_ucode));
extern void	BRGTc_cjkkana_oconv P_((skf_ucode));
extern void	BRGTc_cjk_oconv P_((skf_ucode));
extern void	BRGTc_ozone_oconv P_((skf_ucode));
extern void	BRGTc_compat_oconv P_((skf_ucode));
extern void	BRGTc_private_oconv P_((skf_ucode));
#endif

/* -------------------------------------------------------------- */
/* code end treatment						  */
/* -------------------------------------------------------------- */
extern void	EUC_finish_procedure();
extern void	SJIS_finish_procedure();
extern void	JIS_finish_procedure();
extern void	utf7_finish_procedure();
extern void	utf8_finish_procedure();
extern void	ucod_finish_procedure();
extern void	BG_finish_procedure();
extern void	BRGT_finish_procedure();

/* -------------------------------------------------------------- */
/* windows filename limitation hooker				  */
/* -------------------------------------------------------------- */
extern const char	charname_conv[];
extern const char	charname_priv_conv[];

/* -------------------------------------------------------------- */
/* error code assist: in error.c				  */
/* -------------------------------------------------------------- */
extern const char	*rev;
extern void	display_help();
extern void	display_version P_((int));
extern void	display_nkf_help();

extern /*@noreturn@*/ void	skferr P_((int, long, long));
extern void	skf_openerr P_((char *,int));
extern void	skf_readerr P_((int));
extern void	error_code_option P_((int));
extern void	error_extend_option P_((int, /*@null@*/ char *));

extern void	show_endian_out();
extern void	initialize_error();
extern void	debug_analyze();

extern void	in_undefined P_((skf_ucode, int));
extern void	out_undefined P_((skf_ucode, int));

extern void	skf_incode_display();
extern void	skf_outcode_display();
extern void	test_support_codeset();
extern void	test_support_charset();
extern void	in_tablefault P_((int, const char *));
extern void	out_tablefault P_((int));
extern void	ValidValueDisplay P_((int, char *));
extern void	ValidLangDisplay P_((char *));
extern void	dump_name_of_lineend P_((int, int));

extern char	*in_file_name;	/* saved input file name	  */

#ifndef SWIG_EXT
extern int	get_output_locale();
#endif

/* in/out_undefined() */
#define SKF_IUNDEF	1	/* unsupported input codepoint	   */
#define SKF_IRGTUNDEF	5	/* undefined in right side	   */
#define SKF_IOUTUNI	9	/* unsupported but defined, or c1  */
#define SKF_UNDEFCSET	10	/* undefined charset:esc_process.c */
#define SKF_OUTTABLE	11	/* codepoint is out of table	   */
#define SKF_UNEXPEOF	12	/* unexpected EOF		   */
#define SKF_NOSURRG	13	/* lower surrogate is missing	   */
#define SKF_IBROKEN	14	/* input char. sequence is wrong.  */
#define SKF_UNSPRT	15	/* defined but not in unic*de(in)  */
#define SKF_MIME_ERR	16	/* MIME decode error case	   */
#define SKF_ENC_ERR	17	/* encode error case		   */
#define SKF_OUT_PROHIBIT 18	/* output is prohibited(idn)	   */
#define SKF_DECODERR	19	/* unexpected char/length in decode */
#define SKF_NOINTABLE	20	/* undefined input table(system?)  */
#define SKF_UNILANGER	21	/* language tag is broken	   */
#define SKF_UNSURG	22	/* not within UTF-32		   */
#define SKF_DECINCONS	23	/* decode enabled under UTF16	   */
#define SKF_UNSUPP	24	/* unsupported sequence		   */
#define SKF_NOOUT	25	/* unsupported for output only 	   */
#define SKF_NOTABLE	26	/* unsupported by skf		   */

#define SKF_AUTOFAIL	28	/* autodetect failure		   */
#define SKF_ARIBERR	29	/* arib unsupport or inconsistent */

#define SKF_KANAUNDEF	42	/* this does not occur?		  */
#define SKF_UX0212	43	/* X-0212			  */
#define	SKF_OUNDEF	44	/* all else case		  */
#define	SKF_UCOMPAT	45	/* compatibility plane case	  */
#define	SKF_OLDWARN	46	/* new jis to old jis output	  */

/* read process	*/
#define SKF_OPENERR	30	/* open() fails			  */
#define SKF_READERR	31	/* read() fails			  */

/* in_tablefault() */
#define SKF_TBLUNDEF	50
#define SKF_TBLBROKN	51
#define SKF_UND_MIME	52	/* undefined MIME charset	   */
#define SKF_TBLNSUPPRT	53
#define SKF_PRESETFAIL	54
#define SKF_TBLSHORT	55
#define SKF_TBLINCNSIS	56
#define SKF_OTBLINCNSIS	57

/* error_code_option() */
#define SKF_UNDEFSNBYCH 60
#define SKF_MISCSETOPT	61
#define SKF_UNKWNCSTOPT 62
#define SKF_UNKWNCDOPT	63

/* error_extend_option() */
#define SKF_DEPRECATOPT	64
#define SKF_UNDEFOPT	65
#define SKF_NKFINCOMPAT 66
#define SKF_UNDEFCARGS	67	/* undefined charset in arguments  */
#define SKF_UNDEFCARGH	68	/* undefined codeset in arguments  */
#define SKF_NOCSET	69	/* no codeset in arguments	  */

/* skferr() */
#define SKF_MALLOCERR		70
#define SKF_IBUFERR		71
#define SKF_OBUFERR		72
#define SKF_OBUFREERR		73
#define SKF_PUTFAILERR		74
#define SKF_EUCPRESETERR	78
#define SKF_TBLALLOCERR		80
#define SKF_DECOMPERR		81	/* during NFD/NFKD */
#define SKF_INTERNALERR		82
#define SKF_DEBUGERR_1		83
#define SKF_DEBUGERR_2		84

#define SKF_ACEBUFOUT		86

#define SKF_TABLEERR_G0A	91
#define SKF_TABLEERR_G0		92
#define SKF_TABLEERR_G1		93
#define SKF_TABLEERR_G2		94
#define SKF_TABLEERR_G3		95
#define SKF_TABLEERR_O		96

#define SKF_ERRDUMP		100
#define SKF_LOW2ERRDUMP		101
#define SKF_UP2ERRDUMP		110
#define SKF_DECERRDUMP		111
#define SKF_DECERRDUMP2		112
#define SKF_OUTPATCHERR		200

/* -------------------------------------------------------------- */
/* arib specific tables						  */
/* -------------------------------------------------------------- */
extern skf_ucode	**arib_macro_tbl;
#define is_arib		((in_codeset >= codeset_aribb24s) && (in_codeset <= codeset_aribb24))

/* --- SWIG-EXTENSION related fixes ---------------------------- */
extern void	fold_value_setup();
extern int	skf_in_converter();

#ifdef SWIG_EXT
extern int	swig_state;
extern int 	skf_script_param_parse P_((char *,int));
extern void 	skf_script_init();
extern int	skf_swig_result;  /* summarize error result.	 */


/* --- do not use built-in locale features --------------------- */
#undef ENABLE_NLS
#endif

#endif
