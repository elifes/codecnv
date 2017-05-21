/* *******************************************************************
** Copyright (c) 2000-2014 Seiji Kaneko. All rights reserved.
** Copyright (c) 2003 The Internet Society. All rights reserved.
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
    in_decoder.c	input-side converter decode routines
    $Id: in_decoder.c,v 1.73 2017/01/05 15:05:48 seiji Exp seiji $

    Punycode decode part is based on the work copyrighted by 
    The Internet society.
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "skf.h"
#include "skf_fileio.h"
#include "oconv.h"
#include "convert.h"

#define CIRCULAR_BUF	1

/* --------------------------------------------------------------- */
/* code decorder : generic decode preprocessor			   */
/* --------------------------------------------------------------- */
#define	PREQDEPTH	512	/* must be 2^n			   */
#define QPAD		16
#define	PSTQDEPTH	256	/* PSTQDEPTH > PUNY_BUFLEN	   */
				/* also must be 2^N		   */
static skf_ucode decode_poke_buf = -1;  /* mime tail state	   */
#ifdef ENABLE_DISABLED
static unsigned short mime_tail = 0;	/* mime tail char state	   */
#endif
static unsigned char decode_pre_queue[PREQDEPTH];
		/* predecode temporal queue: already cheched	   */
static int decode_post_queue[PSTQDEPTH];
		/* postdecode temporal queue: unchecked		   */
static int decode_pre_rptr = 0;    /* predecode read pointer	   */
static int decode_pre_wptr = 0;    /* predecode write pointer	   */
static int decode_post_rptr = 0;   /* postdecode read pointer	   */
static int decode_post_wptr = 0;   /* postdecode write pointer	   */
static int decode_post_mark = 0;   /* postdecode header marking	   */
static int mime_res_bit = 0;
static int mime_y_res = 0;
static int sp_mime = 0;
static int in_mime = 0;
static int in_rfc2231 = 0;
static int in_hex = 0;
       int has_mime = 0;
static struct iso_byte_defs *g0_table_save; /* save in mime mode   */
static struct iso_byte_defs *g1_table_save;
static unsigned long	ucod_fl_save = 0;
static unsigned long encode_cap_save = 0;
static unsigned long codeset_flavor_save = 0;
static int	le_detect_save = 0;
static int	i_codeset_save = -1;
static unsigned long conv_cap_save = 0;
static int	expect_discard = FALSE;

static int	uri_esc_hook = 0;
static int	rot_esc_hook = 0;

/* --- internal work for u_dec_hook ------------------------------ */
static int	    utf7in = 0;
static int	    res_bit = 0;
static int	    y_res = 0;

#define SP_Q	0x0002
#define SP_B	0x0001
#define SP_D7	0x0010
#define SP_D8	0x0020

#define is_b_encode	(((sp_mime & 0x0fU) == 0x01) \
	|| ((sp_mime == 0) && (is_mimeb_strict || is_mimeb_encoded)))
#define is_q_encode	(((sp_mime & 0x0fU) == 0x02) \
	|| ((sp_mime == 0) && is_mimeq_encoded))
#define is_d7_encode	((sp_mime & 0x00f0U) == SP_D7)
#define is_d8_encode	((sp_mime & 0x00f0U) == SP_D8)
/* Note: mime spec(rfc2047) requires q-encoding hex is capital,
    but skf will process both (no reason to restrict for decoding). */
#define is_oct_char(x)  ((x >= '0') && (x <= '7'))

#if !defined(CIRCULAR_BUF)
#define pre_Qfull()	(decode_pre_wptr == (PREQDEPTH - QPAD - 4))
#define pre_Qofull()	(decode_pre_wptr >= (PREQDEPTH - QPAD))
#define pre_Qempty	(decode_pre_wptr <= decode_pre_rptr)
#define pre_Qdeque	(decode_pre_queue[decode_pre_rptr++])
#define pre_Qenque(x)	decode_pre_queue[decode_pre_wptr++] = x
#define pre_Qclear()	{decode_pre_wptr = (decode_pre_rptr = 0);}
#define post_Qfull()	(decode_post_wptr == (PSTQDEPTH - QPAD - 4))
#define post_Qempty	(decode_post_wptr <= decode_post_rptr)
#define post_Qdeque	(decode_post_queue[decode_post_rptr++])
#define post_Qenque(x)	decode_post_queue[decode_post_wptr++] = x
#define post_Qclear()	{decode_post_wptr = (decode_post_rptr = 0);}
#define post_Qunenque()	{if (decode_post_wptr > 0) decode_post_wptr--;}
#define get_postq_mark() decode_post_wptr;
#define set_postq_mark(x) {decode_post_wptr = x;}
#define delete_postQ_tail() {decode_post_wptr--;}
#define pre_Qdepth()	(decode_pre_wptr - decode_pre_rptr)
#define post_Qdepth()   (decode_post_wptr - decode_post_rptr)
#define pre_Q128spare	(decode_pre_wptr <= (PREQDEPTH - 129))

static void post2preQueue()
{
    int i,j;
    for (i=decode_post_mark,j=0;i<decode_post_wptr;i++,j++) {
	decode_pre_queue[j] = decode_post_queue[i];
    };
    decode_pre_wptr = j+1; decode_pre_rptr = 0;
    post_Qclear();
}

#else	/* CIRCULAR BUFFER */
/*@+boolint@*/
int pre_Qfull()
{
    int	out_pre_rbuf = decode_pre_rptr + PREQDEPTH;
    if (decode_pre_rptr <= decode_pre_wptr) {
	return((decode_pre_wptr + QPAD - 4) > out_pre_rbuf);
    } else return((decode_pre_wptr + QPAD - 4) > decode_pre_rptr);
}

/*@+boolint@*/
static int pre_Qofull()
{
    int	out_pre_rbuf = decode_pre_rptr + PREQDEPTH;
    if (decode_pre_rptr <= decode_pre_wptr) {
	return((decode_pre_wptr + QPAD) > out_pre_rbuf);
    } else return((decode_pre_wptr + QPAD) > decode_pre_rptr);
}
#define pre_Qempty	(decode_pre_wptr == decode_pre_rptr)
#define pre_Qdeque	(decode_pre_queue[((decode_pre_rptr++)&(PREQDEPTH-1))])
#define pre_Qenque(x) \
	decode_pre_queue[((decode_pre_wptr++)&(PREQDEPTH-1))] = x
#define pre_Qclear()	{decode_pre_wptr = (decode_pre_rptr = 0);}

/*@+boolint@*/
int post_Qfull()
{
    int	out_post_rbuf = decode_post_rptr + PSTQDEPTH;
    if (decode_post_rptr < decode_post_wptr) {
	return((decode_post_wptr + QPAD) > out_post_rbuf);
    } else return((decode_post_wptr + QPAD) > decode_post_rptr);
}
#define post_Qempty	(decode_post_wptr == decode_post_rptr)
#define post_Qdeque \
	(decode_post_queue[((decode_post_rptr++)&(PSTQDEPTH-1))])
#define post_Qenque(x) \
	decode_post_queue[((decode_post_wptr++)&(PSTQDEPTH-1))] = x
#define post_Qclear() \
	{decode_post_wptr = (decode_post_rptr = 0);}
#define post_Qunenque()	\
    {if (decode_post_wptr != decode_post_rptr) \
       decode_post_wptr = ((PSTQDEPTH + decode_post_wptr - 1)&(PSTQDEPTH-1));}
#define pre_Qdepth()	((decode_pre_wptr >= decode_pre_rptr) ? \
	(decode_pre_wptr - decode_pre_rptr) : \
	((decode_pre_wptr + PREQDEPTH) - decode_pre_rptr))
#define post_Qdepth()   ((decode_post_wptr >= decode_post_rptr) ? \
	(decode_post_wptr - decode_post_rptr) : \
	((decode_post_wptr + PSTQDEPTH) - decode_post_rptr))
#define get_postq_mark() decode_post_wptr;
#define set_postq_mark(x) {decode_post_wptr = x;}
#define delete_postQ_tail() \
    { decode_post_wptr = ((PSTQDEPTH + decode_post_wptr - 1)&(PSTQDEPTH-1));}
static void post2preQueue()
{
    int i,j;
    pre_Qclear();
    for (i=decode_post_mark,j=0;i!=decode_post_wptr;i++,j++) {
	decode_pre_queue[j] = decode_post_queue[i];
	i &= (PSTQDEPTH-1);
    };
    decode_pre_wptr = j+1; decode_pre_rptr = 0;
    post_Qclear();
}

#endif

#ifdef ACE_SUPPORT
/* --------------------------------------------------------------- */
/* internationalized IDN decoders(RFC 3492)			   */
/* --------------------------------------------------------------- */
static skf_ucode puny_in_buf[PUNY_BUFLEN+16]; /* input-side buffer */
static skf_ucode puny_out_buf[PUNY_BUFLEN+16];/* output-side buffer */

int in_ace = 0;

#define is_label_delim(x) ((x == '.') || (x <= 0x20))
#define is_puny_flagged(x) ((punycode_uint)(x) - 65 < 26)
#define puny_flagged(x) (x <= 'Z')

/*@+boolint@*/
static int is_puny_attr_char(int x)
{
   if (is_digit(x) || is_alpha(x)) return(TRUE);
   return((x == '_') || (x == '.') ||
	  (x == '!') || (x == 0x23) || (x == 0x24) || 
	  (x == '+') || (x == '-') || (x == '^') || 
	  (x == '&') || (x == 0x60) || ((x >= 0x7b) && (x <= 0x7e)));
}

skf_ucode puny_adapt(delta, num, firsttime)
long delta; long num; int firsttime;
{
    long k;
    delta = firsttime ? (delta / PUNY_DUMP) : (delta >> 1);
    delta += delta / num;
    for (k=0; delta > ((PUNY_BASE - PUNY_TMIN) * PUNY_TMAX)/2;
		k += PUNY_BASE) { delta /= (PUNY_BASE - PUNY_TMIN);
    };
    return ((skf_ucode)(k + 
	((PUNY_BASE - PUNY_TMIN + 1) * delta)/(delta + PUNY_SKEW)));
} 

