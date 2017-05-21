/* *******************************************************************
** Copyright (c) 1993-2015 Seiji Kaneko. All rights reserved.
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
** oconv.c:	unic*de post-handler. called from in_converter.
** $Id: oconv.c,v 1.145 2017/01/05 15:05:48 seiji Exp seiji $
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
#else
#include <strings.h>
#endif

#if defined(ROT_SUPPORT) && defined(NEW_ROT_CODE) 
static void SKFROTPUT P_((int));
#endif
/* --------------------------------------------------------------- */
/* conversion tables						   */
/* --------------------------------------------------------------- */
/* --- language tag save areas ----------------------------------- */
static  int	lang_tag_index = 0;

/* --------------------------------------------------------------- */
/* code output definitions					   */
/* --------------------------------------------------------------- */
unsigned int	g0_char = 'J';	/* g0 iso-2022-char		   */
unsigned int	g0_mid = '(';	/* g0 iso-2022-char mid call	   */
unsigned int	g0_midl = 0;	/* g0 iso-2022-char 4th call	   */
unsigned long	g0_typ = 0;	/* g0 iso-2022-char type	   */
unsigned int	ag0_char = 0;	/* ag0 iso-2022-char		   */
unsigned int	ag0_mid = '$';	/* ag0 iso-2022-char mid call	   */
unsigned int	ag0_midl = '(';	/* ag0 iso-2022-char 4th call	   */
unsigned long	ag0_typ = COD_MB;	/* ag0 iso-2022-char type   */
unsigned int	g1_char = 0;	/* g1 iso-2022-char		   */
unsigned int	g1_mid = 0x2a;	/* g1 iso-2022-char mid call	   */
unsigned int	g1_midl = 0;	/* g1 iso-2022-char 4th call	   */
unsigned long	g1_typ = COD_SET96;	/* g1 iso-2022-char type   */
unsigned int	g2_char = 0;	/* g2 iso-2022-char		   */
unsigned int	g2_mid = '(';	/* g2 iso-2022-char mid call	   */
unsigned int	g2_midl = 0;	/* g2 iso-2022-char 4th call	   */
unsigned long	g2_typ = 0;	/* g2 iso-2022-char type	   */
unsigned int	g3_char = 0;	/* g3 iso-2022-char		   */
unsigned int	g3_mid = '$';	/* g3 iso-2022-char mid call	   */
unsigned int	g3_midl = '(';	/* g3 iso-2022-char 4th call	   */
unsigned long	g3_typ = COD_MB | COD_MB_4;
				/* g3 iso-2022-char type	   */
unsigned int	g4_char = 'P';	/* g4 iso-2022-char		   */
unsigned int	g4_mid = '$';	/* g4 iso-2022-char mid call	   */
unsigned int	g4_midl = '(';	/* g4 iso-2022-char 4th call	   */
unsigned long	g4_typ = COD_MB | COD_MB_4;
				/* g4 iso-2022-char type	   */
/* --------------------------------------------------------------- */
/* ISO 2022 status definition					   */
/*  only g0 has mode in skf					   */
/*   g0_mod:	0 - g0 definition is used			   */
/*		1 - ag0 definition is used			   */
/*		-1 - Neither g0 nor ag0				   */
/* code must be returned to g0 before locking shift(skf convension)*/
/* skf does not hold locking shift, for performance reason	   */
/* --------------------------------------------------------------- */
/* ---  shift switches ---------------------------------------- */
int		g0_mod = 0;

unsigned long	g0_output_shift = 0;
unsigned long	g1_output_shift = 0;
unsigned long	g23_output_shift = 0;

unsigned long	hzzwshift = 0;		/* for HZ and zW	   */

int		fold_count = 0;	   /* linewise character count	   */

skf_ucode	sgbuf = 0;		/* oconv single buffer	   */
int		sgbuf_buf = 0;

skf_ucode	oobuf[OOBUFSIZE];  /* MIME/fold buffer		   */
int		oobufip = 0;	/* write pointer		   */
int		oobufop = 0;	/* read pointer			   */

#ifdef FOLD_SUPPORT
static int	is_intag = FALSE; /* within HTML tags		   */
#endif

/* --------------------------------------------------------------- */
#ifdef UNI_DECOMPOSE
static int		decompose_recursion_depth = 0;
static skf_ucode	decompose_buf[32];
static int		decompose_bufp;
static void		decompose_code_dec P_((int));

#ifdef UNI_ENCOMPOSE
static skf_ucode	encompose_buf[ENCOMPOSE_DEPTH];
static int		encompose_tail;
struct encompose_leaf	*encompose_tree;
static int		encompose_start[ENCOMPOSE_SDEPTH];

int	encompose_list[ENCOMPOSE_TBLSIZE];

static	int	skf_encompose P_(((skf_ucode *),int,int,int));
static  unsigned short	enck_mask[ENCOMPOSE_MASK_LEN];

/* table indicates entry # of encompose_tree -1 */
/* for 0x0000 - 0x1ffff in each 256-char set	*/
static int default_enck_mask[ENCOMPOSE_MASK_LEN] = {
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1,

 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};
#endif
#endif

/* --------------------------------------------------------------- */
const char u_dakuten[96] = {
 0,0,0,0,0,0,1,0,0,0,0,4,0,4,0,4,
 0,4,0,4,0,1,0,1,0,1,0,4,0,1,0,1,
 0,1,0,0,4,0,1,0,4,0,0,0,0,0,0,3,
 0,0,3,0,0,3,0,0,3,0,0,3,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
 1,1,1,0,0,0,0,0,0,0,0,0,0,1,0,0,
};

#ifdef FOLD_SUPPORT
/* --------------------------------------------------------------- */
const char kinsoku_map0[256] = {
 0x00,0x01,0x01,0x01,0x00,0x01,0x00,0x00,
 0x10,0x01,0x10,0x01,0x10,0x01,0x10,0x01,
 0x10,0x01,0x00,0x00,0x10,0x01,0x10,0x01,
 0x10,0x01,0x10,0x01,0x00,0x10,0x01,0x01,

 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x01,0x01,0x10,0x01,0x00,0x00,0x01,
 0x00,0x00,0x00,0x00,0x01,0x10,0x00,0x00,

/* 0x3040 - Hiragana */
 0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,
 0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

 0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

 0x00,0x00,0x00,0x03,0x00,0x03,0x00,0x03,
 0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
 0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,
 0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,

/* 0x30a0 - Katakana */
 0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,
 0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

 0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

 0x00,0x00,0x00,0x03,0x00,0x03,0x00,0x03,
 0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
 0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,
 0x00,0x00,0x00,0x03,0x01,0x01,0x01,0x01
};

/* --------------------------------------------------------------- */
skf_ucode	prev_ch = 0;
#endif

/* --------------------------------------------------------------- */
/* oobuf enque/deque						   */
/* --------------------------------------------------------------- */
skf_ucode pokeoobuf()
{
    int po;
    po = oobufop;
    oobufop = (oobufop == (OOBUFSIZE - 1)) ? 0 : (oobufop + 1);
    return(oobuf[po]);
}

void pushoobuf(ch)
skf_ucode ch;
{
    oobuf[oobufip] = ch;
    oobufip = (oobufip == (OOBUFSIZE - 1)) ? 0 : (oobufip + 1);
}

#define oobufempty()	(oobufip == oobufop)
#define oobufhas1()	((oobufip == (oobufop + 1)) \
		|| ((oobufip == 0) && (oobufop == (OOBUFSIZE - 1))))

#define oobuffull()	((oobufop == (oobufip + 1)) \
		|| ((oobufop == 0) && (oobufip == (OOBUFSIZE - 1))))
/* --------------------------------------------------------------- */
/* code output initializer					   */
/* --------------------------------------------------------------- */
/*@-globstate@*/
int	oconv_init()
{
/* undefine character fix */
    if (out_codeset < 0) return(0);
    if ((pref_subst_char > 0) && (test_out_char(pref_subst_char) != 0)) {
	ucode_undef = pref_subst_char;
    } else if (!is_ucs_ufam(conv_cap)) { /* NOT ucs, brgt, trans   */
	if ((uni_o_kana_defs != NULL) && (uni_o_kana != NULL) &&
	     (test_out_char(0x3013) != 0) &&
	     ((uni_o_kana[0x13] < 0x8000) || (out_bg(conv_cap)))) {
	     ucode_undef = 0x3013;
	} else if ((uni_o_symbol_defs != NULL) && (uni_o_symbol != NULL) &&
	    (test_out_char(0x25a0) != 0) &&
	    ((uni_o_symbol[0x5a0] < 0x8000) || (out_bg(conv_cap)))) {
	     ucode_undef = 0x25a0;
	} else {
	    ucode_undef = '.';	/* some unknown code treats	   */
	};
	if (is_euc_51932(conv_cap)) {
	    set_kana_call;
	};
    } else {
	if (use_uni_repl) {
	    ucode_undef = 0xfffd;
	} else {
	    ucode_undef = 0x3013;
	};
    };

    if (is_o_encode) mime_limit_set();

    if (is_o_encode) {
/* encoding and MIME */
    /* do not set encoding if GL part is not iso-2022 like code */
    	if (is_ucs_utf7(conv_cap) || is_ucs_brgt(conv_cap)
	  || is_ucs_uri(conv_cap)
	  || is_ucs_transp(conv_cap)
	  || is_hzzW(conv_cap) || is_vni(conv_cap) 
	  || is_viqr_or_vimn(conv_cap) 
	  || is_keis(conv_cap)) {
	  /* these output encodings do not support MIMEs	*/
	  out_undefined(sFLSH,SKF_ENC_ERR);
	  o_encode = 0;
	} else;
	if (is_o_encode_hex(o_encode) 
		|| (is_o_encode_oct(o_encode))) 
	    o_encode_stat = TRUE;
	else if (is_o_encode_b64(o_encode)) 
	    o_encode_stat = TRUE;
	else if (is_o_encode_oct(o_encode)) 
	    o_encode_stat = TRUE;
	else if (is_o_encode_uri(o_encode)) 
	    o_encode_stat = TRUE;
/* encoding and inquiry */
	if (input_inquiry) {
	  o_encode = 0;
	};
    } else;

    /* ------------------------ */
    /* UTF-8 Cellular overlay	*/
    /* ------------------------ */
    if (enable_cellconvert) {
#ifdef DYNAMIC_LOADING
	if (load_external_table(
		&(ovlay_byte_defs[emot_prv_c_index])) < 0) {
	    in_tablefault(SKF_PRESETFAIL,
			ovlay_byte_defs[emot_prv_c_index].desc);
	} else;
#endif
    };

#if UNI_ENCOMPOSE
    if (o_enbl_encomp) {
	if (load_external_table(&ovlay_byte_defs[unicode_enckm_index]) < 0) {
	    in_tablefault(SKF_PRESETFAIL,ovlay_byte_defs[unicode_enckm_index].desc);
	    return(-1);
	} else;
	enck_mask = ovlay_byte_defs[unicode_enckm_index].unitbl;
    } else {
    	enck_mask = default_enck_mask;
    };
#endif

#ifdef UNI_DECOMPOSE
    /* Unicord normalization */
    unicode_normalize_setup();
#endif
    /* -------------------------- */
    /* VISCII/VSQR Escape overlay */
    /* -------------------------- */
 /* TODO */
    return (0);
}

