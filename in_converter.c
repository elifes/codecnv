/* *******************************************************************
** Copyright (c) 2000-2015 Seiji Kaneko. All rights reserved.
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
    in_converter.c	input-side converter
    $Id: in_converter.c,v 1.177 2017/01/05 15:05:48 seiji Exp seiji $
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include "skf.h"
#include "skf_fileio.h"
#include "oconv.h"
#include "convert.h"
#include "in_code_table.h"

#define GBK_UOFFSET	0x81
#define BIG5_UOFFSET	0xa1
#define BIG5P_UOFFSET	0x81

static skf_ucode   viqr_parse P_((int,int,int,unsigned long));
static int	   reput_c1 = sOCD;

static skf_ucode   get_in_shift_value P_((int));

static int	ms_in_calc_index P_((int,int));
static int	ms213_in_calc_index P_((int,int));
#define	mscel_in_calc_index	ms_in_calc_index
static int	johab_in_calc_index P_((int,int));
static int	big5_in_calc_index P_((int,int));
static int	big5p_in_calc_index P_((int,int));
static int	big5c_in_calc_index P_((int,int,int));
static int	gbk_in_calc_index P_((int,int));
static int	gb2k_in_calc_index P_((int,int));

#define hku_in_calc_index big5_in_calc_index

#ifdef OFFSET_CALC_ENBL
static int	thru_in_calc_index P_((int,int));
#endif

static int	viqr_in_calc_index P_((int,int));
static int	vni_in_calc_index P_((int,int));
static int	iso_in_calc_index P_((int,int));
static int	hz8_in_calc_index P_((int,int));
static int	dmy_in_calc_index P_((int,int));
static int	nniso_in_calc_index P_((int,int));
static int	iscii_in_calc_index P_((int,int));
static void	gbk_area5_conv P_((int,int,int,int,int,int));
static void	rpclockparse P_((skf_ucode,int));

/* --- SJIS X-0213 conversion map	-------------------------- */
const int x213_sjis_map[10] = {
    0x21,0x28,0x23,0x24,0x25,0x2c,0x2d,0x2e,0x2f,0x6d
};

#ifdef FAST_MULT
#define    index_cal(c1,c2) (((c2 - 0x21) * 94) + c1 - 0x21)
#else
#define	   index_cal(c1,c2) (mul94[c2 - 0x21] + c1 - 0x21)
#endif

static  unsigned short *gb2k_a5_tbl = NULL;

#if	defined(ROT_SUPPORT) && !defined(NEW_ROT_CODE)
/* for rot13 ---------------- */
static const unsigned char rot13_tbl[58] = {
   'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   'A','B','C','D','E','F','G','H','I','J','K','L','M',
   0x5b,0x5c,0x5d,0x5e,0x5f,0x60,
   'n','o','p','q','r','s','t','u','v','w','x','y','z',
   'a','b','c','d','e','f','g','h','i','j','k','l','m'
};

int skf_rot47conv(c2)
int c2;
{
    if ((c2 > A_SP) && (c2 < A_DEL)) {
	return (((c2 < 'P') ? (c2 + 47) : (c2 - 47)));
    } else return(c2);
}

int skf_rot13conv(c1)
int c1;
{
    if ((c1 < 'A') || (c1 > 'z') || (c1 == sEOF)) return(c1);
	    else return(rot13_tbl[c1 - 'A']);
}
#endif

/* --------------------------------------------------------------- */
void in_sbroken(c2,c1)
int c2,c1;
{
    in_undefined((c2 << 8)+c1,SKF_IUNDEF);
}

/* --------------------------------------------------------------- */
/* Second major loop: called after preConvert			   */
/*   for faster conversion. Code is already established.           */
/*   jis 8-bit(with kana) is processed as MS-JIS		   */
/* --------------------------------------------------------------- */
/*  e_in: EUC(Japanese), MS-Code and JIS to UCS4 converter	   */
/* --------------------------------------------------------------- */
/*   Note: U+D800 to DFFF is used for internal skf handling	   */
/*    D800-D87f: packed JIS X-0213 ligatures. See lig_x0213_out	   */
/* --------------------------------------------------------------- */
/* Note for error operation related single shift		   */
/*   Refer SVR4 MNLS feature specification from USL		   */
/* character after single shift is treated as bit8=1 in EUC. 	   */
/*	g0 designated to jis-x0201 (or ascii)			   */
/*	g1 designated to jis-x0208				   */
/*	g2 designated to jis-x0201 kana part			   */
/*	g3 designated to jis-x0212				   */
/* --------------------------------------------------------------- */
/* Note for Micr*soft cp50220,50221,50222			   */
/*  These codeset is handled by e_in. It's just iso-2022-jp with   */
/*  NEC/IBM gaiji with private area hiccups.			   */
/* --------------------------------------------------------------- */

