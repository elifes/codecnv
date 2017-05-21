#ident "$Id: skf.c,v 1.269 2017/01/05 15:05:48 seiji Exp seiji $"
const char *rev = "SKF version 2.1.1 2017-03-01\n";
/* *******************************************************************
** Copyright (c) 1993-2016 Seiji Kaneko. All rights reserved.
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
** Disclaimer: This software is provided and distributed AS IS,
**	without any implicit or explicit warranties, and not
**	guaranteed to be error-free. In no event shall the author be
**	liable for any direct, indirect or incidental damages,
**	including, but not limited to, loss of data, use or profits
**	responsibility for any direct or indirect damages or results
**	arising by using whole or a part of this software.
**********************************************************************
** Warning:
** (1) If input stream contains X-0201 kana part, code auto detection
**	may not work correctly. In this case, input stream should be
**      specified explicitly.
** (2) This program, as default, converts X-0201 kana part into X-0208
**      kana. Beware line length, for such conversion changes
**	byte counts of each line. You can disable this feature by
**	option -O7, -O8 or -Ok.
** (3) When both X-0212 enable and shift-JIS output is specified, all
**      X-0212 code is converted to geta-code.
***********************************************************************
**
** skf: simple kanji filter (i18n support)
**
** USAGE:       skf [flags] [Infile]
**
** Flags: *************************************************************
** b    Output is buffered		(DEFAULT)
** u    Output is unbuffered
** Output codes: ******************************************************
** j,n  Outout code is JIS 7/8 bit      ('n' denotes X-0208(1983))
**        8-bit feature is controlled by extended control.
** s,x  Output code is Shift JIS        ('s' denotes X-0208(1983))
** e,a  Output code is EUC AT&T JIS     ('a' denotes X-0208(1983))
** q	Output code is UCS2
** k	Output code is EBCDIK/KEIS83
** z	Output code is UTF-8
**
** ISO 2022/JIS X-0202 Output code sequence control (discourged)
** i_   Output Kanji-in is ESC+'$'+ _   (DEFAULT_KI)
** o_   Output Kanji-out is ESC+'('+ _  (DEFAULT_KO)
**
** Input Code controls: ***********************************************
** S,X  input character set is pre-defined to shift-jis
** E,A  input character set is pre-defined to EUC
** N	input character set is pre-defined to jis 8bit
** Q	input character set is pre-defined to UCS2
** K	input character set is pre-defined to EBCDIK/KEIS83
** Z	input character set is pre-defined to UTF-8
**
** I    output warning message when non-jis character is detected.
**
**/

#include <stdio.h>

#include <stdlib.h>
#include <sys/types.h>
#ifndef S_SPLINT_S
#include <sys/stat.h>
#endif

#include "config.h"

#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#if defined(__MINGW32__) || defined(SKF_MINGW)
#include <windows.h>
#endif

#include <utime.h>
#include <fcntl.h>
#include <errno.h>

#include "skf.h"
#include "skf_fileio.h"
#include "convert.h"
#include "oconv.h"
#include "in_code_table.h"

#ifdef SWIG_EXT
#include "skf_convert.h"
#else
/* ------------------------------------------------------------- */
/* where everything will go.					 */
/* ------------------------------------------------------------- */
skfoFILE  *fout;
#endif

/* options */
/* ------------------------------------------------------------- */
/* various control variables: see skf.h				 */
/* ------------------------------------------------------------- */
int             unbuf_f = FALSE;
/* ---- refer skfdefs.h for inner bit assignment --------------- */
unsigned long	conv_cap = 0;		/* output mode pack	 */
unsigned long	conv_alt_cap = 0;	/* output mode pack+	 */

	 long	encode_cap = 0;		/* encode support	 */
unsigned int	o_encode = 0;		/* output encode support */
unsigned long	ucod_flavor = 0;	/* Unic*de output opt.   */
unsigned long	codeset_flavor = 0;	/* codeset minor change  */

/* ------------------------------------------------------------- */
/* LINE_END detection						 */
/* ------------------------------------------------------------- */
int		le_detect = 0;		/* lineend detect marker */
int		le_defs = 0;		/* normalized line-end	 */

/* ------------------------------------------------------------- */
static int	skf_fileout = FALSE;	/* not to stdout	 */
/* ------------------------------------------------------------- */
short		debug_opt = 0;

#ifdef	FOLD_SUPPORT
/* -------------------------------------------------------------- */
/*  fold_fclap:	folding switch. Folding is off if == 0		  */
/* 		if != 0, this value means force wrap limit.	  */
/*		default: DEFAULT_FOLD + DEFAULT_FOLD_WRAP	  */
/*			 or 0 (folding is off)			  */
/*  fold_clap:	standard wrap length				  */
/*		default: DEFAULT_FOLD				  */
/* -------------------------------------------------------------- */
/* Interraction between wordwrap and CR/LF conversion		  */
/*    non-wordwrap:	output lineend char as specified	  */
/*    wordwrap wo. notrunc_le: 					  */
/*		lineend is controlled by fold_count. CR, LF and	  */
/*		CR-LF (regard as 1 char) is converted to SPACE,	  */
/*		expect in case that it is placed to real lineend. */
/*		The first CR or LF is regard as line-end char.	  */
/*		Unselected CR/LF is just discarded.		  */
/*    wordwrap with notrunc_le: output lineend char as specified. */
/* -------------------------------------------------------------- */
int		fold_clap = 0;
int		fold_fclap = 0;	/* folding is off if == 0	  */
int		fold_omgn = DEFAULT_OIDASH_MGN;
static int	fold_hmgn = -1;

#if 0
void		fold_value_setup();
#endif
#endif /* FOLD_SUPPORT */

int		out_codeset = -1;	/* output codeset	 */
int		in_codeset = -1;	/* input codeset	 */

/* ------------------------------------------------------------- */
unsigned long	in_param = 0;		/* inputside parameter	 */
unsigned long	nkf_compat = 0;		/* nkf compatibility	 */
#if	defined(NKF_NAMETEST) && !defined(SWIG_EXT)
static int	is_name_nkf = FALSE;	/* called by thy name	 */
#endif
/* ------------------------------------------------------------- */
unsigned long	error_opt = 0;		/* behavior on errors	 */

unsigned long	preconv_opt = 0;	/* preconvert misc opt.  */
unsigned long	option_guarding = 0;	/* option guarding	 */

unsigned long	skf_input_lang = 0;	/* input language set	 */
unsigned long	skf_output_lang = 0;	/* output language set	 */
unsigned long	skf_given_lang = 0;	/* parametered language	 */

/* ------------------------------------------------------------- */
/* TEXT type detection						 */
/* ------------------------------------------------------------- */
unsigned long	skf_in_text_type = 0;

/* control variables */
skf_ucode	ucode_undef = 0;	/* substitute character	 */
int		mime_fold_llimit = MIME_ENCODE_LLIMIT;
				/* MIME line clip length	 */

char            k_in = (char)0,	/* DEFAULT is unused in 1.92	 */
		k_out = (char)0;

/* Global states */
char		ucs_tagstr[6] = {   /* unic*de tag string buffer  */
		'j','a','-','j','p','\0'};

skf_ucode	pref_subst_char = -1;/* preferred substitute char */
int		viscii_escape = -1; /* viscii escape char - in	  */
int		o_viscii_escape = -1; /* viscii escape char - out */

#ifndef SWIG_EXT
static skfFILE	skf_file_buf[1];
#endif

/* --- internal states ------------------------------------------ */
static	short	opt_fileshow = FALSE;
static	short	opt_filenoshow = FALSE;
static int	out_code = -1;		/* internal parse pass	  */

#ifndef SWIG_EXT
char		*in_file_name;	/* input file name save area 	  */
static char	*out_file_name;	/* output file name save area 	  */
static char	*inb_file_name;	/* input file name save area 	  */
static char	*outb_file_name = NULL; 
				/* output file name save area 	  */
static char	*outs_file_name; /* suffix added to outfile name  */
		/* used only with --in-place or --overwrite	  */
#endif /* SWIG_EXT */

int		**arib_macro_tbl = NULL; /* arib macro area	  */

/* converters */
static int	argeval P_((int,char **,int));
int		skf_in_converter();
static int	skf_kanaconv_parser P_((int));

#ifndef SWIG_EXT
static const char *string_stdin = "(stdin)";
#ifdef NEED_BINMODE
static const char *string_stdout = "(stdout)";
#endif
static char *default_outname = "skf.out";
#endif /* SWIG_EXT */

#ifndef	SWIG_EXT
static void	skf_gangfinish P_((skfoFILE *));
#endif

/* -------------------------------------------------------------- */
/* extended option parser					  */
/* -------------------------------------------------------------- */
/* Note: parse indexes regions is as follows			  */
/*	0x0000 - 0x02ff		misc options without parameters	  */
/*	0x0300 - 0x03bf		misc options with dec parameters  */
/*	0x03c0 - 0x03ff		misc options with hex parameters  */
/*	0x0400 - 0x05ff		input codeset settings		  */
/*	0x0600 - 0x07ff		output codeset settings		  */
/*	0x0800 - 0x0fff		(reserved)			  */
/*	0x1000 - 0x1fff		decode feature settings		  */
/*	0x2000 - 0x403f		(reserved)			  */
/*	0x4040 - 0x5a5a		Language parsers		  */
/*	0x5a5b - 0x5aff		(reserved)			  */
/*	0x5b00 - 0x5bff		(reserved)			  */
/*	0x5c00 - 0x5fff		(reserved)			  */
/*	0x6000 - 0x67ff		G0 setting charset specification  */
/*	 - 0x000 - 0x0ff	94-char graphic characters	  */
/*	 - 0x100 - 0x1ff	96-char graphic characters	  */
/*	 - 0x200 - 0x2ff	94-char with 2nd byte characters  */
/*	 - 0x300 - 0x3ff	3-octet MB graphic characters	  */
/*	 - 0x400 - 0x4ff	4-octet MB graphic characters	  */
/*	 - 0x500 - 0x5ff	non-iso2022 graphic characters	  */
/*	 - 0x600 - 0x6ff	(reserved)			  */
/*	 - 0x700 - 0x7ff	(reserved)			  */
/*	0x6800 - 0x6fff		G1 setting charset specification  */
/*	0x7000 - 0x77ff		G2 setting charset specification  */
/*	0x7800 - 0x7fff		G3 setting charset specification  */
/* -------------------------------------------------------------- */
#define M_ILIMIT  0x2ff
#define M_PLIMIT  0x3ff
#define M_PDLIMIT  0x3bf
#define LNG_INDEX 0x4040
#define LNG_ILIMIT 0x5a5a

#define CST_INDEX 0x0400
#define CST_ILIMIT 0x05ff
#define DST_INDEX 0x0600
#define DST_ILIMIT 0x07ff
#define ST_P_MASK 0x01ff

#define DEC_INDEX 0x1000
#define DEC_ILIMIT 0x1fff
#define ENC_INDEX 0x2000
#define ENC_ILIMIT 0x2fff

#define SG0_INDEX 0x6000
#define SG1_INDEX 0x6800
#define SG2_INDEX 0x7000
#define SG3_INDEX 0x7800
#define SG_ILIMIT 0x7fff
#define SG_PH_MASK 0x1fff