/* --------------------------------------------------------------- */
/* unic*de parse routine (oconv)				   */
/* --------------------------------------------------------------- */
/*  input: ch - Unicode character in UTF32			   */
/*  output: none (output error is ignored)			   */
/*  side-effect: output character(s) to stdout.			   */
/* --------------------------------------------------------------- */
/*  This routine performs various codeset independent processes,   */
/*  and passes output to codeset dependent post-process in	   */
/*  genoconv.c, ucodoconv.c or brgtconv.c.			   */
/* --------------------------------------------------------------- */
/* input: unicode U+0000 - U+10FFFF (except U+d800)		   */
/*  sFLSH is regarded as force-flush, and outputs nothing.	   */
/* --------------------------------------------------------------- */
/* Normalization note: 						   */
/* 3400 - f8ff		blank area in < 10000			   */
/* 1d15e - 1d1c0	musical symbols				   */
/* 2f800 - 2fa1a	cjk-cc					   */
/* --------------------------------------------------------------- */

void oconv(ch)			/* unic*de output parsing	   */
skf_ucode ch;
{
    skf_ucode	c1;
    skf_ucode	rch,vch;
    skf_ucode	c2;
    int		dd;
    int		de;
    int		x;
    unsigned short packedch;
#ifdef UNI_DECOMPOSE
    unsigned short uukp;
#endif
    unsigned short cnvpk;

#ifdef FOLD_SUPPORT
    int		charProp = 0;	/* character property buffer	   */
    char	kinsoku_stat;
    int		trim = 0;
    int		nkftrim = 0;
#endif

#ifdef SKFDEBUG
    if (is_vv_debug) {
	if (ch == sEOF) fprintf(stderr," oc:sEOF");
	else if (ch == sOCD) fprintf(stderr," oc:sOCD");
	else if (ch == sKAN) fprintf(stderr," oc:sKAN");
	else if (ch == sUNI) fprintf(stderr," oc:sUNI");
	else if (ch == sFLSH) fprintf(stderr," oc:sFLSH");
	else fprintf(stderr," oc:0x%04x",ch);
	if (sgbuf_buf == 0) fprintf(stderr,"@");
#ifdef FOLD_SUPPORT
	if (fold_fclap > 0)
	    fprintf(stderr," %d:%d-%d",fold_clap,fold_fclap,fold_count);
#endif
    };
#endif
    rch = ch; vch = 0;

    if (sgbuf_buf >= 1) {
	if (ch < 0) {
    	    if (ch == sOCD) return;
	    if (sgbuf_buf > 0) {
		sgbuf_buf = 0;
		ch = sgbuf;
	    } else {
	    	return;
	    };
	    if ((ch >= 0xff01) && (ch <= 0xff9f)) {
		if ((ch >= 0xff61) && (ch <= 0xff9f) && 
		    (is_nkf_no_hk ||
		     (!hk_enbl && !(use_compat && is_ucs_ufam(conv_cap))))) {
		    (void)x0201conv((ch - 0xff40),0xb0);
		    return;
		} else if ((ch >= 0xff01) && (ch <= 0xff5e) && 
		    (is_nkf_no_hk || is_o_prefer_ascii)) {
		    ch -= (0xff00 + 0x20);
		    /* pass to later process */
		} else;
	    } else;
	} else {
    /* --- Note: This is a point where all normalize packing	   */
    /* 		process should be inserted.			   */
    /* ----------------------------------------------------------- */
    /* ----------------------------------------------------------- */
	    if (!dsbl_ucod_encomp || enbl_ucod_cencomp) {
		if ((ch >= 0x3099) && (ch <= 0x309c) && (sgbuf_buf > 0)
		    && (sgbuf >= 0x3046) && (sgbuf <= 0x30f2)) {
	    /* --------------------------------------------------- */
	    /* unicode kana normalized unification		   */
	    /* --------------------------------------------------- */
		    c1 = ((sgbuf >= 0x30a0) ? 
			    (sgbuf - 0x30a0) : (sgbuf - 0x3040));
		    if (((ch == U_KDAK) || (ch == U_LDAK)) &&
			    (u_dakuten[c1] > 0) &&
		    	    (!dsbl_uni_kana_concat)) {
			vch = ch;
			if (c1 == 0x06) sgbuf += 0x4e;
			else if (sgbuf >= 0x30ef) sgbuf += 8;
			else sgbuf += 1;
			ch = sgbuf; sgbuf_buf = 0;
		    } else if (((ch == U_KHDK) || (ch == U_LHDK)) &&
			    (u_dakuten[c1] == 3) &&
		    	    (!dsbl_uni_kana_concat)) {
			vch = ch;
			ch = sgbuf + 2; sgbuf_buf = 0;
		    } else if ((ch == U_KHDK) && (u_dakuten[c1] == 4) &&
		    	    (!dsbl_uni_kana_concat ||
			    	enbl_uni_hain_concat)) {
			vch = ch;
			switch (sgbuf) {
			    case 0x304b: ch = 0xd801; sgbuf_buf = 0; break;
			    case 0x30ab: ch = 0xd808; sgbuf_buf = 0; break;
			    case 0x304d: ch = 0xd802; sgbuf_buf = 0; break;
			    case 0x30ad: ch = 0xd809; sgbuf_buf = 0; break;
			    case 0x304f: ch = 0xd803; sgbuf_buf = 0; break;
			    case 0x30af: ch = 0xd80a; sgbuf_buf = 0; break;
			    case 0x3051: ch = 0xd804; sgbuf_buf = 0; break;
			    case 0x30b1: ch = 0xd80b; sgbuf_buf = 0; break;
			    case 0x3053: ch = 0xd805; sgbuf_buf = 0; break;
			    case 0x30b3: ch = 0xd80c; sgbuf_buf = 0; break;
			    case 0x30bb: ch = 0xd80d; sgbuf_buf = 0; break;
			    case 0x30c4: ch = 0xd80e; sgbuf_buf = 0; break;
			    case 0x30c8: ch = 0xd80f; sgbuf_buf = 0; break;
			    default:
				ch = sgbuf;
				sgbuf = rch;
			};
		    } else {
			ch = sgbuf;
			sgbuf = rch;
		    };
		} else if ((sgbuf >= 0x00e6) && (sgbuf < 0x300) &&
			   ((ch == 0x300) || (ch == 0x301))) {
		    if ((sgbuf == 0x00e6) && (ch == 0x300)) {
			vch = ch; ch = 0xd814; sgbuf_buf = 0;
		    } else if (sgbuf == 0x0254) {
			vch = ch; ch = 0xd818 + ((ch == 0x301) ? 1 : 0);
			sgbuf_buf = 0;
		    } else if (sgbuf == 0x028c) {
			vch = ch; ch = 0xd81a + ((ch == 0x301) ? 1 : 0);
			sgbuf_buf = 0;
		    } else if (sgbuf == 0x0259) {
			vch = ch; ch = 0xd81c + ((ch == 0x301) ? 1 : 0);
			sgbuf_buf = 0;
		    } else if (sgbuf == 0x025a) {
			vch = ch; ch = 0xd81e + ((ch == 0x301) ? 1 : 0);
			sgbuf_buf = 0;
		    } else {
			ch = sgbuf;
			sgbuf = rch;
		    };
		} else if ((sgbuf == 0x2e9) && (ch == 0x2e5)) {
		    vch = ch;
		    ch = 0xd820; sgbuf_buf = 0; 
		} else if ((sgbuf == 0x2e5) && (ch == 0x2e9)) {
		    vch = ch;
		    ch = 0xd821; sgbuf_buf = 0;
		} else if ((sgbuf == 0x31f7) && 
			   ((ch == U_KHDK) || (ch == U_LHDK))) {
		    vch = ch;
		    ch = 0xd807; sgbuf_buf = 0;
		} else if ((sgbuf >= 0xff61) && (sgbuf <= 0xff9f) &&
		    (!dsbl_uni_hkna_concat) &&
		    (is_nkf_no_hk ||
		     (!hk_enbl && !(use_compat && is_ucs_ufam(conv_cap))))) {
		    dd = sgbuf; sgbuf = sOCD; sgbuf_buf = 0;
		    vch = ch;
		    if ((ch == U_CDAK) || (ch == U_CHDK)) {
			de = ch - 0xff40;
		    } else if ((ch >= U_KDAK) && (ch <= U_LHDK)) {
			de = ch - U_KDAK + 0x20;
		    } else de = 0xb0;	/* dummy		   */
		    if ((x = x0201conv((dd - 0xff40),de)) != 0) {
			sgbuf = ch; sgbuf_buf = 1;
		    };
		    return;
		} else if ((sgbuf >= 0xd800) && (sgbuf < 0xd812)) {
		    vch = 0x309a; ch = sgbuf; sgbuf = rch;
		} else {
		    ch = sgbuf;
		    sgbuf = rch;
		};
		if (is_kanaconv_x0201 && 
		    (((ch >= 0x3041) && (ch <= 0x309c)) ||
		     ((ch >= 0x30a1) && (ch <= 0x30ec)) ||
		     ((ch >= 0x3001) && (ch <= 0x3002)) ||
		     ((ch >= 0x300c) && (ch <= 0x300d)))) {
		    packedch = x0201rconv(ch);
		    if (packedch != 0) {
			skf_ucode s0,s1;
			s0 = ((packedch & (skf_ucode)0xff00UL) >> 8);
			s1 = (packedch & (skf_ucode)0xffU);
			if (s0 != 0) o_compat_conv(s0 + (skf_ucode)0xff00UL);
			o_compat_conv(s1 + (skf_ucode)0xff00UL);
			return;
		    } else;
		};
	    } else; 
	};
    } else {
	if (ch < 0) {
	    if (o_encode != 0) {
		o_c_encode(ch);
	    };
	    if (ch == sFLSH) {
		return;
	    } else if (ch == sOCD) return;
	} else {
	    sgbuf = ch; 
	    sgbuf_buf = 1;
#ifdef SKFDEBUG
	    if (is_vv_debug) {
		if (sgbuf_buf == 0) fprintf(stderr," -sgbuf_pushed");
	    };
#endif
	    return;
	};
    };
/* nkf -Z and various zenkaku-hankaku conversion hook */
    if (is_nkf_convert_hook) {
    /* FIXME: should test whole unicode region for --no-best-fit-chars */
	if (use_latin2null && is_no_bfc && is_nkf_compat && (
		((ch >= 0xffe0) && (ch <= 0xffef)) 
	     ||	((ch >= 0x80) && (ch <= 0xff)) 
	     ||	((ch >= 0x2014) && (ch <= 0x2016)) 
	     || (ch == 0x2212) || (ch == 0x301c)
	     || (ch == 0x203e) || (ch == 0x2225) || (ch == 0xff0d))) {
	     return;	/* discard */
	} else if (is_ascii_conv && (ch >= 0xff01) && (ch <= 0xff5e)) {
	    ch -= 0xfee0;	/* conver to pure ascii		  */
	} if (is_ascii_conv && (ch == 0x2212)) { /* minus hack	  */
	    ch = 0x2d;
	} else if (is_kanaconv_x0201 && 
		(((ch >= 0x3041) && (ch <= 0x309c)) ||
		 ((ch >= 0x30a1) && (ch <= 0x30fc)) ||
		 ((ch >= 0xd801) && (ch <= 0xd80f)))) {
	    cnvpk = x0201rconv(ch);
	    if (cnvpk != 0) {
		c2 = (skf_ucode)((cnvpk & 0xff00U) >> 8);
		ch = (skf_ucode)((cnvpk & 0x00ffU) + 0xff00UL);
		if (c2 != 0) post_oconv(c2 + 0xff00UL);
	    } else;
	} else;
    } else;
    if (ch < 0x080) {	/* is ascii and a part of ISO8859 */
#ifdef FOLD_SUPPORT
	if (ch >= 0) { charProp = o_ascii_entity[ch];
	} else;
#endif    
        if (ch < A_SP) {	/* 0x00 - 0x1f, < 0		  */
	    if (ch < 0) {
	    	SKF1FLSH();
		return;
	    };
	    c1 = ch; 
	    if (ch == A_CR) {
	      if (!detect_cr) {
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"-CR mixture-");
#endif
		set_detect_cr;
		if (!detect_lf) set_first_detect_cr;
#ifdef FOLD_SUPPORT
		if (!is_lineend_normalize || !fold_fclap) {
#else
		if (!is_lineend_normalize) {
#endif
		    set_use_detect_cr;
		    if (!use_detect_lf) set_use_first_detect_cr;
		} else;
	      } else;
#ifdef FOLD_SUPPORT
	      if (fold_fclap) {
		if (notrunc_le) {
		    fold_count = 0; 
#ifdef NEWSENTCLIP
	      	} else if (sentence_clip && is_lineend_cr) {
		    SKFCRLF(); fold_count = 0; c1 = -1;
#endif
		} else if (
		((!fold_flat && (fold_count == 0)) 
		  || (term_prevch(prev_ch) && (fold_count > (fold_clap>>1)))
		  || (tail_prevch(prev_ch) && (fold_count == 0))
		  || (fold_count > fold_clap))
		&& first_detect_cr) {
		    SKFCRLF(); fold_count = 0; c1 = -1;
		} else if (tail_prevch(prev_ch) && (fold_count != 0)
			&& first_detect_cr) {
		    SKFCRLF(); SKFCRLF(); fold_count = 0; c1 = -1;
		} else if (detect_lf && !first_detect_cr) {
			/* LF-CR case. may be ignored		  */
		    if (fold_count > fold_clap) {
			SKFCRLF(); fold_count = 0; c1 = -1;
		    } else c1 = -1;	/* eliminate		  */
		} else if (sgbuf_buf == 0) {	/* EOF or FLSH	  */
		    SKFCRLF(); c1 = -1;
		} else if (is_nkf_compat && (prev_ch > 0x2000)) {
		    c1 = -1;	/* eliminate			  */
		} else if (is_sentence_clip && (fold_count == 0)) {
		    c1 = -1;	/* eliminate			  */
		} else {
		    c1 = A_SP; fold_count++;
		}; 
	      } else fold_count = 0;
#endif	/* FOLD_SUPPORT */
	      if (!is_lineend_thru
	      	|| (notrunc_le && is_lineend_normalize)) {
		  if (!detect_lf || first_detect_cr) {
		      SKFCRLF();
#ifdef FOLD_SUPPORT
		  } else if (notrunc_le && is_lineend_normalize
		  	&& (prev_ch == A_LF)) {
		      ch = -1;		/* LF-CR: just discard		  */
		      		/* put dummy char to prev_ch 		  */
#endif
#if 0	/* do we really need these hack? */
		  } else if (is_nkf_compat && detect_lf 
		  	&& (prev_ch != A_LF)) {
		      SKFCRLF();
#endif
		  } else if (is_nkf_compat && detect_lf) {
		      SKFCRLF();
		  } else;
		  c1 = -1;
	      };
	    } else if (c1 == A_LF) {
	      if (!detect_lf) {
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"-LF mixture-");
#endif
		set_detect_lf;
#ifdef FOLD_SUPPORT
		if (!is_lineend_normalize || !fold_fclap) {
#else
		if (!is_lineend_normalize) {
#endif
		    set_use_detect_lf;
		} else;
	      } else;
#ifdef FOLD_SUPPORT
	      if (fold_fclap) {
		if (notrunc_le) {
		    fold_count = 0; 
#ifdef NEWSENTCLIP
	      	} else if (sentence_clip && is_lineend_lf) {
			SKFCRLF(); fold_count = 0; c1 = -1;
#endif
		} else if (((!fold_flat && (fold_count == 0)) 
		  || (term_prevch(prev_ch) && (fold_count > (fold_clap>>1)))
		  || (tail_prevch(prev_ch) && (fold_count == 0))
		  || (fold_count > fold_clap))
		&& !first_detect_cr) {
			SKFCRLF(); fold_count = 0; c1 = -1;
		} else if (tail_prevch(prev_ch) && (fold_count != 0)
		&& !first_detect_cr) {
		    SKFCRLF(); SKFCRLF(); fold_count = 0; c1 = -1;
		} else if (first_detect_cr) {
		    if (fold_count > fold_clap) {
			SKFCRLF(); fold_count = 0; c1 = -1;
		    } else c1 = -1;	/* eliminate		  */
		} else if (sgbuf_buf == 0) {	/* EOF or FLSH	  */
		    SKFCRLF(); c1 = -1;
		} else if (is_nkf_compat && (prev_ch > 0x2000)) {
		    c1 = -1;	/* eliminate			  */
		} else if (is_sentence_clip && (fold_count == 0)) {
		    c1 = -1;	/* eliminate			  */
		} else {
		    c1 = A_SP; fold_count++;
		};
	      } else fold_count = 0;
#endif		/* FOLD_SUPPORT */
	      if (!is_lineend_thru
	      	|| (notrunc_le && is_lineend_normalize)) {
		  if (!detect_cr) {
		      SKFCRLF();
#ifdef FOLD_SUPPORT
		  } else if (notrunc_le && is_lineend_normalize
		  	&& (prev_ch == A_CR)) {
		      ch = -1;	/* CR-LF: just discard			  */
#endif
		  } else if (!is_nkf_compat && !first_detect_cr) {
		      SKFCRLF();
#ifdef FOLD_SUPPORT
		  } else if (is_nkf_compat && detect_cr
		  	&& (prev_ch != A_CR)) {
		      SKFCRLF();
#endif
		  } else;
		  c1 = -1;
	      };
/* #ifdef FOLD_SUPPORT */
	    } else if (c1 == A_SUB) {
	    	fold_count = 0;
	    } else if (c1 == A_BS) {
	    	fold_count--;
	    } else if (c1 == A_FF) {
	    	fold_count = 0;
	    } else if (c1 == A_HT) {
	        if (is_sentence_clip) {
		    if (fold_count == 0) c1 = -1;
		    else c1 = A_SP;
		} else 
		    fold_count = (fold_count | (TAB_WIDTH - 1)) + 1;
/* #endif */
	    } else;
	    if (c1 >= 0) {
		o_ascii_conv(c1);
	    } else;
	} else {	/* 0x20 - 0x7f				   */
#ifdef FOLD_SUPPORT
	    if (fold_fclap) {
		if (or_fold_isalphanum(charProp)
			|| or_fold_isburasa(charProp)) {
		    if (fold_count > fold_fclap) {
			SKFCRLF(); fold_count = 0;
		    } else;
		} else if (fold_count > (fold_clap - fold_omgn)) {
		    if (or_fold_isburasa(charProp)) {
			if (fold_count > fold_fclap) {
			    SKFCRLF(); fold_count = 0;
			} else ;		/* burasage	   */
		    } else if (or_fold_isoidash(charProp) 
			    || (fold_count > fold_clap)) {
				    /* oidasi kinsoku		   */
			SKFCRLF(); fold_count = 0;
		    } else ;
		} else if (is_sentence_clip && (ch == A_SP)
			&& (fold_count == 0)) {
		    return;	/* just discard			   */
		} else ;
	    };
#endif
	    o_ascii_conv(ch);
	    fold_count++;
#ifdef FOLD_SUPPORT
	    if (is_sentence_clip) {
	    	if (or_fold_sdelim(charProp)) {
		    if (!is_intag && fold_count != 0) {
			SKFCRLF(); fold_count = 0;
		    };
		} else if (ch == A_BRA) {
		    is_intag = TRUE;
		} else if (ch == A_KET) {
		    is_intag = FALSE;
		    SKFCRLF(); fold_count = 0;
		};
	    } else if (fold_fclap > 0) {
		if ((fold_count > (fold_clap - fold_omgn))
			&& or_fold_adelim(charProp)) {
		    SKFCRLF(); fold_count = 0;
		} else;
	    };
#endif
	};
    } else if (ch <= 0x4dff) {	/* 0x80 - 0x4dff		   */
	if (ch >= 0x3000) {
/* nkf-compatible katakana-hiragana-convert */
	  if ((ch >= 0x3041) && (ch <= 0x3096)) {
		if (is_nkf_c_katakana) ch += 0x60L;
	  } else if ((ch >= 0x30a1) && (ch <= 0x30a6)) {
		if (is_nkf_c_hiragana) ch -= 0x60L;
	  } else;
#ifdef	FOLD_SUPPORT
	  if (fold_fclap > 0) {
	    if (fold_count > (fold_clap - 2)) { /* dbytes	  */
	    	kinsoku_stat = kinsoku_map0[ch - 0x3000];
		if (fold_count >= fold_fclap) {
			SKFCRLF(); fold_count = 0;
		} else if (is_weak_burasage(kinsoku_stat) ||
		   (is_strong_burasage(kinsoku_stat) && fold_strong)) {
				/* burasage kinsoku		  */
		    ;
		} else if (is_weak_oidasi(kinsoku_stat)
			|| (fold_count >= fold_clap)) {
				/* oidasi kinsoku		  */
		    SKFCRLF(); fold_count = 0;
		} else;
	    } else;
	  };
#endif
#ifdef UNI_DECOMPOSE
	    if (enbl_decomp) {
	    	if ((uukp = nkdc_lowptr[ch - UNI_LAT_OFF]) != 0) {
		    decompose_code((int)uukp);
		    return;
		};
	    };
#endif
	    o_cjkkana_conv(ch);	/* Bracketed char/kana		  */
	    fold_count += 2;
	} else if (ch < 0xa0) {
#ifdef	FOLD_SUPPORT
	    if (fold_fclap > 0) {
		if ((fold_count > fold_clap) &&
			((ch != A_SP) || (!is_noadelim))) {
		    SKFCRLF(); fold_count = 0;
		} else;
	    };
#endif
	    fold_count += 2;
	    out_undefined(ch,SKF_IOUTUNI);
	} else {
#ifdef UNI_DECOMPOSE
	    if (enbl_decomp) {
	    	if (((uukp = nkdc_lowptr[ch - UNI_LAT_OFF]) != 0) &&
	    (!decomp_apple || ((ch > 0x2adc) || (ch < 0x2000)))) {
		    decompose_code((int)uukp);
		    return;
		};
	    } else;
#endif
#ifdef	FOLD_SUPPORT
	    if (fold_fclap > 0) {
	      if (fold_count > (fold_clap - 1)) { /* single byte  */
		if (fold_count >= fold_fclap) {
			SKFCRLF(); fold_count = 1;
		} else if ((ch == 0x2019) || (ch == 0x201d)
			|| (ch == 0x2011) || (ch == 0x203c)
			|| (ch == 0x200b) || (ch == 0x200d)
			|| (ch == 0x201a) || (ch == 0x201e)) {
			;
		} else if (((ch >= 0x2018) && (ch <= 0x201f))
				|| (fold_count >= fold_clap)) {
		    SKFCRLF(); fold_count = 1;
		} else;
	      } else fold_count ++;
	    };
#endif
	    o_latin_conv(ch);
	    fold_count++;
#ifdef	FOLD_SUPPORT
	    if (is_sentence_clip && 
		((ch == 0xa1) || (ch == 0xbf) || 
		 (ch == 0x3002))) {
		if (fold_count != 0) {
		    SKFCRLF(); fold_count = 0;
		};
	    } else ;
#endif
	};
    } else if (ch <= 0x9fff) {	/* Han part			  */
#ifdef	FOLD_SUPPORT
	if (fold_fclap > 0) {
	    if (fold_count >= fold_clap) {
		SKFCRLF(); fold_count = 0;
	    } else;
	};
#endif
	o_cjk_conv(ch);
	fold_count += 2;
    } else if (ch <= 0xd7ff) {	/* hangul area			  */
#ifdef	FOLD_SUPPORT
	if (fold_fclap > 0) {
	    if (fold_count >= fold_clap) {
		SKFCRLF(); fold_count = 0;
	    } else;
	};
#endif
#ifdef UNI_DECOMPOSE
	if ((ch >= 0xa800) && enbl_decomp) {
	    decompose_hangul(ch);
	    return;
	} else;
#endif
	o_ozone_conv(ch);
	fold_count += 2;
    } else if (ch <= 0xf8ff) {	/* private use			  */
#ifdef	FOLD_SUPPORT
	if (fold_fclap > 0) {
	    if (fold_count >= fold_clap) {
		SKFCRLF(); fold_count = 0;
	    } else;
	};
#endif
	if ((ch >= 0xe000) && enable_cellconvert) {
	    if (ovlay_byte_defs[emot_prv_c_index].uniltbl != NULL) {
		c2 = (ovlay_byte_defs[emot_prv_c_index].uniltbl[ch - 0xe000]);
		if (c2 != 0) {
		    post_oconv(c2);
		    fold_count += 2;
		    return;
		} else;
	    } else;
	} else;
	if ((vch >= 0xd800) && (vch < 0xe000)) o_private_conv(ch,0);
	else o_private_conv(ch,vch);
	fold_count += 2;
    } else if (ch < 0x10000) {	/* compatibility plane		  */
#ifdef	FOLD_SUPPORT
	if (fold_fclap > 0) {	/* if folding is enabled	  */
	  if (is_nkf_convert_hook) { nkftrim = 1;
	  } else;
	  trim = ((ch >= 0xff60) && (ch < 0xffe0)) ? 1 : 0;
	  if (fold_count > (fold_clap - 2 + trim + nkftrim)) {
	    if (fold_count > (fold_fclap + trim)) {
		SKFCRLF(); fold_count = 0;
	    } else if ((ch == 0xff08) || (ch == 0xff3b)
		    || (ch == 0xff5b) || (ch == 0xff62)
	    	    || (fold_count >= fold_clap)) {
			    /* tail kinsoku			  */
		SKFCRLF(); fold_count = 0;
	    } else if ((ch == 0xff09) || (ch == 0xff0c) 
		|| (ch == 0xff0e) || (ch == 0xff3d)
		|| (ch == 0xff5d) || (ch == 0xff1b)
		|| (ch == 0xff9e) || (ch == 0xff9f)
		|| ((ch >= 0xff61) && (ch <= 0xff65))) {
		; 			/* head kinsoku		  */
	    } else;
	  } else ;
	};
#endif
#ifdef UNI_DECOMPOSE
	if (enbl_decomp) {
	    if ((uukp = nkdc_lowptr[ch - 0xf900 + UNI_UP_OFF]) != 0) {
	    	if (!decomp_apple || ((ch >= 0xfb1d) && (ch <= 0xfb4e))) {
		    decompose_code((int)uukp);
		    return;
		} else;
	    } else;
	} else;
#endif
	o_compat_conv(ch);
	fold_count += (2 - trim);
    } else if ((ch >= 0x0e0000) && (ch <= 0x0e007f)) {
			    /* input language tag		  */
	if (ch == 0x0e0001) { 
	    lang_tag_index = 0;
	    output_language_tag_trigger();
	} else if (ch == 0x0e007f) { /* cancel character	  */
				/* reset to default (ja-jp)	  */
	    ucs_tagstr[0] = 'j'; ucs_tagstr[1] = 'a';
	    ucs_tagstr[2] = '-'; ucs_tagstr[3] = 'j';
	    ucs_tagstr[4] = 'p'; 
	    ucs_tagstr[5] = '\0'; lang_tag_index = 0;
	    skf_output_lang = L_JA;
	    output_language_tag_trigger();
	} else if (ch >= 0x0e0041) { /* language spec character */
	    ucs_tagstr[lang_tag_index] = (char)(ch & 0x00007f);
	    if (lang_tag_index < 4) lang_tag_index++;
	    if (lang_tag_index == 2) output_language_tag_trigger();
	};
    } else if ((ch >= 0x0e0100) && (ch <= 0x0e01ff)) {
			/* Variation selector supprement	  */
		;	/* just discard				  */
    } else {
#ifdef UNI_DECOMPOSE
	if (enbl_decomp && !decomp_apple) {
	    if ((ch >= 0x1d100) && (ch < 0x1d800)) {
		if ((uukp = nkdc_lowptr[ch - 0x1d100 + UNI_COM_OFF]) != 0) {
		    decompose_code((int)uukp);
		    return;
		};
	    } else if ((ch >= 0x11090) && (ch < 0x11600)) {
	    	/* KAITHI and other codes */
		if ((uukp = nkdc_lowptr[ch - 0x11090 + UNI_KAITHI_OFF]) != 0) {
		    decompose_code((int)uukp);
		    return;
		};
	    } else if ((ch >= 0x1ee00) && (ch < 0x1ef00)) {
	    	/* arabic mathmatical addendums */
		if ((uukp = nkdc_lowptr[ch - 0x1ee00 + UNI_ARAM_OFF]) != 0) {
		    decompose_code((int)uukp);
		    return;
		};
	    } else if ((ch >= 0x1f100) && (ch < 0x1f400)) {
	    	/* Enclosed CJK addendums and emoticons */
		if ((uukp = nkdc_lowptr[ch - 0x1f100 + UNI_ECSP_OFF]) != 0) {
		    decompose_code((int)uukp);
		    return;
		};
	    } else if ((ch >= 0x2f800) && (ch < 0x2fa20)) {
			    /* CJK cpt.sup  */
		if ((uukp = nkdc_lowptr[ch - 0x2f800 + UNI_CJKC_OFF]) != 0) { 
		    decompose_code((int)uukp);
		    return;
		};
	    };
	};
#endif
#ifdef	FOLD_SUPPORT
	if (fold_fclap > 0) {
	    if (fold_count >= fold_clap) {
		SKFCRLF(); fold_count = 0;
	    } else;
	};
#endif
	o_ozone_conv(ch);
	fold_count += 2;
    };

#ifdef		FOLD_SUPPORT
    prev_ch = ch;
#endif
    if (unbuf_f) {
    	if (sgbuf_buf >= 1) {
	    sgbuf_buf = 0;
	    post_oconv(sgbuf);
	    sgbuf = sOCD;
	};
	SKFfflush((skfoFILE *)stdout);
    };
    return;
}