/*@-nullderef@*/ /* low_table is checked before here.		   */
int e_in(f)			/* euc input			   */
skfFILE *f;			/* can process euc, euc+jis-mix	   */
{				/*  ... jis code contains no kana  */
    register int    c1, c2 = 0;
    skf_ucode   k0;		/* utf32 result			   */
    int    ch;
    int	   c2s,c1s;
    /* Note: e_in doesn't preserve tail BS, because no following   */
    /*  modifier character(s) is given.				   */
    unsigned long	encode;
    int		is_nkf_euc = FALSE;
    int		rpclock = 0;

    encode = i_codeset[in_codeset].encode;
    if (is_nkf_compat && ((in_codeset == codeset_eucjp)
			|| (in_codeset == codeset_jis)
			|| (in_codeset == codeset_cp5022x)
			|| (in_codeset == codeset_cp50221)
			|| (in_codeset == codeset_cp50222)
			|| (in_codeset == codeset_cp20932)
			|| (in_codeset == codeset_cp51932))) {
	is_nkf_euc = TRUE;
	if (is_nkf_rotmode) return(be_in(f));
    } else;
    if (is_nkf_compat && (is_nkf_jbroken || is_nkf_jffbroken) &&
    			(  (in_codeset == codeset_jis)
			|| (in_codeset == codeset_cp5022x)
			|| (in_codeset == codeset_cp50221)
			|| (in_codeset == codeset_cp50222))) {
	return(be_in(f));
    } else;

    /* wrong state change guard */
    if (low_table == NULL) {
    	if (g0_table_mod == NULL)
		g0_table_mod = &(iso_unibyte_defs[ascii_index]);
	g0table2low();
    } else;

    /* --- conversion loop --------------------------------------- */
    while (TRUE) {
	if (c2) {       /* ***** second byte ********************* */
	    c2s = c2;	/* save for later test			   */
	    if ((c1 = vGETC(f)) == sEOF) {
			/* c2 is rot'd even if c1 is EOF	   */
		if (!encode_enbl || !is_mime_nkfmode) {
			in_undefined(c2,SKF_UNEXPEOF);
		};
		break;
	    } else if (c1 == sOCD) return(sOCD);
	    c1s = c1;
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x",c1);
#endif
	    if ((is_multibyte(up_dbyte) && (c1 > A_KSP)) || in_single_shift) {
		c2 &= 0x7f; c1 &= 0x7f;
		if (c1 == 0x7f) { /* DEL with bit8 == 1 is bogus   */
		    in_sbroken(c2,c1);
		    shift_cond_recovery(); c2 = 0; continue;
		};
	    } else if ((c2 >= 0x7f) && is_cp5022x(encode) 
			    && (c1 < A_DEL)) {
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr," cp5022x_gaiji");
#endif
		k0 = ((c2 - 0x7f) * 94) + (c1 - 0x21);
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"(cp5022x:%4x)",k0);
#endif
		if (k0 <= 0x757) {
		    oconv(k0 + 0xe000);
		} else if (k0 < 0x774) {
		    oconv(uni_k_ibm_fa[k0 - 0x758]);
		} else if (k0 < 0x8dc) {
		    oconv(uni_t_x208[k0 - 0x774 + 0x2050]);
		} else in_sbroken(c2,c1);
		shift_cond_recovery();
		c2 = 0; continue;
	    } else if ((c1 > 0x20) && (c1 < A_DEL) && low_dbyte) {
		;	 	/* JIS case: pass as it is	   */
	    } else if (is_euc_mseuc(encode) && (c2 > A_KSP)) {
		if ((c1 <= A_KSP) && !is_euc_20932(encode)) {
		    if (c2 >= 0xf0) {
			k0 = ((c2 - 0xf0) * 188) + (c1 - 0x40);
			if (c1 >= 0x7f) k0--;
			oconv(U_PRIV + k0); 
		    } else {
			in_undefined(c1,SKF_IUNDEF);
		    };
		    if (in_single_shift) shift_cond_recovery();
		    c2 = 0; continue;   /* goto next_word	   */
		};
		c2 &= 0x7f; 	/* Vendor EUC case: pass as it is  */
		c1 &= 0x7f;
	    } else { 		/* inconsistent case:		   */
		/* case: is_jis c1 < A_SP			   */
		/* 	 is_jis c1 >= DEL 			   */
		/* 	 is_euc c1 <= DEL 			   */
		in_undefined(c2,SKF_IBROKEN);
		if (c1 == A_ESC) { /* proceed anyway.		   */
		    if ((c1 = esc_process(f)) < 0) return(c1);
		} else in_undefined(c1,SKF_IBROKEN);
		shift_cond_recovery();
		c2 = 0; continue;   	/* goto next_word          */
	    };
#if	defined(ROT_SUPPORT) && !defined(NEW_ROT_CODE)
     	    if (is_rot_encoded) {/* double byte: rot47		   */
	    	c2 = skf_rot47conv(c2);
	    	c1 = skf_rot47conv(c1);
	    };
#endif
	    /* --- 0x20 < c2 < 0x7f, 0x20 < c1 < 0x7f ------------ */
	    /* Note: EUC, MS-JIS and JIS come here with low_dbyte  */
	    k0 = 0;		/* clear result value 		   */

	    ch = index_cal(c1,c2);

#ifdef SKFDEBUG
	    if (is_vv_debug) {
		fprintf(stderr,"(%d)",ch); (void)fflush(stderr);
	    };
#endif
		/* normal JIS(X-0208, X-0212) case.	   */
	    if ((ch >= X0208_KANJI_END) && (use_cde_compat) &&
		(((c2s < A_DEL) && (is_tbl_kanji(low_kana)))
		    || ((c2s > A_KSP) && (is_tbl_kanji(up_kana))))) {
		    		/* X-0208 & CDE	   */
		if (((c2s < A_DEL) && (is_tbl_x0208(low_kana)))
		    || ((c2s > A_KSP) && (is_tbl_x0208(up_kana)))) {
		    k0 = UNI_UDF_A + ch - X0208_KANJI_END;
		} else {	/* X-0212 */
		    k0 = UNI_UDF_B + ch - X0208_KANJI_END;
		};
	    } else if (((c2s < A_DEL) && (ch < low_table_limit))
		     || ((c2s > A_KSP) && (ch <up_table_limit))) {
		if (is_euc_mseuc(encode) || is_cp5022x(encode)) {
		    if (ch < X0208_KANJI_END) {
			if (is_cp5022x(encode)) k0 = low_table[ch];
			else k0 = up_table[ch];
		    } else if ((((c2 >= 0x75) && (c2 < 0x79))
			    || ((c2 >= 0x7d) && (c2 <= 0x7e)))
			    && (c1 > A_SP)
			    && (is_euc_mseuc(encode))) {
			k0 = ((c2s - 0xf5) * 94)
			    + (c1s - 0xa1) + 0xe40c;
		    } else if ((c2 < 0x7d) &&
			(((c1 > A_SP) && (is_cp5022x(encode))) ||
			(((c1s > A_KSP)
			&& (is_euc_mseuc(encode)))))) { /* NEC Gaiji   */
			if (is_cp5022x(encode)) {
			    if (is_tbllong(low_table_mod->char_width))
			    	k0 = low_ltable[ch];
			    else k0 = low_table[ch];
			} else {
			    if (is_tbllong(up_dbyte)) k0 = up_ltable[ch];
			    else k0 = up_table[ch];
			};
		    } else if (((c2s >= 0xf5) &&
			    (c1s < 0xa1) && (c1s != A_DEL)) ||
			    ((c2s == 0xf4) &&
			     ((c1s < 0xa1) || (c1s > 0xa6)))) {
		    /* cp932 - cp51932 conversion bug recovery */
			k0 = 0xe000 + ((c2 - 0xf0) * 188) 
			    + (c1 - 0x40) + ((c1 < A_DEL) ? 0 : 1);
		    } else;
		} else if (c2s > A_KSP) {
			/* normal EUC case.			   */
		    if (c1s > A_KSP) { /* normal EUC & JIS	   */
			if (is_tbllong(up_dbyte)) k0 = up_ltable[ch];
			else k0 = up_table[ch];
		    } else if (gx_table_mod->table_len != 0) {
			if (ch < gx_table_mod->table_len) {
			    if (is_tbllong(gx_table_mod->char_width))
				k0 = gx_table_mod->uniltbl[ch];
			    else k0 = gx_table_mod->unitbl[ch];
			} else k0 = 0;
		    } else k0 = 0;
		} else {
		    if (is_tbllong(low_table_mod->char_width))
			    	k0 = low_ltable[ch];
		    else k0 = low_table[ch];
		};
	    } else;
	    if (k0 != 0) {	/* found in tables.		   */
		oconv(k0);
		if (rpclock > 0) {
		    rpclockparse(k0,rpclock);
		    rpclock = 0;
		    oconv(sFLSH);
		} else;
	    } else {
		in_sbroken(c2,c1);
	    };
	    c2 = 0;
	    if (in_single_shift) {
		shift_cond_recovery();
	    };
	    continue;
	} else {	/* ***** first byte ********************** */
	    if ((c1 = vGETC(f)) < 0) return(c1);
#ifdef SKFDEBUG
	    if (is_v_debug) {
		fprintf(stderr,"\ne_in:%02x",c1); 
	    };
#endif
	/* skf does not detect SO+ASCII as a SJIS kanji code.	   */
	/*  ... Because I've never seen such weird coding scheme.  */
	    if (c1 <= A_SP) { 	/* 0x00-0x20			   */
		/* lower side is always treated as 94 charset.	   */
	    	if (c1 == A_SP) {	/* space hook		   */
		    oconv(c1); 
		    if (rpclock > 0) {
			rpclockparse(c1,rpclock);
			rpclock = 0;
		    } else;
		    continue;
		} else if (c1 < 0x0e) {	/* 0x00-0x0d		   */
		    oconv(c1);		/* common control code	   */
		    if ((c1 >= 0x08) && (c1 <= A_CR) && (c1 != 0x0c)
		    		&& (rpclock > 0)) {
			rpclockparse(c1,rpclock);
			rpclock = 0;
		    } else;
		    if (kuni_opt && is_lineend(c1)) {
			res_all_shift; return(c1);
		    };
		    continue;
		} else if (c1 == A_ESC) { /* escape treatment	   */
		    if ((c1 = esc_process(f)) < 0) return(c1);
		    continue;
		} else if (c1 == A_SI) {  /* shift-in control      */
		    g0table2low();
		    res_locking_shift;
		} else if (c1 == A_SO) {  /* shift-out control     */
		    g1table2low();
		    set_ls1;
		} else if ((c1 == A_SS)   /* NATS single shift	   */
			&& (is_tbl_has_ss(low_kana))) {
		    res_single_shift; set_ss1;
		} else oconv(c1); 	/* just pass other code.   */
		continue;
	/* --- ASCII alphanumeric area --------------------------- */
	    } else if (in_single_shift && 
			((c1 <= A_DEL) || (c1 >= A_KSP))) {
				/* single shift has priority	   */	
		if ((k0 = get_in_shift_value(c1)) >= 0) {
		    c2 = k0;
		} else {
		    res_single_shift;	/* single shift end	   */
		};
		continue;
	    } else if (c1 <= A_DEL) {	/* 0x21-0x7f               */
		if (low_dbyte) { 
				/* lower plane is 2byte locked	   */
		    c2 = c1 ;	/* pass it to second byte	   */
		    continue;	/* goto next_byte		   */
#if	defined(ROT_SUPPORT) && !defined(NEW_ROT_CODE)
		} else if (is_rot_encoded) {
		    k0 = skf_rot13conv(c1);
		    oconv(k0);
		    if (rpclock > 0) {
			rpclockparse(k0,rpclock);
			rpclock = 0;
			oconv(sFLSH);
		    } else;
#endif
		} else if ((k0 = low_table[c1]) != 0) {
			    /* is not always ascii.		   */
		    oconv(k0);
		    if (rpclock > 0) {
			rpclockparse(k0,rpclock);
			rpclock = 0;
			oconv(sFLSH);
		    } else;
		    continue;
		} else {
		    in_undefined(c1,SKF_OUTTABLE);
		};
		/* --- 0xa1 <= c1 <= 0xff ------------------------ */
	    } else if (c1 >= A_KSP) {	/* c1 >= 0xa0		   */
			/* code is within JIS upper plane	   */
		if (is_multibyte(up_dbyte)) {
				/* lower plane is 2byte locked	   */
		    c2 = c1 ;	/* pass it to second byte	   */
		    continue;	/* goto next_byte		   */
		} else if ((up_table != NULL) && 
			((k0 = up_table[c1 & 0x7f]) != 0)) {
		    oconv(k0);
		    if (rpclock > 0) {
			rpclockparse(k0,rpclock);
			rpclock = 0;
			oconv(sFLSH);
		    } else;
		} else {	
		    in_undefined(c1,SKF_IUNDEF);
		};
		/* --- rest 0x80 <= c1 < 0xa0 -------------------- */
	    } else if (is_cp5022x(encode) && (c1 <= 0x98)) {
		c2 = c1; continue;
	    } else if ((c1 == A_SS2) && up_has_tbl &&
		(is_multibyte(up_dbyte) || (up_table[c1 & 0x7f] == A_SS2))) {
		res_single_shift; 
		set_ss2 ;	    /* SS2 in 8bit mode		   */
	    } else if ((c1 == A_SS3) && up_has_tbl &&
		(is_multibyte(up_dbyte) || (up_table[c1 & 0x7f] == A_SS3))) {
		res_single_shift; 
		set_ss3 ;	    /* SS3 in 8bit mode		   */
#ifdef FORCE_ENABLE_ARIB_MACRO_DEF
	    } else if ((c1 == AR_MAC) && is_arib) { /* arib macro  */
		if ((c1 = arib_macro_process(f)) < 0) continue;
			/* fail case: throw read characters	   */
#endif
	    } else if ((c1 == AR_RPC) && is_arib) { /* arib repeat */
		if ((c1 = arib_rpc_process(f)) < 0) continue;
			/* fail case: throw read characters	   */
		else rpclock = c1;
	    } else {	/* 0x80 <= c1 < 0xa0 & !is_msfam	   */
		if (is_nkf_euc) {
		    reput_c1 = c1;
		    if ((in_codeset == codeset_eucjp) 
		    	|| (in_codeset == codeset_jis)) {
			in_codeset = codeset_sjis;
		    } else in_codeset = codeset_cp932;
		    if ((in_codeset_preload() < 0) && disp_warn) {
			in_undefined(0,SKF_UNDEFCSET);
		    } else;
		    return(s_in(f));
		} else if ((low_table_limit <= 128) && (up_table == NULL)
			&& (up_ltable == NULL)) {
		    in_undefined(c1,SKF_IRGTUNDEF);
		} else if ((up_table != NULL) && 
			((k0=(skf_ucode)(up_table[c1 & 0x7f])) >= A_SP)) {
		    oconv(k0);	/* KOI8 and TCVN5712		   */
		} else if ((up_ltable != NULL) && 
			((k0=(skf_ucode)(up_ltable[c1 & 0x7f])) >= A_SP)){
		    oconv(k0);	/* for future use		   */
		} else {	
		    in_undefined(c1,SKF_IUNDEF);
		};
	    };
	};
    };
    return(sEOF); 
}
/*@+nullderef@*/

/* --------------------------------------------------------------- */
/*	s_in: SJIS, Big5 etc.(Non iso-2022 compliant sets)	   */
/* --------------------------------------------------------------- */
/*  in_code : SKF_SJIS or SKF_NONISO. NONISO is always given as    */
/*		command line option.				   */
/* --------------------------------------------------------------- */

