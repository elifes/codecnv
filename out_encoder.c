/* *******************************************************************
** Copyright (c) 2008-2015 Seiji Kaneko. All rights reserved.
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
    out_encoder.c	output-side converter encode routines
    $Id: out_encoder.c,v 1.27 2017/01/05 15:05:48 seiji Exp seiji $

    Punycode encode part is copyrighted by The Internet society.
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>

#include "skf.h"
#include "skf_fileio.h"
#include "oconv.h"
#include "convert.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#define BUGGO	TRUE

/* --------------------------------------------------------------- */
/* code decorder : generic decode preprocessor			   */
/* --------------------------------------------------------------- */
#define O_PREQDEPTH	256	/* must be power of 2		   */
#define O_PREQLIMIT	O_PREQDEPTH - 2
#define O_PSTQDEPTH	512	/* must be power of 2		   */

/* o_encode_stat: '1' indicate that mime encoding being proceed.   */
	int	o_encode_stat = 0; /* encode state		   */
/* o_encode_pend: Encoding temporally suspend. used when
 * no_early_mime_out is disabled.
 */
static 	int	o_encode_pend = 0;
/* o_encode_lc: character count which o_c_encode has received
 * 		plus header/tail length.
 * o_encode_lm: character count which has been thrown to stdout	   */
static 	int	o_encode_lc = 0;   /* line length counter	   */
static 	int	o_encode_lm = 0;   /* line length counter	   */
#ifdef SUPERFOLD
static	int	o_encode_lc_save = 0;   /* line pos just before MIME */
#endif
static 	skf_ucode o_encode_max = 0;  /* line character upper limit */
static	int	pre_ch = 0;	   /* previous character	   */

static int b64_encoder_table[64] = {
  0x41,0x42,0x43,0x44, 0x45,0x46,0x47,0x48,
  0x49,0x4a,0x4b,0x4c, 0x4d,0x4e,0x4f,0x50,
  0x51,0x52,0x53,0x54, 0x55,0x56,0x57,0x58,
  0x59,0x5a,0x61,0x62, 0x63,0x64,0x65,0x66,
  0x67,0x68,0x69,0x6a, 0x6b,0x6c,0x6d,0x6e,
  0x6f,0x70,0x71,0x72, 0x73,0x74,0x75,0x76,
  0x77,0x78,0x79,0x7a, 0x30,0x31,0x32,0x33,
  0x34,0x35,0x36,0x37, 0x38,0x39,0x2b,0x2f
};

static int b64_encoder_res = 0;	/* encode residuals		  */
static int b64_encoder_cnt = 0;	/* encode counters		  */

static void mime_header_gen P_((int));
static void mime_tail_gen P_((int));
static void show_encode_codeset P_((int));

static void char2oct P_((int));
static void char2hex P_((int));

static void output_to_mime P_((int,int));
static void queue_to_mime P_((int));

static void SKFrCRLF P_(());

static void base64_enc P_((skf_ucode,int));
static void mimeq_enc P_((int,int));

/* --------------------------------------------------------------- */
static skf_ucode encode_pre_queue[O_PREQDEPTH];
		/* predecode temporal queue: already cheched	   */
		/* Note: this is circular buffer		   */
static int encode_post_queue[O_PSTQDEPTH];
		/* postdecode temporal queue: unchecked		   */
#ifdef ACE_SUPPORT
static int encode_post_wptr = 0;   /* puny output length	   */
#endif

static int	enc_pre_q_w_ptr = 0; /* pre-queue write pointer	   */
static int	enc_pre_q_r_ptr = 0; /* pre-queue read pointer	   */

static int	puny_maxcp = 0;	     /* used in o_p_encode	   */

#define hex_beescape(x) ((x < '0') || (x > 'z') \
	|| ((x > '9') && (x < 'A')) || ((x > 'Z') && (x < 'a')))
/* --------------------------------------------------------------- */
/* mime fold limits.						   */
/*  mime_start_limit: a point which mime should start		   */
/*  mime_tail_limit: mime length limit(72) - 2 ("?=")		   */
/* --------------------------------------------------------------- */
static int	mime_start_limit;
static int	mime_tail_limit;
/* maximum character length to return to latin			   */
static int	mime_max_chlen = 0;

/* --------------------------------------------------------------- */
void mime_limit_set()
{
    /* we couldn't process if limit is too low.			   */
    if (mime_fold_llimit < 32) mime_fold_llimit = 32;

    if (is_o_encode_mimeb(o_encode)) {
    /* MIME length trimming for "=?(encode)?B?(body)?="	*/
	mime_tail_limit = mime_fold_llimit - 2;	/* "?="		   */
	mime_start_limit = mime_fold_llimit - 9 -1
			- (int)(skf_strlen(i_codeset[out_codeset].cname,32));
    } else if (is_o_encode_mimeq(o_encode)) {
	mime_tail_limit = mime_fold_llimit - 2;
	mime_start_limit = mime_fold_llimit - 8 
			- (int)(skf_strlen(i_codeset[out_codeset].cname,32));
    } else if (is_o_encode_q(o_encode) || is_o_encode_b64(o_encode)) {
	mime_tail_limit = 70;
	mime_start_limit = 69;
    } else {
    	mime_tail_limit = 0;
	mime_start_limit = 0;
    };

    if (mime_tail_limit < 32) mime_tail_limit = 32;

    o_encode_lc = 0;
    o_encode_lm = 0;

    if (is_gb18030(conv_cap) || is_ucs_utf8(conv_cap)) {
    	/* maximum 4 byte length */
    	mime_max_chlen = 0;
    } else if (is_jis(conv_cap)) {
    	mime_max_chlen = 3;
    } else if (is_euc(conv_cap)) {
    	mime_max_chlen = 1;
    } else;

#ifdef SKFDEBUG
  if (is_vvv_debug) {
     fprintf(stderr,"tail_limit: %d  start_limit: %d chlen: %d\n",
     	mime_tail_limit,mime_start_limit,mime_max_chlen);
  };
#endif
}