static punycode_uint decode_puny_digit(punycode_uint cp)
{
  return  ((cp - 48 < 10) ? (cp - 22) :  ((cp - 65 < 26) ? cp - 65 :
          (((cp - 97) < 26) ? (cp - 97) :  PUNY_BASE)));
}

/*** Main decode function ***/

/* --- Punycode (RFC 3492) --- */
/*@+boolint@*//*@-looploopbreak@*/
static int punycode_decode(input_length, input, output_length, output)
  int input_length;
  const skf_ucode input[];
  int *output_length;
  skf_ucode output[];
{
  punycode_uint n, out, i, bias,
                 b, j, in, oldi, w, k, digit, t;

  /* Initialize the state: */

  n = PUNY_INIT_N;
  out = i = 0;
  bias = PUNY_I_BIAS;

  /* Handle the basic code points:  Let b be the number of input code */
  /* points before the last delimiter, or 0 if there is none, then    */
  /* copy the first b code points to the output.                      */

  for (b = j = 0;  j < input_length;  ++j)
	if (is_puny_delim(input[j])) b = j;

  for (j = 0;  j < b;  ++j) {
    if (!is_puny_basic(input[j])) {
	return punycode_lencheck;
    };
    output[out++] = input[j];
  }

  /* Main decoding loop:  Start just after the last delimiter if any  */
  /* basic code points were copied; start at the beginning otherwise. */

  for (in = b > 0 ? b + 1 : 0;  in < input_length;  ++out) {

    /* in is the index of the next character to be consumed, and */
    /* out is the number of code points in the output array.     */

    /* Decode a generalized variable-length integer into delta,  */
    /* which gets added to i.  The overflow checking is easier   */
    /* if we increase i as we go, then subtract off its starting */
    /* value at the end to obtain delta.                         */

    for (oldi = i, w = 1, k = PUNY_BASE;  ;  k += PUNY_BASE) {
      if (in >= input_length) {
	return punycode_lenover;
      };
      digit = decode_puny_digit(input[in++]);
      if (digit >= PUNY_BASE) {
	return punycode_bad_input;
      };
      if (digit > (PUNY_MAXINT - i) / w) return punycode_overflow;
      i += digit * w;
      t = k <= bias /* + PUNY_TMIN */ ? PUNY_TMIN : /* +tmin not needed */
          (k >= bias + PUNY_TMAX ? PUNY_TMAX : k - bias);
      if (digit < t) break;
      if (w > (PUNY_MAXINT / (PUNY_BASE - t))) {
	return punycode_woverflow;
      };
      w *= (PUNY_BASE - t);
    }
    bias = puny_adapt(i - oldi, out + 1, oldi == 0);

    /* i was supposed to wrap around from out+1 to 0,   */
    /* incrementing n each time, so we'll fix that now: */

    if (i / (out + 1) > PUNY_MAXINT - n) {
	return punycode_overflow;
    };
    n += i / (out + 1);
    i %= (out + 1);

    /* Insert n at position i of the output: */

    /* not needed for Punycode: */
    /* if (decode_puny_digit(n) <= base) return punycode_invalid_input; */
    if (out >= PUNY_BUFLEN) {
	return punycode_big_output;
    };

#ifdef HAVE_MEMMOVE
	/* SVR4, 4.3 BSD, C99 */
    memmove(output + i + 1, output + i, 
	(size_t)((out - i) * sizeof * output));
#else
#ifdef HAVE_BCOPY
	/* no memmove -> seems to be old system. try bcopy */
    bcopy(output + i, output + i + 1, 
	(size_t)((out - i) * sizeof * output));
#else
	/* no memmove nor bcopy. maybe kind of minGW */ 
    memcpy(output + i + 1, output + i, 
	(size_t)((out - i) * sizeof * output));
#endif
#endif
    output[i++] = n;
  }

  *output_length = out;
  return punycode_success;
}

/* --- RACE code --- */
static int race_decode_digit(punycode_uint rc)
{
    return((rc < 'a') ? ((rc < '2') ? -1 : ((rc < '8') ? rc - '2'+26 : -1)) :
    	((rc <= 'z') ? (rc - 'a') : -1));
}

/*@+boolint@*/ /*@-paramuse@*/
static int racecode_decode(input_length, input, output_length, output)
  punycode_uint input_length;
  const skf_ucode input[];
  punycode_uint *output_length;
  skf_ucode output[];
{
    int	b32stat = 0;
    int res = 0;
    int	i,chu,uv,uw;
    int u1 = 0;
    int	lcheck = 0, wn1 = 0;

#ifdef SKFDEBUG
    if (is_vvv_debug) fprintf(stderr,"race(%d): -",input_length);
#endif
    *output_length = 0;
    for (i=0; i<input_length;) {
	if ((uv = race_decode_digit(input[i++])) < 0) {
	    break;	/* conversion end (tail or error)	   */
	};
	if (b32stat == 2) {
	    if ((uw = race_decode_digit(input[i++])) < 0) {
		return punycode_bad_input; /* should not occur	   */
	    };
	    chu = res + ((uv & 0x1fU) << 1) + ((uw & 0x10U) >> 4);
	    res = ((uw & 0x0fU) << 4);
	    b32stat = 4;
	} else if (b32stat == 4) {
	    chu = res + ((uv & 0x1eU) >> 1);
	    res = ((uv & 0x01U) << 7);
	    b32stat = 1;
	} else if (b32stat == 1) {
	    if ((uw = race_decode_digit(input[i++])) < 0) {
		return punycode_bad_input; /* should not occur	   */
	    };
	    chu = res + ((uv & 0x1fU) << 2) + ((uw & 0x18U) >> 3);
	    res = ((uw & 0x07U) << 5);
	    b32stat = 3;
	} else if (b32stat == 3) {
	    chu = res + ((uv >> 2) & 0xfU); b32stat = 2;
	    res = (uv << 6) & 0xc0U;
	    b32stat = 3;
	} else {	/* b32stat == 0				   */
	    if ((uw = race_decode_digit(input[i++])) < 0) {
		return punycode_bad_input; /* should not occur	   */
	    };
	    chu = ((uv & 0x1fU) << 3) + ((uw & 0x1cU) >> 2);
	    res = ((uw & 0x03U) << 6);
	    b32stat = 2;
	};
	if (((i != 0) || (chu != 0xd8)) 
		&& (lcheck == 0) && (wn1 == 0)) {
	    u1 = chu; wn1 = 1;
	} else if (wn1 == 1) {		/* Step 2		   */
	    if (uv == 0xff) {
		wn1 = 5; continue;
	    };
	    if ((u1 == 0) && (chu == 0x99)) { /* Step 3 & 4	   */
		return(punycode_bad_input); /* expected error	   */
	    };
	    output[*output_length] = (u1 << 8) + chu;
	    *output_length += 1;
	    wn1 = 1; continue; 
	} else if (wn1 == 5) {		/* Step 5		   */
	    if (chu == 0x99) {
		output[*output_length] = (u1 << 8) + 0xff;
		*output_length += 1;
	    } else {
		output[*output_length] = chu;
		*output_length += 1;
	    };
	    wn1 = 1; continue;		/* return to step 2	   */
	} else if (lcheck == 1) {
	    u1 = chu; lcheck = 2; continue;
	} else if (lcheck == 2) {
	    output[*output_length] = (u1 << 8) + chu;
	    *output_length += 1;
	    lcheck = 1; continue;
	} else {	/* LCHECK start  case			   */
	    lcheck = 1; continue; /* skip lcheck validate, for	   */
			  /* such check in skf is worthless	   */
	}; 
    };
    return (punycode_success);
}
#endif

/* --------------------------------------------------------------- */
/* Various tools						   */
/* --------------------------------------------------------------- */
static int mime_res_tbl[5] = { 4,0,0,0,2 };

static void low2convtbl()
/*@globals low_table,low_ltable,low_table_limit,low_dbyte,low_table_mod,
	low_kana,skf_output_lang,skf_input_lang;@*/
/*@modifies low_table,low_ltable,low_table_limit,low_dbyte,
	low_kana,skf_output_lang,skf_input_lang;@*/
{
    low_table = low_table_mod->unitbl;
    low_ltable = low_table_mod->uniltbl;
    low_table_limit = low_table_mod->table_len;
    low_dbyte = is_multibyte(low_table_mod->char_width);
    low_kana = cod_kana_mask(low_table_mod->is_kana);
    if (low_table_mod->lang != L_NU) {
	  skf_input_lang = skf_get_wlang(low_table_mod->lang);
	  if (output_lang == 0) {
	      skf_output_lang = skf_input_lang;
	      show_lang_tag();
	  }; };
    if ((is_tblshort(low_table_mod->char_width) && (low_table == NULL))||
    	(is_tbllong(low_table_mod->char_width) && (low_ltable == NULL))) {
	skferr(SKF_LOW2ERRDUMP,(long)0,(long)0);
    };
}

static void up2convtbl()
/*@globals up_table,up_ltable,up_table_limit,up_dbyte,up_table_mod,
	up_kana;@*/
/*@modifies up_table,up_ltable,up_table_limit,up_dbyte,
	up_kana;@*/
{
    up_table = up_table_mod->unitbl;
    up_ltable = up_table_mod->uniltbl;
    up_table_limit = up_table_mod->table_len;
    up_dbyte = up_table_mod->char_width;
    up_kana = cod_kana_mask(up_table_mod->is_kana);
    if (((up_table == NULL) && (is_tblshort(up_dbyte)))
      || ((up_ltable == NULL) && (is_tbllong(up_dbyte)))) {
	skferr(SKF_UP2ERRDUMP,(long)0,(long)0);
    };
}

/*@-globstate@*/
void g0table2low()
{
    if (g0_table_mod != NULL) {	/* it is null when input undefined */
      if ((is_tbllong(g0_table_mod->char_width) &&
		(g0_table_mod->uniltbl != NULL)) ||
	  (g0_table_mod->unitbl != NULL)) {
	  low_table_mod = g0_table_mod;
	  low2convtbl();
      };
      if (is_charset_macro(low_table_mod) == 1) set_macl;
      else res_macl;
    };
}