/*@-globstate@*/ /* looks strange. should check it later.	   */
/*@-usereleased@*/ /* that should be false positive		   */
int s_in(f)			/* sjis input			   */
skfFILE *f;			/* can process sjis, big5 etc.	   */
{
    register int    c1, c2 = 0;
    skf_ucode   k0;		/* utf32 result			   */
    int	   c3,c2s;
#ifdef FUTURESUPPORT
    int	   d2,d3;		/* GB 18030 2nd and 3rd char.	   */
#endif
    int	   low_hz = 0;
    int	   low_hz_ss = 0;
    unsigned long  encode;
    unsigned short *kana_table = NULL;
#ifdef OFFSET_CALC_ENBL
    int	   u_offset = 0xa1;
#endif
    int	   low_bound = 0;
    int	   up_bound;
    int	   up_width = 2;
    struct iso_byte_defs *m213c_tbl = NULL;
    struct iso_byte_defs *emot_tbl = NULL;

    int		(*in_calc_index) P_((int,int)) = NULL;
    skf_ucode	*high_ltable = NULL;

    encode = i_codeset[in_codeset].encode;
    set_force_disp_warn;
    set_up_block;		/* guarding			   */

#ifdef FUTURESUPPORT
    d2 = 0; d3 = 0;
#endif

    if ((gx_table_mod != NULL) && (is_tblshort(gx_table_mod->char_width))) {
	kana_table = gx_table_mod -> unitbl;
    };

    reset_force_disp_warn;
    if (is_gbk(encode)) {
#ifdef OFFSET_CALC_ENBL
	u_offset = 0x81;
#endif
	low_bound = 0x81; up_bound = 0xfe;
	if (is_gb18030(encode)) {
	    if (gb2k_a5_tbl == NULL) {
		gb2k_a5_tbl = ovlay_byte_defs[gb18030_a5_index].unitbl;
	    } else;
	    if (gb2k_a5_tbl == NULL) {
		in_tablefault(SKF_PRESETFAIL,
			ovlay_byte_defs[gb18030_a5_index].desc);
		/* put dummies */
		if (abt_conv_err) {
		    return(sABRT);
		} else {
		    gb2k_a5_tbl =
		    	iso_4_dblbyte_defs[x0208_index].unitbl;
		};
	    };
	    in_calc_index = gb2k_in_calc_index;
	} else {
	    in_calc_index = gbk_in_calc_index;
	};
    } else if (is_msfam(encode) || is_johab(encode)) {
#ifdef OFFSET_CALC_ENBL
	u_offset = 0x81;
#endif
	low_bound = 0x81; up_bound = 0xfe;
	up_table_limit = g1_table_mod->table_len;
	up_width = g1_table_mod->char_width;
	if (is_tbllong(up_width) && (up_ltable == NULL)) {
	    up_ltable = input_get_dummy_ltable();
	} else if (is_tblshort(up_width) && (up_table == NULL)) {
	    up_table = input_get_dummy_table();
	} else;
	if (is_johab(encode)) {
	    in_calc_index = johab_in_calc_index;
	} else if (is_ms_213c(encode)) {
	    m213c_tbl = &(iso_4_dblbyte_defs[x0213_2_index]);
	    if (m213c_tbl->uniltbl == NULL) {
		in_tablefault(SKF_PRESETFAIL,m213c_tbl->desc);
		/* put dummies */
		if (abt_conv_err) {
		    return(sABRT);
		} else {
		    m213c_tbl = &(iso_4_dblbyte_defs[x0208_index]);
		};
	    };
	    in_calc_index = ms213_in_calc_index;
	} else if (is_ms_cel(encode)) {
	    emot_tbl = &(ovlay_byte_defs[nttemot_index]);
	    if (((in_codeset == codeset_sjiscl) && use_old_cell_map)
	    	|| (in_codeset == codeset_sjisontt)) { /* old map   */
	    	emot_tbl = &(ovlay_byte_defs[nttgrc1_index]);
	    } else if (in_codeset == codeset_sjisau) {
		emot_tbl = &(ovlay_byte_defs[auemot_index]);
	    } else if (in_codeset == codeset_sjissb) {
		emot_tbl = &(ovlay_byte_defs[sbemot_index]);
	    } else;
	    if (is_tbllong(emot_tbl->char_width) &&
	    		(emot_tbl->uniltbl == NULL)) {
		return(sABRT);
	    } else if (is_tblshort(emot_tbl->char_width) &&
	    		(emot_tbl->unitbl == NULL)) {
		return(sABRT);
	    } ;
	    in_calc_index = mscel_in_calc_index;
	    high_ltable = emot_tbl->uniltbl;
	} else {
	    in_calc_index = ms_in_calc_index;
	};
    } else if (is_big5(encode) || is_cp950(encode) || is_hku(encode)) {
#ifdef OFFSET_CALC_ENBL
	u_offset = 0xa1;
#endif
	low_bound = 0xa1; up_bound = 0xfe;
	up_table_limit = g1_table_mod->table_len;
	up_width = g1_table_mod->char_width;
	if (is_hku(encode)) {
	    in_calc_index = hku_in_calc_index;
	} else {
	    in_calc_index = big5_in_calc_index;
	};
    } else if (is_big5p(encode) || is_uhc(encode) || is_big5_uao(encode)) {
		/* BIG5+HKSCS, UAO, UHC */
#ifdef OFFSET_CALC_ENBL
	u_offset = 0x81;
#endif
	low_bound = 0x81; up_bound = 0xfe;
	up_table_limit = g1_table_mod->table_len;
	up_width = g1_table_mod->char_width;
	in_calc_index = big5p_in_calc_index;
    } else if (is_viqr_or_vimn(encode)) {
	low_bound = 0xa1; up_bound = 0xfe;
    	in_calc_index = viqr_in_calc_index;
    } else if (is_vni(encode)) {
	low_bound = 0xa1; up_bound = 0xfe;
    	in_calc_index = vni_in_calc_index;
    } else if (is_iscii(encode)) {
	low_bound = 0xa1; up_bound = 0xfe;
    	in_calc_index = iscii_in_calc_index;
    } else if (is_hzzW(encode)) {
	low_bound = 0xa1; up_bound = 0xfe;
	if (is_hz8(encode)) {
	    in_calc_index = hz8_in_calc_index;
	} else in_calc_index = iso_in_calc_index;
    } else if (is_nniso_gsm(encode)) {
	low_bound = 0xa1; up_bound = 0xfe;
    	in_calc_index = nniso_in_calc_index;
#ifdef OFFSET_CALC_ENBL
    } else if (is_thru(encode)) {
	low_bound = 0xa1; up_bound = 0xfe;
    	in_calc_index = thru_in_calc_index;
#endif
    } else {
	low_bound = 0xa1; up_bound = 0xfe;
    	in_calc_index = dmy_in_calc_index;
    };

    if (is_nkf_compat && (is_msfam(encode)) && is_nkf_rotmode) {
	return(be_in(f));
    } else;

    while (TRUE) {
	if (c2) {       /* ***** second byte ********************* */
	    c2s = c2;	/* save for later test			   */
	    if ((c1 = vGETC(f)) == sEOF) {
		if (is_nniso_gsm(encode)) {
		    if (c2 == 0x80) {
			oconv(0x00);
			res_single_shift;
			c2 = 0; continue;  /* goto next_word       */
		    } else in_undefined(c2,SKF_UNEXPEOF);
		} else if (!encode_enbl || !is_mime_nkfmode) {
			in_undefined(c2,SKF_UNEXPEOF);
		};
		break;
	    } else if (c1 == sOCD) { return(sOCD);
	    } else if (c1 < 0) return(c1); /* should be trapped    */
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x",c1);
#endif
	    /* there are 3 case in calc_in_index process.	   */
	    /*  1. just calculate index for the codeset		   */
	    /*    --- returns index value			   */
	    /*  2. index error (i.e. out of bound, etc.)	   */
	    /*    --- returns sABRT				   */
	    /*  3. further processing (for GB18030)		   */
	    /*    --- returns sOCD (not used)			   */

	    c3 = (*in_calc_index)(c1,c2);
	    if (c3 < 0) {
		if (c3 == sABRT) {	/* fail case		   */
		    res_single_shift;
		    c2 = 0; continue;
		} else if (c3 == sCONT) {
		    c2 = (c2 << 8) + c1; continue;
		} else if (c3 == sRETY) {
		    reput_c1 = c1;
		    c2 = 0; continue;
		} else if (c3 == sOCD) {
		    continue;
		} else {	/* other various terminal chars.   */
		    return(c3);
		};
	    } else;

	    /* --- 0x20 < c2 < 0x7f, 0x20 < c1 < 0x7f ------------ */
	    /* Note: EUC, MS-JIS and JIS come here with low_dbyte  */

	    k0 = 0;		/* clear result value 		   */

#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"(%x)",c3);
#endif
	    if (is_msfam(encode)) {
			/* normal MS-ShiftJIS case.		   */
			/*	ms-jis is always japanese kanji	   */
		if (is_ms_213c(encode) && (c2s >= 0xf0)) {
		    if (m213c_tbl == NULL) { /* fail */
			in_undefined(c3, SKF_IUNDEF);
		    } else if (c3 < m213c_tbl->table_len) {
			k0 = m213c_tbl->uniltbl[c3];
		    } else;
		} else if (c3 < X0208_KANJI_END) {
		    if (is_tbllong(up_width)) {
			k0 = up_ltable[c3];
		    } else
			k0 = up_table[c3];
		} else if (is_ms_cel(encode) && (c3 >= ISOMB_CODE_END)
				&& (c3 < 10744)) {
		    if (high_ltable != NULL) {
		    	k0 = high_ltable[c3 - ISOMB_CODE_END];
		    } else k0 = 0;
		} else if ((c3 < up_table_limit) 
			&& (c3 < X0208_CODE_END)) {
			if (is_tbllong(up_width)) {
			    k0 = up_ltable[c3];
			} else
			    k0 = up_table[c3];
		} else if (in_codeset != codeset_aribb24s) {
		    if (c3 < 10716) { /* MS NT-compat gaiji */
			k0 = UNI_UDF_A + c3 - X0208_CODE_END;
		    } else if (c3 < 10744) { /* 0xfcxx in SJIS	   */
			k0 = uni_k_ibm_fa[c3-10716];
		    } else if (c3 < 11104) {
			if (is_tbllong(up_width)) {/* to NEC area  */
			    k0 = up_ltable[c3-10744+8272];
			} else
			    k0 = up_table[c3-10744+8272];
		    } else;	/* out of range			   */
		} else if (use_cde_compat && (c3 < X0208_CODE_END)) {
		    k0 = UNI_UDF_A + c3 - X0208_KANJI_END;
		};
	    } else if (is_hzzW(encode) || (is_big5fam(encode))
		|| (is_gbk(encode)) || (is_uhc(encode))) {
	      if (c3 < up_table_limit) {
		if (is_tbllong(up_width)) {
		    k0 = up_ltable[c3];
		} else
		    k0 = up_table[c3];
	      } else;
	    } else if (is_johab(encode)) {
	      if (c3 < up_table_limit) {
		if (c2 <= JOHAB_HANGLE) {
		    if (is_tbllong(up_width)) {
			k0 = up_ltable[c3];
		    } else
			k0 = up_table[c3];
		} else { /*@-nullderef@*/
		    k0 = kana_table[c3];
		};
	      } else;
	    } else if (is_iscii(encode)) {
		if ((c2 == A_IATR) && (c1 >= 0x30) && (c1 < 0x40)) {
		    k0 = (0xd840 + c1);
		} else k0 = 0;
	    } else;
	    if (k0 != 0) {	/* not found in tables.		   */
		oconv(k0);
	    } else {
		in_sbroken(c2,c1);
	    };
	    c2 = 0;
	} else {	/* ***** first byte ********************** */
			/* Note: do not return EOF to reput_c1	   */
	    if (reput_c1 >= 0) { 
		c1 = reput_c1; reput_c1 = sOCD;
	    } else if ((c1 = vGETC(f)) < 0) return(c1);
#ifdef SKFDEBUG
	    if (is_vv_debug) {
		fprintf(stderr,"\ns_in:%02x",c1); 
	    };
#endif
	/* skf does not detect SO+ASCII as a SJIS kanji code.	   */
	/*  ... Because I've never seen such weird coding scheme.  */
	    if (c1 <= A_SP) { 	/* 0x00-0x20			   */
		/* lower side is always treated as 94 charset.	   */
	    	if (c1 == A_SP) {	/* space hook		   */
		    if (is_hzzW(encode)) {
			if (!(low_hz_ss)) {
			    oconv(c1); low_hz_ss = FALSE;
			} else {
			    low_hz_ss = TRUE;
			};
		    } else {
			oconv(c1);
		    };
		    continue;
		} else if (c1 < 0x0e) {	/* 0x00-0x0d		   */
		    if ((c1 == 0x00) && is_nniso_gsm(encode)) {
			c2 = 0x80; continue;
		    } else if (!is_zW(encode) || !is_lineend(c1)) {
			oconv(low_table[c1]); /* common control	   */
		    } else if (is_lineend(c1)) {
			if (low_hz_ss) oconv(c1);
			low_hz = FALSE; low_hz_ss = FALSE;
			if (kuni_opt) {
			    res_all_shift; return(c1);
			};
		    };
		} else if (c1 == A_ESC) { /* escape treatment	   */
		    if (is_nniso_gsm(encode)) {
			c2 = c1; continue;
		    } else if ((c1 = esc_process(f)) < 0) return(c1);
		    if (low_table == NULL) low_table = ascii_uni_byte;
		    if (c1 > A_DEL) {
		    	c2 = c1;
		    	continue;
		    } else;
		} else if (c1 == A_SI) {  /* shift-in control      */
		    g0table2low();
		    res_locking_shift;
		} else if (c1 == A_SO) {  /* shift-out control     */
		    g1table2low();
		    set_ls1;
		} else {		/* just pass other code.   */
		    if (low_table == NULL) {
		    	in_tablefault(SKF_TBLINCNSIS,"SIN");
			return(sEOF);
		    } else
			oconv(low_table[c1]);
		};
		/* --- ASCII alphanumeric area ------------------- */
	    } else if (c1 <= A_DEL) {	/* 0x21-0x7f               */
		if (low_dbyte) {
		    c2 = c1; continue;
		} else if ((c1 == '~') && is_hz(encode)) {
		    if ((c1 = vGETC(f)) < 0) {
			oconv('~'); return(c1);
		    };
		    if (c1 == '~') {
			oconv(c1);
		    } else if (c1 == '{') { low_hz = TRUE;
		    } else if (c1 == '}') { low_hz = FALSE;
		    } else if ((c1 == 0x0d) || (c1 == 0x0a)) {
			oconv(c1);
		    } else {
			low_hz = FALSE; oconv('~'); oconv(c1);
		    };
		} else if ((c1 == '#') && is_zW(encode)) {
		    if ((c1 = vGETC(f)) < 0) {
			oconv('#'); return(c1);
		    };
		    if (is_lineend(c1)) {
			oconv(c1); low_hz = FALSE; low_hz_ss = FALSE;
		    } else {
			c2 = c1;
		    };
		    continue;
		} else if (low_hz) {	/* HZ shifted		   */
		    if (low_hz_ss) {
			low_hz_ss = FALSE; oconv(c1);
		    } else {
			c2 = c1; continue;
		    };
		} else if ((c1 == 'z') && is_zW(encode)) {
		    if ((c1 = vGETC(f)) < 0) return(c1);
		    if (c1 == 'W') {
			low_hz = TRUE;
		    } else {
			oconv('z');
			oconv(c1);
		    };
		} else if (is_viqr_or_vimn(encode) 
			&& ((c1 == 'a') || (c1 == 'e') || (c1 == 'i')
			  || (c1 == 'o') || (c1 == 'u') || (c1 == 'y')
			  || (c1 == 'A') || (c1 == 'E') || (c1 == 'I')
			  || (c1 == 'O') || (c1 == 'U') || (c1 == 'Y')
			  || (c1 == 'D') || (c1 == 'd'))) {
		    c2 = c1; continue;
		} else if (is_vni(encode)
			&& ((c1 == 'a') || (c1 == 'e') 
			  || (c1 == 'o') || (c1 == 'u') || (c1 == 'y')
			  || (c1 == 'A') || (c1 == 'E')
			  || (c1 == 'O') || (c1 == 'U') || (c1 == 'Y'))) {
			c2 = c1; continue;
#if	defined(ROT_SUPPORT) && !defined(NEW_ROT_CODE)
		} else if (is_rot_encoded) {
		    k0 = skf_rot13conv(c1);
		    oconv(k0);
#endif
		} else if ((k0 = low_table[c1]) != 0) {
			    /* not always be ascii.		   */
		    oconv(k0);
		} else {
		    in_undefined(c1,SKF_OUTTABLE);
		};
		low_hz_ss = FALSE;
		/* --- 0x80 <= c1 <= 0xff ------------------------ */
	    } else if (is_msfam(encode)) {	/* c1 >= 0x80	   */
		if (((c1 >= low_bound) && (c1 < A_KSP))  /* kanji  */
			|| ((c1 > KANA_END) && (c1 <= up_bound))) { 
		    c2 = c1; 
		} else if (c1 == A_KSP) {
		    oconv(0x20); 
		} else {		/* 0xa1 <= c1 <= 0xdf	   */
		    if (is_ms_kanas(encode)) {
		        oconv(c1 + 0xff60 - 0xa0);
		    } else if ((up_table != NULL) && 
		    	((k0 = up_table[c1 & 0x7f]) != 0)) {
			oconv(k0);
		    } else in_undefined(c1,SKF_IUNDEF);
		};
	    } else if (is_big5fam(encode) || is_uhc(encode)) { 
			/* c1 >= 0x80, big5, GBK or GB18030	    */
		if ((c1 >= low_bound) && (c1 <= up_bound)){ 
		    c2 = c1;
		} else if (is_gbk(encode) && !is_gb18030(encode)) {
			/* pure GBK, not GB18030		    */
		    if (c1 == 0x80) oconv(0x20ac);
		    else oconv(0xf8f5);
		} else if (is_uhc(encode)) {
		    if (c1 == 0x80) oconv(0x0080);
		    else oconv(0xf8f7);
		} else {
		    in_undefined(c1,SKF_IUNDEF);
		};
	    } else if (is_johab(encode)) {  /* c1 >= 0x80, johab   */
		if ((c1 >= 0x84) && (c1 <= JOHAB_HANGLE)) { 
		    c2 = c1; continue;
		} else if (((c1 >= JOHAB_HANJA0) && (c1 <= JOHAB_HANJA0E))
			|| ((c1 >= JOHAB_HANJA1) && (c1 <= JOHAB_HANJA1E))){
			/* johab hanja area */
		    c2 = c1; 
		} else {
		    in_undefined(c1,SKF_IUNDEF);
		};
	    } else if (c1 >= A_KSP) {	/* c1 >= 0xa0		   */
			/* code is within JIS upper plane	   */
		if (is_multibyte(up_dbyte)) {
				/* lower plane is 2byte locked	   */
		    c2 = c1 ;	/* pass it to second byte	   */
		    continue;
		} else if (is_iscii(encode)) {
		    if (c1 == A_IATR) {
			c2 = c1;
		    } else if (c1 == A_IEXT) {
			in_undefined(c1,SKF_UNSUPP);
		    } else {
			if ((up_table != NULL) && 
				((k0 = up_table[c1 & 0x7f]) > 0)) {
			    oconv(k0);
			} else in_undefined(c1,SKF_IUNDEF);
		    };
		} else if ((low_table_limit == 256) &&
				((k0 = low_table[c1 & 0xff]) != 0)) {
		    oconv(k0);
		} else if (up_table == NULL) {
		    in_undefined(c1,SKF_IUNDEF);
		} else if ((up_table != NULL) && 
				((k0 = up_table[c1 & 0x7f]) != 0)) {
		    oconv(k0);
		} else if (is_vni(encode)) {
		    c2 = c1; 
		} else in_undefined(c1,SKF_IUNDEF);
		continue;
	    } else {	/* 0x80 <= c1 < 0xa0 & !is_msfam	   */
		/* --- rest 0x80 <= c1 < 0xa0 -------------------- */
		/* comparison is for KOI-8-R consideration	   */
		if (low_table_limit > 128) {
		    oconv(low_table[c1 & 0xffU]);
		} else if (
		    (k0 = (skf_ucode)(up_table[c1 & 0x7fU])) != 0) {
		    oconv(k0);
		} else {	
		    in_undefined(c1,SKF_IRGTUNDEF);
		};
	    };
	};
    };
    return(sEOF); 
}