/* --------------------------------------------------------------- */
void post_oconv(ch)
skf_ucode ch;
{
#ifdef SKFDEBUG
    if (is_vv_debug) {
	if (ch == sEOF) fprintf(stderr," post_oconv:sEOF");
	else if (ch == sOCD) fprintf(stderr," post_oconv:sOCD");
	else if (ch == sKAN) fprintf(stderr," post_oconv:sKAN");
	else if (ch == sUNI) fprintf(stderr," post_oconv:sUNI");
	else if (ch == sFLSH) fprintf(stderr," post_oconv:sFLSH");
	else fprintf(stderr," post_oconv:0x%04x",ch);
#ifdef FOLD_SUPPORT
	if (fold_fclap > 0)
	    fprintf(stderr," %d:%d-%d",fold_clap,fold_fclap,fold_count);
#endif
    };
#endif
    if (ch < 0x080) {	/* is ascii and a part of ISO8859 */
    	if ((ch < 0) && (ch != sFLSH)) SKF1FLSH();
	else o_ascii_conv(ch);
    } else if (ch <= 0x4dff) {
	if (ch >= 0x3000) {
	    o_cjkkana_conv(ch);	/* Bracketed char/kana		  */
	} else if (ch < 0xa0) {
	    out_undefined(ch,SKF_IOUTUNI);
	} else {
	    o_latin_conv(ch);
	};
    } else if (ch <= 0x9fff) {	/* Han part			  */
	o_cjk_conv(ch);
    } else if (ch <= 0xd7ff) {	/* hangul area			  */
	o_ozone_conv(ch);
    } else if (ch <= 0xf8ff) {	/* private use			  */
	o_private_conv(ch,0);
    } else if (ch < 0x10000) {	/* compatibility plane		  */
	o_compat_conv(ch);
    } else if ((ch >= 0x0e0100) && (ch <= 0x0e01ff)) {
			/* Variation selector supprement	  */
		;	/* just discard				  */
    } else {
	o_ozone_conv(ch);
    };

    return;
}