/* --------------------------------------------------------------- */
void mime_limit_add(offset)
int	offset;
{
    mime_start_limit += offset;
}
/* --------------------------------------------------------------- */
#define clear_enc_prequeue()	enc_pre_q_wptr = enc_pre_q_r_ptr = 0
#define enc_pre_qempty() (enc_pre_q_w_ptr == enc_pre_q_r_ptr)

#define prequeue_depth (((enc_pre_q_w_ptr < enc_pre_q_r_ptr) ? \
	O_PREQDEPTH : 0) + enc_pre_q_w_ptr - enc_pre_q_r_ptr)

#define enc_pre_qreset() { enc_pre_q_w_ptr = 0; enc_pre_q_r_ptr = 0;}

static int enc_pre_qfull()
{
    if (enc_pre_q_w_ptr < O_PREQLIMIT) return(0);
    else return(1);
}

/* --------------------------------------------------------------- */
static void enc_pre_enque(ch)
int ch;
{
    encode_pre_queue[enc_pre_q_w_ptr++] = ch;
    if (enc_pre_q_w_ptr == O_PREQDEPTH) enc_pre_q_w_ptr = 0;
}

static int enc_pre_deque()
{
    int ch;
    if (enc_pre_qempty()) return(sEOF);
    ch = encode_pre_queue[enc_pre_q_r_ptr++];
    if (enc_pre_q_r_ptr == O_PREQDEPTH) enc_pre_q_r_ptr = 0;
    return(ch);
}

#ifdef ACE_SUPPORT
/* --------------------------------------------------------------- */
/* internationalized IDN decoders(RFC3492)			   */
/* --------------------------------------------------------------- */
/* for punycode encoder/decoder --- */
#define encode_digit(d)	((char)(d + 22 + 75 * (d < 26)))

/*** Main decode function ***/

/* --- Punycode (RFC 3492) --- */
/*@-looploopbreak@*/
static int punycode_encode(input_length,input,output_length,output)
  int input_length;
  const skf_ucode input[];
  int *output_length;
  int output[];
{
  skf_ucode n, delta, h, b, bias, m, q, k, t;
  int out_p,j;

#ifdef SKFDEBUG
  if (is_vvv_debug) {
     fprintf(stderr,"-pe");
  };
#endif

  /* Initialize the state: */
  n = PUNY_INIT_N;
  delta = 0;
  bias = PUNY_I_BIAS;

  /* Handle the basic code points: */
  for (j = 0, out_p = 0;  j < input_length;  ++j) {
    if (is_puny_basic(input[j])) {
      if (O_PSTQDEPTH - out_p < 2) return punycode_big_output;
      output[out_p++] = input[j];
    }
  }
  h = b = out_p;

  /* h is the number of code points that have been handled, b is the  */
  /* number of basic code points, and out is the number of characters */
  /* that have been output.                                           */

  if (b > 0) output[out_p++] = PUNY_DELIM;

  /* Main encoding loop: */

  while (h < input_length) {
    /* All non-basic code points < n have been handled already.	*/
    /* Find the next larger one: */

    for (m = PUNY_MAXINT, j = 0;  j < input_length;  ++j) {
      if (input[j] >= n && input[j] < m) m = input[j];
    }

    /* Increase delta enough to advance the decoder's    */
    /* <n,i> state to <m,0>, but guard against overflow: */

    if (m - n > (PUNY_MAXINT - delta) / (h + 1)) return punycode_overflow;
    delta += (m - n) * (h + 1);
    n = m;

    for (j = 0;  j < input_length;  ++j) {
      if (input[j] < n) {
        if (++delta == 0) return punycode_overflow;
      }

      if (input[j] == n) {
        /* Represent delta as a generalized variable-length integer: */

        for (q = delta, k = PUNY_BASE;;  k += PUNY_BASE) {
          if (out_p >= O_PSTQDEPTH) return punycode_big_output;
          t = (k <= bias) ? PUNY_TMIN :
              ((k >= (bias + PUNY_TMAX)) ? PUNY_TMAX : k - bias);
          if (q < t) break;
          output[out_p++] = encode_digit(t + (q - t) % (PUNY_BASE - t));
          q = (q - t) / (PUNY_BASE - t);
        }

        output[out_p++] = encode_digit(q);
        bias = puny_adapt(delta, h + 1, h == b);
        delta = 0;
        ++h;
      }
    }
    ++delta, ++n;
  }

  *output_length = out_p;
  return punycode_success;
}

#endif	/* ACE_SUPPORT */