static const struct long_option option_table[] = {
    {"x0212-enable",2},
    {"enable-x0212",2},
    {"disable-x0212",3},
    {"use-compat",4},
    {"enable-compat",4},
    {"disable-compat",5},
/*    {"suppress-compat",5}, */
/*     {"no-compat",5}, */
    {"use-ms-compat",6},
    {"enable-ms-compat",6},
    {"ms-ucs-map",6},
/*    {"no-ms-compat",7}, */
    {"disable-ms-compat",7},
/*    {"suppress-ms-compat",60}, */
    {"use-cde-gaiji",8},
    {"use-cde-compat",8},
    {"enable-cde-compat",8},
    {"big-endian",10},
    {"output-big-endian",10},
    {"little-endian",11},
    {"output-little-endian",11},
    {"input-big-endian",12},
    {"input-little-endian",13},
    {"disable-endian-mark",15},		/* for UCS2 & UTF8	*/
    {"disable-bom",15},
/*    {"suppress-endian-mark",15}, */
    {"enable-endian-mark",16},		/* for UTF7 and UTF8	*/
    {"enable-bom",16},
    {"no-kana",17},			/* kanji detect hint	*/
    {"no-utf7",18},			/* kanji detect hint	*/
#ifdef DYNAMIC_LOADING
    {"input-detect-jis78",19},		/* kanji detect hint	*/
#endif
    {"detect-x201-kana",20},		/* kanji detect hint	*/
    {"detect-x0201-kana",20},		/* kanji detect hint	*/
    {"input-x201-kana",20},		/* kanji detect hint	*/
    {"input-x0201-kana",20},		/* kanji detect hint	*/
    {"fuzzy-detect",21},		/* kanji detect hint	*/
#ifdef OLD_NEC_COMPAT
    {"old-nec-compat",22},
#endif
    {"suppress-jis90",23},		/* kanji detect hint	*/
    {"use-replace-char",25},
    {"quad-char",26},
    {"disable-chart",28},
    {"use-apple-gaiji",29},
    {"use-mac-gaiji",29},
    {"kana-jis7",30},		/* kana output controls.	*/
    {"kana-siso",30},
    {"kana-jis8",31},
    {"kana-esci",32},
    {"kana-call",32},
    {"kana-enable",32},
    {"enable-kana",32},
    {"use-kana",32},
    {"no-x0201-kana",33},
    {"disable-ms-gaiji",40},
    {"disable-ibm-gaiji",41},
    {"disable-nec-gaiji",42},
    {"oldcell-to-emoticon",47},
    {"use-old-cell-map",48},
    {"filewise-detect",49},
    {"no-filewise-detect",50},
    {"disable-filewise-detect",50},
    {"linewise-detect",51},
    {"disable-linewise-detect",52},
    {"fold-strong",53},
    {"bg5cc",54},
    {"enable-latin-announce",55},
    {"disable-latin-announce",56},
    {"use-iso8859-1-right",65},
    {"use-iso8859-1-left",66},
    {"use-iso8859-1",65},
    {"use-g0-ascii",67},
    {"use-out-g0-ascii",67},
    {"disable-iso8859-1",69},
    {"add-renew",80},
    {"add-announce",80},
    {"disable-announce",84},
    {"enable-lang-tag",81},
    {"add-annon",82},
    {"force-lang-tag",83},
    {"uncheck-utf32-range",85},
    {"enable-cesu8",86},
    {"non-strict-utf8",87},
    {"enable-double-latin",90},
    {"disable-code-system-sense",91},
    {"enable-variation-selector",92},
    {"input-limit-to-jp",94},
    {"reset",100},
    {"euc-protect-g1",110},		/* euc only control	*/
    {"preserve-euc-charset",110},	   /* GNU style		*/
    {"endian-protect",111},		/* ignore endian mark	 */
    {"preserve-endian",111},
/*    {"disable-table-loading",112},	:* Dynamic table disable */
    {"disable-lang-preserve",114},
    {"convert-html-hexadecimal",124},
    {"convert-html-hex",124},
    {"convert-html-decimal",123},
    {"convert-html-dec",123},
    {"convert-html-uri",125},
    {"convert-html",120},
    {"convert-sgml",120},
    {"convert-tex",121},
/*    {"convert-x0212",122}, */
    {"convert-null",135},
    {"html-sanitize",126},
    {"force-html-pri",127},
    {"force-private-idn-out",128},
    {"suppress-space-convert",131},
    {"disable-space-convert",131},
    {"enable-space-single-convert",132},
    {"enable-space-conv",133},
    {"suppress-space-conv",131},
    {"disable-space-conv",131},
    {"enable-space-single-conv",132},
    {"enable-space-convert",133},
    {"enable-ascii-convert",134},
    {"enable-ascii-conv",134},
    {"lineend-crlf",141},
    {"lineend-cr",140},
    {"lineend-mac",140},
    {"lineend-msdos",141},
    {"lineend-windows",141},
    {"lineend-lf",142},
    {"lineend-unix",142},
    {"mac",140},
    {"msdos",141},
    {"windows",141},
    {"unix",142},
    {"lineend-thru",143},
    {"disable-adelim",144},
    {"sentence-clip",146},
    {"lineend-normalize",148},
    {"inquiry",150},
    {"guess",150},			/* nkf inquiry		   */
    {"hard-inquiry",154},
    {"short-inquiry",155},
    {"mime-ms-compat",156},
    {"show-filename",160},		/* inquiry control	*/
    {"print-file-name",160},		   /* GNU style		*/
    {"suppress-filename",161},		/* inquiry control	*/
    {"limit-to-ucs2",170},
    {"disable-cjk-extension",171},
    {"disable-cjk-compat",172},
    {"input-cr",175},
    {"input-lf",176},
    {"input-crlf",177},
    {"nkf-compat",180},
    {"skf-compat",181},
    {"mime-nkf-mode",184},
    {"mime-nkfmode",184},
    {"mime-fold-length",186},
    {"mime-stringprep",187},
    {"mime-skf196compat",188},
    {"x0212",2},
    {"fj",185},
    {"kana-convert",189},
    {"hiragana",197},
    {"katakana",198},
    {"hiragana-katakana",199},
    {"katakana-hiragana",199},
    {"exec-in",192},
    {"exec-out",192},
    {"cp932",194},
    {"no-cp932ext",195},
    {"no-best-fit-chars",193},
    {"cp932inv",192},
    {"no-cp932",196},
    {"utf8mac",192},
    {"mime-limit-aware",215},
    {"mail-mime-out",214},
    {"cap-input",DEC_INDEX + 1},
    {"url-input",DEC_INDEX + 3},
    {"base64",DEC_INDEX + 32},
    {"numchar-input",DEC_INDEX + 64},
    {"euc-input",CST_INDEX+codeset_eucjp},
    {"sjis-input",CST_INDEX+codeset_sjis},
    {"jis-input",CST_INDEX+codeset_x0208},
    {"base64-input",DEC_INDEX+32},
    {"mime-input",DEC_INDEX+8},
    {"mime",DEC_INDEX + 6},
    {"fb-html",120},
    {"fb-skip",133},
    {"fb-subchar",0x1c0},
    {"fb-",192},
    {"start-kanji",190},
    {"invis-strip",191},
    {"version",200},
    {"help",201},
    {"show-supported-charset",202},
    {"verbose-version",205},
    {"nkf-help",207},
    {"in-place",211},
    {"overwrite",210},
#ifdef SKFPDEBUG
    {"table-debug",203},
#endif
    {"show-supported-codeset",204},
    {"abort-on-conv-error",206},
    {"mime-persistent",214},
    {"no-mime-persistent",216},
#ifdef ALTERNATE_TABLE_DIR
    {"alt-table-dir",220},
#endif
    {"disable-nfd-decomposition",240},
    {"enable-nfd-decomposition",241},
    {"disable-nfkd-decomposition",242},
    {"enable-nfkd-decomposition",243},
    {"disable-nfda-decomposition",244},
    {"enable-nfda-decomposition",245},
    {"disable-nfd-encomposition",250},
    {"enable-nfd-encomposition",251},
    {"disable-nfkd-encomposition",252},
    {"enable-nfkd-encomposition",253},
    {"enable-kana-concat",254},
    {"disable-kana-concat",255},
    {"enable-hkana-concat",256},
    {"disable-hkana-concat",257},
    {"preferred-substitute-char",0x3c0},
    {"viscii_escape",260},
    {"itext-sgml",272},		/* specify input text type	*/
    {"itext-mail",273},
    {"itext-maillike",274},
    {"itext-ftext",275},
    {"itext-man",276},
    {"itext-sgml",276},
#if defined(SWIG_EXT) && (defined(SWIGPYTHON) && defined(SKF_PYTHON3)) 
    {"py-out-binary",282},
    {"py-out-string",283},
#endif
    {"arib_bmp0_convert",285},
    {"arib_bmp1_convert",286},
    {"arib_bmp2_convert",287},
/* input encoding shortcuts */
    {"input-ms",CST_INDEX+codeset_cp932},	
    {"input-sjis",CST_INDEX+codeset_sjis},
    {"input-euc-kr",CST_INDEX+codeset_euckr},
    {"input-euc-cn",CST_INDEX+codeset_euccn},
    {"input-euc",CST_INDEX+codeset_eucjp},
    {"input-jis",CST_INDEX+codeset_x0208},
    {"input-utf16",CST_INDEX+codeset_utf16leb},
    {"input-utf8",CST_INDEX+codeset_utf8},
    {"input-big5",CST_INDEX+codeset_big5},
    {"input-keis",CST_INDEX+codeset_keis},
    {"set-g0",SG0_INDEX},
    {"set-g1",SG1_INDEX},
    {"set-g2",SG2_INDEX},
    {"set-g3",SG3_INDEX},
    {"decode",DEC_INDEX},
    {"encode",ENC_INDEX},
    {"set-output-charset",DST_INDEX},
    {"output-charset",DST_INDEX},
    {"oc",DST_INDEX},
    {"set-input-charset",CST_INDEX},
    {"input-charset",CST_INDEX},
    {"ic",CST_INDEX},
    {"set-lang",0x4040},
    {" ",-1}		/* tail					   */
};