/* --------------------------------------------------------------- */
void ox_ascii_conv(x)
skf_ucode x;
{
    o_ascii_conv(x);
}

void o_latin_conv(x)
skf_ucode x;
{
    if (is_jiscat(conv_cap)) {
	if (is_jis(conv_cap)) JIS_latin_oconv(x); 
	else EUC_latin_oconv(x);
    } else if (is_ucs_ufam(conv_cap)) {
	UNI_latin_oconv(x); 
    } else if (out_ocat) {
	if (is_msfam(conv_cap)) SJIS_latin_oconv(x);
	else if (out_bg(conv_cap)) BG_latin_oconv(x);
	else if (is_keis(conv_cap)) KEIS_latin_oconv(x); 
	else BRGT_latin_oconv(x);
    } else {
	EUC_latin_oconv(x);	/* transparent	   */
    };
}

void o_private_conv(x,rx)	
skf_ucode x;
skf_ucode rx;
{
    if (is_jiscat(conv_cap)) {
	if (is_jis(conv_cap)) JIS_private_oconv(x); 
	else EUC_private_oconv(x);
    } else if (is_ucs_ufam(conv_cap)) {
    	UNI_private_oconv(x,rx);
    } else if (out_ocat) {
	if (is_msfam(conv_cap)) SJIS_private_oconv(x);
	else if (out_bg(conv_cap)) BG_private_oconv(x);
	else if (is_keis(conv_cap)) KEIS_private_oconv(x); 
	else BRGT_private_oconv(x);
    } else {
	EUC_private_oconv(x);	/* transparent	   */
    };
}

