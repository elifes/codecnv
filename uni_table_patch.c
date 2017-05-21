/* *******************************************************************
** Copyright (c) 2004-2007 Seiji Kaneko. All rights reserved.
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
** 4. Do not distribute uni_table.c generated from this source alone.
**   Redistributing uni_table.c with this source is permitted.
**********************************************************************
** Disclaimer: This software is provided and distributed AS iS, 
**	without any implicit or explicit warranties, and not
**	guaranteed to be error-free. In no event shall the author be
**	liable for any direct, indirect or incidental damages,
**	including, but not limited to, loss of data, use or profits
**	responsibility for any direct or indirect damages or results
**	arising by using whole or a part of this software.
**********************************************************************
   uni_table_patch.c: To and from ucs2 table generator patches
   $Id: uni_table_patch.c,v 1.5 2008/04/01 14:44:30 seiji Exp $
*/

/* --------------------------------------------------------------- */
/* Note: this table is fix for converting Ucs2 to JIS.		   */
/* --------------------------------------------------------------- */
/* format: ucs2, JIS code((1stchar << 8) + 2ndcode)		   */
/*			+ (x212 char) ? 0x8000 : 0		   */
/*			+ (isochar) ? (tablenum << 16) : 0;	   */
/* for table number, refer in_converter.c's comments.		   */
/* --------------------------------------------------------------- */
long uni_fix_table[] = {
    0x00a0,0x0020, /*    latin[0x00] = 0x0020;*/
    0x00a1,0xa242, /*    latin[0x01] = 0xa242;*/
    0x00a2,0x2171, /*    latin[0x02] = 0x2171;*/
    0x00a3,0x2172, /*    latin[0x03] = 0x2173;*/
    0x00a4,0xa270, /*    latin[0x04] = 0xa270;*/
    0x00a5,0x216f, /*    latin[0x05] = 0x216f;*/
    0x00a6,0xa243, /*    latin[0x06] = 0xa243;*/
    0x00ab,0x2263, /*    latin[0x0b] = 0x2263;*/
    0x00ac,0x224c, /*    latin[0x0c] = 0x224c;*/
    0x00ad,0x215d, /*    latin[0x0d] = 0x215d;*/
    0x00af,0xa234, /*    latin[0x0f] = 0xa234;*/
    0x00b5,0x264c, /*    latin[0x15] = 0x264c;*/
    0x00b7,0x2126, /*    latin[0x17] = 0x2126;*/
    0x00b8,0xa231, /*    latin[0x18] = 0xa231;*/
    0x00bb,0x2264, /*    latin[0x1b] = 0x2264;*/
    0x00bf,0xa244, /*    latin[0x1f] = 0xa244;*/
    0x00ff,0xab73, /*    latin[0x5f] = 0xab73;	this must fix.	  */
    0x0114,0xaa35, /*    latin[0x74] = 0xaa35;*/
    0x0115,0xaa36, /*    latin[0x75] = 0xaa36;*/
    0x0123,0xab39, /*    latin[0x83] = 0xab39;	this must fix.	  */
    0x012c,0xaa43, /*    latin[0x8c] = 0xaa43;*/
    0x012d,0xab43, /*    latin[0x8d] = 0xab43;*/
    0x0131,0xa945, /*    latin[0x91] = 0xa945;	this must fix.	  */
    0x01f5,0xab39, /*    latin[0x155] = 0xab39;*/
    0x02b9,0x0027, /*    latin[0x219] = 0x0027;*/
    0x02ba,0x0022, /*    latin[0x21a] = 0x0022;*/
    0x02bb,0x0060, /*    latin[0x21b] = 0x0060;*/
    0x02bc,0x0027, /*    latin[0x21c] = 0x0027;*/
    0x02c6,0x2130, /*    latin[0x2c6] = 0x2130;*/
    0x02ca,0x212d, /*    latin[0x2ca] = 0x212d;*/
    0x02cb,0x212e, /*    latin[0x2cb] = 0x212e;*/
    0x0300,0x212e, /*    latin[0x260] = 0x212e;*/
    0x0301,0x212d, /*    latin[0x261] = 0x212d;*/
    0x0302,0x2130, /*    latin[0x262] = 0x2130;*/
    0x0305,0x2131, /*    latin[0x265] = 0x2131;*/
    0x0308,0x212f, /*    latin[0x268] = 0x212f;*/
    0x0332,0x2132, /*    latin[0x292] = 0x2132;*/
    0x20dd,0x227e, /*    uni_glyph[0x0dd] = 0x227e;*/
    0x2108,0x273f, /*    uni_glyph[0x109] = 0x273f;*/
    0x2126,0x2638, /*    uni_glyph[0x126] = 0x2638;*/
    0x212a,0x004b, /*    uni_glyph[0x12a] = 0x004b;*/
    0x2206,0x2624, /*    uni_glyph[0x206] = 0x2624;*/
    0x220a,0x2645, /*    uni_glyph[0x20a] = 0x2645;*/
    0x220e,0x2223, /*    uni_glyph[0x20e] = 0x2223;*/
    0x220f,0x2630, /*    uni_glyph[0x20f] = 0x2630;*/
    0x2211,0x2632, /*    uni_glyph[0x211] = 0x2632;*/
    0x2212,0x215d, /*    uni_glyph[0x212] = 0x215d;*/
    0x2215,0x213f, /*    uni_glyph[0x215] = 0x213f;*/
    0x2216,0x2140, /*    uni_glyph[0x216] = 0x2140;*/
    0x2223,0x2143, /*    uni_glyph[0x223] = 0x2143;*/
    0x2225,0x2142, /*    uni_glyph[0x225] = 0x2142;*/
    0x223e,0x2266, /*    uni_glyph[0x23e] = 0x2266;*/
    0x2264,0x2165, /*    uni_glyph[0x264] = 0x2165;*/
    0x2265,0x2166, /*    uni_glyph[0x265] = 0x2166;*/
    0x229a,0x217d, /*    uni_glyph[0x29a] = 0x217d;*/
    0x2312,0x225e, /*    uni_glyph[0x312] = 0x225e;*/
    0x2322,0x225e, /*    uni_glyph[0x322] = 0x225e;*/
    0x2014,0x213d, /*    uni_punct[0x15] = 0x213d;*/
    0x203e,0x007e, /*    uni_punct[0x3e] = 0x007e;*/
    0x9faf,0x6e68, /*	 uni_f_4e[0x51af] = 0x6e68; (variant)*/
    -1
};

