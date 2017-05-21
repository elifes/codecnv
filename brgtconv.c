/* *******************************************************************
** Copyright (c) 1999-2009 Seiji Kaneko.  All rights reserved.
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
    brgtconv.c: output converters for B-Right/V(TM) variants
	v1.92	Newly written from ucodeconv.c 
    $Id: brgtconv.c,v 1.33 2017/01/05 15:05:48 seiji Exp seiji $
**/

#include <stdio.h>
#include "skf.h"
#include "skf_fileio.h"
#include "convert.h"
#include "oconv.h"

#define  SKFBRGTSW	TRUE
#define	 J_NC		0x222e

#define  AZ_LIM		8836
#define	 BZ_LIM		20680
#define	 CZ_LIM		32524

static	int	brgt_hwid = 0;
static  int	brgt_ucod = 0;

static unsigned short ascii2x0208[256] = {
    0x0000,0x0000,0x0000,0x0000, 0x0000,0x0000,0x0000,0x0000,
    0x0008,0x0009,0x000a,0x000b, 0x000c,0x000d,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000, 0x0000,0x0000,0x0000,0x0000,
    0x0018,0x0000,0x0000,0x0000, 0x0000,0x0000,0x0000,0x0000,
    0x2121,0x212a,0x2149,0x2174, 0x2170,0x2173,0x2175,0x2147,
    0x214a,0x214b,0x2176,0x215c, 0x2124,0x215d,0x2125,0x213f,
    0x2330,0x2331,0x2332,0x2333, 0x2334,0x2335,0x2336,0x2337,
    0x2338,0x2339,0x233a,0x233b, 0x233c,0x233d,0x233e,0x233f,

    0x2340,0x2341,0x2342,0x2343, 0x2344,0x2345,0x2346,0x2347,
    0x2348,0x2349,0x234a,0x234b, 0x234c,0x234d,0x234e,0x234f,
    0x2350,0x2351,0x2352,0x2353, 0x2354,0x2355,0x2356,0x2357,
    0x2358,0x2359,0x235a,0x235b, 0x235c,0x235d,0x235e,0x235f,
    0x2360,0x2361,0x2362,0x2363, 0x2364,0x2365,0x2366,0x2367,
    0x2368,0x2369,0x236a,0x236b, 0x236c,0x236d,0x236e,0x236f,
    0x2370,0x2371,0x2372,0x2373, 0x2374,0x2375,0x2376,0x2377,
    0x2378,0x2379,0x237a,0x237b, 0x237c,0x237d,0x237e,0x237f,

    0x0000,0x0000,0x0000,0x0000, 0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000, 0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000, 0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000, 0x0000,0x0000,0x0000,0x0000,
    0x2121,0xa242,0x2171,0x2172, 0xa270,0x216f,0xa243,0x2178,
    0x212f,0xa26d,0xa26c,0x2263, 0x224c,0x215d,0xa26e,0xa234,
    0x216b,0x215e,0x0000,0x0000, 0x212d,0x264c,0x2279,0x2126,
    0xa231,0x0000,0xa26b,0x2264, 0x0000,0x0000,0x0000,0xa244,

    0xaa22,0xaa21,0xaa24,0xaa2a, 0xaa23,0xaa29,0xa921,0xaa2e,
    0xaa32,0xaa31,0xaa34,0xaa33, 0xaa40,0xaa3f,0xaa42,0xaa41,
    0xa923,0xaa50,0xaa52,0xaa51, 0xaa54,0xaa58,0xaa53,0x215f,
    0xa92c,0xaa63,0xaa62,0xaa65, 0xaa64,0xaa72,0xa930,0xa94e,
    0xab22,0xab21,0xab24,0xab2a, 0xab23,0xab29,0xa941,0xab2e,
    0xab32,0xab31,0xab34,0xab33, 0xab40,0xab3f,0xab42,0xab41,
    0xa943,0xab50,0xab52,0xab51, 0xab54,0xab58,0xab53,0x2160,
    0xa94c,0xab63,0xab62,0xab65, 0xab64,0xab72,0xa950,0xab73
};

/* --------------------------------------------------------------- */
/* Tron code announcer						   */
/* --------------------------------------------------------------- */
static unsigned short tron_announcer[] = {
  0xff,0xe1,0x00,0x18, 0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
  0x21,0x00,0x00,0x00, 0x100
};

static unsigned short tron_finale[] = {
  0xff,0xe2,0x00,0x00, 0x100
};

static unsigned short tron_turnhalf[] = {
  0xff,0xa2,0x00,0x06, 0x03,0x00,0x00,0x01,
  0x00,0x02, 0x100
};

static unsigned short tron_turnfull[] = {
  0xff,0xa2,0x00,0x06, 0x03,0x00,0x00,0x00,
  0x00,0x00, 0x100
};

