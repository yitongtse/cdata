/* This file is copied and modified from mpeg2play */

/* display.c, X11 interface                                                 */

/*
 * All modifications (mpeg2decode -> mpeg2play) are
 * Copyright (C) 1994, Stefan Eckart. All Rights Reserved.
 */

/* Copyright (C) 1994, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

 /* the Xlib interface is closely modeled after
  * mpeg_play 2.0 by the Berkeley Plateau Research Group
  */
/* Rui_B */
#include "def.h"
#ifdef UNIX
/* Rui_E */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>


/****************************************************************************/

#include "util.h"
#include "mpeg2.h"
#include "info.h"

static int horizontal_size, vertical_size;
static int coded_picture_width, coded_picture_height, chrom_width;
static int prog_seq, pict_struct, topfirst, chroma_format, matrix_coefficients;
static int quiet = 0;
static uchar *clp;

int convmat[8][4] =
{
  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
  {104597, 132201, 25675, 53279}, /* unspecified */
  {104597, 132201, 25675, 53279}, /* reserved */
  {104448, 132798, 24759, 53109}, /* FCC */
  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
  {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
};

/****************************************************************************/


/* private prototypes */
static void display_image(XImage *ximage, uchar *dithered_image);
static void ditherframe(uchar *src[]);
static void dithertop(uchar *src[], uchar *dst);
static void ditherbot(uchar *src[], uchar *dst);
static void ditherframe444(uchar *src[]);
static void dithertop444(uchar *src[], uchar *dst);
static void ditherbot444(uchar *src[], uchar *dst);

/* local data */
static uchar *dithered_image, *dithered_image2;

static uchar ytab[16*(256+16)];
static uchar uvtab[256*269+270];

/* X11 related variables */
static Display *display;
static Window window;
static GC gc;
static XImage *ximage, *ximage2;
static uchar pixel[256];

#ifdef SH_MEM

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

static int HandleXError(Display *dpy, XErrorEvent *event);
static void InstallXErrorHandler(void);
static void DeInstallXErrorHandler(void);

static int shmem_flag;
static XShmSegmentInfo shminfo1, shminfo2;
static int gXErrorFlag;
static int CompletionType = -1;

static int HandleXError(Display *dpy, XErrorEvent *event)
{
  gXErrorFlag = 1;

  return 0;
}

static void InstallXErrorHandler()
{
  XSetErrorHandler(HandleXError);
  XFlush(display);
}

static void DeInstallXErrorHandler()
{
  XSetErrorHandler(NULL);
  XFlush(display);
}

#endif


static void error(char *text)
{
  fprintf(stderr,text);
  exit(1);
}


/* connect to server, create and map window,
 * allocate colors and (shared) memory
 */
void init_display(char *name, PicInfo* pic)
{
  int crv, cbu, cgu, cgv;
  int y, u, v, r, g, b;
  int i;
  char dummy;
  int screen;
  Colormap cmap;
  int private;
  XColor xcolor;
  uint fg, bg;
  char *hello = "MPEG-2 Display";
  XSizeHints hint;
  XVisualInfo vinfo;
  XEvent xev;
  unsigned long tmp_pixel;
  XWindowAttributes xwa;


  clp = clipTab;

  /* Copy sequence information into static variables in this module */
  horizontal_size = pic->horizontal_size;
  vertical_size = pic->vertical_size;
  coded_picture_width = pic->nCol;
  coded_picture_height = pic->nRow;
  chrom_width = coded_picture_width >> 1;
  prog_seq = pic->progressive_sequence;
  chroma_format = pic->chroma_format;
  matrix_coefficients = pic->matrix_coefficients;


  display = XOpenDisplay(name);

  if (display == NULL)
    error("Can not open display\n");

  screen = DefaultScreen(display);

  hint.x = 200;
  hint.y = 200;
  hint.width = horizontal_size;
  hint.height = vertical_size;
  hint.flags = PPosition | PSize;

  /* Get some colors */

  bg = WhitePixel (display, screen);
  fg = BlackPixel (display, screen);

  /* Make the window */

  if (!XMatchVisualInfo(display, screen, 8, PseudoColor, &vinfo))
  {
    if (!XMatchVisualInfo(display, screen, 8, GrayScale, &vinfo))
      error("requires 8 bit display\n");
  }

  window = XCreateSimpleWindow (display, DefaultRootWindow (display),
             hint.x, hint.y, hint.width, hint.height, 4, fg, bg);

  XSelectInput(display, window, StructureNotifyMask);

  /* Tell other applications about this window */

  XSetStandardProperties (display, window, hello, hello, None, NULL, 0, &hint);

  /* Map window. */

  XMapWindow(display, window);

  /* Wait for map. */
  do
  {
    XNextEvent(display, &xev);
  }
  while (xev.type != MapNotify || xev.xmap.event != window);

  XSelectInput(display, window, NoEventMask);

  /* matrix coefficients */
  crv = convmat[matrix_coefficients][0];
  cbu = convmat[matrix_coefficients][1];
  cgu = convmat[matrix_coefficients][2];
  cgv = convmat[matrix_coefficients][3];

  /* allocate colors */

  gc = DefaultGC(display, screen);
  cmap = DefaultColormap(display, screen);
  private = 0;

  /* color allocation:
   * i is the (internal) 8 bit color number, it consists of separate
   * bit fields for Y, U and V: i = (yyyyuuvv), we don't use yyyy=0000
   * yyyy=0001 and yyyy=1111, this leaves 48 colors for other applications
   *
   * the allocated colors correspond to the following Y, U and V values:
   * Y:   40, 56, 72, 88, 104, 120, 136, 152, 168, 184, 200, 216, 232
   * U,V: -48, -16, 16, 48
   *
   * U and V values span only about half the color space; this gives
   * usually much better quality, although highly saturated colors can
   * not be displayed properly
   *
   * translation to R,G,B is implicitly done by the color look-up table
   */
  for (i=32; i<240; i++)
  {
    /* color space conversion */
    y = 16*((i>>4)&15) + 8;
    u = 32*((i>>2)&3)  - 48;
    v = 32*(i&3)       - 48;

    y = 76309 * (y - 16); /* (255/219)*65536 */

    r = clp[(y + crv*v + 32768)>>16];
    g = clp[(y - cgu*u -cgv*v + 32768)>>16];
    b = clp[(y + cbu*u + 32786)>>16];

    /* X11 colors are 16 bit */
    xcolor.red   = r << 8;
    xcolor.green = g << 8;
    xcolor.blue  = b << 8;

    if (XAllocColor(display, cmap, &xcolor) != 0)
      pixel[i] = xcolor.pixel;
    else
    {
      /* allocation failed, have to use a private colormap */

      if (private)
        error("Couldn't allocate private colormap");

      private = 1;

      if (!quiet)
        fprintf(stderr, "Using private colormap (%d colors were available).\n",
          i-32);

      /* Free colors. */
      while (--i >= 32)
      {
        tmp_pixel = pixel[i]; /* because XFreeColors expects unsigned long */
        XFreeColors(display, cmap, &tmp_pixel, 1, 0);
      }

      /* i is now 31, this restarts the outer loop */

      /* create private colormap */

      XGetWindowAttributes(display, window, &xwa);
      cmap = XCreateColormap(display, window, xwa.visual, AllocNone);
      XSetWindowColormap(display, window, cmap);
    }
  }

#ifdef SH_MEM
  if (XShmQueryExtension(display))
    shmem_flag = 1;
  else
  {
    shmem_flag = 0;
    if (!quiet)
      fprintf(stderr, "Shared memory not supported\nReverting to normal Xlib\n");
  }

  if (shmem_flag)
    CompletionType = XShmGetEventBase(display) + ShmCompletion;

  InstallXErrorHandler();

  if (shmem_flag)
  {

    ximage = XShmCreateImage(display, None, 8, ZPixmap, NULL,
                             &shminfo1,
                             coded_picture_width, coded_picture_height);

    if (!prog_seq)
      ximage2 = XShmCreateImage(display, None, 8, ZPixmap, NULL,
                                &shminfo2,
                                coded_picture_width, coded_picture_height);

    /* If no go, then revert to normal Xlib calls. */

    if (ximage==NULL || (!prog_seq && ximage2==NULL))
    {
      if (ximage!=NULL)
        XDestroyImage(ximage);
      if (!prog_seq && ximage2!=NULL)
        XDestroyImage(ximage2);
      if (!quiet)
        fprintf(stderr, "Shared memory error, disabling (Ximage error)\n");
      goto shmemerror;
    }

    /* Success here, continue. */

    shminfo1.shmid = shmget(IPC_PRIVATE, 
                            ximage->bytes_per_line * ximage->height,
                            IPC_CREAT | 0777);
    if (!prog_seq)
      shminfo2.shmid = shmget(IPC_PRIVATE, 
                              ximage2->bytes_per_line * ximage2->height,
                              IPC_CREAT | 0777);

    if (shminfo1.shmid<0 || (!prog_seq && shminfo2.shmid<0))
    {
      XDestroyImage(ximage);
      if (!prog_seq)
        XDestroyImage(ximage2);
      if (!quiet)
        fprintf(stderr, "Shared memory error, disabling (seg id error)\n");
      goto shmemerror;
    }

    shminfo1.shmaddr = (char *) shmat(shminfo1.shmid, 0, 0);
    shminfo2.shmaddr = (char *) shmat(shminfo2.shmid, 0, 0);

    if (shminfo1.shmaddr==((char *) -1) ||
        (!prog_seq && shminfo2.shmaddr==((char *) -1)))
    {
      XDestroyImage(ximage);
      if (shminfo1.shmaddr!=((char *) -1))
        shmdt(shminfo1.shmaddr);
      if (!prog_seq)
      {
        XDestroyImage(ximage2);
        if (shminfo2.shmaddr!=((char *) -1))
          shmdt(shminfo2.shmaddr);
      }
      if (!quiet)
      {
        fprintf(stderr, "Shared memory error, disabling (address error)\n");
      }
      goto shmemerror;
    }

    ximage->data = shminfo1.shmaddr;
    dithered_image = (uchar *)ximage->data;
    shminfo1.readOnly = False;
    XShmAttach(display, &shminfo1);
    if (!prog_seq)
    {
      ximage2->data = shminfo2.shmaddr;
      dithered_image2 = (uchar *)ximage2->data;
      shminfo2.readOnly = False;
      XShmAttach(display, &shminfo2);
    }

    XSync(display, False);

    if (gXErrorFlag)
    {
      /* Ultimate failure here. */
      XDestroyImage(ximage);
      shmdt(shminfo1.shmaddr);
      if (!prog_seq)
      {
        XDestroyImage(ximage2);
        shmdt(shminfo2.shmaddr);
      }
      if (!quiet)
        fprintf(stderr, "Shared memory error, disabling.\n");
      gXErrorFlag = 0;
      goto shmemerror;
    }
    else
    {
      shmctl(shminfo1.shmid, IPC_RMID, 0);
      if (!prog_seq)
        shmctl(shminfo2.shmid, IPC_RMID, 0);
    }

    if (!quiet)
    {
      fprintf(stderr, "Sharing memory.\n");
    }
  }
  else
  {
shmemerror:
    shmem_flag = 0;
#endif

    ximage = XCreateImage(display,None,8,ZPixmap,0,&dummy,
                          coded_picture_width,coded_picture_height,8,0);

    if (!(dithered_image = (uchar *)malloc(coded_picture_width*
                                                   coded_picture_height)))
      error("malloc failed");

    if (!prog_seq)
    {
      ximage2 = XCreateImage(display,None,8,ZPixmap,0,&dummy,
                             coded_picture_width,coded_picture_height,8,0);

      if (!(dithered_image2 = (uchar *)malloc(coded_picture_width*
                                                      coded_picture_height)))
        error("malloc failed");
    }

#ifdef SH_MEM
  }

  DeInstallXErrorHandler();
#endif
}

void exit_display()
{
#ifdef SH_MEM
  if (shmem_flag)
  {
    XShmDetach(display, &shminfo1);
    XDestroyImage(ximage);
    shmdt(shminfo1.shmaddr);
    if (!prog_seq)
    {
      XShmDetach(display, &shminfo2);
      XDestroyImage(ximage2);
      shmdt(shminfo2.shmaddr);
    }
  }
#endif
}

static void display_image(XImage *ximage, uchar *dithered_image)
{
  /* display dithered image */
#ifdef SH_MEM
  if (shmem_flag)
  {
    XShmPutImage(display, window, gc, ximage, 
       	         0, 0, 0, 0, ximage->width, ximage->height, True);
    XFlush(display);
      
    while (1)
    {
      XEvent xev;
	
      XNextEvent(display, &xev);
      if (xev.type == CompletionType)
        break;
    }
  }
  else 
#endif
  {
    ximage->data = (char *) dithered_image; 
    XPutImage(display, window, gc, ximage, 0, 0, 0, 0, ximage->width, ximage->height);
  }
}

void display_second_field()
{
  display_image(ximage2,dithered_image2);
}

/* 4x4 ordered dither
 *
 * threshold pattern:
 *   0  8  2 10
 *  12  4 14  6
 *   3 11  1  9
 *  15  7 13  5
 */

void init_dither()
{
  int i, j, v;
  uchar ctab[256+32];

  for (i=0; i<256+16; i++)
  {
    v = (i-8)>>4;
    if (v<2)
      v = 2;
    else if (v>14)
      v = 14;
    for (j=0; j<16; j++)
      ytab[16*i+j] = pixel[(v<<4)+j];
  }

  for (i=0; i<256+32; i++)
  {
    v = (i+48-128)>>5;
    if (v<0)
      v = 0;
    else if (v>3)
      v = 3;
    ctab[i] = v;
  }

  for (i=0; i<255+15; i++)
    for (j=0; j<255+15; j++)
      uvtab[256*i+j]=(ctab[i+16]<<6)|(ctab[j+16]<<4)|(ctab[i]<<2)|ctab[j];
}

void dither(uchar *src[], PicInfo *pic)
{
  pict_struct = pic->picture_structure;
  topfirst = pic->top_field_first;

  if (prog_seq)
  {
    if (chroma_format!=CHROMA_FORMAT_444)
      ditherframe(src);
    else
      ditherframe444(src);
  }
  else
  {
    if ((pict_struct==FRAME && topfirst) || pict_struct==BOTTOM_FIELD)
    {
      /* top field first */
      if (chroma_format!=CHROMA_FORMAT_444)
      {
        dithertop(src,dithered_image);
        ditherbot(src,dithered_image2);
      }
      else
      {
        dithertop444(src,dithered_image);
        ditherbot444(src,dithered_image2);
      }
    }
    else
    {
      /* bottom field first */
      if (chroma_format!=CHROMA_FORMAT_444)
      {
        ditherbot(src,dithered_image);
        dithertop(src,dithered_image2);
      }
      else
      {
        ditherbot444(src,dithered_image);
        dithertop444(src,dithered_image2);
      }
    }
  }

  display_image(ximage,dithered_image);
}

/* only for 4:2:0 and 4:2:2! */

static void ditherframe(uchar *src[])
{
  int i,j;
  uint uv;
  uchar *py,*pu,*pv,*dst;

  py = src[0];
  pu = src[1];
  pv = src[2];
  dst = dithered_image;

  for (j=0; j<coded_picture_height; j+=4)
  {
    /* line j + 0 */
    for (i=0; i<coded_picture_width; i+=8)
    {
      uv = uvtab[(*pu++<<8)|*pv++];
      *dst++ = ytab[((*py++)<<4)|(uv&15)];
      *dst++ = ytab[((*py++ +8)<<4)|(uv>>4)];
      uv = uvtab[((*pu++<<8)|*pv++)+1028];
      *dst++ = ytab[((*py++ +2)<<4)|(uv&15)];
      *dst++ = ytab[((*py++ +10)<<4)|(uv>>4)];
      uv = uvtab[(*pu++<<8)|*pv++];
      *dst++ = ytab[((*py++)<<4)|(uv&15)];
      *dst++ = ytab[((*py++ +8)<<4)|(uv>>4)];
      uv = uvtab[((*pu++<<8)|*pv++)+1028];
      *dst++ = ytab[((*py++ +2)<<4)|(uv&15)];
      *dst++ = ytab[((*py++ +10)<<4)|(uv>>4)];
    }

    if (chroma_format==CHROMA_FORMAT_420)
    {
      pu -= chrom_width;
      pv -= chrom_width;
    }

    /* line j + 1 */
    for (i=0; i<coded_picture_width; i+=8)
    {
      uv = uvtab[((*pu++<<8)|*pv++)+2056];
      *dst++ = ytab[((*py++ +12)<<4)|(uv>>4)];
      *dst++ = ytab[((*py++ +4)<<4)|(uv&15)];
      uv = uvtab[((*pu++<<8)|*pv++)+3084];
      *dst++ = ytab[((*py++ +14)<<4)|(uv>>4)];
      *dst++ = ytab[((*py++ +6)<<4)|(uv&15)];
      uv = uvtab[((*pu++<<8)|*pv++)+2056];
      *dst++ = ytab[((*py++ +12)<<4)|(uv>>4)];
      *dst++ = ytab[((*py++ +4)<<4)|(uv&15)];
      uv = uvtab[((*pu++<<8)|*pv++)+3084];
      *dst++ = ytab[((*py++ +14)<<4)|(uv>>4)];
      *dst++ = ytab[((*py++ +6)<<4)|(uv&15)];
    }

    /* line j + 2 */
    for (i=0; i<coded_picture_width; i+=8)
    {
      uv = uvtab[((*pu++<<8)|*pv++)+1542];
      *dst++ = ytab[((*py++ +3)<<4)|(uv&15)];
      *dst++ = ytab[((*py++ +11)<<4)|(uv>>4)];
      uv = uvtab[((*pu++<<8)|*pv++)+514];
      *dst++ = ytab[((*py++ +1)<<4)|(uv&15)];
      *dst++ = ytab[((*py++ +9)<<4)|(uv>>4)];
      uv = uvtab[((*pu++<<8)|*pv++)+1542];
      *dst++ = ytab[((*py++ +3)<<4)|(uv&15)];
      *dst++ = ytab[((*py++ +11)<<4)|(uv>>4)];
      uv = uvtab[((*pu++<<8)|*pv++)+514];
      *dst++ = ytab[((*py++ +1)<<4)|(uv&15)];
      *dst++ = ytab[((*py++ +9)<<4)|(uv>>4)];
    }

    if (chroma_format==CHROMA_FORMAT_420)
    {
      pu -= chrom_width;
      pv -= chrom_width;
    }

    /* line j + 3 */
    for (i=0; i<coded_picture_width; i+=8)
    {
      uv = uvtab[((*pu++<<8)|*pv++)+3598];
      *dst++ = ytab[((*py++ +15)<<4)|(uv>>4)];
      *dst++ = ytab[((*py++ +7)<<4)|(uv&15)];
      uv = uvtab[((*pu++<<8)|*pv++)+2570];
      *dst++ = ytab[((*py++ +13)<<4)|(uv>>4)];
      *dst++ = ytab[((*py++ +5)<<4)|(uv&15)];
      uv = uvtab[((*pu++<<8)|*pv++)+3598];
      *dst++ = ytab[((*py++ +15)<<4)|(uv>>4)];
      *dst++ = ytab[((*py++ +7)<<4)|(uv&15)];
      uv = uvtab[((*pu++<<8)|*pv++)+2570];
      *dst++ = ytab[((*py++ +13)<<4)|(uv>>4)];
      *dst++ = ytab[((*py++ +5)<<4)|(uv&15)];
    }
  }
}

static void dithertop(uchar *src[], uchar *dst)
{
  int i,j;
  uint y,uv1,uv2;
  uchar *py,*py2,*pu,*pv,*dst2;

  py = src[0];
  py2 = src[0] + (coded_picture_width<<1);
  pu = src[1];
  pv = src[2];
  dst2 = dst + coded_picture_width;

  for (j=0; j<coded_picture_height; j+=4)
  {
    /* line j + 0, j + 1 */
    for (i=0; i<coded_picture_width; i+=4)
    {
      y = *py++;
      uv2 = (*pu++<<8)|*pv++;
      uv1 = uvtab[uv2];
      uv2 = uvtab[uv2+2056];
      *dst++  = ytab[((y)<<4)|(uv1&15)];
      *dst2++ = ytab[((((y + *py2++)>>1)+12)<<4)|(uv2>>4)];

      y = *py++;
      *dst++  = ytab[((y+8)<<4)|(uv1>>4)];
      *dst2++ = ytab[((((y + *py2++)>>1)+4)<<4)|(uv2&15)];

      y = *py++;
      uv2 = (*pu++<<8)|*pv++;
      uv1 = uvtab[uv2+1028];
      uv2 = uvtab[uv2+3072];
      *dst++  = ytab[((y+2)<<4)|(uv1&15)];
      *dst2++ = ytab[((((y + *py2++)>>1)+14)<<4)|(uv2>>4)];

      y = *py++;
      *dst++  = ytab[((y+10)<<4)|(uv1>>4)];
      *dst2++ = ytab[((((y + *py2++)>>1)+6)<<4)|(uv2&15)];
    }

    py += coded_picture_width;

    if (j!=(coded_picture_height-4))
      py2 += coded_picture_width;
    else
      py2 -= coded_picture_width;

    dst += coded_picture_width;
    dst2 += coded_picture_width;

    if (chroma_format==CHROMA_FORMAT_420)
    {
      pu -= chrom_width;
      pv -= chrom_width;
    }
    else
    {
      pu += chrom_width;
      pv += chrom_width;
    }

    /* line j + 2, j + 3 */
    for (i=0; i<coded_picture_width; i+=4)
    {
      y = *py++;
      uv2 = (*pu++<<8)|*pv++;
      uv1 = uvtab[uv2+1542];
      uv2 = uvtab[uv2+3598];
      *dst++  = ytab[((y+3)<<4)|(uv1&15)];
      *dst2++ = ytab[((((y + *py2++)>>1)+15)<<4)|(uv2>>4)];

      y = *py++;
      *dst++  = ytab[((y+11)<<4)|(uv1>>4)];
      *dst2++ = ytab[((((y + *py2++)>>1)+7)<<4)|(uv2&15)];

      y = *py++;
      uv2 = (*pu++<<8)|*pv++;
      uv1 = uvtab[uv2+514];
      uv2 = uvtab[uv2+2570];
      *dst++  = ytab[((y+1)<<4)|(uv1&15)];
      *dst2++ = ytab[((((y + *py2++)>>1)+13)<<4)|(uv2>>4)];

      y = *py++;
      *dst++  = ytab[((y+9)<<4)|(uv1>>4)];
      *dst2++ = ytab[((((y + *py2++)>>1)+5)<<4)|(uv2&15)];
    }

    py += coded_picture_width;
    py2 += coded_picture_width;
    dst += coded_picture_width;
    dst2 += coded_picture_width;
    pu += chrom_width;
    pv += chrom_width;
  }
}

static void ditherbot(uchar *src[], uchar *dst)
{
  int i,j;
  uint y2,uv1,uv2;
  uchar *py,*py2,*pu,*pv,*dst2;

  py = src[0] + coded_picture_width;
  py2 = py;
  pu = src[1] + chrom_width;
  pv = src[2] + chrom_width;
  dst2 = dst + coded_picture_width;

  for (j=0; j<coded_picture_height; j+=4)
  {
    /* line j + 0, j + 1 */
    for (i=0; i<coded_picture_width; i+=4)
    {
      y2 = *py2++;
      uv2 = (*pu++<<8)|*pv++;
      uv1 = uvtab[uv2];
      uv2 = uvtab[uv2+2056];
      *dst++  = ytab[((((*py++ + y2)>>1))<<4)|(uv1&15)];
      *dst2++ = ytab[((y2+12)<<4)|(uv2>>4)];

      y2 = *py2++;
      *dst++  = ytab[((((*py++ + y2)>>1)+8)<<4)|(uv1>>4)];
      *dst2++ = ytab[((y2+4)<<4)|(uv2&15)];

      y2 = *py2++;
      uv2 = (*pu++<<8)|*pv++;
      uv1 = uvtab[uv2+1028];
      uv2 = uvtab[uv2+3072];
      *dst++  = ytab[((((*py++ + y2)>>1)+2)<<4)|(uv1&15)];
      *dst2++ = ytab[((y2+14)<<4)|(uv2>>4)];

      y2 = *py2++;
      *dst++  = ytab[((((*py++ + y2)>>1)+10)<<4)|(uv1>>4)];
      *dst2++ = ytab[((y2+6)<<4)|(uv2&15)];
    }

    if (j==0)
      py -= coded_picture_width;
    else
      py += coded_picture_width;

    py2 += coded_picture_width;
    dst += coded_picture_width;
    dst2 += coded_picture_width;

    if (chroma_format==CHROMA_FORMAT_420)
    {
      pu -= chrom_width;
      pv -= chrom_width;
    }
    else
    {
      pu += chrom_width;
      pv += chrom_width;
    }

    /* line j + 2, j + 3 */
    for (i=0; i<coded_picture_width; i+=4)
    {
      y2 = *py2++;
      uv2 = (*pu++<<8)|*pv++;
      uv1 = uvtab[uv2+1542];
      uv2 = uvtab[uv2+3598];
      *dst++  = ytab[((((*py++ + y2)>>1)+3)<<4)|(uv1&15)];
      *dst2++ = ytab[((y2+15)<<4)|(uv2>>4)];

      y2 = *py2++;
      *dst++  = ytab[((((*py++ + y2)>>1)+11)<<4)|(uv1>>4)];
      *dst2++ = ytab[((y2+7)<<4)|(uv2&15)];

      y2 = *py2++;
      uv2 = (*pu++<<8)|*pv++;
      uv1 = uvtab[uv2+514];
      uv2 = uvtab[uv2+2570];
      *dst++  = ytab[((((*py++ + y2)>>1)+1)<<4)|(uv1&15)];
      *dst2++ = ytab[((y2+13)<<4)|(uv2>>4)];

      y2 = *py2++;
      *dst++  = ytab[((((*py++ + y2)>>1)+9)<<4)|(uv1>>4)];
      *dst2++ = ytab[((y2+5)<<4)|(uv2&15)];
    }

    py += coded_picture_width;
    py2 += coded_picture_width;
    dst += coded_picture_width;
    dst2 += coded_picture_width;
    pu += chrom_width;
    pv += chrom_width;
  }
}

/* only for 4:4:4 */

static void ditherframe444(uchar *src[])
{
  int i,j;
  uchar *py,*pu,*pv,*dst;

  py = src[0];
  pu = src[1];
  pv = src[2];
  dst = dithered_image;

  for (j=0; j<coded_picture_height; j+=4)
  {
    /* line j + 0 */
    for (i=0; i<coded_picture_width; i+=8)
    {
      *dst++ = ytab[((*py++)<<4)|(uvtab[(*pu++<<8)|*pv++]&15)];
      *dst++ = ytab[((*py++ +8)<<4)|(uvtab[(*pu++<<8)|*pv++]>>4)];
      *dst++ = ytab[((*py++ +2)<<4)|(uvtab[((*pu++<<8)|*pv++)+1028]&15)];
      *dst++ = ytab[((*py++ +10)<<4)|(uvtab[((*pu++<<8)|*pv++)+1028]>>4)];
      *dst++ = ytab[((*py++)<<4)|(uvtab[(*pu++<<8)|*pv++]&15)];
      *dst++ = ytab[((*py++ +8)<<4)|(uvtab[(*pu++<<8)|*pv++]>>4)];
      *dst++ = ytab[((*py++ +2)<<4)|(uvtab[((*pu++<<8)|*pv++)+1028]&15)];
      *dst++ = ytab[((*py++ +10)<<4)|(uvtab[((*pu++<<8)|*pv++)+1028]>>4)];
    }

    /* line j + 1 */
    for (i=0; i<coded_picture_width; i+=8)
    {
      *dst++ = ytab[((*py++ +12)<<4)|(uvtab[((*pu++<<8)|*pv++)+2056]>>4)];
      *dst++ = ytab[((*py++ +4)<<4)|(uvtab[((*pu++<<8)|*pv++)+2056]&15)];
      *dst++ = ytab[((*py++ +14)<<4)|(uvtab[((*pu++<<8)|*pv++)+3084]>>4)];
      *dst++ = ytab[((*py++ +6)<<4)|(uvtab[((*pu++<<8)|*pv++)+3084]&15)];
      *dst++ = ytab[((*py++ +12)<<4)|(uvtab[((*pu++<<8)|*pv++)+2056]>>4)];
      *dst++ = ytab[((*py++ +4)<<4)|(uvtab[((*pu++<<8)|*pv++)+2056]&15)];
      *dst++ = ytab[((*py++ +14)<<4)|(uvtab[((*pu++<<8)|*pv++)+3084]>>4)];
      *dst++ = ytab[((*py++ +6)<<4)|(uvtab[((*pu++<<8)|*pv++)+3084]&15)];
    }

    /* line j + 2 */
    for (i=0; i<coded_picture_width; i+=8)
    {
      *dst++ = ytab[((*py++ +3)<<4)|(uvtab[((*pu++<<8)|*pv++)+1542]&15)];
      *dst++ = ytab[((*py++ +11)<<4)|(uvtab[((*pu++<<8)|*pv++)+1542]>>4)];
      *dst++ = ytab[((*py++ +1)<<4)|(uvtab[((*pu++<<8)|*pv++)+514]&15)];
      *dst++ = ytab[((*py++ +9)<<4)|(uvtab[((*pu++<<8)|*pv++)+514]>>4)];
      *dst++ = ytab[((*py++ +3)<<4)|(uvtab[((*pu++<<8)|*pv++)+1542]&15)];
      *dst++ = ytab[((*py++ +11)<<4)|(uvtab[((*pu++<<8)|*pv++)+1542]>>4)];
      *dst++ = ytab[((*py++ +1)<<4)|(uvtab[((*pu++<<8)|*pv++)+514]&15)];
      *dst++ = ytab[((*py++ +9)<<4)|(uvtab[((*pu++<<8)|*pv++)+514]>>4)];
    }

    /* line j + 3 */
    for (i=0; i<coded_picture_width; i+=8)
    {
      *dst++ = ytab[((*py++ +15)<<4)|(uvtab[((*pu++<<8)|*pv++)+3598]>>4)];
      *dst++ = ytab[((*py++ +7)<<4)|(uvtab[((*pu++<<8)|*pv++)+3598]&15)];
      *dst++ = ytab[((*py++ +13)<<4)|(uvtab[((*pu++<<8)|*pv++)+2570]>>4)];
      *dst++ = ytab[((*py++ +5)<<4)|(uvtab[((*pu++<<8)|*pv++)+2570]&15)];
      *dst++ = ytab[((*py++ +15)<<4)|(uvtab[((*pu++<<8)|*pv++)+3598]>>4)];
      *dst++ = ytab[((*py++ +7)<<4)|(uvtab[((*pu++<<8)|*pv++)+3598]&15)];
      *dst++ = ytab[((*py++ +13)<<4)|(uvtab[((*pu++<<8)|*pv++)+2570]>>4)];
      *dst++ = ytab[((*py++ +5)<<4)|(uvtab[((*pu++<<8)|*pv++)+2570]&15)];
    }
  }
}


static void dithertop444(uchar *src[], uchar *dst)
{
  int i,j;
  uint y,uv;
  uchar *py,*py2,*pu,*pv,*dst2;

  py = src[0];
  py2 = src[0] + (coded_picture_width<<1);
  pu = src[1];
  pv = src[2];
  dst2 = dst + coded_picture_width;

  for (j=0; j<coded_picture_height; j+=4)
  {
    /* line j + 0, j + 1 */
    for (i=0; i<coded_picture_width; i+=4)
    {
      y = *py++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((y)<<4)|(uvtab[uv]&15)];
      *dst2++ = ytab[((((y + *py2++)>>1)+12)<<4)|(uvtab[uv+2056]>>4)];

      y = *py++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((y+8)<<4)|(uvtab[uv]>>4)];
      *dst2++ = ytab[((((y + *py2++)>>1)+4)<<4)|(uvtab[uv+2056]&15)];

      y = *py++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((y+2)<<4)|(uvtab[uv+1028]&15)];
      *dst2++ = ytab[((((y + *py2++)>>1)+14)<<4)|(uvtab[uv+3072]>>4)];

      y = *py++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((y+10)<<4)|(uvtab[uv+1028]>>4)];
      *dst2++ = ytab[((((y + *py2++)>>1)+6)<<4)|(uvtab[uv+3072]&15)];
    }

    py += coded_picture_width;

    if (j!=(coded_picture_height-4))
      py2 += coded_picture_width;
    else
      py2 -= coded_picture_width;

    dst += coded_picture_width;
    dst2 += coded_picture_width;

    pu += chrom_width;
    pv += chrom_width;

    /* line j + 2, j + 3 */
    for (i=0; i<coded_picture_width; i+=4)
    {
      y = *py++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((y+3)<<4)|(uvtab[uv+1542]&15)];
      *dst2++ = ytab[((((y + *py2++)>>1)+15)<<4)|(uvtab[uv+3598]>>4)];

      y = *py++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((y+11)<<4)|(uvtab[uv+1542]>>4)];
      *dst2++ = ytab[((((y + *py2++)>>1)+7)<<4)|(uvtab[uv+3598]&15)];

      y = *py++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((y+1)<<4)|(uvtab[uv+514]&15)];
      *dst2++ = ytab[((((y + *py2++)>>1)+13)<<4)|(uvtab[uv+2570]>>4)];

      y = *py++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((y+9)<<4)|(uvtab[uv+514]>>4)];
      *dst2++ = ytab[((((y + *py2++)>>1)+5)<<4)|(uvtab[uv+2570]&15)];
    }

    py += coded_picture_width;
    py2 += coded_picture_width;
    dst += coded_picture_width;
    dst2 += coded_picture_width;
    pu += chrom_width;
    pv += chrom_width;
  }
}

