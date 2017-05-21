/* *******************************************************************
** Copyright (c) 1993-2014 Seiji Kaneko. All rights reserved.
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
** Disclaimer: This software is provided and distributed AS IS, 
**	without any implicit or explicit warranties, and not
**	guaranteed to be error-free. In no event shall the author be
**	liable for any direct, indirect or incidental damages,
**	including, but not limited to, loss of data, use or profits
**	responsibility for any direct or indirect damages or results
**	arising by using whole or a part of this software.
**********************************************************************
	skf_fileio.c:	various file io routines
		v1.x	for skf v1.x
	$Id: skf_fileio.c,v 1.52 2017/01/05 15:05:48 seiji Exp seiji $
*/

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <errno.h>

#ifndef HAVE_MKSTEMP
#include <time.h>
#endif

#if defined(__MINGW32__) || defined(SKF_MINGW)
#include <windows.h>
#define		skf_sleep	Sleep
#else
#include <sys/stat.h>
#define		skf_sleep	sleep
#endif

#include "skf.h"
#include "skf_fileio.h"
#include "oconv.h"

#ifdef NEED_BINMODE
#if defined(WIN32) || defined(SKF_MINGW)
#include <io.h>
#endif
const char *skf_inmode = "rb";
const char *skf_outmode = "wb";
#else
const char *skf_inmode = "r";
const char *skf_outmode = "w";
#endif

int    hold_size = 0;

static	short   hold_count = 0;
static	short   hold_pntr = 0;

static  int	hold_buf[DEFAULT_HOLD_SIZE+128+2+2];
/* 130 is for queueing arib-macro(128 + MACRO-0x40.
   1 is for 0x00. 1 is for margin. */

/* --------------------------------------------------------------- */
/* hold stack control: This is treated as Queue			   */
/* Memo: Since program pushes at most 2-byte without test, at	   */
/*	 least 2 space to push is reserved to stack full test.	   */
/* --------------------------------------------------------------- */

/* queue control						   */
void enque(c)
int c;
{
	if (hold_count == DEFAULT_HOLD_SIZE) hold_count = 0;
	hold_buf[hold_count++] = c; hold_size++;
}

int deque()
{
	int s ;

	if (!Qempty) {
		s = hold_buf[hold_pntr++]; hold_size--;
		if (hold_pntr == DEFAULT_HOLD_SIZE) hold_pntr = 0;
	} else s = sEOF;
	return(s);
}

void Qflush()
{
	hold_pntr = 0; hold_count = 0; hold_size = 0;
}

#if defined(SWIG_EXT)
/* --------------------------------------------------------------- */
/* buffers							   */
/* --------------------------------------------------------------- */
unsigned char *stdibuf;

/* --------------------------------------------------------------- */
/* SWIG								   */
/* --------------------------------------------------------------- */
long	buf_p = 0;
long	skf_fpntr;
long	obuf_p = 0;

void *skf_fopen(len,str)
char	*len;
const char	*str;
{
    return ((void *)0);
}

int	skf_fillbuf(p)
int	*p;		/* file descripter			  */
{
	skf_fpntr = 0;
	return(sEOF);
}

#else	
/* --------------------------------------------------------------- */
/* not SWIG							   */
/* --------------------------------------------------------------- */
/* buffers							   */
/* --------------------------------------------------------------- */
unsigned char stdobuf[O_BUFSIZ];
unsigned char *stdibuf;

#if defined(FAST_GETC)

skfFILE skf_infile;

long	buf_p = 0;
long	obuf_p = 0;
long	skf_fpntr;

/*@null@*/ /*@-paramuse@*/ /*ARGSUSED*/ /*@-immediatetrans@*/ 
/*@-incondefs@*/
skfFILE *skf_fopen(name,mode)
char	*name;
const char	*mode;
{
	/* actually, skf_fopen is called only with mode "r", we	  */
	/* don't have to make certain.				  */
#ifdef NEED_BINMODE
	skf_infile = (skfFILE) open(name, O_RDONLY | O_BINARY);
#else
	skf_infile = (skfFILE) open(name, O_RDONLY);
#endif
	buf_p = -1; skf_fpntr = 0; 
	Qflush();
	if (skf_infile >=0) return ((skfFILE *) &skf_infile);
		else	    return ((skfFILE *) NULL);
}