void o_ozone_conv(x)	
skf_ucode x;
{
    if (is_jiscat(conv_cap)) {
	if (is_jis(conv_cap)) JIS_ozone_oconv(x); 
	else EUC_ozone_oconv(x);
    } else if (is_ucs_ufam(conv_cap)) {
	UNI_ozone_oconv(x);
    } else if (out_ocat) {
	if (is_msfam(conv_cap)) SJIS_ozone_oconv(x);
	else if (out_bg(conv_cap)) BG_ozone_oconv(x);
	else if (is_keis(conv_cap)) KEIS_ozone_oconv(x); 
	else BRGT_ozone_oconv(x);
    } else {
	EUC_ozone_oconv(x);	/* transparent	   */
    };
}

/* --- iso-2022 style process --------------------------------- */
/* --- JIS output routines ------------------------------------ */
/* 0 < c1 < 0x7f must be guarantteed by caller.			*/
void SKFJIS1OUT(c1)
skf_ucode	c1;
{
    r_SKFJIS1OUT(c1);
}

void SKFJIS1ASCOUT(c1)		/* ascii output			*/
skf_ucode	c1;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFJIS1ASCOUT: 0x%04x",c1);
#endif
    if (!(is_ascii_shift) && 
	(!is_kanji_shift && (g0_char != 'B') && enable_dbl_latin)) {
	set_ascii_shift;
	SKFputc(A_ESC); SKFputc('('); SKFputc('B');
	if (o_encode) SKFputc(mFLSH);
    };
    SKFputc(c1);
}

/* --- kana functionality ---------------------------------------- */
/* comes here with 0x21 to 0x7f or 0xa1 to 0xff			   */
void SKFJISK1OUT(c1)
skf_ucode c1;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFJISK1OUT: 0x%02x",c1);
#endif
    c1 &= 0x7f;			/* reset bit 7			   */
    if (eight_bit || si_enbl) {
#ifndef	SUPPRESS_FJ_CONVENSION
	if (is_kanji_shift) {
	    reset_kanji_shift;
	    SKFputc(A_ESC); SKFputc('('); SKFputc(g0_char);
	    if (o_encode) SKFputc(mFLSH);
	};
#endif
	if (eight_bit) {
	    SKFputc(c1 | 0x80);
	} else {
	/* skf does not hold locking shift status, for checking	   */
	/* this status decrease normal performance.		   */
	    SKFputc(A_SO); SKFputc(c1); SKFputc(A_SI); 
	};
    } else {			/* default is kana_call		   */
	if (!is_x0201_shift) {
	    set_x0201_shift; g0_mod = -2;
	    SKFputc(A_ESC); SKFputc('('); SKFputc('I');
	    if (o_encode) SKFputc(mFLSH);
	};
	SKFputc(c1);
    };
}

/* --- non-kana functionality ------------------------------------ */
void SKFJISG2OUT(c1)
skf_ucode c1;
{
    int c2,c3;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFJISG2OUT: 0x%04x",c1);
#endif
    c1 = stripchar(c1);

    c2 = (c1 >> 8);
    c3 = c1 & 0x7f;
    if (!is_g2cjk_shift) {
	set_g2cjk_shift;
	SKFputc(A_ESC); SKFputc(g2_mid);
	if (g2_quad) SKFputc(g2_midl);
	SKFputc(g2_char);
    }; 
    SKFputc(c2); SKFputc(c3);
}

/* --- x0208 functionality --------------------------------------- */
void SKFJISOUT(c1)
skf_ucode c1;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFJISOUT: 0x%04x",c1);
#endif
    r_SKFJISOUT(c1);
}

/* --- x0212 functionality --------------------------------------- */
void SKFJISG3OUT(c1)
skf_ucode c1;
{
    int c2,c3;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFJISG3OUT: 0x%04x",c1);
#endif
    c1 = stripchar(c1);

    c2 = ((c1 >> 8) & 0x7f);
    c3 = c1 & 0x7f;
    if ((!is_altcjk_shift && (g3_mid < 0x2d))
	|| (!is_i8859x_shift && (g3_mid >= 0x2d))) {
	if (!is_altcjk_shift) set_altcjk_shift;
	if (!is_i8859x_shift) set_i8859_x_shift;
	SKFputc(A_ESC); SKFputc(g3_mid);
	if (g3_quad) SKFputc(g3_midl);
	SKFputc(g3_char);
    }; 
    if (c2 != 0) {
	SKFputc(c2); SKFputc(c3);
    } else {
	if ((g3_mid >= 0x2d) || (g3_mid <= 0x2f)) c3 |= 0x80;
	SKFputc(c3);
    };
}

void SKFJISG4OUT(c1)
skf_ucode c1;
{
    int c2,c3;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFJISG4OUT: 0x%04x",c1);
#endif
    c1 = stripchar(c1);

    c2 = (c1 >> 8);
    c3 = c1 & 0x7f;
    if (!is_g4cjk_shift) {
	set_g4cjk_shift;
	SKFputc(A_ESC); SKFputc(g4_mid);
	if (g4_quad) SKFputc(g4_midl);
	SKFputc(g4_char);
    }; 
    SKFputc(c2); SKFputc(c3);
}

/* ---- iso-8859-1 functionality --------------------------------- */
void SKFJIS8859OUT(c1)
skf_ucode c1;
{
    int	 c3;			/* c1 must be 0xa1 <= c1 <= 0xff   */
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFJIS8859OUT: 0x%02x",c1);
#endif
    /*
     * Note: since table defined as iso8859 to g1, g1_char is correct.
     */
    c3 = c1 & 0x7f;
    if ((use_iso8859_1_left) && 
	((!is_g2_8859_shift) || (g2_extract_code_set() != 1))) {
	set_g2_8859_1_shift;
	SKFputc(A_ESC); SKFputc(0x2e); SKFputc(g1_char);
	if (o_encode) SKFputc(mFLSH);
    } else if (!(use_iso8859_1_left) && (!is_i8859_shift)) {
	set_i8859_1_shift;
	if (enbl_latin_annon) {
	    SKFputc(A_ESC); SKFputc(0x2d); SKFputc(g1_char);
	    if (o_encode) SKFputc(mFLSH);
	};
    };
    if (use_iso8859_1_left) {
	SKFputc(0x1b);SKFputc(0x4e); SKFputc(c3);
	if (o_encode) SKFputc(mFLSH);
    } else {
	SKFputc(c3 | 0x80);
    };
}