static void ditherbot444(uchar *src[], uchar *dst)
{
  int i,j;
  uint y2,uv;
  uchar *py,*py2,*pu,*pv,*dst2;

  py = src[0] + coded_picture_width;
  py2 = py;
  pu = src[1] + chrom_width;
  pv = src[2] + chrom_width;
  dst2 = dst + coded_picture_width;

  for (j=0; j<coded_picture_height; j+=4)
  {
    /* line j + 0, j + 1 */
    for (i=0; i<coded_picture_width; i+=4)
    {
      y2 = *py2++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((((*py++ + y2)>>1))<<4)|(uvtab[uv]&15)];
      *dst2++ = ytab[((y2+12)<<4)|(uvtab[uv+2056]>>4)];

      y2 = *py2++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((((*py++ + y2)>>1)+8)<<4)|(uvtab[uv]>>4)];
      *dst2++ = ytab[((y2+4)<<4)|(uvtab[uv+2056]&15)];

      y2 = *py2++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((((*py++ + y2)>>1)+2)<<4)|(uvtab[uv+1028]&15)];
      *dst2++ = ytab[((y2+14)<<4)|(uvtab[uv+3072]>>4)];

      y2 = *py2++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((((*py++ + y2)>>1)+10)<<4)|(uvtab[uv+1028]>>4)];
      *dst2++ = ytab[((y2+6)<<4)|(uvtab[uv+3072]&15)];
    }

    if (j==0)
      py -= coded_picture_width;
    else
      py += coded_picture_width;

    py2 += coded_picture_width;
    dst += coded_picture_width;
    dst2 += coded_picture_width;

    pu += chrom_width;
    pv += chrom_width;

    /* line j + 2, j + 3 */
    for (i=0; i<coded_picture_width; i+=4)
    {
      y2 = *py2++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((((*py++ + y2)>>1)+3)<<4)|(uvtab[uv+1542]&15)];
      *dst2++ = ytab[((y2+15)<<4)|(uvtab[uv+3598]>>4)];

      y2 = *py2++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((((*py++ + y2)>>1)+11)<<4)|(uvtab[uv+1542]>>4)];
      *dst2++ = ytab[((y2+7)<<4)|(uvtab[uv+3598]&15)];

      y2 = *py2++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((((*py++ + y2)>>1)+1)<<4)|(uvtab[uv+514]&15)];
      *dst2++ = ytab[((y2+13)<<4)|(uvtab[uv+2570]>>4)];

      y2 = *py2++;
      uv = (*pu++<<8)|*pv++;
      *dst++  = ytab[((((*py++ + y2)>>1)+9)<<4)|(uvtab[uv+514]>>4)];
      *dst2++ = ytab[((y2+5)<<4)|(uvtab[uv+2570]&15)];
    }

    py += coded_picture_width;
    py2 += coded_picture_width;
    dst += coded_picture_width;
    dst2 += coded_picture_width;
    pu += chrom_width;
    pv += chrom_width;
  }
}

/* Rui_B */
#endif
/* Rui_E */