/* --------------------------------------------------------------- */
/* Various tools						   */
/* --------------------------------------------------------------- */
/* 1.mime encoder subroutines					   */
/* --------------------------------------------------------------- */
/* canonical codeset dumper					   */
/* --------------------------------------------------------------- */
void show_encode_codeset(oo_codeset)
int oo_codeset;
{
    int i;
    char	*desc;

    desc = i_codeset[oo_codeset].cname;

    if ((oo_codeset == codeset_utf16le) 
     || (oo_codeset == codeset_utf16be)
     || (oo_codeset == codeset_utf16leb)
     || (oo_codeset == codeset_utf16beb)
     || (oo_codeset == codeset_nyukan_utf16)
     || (oo_codeset == codeset_utf16)) {
	if (o_add_bom) {
	    desc = "utf-16";
	} else if (out_endian(conv_cap)) { /* big */
	    desc = "utf-16be"; 	
	} else {
	    desc = "utf-16le"; 	
	};
    } else if ((oo_codeset == codeset_utf32le) 
     || (oo_codeset == codeset_utf32be)
     || (oo_codeset == codeset_utf32leb)
     || (oo_codeset == codeset_utf32beb)
     || (oo_codeset == codeset_utf32)) {
	if (o_add_bom) {
	    desc = "utf-32";
	} else if (out_endian(conv_cap)) { /* big */
	    desc = "utf-32be"; 	
	} else {
	    desc = "utf-32le"; 	
	};
    } else;

    for (i=0; i< 16; i++, desc++) {
	if (*desc == '\0') break;
	SKFcputc(SKFtoupper(*desc));
    };
}

#if defined(FUTURESUPPORT)
/* reserved for future use */
/* --------------------------------------------------------------- */
int is_idnspace(ch)	/* rfc3454 compliant space detection	   */
skf_ucode ch;
{
    if (is_space(ch)) return(1);
    if ((ch >= 0x2000) && (ch <= 0x200b)) return(1);
    if ((ch == 0x202f) || (ch == 0x205f) || (ch == 0x3000)) return(1);
    return(0);
}
#endif
/* --------------------------------------------------------------- */
/* head and tails						   */
/* --------------------------------------------------------------- */
void mime_header_gen(encode)
int	encode;
{
#ifdef SKFDEBUG
    if (is_vvv_debug) fprintf(stderr," HdGn");
#endif
    if (encode) {
	if (is_o_encode_mimeb(encode) || is_o_encode_mimeq(encode)) {
	    SKFcputc('=');
	    SKFcputc('?');
	    show_encode_codeset(out_codeset);
	    SKFcputc('?');
	    if (is_o_encode_mimeb(encode)) {
		SKFcputc('B');
	    } else SKFcputc('Q');
	    SKFcputc('?');
	} else if (is_o_encode_2231(encode)) {
	    SKFcputc('=');
	    show_encode_codeset(out_codeset);
	    SKFcputc(0x27);	/* apostorph */
	    if (skf_input_lang != 0) {
		SKFcputc((int)((skf_input_lang >> 8) & 0x7fU));
		SKFcputc((int)(skf_input_lang & 0x7fU));
	    } else {
		SKFcputc('u');
		SKFcputc('s');
	    };
	    SKFcputc(0x27);	/* apostorph */
	} else if (is_o_encode_b64(encode)) {
		;	/* do nothing				  */
	} else if (is_o_encode_oct(encode) || is_o_encode_hex(encode)
		|| is_o_encode_uri(encode) || is_o_encode_perc(encode)) {
		;	/* do nothing				  */
	} else if (is_o_encode_q(encode)) {
		;	/* do nothing				  */
	} else {
		;
	};
	o_encode_pend = 0;
    };
    pre_ch = 0;
}

/* --------------------------------------------------------------- */
void mime_tail_gen(encode)
int	encode;
{
#ifdef SKFDEBUG
    if (is_vvv_debug) fprintf(stderr," TlGn");
#endif
    if (encode && o_encode_stat) {
	if (is_o_encode_mimeb(encode) || is_o_encode_mimeq(encode)) {
	    base64_enc(sFLSH,encode);
	    SKFcputc(QU_DEL);
	    SKFcputc(EQU_DEL);
	    o_encode_lm += 2;
	    o_encode_lc += 2;
	} else if (is_o_encode_2231(encode)) {
		;
	} else if (is_o_encode_b64(encode)) {
	    base64_enc(sFLSH,encode);
	} else if (is_o_encode_oct(encode) || is_o_encode_hex(encode)
		|| is_o_encode_uri(encode) || is_o_encode_perc(encode)) {
		;	/* do nothing				  */
	} else if (is_o_encode_q(encode)) {
		;	/* do nothing				  */
	} else {
		;
	};
    };
    o_encode_stat = 0;
    b64_encoder_res = 0;
    b64_encoder_cnt = 0;
    o_encode_max = 0;
    pre_ch = -1;		/* marker			   */
    o_encode_pend = 0;
}

/* --------------------------------------------------------------- */
/* encode result clipper					   */
/*  state: TRUE - ADD CRLF on clip				   */
/* --------------------------------------------------------------- */
void encode_clipper(encode,state)
int	encode;
int	state;
{
#ifdef SUPERFOLD
    int i;
#endif
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," EC(%d)",state);
#endif

    if (is_o_encode_mimeb(encode) ||
		is_o_encode_mimeq(encode)) {
	o_encode_lc = 0;
	o_encode_lm = 0;
#ifdef SUPERFOLD
	o_encode_lc_save = 0;
#endif
	mime_tail_gen(encode);
	if (state) {
	    SKFrCRLF();
	    if (is_keis(conv_cap)) {
	    	SKFcputc(KEIS_SP);
	    } else  SKFcputc(A_SP);
	    o_encode_lc = 1;
	    mime_header_gen(encode);
	    o_encode_stat = TRUE;
	} else {
	    o_encode_stat = FALSE;
	};
    } else if (is_o_encode_b64(encode)) {
	SKFrCRLF();
    } else if (is_o_encode_q(encode)) {
	SKFcputc(EQU_DEL);
	SKFrCRLF();
    } else;
    return ;
}