/* charset option: input side charset select for plane setting	   */
/* Note: charset name is case *insensitive*			   */
static const struct long_option option_code[] = {
    {"x0201r",		SK_UB_UNI+x0201r_index},
    {"jiskana",		SK_UB_UNI+x0201r_index},
    {"katakana",	SK_UB_UNI+x0201r_index},
    {"jisx0201",	SK_UB_UNI+x0201_index},
    {"x0201",		SK_UB_UNI+x0201_index},
    {"iso646-jp",	SK_UB_UNI+x0201_index},
    {"kana",		SK_UB_UNI+x0201r_index},
    {"iso-ir-8-1",	SK_UB_UNI+nats_f_a_index},
    {"iso-ir-8-2",	SK_UB_UNI+nats_a_index},
    {"iso-ir-9-1",	SK_UB_UNI+nats_d_a_index},
    {"iso-ir-9-2",	SK_UB_UNI+nats_ad_index},
    {"iso-ir-226",	SK_UB_8859+iso8859_16_index},
    {"iso-ir-149",	SK_UB_4DB+ksc5601_index},
    {"iso-ir-10",	SK_UB_UNI+sen8502_b_index},
    {"iso-ir-11",	SK_UB_UNI+sen8502_c_index},
    {"iso-ir-13",	SK_UB_UNI+x0201r_index},
    {"iso-ir-14",	SK_UB_UNI+x0201_index},
    {"iso-ir-15",	SK_UB_UNI+iso646_it_index},
    {"iso-ir-16",	SK_UB_UNI+iso646_p_index},
    {"iso-ir-17",	SK_UB_UNI+ecma114_sp_index},
    {"iso-ir-18",	SK_UB_UNI+iso646_gro_index},
    {"iso-ir-19",	SK_UB_UNI+iso646_lgo_index},
    {"iso-ir-21",	SK_UB_UNI+din66083_index},
    {"iso-ir-25",	SK_UB_UNI+nfz62010_index},
    {"iso-ir-27",	SK_UB_UNI+ecma_gr_b_index},
    {"iso-ir-37",	SK_UB_UNI+iso5427b_index},
    {"iso-ir-42",	SK_UB_3DB+jisc6226_78_index},
    {"iso-ir-57",	SK_UB_UNI+gb198880_index},
    {"iso-ir-58",	SK_UB_4DB+gb2312_index},
    {"iso-ir-60",	SK_UB_UNI+ns4551_index},
    {"iso-ir-61",	SK_UB_UNI+ns4551_2_index},
    {"iso-ir-84",	SK_UB_UNI+iso646_pri_index},
    {"iso-ir-85",	SK_UB_UNI+iso646_spi_index},
    {"iso-ir-86",	SK_UB_UNI+iso646_hui_index},
    {"iso-ir-87",	SK_UB_3DB+x0208_index},
    {"iso-ir-2",	SK_UB_UNI+iso646_irv_index},
    {"iso-ir-4",	SK_UB_UNI+bs4730_index},
    {"iso-ir-6",	SK_UB_UNI+ascii_index},
    {"iso646,irv",	SK_UB_UNI+iso646_irv_index},
    {"iso646-ca2",	SK_UB_UNI+csa_z243_2_index},
    {"iso646-ca",	SK_UB_UNI+csa_z243_1_index},
    {"iso646-cn",	SK_UB_UNI+gb198880_index},
    {"iso646-cu",	SK_UB_IM2+cuban_spanish_index},
    {"iso646-de",	SK_UB_UNI+din66083_index},
    {"iso646-dk",	SK_UB_CP+ds2089_index},
    {"iso646-es2",	SK_UB_UNI+iso646_spi_index},
    {"iso646-es",	SK_UB_UNI+ecma114_sp_index},
    {"iso646-fi",	SK_UB_UNI+sen8502_b_index},
    {"iso646-fr",	SK_UB_UNI+nfz62010_index},
    {"iso646-gb",	SK_UB_UNI+bs4730_index},
    {"iso646-hu",	SK_UB_UNI+iso646_hui_index},
    {"iso646-it",	SK_UB_UNI+iso646_it_index},
    {"iso646-kr",	SK_UB_MISC+ksx1003_index},
    {"iso646-no",	SK_UB_UNI+ns4551_index},
    {"iso646-no2",	SK_UB_UNI+ns4551_2_index},
    {"iso646-pt2",	SK_UB_UNI+iso646_pri_index},
    {"iso646-pt",	SK_UB_UNI+iso646_p_index},
    {"iso646-se2",	SK_UB_UNI+sen8502_c_index},
    {"iso646-se",	SK_UB_UNI+sen8502_b_index},
    {"iso646-us",	SK_UB_UNI+ascii_index},
    {"iso646-yu",	SK_UB_UNI+serb_slov_index},
    {"ksx1003",		SK_UB_MISC+ksx1003_index},
    {"us-ascii",	SK_UB_UNI+ascii_index},
    {"ascii",		SK_UB_UNI+ascii_index},
    {"ansi-x3.4-1968",	SK_UB_UNI+ascii_index},
    {"ansi-x3.4",	SK_UB_UNI+ascii_index},
    {"serbian",		SK_UB_UNI+serb_cyr_index},
    {"macedonian",	SK_UB_UNI+macedonian_index},
    {"iso-9036",	SK_UB_UNI+iso9036_index},
    {"iso-6937-2",	SK_UB_UNI+iso6937_sp2_index},
    {"arabic7",		SK_UB_UNI+iso9036_index},
    {"ecma114-sp",	SK_UB_UNI+ecma114_sp_index},
    {"ecma114-greek",	SK_UB_UNI+ecma_gr_b_index},
    {"jisx0208",	SK_UB_3DB+x0208_index},
    {"x0208",		SK_UB_3DB+x0208_index},
    {"oldjis",		SK_UB_3DB+jisc6226_78_index},
    {"jisx0212",	SK_UB_4DB+x0212_index},
    {"jisx0213-2",	SK_UB_4DB+x0213_2_index},
    {"jisx0213n",	SK_UB_4DB+x0213_1n_index},
    {"x0213n",		SK_UB_4DB+x0213_1n_index},
    {"jisx0213",	SK_UB_4DB+x0213_1_index},
    {"chinese",		SK_UB_4DB+gb2312_index},
    {"ks_c_5601",	SK_UB_4DB+ksc5601_index},
    {"korian",		SK_UB_4DB+ksc5601_index},
    {"cns11643",	SK_UB_4DB+cns11643_1_index},
    {"iso-8859-11",	SK_UB_8859+iso8859_11_index},
    {"latin1-2-5",	SK_UB_8859+iso8859_s_index},
    {"latin10",		SK_UB_8859+iso8859_16_index},
    {"latin9",		SK_UB_8859+iso8859_15_index},
    {"latin8",		SK_UB_8859+iso8859_14_index},
    {"latin1",		SK_UB_8859+iso8859_1_index},
    {"latin2",		SK_UB_8859+iso8859_2_index},
    {"latin3",		SK_UB_8859+iso8859_3_index},
    {"latin4",		SK_UB_8859+iso8859_4_index},
    {"latin5",		SK_UB_8859+iso8859_9_index},
    {"latin6",		SK_UB_8859+iso8859_10_index},
    {"cyrillic",	SK_UB_8859+iso8859_5_index},
    {"asmo-708",	SK_UB_8859+iso8859_6_index},
    {"ecma-114",	SK_UB_8859+iso8859_6_index},
    {"arabic",		SK_UB_8859+iso8859_6_index},
    {"greek8",		SK_UB_8859+iso8859_7_index},
    {"greek",		SK_UB_8859+iso8859_7_index},
    {"elot-928",	SK_UB_8859+iso8859_7_index},
    {"ecma-118",	SK_UB_8859+iso8859_7_index},
    {"hebrew",		SK_UB_8859+iso8859_8_index},
    {"csa7-1",		SK_UB_UNI+csa_z243_1_index},
    {"csa7-2",		SK_UB_UNI+csa_z243_2_index},
    {"ecma94-cyrillic",	SK_UB_8859+ecma113_c_index},
    {"koi8-e",		SK_UB_8859+ecma113_c_index},
    {"st-sev-358",	SK_UB_8859+gost19768_index},
    {"koi8r",		SK_UB_MISC+koi8_index},
    {"cp819",		SK_UB_8859+iso8859_1_index},
    {"cp20866",		SK_UB_MISC+koi8_index},
    {"cp878",		SK_UB_MISC+koi8_index},
    {"cp20866",		SK_UB_MISC+koi8_index},
    {"csinvaliant",	SK_UB_IM2+iso646_invar_index},
    {"viscii",		SK_UB_MISC+viscii_index},
    {"cp1250",		SK_UB_MISC+cp1250_index},
    {"cp1251",		SK_UB_MISC+ms_cp1251_index},
    {"cp1252",		SK_UB_MISC+cp1252_index},
    {"cp1253",		SK_UB_MISC+cp1253_index},
    {"cp1254",		SK_UB_MISC+cp1254_index},
    {"cp1255",		SK_UB_MISC+cp1255_index},
    {"cp1256",		SK_UB_CP+cp1256_index},
    {"cp1257",		SK_UB_CP+cp1257_index},
    {"rk1048",		SK_UB_CP+kz1048_index},
    {"cp1258",		SK_UB_MISC+cp1258_index},
    {"windows-874",	SK_UB_MISC+cp874_index},
    {"windows-943",	SK_UB_MISC+cp943_index},
    {"windows-949",	SK_UB_MISC+cp949_index},
    {"cp367",		SK_UB_UNI+ascii_index},
    {"cp500",		SK_UB_MISC+cp775_index},
    {"cp775",		SK_UB_MISC+cp775_index},
    {"cp852",		SK_UB_MISC+cp852_index},
    {"cp790",		SK_UB_MISC+cp852_index}, 
    {"mazovia",		SK_UB_MISC+cp852_index}, 
    {"kamenicky",	SK_UB_MISC+cp895_index}, 
    {"cp874",		SK_UB_MISC+cp874_index},
    {"cp862",		SK_UB_MISC+cp862_index},
    {"ibm866",		SK_UB_MISC+cp866_index},
    {"cp869",		SK_UB_MISC+cp869_index},
    {"cp943",		SK_UB_MISC+cp943_index},
    {"cp949",		SK_UB_MISC+cp949_index},
    {"cuba",		SK_UB_IM2+cuban_spanish_index},
    {"lap",		SK_UB_8859+iso4873_index},
    {"uhc",		SK_UB_MISC+cp949_index},
    {"alt",		SK_UB_MISC+cp866_index},
    {"mac",		SK_UB_MISC+mac_roman_index},
    {"irv",		SK_UB_UNI+iso646_irv_index},
    {"se2",		SK_UB_UNI+sen8502_c_index},
    {"it",		SK_UB_UNI+iso646_it_index},
    {"dk",		SK_UB_CP+ds2089_index},
    {"es",		SK_UB_UNI+ecma114_sp_index},
    {"yu",		SK_UB_UNI+serb_slov_index},
    {"no",		SK_UB_UNI+ns4551_index},
    {"no2",		SK_UB_UNI+ns4551_2_index},
    {"fl",		SK_UB_UNI+sen8502_b_index},
    {"se",		SK_UB_UNI+sen8502_b_index},
    {"js",		SK_UB_UNI+serb_slov_index},
    {"jp",		SK_UB_UNI+x0201_index},
    {"us",		SK_UB_UNI+ascii_index},
    {" ",-1}
};

/* codeset option: set of charset to use as output specification   */
/* Note: codeset name is case *insensitive*			   */
/* Note2: this table is also used for MIME charset name		   */
const struct long_option pre_codeset_option_code[] = {
    {"utf-8n",		codeset_utf8},
    {" ",-1}
};