void  tron_announce()
{
    SKF_STRPUT(tron_announcer);
}

static void  BRGT_TURNHALF()
{
    SKF_STRPUT(tron_turnhalf);
	brgt_hwid = 1;
}

static void  BRGT_TURNFULL()
{
    SKF_STRPUT(tron_turnfull);
    brgt_hwid = 0;
}

/* --------------------------------------------------------------- */
void SKFBRGT1OUT(x)
			/* currently not supported by B-Right/V	   */
skf_ucode	x;
{
    if (brgt_ucod) {
	SKFputc(0xfe); SKFputc(0x21);	/* back to the system plane */
	brgt_ucod = 0;
    };
    SKFputc(x & 0xff);
}

void SKFBRGTOUT(x)
skf_ucode	x;
{
    if (brgt_ucod) {
	SKFputc(0xfe); SKFputc(0x21);	/* back to the system plane */
	brgt_ucod = 0;
    };
    SKFputc((x >> 8) & 0xff);
    SKFputc(x & 0xff);
}

static void SKFBRGTUOUT(x)	/* Unicode out			   */
skf_ucode	x;
{
    skf_ucode 	d1,d2;

    if (brgt_ucod == 0) {
	SKFputc(0xfe); SKFputc(0x30);
	brgt_ucod = 1;
    };
    if (x < 0xac00) {
	if (x < BZ_LIM) {	/* AB Zone			   */
	    d1 = (x / 94); d2 = x - (x * 94);
	    SKFputc(d1 + 0x21 + ((x < AZ_LIM) ? 0 : 1));
	    SKFputc(d2 + 0x21);
	} else {			/* CD Zone		   */
	    d1 = (x / 126); d2 = x - (x * 126);
	    SKFputc(d1 + 0x21 + ((x < CZ_LIM) ? 0 : 1));
	    SKFputc(d2 + 0x80);
	};
    } else if (x < 0x10000) {
	x = x - 0xac00;
	d1 = (x / 94); d2 = x - (x * 94);
	SKFputc(d1 + 0x21 + ((x < AZ_LIM) ? 0 : 1));
	SKFputc(d2 + 0x21);
    } else {
	out_undefined(x,SKF_UX0212);
    };
}

static void SKFBRGTX0212OUT(x)
skf_ucode	x;
{
	SKFBRGTOUT(x);
}

#if 0
/* not used */
static void SKFBRGTK1OUT(x)
skf_ucode	x;
{
    if (brgt_ucod) {
	SKFputc(0xfe); SKFputc(0x21);	/* back to the system plane */
	brgt_ucod = 0;
    };
    out_undefined(x,SKF_UX0212);
}
#endif

/* --------------------------------------------------------------- */
static unsigned short brgt_subdeco0[] = {
  0xff,0xa4,0x00,0x06, 0x04,0x81,0x00,0x00,
  0x01,0x02, 0x100
};
static unsigned short brgt_subdeco1[] = {
  0xff,0xa4,0x00,0x02, 0x05,0x00,0x100
};

static void BRGTSUBSCRIPT(x)	/* subscript decoration		   */
unsigned int x;
{
    SKF_STRPUT(brgt_subdeco0);
    SKFputc((x >> 8) & 0xff);
    SKFputc(x & 0xff);
    SKF_STRPUT(brgt_subdeco1);
}
/* --------------------------------------------------------------- */
/* B/Right converters						   */
/* --------------------------------------------------------------- */
/* handle 0x00 - 0x7f -------------------------------------------- */
/*@-globstate@*/
void	BRGT_ascii_oconv(c2)
skf_ucode c2;
{
    skf_ucode	c1,d1,c3;

    c1 = c2 & 0x7f;
#ifdef SKFDEBUG
    if (is_vv_debug) {
    	fprintf(stderr," brgt_ascii: %02x",c1);
	debugcharout((int)c1);
    };
#endif
    if (brgt_hwid == 0) {	/* convert to half-width	   */
	BRGT_TURNHALF();
    };
    d1 = ascii2x0208[c1];
    if (uni_o_ascii != NULL) c3 = uni_o_ascii[c1];
    else c3 = 0;
    if ((d1 != 0) && (c3 == 0)) {
			/* can't output because user says so	   */
	if ((c1 == A_LF) || (c1 == A_CR) || (c1 == A_SUB)
	    || (c1 == A_FF) || (c1 == A_HT) || (c1 == A_BS)) {
	    SKFBRGTOUT(d1); return;
	};
	skf_lastresort(c1);
    } else if (d1 == 0) {
	out_undefined(c1,SKF_OUNDEF); fold_count++;
    } else {
	if (d1 < 0x8000) {
	    SKFBRGTOUT(d1);
	} else {
	    SKFBRGTX0212OUT(d1);
	};
    };

}