/* --------------------------------------------------------------- */
/*	ks_in: KEIS83, JEF, EBCDIC input converter		   */
/* --------------------------------------------------------------- */
int ks_in(f)
register skfFILE *f;
{
    int c1,c2 = 0,c3 = 0;
    skf_ucode	k0;
    unsigned short *ebcdic_tbl;
    unsigned long encode;

    /* table is not used directly in KEIS, but information is	   */
    /* necessary because KEIS is EUC based.			   */

    encode = i_codeset[in_codeset].encode;
    ebcdic_tbl = gx_table_mod->unitbl;

    if (start_kanji) { in_keis = TRUE; };
    up_table_limit = g1_table_mod->table_len;

    while ((c1 = GETC(f)) >= 0) {
        if (c2) {	/* second bytes -------------------------- */	
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x",c1);
#endif
	  if ((c1 == 0x40) && (c2 == 0x40)) { /* space trap	   */
	  	/* JEF and IBM DBCS regard 0x4040 as space.	   */
		/* go with KEIS too. Won't be harm anyway.	   */
		oconv(U_BLANK); c2 = 0; continue;
	  } else if (is_keis_dbcs(encode)) {	/* --- IBM	   */ 
	      c3 = ((c2 - 0x41) * 190) + (c1 - 0x41);
	  } else if (is_keis_jef(encode) && (c2 <= JEF_E1E) 
		&& (c2 >= JEF_E1)){ /* JEF extend characters	   */
	      jef_conv(c2,c1);
	      c2 = 0; continue;
          } else if (is_keis_keis(encode) && (c2 == KEIS_SMM)) {
				/* KEIS Kanji-in & Out		   */
            	if (c1 == KEIS_SO) {
            	    in_keis = TRUE;
#ifdef SKFDEBUG
		    if (is_vv_debug) fprintf(stderr,"(KEISSO)");
#endif
		} else if (c1 == KEIS_SI) {
            	    in_keis = FALSE;
#ifdef SKFDEBUG
		    if (is_vv_debug) fprintf(stderr,"(KEISSI)");
#endif
		} else {	/* discard all else		   */
		    in_sbroken(c2,c1);
		};
		c2 = 0; continue;
	  } else {		/* keis or pure JIS -------------- */
            if (c2 < KEIS_3) {	/* Oops.  ------------------------ */
		if ((is_keis_jef(encode) || is_keis_dbcs(encode)
			|| is_keis_dbcst(encode))
			&& (c1 == KEIS_SP) && (c2 == KEIS_SP)){
		    oconv(0x3001);	/* fullwidth space	   */
		} else {
		    in_sbroken(c2,c1);
		};
		c2 = 0; continue;
	    } else if ((c1 < KEIS_S) || (c1 == 0xff)) {
	    			/* should not appear in KEIS	   */
		in_sbroken(c2,c1);
		c2 = 0; continue;
	    } else if (c2 < KEIS_S) {  /* various extended field   */
		if (is_keis_keis(encode)) { /* pure KEIS	   */
		    keis_conv(c2,c1); c2 = 0; /* pass to converter */
		    continue;
		} else {
		    in_sbroken(c2,c1);
		    c2 = c1; continue;
		};
	    } else if (c2 == KEIS_2) { /* In KEIS expansion field 2*/
		if (is_keis_keis(encode)) { /* pure KEIS	   */
		    keis_conv(c2,c1); c2 = 0; continue;
		} else {
		    in_sbroken(c2,c1);
		    c2 = c1; continue;
		};
	    } else {  		 /* JIS X0208 Kanji comes here	   */
		c3 = index_cal((c1 & 0x7f),(c2 & 0x7f));
	    };
	  };
	  k0 = 0;
	  if (c3 < up_table_limit) {
		k0 = up_table[c3];
	  };
	  if (k0 != 0) {
		oconv(k0);		/* converted into JIS	   */
	  } else 
	  	in_sbroken(c2,c1);
	  c2 = 0;
        } else {	/* first byte ---------------------------- */
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"\nks_in:%02x",c1);
#endif
            if (is_keis_keis(encode) && (c1 == KEIS_SMM)) {
		c2 = c1; continue;
	    } else if ((is_keis_jef(encode) || is_keis_dbcs(encode)
			|| is_keis_dbcst(encode))
		&& (c1 == KEIS_JEF_SO)) {
		in_keis = TRUE;
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr," :JO");
#endif
	    } else if ((is_keis_jef(encode) || is_keis_dbcs(encode)
			|| is_keis_dbcst(encode))
		&& (c1 == KEIS_JEF_SI)) {
		in_keis = FALSE;
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr," :JI");
#endif
	    } else if (is_keis_dbcs(encode) && (c1 == KEIS_IBM_SI)) {
		in_keis = FALSE;
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr," :II");
#endif
	    } else if (is_keis_dbcs(encode) && (c1 == KEIS_IBM_SO)) {
		in_keis = TRUE;
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr," :IO");
#endif
	    } else if ((is_keis_jef(encode) || is_keis_dbcs(encode))
		&& in_keis && (c1 == KEIS_SP)) {
		oconv(0x20); continue;
	    } else if (is_keis_dbcs(encode)
		&& in_keis && (c1 >= IBMDBCS_S) && (c1 <= IBMDBCS_E)) {
	    	c2 = c1; continue;    /* take a look to 2nd byte   */
	    } else if (is_keis_jef(encode) && in_keis &&
			(((c1 >= A_KSP) && (c1 <= 0xFE))
			|| ((c1 >= JEF_E1) && (c1 <= JEF_E1E)))) {
				/* JIS X208 Kanji comes here	   */
	    	c2 = c1; continue;    /* take a look to 2nd byte   */
	    } else if ((in_keis == 0) || (c1 < KEIS_3)) {
		if (c1 < 0xfa) {	/* 0x00 - 0xf9		   */
		    k0 = ebcdic_tbl[c1];
		    if (k0 != 0x00) { 
			oconv(k0); 
		    } else {
			in_undefined(c1,SKF_IBROKEN);
		    }
		} else {		/* ??			   */
		    in_undefined(c1,SKF_IBROKEN);
		};
	    } else if (is_keis_keis(encode) && 
			(c1 >= KEIS_3) && (c1 <= 0xFE)) {
				/* JIS X208 Kanji comes here	   */
	    	c2 = c1; continue;    /* take a look to 2nd byte   */
	    } else {		/* character drop? or error	   */
		in_keis = FALSE; /* inconsistent: reset keisShift */
		if (c1 < 0xfa) {	/* 0x00 - 0xf9		   */
		    k0 = ebcdic_tbl[c1];
		    if (k0 != 0x00) { 
			oconv(k0); 
		    } else {
			in_undefined(c1,SKF_IBROKEN);
		    }
		} else {		/* ??			   */
		    in_undefined(c1,SKF_IBROKEN);
		};
	    };
	};
    };
    return(c1); 
}