const struct long_option codeset_option_code[] = {
    {"ascii-8bit",	codeset_binary},
    {"ascii",		codeset_ascii},	
    {"ansi-x3.4-1986",	codeset_ascii},	
    {"ansi-x3.4-1968",	codeset_ascii},	
    {"ansi-x3.4",	codeset_ascii},	
    {"iso646-us",	codeset_ascii},	
    {"roman8",		codeset_ascii},	
    {"iso2022-jp-ms",	codeset_jisms},
    {"iso2022-jp-1",	codeset_x0208},
    {"iso2022-jp-2004", codeset_x213a}, /* X-0213(2004)	   */
    {"iso2022-jp-2003", codeset_x213a}, /* X-0213(2004)	   */
    {"iso2022-jp-arib",  codeset_aribb24},/* ARIB-B24-SI alias(from citrus)   */
    {"iso2022-jp-3-compat",   codeset_x0213_s}, /* X-0213 strict  */
    {"iso2022-jp-3-plane1",   codeset_x0213_s}, /* X-0213 strict  */
    {"iso2022-jp-3-2012", codeset_x213a}, /* X-0213(2004/2012) relax	   */
    {"iso2022-jp-3-2004", codeset_x213a}, /* X-0213(2004/2012) relax	   */
    {"iso2022-jp-3-2003", codeset_x213a}, /* X-0213(2004) relax	   */
    {"iso2022-jp-3-2000",  codeset_x0213},/* simple X-0213 (2000)  */
    {"iso2022-jp-3",  codeset_x213a},/* simple X-0213 (2004/2012)  */
    {"iso2022-arib",  codeset_aribb24},/* ARIB-B24-SI alias(from citrus)   */
    {"euc7-kr",		codeset_kr},
    {"iso-ir-100",	codeset_8859_1},
    {"iso-ir-101",	codeset_8859_2},
    {"iso-ir-109",	codeset_8859_3},
    {"iso-ir-110",	codeset_8859_4},
    {"iso-ir-144",	codeset_8859_5},
    {"iso-ir-127",	codeset_8859_6},
    {"iso-ir-126",	codeset_8859_7},
    {"iso-ir-148",	codeset_8859_9},
    {"iso-ir-157",	codeset_8859_10},
    {"iso-ir-58",	codeset_cn},
    {"asmo-708",	codeset_8859_6},
    {"ecma-114",	codeset_8859_6},
    {"arabic",		codeset_8859_6},
    {"ecma-118",	codeset_8859_7},
    {"elot-928",	codeset_8859_7},
    {"greek",		codeset_8859_7},
    {"jisx0213n",	codeset_x213a},
    {"jis-x0201-ro",	codeset_x0208},
    {"jis-x0201",	codeset_x0208},
    {"jis-x0213-2004",	codeset_x213a},
    {"jis-x0213-2000",	codeset_x0213},
    {"jis-x0213",	codeset_x0213},
    {"jis-x0208-nj",	codeset_x0208nj},
    {"jis-x0208",	codeset_x0208},
    {"jis",		codeset_x0208},
    {"tis620",		codeset_8859_11}, 
    {"cstis620",	codeset_8859_11}, 
    {"latin10",		codeset_8859_16}, 
    {"latin1",		codeset_8859_1}, 
    {"latin2",		codeset_8859_2}, 
    {"latin3",		codeset_8859_3}, 
    {"latin4",		codeset_8859_4}, 
    {"latin5",		codeset_8859_9}, 
    {"latin6",		codeset_8859_10}, 
    {"latin7",		codeset_8859_13}, 
    {"latin8",		codeset_8859_14}, 
    {"latin9",		codeset_8859_15}, 
    {"x208",		codeset_x0208}, /* discouraged		   */
    {"x0208-nj",	codeset_x0208nj}, /* discouraged	   */
    {"x0208",		codeset_x0208}, /* discouraged		   */
    {"x213",		codeset_x0213}, /* discouraged		   */
    {"x0213",		codeset_x0213}, /* discouraged		   */
    {"junet",		codeset_x0208}, /* discouraged		   */
    {"ks-c5601",	codeset_kr},
    {"ks-x1001",	codeset_kr},
    {"ks",		codeset_kr},
    {"uhc",		codeset_uhc},
    {"gb2312-80",	codeset_cn},
    {"gb2312",		codeset_cn},
    {"gbf",		codeset_gb12},
    {"gb2k",		codeset_gb18},
    {"gb",		codeset_cn},
    {"koi8",		codeset_koi8r},
    {"koi",		codeset_koi8r}, /* discouraged		   */
    {"cyrillic",	codeset_8859_5}, 
    {"greek",		codeset_8859_7}, 
    {"extended-unix-code-packed-format-for-japanese",codeset_eucjp},
    {"eucjp-ascii",	codeset_cp51932},
    {"eucjp-ms",	codeset_cp51932},
    {"eucjp-arib",	codeset_aribb24},
    {"eucjp",		codeset_eucjp},
    {"ujis",		codeset_eucjp},
    {"cns11643",	codeset_euctw},
    {"x-euc-cn",	codeset_euccn},
    {"cn-gb",		codeset_euccn},
    {"euc-cns",		codeset_euctw},	/* CNS 11643 */
    {"euc-x0213-2003",	codeset_euc_213a},
    {"euc-x0213-2004",	codeset_euc_213a},
    {"euc-x0213-2012",	codeset_euc_213a},
    {"euc-jis-2003",	codeset_euc_213a},
    {"euc-jis-2004",	codeset_euc_213a},
    {"euc-jis-2012",	codeset_euc_213a},
    {"euc-jis-2000",	codeset_euc_213},
    {"euc-jis-3",	codeset_euc_213a},
    {"euc-jisx0213",	codeset_euc_213},
    {"euc",		codeset_eucjp},
    {"x-chinese-eten",	codeset_big5},
    {"big5-hkscs",	codeset_big5p},
    {"big5plus",	codeset_big5p},
    {"big5p",		codeset_big5p},
    {"hz",		codeset_cnhz},
    {"zw",		codeset_cnzw},
    {"shift-jis-2012",	codeset_sj_213a},
    {"shift-jis-2004",	codeset_sj_213a},
    {"shift-jis-2003",	codeset_sj_213a},
    {"shift-jis-2000",	codeset_sj_0213},
    {"shift-jis-arib",	codeset_aribb24s},
    {"sjis-x0213-2003",	codeset_sj_213a},
    {"shift-jisx0213",	codeset_sj_0213},
    {"shift-jis-3",	codeset_sj_213a},
    {"sjis-cellular",	codeset_sjiscl},
    {"sjis-arib",	codeset_aribb24s},
    {"sjis",		codeset_sjis},
    {"ansi",		codeset_sjis},
    {"keis",		codeset_keis},
#if !defined(SWIG_EXT) || defined(HAVE_FAST_LWLSTRLEN)
/*    {"utf-16le-bom",	codeset_utf16leb},	*/
/*    {"utf-16be-bom",	codeset_utf16beb},	*/
/*    {"utf-16le",	codeset_utf16le},	*/
/*    {"utf-16be",	codeset_utf16be},	*/
/*    {"utf-32le",	codeset_utf32le},	*/
/*    {"utf-32be",	codeset_utf32},		*/
    {"utf-16le-nobom",	codeset_utf16le},
    {"utf-16be-nobom",	codeset_utf16be},
    {"utf-16",	codeset_utf16},
/*    {"utf-32le-bom",	codeset_utf32leb},	*/
/*    {"utf-32be-bom",	codeset_utf32beb},	*/
    {"utf-32le-nobom",	codeset_utf32le},
    {"utf-32be-nobom",	codeset_utf32be},
/*    {"utf-32",	codeset_utf32},		*/
    {"ucs2",		codeset_utf16beb},
    {"iso-10646-ucs-2",	codeset_utf16beb},
    {"iso-10646-j-1",	codeset_utf16leb},
    {"unicodebig",	codeset_utf16be},  /* big endian	   */
    {"unicodelittle",	codeset_utf16le},  /* little endian	   */
    {"csunicode",	codeset_utf16le},  /* little endian	   */
    {"x-utf-16le",	codeset_utf16le},  /* little endian	   */
    {"x-utf-16be",	codeset_utf16be}, /* big endian		   */
    {"ibm1200",		codeset_utf16be}, /* big endian w.PUA	   */
    {"ibm1201",		codeset_utf16be}, /* big endian		   */
    {"ibm1202",		codeset_utf16le},  /* little endian w.PUA  */
    {"ibm1203",		codeset_utf16le},  /* little endian	   */
    {"cp1200",		codeset_utf16le},  /* little endian	   */
    {"cp1201",		codeset_utf16be}, /* big endian		   */
    {"unicodefffe",	codeset_utf16be},
    {"unicode",		codeset_utf16le},
#endif
    {"arib-b24-sjis",	codeset_aribb24s},
    {"arib-b24",	codeset_aribb24},
    {"arib",		codeset_aribb24},
    {"utf-8n",		codeset_utf8},
    {"unicode-?.?-utf-7",codeset_utf7},
    {"b-right/v",	codeset_brgt},
    {"b-right",		codeset_brgt},
    {"tron",		codeset_brgt},
    {"jef-kana",	codeset_jef},
    {"ibm-dbcs",	codeset_ibm},
    {"ibm33722",	codeset_eucjp},
    {"ibm5050",		codeset_eucjp},
    {"ibm1392",		codeset_gb18},
    {"ibm1368",		codeset_big5},
    {"ibm1375",		codeset_big5p},
    {"ibm1383",		codeset_euccn},
    {"ibm1208",		codeset_utf8},
    {"ibm1089",		codeset_8859_6},
    {"ibm813",		codeset_8859_7},/* w/o euro update	   */
    {"ibm819",		codeset_8859_1},/* ibm-819		   */
    {"ibm912",		codeset_8859_2},
    {"ibm913",		codeset_8859_3},
    {"ibm914",		codeset_8859_4},
    {"ibm915",		codeset_8859_5},
    {"ibm916",		codeset_8859_8},
    {"ibm920",		codeset_8859_9},
    {"ibm954",		codeset_eucjp},
    {"ibm970",		codeset_euckr}, /* ibm-970		   */
    {"csgb2312",	codeset_cn},
    {"cseucpkdfmtjapanese",	codeset_eucjp},
    {"csbig5",		codeset_big5},
    {"csisolatin1",	codeset_8859_1},
    {"csisolatin2",	codeset_8859_2},
    {"csisolatin3",	codeset_8859_3},
    {"csisolatin4",	codeset_8859_4},
    {"csisolatin5",	codeset_8859_9},
    {"csisocyrillic",	codeset_8859_5},
    {"csisolatinhebrew",codeset_8859_8},
    {"csiso2022jp",	codeset_x0208},
    {"cswindows-31j",	codeset_cp932}, 
    {"cp28591",		codeset_8859_1},
    {"cp50225",		codeset_kr},
    {"cp51949",		codeset_euckr},
    {"cp51936",		codeset_euccn},
    {"cp52936",		codeset_cnhz},
    {"cp54936",		codeset_gb18},
    {"cp1383",		codeset_cn},
    {"cp1370",		codeset_big5m}, /* cp950 + euro		   */
    {"cp1361",		codeset_johab},
    {"cp1208",		codeset_utf8},
    {"cp1089",		codeset_8859_6},
    {"cp037",		codeset_ibm},	/* EBCDIC US		   */
    {"cp367",		codeset_ascii},	
    {"cp813",		codeset_8859_7},
    {"cp819",		codeset_8859_1},/* cp-819		   */
    {"cp912",		codeset_8859_2},
    {"cp913",		codeset_8859_3},
    {"cp914",		codeset_8859_4},
    {"cp915",		codeset_8859_5},
    {"cp916",		codeset_8859_8},
    {"cp920",		codeset_8859_9},
    {"cp921",		codeset_8859_13}, 
    {"cp923",		codeset_8859_15}, 
    {"cp930",		codeset_ibm},	/* EBCDIC IBM-Japanese	   */
    {"cp932",		codeset_cp932}, 
    {"cp936",		codeset_gbk},	/* GBK			   */
    {"cp939",		codeset_ibm},	/* EBCDIC IBM-MBCSJapanese */
    {"cp942",		codeset_sjis78},
    {"cp949",		codeset_uhc},
    {"cp964",		codeset_euctw},
    {"cp970",		codeset_euckr}, /* ibm-970		   */
    {"gsm0338",		codeset_gsm0338}, /* GSM 03.38		   */
    {"tcvn-5712",	codeset_viscii},
    {"windows-54936",	codeset_gb18},
    {"windows-65000",	codeset_utf7},
    {"windows-65001",	codeset_utf8},
    {"windows-1200",	codeset_utf16le},/* little endian	   */
    {"windows-1201",	codeset_utf16be},/* big endian		   */
    {"windows-936",	codeset_gbk},	/* GBK			   */
    {"cesu8",		codeset_utf8},
    {"x-mac-ukrainian",	codeset_maccyrl},
    {"mac-ukrainian",	codeset_maccyrl},
    {"macicelandic",	codeset_maciceln},
    {"macgreek",	codeset_macgreek},
    {"maccyrillic",	codeset_maccyrl},
    {"macturkish",	codeset_macturka},
    {"macgujarati",	codeset_macgujar},
    {"macgurmukhi",	codeset_macgurmu},
    {"macdevanagari",	codeset_macdevang},
    {"macromanian",	codeset_macromna},
    {"maccroatian",	codeset_maccroatian},
    {"macce",		codeset_macCE},
    {"binary",		codeset_binary},
    {"locale",		codeset_locale},
    {"extended-unix-code-packed-format-for-japanese",
    			codeset_eucjp},
    {"cseuckr",		codeset_euckr},
    {"cscp50220",	codeset_cp5022x},
    {"l6",		codeset_8859_10}, 
    {"chinese",		codeset_cn},
    {"pck",		codeset_sjis},
    {"us",		codeset_ascii},	
    {"646",		codeset_ascii},	
    {"jp",		codeset_x0208},
    {"guess",		-1},
    {"?",99},
    {" ",-1}
};

static const struct long_option lang_option_str[] = {
    {"EN",L_EN},	/* English	*/
    {"CN",L_ZH},	/* Chinese	*/
    {"DE",L_DE},	/* German	*/
    {"FR",L_FR},	/* French	*/
    {"JA",L_JP},
    {"JP",L_JP},
    {"KR",L_KO},	/* Korian	*/
    {"RU",L_RU},	/* Russian	*/
    {"UK",L_UK},	/* Ukrainian	*/
    {"US",L_EN},	/* US ascii	*/
    {"VI",L_VI},	/* Vietnamise	*/
    {"ZH",L_ZH},	/* Chinese	*/
    {"UN",L_UNI},	/* Unicode */
    {"?",0x4041},
    {" ",-1}
};

static const struct long_option encode_option_str[] = {
    {"hex-encode",1},
    {"hex-perc-encode",3},
    {"hex-decode",1},
    {"hex-perc-decode",3},
    {"uri",64},
    {"url",64},
    {"uri-docomo",65},
    {"hex",0x1},
    {"cap",0x1},
#if defined(ROT_SUPPORT) && !defined(SWIG_EXT)
    {"rot47",0x2},
    {"rot",0x2},
#endif
    {"mime-q",7},
    {"mime-b-strict",9},
    {"mime-b",8},
    {"mime",6},
#ifdef ACE_SUPPORT
    {"racecode",21},
    {"race",21},
    {"ace",20},
    {"punycode",20},
    {"puny",20},
    {"idn",20},
#endif
    {"base64",32},
    {"oct",33},
    {"rfc2231",34},
    {"qencode",35},
    {"none",36},
    {" ",-1}
};

