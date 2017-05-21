/* *******************************************************************
** Copyright (c) 1999-2015 Seiji Kaneko. All rights reserved.
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
    esc_process.c	escape sequence handler
    $Id: esc_process.c,v 1.12 2017/01/05 15:05:48 seiji Exp seiji $
*/

#include <stdio.h>
#include <sys/types.h>
#include "skf.h"
#include "skf_fileio.h"
#include "oconv.h"
#include "convert.h"
#include "in_code_table.h"
#include "stdlib.h"

#define  has_valid_ctable(x) (((x)->defschar != 0) && \
		((((x)->char_width <= 2) && ((x)->unitbl != NULL)) || \
		 ((x)->uniltbl != NULL) || \
		 (load_external_table(x) == 0)))

static unsigned short	in_depth = FALSE;  /* recursion stopper?   */

#define	ascii_index	3
#define	x0208_index	0

static int		codeset_stack_depth = 0;
static int 		in_codeset_sr_save;
static unsigned long	encode_cap_sr_save;
static unsigned long	ucod_fl_sr_save;
static unsigned long	codeset_fl_sr_save;

static int defschar_search P_((struct iso_byte_defs *,int));
static void set_defschar_tuple P_((struct iso_byte_defs *,int,int));

/* --------------------------------------------------------------- */
/* esc_process: The most messy part of skf. Treat all sequences    */
/*		start with ESC					   */
/* In this routine. skf detects Han and Chinese, but does nothing. */
/*  if someone will write some remedy to handle these ...	   */
/* --------------------------------------------------------------- */
/*@-globstate@*//*@-usereleased@*/
int esc_process(f)	/* escape parsing process.		   */
    skfFILE *f;
{
    int c1,c3 = 0;      /* does not pass to/from main routine      */
    int c2;
    int idx;		/* table search index			   */
#if defined(SKF_OLDESCCODE)
    struct iso_byte_defs *g_table_mod;
#endif

    if ((c1 = vGETC(f)) == sEOF) {  /* case: esc-eof		   */
		;		/* pass esc may screw up terminal  */
	in_undefined('p',SKF_UNEXPEOF);
/* --------------------------------------------------------------- */
    } else if (c1 == '$') {	/* ISO-2022. ESC-$(0x24)	   */
		/* iso-2022 multibyte character sequences	   */
	if ((c1 = vGETC (f)) == sEOF) {	/* case: esc-$-eof         */
	    if (!stripinvis) {
		;
	    };
	    in_undefined('m',SKF_UNEXPEOF);
/* --------------------------------------------------------------- */
	} else if ((c1 >= A_AT) && (c1 < 0x60)) {
	/* ------------------------------------------------------- */
	/* old multi-byte iso-2022 94 character sequence to G0	   */
	/* ------------------------------------------------------- */
		/* this sequence set char_set to G0		   */
		/* actually, only @,A,B,C,D is allowed, but not	   */
		/* explicitly checked here.			   */
#ifdef SKFDEBUG
	    if (is_vv_debug)
			fprintf(stderr," 2022-3-mB: $,%2x - ",c1);
#endif
	    if (is_nkf_jfbroken) {
		    c1 = 'B';	/* JIS X 0208			   */
	    } else;
	    if (c1 == 'B') {
	    	if (!is_skf_aribmd) {
		    idx = x0208_index;
		    set_defschar_tuple(iso_3_dblbyte_defs,idx,0x28);
		} else {
		    idx = arib_x208_index;
		    set_defschar_tuple(miscmul_byte_defs,idx,0x28);
		};
	    } else {
		idx = defschar_search(iso_3_dblbyte_defs,c1);
		if (idx >= 0) {
		    set_defschar_tuple(iso_3_dblbyte_defs,idx,0x28);
		} else {
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		    if (is_jiscat(conv_cap)) {
			ox_ascii_conv(A_ESC); ox_ascii_conv('$');
			ox_ascii_conv(c1); 
		    };
#endif
		    g0_table_mod = &(iso_unibyte_defs[ascii_index]);
#ifndef	SUPPRESS_FJ_CONVENSION
		    if (!in_left_locking_shift) {
			g0table2low();
		    };
#endif
#ifdef SKFDEBUG
		    if (is_vv_debug) fprintf(stderr,
			    " return to ascii due to undef set: $,%2x",c1);
#endif
		};
	    };
	    res_all_shift;
	} else if ((c1 >= '0') && (c1 <= 0x3f)) { /* ARIB DRCS B24 */
/* --------------------------------------------------------------- */
/* ARIB B24/ISO-2022-MS shorten private sequence to G0		   */
/* --------------------------------------------------------------- */
#ifdef SKFDEBUG
	    if (is_vv_debug)
		fprintf(stderr," 2022-3-priv-mB: $,%2x - ",c1);
#endif
	    idx = defschar_search(priv_dblbyte_defs,c1);
	    if (idx >= 0) {
	    	set_defschar_tuple(priv_dblbyte_defs,idx,0x28);
	    } else {
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		if (is_jiscat(conv_cap)) {
		    ox_ascii_conv(A_ESC); ox_ascii_conv('$');
		    ox_ascii_conv(c1); 
		};
#endif
		g0_table_mod = &(iso_unibyte_defs[ascii_index]);
#ifndef	SUPPRESS_FJ_CONVENSION
		if (!in_left_locking_shift) {
		    g0table2low();
		};
#endif
#ifdef SKFDEBUG
	    	if (is_vv_debug) fprintf(stderr,
			" return to ascii due to undef set: $,%2x",c1);
#endif
	    };
/* --------------------------------------------------------------- */
	} else if ((c1 >= '!') && (c1 <= '/')) { /* ISO-2022(1986) */
		/* 4 octet multibyte calling sequences.		   */
	    c3 = c1;			/* pass for later use	   */
	    if ((c1 = vGETC(f)) == sEOF) { /* case: esc-!-eof	   */
		res_all_shift;
		in_undefined('M',SKF_UNEXPEOF);
/* --------------------------------------------------------------- */
/* multi-byte iso-2022 94 character sequence to G0,G1,G2,G3	   */
/* --------------------------------------------------------------- */
/* 	case: ESC $ 2/8,2/9,2/10,2/11 F				   */
/* --------------------------------------------------------------- */
	    } else if ((c1 >= A_AT) && (c3 >= 0x28) && (c3 <= 0x2b)) {
#if 1
#ifdef SKFDEBUG
		if (is_vv_debug)
		    fprintf(stderr," 2022-4-mB: $,%02x,%2x - ",c3,c1);
#endif
		idx = defschar_search(iso_4_dblbyte_defs,c1);
		if (idx >= 0) {
		    set_defschar_tuple(iso_4_dblbyte_defs,idx,c3);
		} else {
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		    if (is_jiscat(conv_cap)) {
			ox_ascii_conv(A_ESC); ox_ascii_conv('$');
			ox_ascii_conv(c1); 
		    };
#endif
		    g0_table_mod = &(iso_unibyte_defs[ascii_index]);
#ifndef	SUPPRESS_FJ_CONVENSION
		    if (!in_left_locking_shift) {
			g0table2low();
		    };
#endif
#ifdef SKFDEBUG
		    if (is_vv_debug) fprintf(stderr,
			    " return to ascii due to undef set: $,%2x",c1);
#endif
		};
#else	/* ********************** */
		idx = 0;
		if (is_nkf_jfbroken) {
		    c1 = 'B';	/* JIS X 0208			   */
		} else;
		idx = defschar_search(iso_4_dblbyte_defs,c1);
		if (idx >= 0) {
#ifdef SKFDEBUG
		    if (is_vv_debug)
			    fprintf(stderr," 2022-4-mB: $,");
#endif
		    set_defschar_tuple(iso_4_dblbyte_defs,idx,c3);
		} else {	/* UNKNOWN: pass as it is	   */
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		    if (is_jiscat(conv_cap)) {
			ox_ascii_conv(A_ESC); ox_ascii_conv('$');
			ox_ascii_conv(c3); ox_ascii_conv(c1); 
		    };
#endif
		    if (c3 == 0x28) {	/* G0 only process	   */
			g0_table_mod = &(iso_unibyte_defs[ascii_index]);
#ifndef	SUPPRESS_FJ_CONVENSION
			g0table2low();
#endif
#ifdef SKFDEBUG
			if (is_vv_debug) fprintf(stderr,
		    " return to ascii due to undef set: $,(,%2x",c1);
#endif
		    };
		};
#endif
		res_all_shift;
	    } else if ((c1 >= A_0) && (c3 >= 0x28) && (c3 <= 0x2b)) {
/* --------------------------------------------------------------- */
/* ARIB B24/ISO-2022-MS private sequence to G0,G1,G2,G3		   */
/* --------------------------------------------------------------- */
/* 	case: ESC $ 2/8,2/9,2/10,2/11 F (0x30 <= F < 0x40)	   */
/* --------------------------------------------------------------- */
#ifdef SKFDEBUG
		if (is_vv_debug)
		    fprintf(stderr," 2022-4-priv-mB: $,%2x,%2x - ",c3,c1);
#endif
		idx = defschar_search(priv_dblbyte_defs,c1);
		if (idx >= 0) {
		    set_defschar_tuple(priv_dblbyte_defs,idx,c3);
		} else {	/* UNKNOWN: pass as it is	   */
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		    if (is_jiscat(conv_cap)) {
			ox_ascii_conv(A_ESC); ox_ascii_conv('$');
			ox_ascii_conv(c3); ox_ascii_conv(c1); 
		    };
#endif
		    if (c3 == 0x28) {	/* G0 only process	   */
			g0_table_mod = &(iso_unibyte_defs[ascii_index]);
#ifndef	SUPPRESS_FJ_CONVENSION
			g0table2low();
#endif
#ifdef SKFDEBUG
			if (is_vv_debug) fprintf(stderr,
		    " return to ascii due to undef set: $,(,%2x",c1);
#endif
		    };
		    res_all_shift;
		};
	    } else if ((c1 == ' ') && (c3 >= 0x28) && (c3 <= 0x2b)) {
/* --------------------------------------------------------------- */
/* ARIB B24 drcs-multibyte sequence to G0,G1,G2,G3		   */
/* --------------------------------------------------------------- */
/* 	case: ESC $ 2/8,2/9,2/10,2/11 ' ' F			   */
/* --------------------------------------------------------------- */
#ifdef SKFDEBUG
		if (is_vv_debug)
			    fprintf(stderr," drcs-mB: $,%2x,' ',",c3);
#endif
		if ((c1 = vGETC(f)) == sEOF) { 
		    res_all_shift;
		    in_undefined('A',SKF_UNEXPEOF);
		} else {
		    idx = -1;
		    if ((c1 >= A_AT) && (c1 < 0x7f)) {
			idx = defschar_search(priv_drcs_dblbyte_defs,c1);
		    } else;
		    if (idx >= 0) {
#ifdef SKFDEBUG
			if (is_vv_debug) fprintf(stderr," @ - ");
#endif
			set_defschar_tuple(priv_drcs_dblbyte_defs,
				idx,c3);
		    } else {
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
			if (is_jiscat(conv_cap)) {
			    ox_ascii_conv(A_ESC); ox_ascii_conv('$');
			    ox_ascii_conv(c3); ox_ascii_conv(' '); 
			    ox_ascii_conv(c1); 
			};
#endif
			g0_table_mod = &(iso_unibyte_defs[ascii_index]);
#ifndef	SUPPRESS_FJ_CONVENSION
			if (!in_left_locking_shift) {
			    g0table2low();
			};
#endif
#ifdef SKFDEBUG
			if (is_vv_debug) fprintf(stderr,
				" return to ascii due to undef set: $,%2x",c1);
#endif
			res_all_shift;
		    };
		};
	    } else if ((c3 == '(') && (c1 >= '@')) {
	/* ------------------------------------------------------- */
	/* defined, but unsupported sequences			   */
	/* ------------------------------------------------------- */
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		ox_ascii_conv(A_ESC); ox_ascii_conv('$');
		ox_ascii_conv('('); ox_ascii_conv(c1); 
#endif
		res_all_shift;
	    } else if (!stripinvis) {   /* unsupported code set	   */
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		if (is_jiscat(conv_cap)) {
			ox_ascii_conv(A_ESC); ox_ascii_conv('$');
			ox_ascii_conv(c3); ox_ascii_conv(c1);
		};
#endif
		res_all_shift;
		c1 = seq_sweep(f,TRUE);
	    } else if ((c1 >= 0x30) && (c1 < 0x40)) { /* private   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,
		    " 94m4_private seqs(%x)..",c1);
#endif
		res_all_shift;
	    } else {	/* stripinvis				   */
		c1 = seq_sweep(f,FALSE); res_all_shift;
	    };  
	} else if (!stripinvis) { /* some unknown seq. 		   */
		c1 = seq_sweep(f,FALSE);
	} else {	/* stripinvis				   */
	    c1 = seq_sweep(f,FALSE);
	};      		/* end of process with ESC-$       */
/* --------------------------------------------------------------- */
/* single byte 94 iso-2022 character sequence to G0,G1,G2,G3  	   */
/* --------------------------------------------------------------- */
/* 	case: ESC 2/8,2/9,2/10,2/11 F				   */
/* --------------------------------------------------------------- */
/* Note: G1 is assumed to be pre-defined to 8bit right plane.	   */
/* --------------------------------------------------------------- */
    } else if ((c1 >= 0x28) && (c1 <= 0x2b)) {
	c2 = c1;
	if ((c1 = vGETC(f)) == sEOF) {	/* case: esc-(-eof         */
	    in_undefined('4',SKF_UNEXPEOF);
	    res_all_shift;
	} else if ((c1 >= A_AT) && (c1 < A_DEL)) { /* unibyte seq. */
	    if (is_nkf_jfbroken) {
		c1 = 'B';	/* force setting to ascii	   */
	    } else;
#if 1
#ifdef SKFDEBUG
	    if (is_vv_debug)
		fprintf(stderr," 2022-94-sB: $,%2x,%2x - ",c2,c1);
#endif
	    if (c1 == 'J') {
	        if (!is_skf_aribmd) {
		    idx = x0201_index;
		    set_defschar_tuple(iso_unibyte_defs,idx,c2);
		} else {
		    idx = arib_x201_index;
		    set_defschar_tuple(cp_byte_defs,idx,c2);
		};
	    } else if (c1 == 'B') {
	        idx = ascii_index;
		set_defschar_tuple(iso_unibyte_defs,idx,c2);
	    } else {
		idx = defschar_search(iso_unibyte_defs,c1);
		if (idx >= 0) {
		    set_defschar_tuple(iso_unibyte_defs,idx,c2);
		} else {	/* UNKNOWN: pass as it is	   */
    #ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		    if (is_jiscat(conv_cap)) {
			ox_ascii_conv(A_ESC);
			ox_ascii_conv(c2); ox_ascii_conv(c1); 
		    };
    #endif
		    if (c3 == 0x28) {	/* G0 only process	   */
			g0_table_mod = &(iso_unibyte_defs[ascii_index]);
    #ifndef	SUPPRESS_FJ_CONVENSION
			g0table2low();
    #endif
    #ifdef SKFDEBUG
			if (is_vv_debug) fprintf(stderr,
		    " return to ascii due to undef set: $,(,%2x",c1);
    #endif
		    };
		    res_all_shift;
		};
	    };
#else
	    idx = 0;
	    while ((iso_unibyte_defs[idx].defschar != 0x00) &&
		   (iso_unibyte_defs[idx].defschar != c1)) {
		   idx++;
	    };
	    if ((iso_unibyte_defs[idx].defschar == c1) &&
	        has_valid_ctable(&(iso_unibyte_defs[idx]))) {
	      if (c2 == 0x28) {		/* to G0		   */
		g0_table_mod = &(iso_unibyte_defs[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
		if (!in_left_locking_shift) {
		    g0table2low();
		};
#endif
		res_all_shift;
		if ((g0_table_mod->lang) != L_NU) {
		    if (!(skf_is_strong_lang(skf_input_lang))) {
			skf_input_lang = g0_table_mod->lang;
			if (output_lang == 0) {
			    skf_output_lang = skf_input_lang;
			    show_lang_tag();
			}; 
		    }; 
		};
#ifdef SKFDEBUG
		if (is_vv_debug)
			fprintf(stderr," 2022-94-G0: (,%2x - %s",c1,
				g0_table_mod->desc);
#endif
	    } else if (c2 == 0x29) {	/* to G1		   */
		g1_table_mod = &(iso_unibyte_defs[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
		if (!up_block) {
		    if (in_ls1) { g1table2low();
		    } else if (!in_right_locking_shift) { g1table2up();
		    };
#ifdef SKFDEBUG
		if (is_vv_debug) 
			fprintf(stderr," 2022-94-G1: ),%2x - %s",c1,
				g1_table_mod->desc);
#endif
		};
#endif
		res_all_shift;
	    } else if (c2 == 0x2a) {	/* to G2		   */
		g2_table_mod = &(iso_unibyte_defs[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
		if (in_ls2) { g2table2low();
		} else if (in_rs2) { g2table2up();
		};
#endif
#ifdef SKFDEBUG
		if (is_vv_debug)
			fprintf(stderr," 2022-94-G2: *,%2x - %s",c1,
				g2_table_mod->desc);
#endif
	    } else {			/* to G3		   */
		g3_table_mod = &(iso_unibyte_defs[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
		if (in_ls3) { g3table2low();
		} else if (in_rs3) { g3table2up();
		};
#endif
#ifdef SKFDEBUG
		if (is_vv_debug) 
			fprintf(stderr," 2022-94-G3: +,%2x - %s",c1,
				g3_table_mod->desc);
#endif
	    };
	  } else {
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
	    if (is_jiscat(conv_cap)) {
		ox_ascii_conv(A_ESC); ox_ascii_conv(c2);
		ox_ascii_conv(c3); ox_ascii_conv(c1); 
	    };
#endif
	    if (c3 == 0x28) {	/* G0 only process	   */
		g0_table_mod = &(iso_unibyte_defs[ascii_index]);
		g0table2low();
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,
	    " return to ascii due to undef set: $,(,%2x",c1);
#endif
	    };
	    res_all_shift;
	  };
#endif
	} else if (c1 >= A_0) {
/* --------------------------------------------------------------- */
/* single-byte iso-2022 private area sequence to G0,G1,G2,G3	   */
/* --------------------------------------------------------------- */
/* 	case: ESC 2/8,2/9,2/10,2/11 F (0x30 <= F < 0x40)	   */
/* --------------------------------------------------------------- */
    	/* iso-2022 private area: currently only to G0 is allowed */
#ifdef SKFDEBUG
	    if (is_vv_debug)
		    fprintf(stderr," priv-sB: $,%2x,%2x - ",c2,c1);
#endif
	    idx = defschar_search(priv_byte_defs,c1);
	    if (idx >= 0) {
#ifdef SKFDEBUG
		if (is_vv_debug)
		    fprintf(stderr,"%s",priv_byte_defs[idx].desc);
#endif
		set_defschar_tuple(priv_byte_defs,idx,c2);
	    } else {	/* UNKNOWN: pass as it is	   */
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		if (is_jiscat(conv_cap)) {
		    ox_ascii_conv(A_ESC); ox_ascii_conv('$');
		    ox_ascii_conv(c3); ox_ascii_conv(c1); 
		};
#endif
		if (c3 == 0x28) {	/* G0 only process	   */
		    g0_table_mod = &(iso_unibyte_defs[ascii_index]);
#ifndef	SUPPRESS_FJ_CONVENSION
		    g0table2low();
#endif
#ifdef SKFDEBUG
		    if (is_vv_debug) fprintf(stderr,
		" return to ascii due to undef set: $,(,%2x",c1);
#endif
		};
		res_all_shift;
	    };
	} else if (c1 == 0x21) {
/* --------------------------------------------------------------- */
/* single-byte iso-2022 94 char with 2nd im byte to G0,G1,G2,G3	   */
/* --------------------------------------------------------------- */
/* 	case: ESC 2/8,2/9,2/10,2/11 ! F (0x30 <= F < 0x40)	   */
/* --------------------------------------------------------------- */
	    c3 = c1;			/* 3rd character	    */
	    if ((c1 = vGETC(f)) == sEOF) {
		res_all_shift; in_undefined('e',SKF_UNEXPEOF);
		return(sEOF);
	    };
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,
		    " 2022-94-im-G: %2x,!,%2x - ",c2,c1);
#endif
	    idx = defschar_search(iso_im2byte_defs,c1);
	    if (idx >= 0) {
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"%s",
		    	iso_im2byte_defs[idx].desc);
#endif
		set_defschar_tuple(iso_im2byte_defs,idx,c2);
	    } else if ((c1 >= 0x30) && (c1 < 0x40)) { /* private   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,
		    " 94ims_private(%x)..",c1);
#endif
		res_all_shift;
	    } else {
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		if (is_jiscat(conv_cap)) {
		    ox_ascii_conv(A_ESC); ox_ascii_conv(c2);
		    ox_ascii_conv(c3); ox_ascii_conv(c1); 
		} else;
#endif
		res_all_shift;
	    };
	} else if (c1 == 0x20) {
/* --------------------------------------------------------------- */
/* ARIB B24 DRCS single byte sequence to G0,G1,G2,G3		   */
/* --------------------------------------------------------------- */
/* 	case: ESC 2/8,2/9,2/10,2/11 ' ' F (0x40 <= F < 0x50)	   */
/* --------------------------------------------------------------- */
	    if ((c1 = vGETC(f)) == sEOF) { /* case: esc-*-' '-EOF  */
		res_all_shift;
		in_undefined(c1,SKF_UNEXPEOF);
	    } else if ((c1 == 0x70) && is_arib) { /* ARIB MACRO	   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,
			" arib-drcs-mac: %2x,0x20,%2x - ",c2,c1);
#endif
#if defined(SKF_OLDESCCODE)
		idx = defschar_search(priv_drcs_byte_defs,c1);
#endif
		set_defschar_tuple(priv_drcs_byte_defs,
			arib_drcs_macro_index,c2);
		res_all_shift;
	    } else if ((c1 >= A_AT) && (c1 < 0x7f) 
	    		&& (c1 != 0x70) && is_arib) {
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,
			" arib-drcs-1: %2x,0x20,%2x - ",c2,c1);
#endif
		idx = defschar_search(priv_drcs_byte_defs,c1);
		if (idx >= 0) {
		    set_defschar_tuple(priv_drcs_byte_defs,idx,c2);
		} else {	/* UNKNOWN: pass as it is	   */
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		    if (is_jiscat(conv_cap)) {
			ox_ascii_conv(A_ESC); ox_ascii_conv(c2);
			ox_ascii_conv(' '); ox_ascii_conv(c1); 
		    };
#endif
		    if (c3 == 0x28) {	/* G0 only process	   */
			g0_table_mod = &(iso_unibyte_defs[ascii_index]);
			g0table2low();
#ifdef SKFDEBUG
			if (is_vv_debug) fprintf(stderr,
		    " return to ascii due to undef set: $,(,%2x",c1);
#endif
		    };
		    res_all_shift;
		};
	    } else {
		in_undefined(c1,SKF_ARIBERR);
#ifdef SKFDEBUG
		if (is_vv_debug)
		    fprintf(stderr," unk-3-privA: $,%02x,%2x,%02x - ",
		    	c2,' ',c1);
#endif
		res_all_shift;
	    };
	} else {	/* completely screwed up		   */
	    in_undefined(c1,SKF_UNSUPP);
#ifdef SKFDEBUG
	    if (is_vv_debug)
		fprintf(stderr," unk-3-privB: $,%02x,%2x - ",c3,c1);
#endif
	    res_all_shift;
	};
    } else if ((c1 == 0x2d) || (c1 == 0x2e) || (c1 == 0x2f)) {
/* --------------------------------------------------------------- */
/* single byte 96 iso-2022 character sequence to G1,G2,G3  	   */
/* --------------------------------------------------------------- */
/* 	case: ESC 2/13,14,15 F					   */
/* --------------------------------------------------------------- */
	c2 = c1;
#ifdef SKFDEBUG
	if (is_v_debug) fprintf(stderr,"in iso-8859 seq");
#endif
	if ((c1 = vGETC(f)) == sEOF) {	/* case: esc-2/13-eof      */
	    in_undefined('6',SKF_UNEXPEOF);
	    res_all_shift;
	} else if ((c1 >= A_AT) && (c1 < A_DEL)) { /* iso8859 seq. */
#if 1
#ifdef SKFDEBUG
	    if (is_vv_debug)
		fprintf(stderr," 2022-96-sB: $,%2x,%2x - ",c2,c1);
#endif
	    idx = defschar_search(iso_iso8859_defs,c1);
	    if (idx >= 0) {
		set_defschar_tuple(iso_iso8859_defs,idx,c2);
	    } else {	/* UNKNOWN: pass as it is	   */
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		if (is_jiscat(conv_cap)) {
		    ox_ascii_conv(A_ESC); 
		    ox_ascii_conv(c2); ox_ascii_conv(c1); 
		};
#endif
		res_all_shift;
	    };
#else
	    idx = 0;
	    while ((iso_iso8859_defs[idx].defschar != 0x00) &&
		   (iso_iso8859_defs[idx].defschar != c1)) {
		   idx++;
	    };
	    if ((iso_iso8859_defs[idx].defschar == c1) &&
	        has_valid_ctable(&(iso_iso8859_defs[idx]))) {
		if (c2 == 0x2d) {
		    g1_table_mod = &(iso_iso8859_defs[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
		    if (!up_block) {
			if (in_ls1) { g1table2low();
			} else if (!in_right_locking_shift) { 
			    g1table2up();
			};
		    };
#endif
		    res_all_shift;
#ifdef SKFDEBUG
		    if (is_vv_debug) fprintf(stderr,
			" 2022-96-G1: -,%2x - %s",c1,g1_table_mod->desc);
#endif
		} else if (c2 == 0x2e) {
		    g2_table_mod = &(iso_iso8859_defs[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
		    if (in_ls2) { g2table2low();
		    } else if (in_rs2) { g2table2up();
		    };
#endif
#ifdef SKFDEBUG
		    if (is_vv_debug) fprintf(stderr,
			" 2022-96-G2: .,%2x - %s",c1,g2_table_mod->desc);
#endif
		} else if (c2 == 0x2f) {
		    g3_table_mod = &(iso_iso8859_defs[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
		    if (in_ls3) { g3table2low();
		    } else if (in_rs3) { g3table2up();
		    };
#endif
#ifdef SKFDEBUG
		    if (is_vv_debug) fprintf(stderr,
			" 2022-96-G3: /,%2x - %s",c1,g3_table_mod->desc);
#endif
		};
	    } else if (!stripinvis) {	/* UNKNOWN: pass as it is  */
#ifndef	SUPPRESS_UNRECOGNIZED_SEQ
		if (is_jiscat(conv_cap)) {
		    ox_ascii_conv(A_ESC); ox_ascii_conv(c2);
		    ox_ascii_conv(c1); 
		};
#endif
		res_all_shift;
	    } else;
#endif
	} else if ((c1 >= A_0) && (c1 < A_AT)) { /* private   */
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,
		    " 96s_private (%x)..",c1);
#endif
		res_all_shift;
	} else {
	    res_all_shift;
	};
/* --------------------------------------------------------------- */
/* Code extension: JIS X-0221 use this sequence			   */
/* --------------------------------------------------------------- */
    } else if ((is_isocode) && (c1 == '%') && (coding_system_sense)) {
				/* code extension method select	   */
	if ((c1 = vGETC(f)) == sEOF) {	/* case: esc-2/5-eof	   */
	    in_undefined('x',SKF_UNEXPEOF);
	    res_all_shift;
	} else if (c1 == '@') { /* standard return. do nothing	   */
#ifdef SKFDEBUG
	    if (is_vv_debug)
		fprintf(stderr," codesw: %c,%2x - ",'%',c1);
#endif
	    if (codeset_stack_depth > 0) { /*@-globstate@*/
		in_codeset = in_codeset_sr_save;
		encode_cap = encode_cap_sr_save;
		ucod_flavor = ucod_fl_sr_save;
		codeset_flavor = codeset_fl_sr_save;
		clear_after_mime();
		return(sOCD);
	    } else {
		res_all_shift;
	    };
	} else if (c1 == 0x47) { /* to UTF-8 with return	   */
#ifdef SKFDEBUG
	    if (is_vv_debug)
		fprintf(stderr," codesw: %c,%2x - ",'%',c1);
#endif
	    if (codeset_stack_depth == 0) {
		in_codeset_sr_save = in_codeset;
		encode_cap_sr_save = encode_cap;
		ucod_fl_sr_save = ucod_flavor;
		codeset_fl_sr_save = codeset_flavor;
		codeset_stack_depth = 1;
		in_codeset = codeset_utf8;
		clear_after_mime();
		return(sOCD);
	    } else {
		res_all_shift;
	    };
	} else if (c1 == '?') { /* X 0221(1994). extensive test	   */
		/* Since we could not treat any composite code in  */
		/* *Code converter*. I regard whole level as same  */
#ifdef SKFDEBUG
	    if (is_vv_debug)
		fprintf(stderr," codesw: %c,%2x - ",'%',c1);
#endif
	    if (((c3 = vGETC (f)) == 0)) { /* padding  */
	    /*@-infloopsuncon@*/
		while (skip_x221_2_null() == 0) { c3 = vGETC(f); };
	    /*@+infloopsuncon@*/
	    };
	    if (c1 == sEOF) {	/* case: esc-%-?-eof		   */
		in_undefined('u',SKF_UNEXPEOF); /* Do nothing	   */
	    } else if (c3 == '@') {	/* 2Octet Level-1 ISO10646 */
		in_codeset = codeset_utf16be; /* point of no-return   */
		clear_after_mime();
		return(sOCD);
	    } else if (c3 == 'A') {	/* 4Octet Level-1 ISO10646 */
		in_codeset = codeset_utf32be; /* point of no-return   */
		res_all_shift; 
	    } else if (c3 == 'C') {	/* 2Octet Level-2 ISO10646 */
		in_codeset = codeset_utf16be; /* point of no-return   */
		clear_after_mime();
		return(sOCD);
	    } else if (c3 == 'D') {	/* 4Octet Level-2 ISO10646 */
		in_codeset = codeset_utf32be; /* point of no-return   */
		res_all_shift; 
	    } else if (c3 == 'E') {	/* 2Octet Level-3 ISO10646 */
		in_codeset = codeset_utf16be; /* point of no-return   */
		clear_after_mime();
		return(sOCD);
	    } else if (c3 == 'F') {	/* 4Octet Level-3 ISO10646 */
		in_codeset = codeset_utf32be; /* point of no-return   */
		res_all_shift; 
	    } else if ((c3 == 'G') || (c3 == 'H') || (c3 == 'I')) {
		in_codeset = codeset_utf8; /* UTF-8 without return */
		clear_after_mime();
		return(sOCD);
	    } else if ((c3 == 'J') || (c3 == 'K') || (c3 == 'L')) {
		in_codeset = codeset_utf16be;  /* UTF-16 without return */
		clear_after_mime();
		return(sOCD);
	    };		/* 					   */
	    in_codeset = codeset_utf16be;
	    return(u_in(f));
	} else {		/* unknown extension method.	   */
		res_all_shift;		/* now, what can we do?	   */
	};
#ifdef SKFDEBUG
	if (is_vv_debug)
		fprintf(stderr," 2022-extcode: %2x,%2x,%2x",'%',c1,c3);
#endif
/* --------------------------------------------------------------- */
/* Miscellaneous ISO-2022 sequences				   */
/* --------------------------------------------------------------- */
    } else if (c1 == 0x21) {    /* control character call sequence */
		/* some of them are already defined.		   */
	if (stripinvis) {
	    c1 = seq_sweep(f,FALSE);	/* bite it from stream	   */
	} else  ;
    } else if (c1 == 0x22) {    /* control character call sequence */
	if (stripinvis) {
	    c1 = seq_sweep(f,FALSE);	/* bite it from stream	   */
	} else  ;
    } else if (c1 == '&') {     /* version announce sequence	   */
	if (c1 == sEOF) {		/* case: esc-&-eof         */
	    in_undefined('a',SKF_UNEXPEOF);
	    res_all_shift;	/* just discard.		   */
	} else if (c1 >= 'A') { /* X 0208(1990) or later. 	   */
		/* we already assume 1990 revision, so ignore it.  */
	    ; 			/* forget unknown one ;^)	   */
	};
#ifdef SKFDEBUG
	if (is_vv_debug)
		fprintf(stderr," 2022-v.anon: %2x,%2x",'&',c1);
#endif
    } else if (c1 == A_SP) {	/* ISO-2022 announcer		   */
		/* ignore every specifications and assignments :^) */
	if (stripinvis) {
	    c1 = seq_sweep(f,FALSE);	/* bite it from stream	   */
	} else { ;
/* Maybe we should pass through these stuff, but these may screw   */
/* up many kanji-aware applications.				   */
	};
#ifdef	OLD_NEC_COMPAT
/* --------------------------------------------------------------- */
    } else if (old_nec_compat && (c1 == 'H')) {	
				/* ANK call for OLD NEC computer   */
	res_all_shift;
	nec_g0_kanjiset();
    } else if (old_nec_compat && (c1 == 'K')) {	
				/* Kanji call for OLD NEC computer */
	res_all_shift;
	nec_g0_kanaset();
#endif
/* --------------------------------------------------------------- */
/* Single shift + Locking shift					   */
/* --------------------------------------------------------------- */
    } else if ((is_isocode) && (c1 == 'N')) {
				/* SS2 for 7bit code set	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ,%2x: esc_p_ss2",c1);
#endif
	res_single_shift; 
	if (is_euc_cns(i_codeset[in_codeset].encode)) {
	    if ((c1 = vGETC(f)) < 0) {	/* valid codeset	   */
		if ((c1 >= 0xa1) && (c1 <= 0xa0 + cns11643_defined_code)) {
		    set_ss2;
		    g2_table_mod = 
			&(iso_4_dblbyte_defs[cns11643_1_index + c1 - 0xa1]);
		};
	    } else {	/* inconsistent: discard all else	   */
		in_undefined('c',SKF_UNEXPEOF);
	    };	
	} else {
	    set_ss2;
	};
    } else if ((is_isocode) && (c1 == 'O')) {
				/* SS3 for 7bit code set	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ,%2x: esc_p_ss3",c1);
#endif
	res_single_shift; set_ss3;
    } else if (c1 == 'd') {	/* exit from iso-2022.		   */
	res_all_shift;		/* reset pending status		   */
    	;			/*  ... but have nothing to do	   */
    } else if ((is_isocode) && (c1 == 'n')) {
				/* LS2 for 7bit code set	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ,%2x: esc_p_ls2",c1);
#endif
	set_ls2;
	g2table2low();
    } else if ((is_isocode) && (c1 == 'o')) {
				/* LS3 for 7bit code set	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ,%2x: esc_p_ls3",c1);
#endif
	set_ls3;
	g3table2low();
    } else if ((is_isocode) && (c1 == 0x7c)) {
				    /* LS3R for 8bit code set	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ,%2x: esc_p_ls3r",c1);
#endif
	set_rs3;
	g3table2up();
    } else if ((is_isocode) && (c1 == 0x7d)) {
				    /* LS2R for 8bit code set	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ,%2x: esc_p_ls2r",c1);
#endif
	set_rs2;
	g2table2up();
    } else if ((is_isocode) && (c1 == 0x7e)) {
				    /* LS1R for 8bit code set	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," ,%2x: esc_p_ls1r",c1);
#endif
	set_rs1;
	g1table2up();
/* --------------------------------------------------------------- */
    } else if ((c1 >= 0x30) && (c1 <= 0x3f)) {	/* private use	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," private3 seqs(%x)..",c1);
#endif
	return(0);
/* --------------------------------------------------------------- */
    } else if (c1 >= ASCII_END) {
	oconv(sFLSH);
	ox_ascii_conv(A_ESC);  /* output as it is		   */
	return(c1);
/* --------------------------------------------------------------- */
    } else if (!stripinvis) {	/* suppress terminal control code  */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," terminal seqs(%x)..",c1);
#endif
	if (c1 == '[') {     /* ANSI terminal control code	   */
	    oconv(sFLSH);
	    ox_ascii_conv(A_ESC);  /* output as it is		   */
	    ox_ascii_conv(c1);
	    c1 = seq_sweep(f,TRUE);
	} else if (c1 == 'O') { /* Cursor key sequence for vt100   */
	    oconv(sFLSH);
	    ox_ascii_conv(A_ESC);  /* output as it is		   */
	    ox_ascii_conv(c1);
	} else if (c1 == A_ESC) { /* seems to be broken seqs.	   */
	    ox_ascii_conv(A_ESC);  /* output as it is		   */
	    if (!in_depth) {
		in_depth = TRUE; c1 = esc_process(f);
		in_depth = FALSE;
	    } else {
		ox_ascii_conv(c1); c1 = seq_sweep(f,TRUE);
	    };
	} else {            	/* skf doesn't know the sequence   */
	    oconv(sFLSH);
	    ox_ascii_conv(A_ESC);   /* output as it is		   */
	    ox_ascii_conv(c1); c1 = seq_sweep(f,TRUE);
	};
	return(1);
    } else {			/* stripinvis mode		   */
	c1 = seq_sweep(f,FALSE); return(1);
    };
    if (c1 == sEOF) { return(sEOF);
    } else	return (0);	/* caller detects if c1 is EOF	   */
}

/* --------------------------------------------------------------- */
/* according to ISO-2022, all escape sequnces end with some code   */
/*			is belonging between 0x40 and 0x7e.	   */
/*	here we use this specification, and discards or just	   */
/*	pass unknowns.						   */
/* --------------------------------------------------------------- */
int	seq_sweep(f,out)
skfFILE	*f;
int	out;		/* out: output character if TRUE	   */
{
   	register int c1;

#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," sswp");
#endif
	while ((c1 = vGETC(f)) != sEOF) {
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"%c%02x",((out) ? '+':'-'),c1);
#endif
	    if (c1 >= ASCII_END) { /* this is not term seqs.	   */
	        unGETC(c1,f);
	    	break;
	    } else if (c1 >= '@') {
	    /* in ISO-2022 terminate ch. area  */
	    /* --- note: actual termination char is > 0x30, but	   */
	    /* 0x30-0x3f is reserved for private purpose, and not  */
	    /* likely to be contained in standard network message. */
	    	if (out) ox_ascii_conv(c1);
		break;
	    } else if (c1 < A_SP) {	/* or in area never happen */
	    	/* character drop or input is corrupted		   */
	    	if (c1 == A_ESC) {
		    if (!in_depth) {	/* rewind again only once  */
		        in_depth = TRUE; c1 = esc_process(f);
		        in_depth = FALSE;
		        break;		/*  ... and terminate	   */
		    } else {	/* second time ;-( trash them.	   */
		    	continue;
		    };
		} else if (out) { oconv(c1);
		} else;
		break;			/* stop sweeping	   */
	    } else {
	    	if (out) oconv(c1); /* pass through.	   */
	    };
	};
#ifdef SKFDEBUG
	if (is_vv_debug) {
	    fprintf(stderr,"!"); (void)fflush(stderr);
	} else;
#endif
	return(c1);
}

/* --------------------------------------------------------------- */
/* shift_cond_recovery: shift condition recovery		   */
/* --------------------------------------------------------------- */
void shift_cond_recovery()
{
    res_single_shift;
    if (!in_left_locking_shift) {
	g0table2low();
    } else if (in_ls1) {
	g1table2low();
    } else if (in_ls2) {
	g2table2low();
    } else if (in_ls3) {
	g3table2low();
    };
    if ((!in_right_locking_shift) || (in_rs1)) {
	g1table2up();
    } else if (in_rs2) {
	g2table2up();
    } else if (in_rs3) {
	g3table2up();
    };
}

/* --------------------------------------------------------------- */
/* c1 process: dummy for skf-1.97				   */
/* --------------------------------------------------------------- */
int c1_process(f,c1)
    skfFILE *f;
    int	c1;
{
    fprintf(stderr,"#(c1:%02x)#",c1);
    return (vGETC(f));
}

/* --------------------------------------------------------------- */
/* defschar search						   */
/* --------------------------------------------------------------- */
static int defschar_search(def,ch)
struct iso_byte_defs def[];
int ch;
{
    int idx = 0;
    while ((def[idx].defschar != 0x00) &&
    	   (def[idx].defschar != ch)) {
	idx++;
    };
    if ((def[idx].defschar != ch) || (!has_valid_ctable(&(def[idx])))){ return(-1);
    } else return(idx);
}

/* --------------------------------------------------------------- */
/* defschar set petit macros					   */
/* --------------------------------------------------------------- */
static void set_defschar_tuple(def,idx,ch)
struct iso_byte_defs def[];
int idx,ch;
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr,"<%02x>(%s)",ch,def[idx].cname);
#endif
    if (ch == 0x28) {		/* to G0		   */
	g0_table_mod = &(def[idx]);
	if (!in_left_locking_shift) {
	    g0table2low();
	};
	if ((g0_table_mod->lang) != L_NU) {
	    if (!(skf_is_strong_lang(skf_input_lang))) {
		skf_input_lang = g0_table_mod->lang;
		if (output_lang == 0) {
		    skf_output_lang = skf_input_lang;
		    show_lang_tag();
		}; 
	    }; 
	};
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"%s",g0_table_mod->desc);
#endif
    } else if ((ch == 0x29) || (ch == 0x2d)) {	/* to G1   */
	g1_table_mod = &(def[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
	if (!up_block) {
	    if (in_ls1) { g1table2low();
	    } else if (!in_right_locking_shift) { g1table2up();
	    };
	};
#endif
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"%s",g1_table_mod->desc);
#endif
    } else if ((ch == 0x2a) || (ch == 0x2e)) {	/* to G2   */
	g2_table_mod = &(def[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
	if (in_ls2) { g2table2low();
	} else if (in_rs2) { g2table2up();
	};
#endif
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"%s",g2_table_mod->desc);
#endif
    } else if ((ch == 0x2b) || (ch == 0x2f)) {	/* to G3   */
	g3_table_mod = &(def[idx]);
#ifndef	SUPPRESS_FJ_CONVENSION
	if (in_ls3) { g3table2low();
	} else if (in_rs3) { g3table2up();
	};
#endif
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"%s",g3_table_mod->desc);
#endif
    } else {
    	in_tablefault(SKF_TBLINCNSIS,"tupleset");
	skf_exit(EXIT_FAILURE); 
    };
    res_all_shift;
}

/* --------------------------------------------------------------- */
/* arib macro process						   */
/* --------------------------------------------------------------- */
/*  arib_macro_process: macro definitions.			   */
/* --------------------------------------------------------------- */
int arib_macro_process(f)
skfFILE *f;
{
    int c1,c3 = 0;      /* does not pass to/from main routine      */
    int c2,c4 = 0;
    int i,j;
    int	*macrobuf;

    if ((c1 = vGETC(f)) == sEOF) {  /* case: macro-eof		   */
	    in_undefined('m',SKF_ARIBERR);
	    return(0);
    } else if ((c1 == 0x40) || (c1 == 0x41)) {
	if ((c2 = vGETC(f)) == sEOF) {  /* case: macro-@-eof	   */
	    in_undefined('m',SKF_UNEXPEOF);
	    return(0);
	} else if ((c2 <= 0x20) || (c2 >= 0x7f)) {
	    in_undefined('o',SKF_ARIBERR);
	    return(0);
	} else;
#ifdef SKFDEBUG
	if (is_vv_debug)
		fprintf(stderr," aribmacro: $,%2x - ",c1);
#endif
	if ((macrobuf = calloc((size_t)(ARIB_MACRO_LIMIT+2),
					sizeof(int))) == NULL) {
	    skferr(SKF_MALLOCERR,2,3);
	} else;
	for (i=0;(i<(ARIB_MACRO_LIMIT - 2)) && 
		((c3 = vGETC(f)) != sEOF);i++) {
	    macrobuf[i] = c3;
	    if (c3 == AR_MAC) break;
	};
	macrobuf[i] = '\0';
	macrobuf[i+1] = '\0';
	for (j=0;j<i;j++) {
	    if ((macrobuf[j] == A_ESC) &&
	        ((macrobuf[j+1] & 0xfc) == 0x28) &&
		(macrobuf[j+2] == 0x20)) {
		/* external macro definition must not include	   */
		/*  macro expansion in macro. Test it here!	   */
		in_undefined('p',SKF_ARIBERR);
		free(macrobuf); return(c1);
	    } else;
#if 0
	    if ((macrobuf[j] == A_ESC) &&
	    	((macrobuf[j+1] == 0x7e) ||	/* LS1R		   */
		 (macrobuf[j+1] == 0x6e) ||	/* LS3		   */
		 (macrobuf[j+1] == 0x6d) ) { 	/* LS2		   */
		in_undefined('q',SKF_ARIBERR);
		free(macrobuf); return(c1);
	    } else;
#endif
	};
	if (c1 == sEOF) {	/* terminated in between	   */
	    in_undefined('m',SKF_UNEXPEOF);
	    free(macrobuf); return(c1);	/* discard everything	   */
	} else if (c3 != AR_MAC) {	/* overflowed		   */
	    in_undefined('n',SKF_ARIBERR);
	    free(macrobuf); return(c1);	/* discard everything	   */
	} else {
	    if ((c4 = vGETC(f)) < 0) { /* incomplete tail	   */
		in_undefined('n',SKF_UNEXPEOF);
		free(macrobuf); return(c1);
	    } else;
	    if ((c4 != 0x40) && (c4 != 0x4f) && (c4 != 0x41)) {
		in_undefined('m',SKF_ARIBERR);
		free(macrobuf); return(c1);	/* discard everything	   */
	    } else;
	};
	macrobuf[++i] = c3;
	macrobuf[++i] = '\0';
	if (arib_macro_rawproc(macrobuf,c2,i+1) != 0) {
	    skferr(SKF_MALLOCERR,2,3);
	} else;
	free(macrobuf);
	if (c1 == 0x41) {
#ifdef FORCE_ENABLE_ARIB_MACRO_DEF
	    (void)paraphrase_arib_macro(c2);
#else
		; /* discard everything */
#endif
	} else;
	if ((c4 == 0x40) || (c4 == 0x41)) {
	    enque(AR_MAC);
	    enque(c4);
	} else;
    } else if (c1 == 0x4f) {	/* stray macro-O */
    	;	/* just discard */
    } else {	/* not macro-@/A */
    	;
    };
    return(c1);
}

/* --------------------------------------------------------------- */
int arib_macro_rawproc(sy,c2,len)
int *sy;
int c2;		/* raw character name */
int len;
{
    int i,j;
    int *tbl;

#ifdef SKFDEBUG
    if (is_vvv_debug) {
    	fprintf(stderr,"macro-rawproc: %c(%d) -",c2,len);
    } else;
#endif
    for (j=0;(j<ARIB_MACRO_LIMIT) && (sy[j] != '\0');j++) ;
    if (arib_macro_tbl == NULL) {
    	if ((arib_macro_tbl = calloc((size_t)94,sizeof(int *))) == NULL) {
	    skferr(SKF_MALLOCERR,2,3);
	} else;
    } else;
#ifdef ARIB_MACRO_NO_REDEFINE
    if (arib_macro_tbl[c2 - 0x21] != 0) {  /* already defined	   */
    	return(0);
    } else;
#endif
    if ((tbl = calloc((size_t)(len+1),sizeof(int))) == NULL) {
    	skferr(SKF_MALLOCERR,2,2);
    } else;
    for (i=0;i<len;i++) tbl[i] = sy[i];
    tbl[i]='\0';
    arib_macro_tbl[c2 - 0x21] = tbl;

    return(0);
}

int is_charset_macro(pset)
struct iso_byte_defs *pset;
{
    if (pset == NULL) return(0);
    if ((pset->defschar) != 0x70) return(0);
    if ((pset->is_kana) != (COD_PRIV | COD_SET96)) return(0);
    return(1);
}

/* --------------------------------------------------------------- */
/* arib rpc process						   */
/*  return: repeat count + 1					   */
/* --------------------------------------------------------------- */
int arib_rpc_process(f)
skfFILE *f;
{
    int c1;

    if ((c1 = vGETC(f)) == sEOF) {
    	return(0);
    } else if ((c1>= 0x40) && (c1 <= 0x7f)) {
    	c1 = c1 - 0x40 + 1;
#if 0
	if (c1 == 0) {
	    c1 = mime_fold_llimit - fold_count - ARIB_RPC_MGN - 1;
	    if (c1 < 0) c1 = 0;
	} else;
#endif
    } else c1 = 0;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr,"repeat -%d ",c1 - 1);
#endif
    return(c1);
}