/* ---- iso-8859-2 to 16 ----------------------------------------- */
void SKFJIS8859XOUT(c1)
skf_ucode c1;
{
    int	 c3;			/* c1 must be 0xa1 <= c1 <= 0xff   */
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFJIS8859XOUT: 0x%02x",c1);
#endif
    c3 = c1 & 0x7f;
    if (!is_i8859x_shift) {
	set_i8859_x_shift;
	if (enbl_latin_annon) {
	    SKFputc(A_ESC); SKFputc(g1_mid); SKFputc(g1_char);
	    if (o_encode) SKFputc(mFLSH);
	};
    };
    SKFputc(c3 | 0x80);
}

/* --------------------------------------------------------------- */
void SKFSJISOUT(c3)
skf_ucode c3;
{ 
    int d1,d2,d3,d4; 
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFSJISOUT: 0x%04x",c3);
#endif

    d1 = ((c3 & 0x7f00) >> 8);
    d2 = ((c3) & 0x007f); 
    if ((c3 < 0x7921) || !(is_ms_932(conv_cap))) {
	SKFputc((int)(((d1 - 1) >> 1) + ((d1 <= 0x5e) ? 0x71 : 0xb1))); 
	SKFputc((int) (d2 + ((d1 & 1) ? ((d2 < 0x60) ? 0x1f : 0x20) : 0x7e)));
    } else if (c3 < 0x7c7f) {/* ms_compat && nec_selected_ibm_gaiji */
    	if (is_nocp932) {	/* force MS mode		   */
	    d3 = ((d1 - 1) >> 1) + ((d1 <= 0x5e) ? 0x71 : 0xb1); 
	    d4 = (d2 + ((d1 & 1) ? ((d2 < 0x60) ? 0x1f : 0x20) : 0x7e));
	} else if (c3 < 0x7c6f) {	/* Kanji area		   */
	    d4 = d2 + ((d1 - 0x79) * 94) + 0x1c - 0x21;
	    d3 = 0xfa;
	    if (d4 >= 376) {
		d4 -= 376; d3 += 2;
	    } else if (d4 >= 188) {
		d4 -= 188; d3++;
	    };
	    d4 = (d4 >= 0x3f) ? (d4 + 0x41) : (d4 + 0x40);
	} else {
	    d1 = uni_ibm_nec_excg[c3 - 0x7c6f];
	    d4 = ((d1) & 0x00ff); 
	    d3 = ((d1 & 0xff00) >> 8);
	};
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"(%02x%02x)",d3,d4);
#endif
	SKFputc((int)d3);
	SKFputc((int)d4);
    } else {	/* forced to output as it is.			   */
    	d1 = ((c3 & 0xff00) >> 8);
	SKFputc((int)(((d1 - 1) >> 1) + ((d1 <= 0x5e) ? 0x71 : 0xb1))); 
	SKFputc((int) (d2 + ((d1 & 1) ? ((d2 < 0x60) ? 0x1f : 0x20) : 0x7e)));
    };
}

void SKFSJISG2OUT(c3) 
skf_ucode c3;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFSJISG2OUT: 0x%04x",c3);
#endif
    out_undefined(c3,SKF_OUNDEF);
}

void SKFSJISG3OUT(c3)
skf_ucode c3;
{
    int  d1,d2;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFSJISG3OUT: 0x%04x",c3);
#endif
    if (is_ms_213c(conv_cap)) {
	d1 = ((c3 >> 8) & 0x7f) - 0x21 + 1;
	d2 = (c3 & 0x7f) - 0x21 + 1;
	SKFputc((int)((d1 <= 15) ? (((d1 + 0x1df)>>1) - (d1 >> 3) * 3) :
			 ((d1 + 0x19b) >> 1)));
	SKFputc((int) (d2 + ((d1 & 1) ? 
		((d2 < 0x40) ? 0x3f : 0x40) : 0x9e)));
    } else if (is_ms_cel(conv_cap)) {
    	d1 = ((c3 & 0x7f00) >> 8);
	d2 = (c3 & 0x7f);
	SKFputc((int)(((d1 - 0x21) >> 1) + 0xf0));
	SKFputc((int) (d2 + ((d1 & 1) ? ((d2 < 0x60) ? 0x1f : 0x20) : 0x7e)));
#ifdef SKFDEBUG
	if (is_vvv_debug) {
	    fprintf(stderr,"(%x-%x)",
	    ((int)(((d1 - 0x21) >> 1) + 0xf0)),
	    ((int) (d2 + ((d1 & 1) ? ((d2 < 0x60) ? 0x1f : 0x20) : 0x7e))));
	};
#endif
    } else 	/* should not be come here			   */
	out_undefined(c3,SKF_OUNDEF);
}

void SKFSJISG4OUT(c3)
skf_ucode c3;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFSJISG4OUT: 0x%04x",c3);
#endif
    out_undefined(c3,SKF_OUNDEF);
}

/* --------------------------------------------------------------- */
void SKFEUC1OUT(c3)
skf_ucode c3;
{
    r_SKFEUC1OUT(c3);
}

void SKFEUCG2OUT(c3)
skf_ucode c3;
{ 
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFEUCG2OUT: 0x%04x",c3);
#endif
    if (c3 >= 0x100) {
	if (is_euc7(conv_cap)) {
	    if (is_kanji_shift) { 
		SKFputc(A_SI); reset_kanji_shift; 
	    } else ;
	    SKFputc(A_ESC); SKFputc(A_2SS2);
	    SKFputc(((c3) & 0x7f00) >> 8); SKFputc((c3) & 0x7f);
	} else {
	    SKFputc(A_SS2);
	    if (is_euc_cns(conv_cap)) {
		SKFputc(0xa2);
	    };
	    SKFputc((((c3) & 0x7f00) >> 8) | 0x80); 
	    SKFputc(((c3) & 0xff) | 0x80);
	};
    } else {
	if (is_euc7(conv_cap)) {
	    if (is_kanji_shift) { 
		SKFputc(A_SI); reset_kanji_shift; 
	    } else ;
	    SKFputc(A_ESC); SKFputc(A_2SS2);
	    SKFputc(c3);
	} else {
	    SKFputc(A_SS2); SKFputc(c3 | 0x80);
	};
    };
}

void SKFEUCOUT(c3)
skf_ucode c3;
{
    r_SKFEUCOUT(c3);
}

/* TODO : following code for gbkr is broken. Will investigate further */
void SKFEUCG3OUT(c3)	
skf_ucode c3;
{ 
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFEUCG3OUT: 0x%04x",c3);
#endif
    if (is_euc7(conv_cap)) {
	if (is_kanji_shift) { 
	    SKFputc(A_SI); reset_kanji_shift; 
	} else ;
	SKFputc(A_ESC); SKFputc(A_2SS2);
	SKFputc(((c3) & 0x7f00) >> 8); SKFputc((c3) & 0x7f);
    } else if (is_euc_gbcn(conv_cap)) {
	SKFputc(A_SS2);
	SKFputc((((c3) & 0x7f00) >> 8) | 0x80); 
	SKFputc(((c3) & 0xff) | 0x80);
    } else {
	if (is_euc7(conv_cap)) {
	    if (is_kanji_shift) { 
		SKFputc(A_SI); reset_kanji_shift; 
	    } else ;
	    SKFputc(A_ESC); SKFputc(A_2SS3);
	    SKFputc(((c3) & 0x7f00) >> 8); SKFputc((c3) & 0x7f);
	} else {
	    SKFputc(A_SS3);
	    if (is_euc_cns(conv_cap)) {
		SKFputc(0xa2);
	    };
	    SKFputc((((c3) & 0x7f00) >> 8) | 0x80); 
	    SKFputc(((c3) & 0xff) | 0x80);
	};
    };
}

void SKFEUCG4OUT(c3)
skf_ucode c3;
{
		/* IS THIS REALLY BE TRUE? */
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFEUCG4OUT: 0x%04x",c3);
#endif
    if (is_euc_gbcn(conv_cap)) {
	SKFputc(A_ESC); SKFputc(g3_mid); SKFputc(g3_midl);
	SKFputc(g3_char);
	if (is_euc7(conv_cap)) {
	    SKFputc(A_SO);
	    SKFputc(((c3) & 0x7f00) >> 8); SKFputc((c3) & 0x7f);
	    SKFputc(A_SI);
	} else {
	    SKFputc((((c3) & 0x7f00) >> 8) | 0x80);
	    SKFputc(((c3) & 0x7f) | 0x80);
	};
	SKFputc(A_ESC); SKFputc(ag0_mid); SKFputc(ag0_midl);
	SKFputc(ag0_char);
    } else 
	out_undefined(c3,SKF_OUNDEF);
}

/* --- bg output ------------------------------------------------- */
/* special character never comes here. Same as EUC --------------- */
/* --------------------------------------------------------------- */
void SKFBG1OUT(c3) 
skf_ucode c3;
{
    int d2;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFBG1OUT: 0x%04x",c3);
#endif

    d2 = ((c3) & 0x007f); 
    if (is_big5fam(conv_cap)) {
	SKFputc(c3);
    } else if (is_hz(conv_cap) || is_hz8(conv_cap)) {
	if (is_hz_shift) {
	    SKFputc(HZ_ANN); SKFputc(HZ_END);
	};
	res_hz_shift;
	if (d2 == HZ_ANN) { SKFputc(HZ_ANN); };
	SKFputc(d2);
    } else if (is_zW(conv_cap)) {
	if (is_zw_shift && (d2 != A_LF) && (d2 != A_CR)) {
	    SKFputc(A_SP);
	} else {
	    SKFputc(ZW_ANN); SKFputc(ZW_ENT); /* dirty but needed  */
	    SKFputc(A_SP);
	    set_zw_shift;
	};
	if (((is_lineend_crlf || is_lineend_cr || is_lineend_thru)
		&& (d2 == A_CR)) ||
	    ((is_lineend_lf || (is_lineend_thru && (!detect_cr)))
		&& (d2 == A_LF))) {
	    SKFputc(ZW_LF); res_zw_shift;
	    if (d2 == A_CR) set_detect_cr;
	    if (d2 == A_LF) set_detect_lf;
	} else ;
	SKFputc(d2); 
    } else if (is_viqr(conv_cap) || is_vimn(conv_cap)) {
	viqr_convert(c3);
    } else if (is_johab(conv_cap) || is_gbthru(conv_cap) 
		|| is_uhc(conv_cap)) {
	SKFputc(c3); 
    } else {	/* output at least ascii for undefined code	   */
	SKFputc(c3 & 0x7f);
    };
}