#if !defined(SWIG_EXT)
/* --- main routine starts! ------------------------------------- */
/*@-compdef@*//*@-maintype@*//*@-globstate@*/
int main(argc, argv)
    int             argc;
    char          **argv;
{
    skfFILE  *fin;
    int  c;
    int	arcc;
    int	skfres = 0;
#ifdef HAVE_GETENV
    char	*c_arg;
#endif
    char	*ofilename;
    mode_t	oldmode = 0;
    int		ofd;
    int		locale_codeset;
#ifdef SKFDEBUG
    int sy;
#endif
#if	defined(NKF_NAMETEST) && !defined(SWIG_EXT)
    char *name_me = argv[0];
    int i,j;
#if defined(_WIN32) || defined(__MINGW32__)
    int k;
#endif
#endif

    res_out_code;
    uni_table_init();
    initialize_error();

    fin = skf_file_buf;		/* allocate fin pointer area	   */
    /*@-onlyunqglobaltrans@*/
    in_file_name = (char *) calloc((size_t)IN_FILE_NAME,sizeof(char));
    inb_file_name = (char *) calloc((size_t)IN_FILE_NAME,sizeof(char));
    outs_file_name = (char *) calloc((size_t)OUT_FILE_NAME,sizeof(char));
    /*@+onlyunqglobaltrans@*/

    if ((in_file_name == NULL) || (inb_file_name == NULL)
    	|| (outs_file_name == NULL)) {
	skferr(SKF_MALLOCERR,(long)10,(long)2); /* will not return  */
    };

#if	defined(NKF_NAMETEST) && !defined(SWIG_EXT)
/*@-type@*/
    {
	j = strnlen(name_me,SKF_TABLE_PATH);
#if defined(_WIN32) || defined(__MINGW32__)
	k = strnlen(SYSTEM_EXEEXT,SKF_TABLE_PATH);
	for (i = 0;i<SKF_TABLE_PATH && (name_me[i] != 0)
			&& (i <= j);i++) {
	    if ((i >= 2) && (k == 0) && (name_me[i-2] == 'n') 
		&& (name_me[i-1] == 'k') && (name_me[i] == 'f')
		&& (name_me[i+1] == 0)) {
		set_nkf_compat; is_name_nkf = TRUE;
		break;
	    } else if ((i >= 3+k) && (name_me[i-(2+k)] == 'n') 
		&& (name_me[i-k-1] == 'k') && (name_me[i-k] == 'f')
		&& (name_me[i-k+1] == '.') && (name_me[i+1] == 0)) {
		set_nkf_compat; is_name_nkf = TRUE;
		break;
	    };
	};
#else
	for (i = 0;i<SKF_TABLE_PATH && (name_me[i] != 0)
			&& (i <= j);i++) {
	    if ((i >= 2) && (name_me[i-2] == 'n') 
		&& (name_me[i-1] == 'k') && (name_me[i] == 'f')
		&& (name_me[i+1] == 0)) {
		set_nkf_compat; is_name_nkf = TRUE;
		break;
	    };
	};
#endif
    };
/*@+type@*/

    if (is_nkf_compat) {
	set_sup_space_conv;	/* space conversion suppress	   */
	set_mime_nkfmode;	/* nkf mime mode		   */
  	set_use_g0ascii; set_use_compat; 
	/* set_par_mscpt; */
	set_latin2null;
	set_lineend_normalize;
	set_pux212;
	set_input_jp_limit;
    } else;
#endif
#ifdef HAVE_GETENV		/* environment argument eval.      */
#ifdef NO_GETEUID
#ifdef HAVE_GETUID
    if (getuid() != 0) {
	if ((c_arg = getenv("SKFENV"))!= NULL) (void)argeval(2,&c_arg,OPTSTR_LIMIT);
	if ((c_arg = getenv("skfenv"))!= NULL) (void)argeval(2,&c_arg,OPTSTR_LIMIT);
    };
#else
    if ((c_arg = getenv("SKFENV"))!= NULL) (void)argeval(2,&c_arg,OPTSTR_LIMIT);
    if ((c_arg = getenv("skfenv"))!= NULL) (void)argeval(2,&c_arg,OPTSTR_LIMIT);
#endif
#else
    /* do not read environment value if running as root		   */
    if (geteuid() != 0) {
	if ((c_arg = getenv("SKFENV"))!= NULL) (void)argeval(2,&c_arg,OPTSTR_LIMIT);
	if ((c_arg = getenv("skfenv"))!= NULL) (void)argeval(2,&c_arg,OPTSTR_LIMIT);
    };
#endif
#endif
    arcc =  argeval(argc,++argv,OPTSTR_LIMIT);     /* get file count specified  */
    if (arcc < 0) { return(0); };
    c = argc - arcc;

/* --- determine output codeset ---------------------------------- */
    if (out_code == codeset_binary) {
    	out_code = -1;
    } else if (out_code == codeset_locale) {
#ifndef SWIG_EXT
    	locale_codeset = get_output_locale();
	if (locale_codeset < 0) {
	    out_undefined(0,SKF_NOOUT);
	    out_code = -1;
	} else out_code = locale_codeset;
#else
	out_code = -1;
#endif
    } else;

    if (out_code < 0) out_code = DEFAULT_O;

    skf_charset_parser(out_code); 
    reset_kanji_shift;

/* --- compatibility various hooks ------------------------------- */
#ifdef SKF196COMPAT
    if (encode_enbl) set_skf196mime;
#endif
    if (is_nkf_compat) {
    	mime_fold_llimit -= 1;
	if (!encode_enbl) set_mimeb_encode;
    };

/* --- codeset specific initialization, include table setting ---- */
    skf_output_table_set();

/* --- reflect in_param into conv_cap -------------------------- */
    if (is_set_ad_ann && !is_ucs_ufam(conv_cap)) set_add_annon;
    else if (is_set_no_ann && !is_ucs_ufam(conv_cap)) res_add_annon;
    else;

    if (is_set_p_iso8859 && !is_ucs_ufam(conv_cap)) set_use_iso8859;
    else if (is_set_p_niso8859 && !is_ucs_ufam(conv_cap)) res_use_iso8859;
    else;

    if (is_par_hk_enbl && !is_ucs_ufam(conv_cap)) {
	set_put_hk_enbl(get_par_hkenbl);
    };

    if (is_ucs_utf16(conv_cap)) {
    if (is_set_out_le && is_ucs_utf16(conv_cap)) set_o_ltl_endian;
    else if (is_set_out_be && is_ucs_utf16(conv_cap)) set_o_big_endian;
    else;
    } else;

    if (is_set_ucs_bom && is_ucs_ufam(conv_cap)) set_o_add_bom;
    else if (is_set_ucs_nobom && is_ucs_ufam(conv_cap)) res_o_add_bom;
    else ;

    if (is_set_mscpt) set_use_ms_compat;
    else if (is_set_no_mscpt) res_use_ms_compat;
    else;

    if (is_set_pux212) set_use_x0212;
    else if (is_set_pnux212) res_use_x0212;
    else;

/* --- output initialize ----------------------------------------- */
    if (oconv_init() < 0) {	/* various initialize		   */
	skferr(SKF_MALLOCERR,(long)10,(long)3);
    } else;

    init_all_stats();

#ifdef FOLD_SUPPORT
/* --- fold value fix -------------------------------------------- */
    fold_value_setup();
#endif

    if ((skf_fileout == FALSE) ||
    	((c >= (argc - 1)) && (is_nkf_overwrite || is_nkf_in_place))) {
	/* not -O or overwrites with no file name given */
#ifdef NEED_BINMODE
	skf_setmode(stdout,O_BINARY,(char *)string_stdout);
#endif
	fout = (skfoFILE *)stdout;
/* --- input/output initialization --- */
	skf_ioinit(fout);
    } else if (!is_nkf_overwrite && !is_nkf_in_place) {
    	/* -O option for nkf					   */
	if (c < (argc - 1)) {	/* has at least one file name	   */
	    ofilename = argv[argc - 2];
	    argc--;
	} else {
	    ofilename = default_outname;
	};
/*@-dependenttrans@*/
	if ((fout = (skfoFILE *)fopen(ofilename,skf_outmode)) == NULL) {
	    skf_openerr(ofilename,1);
	    skf_exit(EXIT_FAILURE);
	};
/*@+dependenttrans@*/
/* --- input/output initialization --- */
	skf_ioinit(fout);
    } else {	/* nkf_overwrite or nkf_in_place		   */
#if defined(__MINGW32__) || defined(SKF_MINGW)
	out_file_name =
		(char *) calloc((size_t)(IN_FILE_NAME+MAX_PATH+20),
					sizeof(char));
	/* 20=filename("skfuuuu.tmp"=11) + Margin(8) + "\0"(1) */
#else
	out_file_name = 
		(char *) calloc((size_t)(IN_FILE_NAME+SUFFIX_LIMIT),
					sizeof(char));
#endif
	outb_file_name = (char *) calloc((size_t)OUT_FILE_NAME,sizeof(char));
	if ((out_file_name == NULL) || (outs_file_name == NULL)
		|| (outb_file_name == NULL)) {
	    skferr(SKF_MALLOCERR,(long)11,(long)2);
	};
    };

/* --- conversion loop ------------------------------------------- */
/*@-branchstate@*/ /*@-usedef@*/
    if (c < (argc - 1)) {		/* loop for multiple file  */
	if (((c == (argc - 2)) && opt_fileshow) || /* single file  */
	    ((c < (argc - 2)) && !(opt_filenoshow))) { /* >= 2file */
	    set_show_filename;
	};
	while (c < (argc - 1)) {	/* open a file.		   */
#ifdef	SKFDEBUG
	    if (is_v_debug)
		fprintf(stderr,"opening input file: %s\n",argv[c]);
#endif
	    strncpy(in_file_name,argv[c],(size_t)(IN_FILE_NAME-1));
					/* save names	   */
	    if ((fin = skf_fopen(in_file_name,skf_inmode)) == NULL) {
		skf_openerr(in_file_name,0);
		skfres = -1;
	    } else {
		if (is_nkf_overwrite || is_nkf_in_place) {
		    strncpy(inb_file_name,in_file_name,
		    		(size_t)(IN_FILE_NAME-1)); 
		    if ((snprintf(out_file_name,(size_t)IN_FILE_NAME,
			    "%s.skftmpXXXXXX",in_file_name)) < 13) {
			/* couldn't generate output file name: FATAL   */
			skferr(SKF_MALLOCERR,(long)11,(long)5);
		    } else;
		    if (snprintf(outb_file_name,(size_t)OUT_FILE_NAME,
		    	"%s%s",inb_file_name,outs_file_name) < 0) {
			skferr(SKF_INTERNALERR,(long)11,(long)6);
		    } else;
		    oldmode = umask(SKF_MASK_VAL);
		    if ((ofd = skf_mkstemp(out_file_name)) < 0) {
			skf_openerr(out_file_name,1);
			skf_exit(EXIT_FAILURE);
		    } else;
			/*@+voidabstract@*/
		    if ((fout = (skfoFILE *)fdopen(ofd,skf_outmode))
		    		== NULL) {
			skf_openerr(out_file_name,1);
			skf_exit(EXIT_FAILURE);
		    } else;
			/*@-voidabstract@*/
		    skf_ioinit(fout);
		} else;
#ifdef	SKFDEBUG
		sy = skf_in_converter(fin);
		if (is_v_debug) fprintf(stderr,"\n[EOF:%d]\n",sy);
#else
		skf_in_converter(fin);
#endif
		(void)skf_fclose (fin);	/* closing file		   */
		if (is_nkf_overwrite || is_nkf_in_place) {
		    struct stat istat;	/*@out@*/
		    struct utimbuf otime;

		    skf_gangfinish((skfoFILE *)fout);

		    SKFfflush((skfoFILE *)fout);
		    (void)fclose(fout);

		    if (stat(in_file_name,&istat)) {
		    	skf_openerr(in_file_name,2);
			skf_exit(EXIT_FAILURE);
		    } else;

		    if (chmod(out_file_name,istat.st_mode)) {
		    	skf_openerr(in_file_name,3);
		    } else;

		    if (is_nkf_in_place) {
		    	otime.actime = istat.st_atime;
			otime.modtime = istat.st_mtime;
			if (utime(out_file_name,&otime)) {
			    skf_openerr(out_file_name,4);
			} else;
		    } else;

		    if (unlink(inb_file_name) < 0) {
		    	skf_openerr(inb_file_name,5);
			skf_exit(EXIT_FAILURE);
		    } else;

		    if (rename(out_file_name,outb_file_name) < 0) {
		    	skf_openerr(out_file_name,6);
			fprintf(stderr," ... %s(%d)\n",outb_file_name,errno);
			skf_exit(EXIT_FAILURE);
		    } else;
		    (void)umask(oldmode);
		} else;
		if (outb_file_name != NULL) {
		    free(outb_file_name);
		} else;
	    };
	    c++;
	};
    } else {		/* no input file is specified		   */
#ifdef	SKFDEBUG
	if (is_v_debug) fprintf(stderr,"read from stdin \n");
#endif
#ifndef SWIG_EXT
	strncpy(in_file_name,string_stdin,(size_t)(IN_FILE_NAME-1));
#endif

#ifdef NEED_BINMODE
	skf_setmode(stdin,O_BINARY,(char *)string_stdin);
#endif

#ifdef FAST_GETC
	*fin = fileno(stdin);
#else
	fin = stdin;	/*  ... then read input from stdin.	   */
    	skf_setvbuf(stdin, stdibuf, I_BUFSIZ);	/* buffer control  */
#endif
#ifdef	SKFDEBUG
	sy = skf_in_converter(fin);
	if (is_v_debug) fprintf(stderr,"\n[EOF(%d)]\n",sy);
#else
	skf_in_converter(fin);
#endif
    };

    if (!input_inquiry && !is_nkf_overwrite && !is_nkf_in_place) {
    	skf_gangfinish((skfoFILE *)stdout);
    } else;

#ifndef SWIG_EXT
    if (skf_fileout && (is_nkf_overwrite || is_nkf_in_place)) {
    	if (out_file_name != NULL) free(out_file_name);
    } else {
	if (out_file_name != NULL) free(out_file_name); /* ??? */
    };
    if (outs_file_name != NULL) free(outs_file_name);
#endif
/*@+branchstate@*/ /*@+usedef@*/

    return (skfres);
}
/*@+compdef@*/
#endif	/* !SWIG_EXT */

/* --------------------------------------------------------------- */
/* conversion common						   */
/* --------------------------------------------------------------- */
int skf_in_converter(fin)
skfFILE *fin;
{
    int	us = sEOF;

    skf_input_lang = skf_get_langcode(skf_output_lang);

    while (TRUE) {
	if ((us = preConvert(fin)) == sEOF) break;
	else if (us == sOCD) {
#ifdef SKFDEBUG
	    if (is_v_debug) fprintf(stderr,"-catched sOCD\n");
#endif
	    continue;
	};
	if (kuni_opt) { res_all_shift;
	    in_codeset = -1; le_detect = 0;
	    skf_in_text_type = 0;
	    if (no_lang_preserve)
		skf_input_lang = skf_get_langcode(skf_output_lang);
	};
    };
#ifndef SUPPRESS_FJ_CONVENSION
    res_single_shift;	/* do not inherit shift between files	   */
#endif 		/* Note: iso-2022 permits shift over lines, but	   */
		/*  not allowed in fj convension 		   */
#ifndef SWIG_EXT
    if (estab_reinit) {
	in_codeset = -1; le_detect = 0;
	res_all_shift;		/* reset in-modes		   */
	if (no_lang_preserve)
		skf_input_lang = skf_get_langcode(skf_output_lang);
    };
#endif
    return(us);
}