/* --------------------------------------------------------------- */
/* --- line end handler ------------------------------------------ */
/* --------------------------------------------------------------- */
static void SKFrCRLF()
{
#ifdef SKFPDEBUG
    if (is_vv_debug) {
	fprintf(stderr," SKFrCRLF:");
	if (is_lineend_thru) fprintf(stderr,"T");
	if (is_lineend_crlf) fprintf(stderr,"M");
	if (is_lineend_cr) fprintf(stderr,"C");
	if (is_lineend_lf) fprintf(stderr,"L");
	if (detect_cr) fprintf(stderr,"R");
	if (detect_lf) fprintf(stderr,"F");
    };
#endif
    if (is_lineend_thru) {
	if (first_detect_cr && detect_cr) {
	    SKFrputc(A_CR);
	    if (detect_lf) SKFrputc(A_LF);
	} else {
	    if (detect_lf) SKFrputc(A_LF);
	    if (detect_cr || !detect_lf) SKFrputc(A_CR);
	};
    } else {
	if (is_lineend_crlf || is_lineend_cr) SKFrputc(A_CR);
	if (is_lineend_crlf || is_lineend_lf) SKFrputc(A_LF);
    };
    o_encode_lc = 0;
    o_encode_lm = 0;
#ifdef SUPERFOLD
    o_encode_lc_save = 0;
#endif
}

/* --------------------------------------------------------------- */
/* hex/octal encoder						   */
/* --------------------------------------------------------------- */
static int hex_conv_table[16] = {
  '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};

void char2hex(ch)
int	ch;
{
    if (ch >= 0x10000) {
	SKFcputc(hex_conv_table[(ch >> 20) & 0x0fU]);
	SKFcputc(hex_conv_table[(ch >> 16) & 0x0fU]);
	ch &= 0xffffU;
    };
    if (ch >= 0x100) {
	SKFcputc(hex_conv_table[(ch >> 12) & 0x0fU]);
	SKFcputc(hex_conv_table[(ch >> 8) & 0x0fU]);
    };
    SKFcputc(hex_conv_table[(ch >> 4) & 0x0fU]);
    SKFcputc(hex_conv_table[(ch & 0x0fU)]);
}

void char2oct(ch)
/*@unused@*/
int ch;
{
    if (ch >= 0x10000) {
	SKFcputc('0'+((ch >> 22) & 0x03U));
	SKFcputc('0'+((ch >> 19) & 0x07U));
	SKFcputc('0'+((ch >> 16) & 0x07U));
    };
    if (ch >= 0x100) {
	SKFcputc('0'+((ch >> 14) & 0x03U));
	SKFcputc('0'+((ch >> 11) & 0x07U));
	SKFcputc('0'+((ch >> 8) & 0x07U));
    };
    SKFcputc('0'+((ch >> 6) & 0x03U));
    SKFcputc('0'+((ch >> 3) & 0x07U));
    SKFcputc('0'+(ch & 0x07U));
}

/* --------------------------------------------------------------- */
/* base64 encoder						   */
/* --------------------------------------------------------------- */

static void base64_enc(ch,encode)
skf_ucode ch;
int	encode;
{
    skf_ucode c3,c4;

#ifdef SKFPDEBUG
    if (is_vvv_debug) {
	if (ch == sEOF) fprintf(stderr,"(sEOF");
	else if (ch == sOCD) fprintf(stderr,"(sOCD");
	else if (ch == sKAN) fprintf(stderr,"(sKAN");
	else if (ch == sUNI) fprintf(stderr,"(sUNI");
	else if (ch == sFLSH) fprintf(stderr,"(sFLSH");
	else fprintf(stderr,"(%x",ch);
	fprintf(stderr,",#%d,%x)",b64_encoder_cnt,b64_encoder_res);
    };
#endif
    if (ch >= 0x100) {
	out_undefined(ch,SKF_ENC_ERR);
	b64_encoder_cnt = 0;
	b64_encoder_res = 0;	/* for extra care		   */
    } else if (ch < 0) {
	if (b64_encoder_cnt == 2) {
	    c3 = (b64_encoder_res << 2) & 0x03cU;
	    c4 = b64_encoder_table[c3];
	    SKFcputc(c4);
	    if (is_o_encode_mimeb(encode) || is_o_encode_b64(encode))
	    	SKFcputc(EQU_DEL);
	    o_encode_lm += 2;
	    o_encode_lc += 2;
	} else if (b64_encoder_cnt == 1) { /* 2nd byte		   */
	    c3 = (b64_encoder_res << 4) & 0x030U;
	    c4 = b64_encoder_table[c3];
	    SKFcputc(c4);
	    if (is_o_encode_mimeb(encode) || is_o_encode_b64(encode)) {
		SKFcputc(EQU_DEL);
		SKFcputc(EQU_DEL);
	    };
	} else;
	b64_encoder_res = 0;
	b64_encoder_cnt = 0; /* clear counter		   */
    } else {
	if (b64_encoder_cnt == 2) { /* 3rd byte		   */
	    c3 = ((ch >> 6) & 0x03U) + ((b64_encoder_res << 2) & 0x3cU);
	    c4 = b64_encoder_table[c3];
	    SKFcputc(c4);
	    c3 = ch & 0x3fU;
	    c4 = b64_encoder_table[c3];
	    SKFcputc(c4);
	    b64_encoder_res = 0;
	    b64_encoder_cnt = 0;
	} else if (b64_encoder_cnt == 1) { /* 2nd byte	   */
	    c3 = ((ch >> 4) & 0x0fU) + ((b64_encoder_res << 4) & 0x30U);
	    c4 = b64_encoder_table[c3];
	    SKFcputc(c4);
	    b64_encoder_res = ch & 0x0fU;
	    b64_encoder_cnt = 2; /* for extra care		   */
	} else {		/* b64_encoder_cnt == 0		   */
	    c3 = ((ch >> 2) & 0x3fU);
	    c4 = b64_encoder_table[c3];
	    SKFcputc(c4);
	    b64_encoder_res = ch & 0x03U;
	    b64_encoder_cnt = 1; /* 2bit = 2			   */
	};
    };
}

/* --------------------------------------------------------------- */
/* MIMEQ encoder						   */
/* --------------------------------------------------------------- */
/*@-paramuse@*/
void mimeq_enc(ch,encode)
skf_ucode ch;
int encode;
{
#ifdef SKFPDEBUG
    if (is_vvv_debug) {
	fprintf(stderr,"(#%x)",ch);
    };
#endif
    if (mime_safe(ch)) {
	SKFcputc(ch);
    } else {
	SKFcputc(EQU_DEL);
	char2hex(ch);
    };
}
/*@+paramuse@*/

/* --------------------------------------------------------------- */
/* poke queue contents to output. o_encode_stat should be TRUE	   */
/* --------------------------------------------------------------- */
void output_to_mime(ch,o_encode)
int ch;
int o_encode;
{

#ifdef SKFDEBUG
     if (is_vv_debug) fprintf(stderr,"(OM:%2x)",ch);
#endif
     if (ch < 0) {
     	return;
     } else if (is_o_encode_mimeb(o_encode) 
     		|| is_o_encode_2231(o_encode)) {
	base64_enc(ch,o_encode);
     } else if (is_o_encode_mimeq(o_encode)) {
	mimeq_enc(ch,o_encode);
     } else if (is_o_encode_b64(o_encode)) {
	base64_enc(ch,o_encode);
     } else if (is_o_encode_oct(o_encode)) {
	if (!is_lineend(ch) && hex_beescape(ch)) {
	    SKFcputc(BSL_DEL);
	    char2oct(ch);
	} else SKFcputc(ch);
     } else if (is_o_encode_hex(o_encode)) {
	if (!is_lineend(ch) && hex_beescape(ch)) {
	    if (is_o_encode_q(o_encode)) { SKFcputc(EQU_DEL);
	    } else if (is_o_encode_perc(o_encode)) { SKFcputc(PERC_DEL);
	    } else {SKFcputc(CAP_DEL); };
	    char2hex(ch);
	} else SKFcputc(ch);
     } else if (is_o_encode_uri(o_encode)) {
	if (!is_lineend(ch)) {
	    SKFcputc(PERC_DEL);
	    char2hex(ch);
	} else SKFcputc(ch);
     } else;
     return;
}

/* --------------------------------------------------------------- */
void queue_to_mime(o_encode)
int o_encode;
{
    int ch;

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr,"QM");
#endif

/*@-infloopsuncon@*/
    while (!enc_pre_qempty()) {
    	ch = enc_pre_deque();
	if (ch < 0) continue;
	if (o_encode_stat == 0) {
	    SKFcputc(ch);
	} else {
	    output_to_mime(ch,o_encode);
	};
    };
/*@+infloopsuncon@*/
}