/* --- latin output ---------------------------------------------- */
void	BRGT_latin_oconv(c2)
skf_ucode c2;
{
    skf_ucode	c3 = 0,c4;
    int		c1;

    c1 = (c2 & 0xff);
    c4 = c2;
    c2 = (c2 >> 8) & 0xff;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," BRGT_latin: %02x,%02x",c2,c1);
#endif

    if (c4 < 0x100) {
	c3 = ascii2x0208[c1-0x80];
	if (c3 == 0) {
	    switch (c1) {
		case 0xb2: BRGTSUBSCRIPT(0x2332); break;
		case 0xb3: BRGTSUBSCRIPT(0x2333); break;
		case 0xb9: BRGTSUBSCRIPT(0x2331); break;
		case 0xbc: 			/* FALLTHROUGH */
		case 0xbd: 			/* FALLTHROUGH */
		case 0xbe: 			/* FALLTHROUGH */
		    SKFBRGTUOUT(c1); break;
		default:
		    out_undefined(c1,SKF_OUNDEF);
		    fold_count++;
	    };
	    return;
	} else {
	    BRGT_TURNHALF();
	};
    } else if ((c2 >= 0x01) && (c2 <= 0x0f)) { /* smaller latin	   */
	if (brgt_hwid == 0) {	/* convert to half-width	   */
	    BRGT_TURNHALF();
	};
	if (uni_o_latin != 0) c3 = uni_o_latin[c4 - 0xa0];
	else c3 = 0;
    } else {
	if (brgt_hwid == 1) {	/* convert to full-width	   */
	    BRGT_TURNFULL();
	};
	if (uni_o_symbol != NULL) c3 = uni_o_symbol[c4 & 0xfff];
	else c3 = 0;
    };
    if (c3 >= 0x8000) {		/* Have code in X-0212		   */
	SKFBRGTX0212OUT(c3);
    } else if (c3 != 0) {	/* in output codeset		   */
	if (c3 < 0x100) {
	    BRGT_ascii_oconv(c3);
	} else SKFBRGTOUT(c3); 
    } else {			/* output as it is		   */
	SKFBRGTUOUT(c4);
    };
}

/* --- CJK kana plane -------------------------------------------- */
/*  handles 0x3000 - 0x4dff					   */
/*@-globstate@*/ /* should we have to check uni_o_kana is not NULL? */
/*@-nullpass@*/ /* cjk_out_parse gocha@! Should fix later */
void	BRGT_cjkkana_oconv(c2)
skf_ucode c2;
{
    int c1;
    unsigned short ch;

    c1 = c2 & 0x3ff; 		/* 1024				   */

#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," BRGT_cjkkana: %02x,%02x",
					((c2 >> 8) & 0xff),c1);
#endif

    if (brgt_hwid) {
	    BRGT_TURNFULL();
    };
    if (c2 < 0x3400) {	 	 /* 0x3000-0x3400		   */
	if (uni_o_kana != NULL) {
	    if (uni_o_kana != NULL) ch = uni_o_kana[c1];
	    else ch = 0;
	    if (ch != 0) { 	/* can convert			   */
		if (ch >= 0x8000) {
		    SKFBRGTX0212OUT(ch);
		} else if (ch < 0x100) {
		    BRGT_ascii_oconv(ch);
		} else {
		    SKFBRGTOUT(ch);
		};
	    } else {
		SKFBRGTUOUT(c2);
	    };
	};
    } else { 			/* hanguls and cjk extension-a	   */
	SKFBRGTUOUT(c2);
    };
}

/* --- CJK plane ------------------------------------------------- */
void BRGT_cjk_oconv (c3)
    skf_ucode    c3;
{
    skf_ucode	c4 = 0;
#ifdef SKFDEBUG
    int	c1,c2;

    if (is_vv_debug) {
	c2 = (c3 >> 8) & 0xff;
	c1 = c3 & 0xff;
	fprintf(stderr," BRGT_cjk: %02x,%02x",c2,c1);
    };
#endif

    if (brgt_hwid) {
	    BRGT_TURNFULL();
    };
    if (uni_o_kanji != NULL) {
	c4 = uni_o_kanji[c3 - 0x4e00];
    };
    if (c4 <= 0) {
	out_undefined(c3,SKF_OUNDEF);
    } else if (c4 < 0x100) {
	BRGT_ascii_oconv(c4);
    } else if (c4 > 0x8000) {
	SKFBRGTX0212OUT(c4);
    } else {
	SKFBRGTOUT(c4);
    };
}

