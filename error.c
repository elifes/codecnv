/* *******************************************************************
 ** Copyright (c) 1997-2015 Seiji Kaneko. All rights reserved.
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
 ** error.c:	error message related routine in skf
 ** $Id: error.c,v 1.142 2017/01/05 15:05:48 seiji Exp seiji $
 **	out_area_out:	error character warning generation
 **	bogus_out:	same as above
 **
 **********************************************************************
 ** Notice for limitation of these converter programming.
 **  Since these routine is called from output converters, these
 **  converter may not use ambiguous characters (can use ascii 
 **  and strict X0208).
 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include "skf.h"
#include "skf_fileio.h"
#include <errno.h>
#include "convert.h"
#include "oconv.h"
#include "in_code_table.h"

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#if	defined(ENABLE_NLS) && defined(HAVE_LIBINTL_H)
#include <libintl.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef HELP_OUTPUT_HELP_OUTPUT
#define skfstderr	stderr
#else
#define skfstderr	stdout
#endif

#define ERRF_LIMIT  (size_t)255

/* ident */
static const char	*cpyr = 
"Copyright (c) S.Kaneko, 1993-2016. All rights reserved.\n";

/* error message text   */
static char	*skf_int_err  = 
"skf: internal error. please report! - code %d\n";
static char	*skf_err_header = "skf: ";
static const char	*skf_err_msg;	/* constant character buffer */
static char	*skf_err_buf;		/* constant character buffer */
static char	*nullconststr = "((null))";

static void	display_version_common P_((int));

#if	defined(ENABLE_NLS) && defined(HAVE_LIBINTL_H)
static int	locale_enable = 0; 
static int	check_format_flaw P_((const char *));

/* --------------------------------------------------------------- */
/*@-mustfreefresh@*/ /*@-statictrans@*//*@-globstate@*/
static const char *skf_gettext(in_str)
    const char	*in_str;
{
    char	*msg_buf;

    if (in_str == NULL) in_str = nullconststr;
    msg_buf = gettext(in_str);
    if ((locale_enable <= 0) || (check_format_flaw(msg_buf) < 0)) {
        return(in_str);
    } else {
        return(msg_buf);
    };
}
#else
#define skf_gettext(x)	x
#endif
/*@+mustfreefresh@*/

/* --------------------------------------------------------------- */
/*@-formatconst@*/
static void	trademark_warn()
{
    skf_err_msg = skf_gettext
        ("\nCodeset names may include trademarks and hereby acknowledged.\n");
    fprintf(skfstderr,skf_err_msg);
}

/* --------------------------------------------------------------- */
#if SKFDEBUG
static void entry_dump(entry)
    struct iso_byte_defs *entry;
{
    fprintf(skfstderr," %s(%lnx)\n",entry->desc,
            ((entry->unitbl == NULL) ? 
             (long int *)(entry->uniltbl) :
             (long int *)(entry->unitbl)));
}
#endif
/* ---------------------------------------------------------------
   in_undefined(), out_undefined(): error reporting 
   these routines are new and self-explanatory error routine.
 */