void g1table2low()
/*@globals g1_table_mod,low_table_mod,sshift_condition;@*/
/*@modifies low_table_mod,sshift_condition;@*/
{
    if (g1_table_mod != NULL) {	/* it is null when input undefined */
      if ((is_tbllong(g1_table_mod->char_width) &&
		(g1_table_mod->uniltbl != NULL)) ||
	  (g1_table_mod->unitbl != NULL)) {
	  low_table_mod = g1_table_mod;
	  low2convtbl();
      };
      if (is_charset_macro(low_table_mod) == 1) set_macl;
      else res_macl;
    };
}

void g2table2low()
/*@globals g2_table_mod,low_table_mod,sshift_condition;@*/
/*@modifies low_table_mod,sshift_condition;@*/
{
    if (g2_table_mod != NULL) {	/* it is null when input undefined */
      if ((is_tbllong(g2_table_mod->char_width) &&
		(g2_table_mod->uniltbl != NULL)) ||
	  (g2_table_mod->unitbl != NULL)) {
	  low_table_mod = g2_table_mod;
	  low2convtbl();
      };
      if (is_charset_macro(low_table_mod) == 1) set_macl;
      else res_macl;
    };
}

void g3table2low()
/*@globals g3_table_mod,low_table_mod,sshift_condition;@*/
/*@modifies low_table_mod,sshift_condition;@*/
{
    if (g3_table_mod != NULL) {	/* it is null when input undefined */
      if ((is_tbllong(g3_table_mod->char_width) &&
		(g3_table_mod->uniltbl != NULL)) ||
	  (g3_table_mod->unitbl != NULL)) {
	  low_table_mod = g3_table_mod;
	  low2convtbl();
      };
      if (is_charset_macro(low_table_mod) == 1) set_macl;
      else res_macl;
    };
}

void g0table2up()
/*@globals g0_table_mod,up_table_mod,sshift_condition;@*/
/*@modifies up_table_mod,sshift_condition;@*/
{
    if (g0_table_mod != NULL) {
      if ((is_tbllong(g0_table_mod->char_width) &&
		(g0_table_mod->uniltbl != NULL)) ||
	  (g0_table_mod->unitbl != NULL)) {
	  up_table_mod = g0_table_mod;
	  up2convtbl();
      } else;
      if (is_charset_macro(up_table_mod) == 1) set_macr;
      else res_macr;
    } else;
}

void g1table2up()
/*@globals g1_table_mod,up_table_mod,sshift_condition;@*/
/*@modifies up_table_mod,sshift_condition;@*/
{
    if (g1_table_mod != NULL) {
      if ((is_tbllong(g1_table_mod->char_width) &&
		(g1_table_mod->uniltbl != NULL)) ||
	  (g1_table_mod->unitbl != NULL)) {
	  up_table_mod = g1_table_mod;
	  up2convtbl();
      } else;
      if (is_charset_macro(up_table_mod) == 1) set_macr;
      else res_macr;
    } else;
}

void g2table2up()
/*@globals g2_table_mod,up_table_mod,sshift_condition;@*/
/*@modifies up_table_mod,sshift_condition;@*/
{
    if (g2_table_mod != NULL) {
      if ((is_tbllong(g2_table_mod->char_width) &&
		(g2_table_mod->uniltbl != NULL)) ||
	  (g2_table_mod->unitbl != NULL)) {
	  up_table_mod = g2_table_mod;
	  up2convtbl();
      };
      if (is_charset_macro(up_table_mod) == 1) set_macr;
      else res_macr;
    };
}

void g3table2up()
/*@globals g3_table_mod,up_table_mod,sshift_condition;@*/
/*@modifies up_table_mod,sshift_condition;@*/
{
    if (g3_table_mod != NULL) {
      if ((is_tbllong(g3_table_mod->char_width) &&
		(g3_table_mod->uniltbl != NULL)) ||
	  (g3_table_mod->unitbl != NULL)) {
	  up_table_mod = g3_table_mod;
	  up2convtbl();
      };
      if (is_charset_macro(up_table_mod) == 1) set_macr;
      else res_macr;
    };
}

#if	defined(ROT_SUPPORT) && defined(NEW_ROT_CODE)
/* --------------------------------------------------------------- */
/* for rot13 ---------------- */
static const unsigned char rot13_tbl_d[58] = {
   'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   'A','B','C','D','E','F','G','H','I','J','K','L','M',
   0x5b,0x5c,0x5d,0x5e,0x5f,0x60,
   'n','o','p','q','r','s','t','u','v','w','x','y','z',
   'a','b','c','d','e','f','g','h','i','j','k','l','m'
};

static const unsigned char valid_name_map[128] = {
   0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
   1,0,1,1,1,1,1,0, 1,1,1,1,1,1,1,0,
   1,1,1,1,1,1,1,1, 1,1,1,1,0,1,0,0,
   1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
   1,1,1,1,1,1,1,1, 1,1,1,1,0,1,1,1,
   0,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
   1,1,1,1,1,1,1,1, 1,1,1,1,0,1,1,0
};

int skf_rot47conv_d(c2)
int c2;
{
    if ((c2 > A_SP) && (c2 < A_DEL)) {
	return (((c2 < 'P') ? (c2 + 47) : (c2 - 47)));
    } else return(c2);
}

int skf_rot13conv_d(c1)
int c1;
{
    if ((c1 < 'A') || (c1 > 'z') || (c1 == sEOF)) return(c1);
	    else return(rot13_tbl_d[c1 - 'A']);
}
#endif

/* --------------------------------------------------------------- */
/* charset/codeset name compare utility				   */
/* --------------------------------------------------------------- */
/* Note: match policies						   */
/*  (1) case is ignored (not case-sensitive)			   */
/*  (2) '-' and '_' is ignored.					   */
/*  (3) '?' in sb matches any one character in sh.		   */
/*  (4) character set name is at most MIME_CSET_LEN length.	   */
/*  (5) terminate if sb is not alphanumeric, - or  _		   */
/* --------------------------------------------------------------- */
/* return value:						   */
/*  1 : exact match						   */
/*  0 : fuzzy match (skf default match)				   */
/*  -1: not match						   */
/* --------------------------------------------------------------- */
int cname_comp(sh,sb)
char	*sh,*sb;
{
    int i = 0;
    char ch,cb;

    if ((sh == NULL) || (sb == NULL)) return(-1);
/* cut x- header */
    if ((*sh == 'x') && (*(sh + 1) == '-')) sh += 2;
    if ((*sb == 'x') && (*(sb + 1) == '-')) sb += 2;

    while ((*sh != '\0') && (*sb != '\0') && (++i < MIME_CSET_LEN)) {
	ch = SKFtolower(*sh); /* with canonicalization		   */
	if (!is_cname_char(ch)) {
	    break;
	};
	cb = SKFtolower(*sb);
	if ((cb == '-') || (cb == '_')) {
	    sb++; continue;
	};
	if ((ch == '-') || (ch == '_')) {
	    sh++; continue;
	};
	if ((ch != '?') && (cb != ch)) {
	    return(-1);
	};
	sb++; sh++;
    };
    if ((*sh == '\0') && (*sb == '\0')) return(1);
    else if (*sb == '\0') return(0);
    else return(-1);
}

/* --------------------------------------------------------------- */
/* mime decoder subroutines					   */
/* --------------------------------------------------------------- */
/*@-globstate@*/
static void codeset_recover_from_mime()
{
#ifdef SKFDEBUG
    if (is_vv_debug) {
    	fprintf(stderr,"\n codeset recovery - ");
	if (i_codeset_save < 0) fprintf(stderr,"RETUNDEF ");
    } else;
#endif
    g0_table_mod = g0_table_save; g0table2low();
    g1_table_mod = g1_table_save; g1table2up();
    in_codeset = i_codeset_save;
    i_codeset_save = -1;
    le_detect = le_detect_save;
    conv_cap = conv_cap_save;
    encode_cap = encode_cap_save; 
    ucod_flavor = ucod_fl_save;
    codeset_flavor = codeset_flavor_save;
    res_all_shift; utf7in = 0; res_bit = 0; y_res = 0;
}

/*@-globstate@*/
static void codeset_push_for_mime()
{
    g0_table_save = g0_table_mod;
    g1_table_save = g1_table_mod;
    i_codeset_save = in_codeset;
    conv_cap_save = conv_cap;
    encode_cap_save = encode_cap; 
    le_detect_save = le_detect;
    ucod_fl_save = ucod_flavor;
    codeset_flavor_save = codeset_flavor;
    res_all_shift; utf7in = 0; res_bit = 0; y_res = 0;
}

/*
 * test whether we are in pushed code set, without determining
 * primary codeset.
 */
int test_primary_codeset()
{
    if (i_codeset_save == -1) return(TRUE);
    else return(FALSE);
}

int hook_getc(f,flg)
int flg;
skfFILE *f;
{ 
    int ch;

    if (!pre_Qempty) {
	ch = pre_Qdeque;
	if (pre_Qempty) pre_Qclear();
    } else {
	ch = (flg) ? (rGETC(f)) : (GETC(f));
    };

    return(ch);
}

int hook_q_getc(f,flg)
int flg;
skfFILE *f;
{
    
    int ch;
    if (!pre_Qempty) {
	ch = pre_Qdeque;
	if (pre_Qempty) pre_Qclear();
    } else {
	ch = (flg) ? (rGETC(f)) : (GETC(f));
    };
    if (ch >= 0) post_Qenque(ch);
    return(ch);
}

int unhook_getc(f,flg)
int flg;
skfFILE *f;
{ 
    int ch;

    ch = (flg) ? (rGETC(f)) : (GETC(f));
    return(ch);
}

/* --------------------------------------------------------------- */
/* parsing mime-specified charset 				   */
/* return value:						   */
/*	0:  parse succeeded 					   */
/*	-1: failed to parse, or undecodable charset		   */
/* --------------------------------------------------------------- */