/* --- 2byte ----------------------------------------------------- */
/* NOTE: if gb18030 a5 area is specified, the code must be packed  */
/* --------------------------------------------------------------- */
void SKFBGOUT(c3) 
skf_ucode c3;
{                       	/* convert into bg83-code set	   */
    int c2,c1;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFBGOUT: 0x%04x ",c3);
#endif
    c2 = (c3 & 0x7f00) >> 8;
    c1 = (c3 & 0xff) ;		/* c1 can be > 0x80 for BG	   */
    if (is_big5fam(conv_cap)) {
	if (is_gb18030(conv_cap) && (c3 > 0x8000)) { /* Area 5	   */
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"GB2K ");
#endif
	    SKFGB2KAOUT(gb18030_unpack(c3));
	} else if (is_big5p(conv_cap)) {
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"BIG5P ");
#endif

	    if (c3 < 0x100) {
		c1 = c3;
	    } else if (c3 < 0xa000) {
		c2 = ((c3 - 0x2000) & 0x7f00) >> 8;
		SKFputc((int) (c2 | 0x80));
	    } else {
		c2 = (c3 & 0x7f00) >> 8;
		SKFputc((int) (c2));
	    };
	    SKFputc((int) (c1));
	} else {
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"BIG5 ");
#endif
	    SKFputc((int) (c2 | 0x80));
	    SKFputc((int) (c1));
	    if (use_bg2cc && (c1 == 0x5c)) {  /* 0x5c: backslash   */
		SKFputc((int) (c1));
	    };
	};
    } else if (is_hz(conv_cap)) {
	if (!is_hz_shift) {
	    SKFputc(HZ_ANN); SKFputc(HZ_ENT);
	};
	set_hz_shift;
	SKFputc(c2); SKFputc(c1);
    } else if (is_zW(conv_cap)) {
	if (!is_zw_shift) {
	    SKFputc(ZW_ANN); SKFputc(ZW_ENT);
	};
	set_zw_shift;
	SKFputc(c2); SKFputc(c1); 
    } else if (is_johab(conv_cap) || is_gbk(conv_cap)) {
	SKFputc((int) (c2 + 0x80));
	SKFputc((int) (c1));
    } else if (is_uhc(conv_cap)) {
	if (c3 < 0x8000) c1 |= 0x80;
	SKFputc((int) (c2 + 0x80));
	SKFputc((int) (c1));
    } else if (is_hz8(conv_cap)) {
	if (!is_hz_shift) {
	    SKFputc(HZ_ANN); SKFputc(HZ_ENT);
	};
	set_hz_shift;
	SKFputc(c2 | 0x80); SKFputc(c1 | 0x80);
    } else {
	SKFputc((int) ('.'));
    }; 
}

/* --------------------------------------------------------------- */
void SKFGB2KAOUT(ch)
skf_ucode ch;
{
    int p1,p2,p3,p4;

    p1 = (ch / 12600);
    p2 = (ch - (p1 * 12600)) / 1260;
    p3 = (ch - (p1 * 12600) - (p2 * 1260)) / 10;
    p4 = (ch - (p1 * 12600) - (p2 * 1260) - (p3 * 10));
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr,
	" SKFGB2KAOUT: 0x%04x(%02x %02x %02x %02x)",ch,
	(p1+0x81),(p2+0x30),(p3+0x81),(p4+0x30));
#endif
    SKFputc(p1 + 0x81); SKFputc(p2 + 0x30);
    SKFputc(p3 + 0x81); SKFputc(p4 + 0x30);
    return;
}

/* --- keis output ----------------------------------------------- */
/* special character never comes here. Same as EUC --------------- */
/* SKFKEIS1OUT: ebcdic part					   */
/* SKFKEISOUT: multibyte part					   */
/* --------------------------------------------------------------- */
void SKFKEISG3OUT(c3) 
skf_ucode c3;
{
    out_undefined(c3,SKF_OUNDEF);
}

void SKFKEISG4OUT(c3)
skf_ucode c3;
{
	out_undefined(c3,SKF_OUNDEF);
}
		
void SKFKEISK1OUT(c3)
skf_ucode c3;
{
	out_undefined(c3,SKF_OUNDEF);
}
		
/* --------------------------------------------------------------- */

void SKFKEIS1OUT(c1) 
skf_ucode c1;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFKEIS1OUT: 0x%04x",c1);
#endif
    if (c1 < 0) return;
    if (is_keis_shift) {
	if (is_keis_keis(conv_cap)) {
	    SKFputc(KEIS_SMM); SKFputc(KEIS_SI); reset_kanji_shift;
	} else if (is_keis_jef(conv_cap)) {
	    SKFputc(KEIS_JEF_SI); reset_kanji_shift;
	} else {
	    SKFputc(KEIS_IBM_SI); reset_kanji_shift;
	};
    };
    SKFputc(c1);
}

/* --------------------------------------------------------------- */
void SKFKEISG2OUT(c3) 
skf_ucode c3;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFKEISG2OUT: 0x%04x",c3);
#endif
    if (is_keis_shift) {
	if (is_keis_keis(conv_cap)) {
	    SKFputc(KEIS_SMM); SKFputc(KEIS_SI); reset_kanji_shift;
	} else if (is_keis_jef(conv_cap)) {
	    SKFputc(KEIS_JEF_SI); reset_kanji_shift;
	} else {
	    SKFputc(KEIS_IBM_SI); reset_kanji_shift;
	};
    };
    if ((c3 <= 0xdf) && is_keis_keis(conv_cap)) {
    	SKFputc((int)(KEISOUT3[c3 - 0xa1]));
    } else;	/* just discard all else */
}
		

/* --------------------------------------------------------------- */
/* --- 2byte --- */
void SKFKEISOUT(c3) 
skf_ucode c3;
{                       	/* convert into keis83-code set	   */
    int c2,c1;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFKEISOUT: 0x%04x",c3);
#endif
    c2 = (c3 & 0xff00) >> 8;
    c1 = (c3 & 0xff) ;
    if (!is_keis_shift) {
	if (is_keis_keis(conv_cap)) {
	    SKFputc(KEIS_SMM); SKFputc(KEIS_SO); set_keis_shift;
	} else if (is_keis_jef(conv_cap)) {
	    SKFputc(KEIS_JEF_SO); set_keis_shift;
	} else {
	    SKFputc(KEIS_IBM_SO); set_keis_shift;
	};
    };
    if (is_keis_keis(conv_cap)) {
	SKFputc(c2 | 0x80); SKFputc(c1 | 0x80); 
    } else {	/* jef and IBM dbcs				  */
	SKFputc(c2); SKFputc(c1); 
    };
}

/* --------------------------------------------------------------- */
void SKFKEISEOUT(c3) 
skf_ucode c3;
{                      	/* convert into extend-code set	   */
    int c2,c1;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFKEISEOUT: 0x%04x",c3);
#endif
    c2 = (c3 & 0x7f00) >> 8;
    c1 = (c3 & 0x7f) ;
    if (!is_keis_shift) {
	if (is_keis_keis(conv_cap)) {
	    SKFputc(KEIS_SMM); SKFputc(KEIS_SO); set_keis_shift;
	} else if (is_keis_jef(conv_cap)) {
	    SKFputc(KEIS_JEF_SO); set_keis_shift;
	} else {
	    SKFputc(KEIS_IBM_SO); set_keis_shift;
	};
    };
    SKFputc(c2); SKFputc(c1 | 0x80); 
}

/* --------------------------------------------------------------- */
void SKFSTROUT(x)
const char *x;
{
	if (is_jis(conv_cap)) SKFJISSTROUT(x); 
	else if (is_msfam(conv_cap)) SKFSJISSTROUT(x);
	else if (is_euc(conv_cap)) SKFEUCSTROUT(x);
	else if (out_bg(conv_cap)) SKFBGSTROUT(x);
	else if (is_ucs_ufam(conv_cap)) SKFUNISTROUT(x);
	else if (is_ucs_brgt(conv_cap)) SKFBRGTSTROUT(x);
	else if (is_keis(conv_cap)) SKFKEISSTROUT(x); 
}

/* --------------------------------------------------------------- */
void SKF1FLSH()
{
#ifdef SKFDEBUG
	if (is_vvv_debug) fprintf(stderr," FCEFLSH");
#endif
	if (is_jis(conv_cap)) { SKFJIS1FLSH(); 
	} else if (is_msfam(conv_cap)) { SKFSJIS1FLSH();
	} else if (is_euc(conv_cap)) { SKFEUC1FLSH();
	} else if (out_bg(conv_cap)) { SKFBG1FLSH();
	} else if (is_ucs_ufam(conv_cap)) { SKFUNI1FLSH();
	} else if (is_ucs_brgt(conv_cap)) { SKFBRGT1FLSH();
	} else if (is_keis(conv_cap)) { SKFKEIS1FLSH(); 
	} else;
}

#if defined(ROT_SUPPORT) && defined(NEW_ROT_CODE)
/* --------------------------------------------------------------- */
static void  SKFROTPUT(cz)
int cz;
{
    if (cz <= A_DEL) {
    	if (is_jis(conv_cap)) {
	    SKFJIS1OUT(cz);
	} else if (is_euc(conv_cap)) {
	    SKFEUC1OUT(cz);
	} else {	/* regard as msfam			   */
	    SKFSJIS1OUT(cz);
	};
    } else {
    	if (is_jis(conv_cap)) {
	    SKFJISOUT(cz);
	} else if (is_euc(conv_cap)) {
	    SKFEUCOUT(cz);
	} else {	/* regard as msfam			   */
	    SKFSJISOUT(cz);
	};
    };
}
/* --------------------------------------------------------------- */
void SKFROTTHRU(c1,c2)
int c2,c1;
{
    int c3,c4;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," SKFROTTHRU: 0x%02x%02x",c1,c2);
