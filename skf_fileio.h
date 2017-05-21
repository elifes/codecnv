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
	skf_fileio.h:	file io routine header
	$Id: skf_fileio.h,v 1.61 2017/01/05 15:05:48 seiji Exp seiji $
*/

/* -------------------------------------------------------------- */
/* queue definitions						  */

/* buffers */
extern int    hold_size;

/* hold stack control: This is treated as Queue			  */
/* Memo: Since program pushes at most 2-byte without check, 	  */
/*	 2 space is reserved when checking queue is full.	  */
#define		Qempty  (hold_size <= 0)
#define		Qle2	(hold_size <= 2)
#define		Qfull   ((hold_size >= DEFAULT_HOLD_SIZE - 2) \
			|| ((unbuf_f) && (hold_size > 0)))

extern	int	deque();
extern	void	enque P_((int));
extern  void	Qflush();
extern	void	SKFrputc P_((int));

extern	int	skf_mkstemp P_((char *));

extern  const char	*skf_inmode;
extern  const char	*skf_outmode;

/* -------------------------------------------------------------- */
/* encoding controls						  */
/* -------------------------------------------------------------- */

extern int decode_hook();

#define	sEOF	(-1)
#define sOCD	(-2)
#define sKAN	(-3)
#define sUNI	(-4)
#define sFLSH	(-5)
#define mFLSH	(-6)	/* flash mime pending character only	  */
#define sDISC	(-7)	/* discard returned character		  */
#define sABRT	(-16)
#define sRETY	(-17)	/* used as return val in in_converter	  */
#define sCONT	(-18)
#define sSUSP	(-19)

#if defined(SWIG_EXT)
/* -------------------------------------------------------------- */
/* SWIG mode:		grab an input buffer and give characters  */
/*			to in_converters.			  */
/* -------------------------------------------------------------- */
/* buffer size definition. reserve some gap for control. */
#define		SKF_STRBUFLEN	8064	/* 8192 - 128	*/
#define		BUFINCSIZE	2048

extern  size_t	skf_strlen P_((char *,int));

extern unsigned char     *stdibuf;	/* input buffer		  */
extern long	buf_p;	/* input side tail pointer		  */
extern		long	obuf_p;	/* output buffer pointer	  */

typedef int	skfFILE;  /* file type				  */
typedef int	skfoFILE;  /* file type				  */
extern  unsigned char     *skfobuf;	/* output buffer	  */
extern	/*@-onlyunqglobaltrans@*/ void	skf_ioinit P_ ((skfoFILE *,int));
extern	long	skf_fpntr;
extern  int	skf_olimit;
#define  SKFfflush(x)	