/* --------------------------------------------------------------- */
/* option parser						   */
/* --------------------------------------------------------------- */
int argeval(pargc,pargv,limit)
    int         pargc;
    char        **pargv;
    int		limit;
{
/* work for getopt      */
    char        *cp;
    char	*cq = NULL;
    long	parse;
    int		pcode = 0;
    int		ch;
    int		fail = 0;
    int		i;
    int		mime_specified_limit = 0;
    int		argv_pos;

    --pargc;
    argv_pos = 0;
    while ((pargc > 0) && **pargv == '-' && (argv_pos < limit)) {
      cp = *pargv; cp++; argv_pos++;
#ifdef SKFDEBUG
      if (is_vv_debug) fprintf(stderr,"#%s#",cp);
#endif
      if (*cp != '-') {		/* normal option		   */
	 while ((*cp > A_SP) && !fail) {
	  switch(*cp++) {
	    case 'b':   /* buffered in				   */
		unbuf_f = FALSE; continue;
	    case 'u':   /* unbuffered in			   */
		unbuf_f = TRUE; continue;
	    case 'n':   /* 7-bit JIS X 0208(1983)                  */
	    	if (is_nkf_compat) {
		    error_extend_option(SKF_NKFINCOMPAT,cp-1);
		    fail = TRUE;
		} else out_code = codeset_x0208;
		continue;
	    case 'j': 
		out_code = codeset_x0208; continue;
	    case 'a':   /* 8-bit EUC-code with JIS X 0208(1983)    */
	    	if (is_nkf_compat) {
		    error_extend_option(SKF_NKFINCOMPAT,cp-1);
		    fail = TRUE;
		} else out_code = codeset_eucjp;
		continue;
	    case 'e':   /* 8-bit EUC-code with JIS X 0208(1983)    */
		if (is_nkf_compat) {
		    out_code = codeset_cp20932;
		} else {
		    out_code = codeset_eucjp;
		};
		continue;
	    case 'x':
	    	if (is_nkf_compat) {
		    set_kana_call;
		} else out_code = codeset_sjis; 
		continue;
	    case 's':   /* shift-jis output                        */
		if (is_nkf_compat) {
		    out_code = codeset_cp932;
		} else {
		    out_code = codeset_sjis;
		};
		continue;
#if !defined(SWIG_EXT) || (defined(SWIGPYTHON) && defined(SKF_PYTHON3)) || defined(SWIGRUBY) || defined(HAVE_FAST_LWLSTRLEN)
	    case 'q':
		res_jis_flavors; set_new_jis; set_use_x0212; 
		reset_in_endian; set_pout_ucs_bom;
		out_code = codeset_utf16le; continue;
#endif
	    case 'z':
		res_jis_flavors; set_new_jis; set_use_x0212; 
		out_code = codeset_utf8;
		continue;
	    case 'y':
		res_jis_flavors; set_new_jis; set_use_x0212;
		out_code = codeset_utf7; continue;
	    case 'i':   /* kanji-call esc-sequence control         */
#ifndef ENABLEDEPRECATED
		error_extend_option(SKF_DEPRECATOPT,"-i");
		continue;
#else
	    	if (is_nkf_compat) {
		    if (*cp != '\0') {	/* if character is given   */
			ch = *cp++;
			if ((ch >= A_AT) && (ch < A_DEL)) {
			    k_in = ch;
			};
		    } else if (pargc > 0) {
			ch = **(pargv+1);
			if ((ch >= A_AT) && (ch < A_DEL)) {
			    k_in = ch; pargv++; pargc--;
			};
		    } else {
			continue;
		    };
		    continue;
		} else {
		    error_extend_option(SKF_NKFINCOMPAT,cp-1);
		    fail = TRUE;
		};
		break;
#endif
	    case 'o':
#ifndef ENABLEDEPRECATED
		error_extend_option(SKF_DEPRECATOPT,"-i");
		continue;
#else
	    	if (is_nkf_compat) {
		    if (*cp != '\0') {	/* if character is given   */
			ch = *cp++;
			if ((ch >= A_AT) && (ch < A_DEL)) {
			    k_out = ch;
			};
		    } else if (pargc > 0) {
			ch = **(pargv+1);
			if ((ch >= A_AT) && (ch < A_DEL)) {
			    k_out = ch; pargv++; pargc--;
			};
		    };
		    continue;
		} else {
		    error_extend_option(SKF_NKFINCOMPAT,cp-1);
		    fail = TRUE;
		};
		break;
#endif
	    case 'A': if (!is_nkf_compat) {
		    error_extend_option(SKF_NKFINCOMPAT,cp-1);
		    fail = TRUE;
		    continue;
		} else in_codeset = codeset_eucjp;
		continue;
	    case 'E':   /* EUC-code in                             */
		in_codeset = codeset_eucjp;
		continue;
	    case 'N':
	    	if (is_nkf_compat) {
		    error_extend_option(SKF_NKFINCOMPAT,cp-1);
		    fail = TRUE;
		    continue;
		} else in_codeset = codeset_x0208;
		continue;
	    case 'J':   /* 8-bit jis code in                       */
		in_codeset = codeset_x0208;
		continue;
	    case 'S':  	/* shift-jis input			   */
		in_codeset = codeset_sjis;
		continue;
	    case 'X':
		if (is_nkf_compat) {
		    reset_hk_enbl;
		    set_nkf_no_hk;
		} else {
		    in_codeset = codeset_sjis; 
		};
		continue;
#if !defined(SWIG_EXT) || (defined(SWIGPYTHON) && defined(SKF_PYTHON3)) || defined(SWIGRUBY) || defined(HAVE_FAST_LWLSTRLEN)
	    case 'Q':
		in_codeset = codeset_utf16;
		continue;
#endif
	    case 'Y':
		in_codeset = codeset_utf7;
		continue;
#ifdef	FOLD_SUPPORT
	    case 'F':
		set_notrunc_le;	/*@FALLTHROUGH@*/
	    case 'f':
		fold_clap = -1; reset_fold_flat;
		fold_hmgn = -1; fold_omgn = -1;
		for (i=0; (i < 4) && is_digit(*cp);i++) {
		    if (fold_clap < 0) fold_clap  = 0;
		    fold_clap *= 10;
		    fold_clap += (*cp++) - '0';
		};
		if (((*cp) == '-') || ((*cp) == '+')) {
		    if ((*cp) == '+') { set_fold_flat; };
		    cp++; fold_hmgn = 0;
		    for (i=0; (i < 4) && is_digit(*cp);i++) {
			fold_hmgn *= 10; fold_hmgn += (*cp++) - '0';
		    };
		} else ;
		if (((*cp) == '-') || ((*cp) == '+')) {
		    if ((*cp) == '+') { set_noadelim; };
		    cp++; fold_omgn = -1;
		    for (i=0; (i < 2) && is_digit(*cp);i++) {
			fold_omgn *= 10; fold_omgn += (*cp++) - '0';
		    };
		} else ;
		continue;
#endif
#if	defined(ROT_SUPPORT)
	    case 'r': if (is_nkf_compat) {
	    		set_nkf_rotmode;
		      } else {
			set_rot_encode;
		      };
		      continue;
#endif
	    case 'm':   /* MIME encoding in			   */
			/* this option is compatible with nkf,	   */
			/* but inconsistent! input should be	   */
			/* controlled with upper case.		   */
		    if (*cp == 'B') {
			set_base64_encode;
			cp++;
		    } else if (*cp == 'Q') {
			set_hex_qencode; 
			cp++;
		    } else if (*cp == 'S') {
			set_mimeb_strict;
			cp++;
		    } else if (*cp == 'N') {
			set_mimeb_encode;
			cp++;
		    } else if (*cp == '0') {
			reset_encoded;
			cp++;
		    } else {
			set_mimeb_encode;
		    };
		    continue;
	    case 'l': if (is_nkf_compat) {
			    set_use_iso8859;
			    in_codeset = codeset_8859_1;
			    out_code = codeset_8859_1;
			    parse = SG1_INDEX + SK_UB_8859 + iso8859_1_index;
			    skf_codeset_parser(parse);
			    set_euc_protect;
			} else {
			    error_extend_option(SKF_NKFINCOMPAT,cp-1);
			    fail = TRUE;
			};
		continue;
	    case 'c': if (is_nkf_compat) { set_lineend_crlf;
			} else {
			    error_extend_option(SKF_NKFINCOMPAT,cp-1);
			    fail = TRUE;
			};
		continue;
	    case 'd': if (is_nkf_compat) { set_lineend_lf;
			} else {
			    error_extend_option(SKF_NKFINCOMPAT,cp-1);
			    fail = TRUE;
			};
		continue;
	    case 'w': if (is_nkf_compat) {
	    /* endian - UTF32 option should be adjacent to	   */
	    /*	avoid file name with digit confusion		   */
			    res_jis_flavors; set_new_jis; set_use_x0212; 
			    out_code = codeset_utf8; 
			    set_pout_ucs_nobom; 
	    		    if (*cp == '8') {
			    	cp++;
				if (*(cp) == '0') {
				    set_pout_ucs_nobom; 
				    cp++;
				} else {
				    set_pout_ucs_bom;
				};
#if !defined(SWIG_EXT) || (defined(SWIGPYTHON) && defined(SKF_PYTHON3)) || defined(SWIGRUBY) || defined(HAVE_FAST_LWLSTRLEN)
			    } else if ((*cp == '1') 
			    		&& (*(cp+1) == '6')) {
				out_code = codeset_utf16; cp += 2;
				set_out_big_endian;
				set_pout_ucs_bom;
				if ((*cp == 'B') || (*cp == 'L')) {
				    if (*cp == 'L') set_out_ltl_endian;
				    else set_out_big_endian;
				    cp++;
				    if (*cp == '0') {
					set_pout_ucs_nobom; 
					cp++;
				    } else {
					set_pout_ucs_bom; 
				    };
				};
			    } else if ((*cp == '3') 
			    		&& (*(cp+1) == '2')) {
				out_code = codeset_utf32; cp += 2;
				set_out_big_endian;
				set_pout_ucs_bom;
				if ((*cp == 'B') || (*cp == 'L')) {
				    if (*cp == 'L') set_out_ltl_endian;
				    else set_out_big_endian;
				    cp++;
				    if (*cp == '0') {
					set_pout_ucs_nobom; 
					cp++;
				    } else {
					set_pout_ucs_bom; 
				    };
				};
#endif
			    } else {
				out_code = codeset_utf8; 
				set_pout_ucs_nobom;
			    };
			} else {
			    error_extend_option(SKF_NKFINCOMPAT,cp-1);
			    fail = TRUE;
			};
		continue;
	    case 'W': if (is_nkf_compat) {
			    in_codeset = codeset_utf8;
			    if (*cp == '8') { cp++;
			    } else if ((*cp == '1') &&
			    		(*(cp+1) == '6')) {
				in_codeset = codeset_utf16;
				cp += 2; set_in_endian;
				if (*cp == 'L') {
				    cp++; reset_in_endian;
				    ;
				} else if (*cp == 'B') cp++;
			    } else if ((*cp == '3') &&
			    		(*(cp+1) == '2')) {
				in_codeset = codeset_utf32;
				cp += 2; set_in_endian;
				if (*cp == 'L') {
				    cp++; reset_in_endian;
				    ;
				} else if (*cp == 'B') cp++;
			    };
			} else {
			    error_extend_option(SKF_NKFINCOMPAT,cp-1);
			    fail = TRUE;
			};
		continue;
	    case 'M': 
		if (*cp == 'B') {
		    set_o_base64_encode;
		    cp++;
		} else if (*cp == 'Q') {
		    set_o_hex_qencode;
		    cp++;
		} else {
		    set_o_mimeb_encode;
		};
		continue;
	    case 'L':
		if (*cp == 'u') {
		    set_lineend_lf; cp++;
		} else if (*cp == 'm') {
		    set_lineend_cr; cp++;
		} else if (*cp == 'w') {
		    set_lineend_crlf; cp++;
		} else {
		    set_lineend_thru;
		};
		continue;
	    case 'g':
	    	if (is_nkf_compat) {
		    set_input_inquiry; set_estab_reinit;
		} else {
		    error_extend_option(SKF_NKFINCOMPAT,cp-1);
		    fail = TRUE;
		};
		continue;
	    case 'h':	/* hiragana-katakana */
		if ((*cp >= '0') && (*cp <= '9')) {
		    ch = (unsigned int)(*cp) & 0x03U;
		    if (ch == 2) {
			set_nkf_c_katakana;
		    } else if (ch == 3) {
			set_nkf_c_hiragana;
			set_nkf_c_katakana;
		    } else if (ch == 0) {
			    ;
		    } else {
			set_nkf_c_hiragana;
		    };
		    cp++;
		} else {
		    set_nkf_c_hiragana;
		};
		continue;
	    case 'O':
	    	skf_fileout = TRUE; break;
	    case 'I':   /* Inform if input contains non-JIS char.  */
		if (!is_nkf_compat) {
			set_disp_warn;
		} else;
		continue;
	    case 'Z':
	    	if (is_nkf_compat) { 
		    set_ascii_conv;
		    if (skf_kanaconv_parser((int)(*cp)) != 0) cp++;
		} else {
		    in_codeset = codeset_utf8;
		};
		continue;
/* old 'n dirty remains... */
	    case '@':	/*FALLTHROUGH*/
	    case 'T':
		error_extend_option(SKF_NKFINCOMPAT,cp-1);
		fail = TRUE;
		continue;
	    case 'B': 
	    	if (is_nkf_compat) {
		    if (*cp == '1') {
		    	set_nkf_jfbroken; cp++;
		    } else if (*cp == '2') {
		    	set_nkf_jffbroken; cp++;
		    } else {
		    	set_nkf_jbroken;
			if (*cp == '0') cp++;
		    };
		    in_codeset = codeset_x0208;
		    continue;
		} else {
		    error_extend_option(SKF_NKFINCOMPAT,cp-1);
		    fail = TRUE;
		};
		continue;
	    case 'V':	/*FALLTHROUGH*/
	    case 'v':   /* display revision information            */
	        if (*cp == 'v') {	/* -vv			   */
		    display_version(2); cp++;
		} else display_version(1);
		skf_terminate(EXIT_SUCCESS);
#ifdef SKFDEBUG
	    case '%':   /* for debug only                          */
		if ((*cp != '\0') && (*cp <= '9')) {
		  if (*cp > '2') { set_vvv_debug;
		  } else if (*cp > '1') { set_vv_debug;
		  } else set_v_debug;
		} else {
		  set_vvv_debug;
		};
		fprintf(stderr,"Debug opt %d - %s",
			(debug_opt),rev);
		cp++;
		set_disp_warn; continue;
#endif
	    default:
		/* bogus option, ignore if not disp_warn           */
		error_extend_option(SKF_UNDEFOPT,cp-1); fail = TRUE;
		continue;
	 };
	};
      } else {		/* extend option			   */
	cp++;
	if (*cp > A_SP) {
	    parse = skf_option_parser(cp,option_table);
	    if (parse <= M_ILIMIT) {
		switch (parse) {
		  case 2: set_pux212; break;
		  case 3: res_pux212; break;
		  case 4: set_use_compat; break;
		  case 5: reset_use_compat; break;
		  case 6: set_par_mscpt; break;
		  case 7: set_par_no_mscpt; break;
	    /*	  case 60: res_use_ms_compat; break;	*/
		  case 8: set_use_cde_compat; break;
		  case 10: set_out_big_endian ; break;
		  case 11: set_out_ltl_endian ; break;
		  case 12: set_in_endian ; break;
		  case 13: reset_in_endian ; break;
		  case 15: set_pout_ucs_nobom; break;
		  case 16: set_pout_ucs_bom; break;
		/* kanji detect hints */
		  case 17: res_input_x201_kana; break;
		  case 18: set_input_x201_kana; break;
#ifdef DYNAMIC_LOADING
		  case 19: set_in_detect_jis78; break;
#endif
		  case 20: set_no_utf7; break;
		  case 21: set_fuzzy_detect; break;
		  case 22: set_old_nec_compat; break;
		  case 23: set_sup_jis90; break;
		  case 25: set_use_uni_repl; break;
		  case 26: set_quad_char; break;
		  case 28: set_chart_dsbl; break;
		  case 29: set_use_apple_gaiji; break;
		  case 30: set_par_si_enbl; break;
		  case 31: set_par_eightbit; break;
		  case 32: set_par_kana_call; break;
		  case 33: reset_hk_enbl; 
		  	set_nkf_no_hk; break;
		  case 40: 
		      error_extend_option(SKF_DEPRECATOPT,cp);
		      break;
		  case 41: 
		      error_extend_option(SKF_DEPRECATOPT,cp);
		      break;
		  case 42: 
		      error_extend_option(SKF_DEPRECATOPT,cp);
		      break;
		  case 47: set_enable_cellconvert; break;
		  case 48: set_use_old_cell_map; break;
		/* filewise/codewise code detection */
		  case 49: set_estab_reinit; break;
		  case 50: res_estab_reinit; break;
		  case 51: set_kuni_opt; break;
		  case 52: res_kuni_opt; break;
		/* misc controls. */
		  case 53: set_fold_strong; break;
		  case 54: set_use_bg2cc; break;
		  case 55: set_enbl_latin_annon; break;
		  case 56: res_enbl_latin_annon; break;
		  case 65: set_pout_iso8859; break;
		  case 66: set_use_iso8859_1_left; break;
		  case 67: set_use_g0ascii; break;
		  case 69: res_pout_iso8859; break;
		  case 80: set_add_renew; break;
		  case 81: set_lang_tag_encr; break;
		  case 82: set_par_ad_ann; break;
		  case 83: set_lang_tag_encr; break;
		  case 84: set_par_no_ann; break;
		  case 85: set_unchk_utf32_range; break;
		  case 86: set_enable_cesu; break;
		  case 87: set_non_strict_utf8; break;
		  case 90: set_enable_dbl_latin; break;
		  case 91: set_suppress_codesys; break;
		  case 92: set_enable_var_cntl; break;
		  case 94: set_input_jp_limit; break;
		  case 100:	/* reset */
		      preconv_opt = 0;
		      conv_cap = 0; conv_alt_cap = 0;
		      skf_input_lang = 0;
		      skf_output_lang = 0; skf_given_lang = 0;
		      ucod_flavor = 0; ucode_undef = 0;
		      res_debug; codeset_flavor = 0;
		      option_guarding = 0; unbuf_f = 0;
		      in_codeset = -1; 
		      out_code = DEFAULT_O; in_param = 0;
		      le_detect = 0; k_in = (char)0; k_out = (char)0;
		      pref_subst_char = -1;
		      mime_fold_llimit = MIME_ENCODE_LLIMIT;
		      error_opt = 0;
		      reset_o_encode; reset_encoded;
#ifdef FOLD_SUPPORT
		      fold_clap = 0; fold_fclap = 0;
		      fold_omgn = DEFAULT_OIDASH_MGN;
		      fold_hmgn = -1;
#endif
#if	defined(NKF_NAMETEST) && !defined(SWIG_EXT)
		      if (is_name_nkf == FALSE) {
		      	  nkf_compat = 0; 
		      } else;
#else
		      nkf_compat = 0;
#endif
		      break;
		  case 110: set_euc_protect; break;
		  case 111: set_endian_protect; break;
		  case 112: 
		      error_extend_option(SKF_DEPRECATOPT,cp);
		      break;
		  case 114: set_no_lang_preserve; break;
		  case 120: set_latin2html; break;
		  case 121: set_latin2tex; break;
/*		  case 122: set_latin2x0212; break; */
		  case 123: set_latin2htmld; break;
		  case 124: set_latin2htmlh; break;
		  case 125: set_latin2htmlu; break;
		  case 126: set_htmlsanitize;
		  	    set_latin2html;
			    break;
		  case 127: set_force_jis_pri; break;
		  case 128: set_force_private_idn_o; break;
		  case 131: set_sup_space_conv; break;
		  case 132: set_spconv_1; /*@FALLTHROUGH@*/
		  case 133: res_sup_space_conv; break;
		  case 134: set_ascii_conv; break;
		  case 135: set_latin2null; break;
		  case 140: set_lineend_cr; break;
		  case 141: set_lineend_crlf; break;
		  case 142: set_lineend_lf; break;
		  case 143: set_lineend_thru; break;
		  case 144: set_noadelim; break;
#ifdef FOLD_SUPPORT
		  case 146: set_sentence_clip;
		  	set_fold_flat;
			fold_clap = SENTENCE_LIMIT; break;
#endif
		  case 148: set_lineend_normalize;
		  	break;
		  case 150:
			set_input_inquiry;
			set_estab_reinit ;
		  	if (is_nkf_compat) {
			    cq = strchr(cp,'=');
			    if (cq != NULL) {
			    	cq++; 
				if (*cq == '2') {
				    set_input_hard_inquiry;
				} else;
			    } else;
			} else;
			break;
		  case 154: set_input_hard_inquiry;
		  	    set_estab_reinit; break;
		  case 155: set_input_soft_inquiry;
		  	    set_estab_reinit; break;
		  case 156: set_mime_ms_compat; break;
		  case 160: opt_fileshow = TRUE;
		      opt_filenoshow = FALSE; break;
		  case 161: opt_fileshow = FALSE;
		      opt_filenoshow = TRUE; break;
		  case 170: set_limit_ucs2; break;
		  case 171: set_limit_ucs2; set_sup_cjk_cmp;
			set_sup_cjk_ext_a; break;
		  case 172: set_sup_cjk_cmp; break;
		  case 175: set_detect_cr; break;
		  case 176: set_detect_lf; break;
		  case 177: set_detect_crlf; set_first_detect_cr;break;
		  case 180: set_nkf_compat;
			set_sup_space_conv;
		  	set_use_g0ascii; set_use_compat; 
			/* set_par_mscpt; */
			set_mime_nkfmode;
			set_latin2null;
			set_lineend_normalize;
			set_pux212;
			set_input_jp_limit;
			set_uni_kana_concat;
			break;
		  case 181: reset_nkf_compat;
			res_sup_space_conv;
			break;
		  case 184: set_mime_nkfmode; break;
		  case 185: set_lineend_lf;
			    out_code = codeset_x0208; break;
		  case 186: 
			cq = strchr(cp,'=');
			if (cq != NULL) {
			    cq++;
			    for (i=0; (i < 4) && is_digit(*cq);i++,cq++) {
				mime_specified_limit *= 10;
				mime_specified_limit += (*cq) - '0';
			    };
			    if ((mime_specified_limit >= 10) &&
				    (mime_specified_limit < 1000)) {
				mime_fold_llimit = mime_specified_limit;
			    } else;
			} else;
			break;
		  case 188: set_skf196mime; break;
		  case 189: cq = strchr(cp,'=');
		  	if (cq != NULL) {
			    cq++;
			    (void)skf_kanaconv_parser((int)(*cq));
			} else;
			break;
		  case 190: set_start_kanji; break;
		  case 191: set_stripinvis; break;
		  case 192: error_extend_option(SKF_NKFINCOMPAT,cp);
			    break;
		  case 193: set_no_bfc; break;
		  case 194:
			if (is_nkf_compat) {
		  	    set_cp932_ext;
			} else {
			    error_extend_option(SKF_NKFINCOMPAT,cp-1);
			    fail = TRUE;
			};
			break;
		  case 195: 
			if (is_nkf_compat) {
			    set_nocp932_ext;
			} else {
			    error_extend_option(SKF_NKFINCOMPAT,cp-1);
			    fail = TRUE;
			};
			break;
		  case 196: 
			if (is_nkf_compat) {
			    set_nocp932;
			} else {
			    error_extend_option(SKF_NKFINCOMPAT,cp-1);
			    fail = TRUE;
			};
			break;
		  case 197: set_nkf_c_hiragana; break;
		  case 199: set_nkf_c_hiragana;	/*@FALLTHROUGH@*/
		  case 198: set_nkf_c_katakana; break;
		  case 201: display_help(); skf_terminate(EXIT_SUCCESS);
		  case 200: display_version(1); skf_terminate(EXIT_SUCCESS);
		  case 202: test_support_charset();
			skf_terminate(EXIT_SUCCESS);
#ifdef SKFPDEBUG
		  case 203: set_table_dump; break;
#endif
		  case 204: test_support_codeset();
			skf_terminate(EXIT_SUCCESS);
		  case 205: display_version(2); skf_terminate(EXIT_SUCCESS);
		  case 206: set_abt_conv_err; break;
		  case 207: display_nkf_help(); 
			skf_terminate(EXIT_SUCCESS);
#if !defined(SWIG_EXT)
		  case 210: set_nkf_in_place; 	/*@FALLTHROUGH@*/
		  case 211: skf_fileout = TRUE;
		  	set_nkf_overwrite;
			cq = strchr(cp,'=');
			if (cq != NULL) {
			    cq++;
			    strncpy(outs_file_name,cq,
			    		(size_t)(SUFFIX_LIMIT-2));
			    outs_file_name[SUFFIX_LIMIT-1]='\0';
			} else;
			break;
#endif 
		  case 214: set_no_early_mime_out; break;
 		  case 215: set_mime_limit_aware; break;
		  case 216: reset_no_early_mime_out; break;
#ifdef ALTERNATE_TABLE_DIR
		  case 220: skf_terminate(EXIT_FAILURE);
#endif
		  case 240: res_enbl_decomp; break;
		  case 241: set_enbl_decomp; break;
		  case 242: res_enbl_decomp; res_decomp_comp; break;
		  case 243: set_enbl_decomp; set_decomp_comp; break;
		  case 244: res_enbl_decomp; res_decomp_apple; break;
		  case 245: set_enbl_decomp; set_decomp_apple; break;
		  case 250: res_dsbl_ucod_encomp; break;
		  case 251: res_dsbl_ucod_encomp;
		  		res_enbl_ucod_dencomp; break;
		  case 252: set_dsbl_ucod_encomp;
		  		set_enbl_ucod_dencomp; break;
		  case 253: res_enbl_ucod_cencomp; break;
		  case 254: res_uni_kana_concat; break;
		  case 255: set_uni_kana_concat; break;
		  case 256: res_uni_hkna_concat; break;
		  case 257: set_uni_hkna_concat; break;
		  case 260: 
			cq = strchr(cp,'=');
		  	if (cq != NULL) {
			    cq++;
			    viscii_escape = (int)(*(cq++));
			} else;
			break;
		  case 261: 
			cq = strchr(cp,'=');
		  	if (cq != NULL) {
			    cq++;
			    o_viscii_escape = (int)(*(cq++));
			} else;
			break;
		  case 272: set_intext_sgml; break;
		  case 273: set_intext_mail; break;
		  case 274: set_intext_maillike; break;
		  case 275: set_intext_ftext; break;
		  case 276: set_intext_man; break;
		  case 277: set_intext_tex; break;
#if defined(SWIG_EXT) && (defined(SWIGPYTHON) && defined(SKF_PYTHON3))
		  case 282: p_out_binary = 1;
		  case 283: p_out_binary = 0;
#endif
		  case 285: set_arib_bmp0_convert; break;
		  case 286: set_arib_bmp1_convert; break;
		  case 287: set_arib_bmp2_convert; break;
		  case 0:	/*FALLTHROUGH*/
		  default:	/* include -1 */
		      error_extend_option(SKF_UNDEFOPT,cp);
		      skf_terminate(EXIT_FAILURE);
		};
	    } else {
	      if ((parse <= M_PLIMIT) || (parse == ENC_INDEX)
	        || (parse == CST_INDEX) || (parse == DST_INDEX)
		|| (parse == LNG_INDEX) || (parse == DEC_INDEX)
		|| (parse == SG0_INDEX) || (parse == SG1_INDEX)
		|| (parse == SG2_INDEX) || (parse == SG3_INDEX)) {
		cq = strchr(cp,'=');
		if ((cq == NULL) || (*(++cq) == '\0')) {
		    if (pargc > 1) { /* has at least one more  */
			cq = *(++pargv); --pargc;
			if (cq == NULL) {
			    error_extend_option(SKF_NOCSET,"NULL");
			    skf_terminate(EXIT_FAILURE);
			};
		    } else {
			error_extend_option(SKF_NOCSET,"NULL");
			skf_terminate(EXIT_FAILURE);
		    };
		};
		if ((parse == CST_INDEX) || (parse == DST_INDEX)) {
		    if ((pcode = skf_option_parser(cq,pre_codeset_option_code)) < 0) {
		      if ((pcode = skf_search_cname(cq)) < 0) {
			pcode = skf_option_parser(cq,codeset_option_code);
		      } else;
		    } else;
		    if (pcode < 0) {	/* unknown codeset	   */
			error_extend_option(SKF_UNDEFCARGS,cq);
			if (parse == CST_INDEX) pcode = DEFAULT_I;
			else pcode = DEFAULT_O;
		    };
		} else if (parse <= M_PDLIMIT) { /* decimal	   */
		    pcode = 0;
		    for (i=0;(i<4) && (*cq != '\0');i++,cq++) {
		    	pcode = (pcode * 10) + ((*cq) - '0');
		    };
		} else if (parse <= M_PLIMIT) { /* hexadecimal	   */
		    pcode = 0;
		    for (i=0;(i<6) && is_hex_char(*cq);i++,cq++) {
		    	pcode = (pcode << 4) + skf_hex(*cq);
		    };
		} else if (parse == LNG_INDEX) {
		    pcode = skf_option_parser(cq,lang_option_str);
		    if (pcode < 0) {
			error_extend_option(SKF_UNDEFCARGH,cq);
			pcode = L_JA;
		    };
		} else if (parse == DEC_INDEX) {
		    pcode = skf_option_parser(cq,encode_option_str);
		    if (pcode < 0) {
			error_extend_option(SKF_UNDEFOPT,cq);
			skf_terminate(EXIT_FAILURE);
		    };
		} else if (parse == ENC_INDEX) {
		    pcode = skf_option_parser(cq,encode_option_str);
		    if (pcode < 0) {
			error_extend_option(SKF_UNDEFOPT,cq);
			skf_terminate(EXIT_FAILURE);
		    };
		} else {	/* SG's				   */
		    if ((pcode = skf_search_chname(cq)) < 0) 
		      pcode = skf_option_parser(cq,option_code);
		    if (pcode < 0) {
			error_extend_option(SKF_UNDEFCARGH,cq);
			skf_terminate(EXIT_FAILURE);
		    };
		};
		if (parse > M_PLIMIT) parse += pcode;
	      };
	      if ((parse >= CST_INDEX) && (parse <= DST_ILIMIT)) {
		  if ((parse >= CST_INDEX) && (parse <= CST_ILIMIT)) {
		      in_codeset = (int)(parse - CST_INDEX);
		  } else if ((parse >= DST_INDEX) && (parse <= DST_ILIMIT)) {
		      out_code = (int)(parse & ST_P_MASK);
		  };
	      } else if ((parse >= DEC_INDEX) && (parse <= DEC_ILIMIT)) {
		switch (parse - DEC_INDEX) {
		  case 1: set_hex_encode; break;
#ifdef ROT_SUPPORT
		  case 2: set_rot_encode;
			break;
#endif
		  case 3: set_hex_perc_encode; break;
		  case 7: set_mimeq_encode; break;
		  case 6:		/*FALLTHROUGH*/
		  case 8: set_mimeb_encode; break;
		  case 9: set_mimeb_strict; break;
		  case 21: 
		  case 20: set_puny_encode; break;
		  case 32: set_base64_encode; break;
		  case 33: set_oct_encode; break;
		  case 34: set_rfc2231_encode; break;
		  case 35: set_hex_qencode; break;
		  case 36: reset_encoded; break;
		  case 64: set_hex_uri_encode; break;
		  case 65: set_hex_uri_docomo_encode; break;
		  default:
		      error_extend_option(SKF_UNDEFOPT,cp);
		      skf_terminate(EXIT_FAILURE);
		};
	      } else if ((parse >= ENC_INDEX) && (parse <= ENC_ILIMIT)) {
		switch (parse - ENC_INDEX) {
		  case 1: set_o_hex_encode; break;
		  case 3: set_o_hex_perc_encode; break;
#if defined(SKF197) || defined(SKF2ORLATER)
		  case 7: set_o_mimeq_encode; break;
		  case 6:		/*FALLTHROUGH*/
		  case 9:		/*FALLTHROUGH*/
		  case 8: set_o_mimeb_encode; break;
#endif
		  case 20: res_jis_flavors; set_new_jis;
		  	out_code = codeset_puny;
		  	set_use_x0212; out_code = codeset_puny; break;
		  case 32: set_o_base64_encode;break;
		  case 33: set_o_oct_encode; break;
		  case 34: set_o_2231_encode; break;
		  case 35: set_o_hex_qencode; break;
		  case 36: reset_o_encode; break;
		  case 64: res_jis_flavors; set_new_jis;
		  	set_use_x0212; set_o_hex_uri_encode;
			break;
		  default:
		      error_extend_option(SKF_UNDEFOPT,cp);
		      skf_terminate(EXIT_FAILURE);
		};
	      } else if ((parse >= LNG_INDEX) && (parse <= LNG_ILIMIT)) {
			/* setting language parser		   */
		parse = parse & 0x5f5f;	
			/* capitalize by resetting bit5 and 7	   */
		skf_given_lang = (unsigned long)parse;
	      } else if ((parse >= SG0_INDEX) && (parse <= SG_ILIMIT)) {
		  skf_codeset_parser((int)(parse & SG_PH_MASK));
	      } else if (parse <= M_PLIMIT) {
		switch(parse) {
		    case 0x3c0: 
			if ((pcode != 0) && (pref_subst_char < 0x110000))
				pref_subst_char = pcode;
			break;
		    default: 
			error_extend_option(SKF_UNDEFCARGH,cq);
		};
	      } else {
		  error_extend_option(SKF_UNDEFOPT,cp);
		  skf_terminate(EXIT_FAILURE);
	      };
	    };
	} else {	/* -- only				   */
		return(--pargc);	/* indicate option end	   */
	};
      };
      pargv++; pargc--;
      argv_pos = 0;
    };
    return(++pargc);
}