#endif

    if (c1 == 0) {	/* rot13				   */
    	c3 = skf_rot13conv_d(c2);
	SKFROTPUT(c3);
    } else {
    	c3 = skf_rot47conv_d(c2);
    	c4 = skf_rot47conv_d(c1);
	SKFROTPUT((c4 << 8) | c3);
    };
}
#endif

/* --------------------------------------------------------------- */
void SKF_rotoconv(ch,mod)
int ch;
int mod;
{
    int c1,c2;

    if (mod) {
    	c1 = (ch >> 8) & 0x7f;
	c2 = (ch & 0x7f);
    	SKFROTTHRU(c1,c2);
    } else {
    	oconv(ch);
    };
}

/* --------------------------------------------------------------- */
/* --- line end handler ------------------------------------------ */
void SKFCRLF()
{
#ifdef SKFDEBUG
    if (is_vv_debug) {
	fprintf(stderr," SKFCRLF:");
	if (is_lineend_thru) fprintf(stderr,"T");
	if (is_lineend_crlf) fprintf(stderr,"M");
	if (is_lineend_cr) fprintf(stderr,"C");
	if (is_lineend_lf) fprintf(stderr,"L");
    };
#endif
    if (is_lineend_thru) {
	if (use_first_detect_cr && use_detect_cr) {
	    ox_ascii_conv(A_CR);
	    if (use_detect_lf) ox_ascii_conv(A_LF);
	} else {
	    if (use_detect_lf) ox_ascii_conv(A_LF);
	    if (use_detect_cr || !use_detect_lf) ox_ascii_conv(A_CR);
	};
    } else {
	if (is_lineend_crlf || is_lineend_cr) ox_ascii_conv(A_CR);
	if (is_lineend_crlf || is_lineend_lf) ox_ascii_conv(A_LF);
    };
#ifdef FOLD_SUPPORT
    fold_count = 0;
#endif
}

/* --------------------------------------------------------------- */
void output_language_tag_trigger()	/* set read output tags	   */
{
    skf_output_lang = skf_set_strong_lang(
		((ucs_tagstr[0]) << 8) + (ucs_tagstr[1]));
    show_lang_tag();
}

#ifdef UNI_DECOMPOSE
/* --------------------------------------------------------------- */
/* Unicode decompose						   */
/* use sgbuf_buf, sgbuf implicitly.				   */ 
/* --------------------------------------------------------------- */
void decompose_code(uu)
int	uu;
{
    int i;
    int str0;

#ifdef SKFDEBUG
    if (is_vvv_debug) {
    	fprintf(stderr,"UU:%x ",uu);
	(void) fflush(stderr);
    };
#endif
    decompose_bufp = 0;
    decompose_code_dec(uu);
    decompose_recursion_depth = 0;
    str0 = get_combine_strength(sgbuf);
    for (i=0;i<decompose_bufp;i++) {
    	if (is_combining(sgbuf) && (sgbuf_buf >= 1)) {
	    if (is_combining(decompose_buf[i])) {
	    	if (str0 < get_combine_strength(decompose_buf[i])) {
		    post_oconv(decompose_buf[i]);
		    sgbuf_buf = 0;
		    sgbuf = sFLSH;
		}; /* check combining order	*/
	    };
	};
	post_oconv(decompose_buf[i]);
    };
}

void decompose_code_dec(uu)
int uu;
{
    int i;
    skf_ucode uukk;
    unsigned short uujj;
    int	offset_p = 0;

#ifdef SKFDEBUG
    if (is_vvv_debug) {
    	fprintf(stderr,"#decm: %x ",uu);
	(void) fflush(stderr);
    };
#endif
    uu--; 	/* Note: uu has +1 bias.	*/
    if (uu >= nkdc_lowsize) {	/* nkdc table overflow check */
    	out_undefined(0,SKF_ENC_ERR);
    	return;
    } else;
 /* limit = 32. In Unicode 5.0, longest word has 18 char decomposition */
    for (i=0;i<32 && ((uukk = nkdc_lowmap[uu]) != 0);i++, uu++) {
    	if ((uukk >= 0xa0) && (uukk < 0x3400)) {
		offset_p = UNI_LAT_OFF;
    	} else if ((uukk >= 0xf900) && (uukk < 0x10000)) {
		offset_p = UNI_UP_OFF - 0xf900;
    	} else if ((uukk >= 0x1d100) && (uukk < 0x1d800)) {
		offset_p = UNI_COM_OFF - 0x1d100;
    	} else if ((uukk >= 0x11090) && (uukk < 0x11600)) {
		offset_p = UNI_KAITHI_OFF - 0x11090;
    	} else if ((uukk >= 0x1ee00) && (uukk < 0x1ef00)) {
		offset_p = UNI_ARAM_OFF - 0x1ee00;
    	} else if ((uukk >= 0x1f100) && (uukk < 0x1f400)) {
		offset_p = UNI_ECSP_OFF - 0x1f100;
    	} else if ((uukk >= 0x2f801) && (uukk < 0x2fa1d)) {
		offset_p = UNI_CJKC_OFF - 0x2f800;
	} else ;
	
	if ((offset_p != 0) 
		&& ((uujj = nkdc_lowptr[uukk+offset_p]) != 0)) {
	    if ((decompose_recursion_depth++) > 32) {
	    	/* Something in table is REALLY WRONG	*/
		skferr(SKF_DECOMPERR,(long)uu,0);
	    } else decompose_code_dec((int)uujj);
	    decompose_recursion_depth--;
	} else {
	    decompose_buf[decompose_bufp++] = uukk;
	};
    };
#ifdef FOLD_SUPPORT
    fold_count = 0;
#endif
}

/* --------------------------------------------------------------- */
void decompose_hangul(uu)
skf_ucode uu;
{
    int index = uu - UNIHAN_SBASE;
    int lvalue,vvalue,tvalue;

    lvalue = UNIHAN_LBASE + index / (UNIHAN_VCOUNT * UNIHAN_TCOUNT);
    vvalue = UNIHAN_VBASE +
    	(index % (UNIHAN_VCOUNT * UNIHAN_TCOUNT)) / UNIHAN_TCOUNT;
    tvalue = UNIHAN_TBASE + index % UNIHAN_TCOUNT;
    post_oconv(lvalue + UNIHAN_LBASE);
    post_oconv(vvalue + UNIHAN_VBASE);
    if (tvalue != 0) post_oconv(tvalue + UNIHAN_TBASE);
}

#ifdef UNI_ENCOMPOSE
/* --------------------------------------------------------------- */
/* encompose: compose set of unicode codes into KD combined char.  */
/*  Note: full-width kana encompose is handled in oconv()	   */
/*  input: chary -  array of skf_ucode chars to compose		   */
/*          target [sptr ... ssize)				   */
/* --------------------------------------------------------------- */
int	skf_encompose(chary,sptr,ssize,oank)
skf_ucode	*chary;
int	sptr;
int	ssize;
int	oank;
{
    int	res = 0;
    int	i,j,k;
    int	hb,lb;
    int	ank,iank;	/* anchor and indirect anchor pointer	   */
    int sank,isank;
    int	bank,bpank;		/* back anchor */
    int	lank,lpos;
    int ffpos,npos;
    skf_ucode prech,ch;

    prech = chary[sptr];
    if (prech < 0) return(-1);

    hb = (prech & 0x7fffff00UL) >> 8;
    lb = (prech & 0xff);
    j=0; localank = 0;

    if ((hb >= ENCOMPOSE_MASK_LEN) || (enck_mask == NULL)) {
    	return(-1);
    } else if (oank == 0) {
	if ((encompose_tree == NULL) || 
	    ((ank = enck_mask[hb]) == 0)) {
	    	/* there's no leaf in page */
	    return(-1);
	} else if ((iank = encompose_start[((ank - 1) << 8) + lb]) == -1) {
		/* there's no leaf in THIS character */
	    return(-1);
	} else;

	for (j=sptr;j < ssize; j++) {
	    ffpos = encompose_tree[iank].f_pos;
	    res = 0;
	    for (i=0;i < encompose_tree[iank].f_ch;i++) {
	    	npos = encompose_list[f_pos+i];
		if (encompose_tree[npos].c_identity == ch) {
		    res = i;
		    break;
		} else;
	    };
	    if (res == 0) { /* not found */
		if (encompose_tree[iank].c_compose != 0) {
		    post_oconv(encompose_tree[iank].c_compose);

	    	if ((lank == 0) 
			&& (encompose_tree[iank].c_compose == 0)) {
			/* not found any seq.	   */
		    return(-1);
	    	if (encompose_tree[iank].c_compose == 0) {
	    	if (encompose_tree[iank].c_compose == 0) {
		} else {
		};
		bank = isank;
		bpank = 0;
		while (encompose_list[bank].c_compose < 0) {
		    bpank = bank;
		    bank = encompose_list[bank].rev_p;
		    j--;
		};
		/* restart with residuals of the array to compose */
		return(skf_encompose(chary,j,ssize,oank));
	    } else {	/* found */
		if (encompose_list[iank].f_pos == 0) {
		    /* we reach tail! */
		    post_oconv(encompose_list[iank].c_compose);
		    return(0);
		} else {  /* may be in the middle. test next j	   */
		    if (encompose_tree[ank].c_compose != 0) {
		    	/* remember last seq. tail candidate	   */
		    	localank = ank;
			lank = ank;
			lpos = j;
		    };
		    ank = encompose_list[i].leaf_id;
		};
	    };
	};
	/* we are now in middle of consistent sequence.		   */
    };

    return(res);
}
/* --------------------------------------------------------------- */
#endif /* UNI_ENCOMPOSE */
#endif	/* UNI_DECOMPOSE */

/* --------------------------------------------------------------- */
#ifdef SKFDEBUG
void debugcharout(c)
int c;
{
    if ((c>= 0x20) && (c <= 0x7e)) {
    	fprintf(stderr,"(%c)",(int)c);
    } else fprintf(stderr,"(.)");
}
#endif
/* --------------------------------------------------------------- */