extern  /*@-retalias@*/ void	*skf_fopen P_((char *, const char *));
#define rGETC(p)	\
	((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : sEOF)

#define	rvGETC(p)	\
	((encode_cap) ? decode_hook(p,1) : \
	(((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : sEOF)))

#define	GETC(p)	\
	((Qempty) ? (((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : \
				 sEOF)) : deque())
 
#define	vGETC(p)	\
	((Qempty) ? \
	 ((encode_cap) ? decode_hook(p,0) : \
	  ((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : \
				 sEOF)) : deque())
#define	vcGETC(p,x)	\
	((Qempty) ? \
	 ((encode_cap) ? decode_hook(p,0) : \
	  ((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : \
				 sEOF)) : deque())

#define unGETC(c,f) ((skf_fpntr > 0) ? (skf_fpntr--) : -1)

/* -------------------------------------------------------------- */
/* putchar substitution 					  */
/* -------------------------------------------------------------- */
#ifndef		SKFputc
#define GROWSIZE	32
extern	int	lwl_putchar P_((int));
#define	SKFputc(x) ((o_encode) ? o_c_encode(x) : (void)lwl_putchar(x))

#define	SKFrputc(x) (void) lwl_putchar(x)
#endif

#else	/* !SWIG */
/* -------------------------------------------------------------- */
/* Normal binary:	i.e. NOT SWIG				  */
/* -------------------------------------------------------------- */
extern unsigned char     *stdibuf; /* input buffer	  */
extern void skf_setmode P_ ((FILE *,int,char *));

extern long	buf_p, skf_fpntr;
/* -------------------------------------------------------------- */
/* fast getc mode:	internally buffered for speed and 	  */
/*			portability.				  */
/* -------------------------------------------------------------- */
#ifdef	FAST_GETC	/* fast getc mode			  */

typedef int	skfFILE;  /* file type				  */

extern unsigned char     stdobuf[O_BUFSIZ]; /* output buffer	  */

extern int	skf_fillbuf();
extern /*@-retalias@*/ skfFILE	*skf_fopen P_((char *, const char *));
extern  size_t	skf_strlen P_((char *,int));

#define	rGETC(p)	\
	((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : \
				 skf_fillbuf(p))	

#define	rvGETC(p)	\
	((encode_cap) ? decode_hook(p,1) : \
	(((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : \
				 skf_fillbuf(p))))

#define	GETC(p)	\
	((Qempty) ? (((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : \
				 skf_fillbuf(p))) : deque())
 
#define	vGETC(p)	\
	((Qempty) ? \
	 ((encode_cap) ? decode_hook(p,0) : \
	  ((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : \
				 skf_fillbuf(p))) : deque())
 
#define	vcGETC(p,x)	\
	((Qempty) ? \
	 ((encode_cap) ? decode_hook(p,0) : \
	  ((skf_fpntr < buf_p) ? (int)(stdibuf[skf_fpntr++]) : \
				 skf_fillbuf(p))) : deque())
 
#define	unGETC(c,f) ((*f < 0) ? -1 : \
			((c != sEOF)?(stdibuf[--skf_fpntr] = c) : -1))

#define skf_fclose(p)	close(*p)

/* -------------------------------------------------------------- */
/* putchar substitution 					  */
/* -------------------------------------------------------------- */
#ifndef		SKFputc
#ifdef USE_FWRITE
extern		long	obuf_p;	/* output buffer pointer	  */
extern		int	skf_oflush P_((int));
typedef int	skfoFILE; /* output side file type		  */
#define		OQfull	(obuf_p == O_BUFSIZ)
#define		SKFputc(x) ((o_encode) ? o_c_encode(x) :
  (OQfull ? skf_oflush(x) : (stdobuf[obuf_p++] = (unsigned char)x)))
#define		SKFrputc(x) \
  (OQfull ? skf_oflush(x) : (stdobuf[obuf_p++] = (unsigned char)x))
#define skf_o_fclose(p)	close(*p)
#else	/* !USE_FWRITE */
typedef FILE	skfoFILE; /* output side file type		  */
#define	SKFputc(x) ((o_encode) ? o_c_encode(x) : (void)putc((int)x,(FILE *)fout))
#define	SKFrputc(x) (void)(putc((int)x,(FILE *)fout))
#define skf_o_fclose(p)	fclose(p)
#endif
#endif

extern 		skfoFILE *fout;
extern		void	SKFfflush P_((skfoFILE *));
extern	/*@-onlyunqglobaltrans@*/ void	skf_ioinit P_((skfoFILE *));

#else			/* not fast_getc mode			  */
/* -------------------------------------------------------------- */
/* non fast getc mode:	for system without 'O_NONBLOCK' feature	  */
/* -------------------------------------------------------------- */

typedef FILE	skfFILE;

extern int	rGETC P_((skfFILE *));

#define		rvGETC(p)	\
	((encode_cap) ? decode_hook(p,1) : rGETC(p))
#define		GETC(p)		((Qempty) ? rGETC(p) : deque())
#define		vGETC(p)	((encode_cap) ? decode_hook(p,0) : \
				    ((Qempty) ? rGETC(p) : deque()))
#define		vcGETC(p,x)	((encode_cap) ? decode_hook(p,0) : \
				    ((Qempty) ? rGETC(p) : deque()))
#define		unGETC(c,p)	ungetc(c,p)
extern /*@-retalias@*/ FILE	*skf_fopen P_((char *, const char *));
#define		skf_fclose(fp)	fclose(fp)

/* -------------------------------------------------------------- */
/* putchar substitution 					  */
/* -------------------------------------------------------------- */
#ifndef		SKFputc
extern skfoFILE *fout;
#define	SKFputc(x) ((o_encode) ? o_c_encode(x) : (void)putc((int)x,fout))
#define	SKFrputc(x) (void)(putc((int)x,fout))
#define		SKFfflush	(void)fflush
#endif

#endif			/* end not fast_getc mode		  */
#endif			/* not SWIG mode			  */
/* -------------------------------------------------------------- */
/* buffer setting function differs among systems(unices)	  */
/* -------------------------------------------------------------- */

#ifdef USE_FWRITE
#define SKF_MASK_VAL	S_IWUSR | S_IWUSR | S_IREAD | S_IWRITE
#else
#if defined(SKF_MINGW) || defined(__MINGW32__)
#define SKF_MASK_VAL	S_IWUSR | S_IWUSR | S_IREAD | S_IWRITE
#else
#define SKF_MASK_VAL	S_IWGRP | S_IWOTH | S_IRGRP | S_IROTH
#endif
#endif

extern void skf_setvbuf P_ ((FILE *,char *,size_t));

/* -------------------------------------------------------------- */
/* exception handling						  */
/* -------------------------------------------------------------- */
#if	defined(SWIG_EXT)
extern void skf_exit	P_((int));
#define skf_terminate(x)	return(-1)
#else
/* if not called in script, just exit on exception	*/
#define	skf_exit	exit
#define skf_terminate	exit
#endif