/* --------------------------------------------------------------- */
/* extended option parser					   */
/* - and _ is treated as separater, and ignored during comparing   */
/* --------------------------------------------------------------- */

int skf_option_parser(cq,opt_t)
char	*cq;
const struct long_option	*opt_t;
{
    int	rval = -1;
    int	idx = 0;
    char *option_str;
    char *cp;

    while ((rval < 0) && (opt_t[idx].index >= 0)) {
	cp = cq;
	option_str = opt_t[idx].option;
	if (cname_comp(cp,option_str) >= 0) {
	    rval = opt_t[idx].index;
	    break;
	};
	idx++;
    };

#ifdef SKFDEBUG
    if (is_vv_debug) {
    	if (rval >= 0) 
	    fprintf(stderr,"opt_parse: %d(%x)\n",rval,(unsigned int)rval);
	else 
	    fprintf(stderr,"opt_parse: %d\n",rval);
    } else;
#endif
    return(rval);
}

/* --------------------------------------------------------------- */
#ifndef	SWIG_EXT
static void skf_gangfinish(fout)
skfoFILE *fout;
{
    if (is_jis(conv_cap)) JIS_finish_procedure();
    else if (is_euc(conv_cap)) EUC_finish_procedure();
    else if (is_msfam(conv_cap)) SJIS_finish_procedure();
    else if (is_ucs_utf7(conv_cap)) utf7_finish_procedure();
    else if (is_ucs_utf8(conv_cap)) utf8_finish_procedure();
    else if (is_ucs_utf16(conv_cap)) ucod_finish_procedure();
    else if (out_bg(conv_cap)) BG_finish_procedure();
    else if (is_ucs_brgt(conv_cap)) BRGT_finish_procedure();
    else;

#if 0
/* if any characters hang in encoder, push out these stuffs. 	   */
/* and also put eol if base64 encode.				   */
    oconv(sFLSH);  /* remains should be cleared	   */
#endif

#if defined(FOLD_SUPPORT) 
/* folding at the end of file					   */
    if (is_nkf_compat && (fold_count != 0) && fold_fclap && !notrunc_le) {
#ifdef SKFDEBUG
	if (is_vv_debug) { fprintf(stderr,"#LEADD#");
	} else;
#endif
    	SKFCRLF();
    } else;
#endif

    if (o_encode) encoder_tail();
    SKFfflush((skfoFILE *)fout);
}
#endif