#ifdef ACE_SUPPORT
/* --------------------------------------------------------------- */
/* ace(punycode) encoder					   */
/* --------------------------------------------------------------- */
/*@-infloopsuncon@*/
void o_p_encode(ch)
skf_ucode ch;
{
    int i;
    int	pres;
    skf_ucode rch;

#ifdef SKFPDEBUG
    if (is_vv_debug) {
	fprintf(stderr,"-ipe%c%lx(%x-%d:%d)",
		((o_encode_stat != 0) ? '!' : ':'),(unsigned long)ch,
		o_encode,enc_pre_q_w_ptr,enc_pre_q_r_ptr);
    };
#endif
    if (o_encode_stat == 0) {
    	if (ch < 0) {
	    return;
	} else if (is_puny_strdelim(ch) || (ch == '/')) {
		/* if basic, ch <= 0x80	 */
		/* maybe some encoding violate pos 0x3f '/', but this */
		/* won't be serious matter for punycode cutting. */
	    while (!enc_pre_qempty()) SKFcputc(enc_pre_deque());
	    SKFcputc(ch);
	    enc_pre_qreset();
	} else if (!is_puny_basic(ch) || (ch == A_ESC)) {
	    enc_pre_enque(ch);
	    o_encode_stat = TRUE;
	} else {
	    enc_pre_enque(ch);
	    o_encode_stat = TRUE;
	};
    } else {
	if ((is_puny_strdelim(ch)) || enc_pre_qfull()) {
	    rch = ch;
	    if (ch < 0) {
	    	ch = 0;
	    };
	    enc_pre_enque(ch);
	    encode_post_wptr = O_PSTQDEPTH;
	    if (puny_maxcp == 0) {
	    	while (!enc_pre_qempty()) {
		    SKFcputc(enc_pre_deque());
		};
	    } else {
		pres = punycode_encode((enc_pre_q_w_ptr - 1),
				 encode_pre_queue,
				 &encode_post_wptr,
				 encode_post_queue);
		if (pres == 0) {
		    SKFcputc(PUNY_PRFX1);
		    SKFcputc(PUNY_PRFX2);
		    SKFcputc('-'); SKFcputc('-');
		    for (i=0;i<encode_post_wptr;i++)
			SKFcputc(encode_post_queue[i]);
		};
	    };
	    o_encode_stat = FALSE;
	    enc_pre_qreset();
	    puny_maxcp = 0;
	    if (is_puny_delim(rch)) SKFcputc(rch);
	} else {
	    if (!is_puny_basic(ch) || (ch == A_ESC)) puny_maxcp = 1;
	    enc_pre_enque(ch);
	};
    };
}
/*@+infloopsuncon@*/
#endif