/* --------------------------------------------------------------- */
/* u_in: Ucs2 input support					   */
/*    This routine processes UCS2 and X-0221(ISO 10646) code.	   */
/*    Note that these routines only process 2-octet code, not 4.   */
/*    Since JIS X-0221 belongs to a family of ISO-2022 code, this  */
/*    includes basic ISO-2022 support.				   */
/* --------------------------------------------------------------- */
int u_parse(f,ch,cod)
skfFILE *f;
skf_ucode ch;
int	cod;
{
    skf_ucode c4,c5;

    if ((ch == 0xfeff) || (ch == 0xfffe)) { /* BOM		  */
	return(0);		/* just discard			  */
    } if ((ch >= 0xd800) && (ch <=0xdbff)){ /* surrogate	  */
		    /* utf7 with surrogate case		  */
	if ((c4 = u_dec_hook(f,cod)) == sEOF) {
	    in_undefined(c4, SKF_NOSURRG); 
	    return(sEOF);
	} else if (c4 == sOCD) return(sOCD);
	if ((c4 >= 0xdc00) && (c4 <= 0xdfff)) {
	    in_undefined(c4, SKF_NOSURRG); return(0);
	};
	c5 = ((ch - 0xd800) << 10) + ((c4 - 0xdc00) & 0x3ff)
	    + 0x10000;
	oconv(c5);
    } else if ((ch < 0xf800) || (ch >= 0xe000)) {
        if (uniuni_o_prv != NULL) {
	    c5 = uniuni_o_prv[ch - 0xe000];
	    if (c5 != 0) ch = c5;
	} else;
	oconv(ch);
    } else if ((ch < 0x110000) || unchk_utf32_range) {
	oconv(ch);
    } else {
	in_undefined(ch,SKF_OUTTABLE);
    };
    return(0);
}

/* --------------------------------------------------------------- */
/* u_in:	UTF-16 Code process				   */
/* --------------------------------------------------------------- */
int u_in(f)			/* ucs2 input			   */
skfFILE *f;
{
    if ((in_codeset == codeset_utf16be)
    	|| (in_codeset == codeset_utf32be)) {
	set_in_endian;
    } else;

    if (is_encode_incomp_ucs2) {
    	/* utf-16 doesn't support any encodings			   */
	if (!is_nkf_compat) in_undefined(0,SKF_DECINCONS);
    	decode_set(0);
    } else;
    if ((in_codeset == codeset_utf32be)
    	|| (in_codeset == codeset_utf32le)
    	|| (in_codeset == codeset_utf32)) {
	return(uni_in(f,DEC_HOOK_UTF32));
    } else {
	return(uni_in(f,DEC_HOOK_UTF16));
    };
}

/* --------------------------------------------------------------- */
/* z_in:	UTF-8 Code process				   */
/* --------------------------------------------------------------- */
/* this routine support only after we know its UTF-8		   */

int z_in(f)			/* utf-8 input			   */
register skfFILE *f;		/* this never mixed with jis 	   */
{
#ifdef ACE_SUPPORT
    if (!in_ace) {
	return(uni_in(f,DEC_HOOK_UTF8));
    } else {
	return(uni_in(f,DEC_HOOK_UTF16));
    };
#else
	return(uni_in(f,DEC_HOOK_UTF8));
#endif
}

/* --------------------------------------------------------------- */
/* y_in:	UTF-7 Code process				   */
/* --------------------------------------------------------------- */
/* this routine support only after we know its UTF-7		   */

int y_in_dec(x)
int	x;
{
    if ((x >= 'A') && (x <= 'Z')) return(x-'A');
    else if ((x >= 'a') && (x <= 'z')) return(x-'a'+26);
    else if ((x >= '0') && (x <= '9')) return(x-'0'+52);
    else if (x == '+') return(62);
    else if (x == '/') return(63);
    else return(-1);
}

/* --------------------------------------------------------------- */
int y_in(f)			/* utf-7 input			   */
register skfFILE *f;		/* this never mixed with jis 	   */
{
    if (is_encode_incomp_ucs2) {
    	/* utf7 doesn't support any encodings			   */
	in_undefined(0,SKF_DECINCONS);
    	decode_set(0);
    } else;
    return(uni_in(f,DEC_HOOK_UTF7));
}

/* --------------------------------------------------------------- */
/* uni_in:	Unic*de common process				   */
/* --------------------------------------------------------------- */
int uni_in(f,cod)	
skfFILE *f;
int cod;
{
    skf_ucode	    c1;
    skf_ucode	    ch;

    while ((ch = u_dec_hook(f,cod)) >= 0) {
#ifdef SKFDEBUG
	if (is_vv_debug) {
	    fprintf(stderr,"\n%s:%04x",
	    	(cod == DEC_HOOK_UTF8) ? "z_in" :
	    		((cod == DEC_HOOK_UTF7) ? "y_in" : "u_in"),ch);
	};
#endif
	if ((c1 = u_parse(f,ch,cod)) < 0) return(c1);
    };
    return(ch); 
}

/* --------------------------------------------------------------- */
/* t_in:	No-convert Code process				   */
/* --------------------------------------------------------------- */
int t_in(f)
register skfFILE *f;
{
    int c1;

    while (TRUE) {
	if ((c1 = vGETC(f)) == sEOF) {
	    return(sEOF);
	} else if (c1 == sOCD) return(sOCD);
	SKFputc((skf_ucode)c1);

    };
    /*NOTREACHED*/
}

/* --------------------------------------------------------------- */
/* b_in:	B-Right input support				   */
/* --------------------------------------------------------------- */	
/*@-globstate@*/ /* looks strange. should check it later.	   */
/*@-nullpass@*/
int b_in(f)			/* B-Right input		   */
register skfFILE *f;
{
    int c1,c2,c3,c4;
    skf_ucode k0;
    int	cur_a;
    long ch,i,j,ck;
    unsigned short	*g1tbl;		/* KS X 1001		   */
    unsigned short	*g2tbl;		/* GB2312		   */
			/* JIS x0212 is set to g3, but not used	   */

    cur_a = BV_JIS;
    g1tbl = NULL; g2tbl = NULL;

    if (g1_table_mod != 0) {	/* KS X1001, GB2312 preset	   */
	g1tbl = g1_table_mod->unitbl;
    } else in_tablefault(SKF_PRESETFAIL,NULL);
    if (g2_table_mod != 0) {
	g2tbl = g2_table_mod->unitbl;
    } else in_tablefault(SKF_PRESETFAIL,NULL);

    while ((c2 = GETC(f)) >= 0) {
	if ((c1 = GETC(f)) == sEOF) {
	    in_undefined(c2,SKF_UNEXPEOF); 
	    break;
	} else if (c1 == sOCD) return(sOCD);
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"\nb_in:%02x,%02x",c2,c1);
#endif
	k0 = 0;

	if ((c2 == 0x00) && 
		((c1 < 0x21) || (c1 == 0x7f))) {  /* control area  */
	    k0 = c1;
	} else if ((c2 == 0x00) && (c1 == 0xa0)) { /* kana space   */
	    k0 = c1;
	} else if ((c1 == BV_ESC) && (c2 == BV_ESC)) { /* EOF	   */
	    break;
	} else if (c2 == 0xfe) {   /* language code in B-right	   */
	    cur_a = c1;
	} else if ((c2 == BV_ESC) && (c1 < 0x7f)) {
				/* special code in B-right	   */
	    continue;	/* currently not supported. Just discard   */
	} else if ((c2 == BV_ESC) && (c1 >= 0x80)) {
				/* escape code in B-right	   */
			/* note: c3, c4 is discarded in EOF case   */
	    if ((c3 = GETC(f)) < 0) {
		in_undefined(c2,SKF_UNEXPEOF); return(c3);
	    };
	    if ((c4 = GETC(f)) < 0) {
		in_undefined(c2,SKF_UNEXPEOF); return(c4);
	    };
	    ch = ((c3 << 8) + c4) & 0x0ffff; k0 = UNC;
	    if (ch != 0xffff) {		/* normal segment	   */
		for (i=0;i<ch;i++) {  /* discard everything	   */
		    if ((c3 = GETC(f)) < 0) {
			in_undefined(c2,SKF_UNEXPEOF); return(c3);
		    };
		};
	    } else {		/* large segment		   */
		if ((c3 = GETC(f)) < 0) {
		    in_undefined(c2,SKF_UNEXPEOF); return(c3);
		};
		if ((c4 = GETC(f)) < 0) {
		    in_undefined(c2,SKF_UNEXPEOF); return(c4);
		};
		ch = ((c3 << 8) + c4) & 0x0ffff;
		if ((c3 = GETC(f)) < 0) {
		    in_undefined(c2,SKF_UNEXPEOF); return(c3);
		};
		if ((c4 = GETC(f)) < 0) {
		    in_undefined(c2,SKF_UNEXPEOF); return(c4);
		};
		ck = ((c3 << 8) + c4) & 0x0ffff;
		for (j=0;j<ck;j++) {  /* discard everything	   */
		    for (i=0;i<256;i++) {
			if ((c3 = GETC(f)) < 0) {
			    in_undefined(c2,SKF_UNEXPEOF); return(c3);
			};
		    };
		};
		if ((c3 = GETC(f)) < 0) return(c3);
		for (i=0;i<ch;i++) {  /* discard everything	   */
		    if ((c3 = GETC(f)) < 0) {
			in_undefined(c2,SKF_UNEXPEOF); return(c3);
		    };
		};
	    };
	    continue;
	} else if ((c1 < 0x21) || (c2 < 0x21)) {
	    in_undefined(c2,SKF_IUNDEF);  /* reserved area	   */
	    continue;
	} else if ((c1 < 0x7f) && (c2 < 0x7f) && (cur_a == BV_JIS)) {
	    ch = (((c2 & 0x7f) - 0x21) * 94) + (c1 & 0x7f) - 0x21;
	    k0 = uni_t_sjisx208[ch];
	} else if ((c2 >= 0x80) && (c1 < 0x7f) && (cur_a == BV_JIS)) {
	    ch = ((c2 & 0x7f) * 94) + (c1 & 0x7f) - 0x21;
	    k0 = uni_t_x212[ch];
	} else if ((c2 < 0x7f) && (c2 >= 0x80) && (cur_a == BV_JIS)) {
	    ch = ((c2 & 0x7f) * 94) + (c1 & 0x7f) - 0x21;
	    if (g2tbl != NULL) {	/* GB-2312	   */
		oconv(g2tbl[ch]);
	    } else in_undefined(c2,SKF_TBLUNDEF);
	    continue;
	} else if ((c2 >= 0x80) && (c2 >= 0x80) && (cur_a == BV_JIS)) {
	    ch = ((c2 & 0x7f) * 94) + (c1 & 0x7f) - 0x21;
	    if (g1tbl != NULL) {	/* KS X 1001		   */
		oconv(g1tbl[ch]);
	    } else in_undefined(c2,SKF_TBLUNDEF);
	    continue;
	} else if (cur_a == BV_UC0) {
	    k0 = (((c2 >= 0x80) ? (c2 - 0x5e) : (c2 - 0x21)) * 220)
		+ ((c1 >= 0x80) ? (c1 - 0x5e) : (c1 - 0x21));
	    if (k0 >= 0xac00) k0 = UNC;
	} else if (cur_a == BV_UC1) {
	    k0 = (((c2 >= 0x80) ? (c2 - 0x5e) : (c2 - 0x21)) * 220)
		+ ((c1 >= 0x80) ? (c1 - 0x5e) : (c1 - 0x21)) + 0xac00;
	    if (k0 >= 0x10000) k0 = UNC;
	} else {		/* all else: not supported	   */
	    in_tablefault(SKF_TBLUNDEF,"-");
	    continue;
	};
	oconv(k0);
    };
    return(c2); 
}

/* --------------------------------------------------------------- */
/* various Vietnamese decoder					   */
/* --------------------------------------------------------------- */
static int is_viqr_tone(x)
int x;
{
    if (is_viqr(i_codeset[in_codeset].encode)) {
	if (x == 0x60) { return(24); 
	} else if (x == '?') { return(48);
	} else if (x == '~') { return(72);
	} else if (x == 0x27) { return(96);
	} else if (x == '.') { return(120);
	} else if (x == viscii_escape) { return(24);
	} else;
    } else {		/* VISCII-MNEM	*/
	if (x == 0x21) { return(24); 
	} else if (x == '?') { return(48);
	} else if (x == 0x22) { return(72);
	} else if (x == 0x27) { return(96);
	} else if (x == '.') { return(120);
	} else if (x == viscii_escape) { return(24);
	} else;
    };
    return(0);
}