int parse_mime_charset(sy)
int	*sy;
{
    int i;
    int	sss;
    char mime_cname[MIME_CSET_LEN];

    sy += 2;	/* skip '=?' part				   */
    /* put codeset name part into mime_cname	*/
    for (i=0;i<MIME_CSET_LEN;i++) {
	if ((sy[i] == '\0') || (sy[i] == '?')
			|| (is_rfc2231_encoded && (sy[i] == 0x27))) {
	    mime_cname[i] = '\0'; break;
	};
	mime_cname[i] = sy[i];
    };
    mime_cname[MIME_CSET_LEN - 1] = '\0'; /* add terminator	   */
    /* call search routines */
    if ((sss = skf_search_cname(mime_cname)) < 0) { /* not found   */
	if ((sss = skf_option_parser(mime_cname,
				codeset_option_code)) < 0) {
	    in_codeset = DEFAULT_I; return(-2);
	};
    };
    in_codeset = sss;
    return(0);
}

/* ------------------------ */
/* decoder main		    */
/* ------------------------ */
int decode_hook(f,flg)
skfFILE *f;
int	flg;
{
    int c1, c2 = 0, c3, ch = sEOF, c4;
    int inp;
    int qmark = 0;
    int	pres = 0;		/* parse result			   */
    int i;			/* character decode limiter	   */
    int decode_hex = FALSE;
    skf_ucode rcod;
    int postq_mark, isdigited;
#ifndef NEW_SOFTWRAP_CODE
    int internal_detect_crlf = 0;
#endif
#ifdef ACE_SUPPORT
    long 	k;
    skf_ucode	w;
    int		outchar = 0;
#endif
    int mime_cset_buf[MIME_CSET_LEN + 3];
#ifdef NEW_SOFTWRAP_CODE
    int mime_mark;
#endif

    /* normal case: get one char and process it */
    while (TRUE) {
      if (!post_Qempty) {	/* output postQueue first.	   */
		      /* it is already determined as not encoded */
	c1 = post_Qdeque;
#ifdef SKFDEBUG
	if (is_vvv_debug) fprintf(stderr,"\ndpq(%d,%d): %02x",
		decode_post_rptr,decode_post_wptr,c1);
#endif
	return(c1);
      };
      post_Qclear();		/* re-initialize after Queue empty */
      if ((c1 = hook_getc(f,flg)) == sEOF) {
	    mime_res_bit = 0; return(c1);
      };
#ifdef SKFDEBUG
      if (is_vvv_debug) fprintf(stderr,"\ndh_raw: %02x(%03d,%03d)",
				c1,decode_pre_rptr,decode_pre_wptr);
#endif
      if (((is_hex_encoded && is_hex_cap && (c1 == ':'))
	|| ((is_hex_uri || (is_hex_encoded && !is_hex_cap)) &&
		(c1 == '%'))
	|| (is_rfc2231_encoded && in_rfc2231 && (c1 == '%')))
	&& ((!low_dbyte) || (uri_esc_hook == 0))) {
			/* *************************************** */
			/* hex-cap-uri expr. decode		   */
			/* *************************************** */
	  post_Qenque(c1);
	  if ((c3 = hook_getc(f,flg)) == sEOF) continue;
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-%02x(%d:HEX)",
		c3,uri_esc_hook);
#endif
	  if (pre_Qofull()) {
		skferr(SKF_DECERRDUMP,decode_pre_wptr,decode_pre_rptr);
	  };
	  if (!is_hex_char(c3)) {
	      pre_Qenque(c3); continue;   /* broken		   */
	  };
	  if ((c2 = hook_q_getc(f,flg)) == sEOF) continue;
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-%02x ",c2);
#endif
	  pre_Qenque(c3); pre_Qenque(c2);
	  if (!is_hex_char(c2)) continue;   /* broken		   */
	  ch = (skf_hex(c3) << 4) + skf_hex(c2);
	  post_Qclear(); pre_Qclear();
	  return(ch);
      } else if (is_oct_encoded && (c1 == A_YEN)) {
			/* *************************************** */
			/* octal expr. decode			   */
			/* *************************************** */
	  post_Qenque(c1);
	  if ((c1 = hook_getc(f,flg)) == sEOF) continue;
	  pre_Qenque(c1); if (!is_oct_char(c1)) continue;
	  if ((c2 = hook_getc(f,flg)) == sEOF) continue;
	  pre_Qenque(c2); if (!is_oct_char(c2)) continue;
	  if ((c3 = hook_getc(f,flg)) == sEOF) continue;
	  pre_Qenque(c3); if (!is_oct_char(c3)) continue;
	  ch = (((c3 - '0') << 6) + ((c2 - '0') << 3) + c1 - '0')
	  	& 0xffU;
	  post_Qclear(); pre_Qclear(); return(ch);
#ifdef ACE_SUPPORT
      } else if (is_rfc2231_encoded && in_rfc2231 && 
		  (!is_puny_attr_char(c1))) {
#else
      } else if (is_rfc2231_encoded && in_rfc2231) {
#endif
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x-Fin",c1);
#endif
	in_rfc2231 = 0;
	codeset_recover_from_mime();
	clear_after_mime();
	decode_poke_buf = -2;	/* end indicator on	   */
	post_Qclear(); pre_Qclear();
	return(sOCD);
      } else if ((is_hex_uri || is_hex_uri_docomo) && (c1 == '&')) {
			/* *************************************** */
			/* uri codepoint expr. decode		   */
			/* *************************************** */
	  post_Qenque(c1);
	  if ((c3 = hook_getc(f,flg)) == sEOF) continue;
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-%02x(URI)",c3);
#endif
	  pre_Qenque(c3);
	  if (c3 != '#') continue;	/* is not uri-hex	   */
	  codeset_push_for_mime();
	  in_codeset = codeset_utf16le;
	  set_dummy_detect;
	  set_in_endian;	/* set big endian		   */
	  post_Qclear(); pre_Qclear(); in_hex = TRUE;
	  return(sOCD);		/* code-change: flush buffers	   */
      } else if (in_hex) {	/* in decoding uri-hex		   */
	  c2 = c1; rcod = 0;
	  for (i=1; i<8; i++) {
	    if ((i == 1) && (c2 == 'x')) { decode_hex = TRUE; rcod = 0;
	    } else if (c2 == ';') { break;
	    } else if (decode_hex && is_hex_char(c2)) {
	      rcod = (rcod << 4) + skf_hex(c2);
	    } else if (is_digit(c2)) {
	      rcod = (10*rcod) + skf_hex(c2);
	    } else {
		post_Qenque(c2); break;
	    };
	    if ((c2 = hook_getc(f,flg)) == sEOF) continue;
	  }; 
	  if (rcod < 0x1000000) {
	      if ((rcod > 0xf740) && (rcod < 0xf8ff) 
			&& is_hex_uri_docomo) {
		if (rcod < 0xf7ff) {
		    oconv(rcod - 0xf740 + 0xe500);
		} else {
		    oconv(rcod - 0xf840 + 0xe800);
		};
	      } else {
		oconv(rcod);
	      };
	  } else {
	    in_undefined(rcod,SKF_DECODERR); continue;
	  };
	  in_hex = FALSE; codeset_recover_from_mime();
	  return(sOCD);		/* rewind to previous code	   */
      } else if (is_base64_encoded) { /* same as mime-b	   */
			/* *************************************** */
			/* Base64 decoder 			   */
			/* *************************************** */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," B64");
#endif
	if ((c3 = y_in_dec(c1)) >= 0) {
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"(%d)",mime_res_bit);
#endif
	  if (mime_res_bit == 2) {
	    ch = mime_y_res + c3; mime_y_res = 0;
	  } else if (mime_res_bit == 4) {
	    ch = mime_y_res + ((c3 >> 2) & 0xfU);
	    mime_y_res = (c3 << 6) & 0xc0U;
	  } else {
	    if ((c2 = hook_q_getc(f,flg)) == sEOF) {
	      if (!is_mime_nkfmode) post_Qenque(c1);
	      in_mime = 0; continue;
	    };
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"-%02x ",c2);
#endif
	    post_Qclear(); 
	    ch = (c3 << 2) + ((y_in_dec(c2) >> 4) & 0x3U);
	    mime_y_res = (y_in_dec(c2) << 4) & 0xf0U;
	  };
	  mime_res_bit = mime_res_tbl[mime_res_bit];
	  return(ch);
	} else if (is_space(c1)) {
	    mime_res_bit = 0; /* discard space within base 64	   */
	    continue;
	} else {
	    mime_res_bit = 0; return(c1); 
	}; 
      } else if (((is_mimeb_encoded) || (is_mimeb_strict) 
		 || (is_mimeq_encoded)) 
		 && (!is_in_ucs_ufamnomime || in_mime)) {
			/* *************************************** */
			/* MIME decoder 			   */
			/* *************************************** */
      /* Note: current skf can't handle mime in unic*de
	      properly. Since this case is quite unlikely,
	      skf suppresses mime detection during unic*ode.	   */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," MIME(%d)",decode_poke_buf);
#endif
#ifndef NEW_SOFTWRAP_CODE
	if (!in_mime && (decode_poke_buf == -2)) {
	  if (is_white(c1) && !is_mime_nkfmode) { /* white space   */
	      continue;		/* discard just after q-encode	   */
	  };
	  decode_poke_buf = -1;
	};