/* --------------------------------------------------------------- */
/* showing mime-specified charset 				   */
/* return value:						   */
/*	0:  parse succeeded 					   */
/*	-1: failed to parse, or undecodable charset		   */
/* --------------------------------------------------------------- */

int show_mime_charset(sy)
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

/* --------------------------------------------------------------- */
/* encoder main(0) - clip test.					   */
/*  in: offset (actual expected character count as bytes)	   */
/*  out: TRUE - clipped internally				   */
/* --------------------------------------------------------------- */
/*  if current_position + offset is over-limit, insert line end.   */
/*  if entering this routine, you should have at least one 	   */
/*  additional character to output (encode_clipper assumes	   */
/*  trailing output follows).					   */
/*  input: offset - followed ascii-like(not cntl) characters	   */
/*	   soffset - followed non-ascii characters		   */
/* --------------------------------------------------------------- */
/* no clip: ace, uri, hex, oct					   */
/* --------------------------------------------------------------- */
/*@-infloopsuncon@*/
int mime_clip_test(offset,soffset)
int offset;
int soffset;
{
    int chlen,whlen,p,q,chrlen;
    int ichrlen,ch;

	/* FIXME: we have to detect line length limit precisely.
		and we also have to treat both mime'd and ascii'd  */
#ifdef SKFDEBUG
    if (is_vv_debug) {
	fprintf(stderr,"%c%c(%d(%d:%d))",
	    ((o_encode_stat == 0) ? '-' : '+'),
	    ((is_o_encode_mimeb(o_encode) || is_o_encode_2231(o_encode)) ?
	    	'B' : 'Q'),
	    o_encode_lm,offset,soffset);
    } else;
#endif
    if (is_o_encode_mimeb(o_encode) || is_o_encode_2231(o_encode)) {
	whlen = (offset + soffset + prequeue_depth);
    	if (o_encode_stat == 0) { /* encoding not started	   */
	    p = whlen / 3; q = whlen - (p * 3);
	    chlen = p * 4 + ((q == 0) ? 0 : 4);
	    if (((o_encode_lc + chlen) >= mime_start_limit) ||
	    	(soffset != 0) || (is_ucs_utf16(conv_cap))) {
		mime_header_gen(o_encode);
		if (is_o_encode_mimeb(o_encode)) {
		    b64_encoder_cnt = 0;
		    b64_encoder_res = 0;
		} else;
		o_encode_stat = TRUE;
		b64_encoder_cnt = 0;   /* for extra care	   */
	    	o_c_encode(sFLSH);	/* poke everything out	   */
	    } else;
	} else {		  /* encoding already started.	   */
		/* test length and clip if mime length exceeded.   */
	    if (b64_encoder_cnt == 1) {
	    	chlen = 3;
	    	if (whlen > 1) whlen -= 2;
		else whlen = 0;
	    } else if (b64_encoder_cnt == 2) {
	    	chlen = 2;
	    	if (whlen > 1) whlen--;
	    } else chlen = 0;
	    p = whlen / 3; q = whlen - (p * 3);
	    chlen += (p * 4) + ((q == 0) ? 0 : 4);
	    chrlen = offset + (soffset * 3) + mime_max_chlen
	    		+ prequeue_depth;
	    if ((o_encode_lm >= (mime_tail_limit - chrlen)) &&
	    	(o_encode_pend > 0)) {
	    			/* overflow without mime	  */
#ifdef SKFDEBUG
		if (is_vvv_debug) {
		    fprintf(stderr,"OW ");
		} else;
#endif
		if (mime_limit_aware(nkf_compat)) {
		    ichrlen = offset + (soffset * 3) + mime_max_chlen;
		    while (!enc_pre_qempty()) {
			ch = enc_pre_deque();
			if (ch < 0) continue;
			if (o_encode_stat == 0) {
			    SKFcputc(ch);
			} else {
			    output_to_mime(ch,o_encode);
			};
			if (o_encode_lm >= 
			      (mime_tail_limit - (++ichrlen))) {
			    SKF1FLSH();
			    encode_clipper(o_encode,TRUE);
			} else;
		    };
		} else;
		return(1);
	    } else if (o_encode_lm >= (mime_tail_limit - chlen)) {
	    			/* overflow with mime		  */
#ifdef SKFDEBUG
		if (is_vvv_debug) {
		    fprintf(stderr,"O ");
		} else;
#endif
		if (o_encode_pend > 0) {
		    o_encode_pend = 2;
		    return(1);
		} else;
		SKF1FLSH();
		queue_to_mime(o_encode);
			/* character may remain due to suspend	   */
		encode_clipper(o_encode,TRUE);
		return(1);
	    } else;
	};
    } else if (is_o_encode_mimeq(o_encode) || is_o_encode_q(o_encode)) {
	chlen = offset + (soffset * 3) + mime_max_chlen + prequeue_depth;
	if (is_o_encode_q(o_encode)) chlen--;
    	if (o_encode_stat == 0) { /* encoding not started	   */
	    if (((o_encode_lc + chlen) >= mime_start_limit) ||
	    	(soffset != 0) || (is_ucs_utf16(conv_cap))) {
		mime_header_gen(o_encode);
		if (is_o_encode_mimeb(o_encode)) {
		    b64_encoder_cnt = 0;
		    b64_encoder_res = 0;
		} else;
		o_encode_stat = TRUE;
	    	o_c_encode(sFLSH);	/* poke everything out	   */
	    } else;
	} else {		  /* encoding already started.	   */
	    if ((o_encode_lm >= (mime_tail_limit - chlen)) &&
	    	(o_encode_pend > 0)) {
	    			/* overflow without mime	  */
#ifdef SKFDEBUG
		if (is_vvv_debug) {
		    fprintf(stderr,"OW ");
		} else;
#endif
		if (mime_limit_aware(nkf_compat)) {
		    ichrlen = offset + (soffset * 3) + mime_max_chlen;
		    while (!enc_pre_qempty()) {
			ch = enc_pre_deque();
			if (ch < 0) continue;
			if (o_encode_stat == 0) {
			    SKFcputc(ch);
			} else {
			    output_to_mime(ch,o_encode);
			};
			if (o_encode_lm >= 
			      (mime_tail_limit - (++ichrlen))) {
			    SKF1FLSH();
			    encode_clipper(o_encode,TRUE);
			} else;
		    };
		} else;
		return(1);
	    } else if (o_encode_lm >= (mime_tail_limit - chlen)) {
#ifdef SKFDEBUG
		if (is_vvv_debug) {
		    fprintf(stderr,"O ");
		} else;
#endif
		SKF1FLSH();
		queue_to_mime(o_encode);
			/* character may remain due to suspend	   */
		encode_clipper(o_encode,TRUE);
		return(1);
	    } else;
	};
    } else if (is_o_encode_b64(o_encode)) {
    	if (o_encode_lm >= (mime_fold_llimit - 4)) {
		/* 4 is for nkf compaibility	*/
		SKFrCRLF();
	} else;
    } else;
    return(0);
}
/*@+infloopsuncon@*/