/* --- o-zone and non-BMP area ----------------------------------- */
/* 0xa000 <= c2 < 0xd7ff or c2 > 0x10000			   */
void BRGT_ozone_oconv(c2)
    skf_ucode	c2;
{
    int	ch, converted = 0;
#ifdef SKFDEBUG
    int	c1,c3;

    c1 = c2 & 0xff;
    c3 = (c2 >> 8) & 0xff;
    if (is_vv_debug) fprintf(stderr," BRGT_ozone: %03x,%02x",c3,c1);
#endif

    if (brgt_hwid) {
	    BRGT_TURNFULL();
    };
    if (c2 < 0xa400) {
      if (uni_o_y != NULL) {
	if ((ch = uni_o_y[c2 - 0xa000]) != 0) {
	    SKFBRGTOUT(ch); converted = 1;
	};
      };
    } else if ((c2 >= 0xac00) && (c2 < 0xd800)) {	/* Hangle  */
	if (uni_o_hngl != NULL) ch = uni_o_hngl[c2 - 0xac00];
	else ch = 0;
	if (ch != 0) {
	    converted = 1;
	    if (ch < 0x100) {
		BRGT_ascii_oconv(ch);
	    } else if (ch > 0x8000) {
		SKFBRGTX0212OUT(ch);
	    } else {
		SKFBRGTOUT(ch);
	    };
	};
    } else {
	out_undefined(c2,SKF_OUNDEF); converted = 1;
    };
    if (converted == 0) {
	SKFBRGTUOUT(c2);
    };
#ifndef		USE_UBUF
    if (unbuf_f) SKFfflush((skfoFILE *)stdout);
#endif
}

/* --- compatibility plane --------------------------------------- */
/*  0xf900 <= c2 < 0x10000					   */

void BRGT_compat_oconv (c2)
    skf_ucode    c2;
{
    skf_ucode	c3;
    int		c1,c4;

    c1 = (c2 & 0xff);
    c4 = (c2 >> 8) & 0xff;
#ifdef SKFDEBUG
    if (is_vv_debug) fprintf(stderr," BRGT_compat: %02x,%02x",c4,c1);
#endif

    if (uni_o_compat != NULL) {
	if (uni_o_compat != NULL) c3 = uni_o_compat[c2 - 0xf900];
	else c3 = 0;
	if ((c4 == 0xff) && (c1 > 0x60) && (c1 <= 0x9f)) {
	    if (brgt_hwid == 0) {  /* convert to half-width	   */
		BRGT_TURNHALF();
	    };
	    (void)x0201conv(c1-0x40,0);
	} else if ((c4 == 0xfe) && (c1 < 0x10)) { /* variation selector */
		;			/* just discard		    */
	} else {
	    if (brgt_hwid) {
		BRGT_TURNFULL();
	    };
	    if (c3 != 0) {
		if (c3 < 0x100) {
		    BRGT_ascii_oconv(c3);
		} else if (c3 > 0x8000) {
		    SKFBRGTX0212OUT(c3);
		} else {
		    SKFBRGTOUT(c3);
		};
	    } else {
		SKFBRGTUOUT(c2);
	    };
	};
    } else {
	SKFBRGTUOUT(c2);
    };
#ifndef	USE_UBUF
    if (unbuf_f) SKFfflush((skfoFILE *)stdout);
#endif
}

/* --- private use plane ----------------------------------------- */
void BRGT_private_oconv (c2)
    skf_ucode    c2;
{
#ifdef SKFDEBUG
    int		c1;
    c1 = (c2 & 0xff);

    if (is_vv_debug) fprintf(stderr," BRGT_private: %02x,%02x",
	(c2 >> 8) & 0xff,c1);
#endif

    if (c2 < 0xe000) {
	if (c2 < 0xe880) {
	    lig_x0213_out(c2,0);
	} else {
	    out_undefined(c2,SKF_IOUTUNI);
	};
	return;
    };
    if (brgt_hwid) {
	BRGT_TURNFULL();
    };
    SKFBRGTUOUT(c2);
#ifndef		USE_UBUF
    if (unbuf_f) SKFfflush((skfoFILE *)stdout);
#endif
}

/* --- finish process -------------------------------------------- */
void BRGT_finish_procedure()
{
    oconv_flush();
    if (brgt_ucod) {
	SKFputc(0xfe); SKFputc(0x21);	/* back to the system plane */
	brgt_ucod = 0;
    };
    if (brgt_hwid) {
	BRGT_TURNFULL();
    };
    SKF_STRPUT(tron_finale);
}

/* --------------------------------------------------------------- */
void SKFBRGTSTROUT(st)
const char *st;
{
    int len;

    for (len=0; (len<30) && (*st != '\0') ; len++,st++) {
	BRGT_ascii_oconv((skf_ucode)(*st));
    };
}

/* --------------------------------------------------------------- */