#endif
	if (!in_mime && (c1 == '=')) { /* mime introduce	   */
#ifdef NEW_SOFTWRAP_CODE
	  while (!pre_Qempty) {
	      pre_Qenque(c1);
	      c1 = pre_Qdeque;
	      return(c1);
	  };
#endif
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-intro:");
#endif
	  if (flg) {
	    unGETC(c1,f); return(sOCD);
	  };
	  post_Qclear(); post_Qenque(c1); /* re-build postQ	   */
	  if ((c1 = hook_q_getc(f,flg)) == '?') { /* ... 2nd	   */
	    for (c3 = 0; c3 < MIME_CSET_LEN; c3++) {
					/* get header into postQ   */
	      if ((c2 = hook_q_getc(f,flg)) == sOCD) continue;
	      if ((c2 < 0) || (c2 >= A_DEL) 
		    || (valid_name_map[c2] == 0)) {
		break;	/* exit and output postQueue value	   */
	      } else if ((c2 == '\0') || is_space(c2)) {
		if (is_mimeb_strict) break;
		else post_Qunenque();
	      } else if (c2 == '?') {
		break;
	      } else {
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"%c",c2); 
#endif
	      };
	    };
	    if (c2 == '?') {	/* successfully got header	   */
	      codeset_push_for_mime();
	      set_in_endian;	/* set big endian		   */
	      pres = parse_mime_charset(decode_post_queue);
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"/cset:%d",pres); 
#endif
	      if (pres < 0) { 
		if (disp_warn) in_undefined(0,SKF_UND_MIME); 
		continue;	/* Parse failed	   */
	      } else {		/* parse success		   */
		if (mime_ms_compat) {
		  if (pres == codeset_sjis) pres = codeset_cp932w;
		  else if (pres == codeset_euc) pres = codeset_cp51932;
		  else if (pres == codeset_jis) pres = codeset_cp5022x;
		  else;
		} else;
		c3 = hook_q_getc(f,flg); 
		c1 = SKFtolower(c3); 
				/* get decode type		   */
		if (c1 == 'b') { sp_mime = SP_B;
		} else if (c1 == 'q') { sp_mime = SP_Q;
		} else continue;  /* output postQueue as it is	   */
#ifdef SKFDEBUG
		if (is_vv_debug) (void)fputc(c1,stderr); 
#endif
		if ((c1 = hook_q_getc(f,flg)) != '?') continue;	
	      }; 	/* reached decoded part			   */
	      decode_post_mark = decode_post_wptr; 
#ifdef SKFDEBUG
		if (is_vv_debug) (void)fputc('H',stderr); 
#endif
	      if (is_mimeb_strict) {
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"-fchk"); 
#endif
		for (inp = 0; inp < MIME_CHAR_LIMIT; inp++) {
		  if ((c1 = hook_q_getc(f,flg)) < 0) {
		    codeset_recover_from_mime();
		    return(post_Qdeque);
		  } else if ((sp_mime == SP_B) && 
			((y_in_dec(c1) >= 0) || (c1 == '='))) {
		    continue;		/* in for-loop: get next   */
		  } else if ((sp_mime == SP_B) && !is_mimeb_strict
		  	&& is_space(c1)) {
		    continue;
		  } else if ((sp_mime == SP_Q) && (c1 >= 0x21)
			    && (c1 < 0x7f) && (c1 != '?')) {
		      if (c1 == '=') { qmark = 2;
		      } else if (qmark > 0) {
			if (!is_hex_char(c1)) return(post_Qdeque);
			qmark--;
		      } else; 
		  } else if (c1 == '?') {
		      if ((c2 = hook_q_getc(f,flg)) != '=') {
				return(post_Qdeque);
		      } else break;	/* successfully escape	   */
		  } else {
		    codeset_recover_from_mime();
		    return(post_Qdeque);	/* invalid char	   */
		  };
		};
		if (c1 != '?') {	  /* exited by count 	   */
		  in_undefined(0,SKF_UND_MIME);
		  codeset_recover_from_mime(); continue;	
		};
	        post2preQueue(); 
	      } else {
	      	post_Qclear();
	      };		/* tested. It *IS* MIME encoded	   */
	    /* MIME introduce: flush queue at this point if	   */
	    /*  code is not yet determined.			   */
	      in_mime = TRUE; has_mime = TRUE; mime_res_bit = 0;
	      return(sOCD);
	    } else {		/* header parse fail		   */
	      continue;	/* output postQueue as it is	   */
	    };
	  } else {		/* not '=?' sequence. output as is */
	      if (is_lineend(c1) && (is_mimeq_encoded)) {
#ifdef SKFDEBUG
		  if (is_vv_debug) fprintf(stderr,"-Q-softwrap ");
#endif
		  post_Qclear();
	          if ((c3 = hook_getc(f,flg)) == sEOF) continue;
		  /* discard CRLF and LFCR */
		  if ((c1 == A_LF) && (c3 == A_CR)) continue;
		  if ((c3 == A_LF) && (c1 == A_CR)) continue;
		  /* other case: restart at c3			   */
		  pre_Qenque(c3);
	      };
	      continue;
	  };
	} else if (in_mime && is_b_encode) {
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-B");
#endif
	  ch = 0x20;			/* guarding		   */
	  if (c1 == '?') {		/* escape from mime	   */
		/* restore old-encoding before escaping mime	   */
	    if ((c2 = hook_q_getc(f,flg)) == '=') post_Qclear();
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x",c2);
#endif
	    in_mime = 0;
	    codeset_recover_from_mime(); clear_after_mime();
	    post_Qclear(); pre_Qclear();
#ifdef NEW_SOFTWRAP_CODE
	    mime_mark = 0;
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr," peeking:");
#endif
	    while ((c3 = unhook_getc(f,flg)) != sEOF) {
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr,"%02x,",c3);
#endif
/*
 * mime_mark: 0 - 2(CR) - 1(LF) -3(CRLF) - 4(white after LE)
 */
	    	if (pre_Qfull()) {
		    pre_Qenque(c3);
		    break;	/* real out. output as it is.	   */
	    	} else if (is_white(c3)) {
		    if ((mime_mark >= 1) && (mime_mark <= 3)) {
		    	pre_Qenque(c3);	/* likely continuation	   */
			mime_mark = 4;	/* let's confirm	   */
		    } else pre_Qenque(c3);
		} else if ((c3 == EQU_DEL) && 
			((mime_mark == 4) || (mime_mark == 0))) {
		    pre_Qclear();	/* is continuation line.   */
		    pre_Qenque(c3);
		    break;
		} else if ((c3 == A_LF) && 
			((mime_mark == 2) || (mime_mark == 0))) {
		    if (mime_mark == 0) {
#if defined(DELETESPACEONLE)
			if (is_skf196mime && !is_nkf_compat) {
			    pre_Qclear();
			} else;
#endif
		    	mime_mark = 1;
		    } else mime_mark = 3;
		    pre_Qenque(c3);
		    set_detect_lf;
		} else if ((c3 == A_CR) && 
			((mime_mark == 1) || (mime_mark == 0))) {
		    if (mime_mark == 0) {
#if defined(DELETESPACEONLE)
			if (is_skf196mime && !is_nkf_compat) {
			    pre_Qclear();
			} else;
#endif
		    	mime_mark = 2;
		    } else mime_mark = 3;
		    pre_Qenque(c3);
		    set_first_detect_cr; set_detect_cr;
		} else {
		    pre_Qenque(c3);
		    break;	/* real out. output as it is.	   */
		};
	    };
#else
	    decode_poke_buf = -2;	/* end indicator on	   */
#endif
	    return(sOCD);
	  } else if (c1 == '=') {  /* padding or soft break	   */
		    /* should be here when mime_res_bit = 2 or 4,  */
		    /* but guarded for broken case		   */
	    mime_res_bit = mime_res_tbl[mime_res_bit];
	    mime_y_res = 0;
	    continue;
	  } else {			/* normal mime decode	   */
	    if ((c3 = y_in_dec(c1)) < 0) {	
			/* perform error recovery after output	   */
			/* Note: MIME spc. tells illegal char.	   */
			/* should be skipped, but skf reads ESC.   */
	      post_Qenque(c1); 
	      if (c1 == A_ESC) {
	      	  return(c1);
	      } else if (!(is_mimeb_strict)) {
	      	  continue;
	      } else {
		  in_undefined(0,SKF_MIME_ERR);
		  codeset_recover_from_mime(); clear_after_mime();
		  post_Qclear(); pre_Qclear(); in_mime = 0; 
		  return(sOCD);
	      };
	    };
	    if (mime_res_bit == 2) {
	      ch = mime_y_res + c3; mime_res_bit = 0; mime_y_res = 0;
	    } else if (mime_res_bit == 4) {
	      ch = mime_y_res + ((c3 >> 2) & 0xfU);
	      mime_res_bit = 2; mime_y_res = (c3 << 6) & 0xc0U;
	    } else {		/* mime_res_bit == 0	   */
	      post_Qenque(c1); 
	      if ((c2 = hook_q_getc(f,flg)) < 0) {
		in_mime = 0; codeset_recover_from_mime();
		return(sOCD);
	      };
#ifdef SKFDEBUG
	      if (is_vv_debug) fprintf(stderr,",%02x",c2);
#endif
	      if ((c4 = y_in_dec(c2)) < 0) {
		if (!(is_mimeb_strict) && is_space(c2)) {
		  in_undefined(0,SKF_MIME_ERR);
		  if (((c2 = hook_getc(f,flg)) == sEOF) 
		    || (y_in_dec(c2) < 0)) {
		    in_mime = 0; codeset_recover_from_mime();
		    return(sOCD);
		  };
		} else {
		  in_mime = 0; codeset_recover_from_mime();
		  return(sOCD);
		};
	      };
	      ch = (c3 << 2) + ((c4 >> 4) & 0x3U);
	      post_Qclear(); mime_res_bit = 4; 
	      mime_y_res = (c4 << 4) & 0xf0U;
	    };
	  };
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-%02x(%d,%02x)",
				ch,mime_res_bit,mime_y_res);