skf_ucode	viqr_parse(ch,ca,cv,ec)
int	ch,ca,cv;
unsigned long ec;	/* encode				  */
{
    unsigned short *viq_tbl;
    unsigned short *viqe_tbl;	/* VNI 2nd byte defs		  */
    int		vow_nrm;
    int		cx;
    int		cy = 0;
    int		vni_a_up = 0;

#ifdef SKFDEBUG
    if (is_vvv_debug) fprintf(stderr," viqr_parse(%x %x %x)",ch,ca,cv);
#endif
    cx = SKFtoupper(ch);
    if (is_vni(ec)) {
	viq_tbl = misc_byte_defs[vni_index].unitbl;
	viqe_tbl = ovlay_byte_defs[vni_ex_prv_index].unitbl;
    } else {
	viq_tbl = misc_byte_defs[visciiq_index].unitbl;
	viqe_tbl = NULL;
    };
    if (viq_tbl == NULL) {
    	in_sbroken(ch,ca);
	return(-1);
    };

    if (is_vni(ec)) {
	if ((ca < 0xc0) || ((cy = viq_tbl[ca - 0x40]) == 0)) {
	    oconv(ch); return(ca);
	};
	/* ch other than follows never comes here */
	switch (ch) {
	    case 'A': vow_nrm = 0; break;
	    case 'E': vow_nrm = 0x11; break;
	    case 'O': vow_nrm = 0x22; break;
	    case 'U': vow_nrm = 0x33; break;
	    case 'Y': vow_nrm = 0x44; break;
	    case 'a': vow_nrm = 0x55; vni_a_up = 1; break;
	    case 'e': vow_nrm = 0x66; vni_a_up = 1; break;
	    case 'o': vow_nrm = 0x77; vni_a_up = 1; break;
	    case 'u': vow_nrm = 0x88; vni_a_up = 1; break;
	    case 'y': vow_nrm = 0x99; vni_a_up = 1; break;
	    case 0xd4: vow_nrm = 0xaa; break;
	    case 0xd6: vow_nrm = 0xbb; break;
	    case 0xf4: vow_nrm = 0xcc; vni_a_up = 1; break;
	    case 0xf6: vow_nrm = 0xdd; vni_a_up = 1; break;
	    default: vow_nrm = 0;
	};
	if (((ca >= 0xe0) && (vni_a_up == 0)) || 
		((ca < 0xe0) && (vni_a_up == 1))) {
	    oconv(ch); return(ca);
	};
	oconv(viqe_tbl[vow_nrm + cy - 1]); return(0);
    } else {
	vow_nrm = (cx == 'A') ? 0 :
		    ((cx == 'E') ? 6 :
		     ((cx == 'I') ? 10 :
		      ((cx == 'O') ? 12 :
		       ((cx == 'U') ? 18 : 22))));
	if ((ch >= 'a') && (ch <= 'y')) vow_nrm++;

	if (ch == 'D') {
	    if (cv == 'D') {
		oconv(viq_tbl[144]); return(-1);
	    } else {
		oconv(ch); return(cv);
	    };
	} else if (ch == 'd') {
	    if (cv == 'd') {
		oconv(viq_tbl[145]); return(-1);
	    } else {
		oconv(ch); return(cv);
	    };
	} else if (ca > 0) {
	    if (((((cx == 'A') && (ca == '^')) 
		    || ((cx == 'O') && (ca == '+'))) && is_viqr(ec))
		|| ((((cx == 'A') && (ca == '>')) 
		    || ((cx == 'O') && (ca == '*'))) && is_vimn(ec))) {
		   vow_nrm += 4;
	    } else vow_nrm += 2;
	};
	if ((cy = is_viqr_tone(cv)) > 0) {
	    vow_nrm += cy;
	    cv = -1;
	} else ;
#ifdef SKFDEBUG
	if (is_vvv_debug) fprintf(stderr,"nrm: %x ",vow_nrm);
#endif

	oconv(viq_tbl[vow_nrm]);
	return(cv);
    };
}

/* --------------------------------------------------------------- */
static skf_ucode get_in_shift_value(c1)
int c1;
{
    int k0,c3;

    c3 = c1 & 0x7f;
    if (in_ss2) {
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ss2");
#endif
	if (is_charset_macro(g2_table_mod) == 1) {
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"(macro)");
#endif
	    c3 = paraphrase_arib_macro(c1);
	    return(c3);
	} else;

	if (is_multibyte(g2_table_mod->char_width)) {
	    g2table2up();
	    return(c3 | 0x80); 
	} else {
	    if (is_tbllong(g2_table_mod->char_width) &&
	    	(g2_table_mod->uniltbl != NULL) &&
		((k0 = (g2_table_mod->uniltbl)[c3]) != 0)) {
		oconv(k0);
	    } else if (is_tblshort(g2_table_mod->char_width) &&
	    	(g2_table_mod->unitbl != NULL) &&
		((k0 = (g2_table_mod->unitbl)[c3]) != 0)) {
		oconv(k0);
	    } else in_undefined(c1,SKF_OUTTABLE);
	};
    } else if (in_ss3) {
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ss3");
#endif
	if (is_charset_macro(g3_table_mod) == 1) {
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"(macro)");
#endif
	    c3 = paraphrase_arib_macro(c1);
	    return(c3);
	} else;
	if (is_multibyte(g3_table_mod->char_width)) {
	    g3table2up();
	    return(c3 | 0x80); 
	} else {
	    if (is_tbllong(g3_table_mod->char_width) &&
	    	(g3_table_mod->uniltbl != NULL) &&
		((k0 = (g3_table_mod->uniltbl)[c3]) != 0)) {
		oconv(k0);
	    } else if (is_tblshort(g3_table_mod->char_width) &&
	    	(g3_table_mod->unitbl != NULL) &&
		((k0 = (g3_table_mod->unitbl)[c3]) != 0)) {
		oconv(k0);
	    } else in_undefined(c1,SKF_OUTTABLE);
	};
    } else if (in_ss1) { 
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ss1");
#endif
	if (is_charset_macro(g1_table_mod) == 1) {
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"(macro)");
#endif
	    c3 = paraphrase_arib_macro(c1);
	    return(c3);
	} else;
	if (is_multibyte(g1_table_mod->char_width)) {
	    g3table2up();
	    return(c3 | 0x80); 
	} else {
	    if (is_tbllong(g1_table_mod->char_width) &&
	    	(g1_table_mod->uniltbl != NULL) &&
		((k0 = (g1_table_mod->uniltbl)[c3]) != 0)) {
		oconv(k0);
	    } else if (is_tblshort(g1_table_mod->char_width) &&
	    	(g1_table_mod->unitbl != NULL) &&
		((k0 = (g1_table_mod->unitbl)[c3]) != 0)) {
		oconv(k0);
	    } else in_undefined(c1,SKF_OUTTABLE);
	};
    } else {	/* arib_macro			   */
	if (is_charset_macro(g0_table_mod) == 1) {
	    (void)paraphrase_arib_macro(c1);
	} else;
    };
    return(sOCD);
}
/* --------------------------------------------------------------- */
static inline int ms_in_calc_index(c1,c2)
int c1,c2;
{
    if ((c1 <= 0xfc) && (c1 >= 0x40) && (c1 != 0x7f)
	&& (c2 <= 0xfc)) {   /* can convert into JIS   */
	c2 = c2 + c2 - ((c2 <= 0x9f) ? 0xe1 : 0x161);
	if (c1 < 0x9f) {
	    c1 -= ((c1 > A_DEL) ? 0x20 : 0x1f);
	} else { 
	    c1 -= 0x7e; c2++;
	};
    } else {			/* inconsistent in ms	   */
    	in_sbroken(c2,c1);
	return(sABRT);
    };
#if	defined(ROT_SUPPORT) && !defined(NEW_ROT_CODE)
    if (is_rot_encoded) {	/* rot47		   */
      c2 = skf_rot47conv(c2);
      c1 = skf_rot47conv(c1);
    };
#endif
    return(index_cal(c1,c2));
}

static int ms213_in_calc_index(c1,c2)
int c1,c2;
{
    int c3;

    c3 = c2;

    if ((c1 <= 0xfc) && (c1 >= 0x40) && (c1 != 0x7f)) {
	if (c2 <= 0xef) {	/* can convert into JIS	   */
	    return(ms_in_calc_index(c1,c2));
	} else { 		/* X-0213 pl.2		   */
	    if (c2 <= 0xf4) {
		c3 = x213_sjis_map
		    [((c2 - 0xf0) << 1)+ ((c1<0x9f) ? 0 : 1)];
	    } else if (c2 <= 0xfc) {
		c3 = (c2 << 1) - 0x17b;
	    } else {
		in_undefined((c2 << 8)+c1, SKF_OUTTABLE);
		return(sABRT);
	    };
	    if (c1 < 0x9f) {
		c1 -= ((c1 > A_DEL) ? 0x20 : 0x1f);
	    } else {
		c1 -= 0x7e; if (c2>=0xf4) c3++;
	    };
	    c2 = c3;
	};
    } else {			/* inconsistent in ms	   */
    	in_sbroken(c2,c1);
	return(sABRT);
    };
#if	defined(ROT_SUPPORT) && !defined(NEW_ROT_CODE)
    if (is_rot_encoded) {	/* rot47		   */
      c2 = skf_rot47conv(c2);
      c1 = skf_rot47conv(c1);
    };
#endif
    c3 = index_cal(c1,c2);
    return(c3);
}
/* --------------------------------------------------------------- */
static int gbk_in_calc_index(c1,c2)
int c1,c2;
{
    int c3;

    if ((c1 >= 0x40) && (c1 < A_DEL)) {
	c3 = ((c2 - GBK_UOFFSET) * 190) + (c1 - 0x40);
    } else if ((c1 > A_DEL) && (c1 <= 0xfe)) {
	c3 = ((c2 - GBK_UOFFSET) * 190) + (c1 - 0x41);
    } else {
    	in_sbroken(c2,c1);
	return(sABRT);
    };
    return(c3);
}
/* --------------------------------------------------------------- */
static int gb2k_in_calc_index(c1,c2)
int c1,c2;
{
    int c3;
    int	d2,d3;
    unsigned long encode;

    encode = i_codeset[in_codeset].encode;
    if (is_gb18030(encode)) {  /* GB 18030 Area 5		   */
    	if (c2 < 0x100) {	/* get 2nd character		   */
	    if ((c1 >= 0x30) && (c1 <= 0x39) &&
		(((c2 >= 0x81) && (c2 <= 0x84)) ||
		 ((c2 >= 0x90) && (c2 <= 0xe3)))) {
		return(sCONT); 
	    } else if (c1 < 0x40) {
	    	in_sbroken(c2,c1);
		return(sABRT);
	    } else;		/* all other GBK area		   */
	} else if (c2 < 0x10000) { /* get 3rd character		   */
	    if ((c1 >= 0x81) && (c1 < 0xff)) {
		return(sCONT); 
	    } else {
		d2 = c2 & 0xff;
		c2 = (c2 >> 8) & 0xff;
	    	in_sbroken(c2,d2);
		in_undefined(c1,SKF_IBROKEN);
		return(sABRT);
	    };
	} else {	/* GBK area 5 captured			   */
	    d3 = (c2 & 0xff);
	    d2 = (c2 >> 8) & 0xff;
	    c2 = (c2 >> 16) & 0xff;
	    gbk_area5_conv(c1,c2,d2,d3,0x81,0x81);
	    return(sABRT);
	};
	/* pure gbk */
	c3 = ((c2 - GBK_UOFFSET) * 190) + c1 - 0x40
	    - ((c1 >= A_DEL) ? 1 : 0);
    } else if ((c1 >= 0x40) && (c1 < A_DEL)) {
	c3 = ((c2 - GBK_UOFFSET) * 190) + (c1 - 0x40);
    } else if ((c1 > A_DEL) && (c1 <= 0xfe)) {
	c3 = ((c2 - GBK_UOFFSET) * 190) + (c1 - 0x41);
    } else {
      in_undefined(c2,SKF_IUNDEF);
      if ((c1 >= 0xfd) || (c1 == 0x7f)) {
	  in_undefined(c1,SKF_OUTTABLE);
      } else oconv(c1);	/* c1 < 0x40		   */
      return(sABRT);
    };

    return(c3);
}
/* --------------------------------------------------------------- */
#ifdef FUTURESUPPORT
static int thru_in_calc_index(c1,c2)
int c1,c2;
{
    return(c2);
}
#endif
/* --------------------------------------------------------------- */
static int johab_in_calc_index(c1,c2)
int c1,c2;
{
    int c3;

    c3 = c2;
    if ((c2 <= JOHAB_HANGLE) 
	    && (c1 >= 0x41) && (c1 < A_DEL)) {
	c3 = (c2 - 0x84) * 188 + (c1 - 0x41);
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," hngl-jhb");
#endif
    } else if ((c2 <= JOHAB_HANGLE) 
	    && (c1 >= 0x81) && (c1 <= 0xfe)) {
	c3 = (c2 - 0x84) * 188 + (c1 - 0x43);
    } else if ((c2 >= JOHAB_HANJA0) && (c2 <= JOHAB_HANJA0E)
	    && (c1 >= 0x31) && (c1 < A_DEL)) {
	c3 = ((((c2 - JOHAB_HANJA0) << 1) + 0) * 94) + c1 - 0x31;
    } else if ((c2 >= JOHAB_HANJA0) && (c2 <= JOHAB_HANJA0E)
	    && (c1 >= 0x91) && (c1 <= 0xfe)) {
	c3 = ((((c2 - JOHAB_HANJA0) << 1) + 0) * 94) + c1 - 0x43;
    } else if ((c2 >= JOHAB_HANJA1) && (c2 <= JOHAB_HANJA1E)
	    && (c1 >= 0x31) && (c1 < A_DEL)) {
	c3 = ((((c2 - JOHAB_HANJA1) << 1) + 41) * 94) + c1 - 0x31;
    } else if ((c2 >= JOHAB_HANJA1) && (c2 <= JOHAB_HANJA1E)
	    && (c1 >= 0x91) && (c1 <= 0xfe)) {
	c3 = ((((c2 - JOHAB_HANJA1) << 1) + 41) * 94) + c1 - 0x43;
    } else {
	in_undefined(c2,SKF_IUNDEF);
	return(sABRT);
    };
    return(c3);
}