/* --------------------------------------------------------------- */
/* encoder main(2) - character encoding				   */
/* called at SKFputc hook					   */
/* we should be during encode here (i.e. o_encode_stat == TRUE).   */
/* --------------------------------------------------------------- */
/* this routine lays on the most deep process in skf. Character	   */
/* is handled bytewise, and doesn't know about Multibyte codes.	   */
/* --------------------------------------------------------------- */
/* Note: punycode and uri hex never comes here. Those encoding is  */
/* handled within ucodoconv.c					   */
/* --------------------------------------------------------------- */
void o_c_encode(ch)
int	ch;
{

#ifdef ACE_SUPPORT
    if (is_o_encode_ace(o_encode)) {
    	o_p_encode(ch); return;
    } else;
#endif
#ifdef SKFDEBUG
    if (is_vv_debug) {
	fprintf(stderr," ioe%c",
	    ((o_encode_stat == 0) ? ':' :
	    	((o_encode_pend == 1) ? '#' : 
		    ((o_encode_pend > 1) ? '@' : '!'))));
	if (ch == sEOF) fprintf(stderr," sEOF");
	else if (ch == sOCD) fprintf(stderr,"sOCD");
	else if (ch == sKAN) fprintf(stderr,"sKAN");
	else if (ch == sUNI) fprintf(stderr,"sUNI");
	else if (ch == sSUSP) fprintf(stderr,"sSUSP");
	else if (ch == sFLSH) fprintf(stderr,"sFLSH");
	else if (ch == mFLSH) fprintf(stderr,"mFLSH");
	else fprintf(stderr,"%02x",ch);
	fprintf(stderr,"(%d/%d-%d)",o_encode_lm,o_encode_lc,
		prequeue_depth);
	if (ch >= 0) {
	    fprintf(stderr,"%c%c%c%c:",
		(no_early_mime_out(nkf_compat) ? 'N':'E'),
		(table_mime_tail(ch) ? 'T':'C'),
		(mime_safe(ch) ? 't':'m'),
		(is_white(ch) ? 'w':'d'));
	} else;
    };
#endif

    if (ch == sOCD) {   /* should not be here, but throw anyway.   */
       pre_ch = ch; return;
    } else if (o_encode_stat == 0) {	/* under testing	   */
	/* b64, hex, dec and perc is always o_encode_stat == TRUE  */
	/* Puny never comes here, So mimeb, mimeq and		   */
	/* rfc2231(not supported) arrive here.			   */
	if (ch < 0) {
	    queue_to_mime(o_encode);
	    pre_ch = ch;
#ifdef SUPERFOLD
	    o_encode_lc_save = o_encode_lc;
#endif
	} else if (is_lineend(ch) && !is_ucs_utf16(conv_cap)) {
	/* Note: in o_c_encode, 0x0a,0d,20 and not ascii only 
	   appears in UTF16/32 case. For performance reason,
	   these codeset is treated as special here, not 
	   codeset specific routine except ucodoconv.c	*/
	    queue_to_mime(o_encode);
	    if (is_true_lineend(ch)) {
		SKFrCRLF();
	    } else;
	} else if (!mime_safe(ch)) {
	    mime_header_gen(o_encode);
	    if (is_o_encode_mimeb(o_encode)) {
		b64_encoder_cnt = 0;
		b64_encoder_res = 0;
	    };
	    o_encode_stat = TRUE;
	    b64_encoder_cnt = 0;   /* for extra care	   */
	    queue_to_mime(o_encode);
	    output_to_mime(ch,o_encode);
	} else if ((is_white(ch) || table_susp_tail(ch))
			&& !is_ucs_utf16(conv_cap)) {
#ifdef SKFDEBUG
	    if (is_vv_debug) fprintf(stderr," WH");
#endif
	    queue_to_mime(o_encode);
	    SKFcputc(ch);
	} else {
	    enc_pre_enque(ch);	/* ELSE - buffering	 	   */
	};
 /* following codes has status: o_encode_stat == 1 */
    } else if ((ch < 0) || is_lineend(ch)) {
		/* Note: sFLSH and sEOF is also handled here	   */
	if (is_o_encode_b64(o_encode) && is_lineend(ch)) { 
	    base64_enc(ch,o_encode);
	    pre_ch = ch;
	    return;
		/* BASE64 OR MIME-B OR MIME-Q OR RFC2231 */
	} else if (is_o_encode_b64(o_encode) ||
			is_o_encode_mimes(o_encode)) {
	    if (o_encode_pend > 0) {  /* sweep out pending chars   */
	    	if (ch == mFLSH) {	/* JIS case		   */
			/* encode should sweep esc seq. beforehand */
		    queue_to_mime(o_encode);
		    o_encode_pend = 0;
		    return;
		} else {
		    mime_tail_gen(o_encode);
		    queue_to_mime(o_encode);
		};
	    } else;
#if 0
	    if ((ch == sFLSH) || (ch == mFLSH) || (ch == sSUSP)) {
#else
	    if ((ch == sFLSH) || (ch == mFLSH)) {
#endif
		    /* puny length limit is considered in other */
		    /* routine. Here we test only MIMEs	   */
		queue_to_mime(o_encode);
		return;
	    } else if (ch == sSUSP) {
	    	queue_to_mime(o_encode);
		if (o_encode_stat > 0) {
			mime_tail_gen(o_encode);
		} else;
		return;
	    } else {
		base64_enc(sOCD,o_encode);/* sweep remains in buffer */
	    };
	    if (o_encode_stat > 0) {
		mime_tail_gen(o_encode);
	    } else;
	    if (is_true_lineend(ch)) {
	    	SKFrCRLF();
	    } else;
	    o_encode_stat = 0;	/* terminate encodings.		   */
#ifdef SUPERFOLD
	    o_encode_lc_save = 0;
#endif
	    o_encode_pend = 0;
	    return;
	} else if (is_o_encode_oct(o_encode)) {
	    if (!is_lineend(ch)) {
		SKFcputc(BSL_DEL);
		char2oct(ch);
		return;
	    };
	} else {
		;	/* hex etc. Do nothing.			   */
	};
	if (is_true_lineend(ch)) {
	    SKFrCRLF();
	};
#if !defined(USE_TRUE_MIME_TAIL)
    } else if ((table_mime_tail(ch) || is_white(ch)) 
		&& !no_early_mime_out(nkf_compat)
		&& (!is_kanji_shift && !is_ucs_utf16(conv_cap))
		&& is_o_encode_mimebq(o_encode)) {
		/* Note: sFLSH and sEOF is also handled here	   */
	o_encode_pend = 1;
	enc_pre_enque(ch);
#endif
    } else if ((o_encode_pend > 0)
		&& !no_early_mime_out(nkf_compat)
		&& is_o_encode_mimebq(o_encode)) {
	if (!mime_safe(ch)) {
	    if ((o_encode_pend == 2) && (o_encode_stat > 0)) {
	    	mime_tail_gen(o_encode);
	    	queue_to_mime(o_encode);
		SKFrCRLF();
		SKFcputc(A_SP);
		mime_header_gen(o_encode);
		o_encode_stat = TRUE;
		enc_pre_enque(ch);
		queue_to_mime(o_encode);
	    } else {
		enc_pre_enque(ch);
		queue_to_mime(o_encode);
	    };
	    o_encode_pend = 0;
	} else if (is_white(ch) || table_mime_tail(ch) 
		|| table_susp_tail(ch)){
	    if (o_encode_stat > 0) {
		mime_tail_gen(o_encode);
	    } else;
	    enc_pre_enque(ch);
	    queue_to_mime(o_encode);
	    o_encode_pend = 0;
	} else {
	    enc_pre_enque(ch);
	};
    } else { 		/* already started encodes		   */
    			/* ch != 0 nor lineend			   */
	output_to_mime(ch,o_encode);
    };
    return;
}

/* --------------------------------------------------------------- */
/* out_encoder EOF process. sFLSH should be performed before call  */
/* --------------------------------------------------------------- */
void encoder_tail()
{
#ifdef SKFDEBUG
    if (is_vv_debug) {
    	fprintf(stderr," ET");
    } else;
#endif
    if (o_encode_stat == 0) {	/* not in mime'd		   */
	if (is_o_encode_mimes(o_encode)) {
	    o_encode_lc = 0;
	    o_encode_lm = 0;
#ifndef SWIG_EXT
	} else if (is_o_encode_b64(o_encode)) {
	    SKFrCRLF();
#endif
	} else;
    } else {			/* in mime			   */
	if (is_o_encode_mimes(o_encode)) {
	    mime_tail_gen(o_encode);
	    o_encode_lc = 0;
	    o_encode_lm = 0;
	} else if (is_o_encode_oct(o_encode) 
		|| is_o_encode_uri(o_encode)
		|| is_o_encode_hex(o_encode)
		|| is_o_encode_perc(o_encode)
		|| is_o_encode_q(o_encode)) {
	    ;
	} else if (is_o_encode_b64(o_encode)) {
	    mime_tail_gen(o_encode);
	    o_encode_lc = 0;
	    o_encode_lm = 0;
#ifndef SWIG_EXT
	    SKFrCRLF();
#endif
	} else;		/* should be none. no other coding exist   */
	o_encode_stat = 0;	/* cleaned. now clear states.	   */
    };
}

/* --------------------------------------------------------------- */
/*@-paramuse@*/
void	utf16_clipper(ch)
skf_ucode ch;
{
#ifdef DEBUG
    if (is_vv_debug) {
    	fprintf(stderr," UT6");
    } else;
#endif
    o_c_encode(sFLSH);
    mime_tail_gen(o_encode);
    o_encode_lc = 0;
    o_encode_lm = 0;
}
/* --------------------------------------------------------------- */