/* --------------------------------------------------------------- */
#ifdef FOLD_SUPPORT
/* --- fold value fix -------------------------------------------- */
void fold_value_setup()
{
    if ((fold_clap < 3) && (fold_clap != 0)) {
    	if (is_nkf_compat) fold_clap = NKF_FOLD;
	else		   fold_clap = DEFAULT_FOLD;
    } else if (fold_clap > 2000) { 
    	fold_clap = 2000; 
    } else ;
    if (fold_clap != 0) {
	fold_clap--;
	if ((fold_hmgn < 0) || (fold_hmgn > 12)) {
	    if (is_nkf_compat) fold_hmgn = NKF_FOLD_WRAP;
	    else		   fold_hmgn = DEFAULT_FOLD_WRAP;
	} else;
	if ((fold_omgn < 0) || (fold_omgn > 12)) {
	    fold_omgn = DEFAULT_OIDASH_MGN;
	} else;
	fold_fclap = fold_clap + fold_hmgn;
    };
}
#endif

/* --------------------------------------------------------------- */
static int skf_kanaconv_parser(cp)
int	cp;
{
    if (cp == '1') {
	res_sup_space_conv; set_spconv_1;
    } else if (cp == '2') {
	res_sup_space_conv; 
    } else if (cp == '3') {
	set_htmlsanitize; 
	set_latin2html;
    } else if (cp == '0') {
    	set_o_prefer_ascii;
    } else if (cp == '4') {
    	set_kanaconv_x0201;
    } else  return(0);
    return(1); 
}

#ifdef SWIG_EXT
/* --------------------------------------------------------------- */
/* parser for swig 						   */
/* --------------------------------------------------------------- */
/* variable re-init						   */
/* --------------------------------------------------------------- */
void skf_script_init()
{
    out_code = -1;

    /* options */
    k_in = 0; k_out = 0;

    conv_cap = 0;		/* output mode pack	 */
    conv_alt_cap = 0;	/* output mode pack+	 */

    reset_encoded; 	/* encode support	 */
    o_encode = 0;
    preconv_opt = 0;	/* preconvert misc opt.  */

    skf_input_lang = 0;	/* input language set	 */
    skf_output_lang = 0;	/* input language set	 */
    out_codeset = -1;	/* output codeset	 */
    in_codeset = -1;	/* input codeset	 */

    option_guarding = 0;	/* option guarding	 */
    ucod_flavor = 0;	/* Unic*de output opt.   */
    nkf_compat = 0;		/* nkf compatibility	 */
    in_param = 0;	/* inputside parameter presave	 */
    codeset_flavor = 0;	/* codeset minor change  */
    debug_opt = 0;

    /* control variables */
    ucode_undef = 0;	/* undefine in unic*de	 */

    init_all_stats();

}

int skf_script_param_parse(optstr,optlen)
char *optstr;
int   optlen;
{
    int		result = 0;
    int		ctail = 256;

    res_out_code;
    uni_table_init();
    initialize_error();
    reset_input_inquiry;

    if (optlen == 0) optlen = OPTSTR_LIMIT;

    while ((*optstr != '\0') && (--ctail > 0)) {
	for (;(is_white(*optstr)); optstr++); /* skip spaces	   */
	result = argeval(2,&optstr,optlen); /* parsing options...  */
	for (;(!is_white(*optstr)) && (*optstr != '\0'); optstr++); /* discard parsed option*/
	if (result < 0) break;
	for (;(is_white(*optstr)); optstr++); /* discard parsed option*/
    };

    if (result >= 0) {
/* --- codeset specific initialization, include table setting ---- */
	skf_output_table_set();
/* --- determine output codeset ---------------------------------- */
	if (out_code < 0) out_code = DEFAULT_O;
	skf_charset_parser(out_code); 
    };

/* --- option parsing is done! ----------------------------------- */
    return (result);
}

#endif