/* --------------------------------------------------------------- */
static int big5_in_calc_index(c1,c2)
int c1,c2;
{
    return(big5c_in_calc_index(c1,c2,(int)BIG5_UOFFSET));
}
/* --------------------------------------------------------------- */
static int big5p_in_calc_index(c1,c2)
int c1,c2;
{
    return(big5c_in_calc_index(c1,c2,(int)BIG5P_UOFFSET));
}
/* --------------------------------------------------------------- */
static int big5c_in_calc_index(c1,c2,u_offset)
int c1,c2;
int u_offset;
{
    int c3;

    if ((c1 >= 0x40) && (c1 < A_DEL)) {
	c3 = ((c2 - u_offset) * 190) + (c1 - 0x40);
    } else if ((c1 > A_DEL) && (c1 <= 0xfe)) {
	c3 = ((c2 - u_offset) * 190) + (c1 - 0x41);
    } else {
    	in_sbroken(c2,c1);
	c3 = sABRT;
    };
    return(c3);
}

/* --------------------------------------------------------------- */
static int viqr_in_calc_index(c1,c2)
int c1,c2;
{
    int c3,c4;
    unsigned long encode;

    encode = i_codeset[in_codeset].encode;
    if ((is_viqr(encode) && (c2 >= 0x100)) ||
	(is_vimn(encode) && (c2 >= 0x100))) {
	c3 = (c2 & 0xff);
	c2 = (c2 >> 8) & 0xff;
	c4 = viqr_parse(c2,c3,c1,encode);
	if (c4 >= 0) {
	    return(sRETY);
	} else;
    } else if ((is_viqr(encode) && is_viqr_ch3(c2,c1)) ||
	       (is_vimn(encode) && is_vimn_ch3(c2,c1))) {
	return(sCONT);
    } else if ((c3 = viqr_parse(c2,0,c1,encode)) >= 0) {
    	return(sRETY);
    } else;
    return(sABRT);
}

/* --------------------------------------------------------------- */
static int vni_in_calc_index(c1,c2)
int c1,c2;
{
    int c4;
    unsigned long encode;

    encode = i_codeset[in_codeset].encode;
    c4 = viqr_parse(c2,c1,0,encode);
    if (c4 > 0) {	/* c1 and c2 IS consumed   */
    	return(sCONT);
    } else;
    return(sABRT);
}

/* --------------------------------------------------------------- */
static int hz8_in_calc_index(c1,c2)
int c1,c2;
{
    return(index_cal((c1 & 0x7f),(c2 & 0x7f)));
}

/* --------------------------------------------------------------- */
static int iso_in_calc_index(c1,c2)
int c1,c2;
{
    return(index_cal(c1,c2));
}

/* --------------------------------------------------------------- */
static int dmy_in_calc_index(c1,c2)
int c1,c2;
{
    in_sbroken(c2,c1);
    return(sABRT);
}

/* --------------------------------------------------------------- */
static int iscii_in_calc_index(c1,c2)
int c1,c2;
{
    return(big5c_in_calc_index(c1,c2,(int)BIG5_UOFFSET));
}

/* --------------------------------------------------------------- */
static int nniso_in_calc_index(c1,c2)
int c1,c2;
{
    return(big5c_in_calc_index(c1,c2,(int)BIG5_UOFFSET));
}

/* --------------------------------------------------------------- */
void gbk_area5_conv(c1,c2,d2,d3,low_bound,u_offset)
int c1,c2,d2,d3;
int low_bound,u_offset;
{
    int c3,c4;
    skf_ucode k0 = 0;

    if ((c2 >= low_bound) && (c2 <= 0x82)
	&& (d2 >= 0x30) && (d2 <= 0x39)) {
	/* u+0000 to u+4dff(19042) */
	c3 = ((((((c2 - u_offset) * 10)
		+ (d2 - 0x30)) * 126)
		+ (d3 - u_offset)) * 10) + c1 - 0x30;
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"(%x)",c3);
#endif
	if ((c3 < 19080) && (gb2k_a5_tbl != NULL)) {
	    k0 = gb2k_a5_tbl[c3]; /* -82358f02 */
	    if (k0 != 0) oconv(k0);
	    else in_undefined(c3,SKF_OUTTABLE);
	} else if ((c3 >= 19080) && (c3 < 25200)) {
	    k0 = c3 + 0x5543; /* U-9fcb ...	  */
	    if (k0 != 0) oconv(k0);
	    else in_undefined(c3,SKF_OUTTABLE);
	} else in_undefined(c2,SKF_OUTTABLE);
    } else if ((c2 == 0x83) ||
		((c2 == 0x84) && (d2 >= 0x30) && (d2 <= 0x31))) {
	/* u+b7b3 to u+ffdf */
	c4 = ((((((c2 - 0x83) * 10)
		+ (d2 - 0x30)) * 126)
		+ (d3 - u_offset)) * 10) + c1 - 0x30;
	/* base: offset=25200(6270) */
	c3 = c4 + 0xb7b3;
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"(%x)",c3);
#endif
	/* 0x8336c739 = U-e76c */
	if (c3 > 0xd7ff) {
#if defined(SHORTEN_GB2K)
	    c3 = c3 - 0x8ab3;
#else
	    c3 = c3 - 0x6ffb;
#endif
	    if ((c3 < 32760) && (gb2k_a5_tbl != NULL)) {
		k0 = gb2k_a5_tbl[c3];
		if (k0 != 0) oconv(k0);
		else in_undefined(c3,SKF_OUTTABLE);
	    } else in_undefined(c3,SKF_OUTTABLE);
	} else oconv(c3);	/* rest of hanguls  */
    } else if ((c2 >= 0x90) && (c2 <= 0xe3)) {
	    /* GB18030 to U+10000-U+10FFFF	    */
	c4 = ((((((c2 - 0x90) * 10)
		+ (d2 - 0x30)) * 126)
		+ (d3 - u_offset)) * 10)
		+ c1 - 0x30 + 0x10000;
	/* base: offset=25200(6270) */
	oconv(c4);
    } else in_undefined(c2,SKF_IBROKEN);
}
/* --------------------------------------------------------------- */
/*  be_in: Broken JIS and rot support				   */
/* --------------------------------------------------------------- */
/* Since this routine provide limited support for legacy feature   */
/* code support is strictly limited. Especially, rot directly pass */
/* codes to tail rouitne, output support is limited to sjis, cp932 */
/* euc, cp51932 and iso-2022-jp.				   */
/* --------------------------------------------------------------- */