#endif
	  return(ch);
        } else if (in_mime && is_q_encode) { /* mime q encoding	   */
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-Q");
#endif
#ifndef NEW_SOFTWRAP_CODE
	  if ((decode_poke_buf == -2) && (c1 == A_LF)) { /* LF	   */
	    internal_detect_crlf = 1;
	    decode_poke_buf = -3; continue;
	  } else if ((decode_poke_buf == -4) && (c1 == A_LF)) {
	    internal_detect_crlf = 2;
	    decode_poke_buf = -5; continue;
	  } else if ((decode_poke_buf == -2) && (c1 == A_CR)) {
	    internal_detect_crlf = 3;
	    decode_poke_buf = -4; continue;
	  } else if ((decode_poke_buf == -3) && (c1 == A_CR)) {
	    internal_detect_crlf = 4;
	    decode_poke_buf = -5; continue;
	  } else if ((decode_poke_buf <= -2) && is_white(c1)) {
	    if (decode_poke_buf < -2) {
	    	decode_poke_buf = -5;
		decode_poke_buf--;
	    };
	    continue;		/* discard after soft break	   */
	  } else if (decode_poke_buf < -2) { /* not white	   */
	    if (internal_detect_crlf == 2) post_Qenque(A_CR);
	    if ((internal_detect_crlf == 1) ||
	    	(internal_detect_crlf == 4) ||
	    	(internal_detect_crlf == 2)) post_Qenque(A_LF);
	    if ((internal_detect_crlf == 3) ||
	    	(internal_detect_crlf == 4)) post_Qenque(A_CR);
	    if (decode_poke_buf < -5) 
		for (i=decode_poke_buf;i<-5;i++) post_Qenque(A_SP);
	  };
#endif
	  decode_poke_buf = -1;		/* throw anyway		   */
	  if (c1 == '?') {		/* escape from mime	   */
	    if ((c2 = hook_getc(f,flg)) != '=') {
	    	post_Qenque(c1); pre_Qenque(c2);
	    	continue;
	    };
	    /* restore old-encoding before escaping mime	   */
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x-Fin",c2);
#endif
	    codeset_recover_from_mime();
	    clear_after_mime();
	    post_Qclear(); pre_Qclear(); in_mime = 0; 
#ifdef NEW_SOFTWRAP_CODE
	    mime_mark = 0;
	    while ((c3 = unhook_getc(f,flg)) != sEOF) {
	    	if (pre_Qfull()) {
		    pre_Qenque(c3);
		    break;	/* real out. output as it is.	   */
	    	} else if (is_white(c3)) {
		    if ((mime_mark >= 1) && (mime_mark <= 3)) {
		    	pre_Qenque(c3);	/* likely continuation	   */
			mime_mark = 4;	/* let's confirm	   */
		    } else pre_Qenque(c3);
		} else if ((c3 == EQU_DEL) && 
			((mime_mark == 4) || (mime_mark == 0))) {
		    pre_Qclear();	/* is continuation line.   */
		    pre_Qenque(c3);
		    break;
		} else if ((c3 == A_LF) && 
			((mime_mark == 2) || (mime_mark == 0))) {
		    if (mime_mark == 0) {
#if defined(DELETESPACEONLE) && defined(SKF_COMPATMODE)
			if (is_skf196mime && !is_nkf_compat) {
			    pre_Qclear();
			} else;
#endif
		    	mime_mark = 1;
		    } else mime_mark = 3;
		    pre_Qenque(c3);
		    set_detect_lf;
		} else if ((c3 == A_CR) && 
			((mime_mark == 1) || (mime_mark == 0))) {
		    if (mime_mark == 0) {
#if defined(DELETESPACEONLE) && defined(SKF_COMPATMODE)
			if (is_skf196mime && !is_nkf_compat) {
			    pre_Qclear();
			} else;
#endif
		    	mime_mark = 2;
		    } else mime_mark = 3;
		    pre_Qenque(c3);
		    set_first_detect_cr; set_detect_cr;
		} else {
		    pre_Qenque(c3);
		    break;	/* real out. output as it is.	   */
		};
	    };
#else
	    decode_poke_buf = -2;	/* end indicator on	   */
#endif
	    return(sOCD);
	  } else if (c1 == '_') { 	/* _ in Q-encode is space  */
	    return(' ');
	  } else if (c1 == '=') {	/* padding before escape   */
	    post_Qenque(c1);
	    if ((c2 = hook_q_getc(f,flg)) < 0) continue;
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x",c2);
#endif
	/* Soft line break detection				   */
	    if (c2 == 0x0d) {		/* CR			   */
	      post_Qclear(); decode_poke_buf = -4; continue;
	    } else if (c2 == 0x0a) {	/* LF			   */
	/* only CR is allowed in RFC 2045, but we take both here.  */
	      post_Qclear(); decode_poke_buf = -3; continue;
	    } else if (is_white(c2)) {
	      post_Qclear(); decode_poke_buf = -2; continue;
	/* just discard these spaces. Spec says we have to skip
	  any number of spaces, but skf throws only at most
	  one space for implementation reason.   */
	    };
	    if ((c3 = hook_q_getc(f,flg)) == sEOF) continue;
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,",%02x",c3);
#endif
	    if (is_hex_char(c3) && is_hex_char(c2)) {
	      post_Qclear();
	      return((skf_hex(c2) << 4) + skf_hex(c3));
	    } else {
	      in_mime = 0; codeset_recover_from_mime();
	      clear_after_mime();
	      post_Qclear(); pre_Qclear();
	      return(sOCD);
	    };
	  } else if ((c1 < A_SP) || (c1 >= A_DEL)) {
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr,"%x-Broke",c1);
#endif
	    in_mime = 0; codeset_recover_from_mime();
	    clear_after_mime();
	    post_Qclear(); pre_Qclear();
	    post_Qenque(c1);
	    return(sOCD);
	  } else return(c1);	/* normal ascii			   */
	} else {
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr," U_MIME");
#endif
	  return(c1);
	};
      } else if ((is_rfc2231_encoded) && (c1 == '*') && (!in_rfc2231)) { 
			/* *************************************** */
			/* RFC2231 (Mime parameter value encode)   */
			/* *************************************** */
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," RFC2231-intro");
#endif
	if (flg) {
	    unGETC(c1,f); return(sOCD);
	};
	post_Qclear(); post_Qenque(c1); /* re-build postQ	   */
	isdigited = 0; 
	for (;;) {
	    c1 = hook_q_getc(f,flg); 
	    if (is_digit(c1)) { isdigited = 1;
	    } else if ((c1 == '*') && (isdigited > 0)) {
		;	/* skip */
	    } else break;
	};
	if (c1 != '=') continue;	/* exit without '='	   */
	postq_mark = get_postq_mark();
	for (c3 = 0; c3 < MIME_CSET_LEN; c3++) {
				    /* get header into postQ   */
	  if ((c2 = hook_q_getc(f,flg)) == sOCD) continue;
	  mime_cset_buf[c3+2] = c2; mime_cset_buf[c3+3] = 0;
	  if ((c2 == sEOF) || (c2 >= A_DEL) || (c2 < A_SP)
	      || (c2 == 0x27) || (c2 == ';')) {
	    break;	/* exit and output postQueue value	   */
	  };
	};
	if (c2 == 0x27) {	/* successfully got header	   */
	  codeset_push_for_mime();
	  set_in_endian;	/* set big endian		   */
	  pres = parse_mime_charset(mime_cset_buf);
	  if (pres < 0) {
	    if (disp_warn) in_undefined(0,SKF_UND_MIME); 
	    continue;		/* Parse failed			   */
	  } else {		/* parse success		   */
	    if (mime_ms_compat) {
	      if (pres == codeset_sjis) pres = codeset_cp932w;
	      else if (pres == codeset_euc) pres = codeset_cp51932;
	      else if (pres == codeset_jis) pres = codeset_cp5022x;
	      else;
	    } else;
	    c3 = hook_q_getc(f,flg); /* get language		   */
	    if (is_alpha(c3)) {	/* if lang is specified	   */
	      c1 = hook_q_getc(f,flg); /* get language	   */
	      if (!is_alpha(c1)) continue;
#ifdef SKFDEBUG
	      if (is_vv_debug) fprintf(stderr," lang:%c%c ",c3,c1); 
#endif
	      c3 = hook_q_getc(f,flg); /* get terminator	   */
	      if (c3 != 0x27) continue;
	    } else if (c3 == 0x27) {  /* lang is omitted	   */
	      ;
	    } else continue;		/* other case: failed	   */
	  }; 		/* reached decoded part			   */
	  post_Qclear(); set_postq_mark(postq_mark);
	  /* MIME introduce: flush queue at this point if	   */
	  /*  code is not yet determined.			   */
	  in_rfc2231 = TRUE; mime_res_bit = 0;
	  return(sOCD);
	} else if (c2 == ';') {	/* non-encoded case		   */
	  delete_postQ_tail(); 
	  continue;
	} else {		/* header parse fail		   */
	  continue;		/* output postQueue as it is	   */
	}; 
#ifdef ACE_SUPPORT
      } else if (is_puny_encoded) {	/* Puny/RACE code	   */
			/* *************************************** */
			/* ACE (Punycode/RACE) decoder		   */
			/* *************************************** */
	if (in_ace) {
	    codeset_recover_from_mime(); in_ace = FALSE;
	    clear_after_mime();
	    post_Qenque(c1); return(sOCD);
	} else if ((SKFtolower(c1) == PUNY_PRFX1)
		 || (SKFtolower(c1) == RACE_PRFX1)) { /* RFC3490   */
	    if (flg) {
		unGETC(c1,f); return(sOCD);
	    };
	    post_Qclear(); post_Qenque(c1);
	    if ((c2 = hook_q_getc(f,flg)) == sEOF) continue;
	    if (((SKFtolower(c1) != PUNY_PRFX1) 
			|| (SKFtolower(c2) != PUNY_PRFX2))
		&& ((SKFtolower(c1) != RACE_PRFX1)
			|| (SKFtolower(c2) != RACE_PRFX2)))
		continue;	/* not xn(PUNY) nor bq(RACE)	   */
	    if ((c3 = hook_q_getc(f,flg)) != '-') continue;
	    if ((c3 = hook_q_getc(f,flg)) != '-') continue;
	    /* ACE label is confirmed now. go to buffering	   */
	    w = 0;
	    while ((c3 = hook_getc(f,flg)) != sEOF) {
		if (is_label_delim(c3)) break;
#ifdef SKFDEBUG
		if (is_vvv_debug) fprintf(stderr,"ace_in: %x ",c3);
#endif
		puny_in_buf[w++] = c3;
		post_Qenque(c3);
		if (w > PUNY_BUFLEN) break;
	    };
	    outchar = PUNY_BUFLEN;

	    if ((SKFtolower(c1) == PUNY_PRFX1) &&  /* PUNY	   */
	    	((pres = punycode_decode(w,puny_in_buf,&(outchar),
		puny_out_buf)) >= 0)) {/* throw to Qbuffer */
	    } else if ((SKFtolower(c1) == RACE_PRFX1) &&  /* RACE  */
	    	((pres = racecode_decode(w,puny_in_buf,&(outchar),
		puny_out_buf)) >= 0)) {/* throw to Qbuffer */
	    } else {	
		in_undefined((0x4000-pres),SKF_DECODERR);
		continue;
	    };

	    post_Qclear();
	    for (k=0; k < outchar; k++) {
		rcod = puny_out_buf[k];
		post_Qenque((rcod >> 8) & LBMSK);
		post_Qenque(rcod & LBMSK);
	    };
	    pre_Qenque(c3);  /* re-supply delimiter	  */
	    codeset_push_for_mime();
	    in_codeset = codeset_utf16le;
	    set_dummy_detect;
	    set_in_endian;		/* set big endian	   */
	    in_ace = TRUE;
	    return(sOCD);	/* code-change: flush buffers	   */
	} else {
	    return(c1);
	};
#endif
      } else if (is_hex_qencode && ((c1 == '=') || expect_discard)) {
			/* *************************************** */
			/* Q-encode decoder 			   */
			/* *************************************** */
      	  if (expect_discard && (is_space(c1) || is_lineend(c1))) {
	      continue;
	  };
	  post_Qenque(c1);
	  if (expect_discard && (c1 != '=')) {
	      expect_discard = FALSE; continue;
	  };
	  if ((c3 = hook_getc(f,flg)) == sEOF) continue;
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-%02x(%d:HEX)",
		c3,uri_esc_hook);