/* input side buffer fill routine				  */
int	skf_fillbuf(p)
int	*p;		/* file descripter			  */
{
	/* This routine only loops when no character is avaliable.*/
	/* Maybe I should pause for signals, but this one is for  */
	/* portability by sacrificing performance.		  */
	buf_p = -1;

	while ((buf_p = (int) read(*p,stdibuf,(size_t)I_BUFSIZ)) < 0) {
	    skf_readerr(errno);
	    if (errno != EAGAIN) {
	    	buf_p = 0;
		return(sABRT);
	    } else {
		(void)skf_sleep(1); /* do not exhaust CPU time	  */
	    };
	};
	skf_fpntr = 0;
	return ((buf_p == 0) ? sEOF : (stdibuf[skf_fpntr++]));
}

/* output file descriptor initialization. */
/*@-onlyunqglobaltrans@*/
void skf_ioinit(fout)
skfoFILE *fout;	/* stdout */
{
    if ((stdibuf = calloc((size_t)I_BUFSIZ,sizeof(unsigned char)))
	    == NULL) {
	skferr(SKF_MALLOCERR,(long)1,(long)0);
    };
#ifndef USE_FWRITE
    skf_setvbuf((FILE *)fout, (char *)stdobuf, (size_t)O_BUFSIZ);
#endif

/* --- preconversion output prepare ------------------------------ */
    if (o_add_bom) show_endian_out();
    if (add_annon) print_announce(out_codeset);

    show_lang_tag();
    return;
}
/*@+onlyunqglobaltrans@*/

/* output flush buffer			*/
void SKFfflush(f)
skfoFILE	*f;
{
    oconv(sFLSH);
#ifdef USE_FWRITE
    if ((fwrite(stdobuf,sizeof(unsigned char),obuf_p,stdout)) < obuf_p) {
	skferr(SKF_PUTFAILERR,(long)1,(long)0);
    };
#else
    (void)fflush((FILE *)f);
#endif
    obuf_p = 0;
}

/* output file_output			*/
int skf_oflush(ch)
int ch;
{
    SKFfflush((skfoFILE *)stdout);
    obuf_p = 1;
    stdobuf[0] = (unsigned char) ch;
    return(0);
}

#else	/* !FAST_GETC */
/*@-dependenttrans@*/ /*@-nullret@*/
FILE *skf_fopen(name,mode)
char *name;
const char *mode;
{
	FILE *ifi;

	if ((stdibuf = calloc((size_t)I_BUFSIZ,sizeof(unsigned char)))
		== NULL) {
	    skferr(SKF_MALLOCERR,(long)1,(long)1);
	};
	ifi = fopen(name,mode);
	return(ifi);
}

int rGETC(f)
skfFILE *f;
{
    int ch;
    if ((ch = getc(f)) == EOF) return(sEOF);
    else return(ch);
}

/* output file descriptor initialization. */
void skf_ioinit(fout)
skfoFILE *fout;	/* stdout */
{
#ifndef	USE_FWRITE
    skf_setvbuf((FILE *)fout, (char *)stdobuf, O_BUFSIZ);
#endif
/* --- preconversion output prepare ------------------------------ */
    if (o_add_bom) show_endian_out();
    if (add_annon) print_announce(out_codeset);

    show_lang_tag();
    return;
}

#endif	/* !SWIG && !FAST_GETC */
#endif

/* --------------------------------------------------------------- */
/* we need function to calculate string length			   */
/* --------------------------------------------------------------- */
size_t  skf_strlen(str,maxlen)
char *str;
int maxlen;
{
    int i;
    size_t len;

    for (i=0,len=0;((i<maxlen) && (*str!='\0')); i++,str++,len++);

    return(len);
}