/*@-globstate@*/ /* looks strange. should check it later.	   */
int be_in(f)			/* be-in			   */
skfFILE *f;			/* can process euc, euc+jis-mix	   */
{				/*  ... jis code contains no kana  */
    register int    c1, c2 = 0;
    skf_ucode   k0;		/* utf32 result			   */
    int    ch;
    int	   c2s,c1s;
    int	   c3;
    int    mod = 0;
    /* Note: e_in doesn't preserve tail BS, because no following   */
    /*  modifier character(s) is given.				   */
    unsigned long	encode;

    skf_input_lang = M_JP;

    encode = i_codeset[in_codeset].encode;
#if defined(ROT_SUPPORT) && defined(NEW_ROT_CODE) 
    if (nkf_compat && is_nkf_rotmode) mod = TRUE;
    else mod = FALSE;
#endif

    /* --- conversion loop --------------------------------------- */
    while (TRUE) {
	if (c2) {       /* ***** second byte ********************* */
	    c2s = c2;	/* save for later test			   */
	    if ((c1 = vGETC(f)) == sEOF) {
			/* c2 is rot'd even if c1 is EOF	   */
		if (!encode_enbl || !is_mime_nkfmode) {
			in_undefined(c2,SKF_UNEXPEOF);
		};
		break;
	    } else if (c1 == sOCD) return(sOCD);
	    c1s = c1;
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x",c1);
#endif
	    c3 = c2;
	    if (is_msfam(encode)) {
	      if ((c1 <= 0xfc) && (c1 >= 0x40)
			&& (c1 != 0x7f)) { 
/*@-type@*/
		if ((c2 <= 0xef) || (is_ms_213c(encode) == 0)) {
					/* can convert into JIS	   */
/*@+type@*/
		    c2 = c2 + c2 - ((c2 <= 0x9f) ? 0xe1 : 0x161);
		    if (c1 < 0x9f) {
			c1 -= ((c1 > A_DEL) ? 0x20 : 0x1f);
		    } else { 
			c1 -= 0x7e; c2++;
		    };
		} else { 		/* X-0213 pl.2		   */
		    if (c2 <= 0xf4) {
			c3 = x213_sjis_map
			    [((c2 - 0xf0) << 1)+ ((c1<0x9f) ? 0 : 1)];
		    } else if (c2 <= 0xfc) {
			c3 = (c2 << 1) - 0x17b;
		    } else {
			in_undefined((c2 << 8)+c1, SKF_OUTTABLE);
			c2 = 0; continue;
		    };
		    if (c1 < 0x9f) {
			c1 -= ((c1 > A_DEL) ? 0x20 : 0x1f);
		    } else {
			c1 -= 0x7e; if (c2>=0xf4) c3++;
		    };
		    c2 = c3;
		};
	      } else {			/* inconsistent in ms	   */
		  in_undefined(c2,SKF_IUNDEF);
		  if ((c1 >= 0xfd) || (c1 == 0x7f)) {
		      in_undefined(c1,SKF_OUTTABLE);
		  } else SKF_rotoconv(c1,mod);	/* c1 < 0x40	   */
		  res_single_shift;
		  c2 = 0; continue;   	/* goto next_word          */
	      };
	    } else if (is_multibyte(up_dbyte) && (c1 > A_KSP)) {
		c2 &= 0x7f; c1 &= 0x7f;
		if (c1 == 0x7f) { /* DEL with bit8 == 1 is bogus   */
		    in_undefined((c2 << 8) + c1, SKF_IBROKEN);
		    shift_cond_recovery(); c2 = 0; continue;
		};
		/* broken jis never comes here with c2 > KSP */
	    } else if ((c2 == '$') && is_nkf_jbroken && !low_dbyte &&
	    	((c1 == 'B') || (c1 == '@'))) {
		/* broken JIS entry	*/
		if (c1 == '@') {
		    g0_table_mod = &(iso_3_dblbyte_defs[jisc6226_78_index]);
		} else {
		    g0_table_mod = &(iso_3_dblbyte_defs[x0208_index]);
		};
		g0table2low();
		c2 = 0; continue;
	    } else if ((c2 == '(') && is_nkf_jbroken && low_dbyte &&
	    	((c1 == 'B') || (c1 == 'J'))) {
		/* broken JIS exit	*/
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"-BE-");
#endif
		if (c1 == 'J') {
		    g0_table_mod = &(iso_unibyte_defs[x0201_index]);
		} else {
		    g0_table_mod = &(iso_unibyte_defs[ascii_index]);
		};
		g0table2low();
		c2 = 0; continue;
	    } else if ((c1 > 0x20) && (c1 < A_DEL) && low_dbyte) {
		;	 	/* JIS case: pass as it is	   */
	    } else { 		/* inconsistent case:		   */
		/* case: is_jis c1 < A_SP			   */
		/* 	 is_jis c1 >= DEL 			   */
		/* 	 is_euc c1 <= DEL 			   */
		in_undefined(c2,SKF_IBROKEN);
		if (c1 == A_ESC) { /* proceed anyway.		   */
		    if ((c1 = esc_process(f)) < 0) return(c1);
		} else in_undefined(c1,SKF_IBROKEN);
		shift_cond_recovery();
		c2 = 0; continue;   	/* goto next_word          */
	    };
	    /* --- 0x20 < c2 < 0x7f, 0x20 < c1 < 0x7f ------------ */
	    /* Note: EUC, MS-JIS and JIS come here with low_dbyte  */
	    k0 = 0;		/* clear result value 		   */

	    ch = index_cal(c1,c2);
	    /* ROT */
	    if (is_nkf_rotmode) {
	    	SKFROTTHRU(c2,c1);
		c2 = 0; continue;
	    } else;
	    /* BROKEN JIS */
#ifdef SKFDEBUG
	    if (is_vv_debug) {
		fprintf(stderr,"(%d)",ch); (void)fflush(stderr);
	    };
#endif
		/* normal JIS(X-0208, X-0212) case.	   */
	    if ((ch >= X0208_KANJI_END) && (use_cde_compat) &&
		(((c2s < A_DEL) && (is_tbl_kanji(low_kana)))
		    || ((c2s > A_KSP) && (is_tbl_kanji(up_kana))))) {
		    		/* X-0208 & CDE	   */
		if (((c2s < A_DEL) && (is_tbl_x0208(low_kana)))
		    || ((c2s > A_KSP) && (is_tbl_x0208(up_kana)))) {
		    k0 = UNI_UDF_A + ch - X0208_KANJI_END;
		} else {	/* X-0212 */
		    k0 = UNI_UDF_B + ch - X0208_KANJI_END;
		};
	    } else if (((c2s < A_DEL) && (ch < low_table_limit))
		     || ((c2s > A_KSP) && (ch <up_table_limit))) {
		if (is_euc_mseuc(encode) || is_cp5022x(encode)) {
		    if (ch < X0208_KANJI_END) {
			if (is_cp5022x(encode)) k0 = low_table[ch];
			else k0 = up_table[ch];
		    } else if ((((c2 >= 0x75) && (c2 < 0x79))
			    || ((c2 >= 0x7d) && (c2 <= 0x7e)))
			    && (c1 > A_SP)
			    && (is_euc_mseuc(encode))) {
			k0 = ((c2s - 0xf5) * 94)
			    + (c1s - 0xa1) + 0xe40c;
		    } else if ((c2 < 0x7d) &&
			(((c1 > A_SP) && (is_cp5022x(encode))) ||
			(((c1s > A_KSP)
			&& (is_euc_mseuc(encode)))))) { /* NEC Gaiji   */
			if (is_cp5022x(encode)) {
			    if (is_tbllong(low_table_mod->char_width))
			    	k0 = low_ltable[ch];
			    else k0 = low_table[ch];
			} else {
			    if (is_tbllong(up_dbyte)) k0 = up_ltable[ch];
			    else k0 = up_table[ch];
			};
		    } else if (((c2s >= 0xf5) &&
			    (c1s < 0xa1) && (c1s != A_DEL)) ||
			    ((c2s == 0xf4) &&
			     ((c1s < 0xa1) || (c1s > 0xa6)))) {
		    /* cp932 - cp51932 conversion bug recovery */
			k0 = 0xe000 + ((c2 - 0xf0) * 188) 
			    + (c1 - 0x40) + ((c1 < A_DEL) ? 0 : 1);
		    } else;
		} else if (c2s > A_KSP) {
			/* normal EUC case.			   */
		    if (c1s > A_KSP) {   /* normal EUC & JIS	   */
			if (up_dbyte > 1) k0 = up_ltable[ch];
			else k0 = up_table[ch];
		    } else if (gx_table_mod->table_len != 0) {
			if (ch < gx_table_mod->table_len) {
			    if (is_tbllong(gx_table_mod->char_width))
				k0 = gx_table_mod->uniltbl[ch];
			    else k0 = gx_table_mod->unitbl[ch];
			} else k0 = 0;
		    } else k0 = 0;
		} else {
		    if (is_tbllong(low_table_mod->char_width))
		    	k0 = low_ltable[ch];
		    else k0 = low_table[ch];
		};
	    } else;
	    if (k0 != 0) {	/* not found in tables.		   */
		oconv(k0);	/* ROT do not come here		   */
	    } else {
		in_sbroken(c2,c1);
	    };
	    c2 = 0;
	    continue;
	} else {	/* ***** first byte ********************** */
	    if ((c1 = vGETC(f)) < 0) return(c1);
#ifdef SKFDEBUG
	    if (is_v_debug) {
		fprintf(stderr,"\nbe_in:%02x",c1); 
	    };
#endif
	/* skf does not detect SO+ASCII as a SJIS kanji code.	   */
	/*  ... Because I've never seen such weird coding scheme.  */
#ifdef SKFDEBUG
	    if (is_vv_debug) {
	    	if (low_dbyte) fprintf(stderr,"D");
	    	if (is_nkf_jbroken) fprintf(stderr,"B");
	    	else if (is_nkf_jfbroken) fprintf(stderr,"F");
	    	else if (is_nkf_jffbroken) fprintf(stderr,"Z");
	    } else;
#endif
	    if (c1 <= A_SP) { 	/* 0x00-0x20			   */
		/* lower side is always treated as 94 charset.	   */
	    	if (c1 == A_SP) {	/* space hook		   */
		    SKF_rotoconv(c1,mod); 
		} else if (c1 < 0x0e) {	/* 0x00-0x0d		   */
		    SKF_rotoconv(c1,mod);/* common control code	   */
		    if (is_nkf_jffbroken && is_lineend(c1)) {
		    	res_all_shift;
		    } else;
		    if (kuni_opt && is_lineend(c1)) {
			res_all_shift; return(c1);
		    };
		} else if (c1 == A_ESC) { /* escape treatment	   */
		    if ((c1 = esc_process(f)) < 0) return(c1);
		    if (low_table == NULL) low_table = ascii_uni_byte;
		    c2 = 0; continue;
		} else SKF_rotoconv(c1,mod); /* just pass other code. */
		continue;
	/* --- ASCII alphanumeric area --------------------------- */
	    } else if ((c1 == '$') && !low_dbyte && is_nkf_jbroken) {
	    		/* BROKEN JIS HANDLING			   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"Brki? ");
#endif
		c2 = c1;
		continue;
	    } else if ((c1 == 0x28) && low_dbyte && is_nkf_jbroken) {
	    		/* BROKEN JIS HANDLING			   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"Brko? ");
#endif
		c2 = c1;
		continue;
	    } else if (c1 <= A_DEL) {	/* 0x21-0x7f               */
		if (low_dbyte) { 
				/* lower plane is 2byte locked	   */
		    c2 = c1 ;	/* pass it to second byte	   */
		    continue;	/* goto next_byte		   */
		} else if ((k0 = low_table[c1]) != 0) {
			    /* is not always ascii.		   */
		    if (is_nkf_rotmode) {
			SKFROTTHRU(0,c1);
		    } else SKF_rotoconv(k0,mod);
		    continue;
		} else {
		    in_undefined(c1,SKF_OUTTABLE);
		};
		/* --- 0xa1 <= c1 <= 0xff ------------------------ */
	    } else if (c1 >= A_KSP) {	/* c1 >= 0xa0		   */
			/* code is within JIS upper plane	   */
		if (up_dbyte) {
				/* lower plane is 2byte locked	   */
		    c2 = c1 ;	/* pass it to second byte	   */
		    continue;	/* goto next_byte		   */
		} else if ((up_table != NULL) && 
			((k0 = up_table[c1 & 0x7f]) != 0)) {
		    if (is_nkf_rotmode) {
			SKFROTTHRU(0,c1);
		    } else SKF_rotoconv(k0,mod);
		    continue;
		} else {	
		    in_undefined(c1,SKF_IUNDEF);
		};
		/* --- rest 0x80 <= c1 < 0xa0 -------------------- */
		/* comparison is for KOI-8-R consideration	   */
	    } else if ((c1_table_mod != NULL) && 
	    		(c1_table_mod->unitbl != NULL)) {
		if ((c1 = c1_process(f,c1)) < 0) return(c1);
	    } else if (is_cp5022x(encode) && (c1 <= 0x98)) {
		c2 = c1; continue;
	    } else {	/* 0x80 <= c1 < 0xa0 & !is_msfam	   */
		if ((low_table_limit <= 128) && (up_table == NULL)
			&& (up_ltable == NULL)) {
		    in_undefined(c1,SKF_IRGTUNDEF);
		} else if ((up_ltable != NULL) && 
			((k0=(skf_ucode)(up_ltable[c1 & 0x7f])) != 0)){
		    if (is_nkf_rotmode) {
			SKFROTTHRU(0,c1);
		    } else SKF_rotoconv(k0,mod);
		    continue;
		} else {	
		    in_undefined(c1,SKF_IUNDEF);
		};
	    };
	};
    };
    return(sEOF); 
}
/* --------------------------------------------------------------- */
/* repeat control(generation) for arib				   */
/* sy: repeat character, cnt: repeat count + 1			   */
/* --------------------------------------------------------------- */
void rpclockparse(sy,cnt)
skf_ucode sy;
int cnt;
{
    int i;

    cnt--;	/* cnt has one offset				   */
#if 0
    if (((sy >= 0x1000) && (sy <= 0xff60)) || (sy >= 0xffe0)) {
    	cnt = (cnt+1) >> 1;
    } else;
#endif
    if (cnt > 0) {
	for (i=0;i<cnt;i++) oconv(sy);
    } else {
    	oconv(sFLSH);	/* get current position in line		   */
#if 0
	cnt = mime_fold_llimit - fold_count - ARIB_RPC_MGN - 1;
#else
	cnt = mime_fold_llimit - fold_count - 1;
#endif
#ifdef SKFDEBUG
	if (is_vvv_debug) {
	    fprintf(stderr,"autorep: %d(%d,%d) ",cnt,mime_fold_llimit,fold_count);
	} else;
#endif
	/* TODO get character width and fix	*/
	if ((sy >= 0x3000) && (sy < 0xff60)) cnt = (cnt >> 1);
	for (i=0;i<cnt;i++) oconv(sy);
    };
    return;
}
/* --------------------------------------------------------------- */