/* in_undefined: input code is not convertable to unic*de	   */
void in_undefined(c1,reason)
                            skf_ucode c1;
                            int	reason;
{
#ifdef SKFDEBUG
    if ((disp_warn || force_disp_warn || (is_v_debug)) && !input_inquiry) {
#else
        if ((disp_warn || force_disp_warn) && !input_inquiry) {
#endif
            switch (reason) {
                case SKF_IUNDEF:
                    skf_err_msg = skf_gettext("skf: This code(%4x) is undefined in ");
                    fprintf(skfstderr,skf_err_msg,c1);
                    skf_incode_display(); fprintf(skfstderr,"\n");
                    break;
                case SKF_IBROKEN:
                    skf_err_msg = skf_gettext("skf: This code(%4x) is broken in ");
                    fprintf(skfstderr,skf_err_msg,c1);
                    skf_incode_display(); fprintf(skfstderr,"\n");
                    break;
                case SKF_IRGTUNDEF:
                    skf_err_msg = skf_gettext("skf: not in right plain code set (%4x)");
                    fprintf(skfstderr,skf_err_msg,c1);
                    fprintf(skfstderr,"\n");
                    break;
                case SKF_UNDEFCSET:
                    skf_err_msg = skf_gettext("skf: undefined code set is specified\n");
                    (void)fputs(skf_err_msg, skfstderr);
                    break;
                case SKF_NOSURRG:
                    skf_err_msg = 
                        skf_gettext("skf: Lower surrogate is missing. upper is u:%4x");
                    fprintf(skfstderr,skf_err_msg,c1);
                    fprintf(skfstderr,"\n");
                    break;
                case SKF_OUTTABLE:
                    skf_err_msg = 
                        skf_gettext("skf: code(%4x) is not in specified code set\n");
                    fprintf(skfstderr,skf_err_msg,c1);
                    break;
                case SKF_UNSPRT:
                    skf_err_msg = skf_gettext(
                            "skf: code(%4x) is defined, but cannot convert to Unicode(TM)\n");
                    fprintf(skfstderr,skf_err_msg,c1);
                    break;
                case SKF_UND_MIME:
                    skf_err_msg = 
                        skf_gettext("skf: undefined mime codeset\n");
                    fprintf(skfstderr,skf_err_msg);
                    break;
                case SKF_MIME_ERR:
                    skf_err_msg = 
                        skf_gettext("skf: error in mime decoding\n");
                    fprintf(skfstderr,skf_err_msg);
                    break;
                case SKF_UNEXPEOF:
                    skf_err_msg = 
                        skf_gettext("skf: unexpected EOF and residual code(%4x)\n");
                    fprintf(skfstderr,skf_err_msg,c1);
                    break;
                case SKF_DECODERR:
                    skf_err_msg = 
                        skf_gettext("skf: decoding failed (%x)\n");
                    fprintf(skfstderr,skf_err_msg,c1);
                    break;
                case SKF_DECINCONS:
                    skf_err_msg = 
                        skf_gettext("skf: decoding is unsupported under UTF-16\n");
                    fprintf(skfstderr,skf_err_msg);
                    break;
                case SKF_UNILANGER:
                    skf_err_msg = 
                        skf_gettext("skf: Unicode(TM) Language tag is inconsistent\n");
                    fprintf(skfstderr,skf_err_msg);
                    break;
                case SKF_ARIBERR:
                    skf_err_msg = 
                        skf_gettext("skf: This ARIB B24 feature(%4x) is not supported\n");
                    fprintf(skfstderr,skf_err_msg,c1);
                    break;
                case SKF_UNSUPP:
                    skf_err_msg = 
                        skf_gettext("skf: This code(%08x) feature is not supported\n");
                    fprintf(skfstderr,skf_err_msg,c1);
                    break;
                default:
                    skf_err_msg = skf_gettext(skf_int_err);
                    fprintf(skfstderr,skf_err_msg,c1);
                    break;
            };
        };
        if (!input_inquiry) {
            switch (reason) {
                case SKF_MIME_ERR:	/* FALLTHROUGH */
                case SKF_UND_MIME:	/* FALLTHROUGH */
                case SKF_UNILANGER:
                    break;
                default:
                    oconv(ucode_undef);
                    break;
            };
        };
#ifdef SWIG_EXT
        if (reason < SKF_MALLOCERR) skf_swig_result = reason;
#endif
    }

    /* out_undefined: output code is not convertable to code specified */
    void out_undefined(c1,reason)
        skf_ucode c1;
    int	reason;
    {
        int	donotout = 0;

#ifdef SKFDEBUG
        if ((disp_warn || force_disp_warn || (is_v_debug)) && !input_inquiry) {
#else
            if ((disp_warn || force_disp_warn) && !input_inquiry) {
#endif
                switch (reason) {
                    case SKF_IOUTUNI:
                        skf_err_msg = skf_gettext(
                                "skf: This Unicode(TM) area code(u:%x) is undefined or unsupported - ");
                        fprintf(skfstderr,skf_err_msg,c1);
                        skf_outcode_display(); fprintf(skfstderr,"\n");
                        break;
                    case SKF_KANAUNDEF:
                        skf_err_msg = skf_gettext("skf: This code(u:%4x) is undefined - ");
                        fprintf(skfstderr,skf_err_msg,c1);
                        skf_outcode_display(); fprintf(skfstderr,"\n");
                        break;
                    case SKF_UX0212:
                        skf_err_msg = skf_gettext("skf: JIS X-0212 is disabled under ");
                        (void)fputs(skf_err_msg, skfstderr);
                        skf_outcode_display(); fprintf(skfstderr,"\n");
                        break;
                    case SKF_OUNDEF:
                        skf_err_msg = skf_gettext("skf: This code(u:%x) is undefined under ");
                        fprintf(skfstderr,skf_err_msg,c1);
                        skf_outcode_display(); fprintf(skfstderr,"\n");
                        break;
                    case SKF_UCOMPAT:
                        skf_err_msg = skf_gettext(
                                "skf: This code(u:%4x) area is explicitly suppressed under ");
                        fprintf(skfstderr,skf_err_msg,c1);
                        skf_outcode_display(); fprintf(skfstderr,"\n");
                        break;
                    case SKF_UNSURG:
                        skf_err_msg = 
                            skf_gettext("skf: code(%8x) is not within UTF-32 area\n");
                        fprintf(skfstderr,skf_err_msg,c1);
                        break;
                    case SKF_NOOUT:
                        skf_err_msg = 
                            skf_gettext("skf: this codeset output is not supported - ");
                        fprintf(skfstderr,skf_err_msg);
                        skf_outcode_display(); fprintf(skfstderr,"\n");
                        donotout = 1;	/* could not handle output properly. */
                        break;
                    case SKF_UNSUPP:
                        skf_err_msg = 
                            skf_gettext("skf: iscii extension code is not supported\n");
                        fprintf(skfstderr,skf_err_msg);
                        break;
                    case SKF_NOTABLE:
                        skf_err_msg = 
                            skf_gettext("skf: this code point(u+%04x) is not supported\n");
                        fprintf(skfstderr,skf_err_msg);
                        break;
                    case SKF_ENC_ERR:
                        skf_err_msg = 
                            skf_gettext("skf: encoding and output codeset is inconsistent\n");
                        fprintf(skfstderr,skf_err_msg);
                        donotout = 1;	/* could not handle output properly. */
                        break;
                    case SKF_OUT_PROHIBIT:
                        skf_err_msg = 
                            skf_gettext("skf: this code point(u+%04x) output is prohibited\n");
                        fprintf(skfstderr,skf_err_msg,c1);
                        break;
                    default:	/* various dirty errors .... */
                        skf_err_msg = skf_gettext(skf_int_err);
                        fprintf(skfstderr,skf_err_msg,c1);
                        donotout = 1;	/* could not handle output properly. */
                };
            };
#ifndef	UNDEF_NOCLASH
            if ((ucode_undef != 0) && !(is_o_encode) && (c1 >= 0) && (donotout == 0)) {
                post_oconv(ucode_undef);
            } else if (c1 >= 0) {
                post_oconv('.');
                post_oconv('.');
            } else;
#else
            if (c1 >= 0) {
                post_oconv('.');
                post_oconv('.');
            } else;
#endif
#ifdef SWIG_EXT
            if (reason < SKF_MALLOCERR) skf_swig_result = reason;
#endif
        }

        /* in_tablefault(): input side table loading fault */

        void in_tablefault(reason,t_desc)
            int	reason;
        const char	*t_desc;
        {

            if (t_desc == NULL) t_desc = nullconststr;
            if (disp_warn || force_disp_warn) {
                switch (reason) {
                    case SKF_PRESETFAIL:
                        skf_err_msg = 
                            skf_gettext("skf: dynamic codeset(%s) pre-loading failed\n");
                        fprintf(skfstderr,skf_err_msg,t_desc);
                        break;
                    case SKF_TBLUNDEF:
                        skf_err_msg = skf_gettext(
                                "skf: code set(%s) is defined, but convert table does not exist.\n");
                        fprintf(skfstderr,skf_err_msg,t_desc);
                        break;
                    case SKF_TBLNSUPPRT:
                        skf_err_msg = 
                            skf_gettext("skf: unsupport for dynamic loading (%s)\n");
                        fprintf(skfstderr,skf_err_msg,t_desc);
                        break;
                    case SKF_TBLSHORT:
                        skf_err_msg = skf_gettext(
                                "skf: code set(%s) is defined, but unexpected EOF in table read.\n");
                        fprintf(skfstderr,skf_err_msg,t_desc);
                        break;
                    case SKF_TBLBROKN:
                        skf_err_msg = skf_gettext(
                                "skf: code set(%s) is defined, but convert table read failed.\n");
                        fprintf(skfstderr,skf_err_msg,t_desc);
                        break;
                    case SKF_TBLINCNSIS:
                        skf_err_msg = skf_gettext(
                                "skf: code set(%s) definition and convert table does not match.\n");
                        fprintf(skfstderr,skf_err_msg,t_desc);
                        break;
                    case SKF_OTBLINCNSIS:
                        skf_err_msg = skf_gettext(
                                "skf: output codeset definition and convert table does not match at region (%s).\n");
                        fprintf(skfstderr,skf_err_msg,t_desc);
                        break;
                    default:
                        skf_err_msg = skf_gettext(skf_int_err);
                        fprintf(skfstderr,skf_err_msg,reason);
                };
            };
#ifdef SWIG_EXT
            if (reason < SKF_MALLOCERR) skf_swig_result = reason;
#endif
        }

        /* out_tablefault(): output side table loading fault */
        void out_tablefault(reason)
            int reason;
        {
            if (disp_warn || force_disp_warn) {
                switch (reason) {
                    case SKF_ACEBUFOUT:
                        skf_err_msg = skf_gettext("skf: ace buffer overflow\n");
                        fprintf(skfstderr,skf_err_msg);
                        break;
                    case SKF_NOOUT:
                        skf_err_msg = 
                            skf_gettext("skf: this codeset output is not supported - ");
                        fprintf(skfstderr,skf_err_msg);
                        skf_outcode_display(); fprintf(skfstderr,"\n");
                        break;
                    default:
                        skf_err_msg = skf_gettext(skf_int_err);
                        fprintf(skfstderr,skf_err_msg,reason);
                };
            };
        }

        /* ---------------------------------------------------------------
         */
        void display_help()
        {
#if defined(SWIG_EXT)
            if (is_nkf_compat) {
                printf("Usage:\tskf\t%s [--] [file]...\n\n",
                        "[-aefghjmnsvwxzAEFIJLMSWXZ] [extended_option] ");
            } else {
                printf("Usage:\tskf\t%s [--] [file]...\n\n",
                        "[-aefhjnsvwxzAEFINSXYZ] [extended_option] ");
            };
#else
            if (is_nkf_compat) {
                printf("Usage:\tskf\t%s [--] [file]...\n\n",
                        "[-abefghjmnqsuvwxzAEIJLMQSWXZ] [extended_option] ");
            } else {
                printf("Usage:\tskf\t%s [--] [file]...\n\n",
                        "[-abefhjnsuvwxzAEFINSXYZ] [extended_option] ");
            };
#endif	/* SWIG_EXT */
            skf_err_msg = skf_gettext(
                    "\tj\tOutout code is JIS 7/8 bit\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\ts\tOutput code is Shift JIS\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\te\tOutput code is EUC-JP\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\tz  \tOutput code is Unicode(TM)(UTF-8)\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\tS\tinput character codeset is set to Shift JIS\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\tE\tinput character codeset is set to EUC\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\tJ\tinput character codeset is set to JIS 8bit\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\tZ\tinput character codeset is set to Unicode(TM)(UTF-8)\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext("\t--help\tdisplay this help\n"); 
            printf(skf_err_msg);
            skf_err_msg = skf_gettext("Extended Option\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext("\t--ic=codeset\tinput codeset(ex. koi-8, viqr, iso-8859-2, gb18030)\n"); 
            printf(skf_err_msg);
            skf_err_msg = skf_gettext("\t--oc=codeset\toutput codeset(ex. ibm930, uhc, big5, cp51932)\n"); 
            printf(skf_err_msg);
            skf_err_msg = skf_gettext("\t--show-supported-codeset display supported codeset\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext("\t--nkf-compat\tnkf compatible mode\n"); 
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\tAbout other extended options, please refer man page for skf.\n");
            printf(skf_err_msg);
            skf_err_msg = skf_gettext(
                    "\tSend bug to http://osdn.jp/projects/skf.\n");
            printf(skf_err_msg);
            display_version_common(0);
        }

        void display_version_common(mode)
            int mode;
        {
#ifdef SKFDEBUG
            short debug_opt_save;
#endif

            fprintf(skfstderr,"%s%s",rev,cpyr);
            skf_err_msg = skf_gettext("Default input code:%s   ");
            fprintf(skfstderr,skf_err_msg,i_codeset[DEFAULT_I].desc);
            skf_err_msg = skf_gettext("Default output code:%s ");
            fprintf(skfstderr,skf_err_msg,i_codeset[DEFAULT_O].desc);
#ifdef DEFAULT_EOL_CRLF
            fprintf(skfstderr,"(CRLF)");
#else
#ifdef DEFAULT_EOL_LF
            fprintf(skfstderr,"(LF)");
#else
            fprintf(skfstderr,"(CR)");
#endif
#endif
            (void)fputc('\n',skfstderr);

#ifdef SKFDEBUG
            if (is_v_debug || (mode > 0)) {
#else
                if (mode > 0) {
#endif
                    skf_err_msg = skf_gettext("OPTIONS: ");
                    fprintf(skfstderr,skf_err_msg);
#ifdef FAST_GETC
                    fprintf(skfstderr,"FG ");
#endif
#ifdef HAVE_GETENV
                    fprintf(skfstderr,"ENV ");
#endif
#ifdef OLD_NEC_COMPAT
                    fprintf(skfstderr,"98 ");
#endif
#ifdef UNIFY_ASCII_JIS
                    fprintf(skfstderr,"UFY_A_J ");
#endif
#ifdef UTF_MIME_RECOVERY
                    fprintf(skfstderr,"MIMEREC ");
#endif
#if	defined(ENABLE_NLS) && defined(HAVE_LIBINTL_H)
                    fprintf(skfstderr,"NLS ");
#endif
#ifdef USE_UBUF
                    fprintf(skfstderr,"UBUF ");
#endif
#ifdef KEIS_DETECT
                    fprintf(skfstderr,"KD ");
#endif
#ifdef OLD_KEIS_DETECT
                    fprintf(skfstderr,"OKD ");
#endif
#ifdef DYNAMIC_LOADING
                    fprintf(skfstderr,"DY ");
#endif
#ifdef SUPPRESS_FJ_CONVENSION
                    fprintf(skfstderr,"!FJ ");
#endif
#ifdef SUPPRESS_UNRECOGNIZED_SEQ
                    fprintf(skfstderr,"SUQ ");
#endif
#ifdef _LARGEFILE_SOURCE
                    fprintf(skfstderr,"F64 ");
#endif
#ifdef SKFDEBUG
#ifdef CD_DEBUG
                    fprintf(skfstderr,"DBGC ");
#else
                    fprintf(skfstderr,"DBG ");
#endif
#endif	/* SKFDEBUG */
#ifdef NKF_NAMETEST
                    fprintf(skfstderr,"NN ");
#endif
#ifdef ALLOW_TYPOGRAPHY
                    fprintf(skfstderr,"AA");
#endif
#ifdef _WIN32
                    fprintf(skfstderr,"WIN32 ");
#endif
#ifdef GEN_SCRIPT_SUPRT
#ifdef GEN_SCRIPT_ULM
                    fprintf(skfstderr,"!ULM ");
#else
                    fprintf(skfstderr,"ULM ");
#endif
#endif
#ifdef HAVE_GETUID
#ifdef HAVE_GETEUID
                    fprintf(skfstderr,"EUID ");
#else
                    fprintf(skfstderr,"UID ");
#endif
#endif
#ifdef ENABLE_DISABLED
                    fprintf(skfstderr,"ENDIS ");
#endif
#ifdef SKF_EXPERIMENTAL
                    fprintf(skfstderr,"EXP ");
#endif
                    (void)fputc('\n',skfstderr);
                } else;
                skf_err_msg = skf_gettext("FEATURES: ");
                fprintf(skfstderr,skf_err_msg);
                fprintf(skfstderr,"KN ");
#ifdef UCS2_KANA_SUPPR
                fprintf(skfstderr,"UK ");
#endif
#ifdef UCS2_NORMALIZE
                fprintf(skfstderr,"UN ");
#endif
#ifdef UNI_DECOMPOSE
                fprintf(skfstderr,"NFD ");
#endif
#ifdef KEIS_EXTRA_SUPPORT
                fprintf(skfstderr,"KX ");
#endif
#ifdef ROT_SUPPORT
                fprintf(skfstderr,"ROT ");
#endif
#ifdef ACE_SUPPORT
                fprintf(skfstderr,"ACE ");
#endif
#ifdef FOLD_SUPPORT
                fprintf(skfstderr,"FD ");
#ifdef SUPERFOLD
                fprintf(skfstderr,"SFD ");
#endif
#if defined(SWIG_EXT) && defined(SKF_PYTHON3)
                fprintf(skfstderr,"PY3 ");
#endif
#if defined(SWIG_EXT) && defined(SKF_RUBY19)
                fprintf(skfstderr,"RBC ");
#endif
#endif
                if (is_lineend_thru) fprintf(skfstderr,"LE_THRU ");
                if (is_lineend_crlf) fprintf(skfstderr,"LE_CRLF ");
                if (is_lineend_cr) fprintf(skfstderr,"LE_CR ");
                if (is_lineend_lf) fprintf(skfstderr,"LE_LF ");

                (void)fputc('\n',skfstderr);

#ifdef SKFDEBUG
                if (is_v_debug) {
                    if (skf_input_lang != 0) {
                        fprintf(skfstderr,"lang: %c%c ",
                                (int)((skf_input_lang >> 8) & 0x7fU),
                                (int)(skf_input_lang & 0x7fU));
                    } else fprintf(skfstderr,"lang: neutral ");
#ifdef DYNAMIC_LOADING
                    skf_err_msg = skf_gettext("Code table dir: %s\n");
                    fprintf(skfstderr,skf_err_msg,skf_ext_table);
#endif
                } else;
#endif
                if (is_nkf_compat) {
                    fprintf(skfstderr,"NKFOPT: ");
#ifdef MIME_DECODE_DEFAULT
                    fprintf(skfstderr,"MIME_DECODE ");
#endif
#ifdef X0201_DEFAULT
                    fprintf(skfstderr,"X0201_DEFAULT ");
#endif
#ifdef HELP_OUTPUT_HELP_OUTPUT
                    fprintf(skfstderr,"SKFSTDERR ");
#else
                    fprintf(skfstderr,"STDOUT ");
#endif
#ifdef SHIFTJIS_CP932
                    fprintf(skfstderr,"SJIS_IS_CP932 ");
#endif
                    fprintf(skfstderr,"\n");
                } else;
#ifdef SKFDEBUG
                if (mode > 1) {
                    debug_opt_save = debug_opt;
                    set_vv_debug;
                    debug_analyze();
                    debug_opt = debug_opt_save;
                } else;
#endif
            }

            void display_version(mod)
                int mod;
            {
                display_version_common(mod);
            }
            /* --------------------------------------------------------------- */
            void display_nkf_help()
            {
                skf_err_msg = skf_gettext(
                        "nkf-help:\n following options are compatible\n");
                (void)fputs(skf_err_msg, skfstderr);
                fprintf(skfstderr,"  -j/J -s/S -e/E -b/u -f/F -L -d -c -r -m -M -h -t -v\n");
                fprintf(skfstderr,"  --fj --unix --mac --msdos --windows --jis --euc --sjis --jis-input\n");
                fprintf(skfstderr,"  --euc-input --sjis-input --in-place --overwrite\n");
                skf_err_msg = skf_gettext(
                        " * following options are enabled by default in skf\n  -I -X\n");
                (void)fputs(skf_err_msg, skfstderr);
                skf_err_msg = skf_gettext(
                        " * left side nkf options and right side skf options are compatible\n");
                (void)fputs(skf_err_msg, skfstderr);
                skf_err_msg = skf_gettext(" nkf option        skf option\n");
                (void)fputs(skf_err_msg, skfstderr);
                fprintf(skfstderr," ----------------- ----------------------------------------------\n");
                fprintf(skfstderr," -w                -z\n");               
                fprintf(skfstderr," -W                -Z\n");               
                fprintf(skfstderr," -w16L             --oc=utf-16le-bom\n");
                fprintf(skfstderr," -W16L             --ic=utf-16le\n");   
                fprintf(skfstderr," -w32L             --oc=utf-32le-bom\n");
                fprintf(skfstderr," -W32L             --ic=utf-32le\n");  
                fprintf(skfstderr," -Z0               --enable-ascii-conv\n");
                fprintf(skfstderr," -Z1               --enable-ascii-conv --enable-space-conv\n");
                fprintf(skfstderr," -Z2               --enable-ascii-conv --enable-space-single-conv\n");
                fprintf(skfstderr," -Z3               --enable-ascii-conv --html-sanitize\n");
                fprintf(skfstderr," -l                --ic=iso-2022-jp-2 --oc=iso-2022-jp-2\n");
                fprintf(skfstderr," -x                --kana-enable\n");
                fprintf(skfstderr," --cap-input       --decode=cap\n");
                fprintf(skfstderr," --url-input       --decode=url\n");
                fprintf(skfstderr," --numchar-input   --decode=hex\n");
                fprintf(skfstderr," --guess=1         -g\n");
                fprintf(skfstderr," --guess=2         --hard-inquiry\n");
                fprintf(skfstderr," --ic=utf-8-bom    -z --enable-endian-mark\n");
                fprintf(skfstderr," ----------------- ----------------------------------------------\n");
                skf_err_msg = skf_gettext(
                        " * following options are not supported: -i -o -T --prefix --fb-skip \n");
                (void)fputs(skf_err_msg, skfstderr);
                fprintf(skfstderr,"   --fb-java --no-cp932ext --no-best-fit-chars --ic=utf8-mac\n");
                return;
            }
            /* --------------------------------------------------------------- */
            void error_code_option(code)
                int	code;
            {
                (void)fputs(skf_err_header,skfstderr);/* header		   */
                switch (code) {
                    case SKF_MISCSETOPT:
                        skf_err_msg = skf_gettext("missing character set option!\n");
                        fprintf(skfstderr,skf_err_msg,code);
                        break;
                    case SKF_UNKWNCSTOPT:
                        skf_err_msg = skf_gettext("unknown character set option!\n");
                        fprintf(skfstderr,skf_err_msg,code);
                        break;
                    case SKF_UNKWNCDOPT:
                        skf_err_msg = skf_gettext("unknown code set option!\n");
                        fprintf(skfstderr,skf_err_msg,code);
                        break;
                    default:
                        skf_err_msg = skf_gettext("unknown option(%d)\n");
                        fprintf(skfstderr,skf_err_msg,code);
                };
#ifdef SWIG_EXT
                if (code < SKF_MALLOCERR) skf_swig_result = code;
#endif
                return;
            }

            /*@-branchstate@*/
            void error_extend_option(skferrno,cp)
                int  skferrno;
            char *cp;
            {
                if (cp == NULL) cp = "UNKNOWN";
                switch (skferrno) {
                    case SKF_NKFINCOMPAT:
                        skf_err_msg = skf_gettext(
                                "skf: this option(%s) is not supported by skf.\n");
                        fprintf(skfstderr,skf_err_msg,cp);
                        break;
                    case SKF_UNDEFCARGS:
                        skf_err_msg = skf_gettext(
                                "skf: undefined charset is specified in command line argument (%s)\n");
                        fprintf(skfstderr,skf_err_msg,cp);
                        break;
                    case SKF_UNDEFCARGH:
                        skf_err_msg = skf_gettext(
                                "skf: undefined codeset is specified in command line argument (%s)\n");
                        fprintf(skfstderr,skf_err_msg,cp);
                        break;
                    case SKF_NOCSET:
                        skf_err_msg = skf_gettext(
                                "skf: no codeset is specified in command line argument\n");
                        fprintf(skfstderr,skf_err_msg);
                        break;
                    case SKF_DEPRECATOPT:
                        skf_err_msg = skf_gettext(
                                "skf: this option has been deprecated (%s)\n");
                        fprintf(skfstderr,skf_err_msg,cp);
                        break;
                    case SKF_UNDEFOPT:	/*@FALLTHROUGH@*/
                    default:
                        skf_err_msg = skf_gettext("skf: unknown option %s\n");
                        fprintf(skfstderr,skf_err_msg,cp);
                };
#ifdef SWIG_EXT
                if (skferrno < SKF_MALLOCERR) skf_swig_result = skferrno;
#endif
                return;
            }
            /*@+branchstate@*/

            /* --------------------------------------------------------------- */
            /*@-mustfreefresh@*/
            void initialize_error()
            {
#if	defined(ENABLE_NLS) && defined(HAVE_LIBINTL_H)
                char *bt,*tl;

                if (skf_err_buf == NULL) {
                    if (setlocale(LC_ALL,"") != NULL) {
                        bt = bindtextdomain(PACKAGE,LOCALEDIR);
                        tl = textdomain(PACKAGE);
                        if ((bt == NULL) || (tl == NULL)) {
                            skferr(SKF_MALLOCERR,(long)32,(long)0);
                        };
                        locale_enable = 1;
                    };
                    skf_err_buf = (char *)calloc((size_t)256,sizeof(char));
                    if (skf_err_buf == NULL) {
                        skferr(SKF_MALLOCERR,(long)32,(long)1);
                    };
                } else;
#else
                if (skf_err_buf == NULL) {
                    skf_err_buf = (char *)calloc((size_t)256,sizeof(char));
                    if (skf_err_buf == NULL) {
                        skferr(SKF_MALLOCERR,(long)32,(long)2);
                    };
                } else;
#endif
                return;
            }
            /*@+mustfreefresh@*/

            /* --------------------------------------------------------------- */
            /* skferr: various serious errors				   */
            /* --------------------------------------------------------------- */
            /*@-globstate@*/
            void skferr(code,a1,a2)
                int	code;
            long	a1,a2;
            {
                struct iso_byte_defs *tdefs = NULL;

                if (code >= SKF_ERRDUMP) {
                    skf_err_msg = skf_gettext(skf_int_err);
                    fprintf(stderr,skf_err_msg, code);
                    fprintf(stderr,
                            "dump: (a1: %lx a2: %lx)\n in_code:%d conv_cap:%08lx conv_alt:%08lx\n",
                            a1,a2,in_codeset,conv_cap,conv_alt_cap);
                    dump_table_address(g0_table_mod,"g0");
                    fprintf(stderr,"\n ");
                    dump_table_address(g1_table_mod,"g1");
                    fprintf(stderr,"\n ");
                    dump_table_address(g2_table_mod,"g2");
                    fprintf(stderr,"\n ");
                    dump_table_address(g3_table_mod,"g3");
                    fprintf(stderr,"\n low_table:%08lx\n",(unsigned long)low_table);
                    fprintf(stderr," up_table:%08lx\n",(unsigned long)up_table);
                } else if (code >= SKF_TABLEERR_G0) {
                    switch(code) {
                        case SKF_TABLEERR_G0A:
                            skf_err_msg = skf_gettext(
                                    "Generic g%1dalt table loading error (table: %s)\n");
                            tdefs = gx_table_mod; code++; break;
                        case SKF_TABLEERR_G0:
                        case SKF_TABLEERR_G1:
                        case SKF_TABLEERR_G2:
                        case SKF_TABLEERR_G3:
                        case SKF_TABLEERR_O:
                            skf_err_msg = skf_gettext(
                                    "Generic g%1d table loading error (table: %d)\n");
                            fprintf(stderr,skf_err_msg,code);
                            tdefs = g0_table_mod; break;
                        default:
                            skf_err_msg = skf_gettext(
                                    "unassigned error(%d)\n");
                            fprintf(stderr,skf_err_msg,a1);
                    };
                    (void)fputs(skf_err_header,skfstderr);  /* header		   */
                    fprintf(stderr,skf_err_msg,(code - SKF_TABLEERR_G0),
                            (tdefs == NULL) ? "(null)" : tdefs->desc);
                } else {
                    (void)fputs(skf_err_header,stderr);	/* header	   */
                    switch (code) {
                        case SKF_OBUFREERR:
                            fprintf(stderr,"re-");		/*@FALLTHROUGH@*/
                        case SKF_OBUFERR:	
                            fprintf(stderr,"obuf ");		/*@FALLTHROUGH@*/
                        case SKF_MALLOCERR:
                            skf_err_msg = skf_gettext("failed to allocate buffer(%d-%d)\n");
                            fprintf(stderr,skf_err_msg,a1,a2);
                            break;
                        case SKF_IBUFERR:	
                            fprintf(stderr,"ibuf ");
                            skf_err_msg = skf_gettext("failed to allocate ibuffer(%d-%d)\n");
                            fprintf(stderr,skf_err_msg,a1,a2);
                            break;
                        case SKF_EUCPRESETERR:
                            skf_err_msg = skf_gettext(
                                    "EUC table loading error\n");
                            (void)fputs(skf_err_msg, stderr); break;
                        case SKF_TBLALLOCERR:
                            skf_err_msg = skf_gettext(
                                    "failed to allocate table for conversion\n");
                            (void)fputs(skf_err_msg, stderr);
                            break;
                        case SKF_DECOMPERR:
                            skf_err_msg = 
                                skf_gettext("decompose internal sequencer failed\n");
                            (void)fputs(skf_err_msg, stderr);
                            break;
                        case SKF_INTERNALERR:
                            skf_err_msg = 
                                skf_gettext("decode internal sequencer failed\n");
                            (void)fputs(skf_err_msg, stderr);
                            break;
                        case SKF_PUTFAILERR:
                            skf_err_msg = 
                                skf_gettext("can't send output character\n");
                            (void)fputs(skf_err_msg, stderr);
                            break;
                        case SKF_DEBUGERR_1:
                            skf_err_msg = 
                                skf_gettext("skf debug error %d");
                            fprintf(stderr,skf_err_msg, 1);
                            fprintf(stderr,"(%lx,%lx)\n",a1,a2);
                            break;
                        case SKF_DEBUGERR_2:
                            skf_err_msg = 
                                skf_gettext("skf debug error %d");
                            fprintf(stderr,skf_err_msg, 2);
                            fprintf(stderr,"(%lx,%lx)\n",a1,a2);
                            break;
                        default:
                            skf_err_msg = skf_gettext("unassigned error(%s)\n");
                            fprintf(stderr,skf_err_msg, "default");
                    };
                };
#ifdef SWIG_EXT
                skf_swig_result = code;
#endif
                skf_exit(EXIT_FAILURE);	/* for extra care */
            }

            /* --------------------------------------------------------------- */
            void skf_openerr(fnam,mode)
                char	*fnam;
            int	mode;
            {
                int res;

                (void)fflush(stdout);
                if (fnam == NULL) fnam = nullconststr;
                if (mode == 1) {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: can't open output file %s\n"), fnam);
                } else if (mode == 2) {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: can't stat input file %s\n"), fnam);
                } else if (mode == 3) {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: can't chmod output file %s\n"), fnam);
                } else if (mode == 4) {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: can't set date output file %s\n"), fnam);
                } else if (mode == 5) {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: can't remove input file %s\n"), fnam);
                } else if (mode == 6) {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: can't rename output file %s\n"), fnam);
#ifdef SWIG_EXT
                } else if (mode == 7) {	/* this case is not serious	  */
                    if (disp_warn || force_disp_warn) {
                        res = snprintf(skf_err_buf,255,
                                skf_gettext("skf: can't open input string\n"));
                        (void)fflush(skfstderr);
                    } else;
                    skf_swig_result = SKF_OPENERR;
                    return;
#endif
                } else if (mode == 8) {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: in-place/overwrite are unsupported on this plathome\n"));
                } else if (mode == 9) {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: temp-file name generation failed\n"));
                } else {
                    res = snprintf(skf_err_buf,ERRF_LIMIT,
                            skf_gettext("skf: can't open input file %s\n"), fnam);
                };
                if ((res >= 0) && (res <= 256)) {	/* glibc workaround	   */
                    (void)fputs(skf_err_buf,skfstderr);
                } else {
                    fprintf(skfstderr,"skf: can't open file %s\n", fnam);
                };
                (void)fflush(skfstderr);
#ifdef SWIG_EXT
                skf_swig_result = SKF_OPENERR;
#endif
                return;
            }

            /* --------------------------------------------------------------- */
            void skf_readerr(geterrno)
                int	geterrno;
            {
                (void)fflush(stdout);
                if (geterrno != EAGAIN) (void)fputs(skf_err_header,skfstderr);
                /* header		   */
#ifndef SWIG_EXT
                if (geterrno == EISDIR) {
                    skf_err_msg = skf_gettext("%s is a directory\n");
                    fprintf(skfstderr,skf_err_msg,in_file_name);
                } else if (geterrno == EINVAL) {
                    skf_err_msg = skf_gettext("cannot read from %s\n");
                    fprintf(skfstderr,skf_err_msg,in_file_name);
                } else if (geterrno != EAGAIN) {
                    skf_err_msg = skf_gettext("read error from %s\n");
                    fprintf(skfstderr,skf_err_msg,in_file_name);
                };
#endif
                (void)fflush(skfstderr);
#ifdef SWIG_EXT
                skf_swig_result = SKF_READERR;
#endif
                return;
            }

#if	defined(ENABLE_NLS) && defined(HAVE_LIBINTL_H)
            /* --------------------------------------------------------------- */
            /* check_format_flaw: check po's for format bug avoidance	   */
            /*	format bugs are exploited by inserting %n format in string */
            /* --------------------------------------------------------------- */
            static int check_format_flaw(format_str)
                const char	*format_str;
            {
                int i,j,fstr,perc;

                fstr = FALSE; perc = FALSE;
                for (i=0; i<256; i++) {
                    if ((j = *(format_str+i)) == '\0') {
                        return(1);
                    } else if (j == '%') {
                        if (perc) {		/* detect %%			   */
                            fstr = FALSE; perc = FALSE; 
                        } else {		/* % itself			   */
                            fstr = TRUE; perc = TRUE;
                        };
                    } else if ((j >= '0') && (j <= '9')) {	/* digits	   */
                        perc = FALSE; continue;
                    } else if (j == 'l') {	/* long */
                        perc = FALSE; continue;
                    } else if (j == 'h') {	/* short */
                        perc = FALSE; continue;
                    } else if (j == 'n') {	/* character in issue */
                        if (fstr) return(-1);	/* Toxic!	   */
                        else continue;
                    } else {		/* all else: exit from format	   */
                        fstr = FALSE; perc = FALSE;
                    };
                };
                if (i == 256) return(-1);	/* too long?			   */
                else return(1);
            }
#endif
            /* --------------------------------------------------------------- */
            void	skf_incode_display()
            {
                /* input codeset = output codeset. use output description.	   */
                if ((in_codeset > 0) && (in_codeset < codeset_end)) {
                    fprintf(skfstderr,"%s",i_codeset[in_codeset].desc);
                } else {
                    skf_err_msg = skf_gettext("Unknown(auto detect)");
                    (void)fputs(skf_err_msg,skfstderr);
                };
                if (detect_cr || detect_lf) {
                    fprintf(stderr," ");
                    if (detect_cr) fprintf(stderr,"CR");
                    if (detect_lf) fprintf(stderr,"LF");
                } else;
#ifdef SWIG_EXT
                skf_swig_result = SKF_AUTOFAIL;
#endif
            }

            /* --------------------------------------------------------------- */
            void	skf_outcode_display()
            {
                /* name and language of the codeset */
                if ((out_codeset > 0) && (out_codeset < codeset_end)) {
                    fprintf(skfstderr,"%s (#%d,%x%x,typ:%lx) ",
                            i_codeset[out_codeset].desc,out_codeset,
                            ((i_codeset[out_codeset].oconv_lang) >> 8) & 0x7f,
                            (i_codeset[out_codeset].oconv_lang) & 0x7f,
                            i_codeset[out_codeset].oconv_type);
                } else {
                    skf_err_msg = skf_gettext("Unknown(internal error)");
                    (void)fputs(skf_err_msg,skfstderr);
                };
                (void)fflush(skfstderr);
            }
            /* --------------------------------------------------------------- */
            /*@-branchstate@*//*@+matchanyintegral@*/
            void test_support_charset()
            {
                int i,j;
                struct iso_byte_defs *uentry;
                char *skf_cname;
                char *pad;

                conv_alt_cap = 0;	/* disable other misc message */
                skf_err_msg = skf_gettext
                    ("Supported charset: cname descriptions (* indicate extenal table)\n");
                fprintf(skfstderr,skf_err_msg);

                (void)fflush(skfstderr); (void)fflush(stdout);
                for (i=0; iso_ubytedef_table[i].ientry != NULL; i++) { 
                    /* do not display these stuffs */
                    if ((i == ub_ovlay_index) || (i == ub_prv_drcs_index) ||
                            (i == ub_prv_drcsdbl_index)) continue;
                    fprintf(skfstderr,"# %s:\n",iso_ubytedef_table[i].desc);
                    uentry = iso_ubytedef_table[i].ientry;
                    for (j=0; uentry[j].defschar != 0; j++) {
                        if (uentry[j].desc == NULL) continue;
                        skf_cname = uentry[j].cname;
                        if (skf_cname == NULL) skf_cname = " -  ";
                        if (strlen(skf_cname) < 8) pad = "\t\t";
                        else pad = "\t";
                        if ((uentry[j].unitbl != NULL)
                                || (uentry[j].uniltbl != NULL)) {
                            if (uentry[j].desc != NULL) {
#if SKFDEBUG
                                if (is_v_debug) {
                                    entry_dump(&uentry[i]);
#if 0
                                    fprintf(skfstderr," %s(%08lx)\n",uentry[j].desc,
                                            ((uentry[j].unitbl == NULL) ? 
                                             (unsigned long)uentry[j].uniltbl :
                                             (unsigned long)uentry[j].unitbl));
#endif
                                } else;
#endif
                                fprintf(skfstderr,"%s%s%s\n",
                                        skf_cname,pad,uentry[j].desc);
                            } else;
                        } else {
#if defined(DYNAMIC_LOADING) && !defined(SWIG_EXT)
                            if (load_external_table(&(uentry[i])) == 0) {
                                if (uentry[j].desc != NULL)
                                    fprintf(skfstderr,"%s%s%s*\n",
                                            skf_cname,pad,uentry[j].desc);
                            };
#endif
                        };
                    };
                    fprintf(skfstderr,"\n");
                };
                fprintf(skfstderr,"# Unicode(TM)\n");
                fprintf(skfstderr," -\t\tUTF-16/UCS2\n -\t\tUTF-8\n -\t\tUTF-7\n");
                fprintf(skfstderr," -\t\tCESU-8\n");
                trademark_warn();
            }
            /* --------------------------------------------------------------- */
            void test_support_codeset()
            {
                int i;
                char *skf_cname,*pad;

                conv_alt_cap = 0;	/* disable other misc message */
                skf_err_msg = skf_gettext
                    ("Supported codeset: cname description \n");
                fprintf(skfstderr,skf_err_msg);

                (void)fflush(skfstderr); (void)fflush(stdout);
                for (i=0; !is_code_invalid(i_codeset[i].encode); i++) { 
                    skf_cname = i_codeset[i].cname;
                    if (skf_cname == NULL) skf_cname = " -   ";
                    if (strlen(skf_cname) < 8) pad = "\t\t";
                    else pad = "\t";
                    if (!is_codehide(i_codeset[i].oconv_type)) {
                        fprintf(skfstderr,"%s%s%s\n",skf_cname,pad,
                                i_codeset[i].desc);
                    };
                };
                trademark_warn();
            }
            /*@+branchstate@*//*@-matchanyintegral@*/
            /* --------------------------------------------------------------- */
#ifdef SKFDEBUG
            /*@-mustfreefresh@*/
            void debug_analyze()
            {
                const char *trans_txt;

                if (is_v_debug) {
                    trademark_warn();
                    trans_txt = skf_gettext("output codeset: ");
                    (void)fputs(trans_txt,skfstderr);
                    skf_outcode_display();
                    fprintf(skfstderr,"conv_cap:%08lx ",conv_cap);
                    if (hk_enbl) fprintf(skfstderr,"X-0201 kana ");
                    if (si_enbl) fprintf(skfstderr,"Si/SO ");
                    fprintf(skfstderr,"\n .. out-opt: ");
                    if (is_lineend_thru) fprintf(skfstderr,"LE_THRU ");
                    if (is_lineend_crlf) fprintf(skfstderr,"LE_CRLF ");
                    if (is_lineend_cr) fprintf(skfstderr,"LE_CR ");
                    if (is_lineend_lf) fprintf(skfstderr,"LE_LF ");
                    if (use_latin2tex) fprintf(skfstderr,"tex_latin ");
                    if (use_latin2htmlu) fprintf(skfstderr,"uri_latin ");
                    if (use_latin2htmlh) fprintf(skfstderr,"uri_latin(hex) ");
                    if (use_latin2htmld) fprintf(skfstderr,"uri_latin(dec) ");
                    if (use_htmlsanitize) fprintf(skfstderr,"sanitize ");
                    if (chart_dsbl) fprintf(skfstderr,"chart_dsbl ");
                    if (stripinvis) fprintf(skfstderr,"stripinvis ");
                    if (use_compat) fprintf(skfstderr,"compat ");
                    if (use_ms_compat) fprintf(skfstderr,"ms_compat ");
                    if (o_add_bom) fprintf(skfstderr,"add_bom ");
                    if (limit_ucs2) fprintf(skfstderr,"limit_ucs2 ");
                    if (sup_jis90) fprintf(skfstderr,"dsbl_jis90 ");
                    if (is_ucs_ufam(conv_cap)) {
                        if (out_endian(conv_cap)) {
                            fprintf(skfstderr,"BE ");
                        } else fprintf(skfstderr,"LE ");
                    } else;
#ifdef UNI_DECOMPOSE
                    if (enbl_decomp) {
                        if (decomp_comp) fprintf(skfstderr,"NFC ");
                        else fprintf(skfstderr,"NFD ");
                    };
#endif
#ifdef FOLD_SUPPORT
                    if (fold_fclap > 0) {
                        fprintf(skfstderr,"FOLD(%d",fold_omgn);
                        if (fold_flat) fprintf(skfstderr,",flat");
                        if (is_noadelim) fprintf(skfstderr,",noadelim");
                        fprintf(skfstderr,")");
                    };
#endif
                    fprintf(skfstderr,"(uc: u+%04x) ",ucode_undef);
                    if (o_encode) {
                        fprintf(skfstderr,"\n");
                        if (is_o_encode_hex(o_encode)) fprintf(skfstderr,"oe:hex");
                        if (is_o_encode_mimeb(o_encode)) fprintf(skfstderr,"oe:MIME");
                        if (is_o_encode_mimeq(o_encode)) fprintf(skfstderr,"oe:MIMEQ");
                        if (is_o_encode_uri(o_encode)) fprintf(skfstderr,"oe:uri");
                        if (is_o_encode_oct(o_encode)) fprintf(skfstderr,"oe:oct");
                        if (is_o_encode_perc(o_encode)) fprintf(skfstderr,"oe:perc");
                        if (is_o_encode_q(o_encode)) fprintf(skfstderr,"oe:q");
                        if (is_o_encode_b64(o_encode)) fprintf(skfstderr,"oe:base64");
                        if (is_ucs_puny(conv_cap)) fprintf(skfstderr,"oe:punycode");
                        fprintf(skfstderr," -llimit: %d",mime_fold_llimit);
                        fprintf(skfstderr,"(");
                        if (no_early_mime_out(nkf_compat)) fprintf(skfstderr,"EM,");
                        if (mime_limit_aware(nkf_compat)) fprintf(skfstderr,"LA,");
                        fprintf(skfstderr,")");
                    };
                    fprintf(skfstderr,"\n");
                    trans_txt = skf_gettext("input code set: ");
                    (void)fputs(trans_txt,skfstderr);
                    skf_incode_display();
                    if (preconv_opt || encode_cap) {
                        fprintf(skfstderr," -");
                        if (input_x201_kana) fprintf(skfstderr," KC");
                        if (fuzzy_detect) fprintf(skfstderr," FZ");
                        if (no_utf7) fprintf(skfstderr," NoUTF7");
                        if (is_mimeq_encoded) fprintf(skfstderr," MIMEQ");
                        if (is_mimeb_encoded) fprintf(skfstderr," MIMEB");
                        if (is_mimeb_strict) fprintf(skfstderr," MIMEBS");
                        if (is_rot_encoded) fprintf(skfstderr," ROT");
                        if (is_hex_encoded) fprintf(skfstderr," HEX");
                        if (is_hex_cap && is_hex_encoded) fprintf(skfstderr,"-PER");
                        if (is_base64_encoded) fprintf(skfstderr,"-B64");
                        if (is_hex_uri) fprintf(skfstderr,"-URI");
                        if (is_puny_encoded) fprintf(skfstderr," PUNY");
                    };
                    fprintf(skfstderr,"\n .. incode opt: ");
                    if (hk_enbl) {
                        if (kana_call) fprintf(skfstderr,"kana-call ");
                        if (si_enbl) fprintf(skfstderr,"SI-enbl ");
                        if (eight_bit) fprintf(skfstderr,"8bit ");
                    };
                    if (disp_warn) fprintf(skfstderr,"Warn ");
                    if (use_x0212) fprintf(skfstderr,"X0212_enabled ");
                    if (is_ms_213c(conv_cap)) fprintf(skfstderr,"X0208_THIRD ");
                    if (use_apple_gaiji) fprintf(skfstderr,"mac compatible ");
                    if (is_ucs_utf16(conv_cap) && use_compat) 
                        fprintf(skfstderr,"compatible_plane ");
                    if (is_ucs_utf16(conv_cap) && use_ms_compat) 
                        fprintf(skfstderr,"Wind*ws Unicode(TM) compatible ");
                    if (is_ucs_utf16(i_codeset[in_codeset].encode) && in_endian) 
                        fprintf(skfstderr,"UCS-2 little endian input ");
                    if (is_ucs_utf16(conv_cap) && out_endian(conv_cap)) 
                        fprintf(skfstderr,"UCS-2 little endian output ");
                    if (is_ucs_utf8(conv_cap)) 
                        fprintf(skfstderr,"UTF-8 little endian output ");
                    if (is_nkf_compat) fprintf(skfstderr,"nkf_CMPT ");
                    if (kuni_opt) fprintf(skfstderr,"LW_DET");
                    if (is_nkf_jbroken) fprintf(skfstderr,"JBRKN ");
                    if (is_nkf_jfbroken) fprintf(skfstderr,"DBRKN ");
                    if (is_nkf_jfbroken) fprintf(skfstderr,"LBRKN ");

                    fprintf(skfstderr,"\n");
                    if (skf_input_lang != 0) {
                        fprintf(skfstderr,"lang: %c%c ",
                                (int)((skf_input_lang >> 8) & 0x7fU),
                                (int)(skf_input_lang & 0x7fU));
                    } else fprintf(skfstderr,"lang: neutral ");
#ifdef FOLD_SUPPORT
                    if (fold_fclap > 0) {
                        fprintf(skfstderr,
                                "fold enabled (%s)- soft_limit:%4d hard_limit:%4d margin:%4d",
                                (notrunc_le) ? "NT":"HW", fold_clap,fold_fclap,fold_omgn);
                    };
#endif
                    fprintf(skfstderr,"\n");
                }; 
            }
            /*@+mustfreefresh@*/
#endif

            /* --------------------------------------------------------------- */
            void ValidValueDisplay(plane,dispstr)
                int plane;
            char *dispstr;
            {
                if (dispstr == NULL) dispstr = nullconststr;
                if (is_euc(i_codeset[in_codeset].encode) && (plane == 1)) {
                    skf_err_msg = skf_gettext("skf: g1 is overwritten in EUC\n");
                    fprintf(skfstderr,skf_err_msg);
                } else {
                    skf_err_msg = 
                        skf_gettext("skf: possible code set for plane G%01d: %s\n");
                    fprintf(skfstderr,skf_err_msg,plane,dispstr);
                };
            }

            /* --------------------------------------------------------------- */
            void ValidLangDisplay(dispstr)
                char *dispstr;
            {
                if (dispstr == NULL) dispstr = nullconststr;
                skf_err_msg = 
                    skf_gettext("skf: possible language set for skf: %s\n");
                fprintf(skfstderr,skf_err_msg,dispstr);
            }
            /* --------------------------------------------------------------- */