#endif
	  if (pre_Qofull()) {
		skferr(SKF_DECERRDUMP2,decode_pre_wptr,decode_pre_rptr);
	  };
	  if (!is_hex_char(c3)) {
	      if (is_space(c3) || is_lineend(c3)) {
	      	  post_Qunenque();
		  expect_discard = TRUE; continue;
	      } else {
		  pre_Qenque(c3); continue;   /* broken		   */
	      };
	  };
	  if ((c2 = hook_q_getc(f,flg)) == sEOF) continue;
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr,"-%02x ",c2);
#endif
	  pre_Qenque(c3); pre_Qenque(c2);
	  if (!is_hex_char(c2)) continue;
	  ch = (skf_hex(c3) << 4) + skf_hex(c2);
	  expect_discard = FALSE;
	  post_Qclear(); pre_Qclear(); return(ch);
#if	defined(ROT_SUPPORT) && defined(NEW_ROT_CODE)
      } else if (is_rot_encoded) {
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr," ROT(%lx,%x,%x,%x)",
		encode_cap,in_mime,pre_Qdepth(),post_Qdepth());
#endif
	if (((low_dbyte && (c1 < 0x80) && (c1 > A_SP)) ||
	    (is_multibyte(up_dbyte) && (c1 >= 0x80))) && !rot_esc_hook) {
	    post_Qenque(c1); 
	    if ((c2 = hook_q_getc(f,flg)) == sEOF) continue;
	    post_Qunenque(); /* c2 */
	    post_Qunenque(); /* c1 */
	    if (is_msfam(i_codeset[in_codeset].encode)) {
	    	if ((c2 <= 0xfc) && (c2 >= 0x40) && (c2 != 0x7f)) {
		    c1 = c1 + c1 - ((c1 <= 0x9f) ? 0xe1 : 0x161);
		    if (c2 < 0x9f) {
			c2 -= ((c2 > A_DEL) ? 0x20 : 0x1f);
		    } else { 
			c2 -= 0x7e; c1++;
		    };
		} else {    /* X-0213 pl.2 Maybe codeset to be buried */
		    if (c1 <= 0xf4) {
			c3 = x213_sjis_map
			    [((c1 - 0xf0) << 1)+ ((c2<0x9f) ? 0 : 1)];
		    } else if (c1 <= 0xfc) {
			c3 = (c1 << 1) - 0x17b;
		    } else {
			in_undefined((c1 << 8)+c1, SKF_OUTTABLE);
			c1 = 0; continue;
		    };
		    if (c2 < 0x9f) {
			c2 -= ((c2 > A_DEL) ? 0x20 : 0x1f);
		    } else {
			c2 -= 0x7e; if (c1>=0xf4) c3++;
		    };
		    c1 = c3;
		};
#ifdef SKFDEBUG
		if (is_vv_debug) fprintf(stderr," [%x,%x]",c1,c2);
#endif
	    } else if (is_euc(i_codeset[in_codeset].encode)) {
	    	if ((c1 == A_SS2) || (c1 == A_SS3)) {
		    pre_Qenque(c2); return(c1);
		} else;
	    } else;
	    if (is_jis(i_codeset[in_codeset].encode) ||
	    	is_msfam(i_codeset[in_codeset].encode) ||
	    	is_euc(i_codeset[in_codeset].encode)) {
		c3 = skf_rot47conv_d(c1 & 0x7fU);
		c4 = skf_rot47conv_d(c2 & 0x7fU);
		c1 = (c3 | (c1 & 0x80U));
		c2 = (c4 | (c2 & 0x80U));
	    } else {
	    	post_Qenque(c2);     /* can't handle		   */
	    };
	    if (is_msfam(i_codeset[in_codeset].encode)) {
	    	c1 &= 0x7f; c2 &= 0x7f;
		c3 = ((c1 - 1) >> 1) + ((c1 <= 0x5e) ? 0x71 : 0xb1); 
		c2 = c2 + ((c1 & 1) ? ((c2 < 0x60) ? 0x1f : 0x20) : 0x7e);
		c1 = c3;
	    };
	    post_Qenque(c2);
	    return(c1);
	} else {	/* single byte				  */
	    if (rot_esc_hook == 0) c1 = skf_rot13conv_d(c1);
	    if (c1 == A_ESC) rot_esc_hook = 1;
	    else if ((c1 >= 0x40) && (c1 < 0x7f)) rot_esc_hook = 0;
	};
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr," -(%x,%x)",c1,c2);
#endif
	return(c1);
#endif
      } else {
        if ((c1 == A_ESC) 
		&& (is_hex_encoded || is_hex_uri || is_rfc2231_encoded)) {
	    if (uri_esc_hook == 0) uri_esc_hook = 1;
	    else uri_esc_hook = 2;
	} else if ((uri_esc_hook >= 2) && (c1 == 0x28)) {
		uri_esc_hook = 0;
	};
      };
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr," NONCODE(%lx,%x,%x,%x)",
		encode_cap,sp_mime,in_mime,uri_esc_hook);
#endif
      return(c1);
    };
    return(sEOF);
}

/* --------------------------------------------------------------- */
/* unic*de code decorder : unified transfer format decoder	   */
/* --------------------------------------------------------------- */

#define	GETC_add(c1,c2,f) \
	{if ((c1 = vGETC(f)) == sEOF) { \
		if (c2 >= 0) in_undefined(c2,SKF_IUNDEF); \
		return(c1); } }