/* --------------------------------------------------------------- */
/* mkstemp wrapper						   */
/* --------------------------------------------------------------- */
#ifndef HAVE_MKSTEMP
	/* assume situation is case insensitive			   */
static int name_encoder_table[32] = {
  0x41,0x42,0x43,0x44, 0x45,0x46,0x47,0x48,
  0x49,0x4a,0x4b,0x4c, 0x4d,0x4e,0x4f,0x50,
  0x51,0x52,0x53,0x54, 0x55,0x56,0x57,0x58,
  0x59,0x5a,0x31,0x32, 0x33,0x34,0x35,0x36
};
static int	rstat = 0;
#endif


int	skf_mkstemp(nam)
char	*nam;
{

#ifdef HAVE_MKSTEMP
    return(mkstemp(nam));
#else
    int try,nlen;
    long  fn0;
    int	  ofd;	/* output file descriptor */
    int	  curtime;

    /* SKF mkstemp: mkstemp quick'n dirty replacement.
    */
    if (rstat == 0) {
    	rstat = 1;
	curtime = time(NULL);
	srand((int)(curtime + (int)name_encoder_table));
    } else;

    nlen = strlen(nam);

    for (try = 0; try < 16;try++) {
	/* assume rand returns 32bit signed int.	*/
	fn0 = ((rand() & 0x7fff) << 16) + (rand() & 0x7fff);

	nam[nlen-6] = name_encoder_table[(fn0 >> 26) & 0x1f];
	nam[nlen-5] = name_encoder_table[(fn0 >> 21) & 0x1f];
	nam[nlen-4] = name_encoder_table[(fn0 >> 16) & 0x1f];
	nam[nlen-3] = name_encoder_table[(fn0 >> 10) & 0x1f];
	nam[nlen-2] = name_encoder_table[(fn0 >> 5) & 0x1f];
	nam[nlen-1] = name_encoder_table[(fn0 >> 0) & 0x1f];

#ifdef SKFDEBUG
	if (is_vv_debug) {
		fprintf(stderr,"tempname: %s\n",nam);
	} else;
#endif

	if ((ofd = open(nam,O_EXCL | O_CREAT | O_WRONLY)) > 0) {
	    return (ofd);
	} else;
    };
    skf_openerr(nam,9);
    return -1;
#endif
}

/* --------------------------------------------------------------- */
/* Note: this routine is just a wrap for stdio function. Proceed   */
/*	 with extra care.					   */
/* --------------------------------------------------------------- */
void skf_setvbuf(fp,buf,size)
FILE *fp;
char *buf;
size_t size;
{
#if defined(__MINGW32__) || defined(SKF_MINGW) || defined(SWIG_EXT)
    if (is_vv_debug) {
    	fprintf(stderr," NO BUFFERING EXTENSION\n");
    } else;
#else
#ifdef HAVE_SETVBUF
    if (setvbuf(fp, buf, (int)((buf != NULL) ? _IOFBF : _IONBF), size) != 0) {
    	skferr(SKF_OBUFERR,(long)3,(long)0);
    } else;
#else
#ifdef HAVE_SETBUFFER
    if (setbuffer(fp, buf, size) != 0) {
    	skferr(SKF_OBUFERR,(long)3,(long)0);
    } else;
#else	/* neither setvbuf nor setbuffer exist			   */
    if (is_vv_debug) {
    	fprintf(stderr," NO BUFFERING EXTENSION\n");
    } else;
#endif
#endif
#endif
}

/* --------------------------------------------------------------- */
#ifndef SWIG_EXT
void skf_setmode(fp,mode,str)
FILE *fp;
int mode;
char *str;
{
#if defined(WIN32) || defined(SKF_MINGW)
    if (setmode(fileno(fp),O_BINARY) < 0) {
    	skf_openerr(str,1);
	skf_exit(EXIT_FAILURE);
    } else;
#else
    (void)fflush(stderr);
#endif
}
#endif
/* --------------------------------------------------------------- */