/*@-nullpass@*/ /* this should be OK */
skf_ucode u_dec_hook(f,cod)
skfFILE	*f;
int cod;
{
    int c1, c2 = 0, c3 = 0, c4;
    int	d1,d2,d3;
    skf_ucode ch = 0;
    int	cm;

    while (TRUE) {
	if ((c1 = vGETC(f)) < 0) return(c1);
#ifdef SKFDEBUG
	if (is_vv_debug) fprintf(stderr,"\nudh_raw: %02x",c1);
#endif
	if (is_cod_utf7(cod)) {	/* Unic*de UTF-7		   */
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr," U7");
#endif
	    c1 &= 0x7fU;	/* reset bit8			   */
	    if (utf7in) {		/* in base64		   */
		if ((c1 == '-') ||
		    (y_in_dec(c1) < 0)) {  /* escape from base64   */
		    if (c3 != 0) in_undefined(c3,SKF_IUNDEF);
		    if (c2 != 0) in_undefined(c2,SKF_IUNDEF);
		    c2 = 0; c3 = 0; res_bit = 0; /* it should be 0  */
		    utf7in = FALSE; continue;
		} else if (c2 == 0) {
		    c2 = c1; continue;
		} else if ((c3 == 0) && (res_bit != 4)) {
#ifdef SKFDEBUG
		    if (is_vvv_debug) fprintf(stderr," y_in: get_c3");
#endif
		    c3 = c2; c2 = c1; continue;
		};
		if (res_bit == 4) {
		    d1 = y_in_dec(c2); d2 = y_in_dec(c1); 
		    ch = y_res + ((d1 << 6) & Y1AMSK)
			    + (d2 & Y2MSK);
		    res_bit = 0; y_res = 0;
		} else {
		    d1 = y_in_dec(c3); d2 = y_in_dec(c2);
		    d3 = y_in_dec(c1); 
		    if (res_bit == 2) {
			ch = y_res + ((d1 << 8) & Y0MSK)
			    + ((d2 << 2) & Y2AMSK) + ((d3 >> 4) & Y3MSK);
			res_bit = 4; y_res = (d3 & Y3AMSK) << 12;
		    } else {
			ch = ((d1 << 10) & Y0AMSK)
			    + ((d2 << 4) & Y1MSK) + ((d3 >> 2) & Y3AMSK);
			res_bit = 2; y_res = (d3 & Y3MSK) << 14;
		    };
		};
#ifdef SKFDEBUG
		if (is_vvv_debug) fprintf(stderr,
			" y_in: %04x(%01d,%04x)",ch,res_bit,y_res);
#endif
		c2 = 0; c3 = 0; c1 = 0; 
	    } else if (c1 == '+') {  /* UTF7 escape character	   */
		utf7in = TRUE; 
		c2 = 0; c3 = 0; res_bit = 0; y_res = 0;
		continue;
	    } else {		/* within normal ascii's	   */
		ch = c1;
	    };
	} else if (is_cod_utf8(cod)) { /* Unic*de UTF-8		   */
#ifdef SKFDEBUG
	  if (is_vv_debug) fprintf(stderr," U8");
#endif
	    if (c1 <= A_DEL) {		/* 0x00 - 0x7f		   */
		return(c1);
	    } else if ((cm = (c1 & ZF2MSK)) < ZF23VAL) {
		in_undefined(c1,SKF_IBROKEN);
		continue;
	    } else if (cm == ZF23VAL) {	/* 0x0080 - 0x07ff	   */
		if ((c1 >= 0xc2) || non_strict_utf8) {
		    GETC_add(c2,c1,f);
#ifdef SKFDEBUG
		    if (is_vvv_debug) fprintf(stderr," z_in_raw: %02x",c2);
#endif
		    ch = ((c1 & Z23MSK) << 6) | (c2 & Z1MSK);
		} else {
		    in_undefined(c1,SKF_IBROKEN);
		    continue;
		};
	    } else if ((c1 & ZF3MSK) == ZF3VAL) { /* 0x800 - 0xffff */
		GETC_add(c2,c1,f);
		ch = ((c1 & Z3MSK) << 12) | ((c2 & Z1MSK) << 6);
		if ((c3 = vGETC(f)) == sEOF) {
		    in_undefined(ch,SKF_UNEXPEOF);
		    return(sEOF);
		};
#ifdef SKFDEBUG
		if (is_vvv_debug)
		    fprintf(stderr," z_in_raw: %02x,%02x",c2,c3);
#endif
		if ((c1 == 0xe0) && (c2 < 0xa0) && !non_strict_utf8) {
		    in_undefined(ch,SKF_IBROKEN);
		    continue;
		};
		ch |= (c3 & Z1MSK);
		if (((ch >= 0xd800) && (ch < 0xe000)) && !enable_cesu) {
		    in_undefined(ch,SKF_IBROKEN);
		    continue;
		};
	    } else if ((c1 & ZF4MSK) == ZF4VAL) {
		GETC_add(c2,c1,f);
		if ((c3 = vGETC(f)) == sEOF) {
		    in_undefined((c1 << 8) + c2,SKF_UNEXPEOF);
		    return(sEOF);
		};
		if ((c4 = vGETC(f)) == sEOF) {
		    in_undefined((c1 << 16) + (c2 << 8) + c3,
			SKF_UNEXPEOF);
		    return(sEOF);
		};
#ifdef SKFDEBUG
		if (is_vvv_debug) fprintf(stderr,
		    " z_in_raw: %02x,%02x,%02x", c2,c3,c4);
#endif
		if ((c1 == 0xf0) && (c2 < 0x90) && !non_strict_utf8) {
		    in_undefined((c1 << 8) + c2,SKF_IBROKEN);
		    continue;
		};
		ch = ((c1 & Z4MSK) << 18) | ((c2 & Z43MSK) << 12)
		    | ((c3 & Z1MSK) << 6) | (c4 & Z1MSK);
	    } else ch = 0;
	} else if (is_cod_utf32(cod)) {	/* utf-32 (a.k.a. UCS4)	   */
#ifdef SKFDEBUG
	  if (is_vv_debug) {
	  	fprintf(stderr," U4:%d",in_codeset);
	  } else;
#endif
	  GETC_add(c2,c1,f);
	  GETC_add(c3,c1,f);
	  GETC_add(c4,c1,f);
	  if ((c1 == 0x00) && (c2 == 0x00) && (c3 == 0xfe)
	  	&& (c4 == 0xff)) {
	      if (!(endian_protect)) set_in_endian ;
	      continue;
	  } else if ((c4 == 0x00) && (c3 == 0x00) && (c2 == 0xfe)
	  	&& (c1 == 0xff)) {
	      if (!(endian_protect)) reset_in_endian ;
	      continue;
	  } else;
	  if (in_endian) {
	      ch = ((c1 & 0xffU) << 24) | ((c2 & 0xffU) << 16)
	      		| ((c3 & 0xffU) << 8) | (c4 & 0xffU);
	  } else {
	      ch = ((c4 & 0xffU) << 24) | ((c3 & 0xffU) << 16)
	      		| ((c2 & 0xffU) << 8) | (c1 & 0xffU);
	  };
	} else {		/* utf-16 (a.k.a. UCS2)		   */
#ifdef SKFDEBUG
	    if (is_vv_debug) {
	  	fprintf(stderr," U2:%d",in_codeset);
	    } else;
#endif
	    c2 = c1;
	    GETC_add(c1,c2,f);
	    if ((c2 == 0xfe) && (c1 == 0xff)) { /* endian marker   */
		if (!(endian_protect)) set_in_endian ;
#ifdef SKFDEBUG
		if (in_endian && is_vv_debug)
			fprintf(stderr," set to BIG-endian");
#endif
		continue;
	    };
	    if ((c2 == 0xff) && (c1 == 0xfe)) { /* endian marker   */
		if (!(endian_protect)) reset_in_endian;
#ifdef SKFDEBUG
		if (in_endian && is_vv_debug)
			fprintf(stderr," set to LTL-endian");
#endif
		continue;
	    };
	    if (in_endian) {	/* swap when big endian		   */
		c3 = c1; c1 = c2; c2 = c3;
	    };
	    if ((c1 >= 0xd8) && (c1 <=0xdf)){ /* surrogate pair	   */
		ch = (((c1 << 8) + c2) & U_SRMSK) + 0x40; 
		if ((c4 = vGETC(f)) == sEOF) {
		    in_undefined(ch, SKF_NOSURRG);
		};
		if ((c3 = vGETC(f)) == sEOF) {
		    in_undefined(ch, SKF_NOSURRG);
		};
		if (in_endian) {  /* swap if big endian		   */
		    c1 = c4; c4 = c3; c3 = c1;
		};
		c2 = ((c3 << 8) + c4) & U_SRMSK ;
		ch = (ch << 10) + c2;
	    } else 			/* normal case		   */
		ch = ((c1 & 0xffU) << 8) | (c2 & 0xffU);
	};
	return(ch);
    };
    /*NOTREACHED*/
}

/* --------------------------------------------------------------- */
/* status test							   */
/* --------------------------------------------------------------- */
int is_in_encoded()
{
#ifdef ACE_SUPPORT
    if (in_mime || in_hex || in_ace) return(1);
    else return(0);
#else
    if (in_mime || in_hex) return(1);
    else return(0);
#endif
}

/* --------------------------------------------------------------- */
/* clear all stats at preconvert entry				   */
/* --------------------------------------------------------------- */
void init_all_stats()
{
    clear_after_mime();
    in_mime = 0; in_rfc2231 = 0;
    post_Qclear(); pre_Qclear(); 
}

void clear_after_mime()
{
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," ... clearing stats\n");
#endif
    res_all_shift;

/* clear all utf-7 decode related variables */
    utf7in = 0; res_bit = 0; y_res = 0;

/* clear all mime status */
    mime_res_bit = 0; mime_y_res = 0; sp_mime = 0; 
}
/* --------------------------------------------------------------- */
/* paraphrase_arib_macro:					   */
/* --------------------------------------------------------------- */
/* TODO: should check macro invoke doesn't contain some chars.	   */
/* --------------------------------------------------------------- */
#define is_macro_low(x) ((x > A_SP) && (x < A_DEL))
#define is_macro_up(x)  ((x > A_KSP) && (x < 0xff))

int paraphrase_arib_macro(ch)
int ch;
{
    int i,sy,sz;
    int *macstr;
    int g0mac = FALSE;
    int g1mac = FALSE;
    int g2mac = FALSE;
    int g3mac = FALSE;
    int glmac = FALSE;
    int gumac = FALSE;
    int inssg2 = FALSE;
    int inssg3 = FALSE;

#ifdef SKFDEBUG
    if (is_vv_debug) {
    	fprintf(stderr,"ARIB_MACRO: ch-%c(",ch);
    } else;
#endif
    if ((ch <= A_SP) || (ch > A_DEL)) return(ch);
    if (arib_macro_tbl == NULL) return(ch);
    if ((macstr = arib_macro_tbl[ch - 0x21]) == NULL) {
	/* no macro defined for this character			   */
	return(ch);
    } else {
	/* has macro definition and <128 length			   */
        if (is_charset_macro(g0_table_mod)) g0mac = TRUE;
        if (is_charset_macro(g1_table_mod)) g1mac = TRUE;
        if (is_charset_macro(g2_table_mod)) g2mac = TRUE;
        if (is_charset_macro(g3_table_mod)) g3mac = TRUE;
        if (is_charset_macro(low_table_mod)) glmac = TRUE;
        if (is_charset_macro(up_table_mod)) gumac = TRUE;

    	for (i=0;(i<128) && (macstr[i] != '\0');i++) {
	    sy = macstr[i];
#ifdef SKFDEBUG
	    if (is_vv_debug) {
		fprintf(stderr,"%02x,",sy);
	    } else;
#endif
	    if (inssg2) {
	    	/* actually not correct in iso-2022 aspect */
	        if ((g2mac) && (sy > A_SP) && (sy < A_DEL)) 
			return(0);	/* terminate		   */
		inssg2 = FALSE;
		inssg3 = FALSE;
	    } else if (inssg3) {
	    	/* actually not correct in iso-2022 aspect */
	        if ((g3mac) && (sy > A_SP) && (sy < A_DEL)) 
			break;		/* terminate		   */
		inssg3 = FALSE;
	    } else if (is_macro_low(sy)) {
	    	if (sy == A_SO) {
		    glmac = g1mac; continue;
		} else if (sy == A_SI) {
		    glmac = g0mac; continue;
		} else if (sy == A_ESC) {
		    sz = macstr[++i];
		    if (sz == 0x6e) { /* LS2 */
		    	glmac = g2mac; continue;
		    } else if (sz == 0x6f) { /* LS3 */
		    	glmac = g3mac; continue;
		    } else if (sz == 0x7e) { /* LS1R */
		    	gumac = g1mac; continue;
		    } else if (sz == 0x7d) { /* LS2R */
		    	gumac = g2mac; continue;
		    } else if (sz == 0x7c) { /* LS3R */
		    	gumac = g3mac; continue;
		    } else;
		} else if (glmac) {
		    ;	/* pass them */
		} else; /* pass others */
	    } else if (is_macro_up(sy)) {
	        if (sy == A_SS2) {
		    inssg2 = TRUE; continue;
		} else if (sy == A_SS3) {
		    inssg3 = TRUE; continue;
		} else if (gumac) {
		    ;	/* pass them */
		} else;
	    } else; /* not within GL nor GR - pass as it is */
	    enque(macstr[i]);
	};
    };
#ifdef SKFDEBUG
    if (is_vv_debug) {
    	fprintf(stderr,")\n");
    } else;
#endif
    return(0);
}
