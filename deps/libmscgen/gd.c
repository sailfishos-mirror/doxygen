/* $Id$ */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gd_intern.h"

/* 2.03: don't include zlib here or we can't build without PNG */
#include "gd.h"
#include "gdhelpers.h"
#include "gd_color.h"
#include "gd_errors.h"

/* 2.0.12: this now checks the clipping rectangle */
#define gdImageBoundsSafeMacro(im, x, y) (!((((y) < (im)->cy1) || ((y) > (im)->cy2)) || (((x) < (im)->cx1) || ((x) > (im)->cx2))))

#ifdef _OSD_POSIX		/* BS2000 uses the EBCDIC char set instead of ASCII */
#define CHARSET_EBCDIC
#define __attribute__(any)	/*nothing */
#endif
/*_OSD_POSIX*/

#ifndef CHARSET_EBCDIC
#define ASC(ch)  ch
#else /*CHARSET_EBCDIC */
#define ASC(ch) gd_toascii[(unsigned char)ch]
static const unsigned char gd_toascii[256] = {
	/*00 */ 0x00, 0x01, 0x02, 0x03, 0x85, 0x09, 0x86, 0x7f,
	0x87, 0x8d, 0x8e, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,	/*................ */
	/*10 */ 0x10, 0x11, 0x12, 0x13, 0x8f, 0x0a, 0x08, 0x97,
	0x18, 0x19, 0x9c, 0x9d, 0x1c, 0x1d, 0x1e, 0x1f,	/*................ */
	/*20 */ 0x80, 0x81, 0x82, 0x83, 0x84, 0x92, 0x17, 0x1b,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x05, 0x06, 0x07,	/*................ */
	/*30 */ 0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04,
	0x98, 0x99, 0x9a, 0x9b, 0x14, 0x15, 0x9e, 0x1a,	/*................ */
	/*40 */ 0x20, 0xa0, 0xe2, 0xe4, 0xe0, 0xe1, 0xe3, 0xe5,
	0xe7, 0xf1, 0x60, 0x2e, 0x3c, 0x28, 0x2b, 0x7c,	/* .........`.<(+| */
	/*50 */ 0x26, 0xe9, 0xea, 0xeb, 0xe8, 0xed, 0xee, 0xef,
	0xec, 0xdf, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0x9f,	/*&.........!$*);. */
	/*60 */ 0x2d, 0x2f, 0xc2, 0xc4, 0xc0, 0xc1, 0xc3, 0xc5,
	0xc7, 0xd1, 0x5e, 0x2c, 0x25, 0x5f, 0x3e, 0x3f,
	/*-/........^,%_>?*/
	/*70 */ 0xf8, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf,
	0xcc, 0xa8, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22,	/*..........:#@'=" */
	/*80 */ 0xd8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0xab, 0xbb, 0xf0, 0xfd, 0xfe, 0xb1,	/*.abcdefghi...... */
	/*90 */ 0xb0, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	0x71, 0x72, 0xaa, 0xba, 0xe6, 0xb8, 0xc6, 0xa4,	/*.jklmnopqr...... */
	/*a0 */ 0xb5, 0xaf, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0xa1, 0xbf, 0xd0, 0xdd, 0xde, 0xae,	/*..stuvwxyz...... */
	/*b0 */ 0xa2, 0xa3, 0xa5, 0xb7, 0xa9, 0xa7, 0xb6, 0xbc,
	0xbd, 0xbe, 0xac, 0x5b, 0x5c, 0x5d, 0xb4, 0xd7,	/*...........[\].. */
	/*c0 */ 0xf9, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0xad, 0xf4, 0xf6, 0xf2, 0xf3, 0xf5,	/*.ABCDEFGHI...... */
	/*d0 */ 0xa6, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
	0x51, 0x52, 0xb9, 0xfb, 0xfc, 0xdb, 0xfa, 0xff,	/*.JKLMNOPQR...... */
	/*e0 */ 0xd9, 0xf7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0xb2, 0xd4, 0xd6, 0xd2, 0xd3, 0xd5,	/*..STUVWXYZ...... */
	/*f0 */ 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0xb3, 0x7b, 0xdc, 0x7d, 0xda, 0x7e	/*0123456789.{.}.~ */
};
#endif /*CHARSET_EBCDIC */

extern const int gdCosT[];
extern const int gdSinT[];

/**
 * Group: Error Handling
 */

void gd_stderr_error(int priority, const char *format, va_list args)
{
	switch (priority) {
	case GD_ERROR:
		fputs("GD Error: ", stderr);
		break;
	case GD_WARNING:
		fputs("GD Warning: ", stderr);
		break;
	case GD_NOTICE:
		fputs("GD Notice: ", stderr);
		break;
	case GD_INFO:
		fputs("GD Info: ", stderr);
		break;
	case GD_DEBUG:
		fputs("GD Debug: ", stderr);
		break;
	}
	vfprintf(stderr, format, args);
	fflush(stderr);
}

static gdErrorMethod gd_error_method = gd_stderr_error;

static void _gd_error_ex(int priority, const char *format, va_list args)
{
	if (gd_error_method) {
		gd_error_method(priority, format, args);
	}
}

void gd_error(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	_gd_error_ex(GD_WARNING, format, args);
	va_end(args);
}
void gd_error_ex(int priority, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	_gd_error_ex(priority, format, args);
	va_end(args);
}

/*
	Function: gdSetErrorMethod
*/
BGD_DECLARE(void) gdSetErrorMethod(gdErrorMethod error_method)
{
	gd_error_method = error_method;
}

/*
	Function: gdClearErrorMethod
*/
BGD_DECLARE(void) gdClearErrorMethod(void)
{
	gd_error_method = gd_stderr_error;
}

static void gdImageBrushApply (gdImagePtr im, int x, int y);
static void gdImageTileApply (gdImagePtr im, int x, int y);

BGD_DECLARE(int) gdImageGetTrueColorPixel (gdImagePtr im, int x, int y);

/**
 * Group: Creation and Destruction
 */

/*
    Function: gdImageCreate

      gdImageCreate is called to create palette-based images, with no
      more than 256 colors. The image must eventually be destroyed using
      gdImageDestroy().

    Parameters:

        sx - The image width.
        sy - The image height.

    Returns:

        A pointer to the new image or NULL if an error occurred.

    Example:
      (start code)

      gdImagePtr im;
      im = gdImageCreate(64, 64);
      // ... Use the image ...
      gdImageDestroy(im);

      (end code)

    See Also:

        <gdImageCreateTrueColor>        

 */
BGD_DECLARE(gdImagePtr) gdImageCreate (int sx, int sy)
{
	int i;
	gdImagePtr im;

	if (overflow2(sx, sy)) {
		return NULL;
	}

	if (overflow2(sizeof (unsigned char *), sy)) {
		return NULL;
	}
	if (overflow2(sizeof (unsigned char *), sx)) {
		return NULL;
	}

	im = (gdImage *) gdCalloc(1, sizeof(gdImage));
	if (!im) {
		return NULL;
	}

	/* Row-major ever since gd 1.3 */
	im->pixels = (unsigned char **) gdMalloc (sizeof (unsigned char *) * sy);
	if (!im->pixels) {
		gdFree(im);
		return NULL;
	}

	im->polyInts = 0;
	im->polyAllocated = 0;
	im->brush = 0;
	im->tile = 0;
	im->style = 0;
	for (i = 0; (i < sy); i++) {
		/* Row-major ever since gd 1.3 */
		im->pixels[i] = (unsigned char *) gdCalloc (sx, sizeof (unsigned char));
		if (!im->pixels[i]) {
			for (--i ; i >= 0; i--) {
				gdFree(im->pixels[i]);
			}
			gdFree(im->pixels);
			gdFree(im);
			return NULL;
		}

	}
	im->sx = sx;
	im->sy = sy;
	im->colorsTotal = 0;
	im->transparent = (-1);
	im->interlace = 0;
	im->thick = 1;
	im->AA = 0;
	for (i = 0; (i < gdMaxColors); i++) {
		im->open[i] = 1;
	};
	im->trueColor = 0;
	im->tpixels = 0;
	im->cx1 = 0;
	im->cy1 = 0;
	im->cx2 = im->sx - 1;
	im->cy2 = im->sy - 1;
	im->res_x = GD_RESOLUTION;
	im->res_y = GD_RESOLUTION;
	im->interpolation = NULL;
	im->interpolation_id = GD_BILINEAR_FIXED;
	return im;
}



/*
    Function: gdImageCreateTrueColor

      <gdImageCreateTrueColor> is called to create truecolor images,
      with an essentially unlimited number of colors. Invoke
      <gdImageCreateTrueColor> with the x and y dimensions of the
      desired image. <gdImageCreateTrueColor> returns a <gdImagePtr>
      to the new image, or NULL if unable to allocate the image. The
      image must eventually be destroyed using <gdImageDestroy>().

      Truecolor images are always filled with black at creation
      time. There is no concept of a "background" color index.

    Parameters:

        sx - The image width.
        sy - The image height.

    Returns:

        A pointer to the new image or NULL if an error occurred.

    Example:
      (start code)

      gdImagePtr im;
      im = gdImageCreateTrueColor(64, 64);
      // ... Use the image ...
      gdImageDestroy(im);

      (end code)

    See Also:

        <gdImageCreateTrueColor>        

*/
BGD_DECLARE(gdImagePtr) gdImageCreateTrueColor (int sx, int sy)
{
	int i;
	gdImagePtr im;

	if (overflow2(sx, sy)) {
		return NULL;
	}

	if (overflow2(sizeof (int *), sy)) {
		return 0;
	}

	if (overflow2(sizeof(int *), sx)) {
		return NULL;
	}

	im = (gdImage *) gdMalloc (sizeof (gdImage));
	if (!im) {
		return 0;
	}
	memset (im, 0, sizeof (gdImage));

	im->tpixels = (int **) gdMalloc (sizeof (int *) * sy);
	if (!im->tpixels) {
		gdFree(im);
		return 0;
	}
	im->polyInts = 0;
	im->polyAllocated = 0;
	im->brush = 0;
	im->tile = 0;
	im->style = 0;
	for (i = 0; (i < sy); i++) {
		im->tpixels[i] = (int *) gdCalloc (sx, sizeof (int));
		if (!im->tpixels[i]) {
			/* 2.0.34 */
			i--;
			while (i >= 0) {
				gdFree(im->tpixels[i]);
				i--;
			}
			gdFree(im->tpixels);
			gdFree(im);
			return 0;
		}
	}
	im->sx = sx;
	im->sy = sy;
	im->transparent = (-1);
	im->interlace = 0;
	im->trueColor = 1;
	/* 2.0.2: alpha blending is now on by default, and saving of alpha is
	   off by default. This allows font antialiasing to work as expected
	   on the first try in JPEGs -- quite important -- and also allows
	   for smaller PNGs when saving of alpha channel is not really
	   desired, which it usually isn't! */
	im->saveAlphaFlag = 0;
	im->alphaBlendingFlag = 1;
	im->thick = 1;
	im->AA = 0;
	im->cx1 = 0;
	im->cy1 = 0;
	im->cx2 = im->sx - 1;
	im->cy2 = im->sy - 1;
	im->res_x = GD_RESOLUTION;
	im->res_y = GD_RESOLUTION;
	im->interpolation = NULL;
	im->interpolation_id = GD_BILINEAR_FIXED;
	return im;
}

/*
  Function: gdImageDestroy

    <gdImageDestroy> is used to free the memory associated with an
    image. It is important to invoke <gdImageDestroy> before exiting
    your program or assigning a new image to a <gdImagePtr> variable.

  Parameters:

    im  - Pointer to the gdImage to delete.

  Returns:

    Nothing.

  Example:
    (start code)

    gdImagePtr im;
    im = gdImageCreate(10, 10);
    // ... Use the image ...
    // Now destroy it
    gdImageDestroy(im);

	(end code)

*/

BGD_DECLARE(void) gdImageDestroy (gdImagePtr im)
{
	int i;
	if (im->pixels) {
		for (i = 0; (i < im->sy); i++) {
			gdFree (im->pixels[i]);
		}
		gdFree (im->pixels);
	}
	if (im->tpixels) {
		for (i = 0; (i < im->sy); i++) {
			gdFree (im->tpixels[i]);
		}
		gdFree (im->tpixels);
	}
	if (im->polyInts) {
		gdFree (im->polyInts);
	}
	if (im->style) {
		gdFree (im->style);
	}
	gdFree (im);
}

/**
 * Group: Color
 */

/**
 * Function: gdImageColorClosest
 * 
 * Gets the closest color of the image
 *
 * This is a simplified variant of <gdImageColorClosestAlpha> where the alpha
 * channel is always opaque.
 *
 * Parameters:
 *   im - The image.
 *   r  - The value of the red component.
 *   g  - The value of the green component.
 *   b  - The value of the blue component.
 *
 * Returns:
 *   The closest color already available in the palette for palette images;
 *   the color value of the given components for truecolor images.
 *
 * See also:
 *   - <gdImageColorExact>
 */
BGD_DECLARE(int) gdImageColorClosest (gdImagePtr im, int r, int g, int b)
{
	return gdImageColorClosestAlpha (im, r, g, b, gdAlphaOpaque);
}

/**
 * Function: gdImageColorClosestAlpha
 *
 * Gets the closest color of the image
 *
 * Parameters:
 *   im - The image.
 *   r  - The value of the red component.
 *   g  - The value of the green component.
 *   b  - The value of the blue component.
 *   a  - The value of the alpha component.
 *
 * Returns:
 *   The closest color already available in the palette for palette images;
 *   the color value of the given components for truecolor images.
 *
 * See also:
 *   - <gdImageColorExactAlpha>
 */
BGD_DECLARE(int) gdImageColorClosestAlpha (gdImagePtr im, int r, int g, int b, int a)
{
	int i;
	long rd, gd, bd, ad;
	int ct = (-1);
	int first = 1;
	long mindist = 0;
	if (im->trueColor) {
		return gdTrueColorAlpha (r, g, b, a);
	}
	for (i = 0; (i < (im->colorsTotal)); i++) {
		long dist;
		if (im->open[i]) {
			continue;
		}
		rd = (im->red[i] - r);
		gd = (im->green[i] - g);
		bd = (im->blue[i] - b);
		/* gd 2.02: whoops, was - b (thanks to David Marwood) */
		/* gd 2.16: was blue rather than alpha! Geez! Thanks to
		   Artur Jakub Jerzak */
		ad = (im->alpha[i] - a);
		dist = rd * rd + gd * gd + bd * bd + ad * ad;
		if (first || (dist < mindist)) {
			mindist = dist;
			ct = i;
			first = 0;
		}
	}
	return ct;
}

/* This code is taken from http://www.acm.org/jgt/papers/SmithLyons96/hwb_rgb.html, an article
 * on colour conversion to/from RBG and HWB colour systems.
 * It has been modified to return the converted value as a * parameter.
 */

#define RETURN_HWB(h, w, b) {HWB->H = h; HWB->W = w; HWB->B = b; return HWB;}
#define RETURN_RGB(r, g, b) {RGB->R = r; RGB->G = g; RGB->B = b; return RGB;}
#define HWB_UNDEFINED -1
#define SETUP_RGB(s, r, g, b) {s.R = r/255.0f; s.G = g/255.0f; s.B = b/255.0f;}

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MIN3(a,b,c) ((a)<(b)?(MIN(a,c)):(MIN(b,c)))
#define MAX(a,b) ((a)<(b)?(b):(a))
#define MAX3(a,b,c) ((a)<(b)?(MAX(b,c)):(MAX(a,c)))


/*
 * Theoretically, hue 0 (pure red) is identical to hue 6 in these transforms. Pure
 * red always maps to 6 in this implementation. Therefore UNDEFINED can be
 * defined as 0 in situations where only unsigned numbers are desired.
 */
typedef struct {
	float R, G, B;
}
RGBType;
typedef struct {
	float H, W, B;
}
HWBType;

static HWBType *
RGB_to_HWB (RGBType RGB, HWBType * HWB)
{

	/*
	 * RGB are each on [0, 1]. W and B are returned on [0, 1] and H is
	 * returned on [0, 6]. Exception: H is returned UNDEFINED if W == 1 - B.
	 */

	float R = RGB.R, G = RGB.G, B = RGB.B, w, v, b, f;
	int i;

	w = MIN3 (R, G, B);
	v = MAX3 (R, G, B);
	b = 1 - v;
	if (v == w)
		RETURN_HWB (HWB_UNDEFINED, w, b);
	f = (R == w) ? G - B : ((G == w) ? B - R : R - G);
	i = (R == w) ? 3 : ((G == w) ? 5 : 1);
	RETURN_HWB (i - f / (v - w), w, b);

}

static float
HWB_Diff (int r1, int g1, int b1, int r2, int g2, int b2)
{
	RGBType RGB1, RGB2;
	HWBType HWB1, HWB2;
	float diff;

	SETUP_RGB (RGB1, r1, g1, b1);
	SETUP_RGB (RGB2, r2, g2, b2);

	RGB_to_HWB (RGB1, &HWB1);
	RGB_to_HWB (RGB2, &HWB2);

	/*
	 * I made this bit up; it seems to produce OK results, and it is certainly
	 * more visually correct than the current RGB metric. (PJW)
	 */

	if ((HWB1.H == HWB_UNDEFINED) || (HWB2.H == HWB_UNDEFINED)) {
		diff = 0;			/* Undefined hues always match... */
	} else {
		diff = fabsf(HWB1.H - HWB2.H);
		if (diff > 3) {
			diff = 6 - diff;	/* Remember, it's a colour circle */
		}
	}

	diff =
	    diff * diff + (HWB1.W - HWB2.W) * (HWB1.W - HWB2.W) + (HWB1.B -
	            HWB2.B) * (HWB1.B -
	                       HWB2.B);

	return diff;
}


#if 0
/*
 * This is not actually used, but is here for completeness, in case someone wants to
 * use the HWB stuff for anything else...
 */
static RGBType *
HWB_to_RGB (HWBType HWB, RGBType * RGB)
{

	/*
	 * H is given on [0, 6] or UNDEFINED. W and B are given on [0, 1].
	 * RGB are each returned on [0, 1].
	 */

	float h = HWB.H, w = HWB.W, b = HWB.B, v, n, f;
	int i;

	v = 1 - b;
	if (h == HWB_UNDEFINED)
		RETURN_RGB (v, v, v);
	i = floor (h);
	f = h - i;
	if (i & 1)
		f = 1 - f;			/* if i is odd */
	n = w + f * (v - w);		/* linear interpolation between w and v */
	switch (i) {
	case 6:
	case 0:
		RETURN_RGB (v, n, w);
	case 1:
		RETURN_RGB (n, v, w);
	case 2:
		RETURN_RGB (w, v, n);
	case 3:
		RETURN_RGB (w, n, v);
	case 4:
		RETURN_RGB (n, w, v);
	case 5:
		RETURN_RGB (v, w, n);
	}

	return RGB;

}
#endif

/*
	Function: gdImageColorClosestHWB
*/
BGD_DECLARE(int) gdImageColorClosestHWB (gdImagePtr im, int r, int g, int b)
{
	int i;
	/* long rd, gd, bd; */
	int ct = (-1);
	int first = 1;
	float mindist = 0;
	if (im->trueColor) {
		return gdTrueColor (r, g, b);
	}
	for (i = 0; (i < (im->colorsTotal)); i++) {
		float dist;
		if (im->open[i]) {
			continue;
		}
		dist = HWB_Diff (im->red[i], im->green[i], im->blue[i], r, g, b);
		if (first || (dist < mindist)) {
			mindist = dist;
			ct = i;
			first = 0;
		}
	}
	return ct;
}

/**
 * Function: gdImageColorExact
 *
 * Gets the exact color of the image
 *
 * This is a simplified variant of <gdImageColorExactAlpha> where the alpha
 * channel is always opaque.
 *
 * Parameters:
 *   im - The image.
 *   r  - The value of the red component.
 *   g  - The value of the green component.
 *   b  - The value of the blue component.
 *
 * Returns:
 *   The exact color already available in the palette for palette images; if
 *   there is no exact color, -1 is returned.
 *   For truecolor images the color value of the given components is returned.
 *
 * See also:
 *   - <gdImageColorClosest>
 */
BGD_DECLARE(int) gdImageColorExact (gdImagePtr im, int r, int g, int b)
{
	return gdImageColorExactAlpha (im, r, g, b, gdAlphaOpaque);
}

/**
 * Function: gdImageColorExactAlpha
 *
 * Gets the exact color of the image
 *
 * Parameters:
 *   im - The image.
 *   r  - The value of the red component.
 *   g  - The value of the green component.
 *   b  - The value of the blue component.
 *   a  - The value of the alpha component.
 *
 * Returns:
 *   The exact color already available in the palette for palette images; if
 *   there is no exact color, -1 is returned.
 *   For truecolor images the color value of the given components is returned.
 *
 * See also:
 *   - <gdImageColorClosestAlpha>
 *   - <gdTrueColorAlpha>
 */
BGD_DECLARE(int) gdImageColorExactAlpha (gdImagePtr im, int r, int g, int b, int a)
{
	int i;
	if (im->trueColor) {
		return gdTrueColorAlpha (r, g, b, a);
	}
	for (i = 0; (i < (im->colorsTotal)); i++) {
		if (im->open[i]) {
			continue;
		}
		if ((im->red[i] == r) &&
		        (im->green[i] == g) && (im->blue[i] == b) && (im->alpha[i] == a)) {
			return i;
		}
	}
	return -1;
}

/**
 * Function: gdImageColorAllocate
 *
 * Allocates a color
 *
 * This is a simplified variant of <gdImageColorAllocateAlpha> where the alpha
 * channel is always opaque.
 *
 * Parameters:
 *   im - The image.
 *   r  - The value of the red component.
 *   g  - The value of the green component.
 *   b  - The value of the blue component.
 *
 * Returns:
 *   The color value.
 *
 * See also:
 *   - <gdImageColorDeallocate>
 */
BGD_DECLARE(int) gdImageColorAllocate (gdImagePtr im, int r, int g, int b)
{
	return gdImageColorAllocateAlpha (im, r, g, b, gdAlphaOpaque);
}

/**
 * Function: gdImageColorAllocateAlpha
 *
 * Allocates a color
 *
 * This is typically used for palette images, but can be used for truecolor
 * images as well.
 *
 * Parameters:
 *   im - The image.
 *   r  - The value of the red component.
 *   g  - The value of the green component.
 *   b  - The value of the blue component.
 *
 * Returns:
 *   The color value.
 *
 * See also:
 *   - <gdImageColorDeallocate>
 */
BGD_DECLARE(int) gdImageColorAllocateAlpha (gdImagePtr im, int r, int g, int b, int a)
{
	int i;
	int ct = (-1);
	if (im->trueColor) {
		return gdTrueColorAlpha (r, g, b, a);
	}
	for (i = 0; (i < (im->colorsTotal)); i++) {
		if (im->open[i]) {
			ct = i;
			break;
		}
	}
	if (ct == (-1)) {
		ct = im->colorsTotal;
		if (ct == gdMaxColors) {
			return -1;
		}
		im->colorsTotal++;
	}
	im->red[ct] = r;
	im->green[ct] = g;
	im->blue[ct] = b;
	im->alpha[ct] = a;
	im->open[ct] = 0;
	return ct;
}

/*
	Function: gdImageColorResolve

	gdImageColorResolve is an alternative for the code fragment
	(start code)
	if ((color=gdImageColorExact(im,R,G,B)) < 0)
	  if ((color=gdImageColorAllocate(im,R,G,B)) < 0)
	    color=gdImageColorClosest(im,R,G,B);
	(end code)
	in a single function.    Its advantage is that it is guaranteed to
	return a color index in one search over the color table.
*/

BGD_DECLARE(int) gdImageColorResolve (gdImagePtr im, int r, int g, int b)
{
	return gdImageColorResolveAlpha (im, r, g, b, gdAlphaOpaque);
}

/*
	Function: gdImageColorResolveAlpha
*/
BGD_DECLARE(int) gdImageColorResolveAlpha (gdImagePtr im, int r, int g, int b, int a)
{
	int c;
	int ct = -1;
	int op = -1;
	long rd, gd, bd, ad, dist;
	long mindist = 4 * 255 * 255;	/* init to max poss dist */
	if (im->trueColor) {
		return gdTrueColorAlpha (r, g, b, a);
	}

	for (c = 0; c < im->colorsTotal; c++) {
		if (im->open[c]) {
			op = c;		/* Save open slot */
			continue;		/* Color not in use */
		}
		if (c == im->transparent) {
			/* don't ever resolve to the color that has
			 * been designated as the transparent color */
			continue;
		}
		rd = (long) (im->red[c] - r);
		gd = (long) (im->green[c] - g);
		bd = (long) (im->blue[c] - b);
		ad = (long) (im->alpha[c] - a);
		dist = rd * rd + gd * gd + bd * bd + ad * ad;
		if (dist < mindist) {
			if (dist == 0) {
				return c;		/* Return exact match color */
			}
			mindist = dist;
			ct = c;
		}
	}
	/* no exact match.  We now know closest, but first try to allocate exact */
	if (op == -1) {
		op = im->colorsTotal;
		if (op == gdMaxColors) {
			/* No room for more colors */
			return ct;		/* Return closest available color */
		}
		im->colorsTotal++;
	}
	im->red[op] = r;
	im->green[op] = g;
	im->blue[op] = b;
	im->alpha[op] = a;
	im->open[op] = 0;
	return op;			/* Return newly allocated color */
}

/**
 * Function: gdImageColorDeallocate
 *
 * Removes a palette entry
 *
 * This is a no-op for truecolor images.
 *
 * Parameters:
 *   im    - The image.
 *   color - The palette index.
 *
 * See also:
 *   - <gdImageColorAllocate>
 *   - <gdImageColorAllocateAlpha>
 */
BGD_DECLARE(void) gdImageColorDeallocate (gdImagePtr im, int color)
{
	if (im->trueColor || (color >= gdMaxColors) || (color < 0)) {
		return;
	}
	/* Mark it open. */
	im->open[color] = 1;
}

/**
 * Function: gdImageColorTransparent
 *
 * Sets the transparent color of the image
 *
 * Parameter:
 *   im    - The image.
 *   color - The color.
 *
 * See also:
 *   - <gdImageGetTransparent>
 */
BGD_DECLARE(void) gdImageColorTransparent (gdImagePtr im, int color)
{
	if (color < 0) {
		return;
	}

	if (!im->trueColor) {
		if((color < -1) || (color >= gdMaxColors)) {
			return;
		}
		if (im->transparent != -1) {
			im->alpha[im->transparent] = gdAlphaOpaque;
		}
		if (color != -1) {
			im->alpha[color] = gdAlphaTransparent;
		}
	}
	im->transparent = color;
}

/*
	Function: gdImagePaletteCopy
*/
BGD_DECLARE(void) gdImagePaletteCopy (gdImagePtr to, gdImagePtr from)
{
	int i;
	int x, y, p;
	int xlate[256];
	if (to->trueColor) {
		return;
	}
	if (from->trueColor) {
		return;
	}

	for (i = 0; i < 256; i++) {
		xlate[i] = -1;
	};

	for (y = 0; y < (to->sy); y++) {
		for (x = 0; x < (to->sx); x++) {
			/* Optimization: no gdImageGetPixel */
			p = to->pixels[y][x];
			if (xlate[p] == -1) {
				/* This ought to use HWB, but we don't have an alpha-aware
				   version of that yet. */
				xlate[p] =
				    gdImageColorClosestAlpha (from, to->red[p], to->green[p],
				                              to->blue[p], to->alpha[p]);
				/*printf("Mapping %d (%d, %d, %d, %d) to %d (%d, %d, %d, %d)\n", */
				/*      p,  to->red[p], to->green[p], to->blue[p], to->alpha[p], */
				/*      xlate[p], from->red[xlate[p]], from->green[xlate[p]], from->blue[xlate[p]], from->alpha[xlate[p]]); */
			};
			/* Optimization: no gdImageSetPixel */
			to->pixels[y][x] = xlate[p];
		};
	};

	for (i = 0; (i < (from->colorsTotal)); i++) {
		/*printf("Copying color %d (%d, %d, %d, %d)\n", i, from->red[i], from->blue[i], from->green[i], from->alpha[i]); */
		to->red[i] = from->red[i];
		to->blue[i] = from->blue[i];
		to->green[i] = from->green[i];
		to->alpha[i] = from->alpha[i];
		to->open[i] = 0;
	};

	for (i = from->colorsTotal; (i < to->colorsTotal); i++) {
		to->open[i] = 1;
	};

	to->colorsTotal = from->colorsTotal;

}

/*
	Function: gdImageColorReplace
*/
BGD_DECLARE(int) gdImageColorReplace (gdImagePtr im, int src, int dst)
{
	register int x, y;
	int n = 0;

	if (src == dst) {
		return 0;
	}

#define REPLACING_LOOP(pixel) do {								\
		for (y = im->cy1; y <= im->cy2; y++) {					\
			for (x = im->cx1; x <= im->cx2; x++) {				\
				if (pixel(im, x, y) == src) {					\
					gdImageSetPixel(im, x, y, dst);				\
					n++;										\
				}												\
			}													\
		}														\
	} while (0)

	if (im->trueColor) {
		REPLACING_LOOP(gdImageTrueColorPixel);
	} else {
		REPLACING_LOOP(gdImagePalettePixel);
	}

#undef REPLACING_LOOP

	return n;
}

/*
	Function: gdImageColorReplaceThreshold
*/
BGD_DECLARE(int) gdImageColorReplaceThreshold (gdImagePtr im, int src, int dst, float threshold)
{
	register int x, y;
	int n = 0;

	if (src == dst) {
		return 0;
	}

#define REPLACING_LOOP(pixel) do {										\
		for (y = im->cy1; y <= im->cy2; y++) {							\
			for (x = im->cx1; x <= im->cx2; x++) {						\
				if (gdColorMatch(im, src, pixel(im, x, y), threshold)) { \
					gdImageSetPixel(im, x, y, dst);						\
					n++;												\
				}														\
			}															\
		}																\
	} while (0)

	if (im->trueColor) {
		REPLACING_LOOP(gdImageTrueColorPixel);
	} else {
		REPLACING_LOOP(gdImagePalettePixel);
	}

#undef REPLACING_LOOP

	return n;
}

static int colorCmp (const void *x, const void *y)
{
	int a = *(int const *)x;
	int b = *(int const *)y;
	return (a > b) - (a < b);
}

/*
	Function: gdImageColorReplaceArray
*/
BGD_DECLARE(int) gdImageColorReplaceArray (gdImagePtr im, int len, int *src, int *dst)
{
	register int x, y;
	int c, *d, *base;
	int i, n = 0;

	if (len <= 0 || src == dst) {
		return 0;
	}
	if (len == 1) {
		return gdImageColorReplace(im, src[0], dst[0]);
	}
	if (overflow2(len, sizeof(int)<<1)) {
		return -1;
	}
	base = (int *)gdMalloc(len * (sizeof(int)<<1));
	if (!base) {
		return -1;
	}
	for (i = 0; i < len; i++) {
		base[(i<<1)]   = src[i];
		base[(i<<1)+1] = dst[i];
	}
	qsort(base, len, sizeof(int)<<1, colorCmp);

#define REPLACING_LOOP(pixel) do {										\
		for (y = im->cy1; y <= im->cy2; y++) {							\
			for (x = im->cx1; x <= im->cx2; x++) {						\
				c = pixel(im, x, y);									\
				if ( (d = (int *)bsearch(&c, base, len, sizeof(int)<<1, colorCmp)) ) { \
					gdImageSetPixel(im, x, y, d[1]);					\
					n++;												\
				}														\
			}															\
		}																\
	} while (0)

	if (im->trueColor) {
		REPLACING_LOOP(gdImageTrueColorPixel);
	} else {
		REPLACING_LOOP(gdImagePalettePixel);
	}

#undef REPLACING_LOOP

	gdFree(base);
	return n;
}

/*
	Function: gdImageColorReplaceCallback
*/
BGD_DECLARE(int) gdImageColorReplaceCallback (gdImagePtr im, gdCallbackImageColor callback)
{
	int n = 0;

	if (!callback) {
		return 0;
	}
	if (im->trueColor) {
		for (int y = im->cy1; y <= im->cy2; y++) {
			for (int x = im->cx1; x <= im->cx2; x++) {
				int c = gdImageTrueColorPixel(im, x, y);
				int d = callback(im, c);
				if (d != c) {
					gdImageSetPixel(im, x, y, d);
					n++;
				}
			}
		}
	} else { /* palette */
		int len = 0;

		int *sarr = (int *)gdCalloc(im->colorsTotal, sizeof(int));
		if (!sarr) {
			return -1;
		}
		for (int c = 0; c < im->colorsTotal; c++) {
			if (!im->open[c]) {
				sarr[len++] = c;
			}
		}
		int *darr = (int *)gdCalloc(len, sizeof(int));
		if (!darr) {
			gdFree(sarr);
			return -1;
		}
		for (int k = 0; k < len; k++) {
			darr[k] = callback(im, sarr[k]);
		}
		n = gdImageColorReplaceArray(im, len, sarr, darr);
		gdFree(darr);
		gdFree(sarr);
	}
	return n;
}

/* 2.0.10: before the drawing routines, some code to clip points that are
 * outside the drawing window.  Nick Atty (nick@canalplan.org.uk)
 *
 * This is the Sutherland Hodgman Algorithm, as implemented by
 * Duvanenko, Robbins and Gyurcsik - SH(DRG) for short.  See Dr Dobb's
 * Journal, January 1996, pp107-110 and 116-117
 *
 * Given the end points of a line, and a bounding rectangle (which we
 * know to be from (0,0) to (SX,SY)), adjust the endpoints to be on
 * the edges of the rectangle if the line should be drawn at all,
 * otherwise return a failure code */

/* this does "one-dimensional" clipping: note that the second time it
   is called, all the x parameters refer to height and the y to width
   - the comments ignore this (if you can understand it when it's
   looking at the X parameters, it should become clear what happens on
   the second call!)  The code is simplified from that in the article,
   as we know that gd images always start at (0,0) */

/* 2.0.26, TBB: we now have to respect a clipping rectangle, it won't
	necessarily start at 0. */

static int
clip_1d (int *x0, int *y0, int *x1, int *y1, int mindim, int maxdim)
{
	double m;			/* gradient of line */
	if (*x0 < mindim) {
		/* start of line is left of window */
		if (*x1 < mindim)		/* as is the end, so the line never cuts the window */
			return 0;
		m = (*y1 - *y0) / (double) (*x1 - *x0);	/* calculate the slope of the line */
		/* adjust x0 to be on the left boundary (ie to be zero), and y0 to match */
		*y0 -= (int)(m * (*x0 - mindim));
		*x0 = mindim;
		/* now, perhaps, adjust the far end of the line as well */
		if (*x1 > maxdim) {
			*y1 += (int)(m * (maxdim - *x1));
			*x1 = maxdim;
		}
		return 1;
	}
	if (*x0 > maxdim) {
		/* start of line is right of window -
		complement of above */
		if (*x1 > maxdim)		/* as is the end, so the line misses the window */
			return 0;
		m = (*y1 - *y0) / (double) (*x1 - *x0);	/* calculate the slope of the line */
		*y0 += (int)(m * (maxdim - *x0));	/* adjust so point is on the right
							   boundary */
		*x0 = maxdim;
		/* now, perhaps, adjust the end of the line */
		if (*x1 < mindim) {
			*y1 -= (int)(m * (*x1 - mindim));
			*x1 = mindim;
		}
		return 1;
	}
	/* the final case - the start of the line is inside the window */
	if (*x1 > maxdim) {
		/* other end is outside to the right */
		m = (*y1 - *y0) / (double) (*x1 - *x0);	/* calculate the slope of the line */
		*y1 += (int)(m * (maxdim - *x1));
		*x1 = maxdim;
		return 1;
	}
	if (*x1 < mindim) {
		/* other end is outside to the left */
		m = (*y1 - *y0) / (double) (*x1 - *x0);	/* calculate the slope of the line */
		*y1 -= (int)(m * (*x1 - mindim));
		*x1 = mindim;
		return 1;
	}
	/* only get here if both points are inside the window */
	return 1;
}

/* end of line clipping code */

/**
 * Group: Pixels
 */

/*
	Function: gdImageSetPixel
*/
BGD_DECLARE(void) gdImageSetPixel (gdImagePtr im, int x, int y, int color)
{
	int p;
	switch (color) {
	case gdStyled:
		if (!im->style) {
			/* Refuse to draw if no style is set. */
			return;
		} else {
			p = im->style[im->stylePos++];
		}
		if (p != (gdTransparent)) {
			gdImageSetPixel (im, x, y, p);
		}
		im->stylePos = im->stylePos % im->styleLength;
		break;
	case gdStyledBrushed:
		if (!im->style) {
			/* Refuse to draw if no style is set. */
			return;
		}
		p = im->style[im->stylePos++];
		if ((p != gdTransparent) && (p != 0)) {
			gdImageSetPixel (im, x, y, gdBrushed);
		}
		im->stylePos = im->stylePos % im->styleLength;
		break;
	case gdBrushed:
		gdImageBrushApply (im, x, y);
		break;
	case gdTiled:
		gdImageTileApply (im, x, y);
		break;
	case gdAntiAliased:
		/* This shouldn't happen (2.0.26) because we just call
		  gdImageAALine now, but do something sane. */
		gdImageSetPixel(im, x, y, im->AA_color);
		break;
	default:
		if (gdImageBoundsSafeMacro (im, x, y)) {
			if (im->trueColor) {
				switch (im->alphaBlendingFlag) {
					default:
					case gdEffectReplace:
						im->tpixels[y][x] = color;
						break;
					case gdEffectAlphaBlend:
					case gdEffectNormal:
						im->tpixels[y][x] = gdAlphaBlend(im->tpixels[y][x], color);
						break;
					case gdEffectOverlay :
						im->tpixels[y][x] = gdLayerOverlay(im->tpixels[y][x], color);
						break;
					case gdEffectMultiply :
						im->tpixels[y][x] = gdLayerMultiply(im->tpixels[y][x], color);
						break;
				}
			} else {
				im->pixels[y][x] = color;
			}
		}
		break;
	}
}

static void
gdImageBrushApply (gdImagePtr im, int x, int y)
{
	int lx, ly;
	int hy;
	int hx;
	int x1, y1, x2, y2;
	int srcx, srcy;
	if (!im->brush) {
		return;
	}
	hy = gdImageSY (im->brush) / 2;
	y1 = y - hy;
	y2 = y1 + gdImageSY (im->brush);
	hx = gdImageSX (im->brush) / 2;
	x1 = x - hx;
	x2 = x1 + gdImageSX (im->brush);
	srcy = 0;
	if (im->trueColor) {
		if (im->brush->trueColor) {
			for (ly = y1; (ly < y2); ly++) {
				srcx = 0;
				for (lx = x1; (lx < x2); lx++) {
					int p;
					p = gdImageGetTrueColorPixel (im->brush, srcx, srcy);
					/* 2.0.9, Thomas Winzig: apply simple full transparency */
					if (p != gdImageGetTransparent (im->brush)) {
						gdImageSetPixel (im, lx, ly, p);
					}
					srcx++;
				}
				srcy++;
			}
		} else {
			/* 2.0.12: Brush palette, image truecolor (thanks to Thorben Kundinger
			   for pointing out the issue) */
			for (ly = y1; (ly < y2); ly++) {
				srcx = 0;
				for (lx = x1; (lx < x2); lx++) {
					int p, tc;
					p = gdImageGetPixel (im->brush, srcx, srcy);
					tc = gdImageGetTrueColorPixel (im->brush, srcx, srcy);
					/* 2.0.9, Thomas Winzig: apply simple full transparency */
					if (p != gdImageGetTransparent (im->brush)) {
						gdImageSetPixel (im, lx, ly, tc);
					}
					srcx++;
				}
				srcy++;
			}
		}
	} else {
		for (ly = y1; (ly < y2); ly++) {
			srcx = 0;
			for (lx = x1; (lx < x2); lx++) {
				int p;
				p = gdImageGetPixel (im->brush, srcx, srcy);
				/* Allow for non-square brushes! */
				if (p != gdImageGetTransparent (im->brush)) {
					/* Truecolor brush. Very slow
					   on a palette destination. */
					if (im->brush->trueColor) {
						gdImageSetPixel (im, lx, ly,
						                 gdImageColorResolveAlpha (im,
						                         gdTrueColorGetRed
						                         (p),
						                         gdTrueColorGetGreen
						                         (p),
						                         gdTrueColorGetBlue
						                         (p),
						                         gdTrueColorGetAlpha
						                         (p)));
					} else {
						gdImageSetPixel (im, lx, ly, im->brushColorMap[p]);
					}
				}
				srcx++;
			}
			srcy++;
		}
	}
}

static void
gdImageTileApply (gdImagePtr im, int x, int y)
{
	gdImagePtr tile = im->tile;
	int srcx, srcy;
	int p;
	if (!tile) {
		return;
	}
	srcx = x % gdImageSX (tile);
	srcy = y % gdImageSY (tile);
	if (im->trueColor) {
		p = gdImageGetPixel (tile, srcx, srcy);
		if (p != gdImageGetTransparent (tile)) {
			if (!tile->trueColor) {
				p = gdTrueColorAlpha(tile->red[p], tile->green[p], tile->blue[p], tile->alpha[p]);
			}
			gdImageSetPixel (im, x, y, p);
		}
	} else {
		p = gdImageGetPixel (tile, srcx, srcy);
		/* Allow for transparency */
		if (p != gdImageGetTransparent (tile)) {
			if (tile->trueColor) {
				/* Truecolor tile. Very slow
				   on a palette destination. */
				gdImageSetPixel (im, x, y,
				                 gdImageColorResolveAlpha (im,
				                         gdTrueColorGetRed
				                         (p),
				                         gdTrueColorGetGreen
				                         (p),
				                         gdTrueColorGetBlue
				                         (p),
				                         gdTrueColorGetAlpha
				                         (p)));
			} else {
				gdImageSetPixel (im, x, y, im->tileColorMap[p]);
			}
		}
	}
}

/**
 * Function: gdImageGetPixel
 *
 * Gets a pixel color as stored in the image.
 *
 * Parameters:
 *   im - The image.
 *   x  - The x-coordinate.
 *   y  - The y-coordinate.
 *
 * See also:
 *   - <gdImageGetTrueColorPixel>
 *   - <gdImagePalettePixel>
 *   - <gdImageTrueColorPixel>
 */
BGD_DECLARE(int) gdImageGetPixel (gdImagePtr im, int x, int y)
{
	if (gdImageBoundsSafeMacro (im, x, y)) {
		if (im->trueColor) {
			return im->tpixels[y][x];
		} else {
			return im->pixels[y][x];
		}
	} else {
		return 0;
	}
}

/**
 * Function: gdImageGetTrueColorPixel
 *
 * Gets a pixel color always as truecolor value.
 *
 * Parameters:
 *   im - The image.
 *   x  - The x-coordinate.
 *   y  - The y-coordinate.
 *
 * See also:
 *   - <gdImageGetPixel>
 *   - <gdImageTrueColorPixel>
 */
BGD_DECLARE(int) gdImageGetTrueColorPixel (gdImagePtr im, int x, int y)
{
	int p = gdImageGetPixel (im, x, y);
	if (!im->trueColor) {
		return gdTrueColorAlpha (im->red[p], im->green[p], im->blue[p],
		                         (im->transparent == p) ? gdAlphaTransparent :
		                         im->alpha[p]);
	} else {
		return p;
	}
}

/**
 * Group: Primitives
 */

/*
	Function: gdImageAABlend
	
	NO-OP, kept for library compatibility.
*/
BGD_DECLARE(void) gdImageAABlend (gdImagePtr im)
{
	(void)im;
}

static void gdImageAALine (gdImagePtr im, int x1, int y1, int x2, int y2, int col);

static void _gdImageFilledHRectangle (gdImagePtr im, int x1, int y1, int x2, int y2,
		int color);

static void gdImageHLine(gdImagePtr im, int y, int x1, int x2, int col)
{
	if (im->thick > 1) {
		int thickhalf = im->thick >> 1;
		_gdImageFilledHRectangle(im, x1, y - thickhalf, x2, y + im->thick - thickhalf - 1, col);
	} else {
		if (x2 < x1) {
			int t = x2;
			x2 = x1;
			x1 = t;
		}

		for (; x1 <= x2; x1++) {
			gdImageSetPixel(im, x1, y, col);
		}
	}
	return;
}

static void gdImageVLine(gdImagePtr im, int x, int y1, int y2, int col)
{
	if (im->thick > 1) {
		int thickhalf = im->thick >> 1;
		gdImageFilledRectangle(im, x - thickhalf, y1, x + im->thick - thickhalf - 1, y2, col);
	} else {
		if (y2 < y1) {
			int t = y1;
			y1 = y2;
			y2 = t;
		}

		for (; y1 <= y2; y1++) {
			gdImageSetPixel(im, x, y1, col);
		}
	}
	return;
}

/*
	Function: gdImageLine
	
	Bresenham as presented in Foley & Van Dam.
*/
BGD_DECLARE(void) gdImageLine (gdImagePtr im, int x1, int y1, int x2, int y2, int color)
{
	int dx, dy, incr1, incr2, d, x, y, xend, yend, xdirflag, ydirflag;
	int wid;
	int w, wstart;
	int thick;

	if (color == gdAntiAliased) {
		/*
		  gdAntiAliased passed as color: use the much faster, much cheaper
		  and equally attractive gdImageAALine implementation. That
		  clips too, so don't clip twice.
		*/
		gdImageAALine(im, x1, y1, x2, y2, im->AA_color);
		return;
	}
	/* 2.0.10: Nick Atty: clip to edges of drawing rectangle, return if no
	   points need to be drawn. 2.0.26, TBB: clip to edges of clipping
	   rectangle. We were getting away with this because gdImageSetPixel
	   is used for actual drawing, but this is still more efficient and opens
	   the way to skip per-pixel bounds checking in the future. */

	if (clip_1d (&x1, &y1, &x2, &y2, im->cx1, im->cx2) == 0)
		return;
	if (clip_1d (&y1, &x1, &y2, &x2, im->cy1, im->cy2) == 0)
		return;
	thick = im->thick;

	dx = abs (x2 - x1);
	dy = abs (y2 - y1);

	if (dx == 0) {
		gdImageVLine(im, x1, y1, y2, color);
		return;
	} else if (dy == 0) {
		gdImageHLine(im, y1, x1, x2, color);
		return;
	}

	if (dy <= dx) {
		/* More-or-less horizontal. use wid for vertical stroke */
		/* Doug Claar: watch out for NaN in atan2 (2.0.5) */
		
		/* 2.0.12: Michael Schwartz: divide rather than multiply;
			  TBB: but watch out for /0! */
		double ac = cos (atan2 (dy, dx));
		if (ac != 0) {
			wid = (int)(thick / ac);
		} else {
			wid = 1;
		}
		if (wid == 0) {
			wid = 1;
		}
		d = 2 * dy - dx;
		incr1 = 2 * dy;
		incr2 = 2 * (dy - dx);
		if (x1 > x2) {
			x = x2;
			y = y2;
			ydirflag = (-1);
			xend = x1;
		} else {
			x = x1;
			y = y1;
			ydirflag = 1;
			xend = x2;
		}

		/* Set up line thickness */
		wstart = y - wid / 2;
		for (w = wstart; w < wstart + wid; w++)
			gdImageSetPixel (im, x, w, color);

		if (((y2 - y1) * ydirflag) > 0) {
			while (x < xend) {
				x++;
				if (d < 0) {
					d += incr1;
				} else {
					y++;
					d += incr2;
				}
				wstart = y - wid / 2;
				for (w = wstart; w < wstart + wid; w++)
					gdImageSetPixel (im, x, w, color);
			}
		} else {
			while (x < xend) {
				x++;
				if (d < 0) {
					d += incr1;
				} else {
					y--;
					d += incr2;
				}
				wstart = y - wid / 2;
				for (w = wstart; w < wstart + wid; w++)
					gdImageSetPixel (im, x, w, color);
			}
		}
	} else {
		/* More-or-less vertical. use wid for horizontal stroke */
		/* 2.0.12: Michael Schwartz: divide rather than multiply;
		   TBB: but watch out for /0! */
		double as = sin (atan2 (dy, dx));
		if (as != 0) {
			wid = (int)(thick / as);
		} else {
			wid = 1;
		}
		if (wid == 0)
			wid = 1;

		d = 2 * dx - dy;
		incr1 = 2 * dx;
		incr2 = 2 * (dx - dy);
		if (y1 > y2) {
			y = y2;
			x = x2;
			yend = y1;
			xdirflag = (-1);
		} else {
			y = y1;
			x = x1;
			yend = y2;
			xdirflag = 1;
		}

		/* Set up line thickness */
		wstart = x - wid / 2;
		for (w = wstart; w < wstart + wid; w++)
			gdImageSetPixel (im, w, y, color);

		if (((x2 - x1) * xdirflag) > 0) {
			while (y < yend) {
				y++;
				if (d < 0) {
					d += incr1;
				} else {
					x++;
					d += incr2;
				}
				wstart = x - wid / 2;
				for (w = wstart; w < wstart + wid; w++)
					gdImageSetPixel (im, w, y, color);
			}
		} else {
			while (y < yend) {
				y++;
				if (d < 0) {
					d += incr1;
				} else {
					x--;
					d += incr2;
				}
				wstart = x - wid / 2;
				for (w = wstart; w < wstart + wid; w++)
					gdImageSetPixel (im, w, y, color);
			}
		}
	}

}
static void dashedSet (gdImagePtr im, int x, int y, int color,
					   int *onP, int *dashStepP, int wid, int vert);

/*
	Function: gdImageDashedLine
*/
BGD_DECLARE(void) gdImageDashedLine (gdImagePtr im, int x1, int y1, int x2, int y2, int color)
{
	int x, y;
	int dashStep = 0;
	int on = 1;
	int wid;
	int vert;

	const int dx = abs (x2 - x1);
	const int dy = abs (y2 - y1);
	if (dy <= dx) {
		/* More-or-less horizontal. use wid for vertical stroke */
		/* 2.0.12: Michael Schwartz: divide rather than multiply;
		   TBB: but watch out for /0! */
		double as = sin (atan2 (dy, dx));
		if (as != 0) {
			wid = (int)(im->thick / as);
		} else {
			wid = 1;
		}
		vert = 1;

		int d = 2 * dy - dx;
		const int incr1 = 2 * dy;
		const int incr2 = 2 * (dy - dx);
		int xend;
		int ydirflag;
		if (x1 > x2) {
			x = x2;
			y = y2;
			ydirflag = (-1);
			xend = x1;
		} else {
			x = x1;
			y = y1;
			ydirflag = 1;
			xend = x2;
		}
		dashedSet (im, x, y, color, &on, &dashStep, wid, vert);
		if (((y2 - y1) * ydirflag) > 0) {
			while (x < xend) {
				x++;
				if (d < 0) {
					d += incr1;
				} else {
					y++;
					d += incr2;
				}
				dashedSet (im, x, y, color, &on, &dashStep, wid, vert);
			}
		} else {
			while (x < xend) {
				x++;
				if (d < 0) {
					d += incr1;
				} else {
					y--;
					d += incr2;
				}
				dashedSet (im, x, y, color, &on, &dashStep, wid, vert);
			}
		}
	} else {
		/* 2.0.12: Michael Schwartz: divide rather than multiply;
		   TBB: but watch out for /0! */
		double as = sin (atan2 (dy, dx));
		if (as != 0) {
			wid = (int)(im->thick / as);
		} else {
			wid = 1;
		}
		vert = 0;

		int d = 2 * dx - dy;
		const int incr1 = 2 * dx;
		const int incr2 = 2 * (dx - dy);
		int yend;
		int xdirflag;
		if (y1 > y2) {
			y = y2;
			x = x2;
			yend = y1;
			xdirflag = (-1);
		} else {
			y = y1;
			x = x1;
			yend = y2;
			xdirflag = 1;
		}
		dashedSet (im, x, y, color, &on, &dashStep, wid, vert);
		if (((x2 - x1) * xdirflag) > 0) {
			while (y < yend) {
				y++;
				if (d < 0) {
					d += incr1;
				} else {
					x++;
					d += incr2;
				}
				dashedSet (im, x, y, color, &on, &dashStep, wid, vert);
			}
		} else {
			while (y < yend) {
				y++;
				if (d < 0) {
					d += incr1;
				} else {
					x--;
					d += incr2;
				}
				dashedSet (im, x, y, color, &on, &dashStep, wid, vert);
			}
		}
	}
}

static void
dashedSet (gdImagePtr im, int x, int y, int color,
		   int *onP, int *dashStepP, int wid, int vert)
{
	int dashStep = *dashStepP;
	int on = *onP;

	dashStep++;
	if (dashStep == gdDashSize) {
		dashStep = 0;
		on = !on;
	}
	if (on) {
		if (vert) {
			int wstart = y - wid / 2;
			for (int w = wstart; w < wstart + wid; w++)
				gdImageSetPixel (im, x, w, color);
		} else {
			int wstart = x - wid / 2;
			for (int w = wstart; w < wstart + wid; w++)
				gdImageSetPixel (im, w, y, color);
		}
	}
	*dashStepP = dashStep;
	*onP = on;
}

/*
	Function: gdImageBoundsSafe
*/
BGD_DECLARE(int) gdImageBoundsSafe (gdImagePtr im, int x, int y)
{
	return gdImageBoundsSafeMacro (im, x, y);
}

/**
 * Function: gdImageChar
 *
 * Draws a single character.
 *
 * Parameters:
 *  im    - The image to draw onto.
 *  f     - The raster font.
 *  x     - The x coordinate of the upper left pixel.
 *  y     - The y coordinate of the upper left pixel.
 *  c     - The character.
 *  color - The color.
 *
 * Variants:
 *  - <gdImageCharUp>
 *
 * See also:
 *  - <gdFontPtr>
 */
BGD_DECLARE(void) gdImageChar (gdImagePtr im, gdFontPtr f, int x, int y, int c, int color)
{
	int cx, cy;
	int px, py;
	int fline;
	cx = 0;
	cy = 0;
#ifdef CHARSET_EBCDIC
	c = ASC (c);
#endif /*CHARSET_EBCDIC */
	if ((c < f->offset) || (c >= (f->offset + f->nchars))) {
		return;
	}
	fline = (c - f->offset) * f->h * f->w;
	for (py = y; (py < (y + f->h)); py++) {
		for (px = x; (px < (x + f->w)); px++) {
			if (f->data[fline + cy * f->w + cx]) {
				gdImageSetPixel (im, px, py, color);
			}
			cx++;
		}
		cx = 0;
		cy++;
	}
}

/**
 * Function: gdImageCharUp
 */
BGD_DECLARE(void) gdImageCharUp (gdImagePtr im, gdFontPtr f, int x, int y, int c, int color)
{
	int cx, cy;
	int px, py;
	int fline;
	cx = 0;
	cy = 0;
#ifdef CHARSET_EBCDIC
	c = ASC (c);
#endif /*CHARSET_EBCDIC */
	if ((c < f->offset) || (c >= (f->offset + f->nchars))) {
		return;
	}
	fline = (c - f->offset) * f->h * f->w;
	for (py = y; (py > (y - f->w)); py--) {
		for (px = x; (px < (x + f->h)); px++) {
			if (f->data[fline + cy * f->w + cx]) {
				gdImageSetPixel (im, px, py, color);
			}
			cy++;
		}
		cy = 0;
		cx++;
	}
}

/**
 * Function: gdImageString
 * 
 * Draws a character string.
 *
 * Parameters:
 *  im    - The image to draw onto.
 *  f     - The raster font.
 *  x     - The x coordinate of the upper left pixel.
 *  y     - The y coordinate of the upper left pixel.
 *  c     - The character string.
 *  color - The color.
 *
 * Variants:
 *  - <gdImageStringUp>
 *  - <gdImageString16>
 *  - <gdImageStringUp16>
 *
 * See also:
 *  - <gdFontPtr>
 *  - <gdImageStringTTF>
 */
BGD_DECLARE(void) gdImageString (gdImagePtr im, gdFontPtr f,
								 int x, int y, unsigned char *s, int color)
{
	int i;
	int l;
	l = (int)strlen((char *) s);
	for (i = 0; (i < l); i++) {
		gdImageChar (im, f, x, y, s[i], color);
		x += f->w;
	}
}

/**
 * Function: gdImageStringUp
 */
BGD_DECLARE(void) gdImageStringUp (gdImagePtr im, gdFontPtr f,
								   int x, int y, unsigned char *s, int color)
{
	int i;
	int l;
	l = (int)strlen((char *) s);
	for (i = 0; (i < l); i++) {
		gdImageCharUp (im, f, x, y, s[i], color);
		y -= f->w;
	}
}

static int strlen16 (unsigned short *s);

/**
 * Function: gdImageString16
 */
BGD_DECLARE(void) gdImageString16 (gdImagePtr im, gdFontPtr f,
								   int x, int y, unsigned short *s, int color)
{
	int i;
	int l;
	l = strlen16 (s);
	for (i = 0; (i < l); i++) {
		gdImageChar (im, f, x, y, s[i], color);
		x += f->w;
	}
}

/**
 * Function: gdImageStringUp16
 */
BGD_DECLARE(void) gdImageStringUp16 (gdImagePtr im, gdFontPtr f,
									 int x, int y, unsigned short *s, int color)
{
	int i;
	int l;
	l = strlen16 (s);
	for (i = 0; (i < l); i++) {
		gdImageCharUp (im, f, x, y, s[i], color);
		y -= f->w;
	}
}

static int
strlen16 (unsigned short *s)
{
	int len = 0;
	while (*s) {
		s++;
		len++;
	}
	return len;
}

#ifndef HAVE_LSQRT
/* If you don't have a nice square root function for longs, you can use
   ** this hack
 */
long
lsqrt (long n)
{
	long result = (long) sqrt ((double) n);
	return result;
}
#endif

/* s and e are integers modulo 360 (degrees), with 0 degrees
   being the rightmost extreme and degrees changing clockwise.
   cx and cy are the center in pixels; w and h are the horizontal
   and vertical diameter in pixels. */

/*
	Function: gdImageArc
*/
BGD_DECLARE(void) gdImageArc (gdImagePtr im, int cx, int cy, int w, int h, int s, int e,
							  int color)
{
	gdImageFilledArc (im, cx, cy, w, h, s, e, color, gdNoFill);
}

/*
	Function: gdImageFilledArc
*/
BGD_DECLARE(void) gdImageFilledArc (gdImagePtr im, int cx, int cy, int w, int h, int s, int e,
									int color, int style)
{
	gdPoint pts[363];
	int i, pti;
	int lx = 0, ly = 0;
	int fx = 0, fy = 0;

	if ((s % 360)  == (e % 360)) {
		s = 0;
		e = 360;
	} else {
		if (s > 360) {
			s = s % 360;
		}

		if (e > 360) {
			e = e % 360;
		}

		while (s < 0) {
			s += 360;
		}

		while (e < s) {
			e += 360;
		}

		if (s == e) {
			s = 0;
			e = 360;
		}
	}

	for (i = s, pti = 1; (i <= e); i++, pti++) {
		int x, y;
		x = ((long) gdCosT[i % 360] * (long) w / (2 * 1024)) + cx;
		y = ((long) gdSinT[i % 360] * (long) h / (2 * 1024)) + cy;
		if (i != s) {
			if (!(style & gdChord)) {
				if (style & gdNoFill) {
					gdImageLine (im, lx, ly, x, y, color);
				} else {
					if (y == ly) {
						pti--; /* don't add this point */
						if (((i > 270 || i < 90) && x > lx) || ((i >  90 && i < 270) && x < lx)) {
							/* replace the old x coord, if increasing on the
							   right side or decreasing on the left side */
							pts[pti].x = x;
						}
					} else {
						pts[pti].x = x;
						pts[pti].y = y;
					}
				}
			}
		} else {
			fx = x;
			fy = y;

			if (!(style & (gdChord | gdNoFill))) {
				pts[0].x = cx;
				pts[0].y = cy;
				pts[pti].x = x;
				pts[pti].y = y;
			}
		}
		lx = x;
		ly = y;
	}
	if (style & gdChord) {
		if (style & gdNoFill) {
			if (style & gdEdged) {
				gdImageLine (im, cx, cy, lx, ly, color);
				gdImageLine (im, cx, cy, fx, fy, color);
			}
			gdImageLine (im, fx, fy, lx, ly, color);
		} else {
			pts[0].x = fx;
			pts[0].y = fy;
			pts[1].x = lx;
			pts[1].y = ly;
			pts[2].x = cx;
			pts[2].y = cy;
			gdImageFilledPolygon (im, pts, 3, color);
		}
	} else {
		if (style & gdNoFill) {
			if (style & gdEdged) {
				gdImageLine (im, cx, cy, lx, ly, color);
				gdImageLine (im, cx, cy, fx, fy, color);
			}
		} else {
			pts[pti].x = cx;
			pts[pti].y = cy;
			gdImageFilledPolygon(im, pts, pti+1, color);
		}
	}
}

/*
	Function: gdImageEllipse
*/
BGD_DECLARE(void) gdImageEllipse(gdImagePtr im, int mx, int my, int w, int h, int c)
{
	int x=0,mx1=0,mx2=0,my1=0,my2=0;
	long aq,bq,dx,dy,r,rx,ry,a,b;

	a=w>>1;
	b=h>>1;
	gdImageSetPixel(im,mx+a, my, c);
	gdImageSetPixel(im,mx-a, my, c);
	mx1 = mx-a;
	my1 = my;
	mx2 = mx+a;
	my2 = my;

	aq = a * a;
	bq = b * b;
	dx = aq << 1;
	dy = bq << 1;
	r  = a * bq;
	rx = r << 1;
	ry = 0;
	x = a;
	while (x > 0) {
		if (r > 0) {
			my1++;
			my2--;
			ry +=dx;
			r  -=ry;
		}
		if (r <= 0) {
			x--;
			mx1++;
			mx2--;
			rx -=dy;
			r  +=rx;
		}
		gdImageSetPixel(im,mx1, my1, c);
		gdImageSetPixel(im,mx1, my2, c);
		gdImageSetPixel(im,mx2, my1, c);
		gdImageSetPixel(im,mx2, my2, c);
	}
}


/*
	Function: gdImageFilledEllipse
*/
BGD_DECLARE(void) gdImageFilledEllipse (gdImagePtr im, int mx, int my, int w, int h, int c)
{
	int x=0,mx1=0,mx2=0,my1=0,my2=0;
	long aq,bq,dx,dy,r,rx,ry,a,b;
	int i;
	int old_y2;

	a=w>>1;
	b=h>>1;

	for (x = mx-a; x <= mx+a; x++) {
		gdImageSetPixel(im, x, my, c);
	}

	mx1 = mx-a;
	my1 = my;
	mx2 = mx+a;
	my2 = my;

	aq = a * a;
	bq = b * b;
	dx = aq << 1;
	dy = bq << 1;
	r  = a * bq;
	rx = r << 1;
	ry = 0;
	x = a;
	old_y2=-2;
	while (x > 0) {
		if (r > 0) {
			my1++;
			my2--;
			ry +=dx;
			r  -=ry;
		}
		if (r <= 0) {
			x--;
			mx1++;
			mx2--;
			rx -=dy;
			r  +=rx;
		}

		if(old_y2!=my2) {
			for(i=mx1; i<=mx2; i++) {
				gdImageSetPixel(im,i,my2,c);
				gdImageSetPixel(im,i,my1,c);
			}
		}
		old_y2 = my2;
	}
}

/*
	Function: gdImageFillToBorder
*/
BGD_DECLARE(void) gdImageFillToBorder (gdImagePtr im, int x, int y, int border, int color)
{
	int lastBorder;
	/* Seek left */
	int leftLimit, rightLimit;
	int i;
	int restoreAlphaBlending;

	if (border < 0 || color < 0) {
		/* Refuse to fill to a non-solid border */
		return;
	}

	if (!im->trueColor) {
		if ((color > (im->colorsTotal - 1)) || (border > (im->colorsTotal - 1)) || (color < 0)) {
			return;
		}
	}

	leftLimit = (-1);

	restoreAlphaBlending = im->alphaBlendingFlag;
	im->alphaBlendingFlag = 0;

	if (x >= im->sx) {
		x = im->sx - 1;
	} else if (x < 0) {
		x = 0;
	}
	if (y >= im->sy) {
		y = im->sy - 1;
	} else if (y < 0) {
		y = 0;
	}
	
	for (i = x; (i >= 0); i--) {
		if (gdImageGetPixel (im, i, y) == border) {
			break;
		}
		gdImageSetPixel (im, i, y, color);
		leftLimit = i;
	}
	if (leftLimit == (-1)) {
		im->alphaBlendingFlag = restoreAlphaBlending;
		return;
	}
	/* Seek right */
	rightLimit = x;
	for (i = (x + 1); (i < im->sx); i++) {
		if (gdImageGetPixel (im, i, y) == border) {
			break;
		}
		gdImageSetPixel (im, i, y, color);
		rightLimit = i;
	}
	/* Look at lines above and below and start paints */
	/* Above */
	if (y > 0) {
		lastBorder = 1;
		for (i = leftLimit; (i <= rightLimit); i++) {
			int c;
			c = gdImageGetPixel (im, i, y - 1);
			if (lastBorder) {
				if ((c != border) && (c != color)) {
					gdImageFillToBorder (im, i, y - 1, border, color);
					lastBorder = 0;
				}
			} else if ((c == border) || (c == color)) {
				lastBorder = 1;
			}
		}
	}
	/* Below */
	if (y < ((im->sy) - 1)) {
		lastBorder = 1;
		for (i = leftLimit; (i <= rightLimit); i++) {
			int c = gdImageGetPixel (im, i, y + 1);
			if (lastBorder) {
				if ((c != border) && (c != color)) {
					gdImageFillToBorder (im, i, y + 1, border, color);
					lastBorder = 0;
				}
			} else if ((c == border) || (c == color)) {
				lastBorder = 1;
			}
		}
	}
	im->alphaBlendingFlag = restoreAlphaBlending;
}

/*
 * set the pixel at (x,y) and its 4-connected neighbors
 * with the same pixel value to the new pixel value nc (new color).
 * A 4-connected neighbor:  pixel above, below, left, or right of a pixel.
 * ideas from comp.graphics discussions.
 * For tiled fill, the use of a flag buffer is mandatory. As the tile image can
 * contain the same color as the color to fill. To do not bloat normal filling
 * code I added a 2nd private function.
 */

static int gdImageTileGet (gdImagePtr im, int x, int y)
{
	int srcx, srcy;
	int tileColor,p;
	if (!im->tile) {
		return -1;
	}
	srcx = x % gdImageSX(im->tile);
	srcy = y % gdImageSY(im->tile);
	p = gdImageGetPixel(im->tile, srcx, srcy);
	if (p == im->tile->transparent) {
		tileColor = im->transparent;
	} else if (im->trueColor) {
		if (im->tile->trueColor) {
			tileColor = p;
		} else {
			tileColor = gdTrueColorAlpha( gdImageRed(im->tile,p), gdImageGreen(im->tile,p), gdImageBlue (im->tile,p), gdImageAlpha (im->tile,p));
		}
	} else {
		if (im->tile->trueColor) {
			tileColor = gdImageColorResolveAlpha(im, gdTrueColorGetRed (p), gdTrueColorGetGreen (p), gdTrueColorGetBlue (p), gdTrueColorGetAlpha (p));
		} else {
			tileColor = gdImageColorResolveAlpha(im, gdImageRed (im->tile,p), gdImageGreen (im->tile,p), gdImageBlue (im->tile,p), gdImageAlpha (im->tile,p));
		}
	}
	return tileColor;
}



/* horizontal segment of scan line y */
struct seg {
	int y, xl, xr, dy;
};

/* max depth of stack */
#define FILL_MAX ((int)(im->sy*im->sx)/4)
#define FILL_PUSH(Y, XL, XR, DY) \
	if (sp<stack+FILL_MAX && Y+(DY)>=0 && Y+(DY)<wy2) \
	{sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}

#define FILL_POP(Y, XL, XR, DY) \
	{sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;}

static void _gdImageFillTiled(gdImagePtr im, int x, int y, int nc);

/*
	Function: gdImageFill
*/
BGD_DECLARE(void) gdImageFill(gdImagePtr im, int x, int y, int nc)
{
	int l, x1, x2, dy;
	int oc;   /* old pixel value */
	int wx2,wy2;

	int alphablending_bak;

	/* stack of filled segments */
	/* struct seg stack[FILL_MAX],*sp = stack; */
	struct seg *stack;
	struct seg *sp;

	if (!im->trueColor && nc > (im->colorsTotal - 1)) {
		return;
	}

	alphablending_bak = im->alphaBlendingFlag;
	im->alphaBlendingFlag = 0;

	if (nc==gdTiled) {
		_gdImageFillTiled(im,x,y,nc);
		im->alphaBlendingFlag = alphablending_bak;
		return;
	}

	wx2=im->sx;
	wy2=im->sy;
	oc = gdImageGetPixel(im, x, y);
	if (oc==nc || x<0 || x>wx2 || y<0 || y>wy2) {
		im->alphaBlendingFlag = alphablending_bak;
		return;
	}

	/* Do not use the 4 neighbors implementation with
	* small images
	*/
	if (im->sx < 4) {
		int ix = x, iy = y, c;
		do {
			do {
				c = gdImageGetPixel(im, ix, iy);
				if (c != oc) {
					goto done;
				}
				gdImageSetPixel(im, ix, iy, nc);
			} while(ix++ < (im->sx -1));
			ix = x;
		} while(iy++ < (im->sy -1));
		goto done;
	}

	if(overflow2(im->sy, im->sx)) {
		return;
	}

	if(overflow2(sizeof(struct seg), ((im->sy * im->sx) / 4))) {
		return;
	}

	stack = (struct seg *)gdMalloc(sizeof(struct seg) * ((int)(im->sy*im->sx)/4));
	if (!stack) {
		return;
	}
	sp = stack;

	/* required! */
	FILL_PUSH(y,x,x,1);
	/* seed segment (popped 1st) */
	FILL_PUSH(y+1, x, x, -1);
	while (sp>stack) {
		FILL_POP(y, x1, x2, dy);

		for (x=x1; x>=0 && gdImageGetPixel(im,x, y)==oc; x--) {
			gdImageSetPixel(im,x, y, nc);
		}
		if (x>=x1) {
			goto skip;
		}
		l = x+1;

		/* leak on left? */
		if (l<x1) {
			FILL_PUSH(y, l, x1-1, -dy);
		}
		x = x1+1;
		do {
			for (; x<=wx2 && gdImageGetPixel(im,x, y)==oc; x++) {
				gdImageSetPixel(im, x, y, nc);
			}
			FILL_PUSH(y, l, x-1, dy);
			/* leak on right? */
			if (x>x2+1) {
				FILL_PUSH(y, x2+1, x-1, -dy);
			}
skip:
			for (x++; x<=x2 && (gdImageGetPixel(im, x, y)!=oc); x++);

			l = x;
		} while (x<=x2);
	}

	gdFree(stack);

done:
	im->alphaBlendingFlag = alphablending_bak;
}

static void _gdImageFillTiled(gdImagePtr im, int x, int y, int nc)
{
	int l, x1, x2, dy;
	int oc;   /* old pixel value */
	int wx2,wy2;
	/* stack of filled segments */
	struct seg *stack;
	struct seg *sp;
	char *pts;

	if (!im->tile) {
		return;
	}

	wx2=im->sx;
	wy2=im->sy;

	if(overflow2(im->sy, im->sx)) {
		return;
	}

	if(overflow2(sizeof(struct seg), ((im->sy * im->sx) / 4))) {
		return;
	}

	pts = (char *) gdCalloc(im->sy * im->sx, sizeof(char));
	if (!pts) {
		return;
	}

	stack = (struct seg *)gdMalloc(sizeof(struct seg) * ((int)(im->sy*im->sx)/4));
	if (!stack) {
		gdFree(pts);
		return;
	}
	sp = stack;

	oc = gdImageGetPixel(im, x, y);

	/* required! */
	FILL_PUSH(y,x,x,1);
	/* seed segment (popped 1st) */
	FILL_PUSH(y+1, x, x, -1);
	while (sp>stack) {
		FILL_POP(y, x1, x2, dy);
		for (x=x1; x>=0 && (!pts[y + x*wy2] && gdImageGetPixel(im,x,y)==oc); x--) {
			nc = gdImageTileGet(im,x,y);
			pts[y + x*wy2]=1;
			gdImageSetPixel(im,x, y, nc);
		}
		if (x>=x1) {
			goto skip;
		}
		l = x+1;

		/* leak on left? */
		if (l<x1) {
			FILL_PUSH(y, l, x1-1, -dy);
		}
		x = x1+1;
		do {
			for (; x<wx2 && (!pts[y + x*wy2] && gdImageGetPixel(im,x, y)==oc) ; x++) {
				if (pts[y + x*wy2]) {
					/* we should never be here */
					break;
				}
				nc = gdImageTileGet(im,x,y);
				pts[y + x*wy2]=1;
				gdImageSetPixel(im, x, y, nc);
			}
			FILL_PUSH(y, l, x-1, dy);
			/* leak on right? */
			if (x>x2+1) {
				FILL_PUSH(y, x2+1, x-1, -dy);
			}
skip:
			for (x++; x<=x2 && (pts[y + x*wy2] || gdImageGetPixel(im,x, y)!=oc); x++);
			l = x;
		} while (x<=x2);
	}

	gdFree(pts);
	gdFree(stack);
}

/**
 * Function: gdImageRectangle
 *
 * Draws a rectangle.
 *
 * Parameters:
 *   im    - The image.
 *   x1    - The x-coordinate of the upper left corner.
 *   y1    - The y-coordinate of the upper left corner.
 *   x2    - The x-coordinate of the lower right corner.
 *   y2    - The y-coordinate of the lower right corner.
 *   color - The color.
 *
 * Note that x1,y1 and x2,y2 may be swapped, i.e. the former may designate the
 * lower right corner and the latter the upper left corner. The behavior for
 * specifying other corners is undefined.
 *
 * See also:
 *   - <gdImageFilledRectangle>
 */
BGD_DECLARE(void) gdImageRectangle (gdImagePtr im, int x1, int y1, int x2, int y2, int color)
{
	int thick = im->thick;

	if (x1 == x2 && y1 == y2 && thick == 1) {
		gdImageSetPixel(im, x1, y1, color);
		return;
	}

	if (y2 < y1) {
		int t;
		t = y1;
		y1 = y2;
		y2 = t;

		t = x1;
		x1 = x2;
		x2 = t;
	}

	if (thick > 1) {
		int cx, cy, x1ul, y1ul, x2lr, y2lr;
		int half = thick >> 1;
		x1ul = x1 - half;
		y1ul = y1 - half;

		x2lr = x2 + half;
		y2lr = y2 + half;

		cy = y1ul + thick;
		while (cy-- > y1ul) {
			cx = x1ul - 1;
			while (cx++ < x2lr) {
				gdImageSetPixel(im, cx, cy, color);
			}
		}

		cy = y2lr - thick;
		while (cy++ < y2lr) {
			cx = x1ul - 1;
			while (cx++ < x2lr) {
				gdImageSetPixel(im, cx, cy, color);
			}
		}

		cy = y1ul + thick - 1;
		while (cy++ < y2lr -thick) {
			cx = x1ul - 1;
			while (cx++ < x1ul + thick) {
				gdImageSetPixel(im, cx, cy, color);
			}
		}

		cy = y1ul + thick - 1;
		while (cy++ < y2lr -thick) {
			cx = x2lr - thick - 1;
			while (cx++ < x2lr) {
				gdImageSetPixel(im, cx, cy, color);
			}
		}

		return;
	} else {
		if (x1 == x2 || y1 == y2) {
			gdImageLine(im, x1, y1, x2, y2, color);
		} else {
			gdImageLine(im, x1, y1, x2, y1, color);
			gdImageLine(im, x1, y2, x2, y2, color);
			gdImageLine(im, x1, y1 + 1, x1, y2 - 1, color);
			gdImageLine(im, x2, y1 + 1, x2, y2 - 1, color);
		}
	}
}

static void _gdImageFilledHRectangle (gdImagePtr im, int x1, int y1, int x2, int y2,
		int color)
{
	int x, y;

	if (x1 == x2 && y1 == y2) {
		gdImageSetPixel(im, x1, y1, color);
		return;
	}

	if (x1 > x2) {
		x = x1;
		x1 = x2;
		x2 = x;
	}

	if (y1 > y2) {
		y = y1;
		y1 = y2;
		y2 = y;
	}

	if (x1 < 0) {
		x1 = 0;
	}

	if (x2 >= gdImageSX(im)) {
		x2 = gdImageSX(im) - 1;
	}

	if (y1 < 0) {
		y1 = 0;
	}

	if (y2 >= gdImageSY(im)) {
		y2 = gdImageSY(im) - 1;
	}

	for (x = x1; (x <= x2); x++) {
		for (y = y1; (y <= y2); y++) {
			gdImageSetPixel (im, x, y, color);
		}
	}
}

static void _gdImageFilledVRectangle (gdImagePtr im, int x1, int y1, int x2, int y2,
		int color)
{
	int x, y;

	if (x1 == x2 && y1 == y2) {
		gdImageSetPixel(im, x1, y1, color);
		return;
	}

	if (x1 > x2) {
		x = x1;
		x1 = x2;
		x2 = x;
	}

	if (y1 > y2) {
		y = y1;
		y1 = y2;
		y2 = y;
	}

	if (x1 < 0) {
		x1 = 0;
	}

	if (x2 >= gdImageSX(im)) {
		x2 = gdImageSX(im) - 1;
	}

	if (y1 < 0) {
		y1 = 0;
	}

	if (y2 >= gdImageSY(im)) {
		y2 = gdImageSY(im) - 1;
	}

	for (y = y1; (y <= y2); y++) {
		for (x = x1; (x <= x2); x++) {
			gdImageSetPixel (im, x, y, color);
		}
	}
}

/*
	Function: gdImageFilledRectangle
*/
BGD_DECLARE(void) gdImageFilledRectangle (gdImagePtr im, int x1, int y1, int x2, int y2,
		int color)
{
	_gdImageFilledVRectangle(im, x1, y1, x2, y2, color);
}

/**
 * Group: Cloning and Copying
 */

/**
 * Function: gdImageClone
 *
 * Clones an image
 *
 * Creates an exact duplicate of the given image.
 *
 * Parameters:
 *   src - The source image.
 *
 * Returns:
 *   The cloned image on success, NULL on failure.
 */
BGD_DECLARE(gdImagePtr) gdImageClone (gdImagePtr src) {
	gdImagePtr dst;
	register int i, x;

	if (src->trueColor) {
		dst = gdImageCreateTrueColor(src->sx , src->sy);
	} else {
		dst = gdImageCreate(src->sx , src->sy);
	}

	if (dst == NULL) {
		return NULL;
	}

	if (src->trueColor == 0) {
		dst->colorsTotal = src->colorsTotal;
		for (i = 0; i < gdMaxColors; i++) {
			dst->red[i]   = src->red[i];
			dst->green[i] = src->green[i];
			dst->blue[i]  = src->blue[i];
			dst->alpha[i] = src->alpha[i];
			dst->open[i]  = src->open[i];
		}
		for (i = 0; i < src->sy; i++) {
			for (x = 0; x < src->sx; x++) {
				dst->pixels[i][x] = src->pixels[i][x];
			}
		}
	} else {
		for (i = 0; i < src->sy; i++) {
			for (x = 0; x < src->sx; x++) {
				dst->tpixels[i][x] = src->tpixels[i][x];
			}
		}
	}

	if (src->styleLength > 0) {
		dst->styleLength = src->styleLength;
		dst->stylePos    = src->stylePos;
		for (i = 0; i < src->styleLength; i++) {
			dst->style[i] = src->style[i];
		}
	}

	dst->interlace   = src->interlace;

	dst->alphaBlendingFlag = src->alphaBlendingFlag;
	dst->saveAlphaFlag     = src->saveAlphaFlag;
	dst->AA                = src->AA;
	dst->AA_color          = src->AA_color;
	dst->AA_dont_blend     = src->AA_dont_blend;

	dst->cx1 = src->cx1;
	dst->cy1 = src->cy1;
	dst->cx2 = src->cx2;
	dst->cy2 = src->cy2;

	dst->res_x = src->res_x;
	dst->res_y = src->res_y;

	dst->paletteQuantizationMethod     = src->paletteQuantizationMethod;
	dst->paletteQuantizationSpeed      = src->paletteQuantizationSpeed;
	dst->paletteQuantizationMinQuality = src->paletteQuantizationMinQuality;
	dst->paletteQuantizationMaxQuality = src->paletteQuantizationMaxQuality;

	dst->interpolation_id = src->interpolation_id;
	dst->interpolation    = src->interpolation;

	if (src->brush) {
		dst->brush = gdImageClone(src->brush);
	}

	if (src->tile) {
		dst->tile = gdImageClone(src->tile);
	}

	if (src->style) {
		gdImageSetStyle(dst, src->style, src->styleLength);
	}

	for (i = 0; i < gdMaxColors; i++) {
		dst->brushColorMap[i] = src->brushColorMap[i];
		dst->tileColorMap[i] = src->tileColorMap[i];
	}

	if (src->polyAllocated > 0) {
		dst->polyAllocated = src->polyAllocated;
		for (i = 0; i < src->polyAllocated; i++) {
			dst->polyInts[i] = src->polyInts[i];
		}
	}

	return dst;
}

/**
 * Function: gdImageCopy
 *
 * Copy an area of an image to another image
 *
 * Parameters:
 *   dst  - The destination image.
 *   src  - The source image.
 *   dstX - The x-coordinate of the upper left corner to copy to.
 *   dstY - The y-coordinate of the upper left corner to copy to.
 *   srcX - The x-coordinate of the upper left corner to copy from.
 *   srcY - The y-coordinate of the upper left corner to copy from.
 *   w    - The width of the area to copy.
 *   h    - The height of the area to copy.
 *
 * See also:
 *   - <gdImageCopyMerge>
 *   - <gdImageCopyMergeGray>
 */
BGD_DECLARE(void) gdImageCopy (gdImagePtr dst, gdImagePtr src, int dstX, int dstY, int srcX,
							   int srcY, int w, int h)
{
	int colorMap[gdMaxColors];

	if (dst->trueColor) {
		/* 2.0: much easier when the destination is truecolor. */
		/* 2.0.10: needs a transparent-index check that is still valid if
		 *          * the source is not truecolor. Thanks to Frank Warmerdam.
		 */

		if (src->trueColor) {
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					int c = gdImageGetTrueColorPixel (src, srcX + x, srcY + y);
					if (c != src->transparent) {
						gdImageSetPixel (dst, dstX + x, dstY + y, c);
					}
				}
			}
		} else {
			/* source is palette based */
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					int c = gdImageGetPixel (src, srcX + x, srcY + y);
					if (c != src->transparent) {
						gdImageSetPixel(dst, dstX + x, dstY + y, gdTrueColorAlpha(src->red[c], src->green[c], src->blue[c], src->alpha[c]));
					}
				}
			}
		}
		return;
	}

	for (int i = 0; i < gdMaxColors; i++) {
		colorMap[i] = (-1);
	}
	int toy = dstY;
	for (int y = srcY; y < (srcY + h); y++) {
		int tox = dstX;
		for (int x = srcX; x < (srcX + w); x++) {
			int mapTo;
			int c = gdImageGetPixel (src, x, y);
			/* Added 7/24/95: support transparent copies */
			if (gdImageGetTransparent (src) == c) {
				tox++;
				continue;
			}
			/* Have we established a mapping for this color? */
			if (src->trueColor) {
				/* 2.05: remap to the palette available in the
				 destination image. This is slow and
				 works badly, but it beats crashing! Thanks
				 to Padhrig McCarthy. */
				mapTo = gdImageColorResolveAlpha (dst,
				                                  gdTrueColorGetRed (c),
				                                  gdTrueColorGetGreen (c),
				                                  gdTrueColorGetBlue (c),
				                                  gdTrueColorGetAlpha (c));
			} else if (colorMap[c] == (-1)) {
				int nc;
				/* If it's the same image, mapping is trivial */
				if (dst == src) {
					nc = c;
				} else {
					/* Get best match possible. This
					   function never returns error. */
					nc = gdImageColorResolveAlpha (dst,
					                               src->red[c], src->green[c],
					                               src->blue[c], src->alpha[c]);
				}
				colorMap[c] = nc;
				mapTo = colorMap[c];
			} else {
				mapTo = colorMap[c];
			}
			gdImageSetPixel (dst, tox, toy, mapTo);
			tox++;
		}
		toy++;
	}
}

/**
 * Function: gdImageCopyMerge
 *
 * Copy an area of an image to another image ignoring alpha
 *
 * The source area will be copied to the destination are by merging the pixels.
 *
 * Note:
 *   This function is a substitute for real alpha channel operations,
 *   so it doesn't pay attention to the alpha channel.
 *
 * Parameters:
 *   dst  - The destination image.
 *   src  - The source image.
 *   dstX - The x-coordinate of the upper left corner to copy to.
 *   dstY - The y-coordinate of the upper left corner to copy to.
 *   srcX - The x-coordinate of the upper left corner to copy from.
 *   srcY - The y-coordinate of the upper left corner to copy from.
 *   w    - The width of the area to copy.
 *   h    - The height of the area to copy.
 *   pct  - The percentage in range 0..100.
 *
 * See also:
 *   - <gdImageCopy>
 *   - <gdImageCopyMergeGray>
 */
BGD_DECLARE(void) gdImageCopyMerge (gdImagePtr dst, gdImagePtr src, int dstX, int dstY,
									int srcX, int srcY, int w, int h, int pct)
{

	int c, dc;
	int x, y;
	int tox, toy;
	int ncR, ncG, ncB;
	toy = dstY;
	for (y = srcY; (y < (srcY + h)); y++) {
		tox = dstX;
		for (x = srcX; (x < (srcX + w)); x++) {
			int nc;
			c = gdImageGetPixel (src, x, y);
			/* Added 7/24/95: support transparent copies */
			if (gdImageGetTransparent (src) == c) {
				tox++;
				continue;
			}
			/* If it's the same image, mapping is trivial */
			if (dst == src) {
				nc = c;
			} else {
				dc = gdImageGetPixel (dst, tox, toy);

				ncR = (int)(gdImageRed (src, c) * (pct / 100.0)
				      + gdImageRed (dst, dc) * ((100 - pct) / 100.0));
				ncG = (int)(gdImageGreen (src, c) * (pct / 100.0)
				      + gdImageGreen (dst, dc) * ((100 - pct) / 100.0));
				ncB = (int)(gdImageBlue (src, c) * (pct / 100.0)
				      + gdImageBlue (dst, dc) * ((100 - pct) / 100.0));

				/* Find a reasonable color */
				nc = gdImageColorResolve (dst, ncR, ncG, ncB);
			}
			gdImageSetPixel (dst, tox, toy, nc);
			tox++;
		}
		toy++;
	}
}

/**
 * Function: gdImageCopyMergeGray
 *
 * Copy an area of an image to another image ignoring alpha
 *
 * The source area will be copied to the grayscaled destination area by merging
 * the pixels.
 *
 * Note:
 *   This function is a substitute for real alpha channel operations,
 *   so it doesn't pay attention to the alpha channel.
 * 
 * Parameters:
 *   dst  - The destination image.
 *   src  - The source image.
 *   dstX - The x-coordinate of the upper left corner to copy to.
 *   dstY - The y-coordinate of the upper left corner to copy to.
 *   srcX - The x-coordinate of the upper left corner to copy from.
 *   srcY - The y-coordinate of the upper left corner to copy from.
 *   w    - The width of the area to copy.
 *   h    - The height of the area to copy.
 *   pct  - The percentage of the source color intensity in range 0..100.
 *
 * See also:
 *   - <gdImageCopy>
 *   - <gdImageCopyMerge>
 */
BGD_DECLARE(void) gdImageCopyMergeGray (gdImagePtr dst, gdImagePtr src, int dstX, int dstY,
										int srcX, int srcY, int w, int h, int pct)
{

	int c, dc;
	int x, y;
	int tox, toy;
	int ncR, ncG, ncB;
	float g;
	toy = dstY;
	for (y = srcY; (y < (srcY + h)); y++) {
		tox = dstX;
		for (x = srcX; (x < (srcX + w)); x++) {
			int nc;
			c = gdImageGetPixel (src, x, y);
			/* Added 7/24/95: support transparent copies */
			if (gdImageGetTransparent (src) == c) {
				tox++;
				continue;
			}
			/*
			 * If it's the same image, mapping is NOT trivial since we
			 * merge with greyscale target, but if pct is 100, the grey
			 * value is not used, so it becomes trivial. pjw 2.0.12.
			 */
			if (dst == src && pct == 100) {
				nc = c;
			} else {
				dc = gdImageGetPixel (dst, tox, toy);
				g = 0.29900f * gdImageRed(dst, dc)
				    + 0.58700f * gdImageGreen(dst, dc) + 0.11400f * gdImageBlue(dst, dc);

				ncR = (int)(gdImageRed (src, c) * (pct / 100.0)
				      + g * ((100 - pct) / 100.0));
				ncG = (int)(gdImageGreen (src, c) * (pct / 100.0)
				      + g * ((100 - pct) / 100.0));
				ncB = (int)(gdImageBlue (src, c) * (pct / 100.0)
				      + g * ((100 - pct) / 100.0));

				/* First look for an exact match */
				nc = gdImageColorExact (dst, ncR, ncG, ncB);
				if (nc == (-1)) {
					/* No, so try to allocate it */
					nc = gdImageColorAllocate (dst, ncR, ncG, ncB);
					/* If we're out of colors, go for the
					   closest color */
					if (nc == (-1)) {
						nc = gdImageColorClosest (dst, ncR, ncG, ncB);
					}
				}
			}
			gdImageSetPixel (dst, tox, toy, nc);
			tox++;
		}
		toy++;
	}
}

/**
 * Function: gdImageCopyResized
 *
 * Copy a resized area from an image to another image
 *
 * If the source and destination area differ in size, the area will be resized
 * using nearest-neighbor interpolation.
 * 
 * Parameters:
 *   dst  - The destination image.
 *   src  - The source image.
 *   dstX - The x-coordinate of the upper left corner to copy to.
 *   dstY - The y-coordinate of the upper left corner to copy to.
 *   srcX - The x-coordinate of the upper left corner to copy from.
 *   srcY - The y-coordinate of the upper left corner to copy from.
 *   dstW - The width of the area to copy to.
 *   dstH - The height of the area to copy to.
 *   srcW - The width of the area to copy from.
 *   srcH - The height of the area to copy from.
 *
 * See also:
 *   - <gdImageCopyResampled>
 *   - <gdImageScale>
 */
BGD_DECLARE(void) gdImageCopyResized (gdImagePtr dst, gdImagePtr src, int dstX, int dstY,
									  int srcX, int srcY, int dstW, int dstH, int srcW,
									  int srcH)
{
	int colorMap[gdMaxColors];
	/* We only need to use floating point to determine the correct
	   stretch vector for one line's worth. */
	if (overflow2(sizeof (int), srcW)) {
		return;
	}
	if (overflow2(sizeof (int), srcH)) {
		return;
	}
	/* Stretch vectors */
	int *stx = (int *) gdMalloc (sizeof (int) * srcW);
	if (!stx) {
		return;
	}

	int *sty = (int *) gdMalloc (sizeof (int) * srcH);
	if (!sty) {
		gdFree(stx);
		return;
	}

	/* Fixed by Mao Morimoto 2.0.16 */
	for (int i = 0; i < srcW; i++) {
		stx[i] = dstW * (i + 1) / srcW - dstW * i / srcW;
	}
	for (int i = 0; i < srcH; i++) {
		sty[i] = dstH * (i + 1) / srcH - dstH * i / srcH;
	}
	for (int i = 0; i < gdMaxColors; i++) {
		colorMap[i] = (-1);
	}
	int toy = dstY;
	for (int y = srcY; y < (srcY + srcH); y++) {
		for (int ydest = 0; ydest < sty[y - srcY]; ydest++) {
			int tox = dstX;
			for (int x = srcX; x < (srcX + srcW); x++) {
				int mapTo;
				if (!stx[x - srcX]) {
					continue;
				}
				if (dst->trueColor) {
					/* 2.0.9: Thorben Kundinger: Maybe the source image is not
					   a truecolor image */
					if (!src->trueColor) {
						int tmp = gdImageGetPixel (src, x, y);
						mapTo = gdImageGetTrueColorPixel (src, x, y);
						if (gdImageGetTransparent (src) == tmp) {
							/* 2.0.21, TK: not tox++ */
							tox += stx[x - srcX];
							continue;
						}
					} else {
						/* TK: old code follows */
						mapTo = gdImageGetTrueColorPixel (src, x, y);
						/* Added 7/24/95: support transparent copies */
						if (gdImageGetTransparent (src) == mapTo) {
							/* 2.0.21, TK: not tox++ */
							tox += stx[x - srcX];
							continue;
						}
					}
				} else {
					int c = gdImageGetPixel (src, x, y);
					/* Added 7/24/95: support transparent copies */
					if (gdImageGetTransparent (src) == c) {
						tox += stx[x - srcX];
						continue;
					}
					if (src->trueColor) {
						/* Remap to the palette available in the
						   destination image. This is slow and
						   works badly. */
						mapTo = gdImageColorResolveAlpha (dst,
						                                  gdTrueColorGetRed (c),
						                                  gdTrueColorGetGreen
						                                  (c),
						                                  gdTrueColorGetBlue
						                                  (c),
						                                  gdTrueColorGetAlpha
						                                  (c));
					} else {
						/* Have we established a mapping for this color? */
						if (colorMap[c] == (-1)) {
							int nc = 0;
							/* If it's the same image, mapping is trivial */
							if (dst == src) {
								nc = c;
							} else {
								/* Find or create the best match */
								/* 2.0.5: can't use gdTrueColorGetRed, etc with palette */
								nc = gdImageColorResolveAlpha (dst,
								                               gdImageRed (src,
								                                       c),
								                               gdImageGreen
								                               (src, c),
								                               gdImageBlue (src,
								                                       c),
								                               gdImageAlpha
								                               (src, c));
							}
							colorMap[c] = nc;
						}
						mapTo = colorMap[c];
					}
				}
				for (int i = 0; i < stx[x - srcX]; i++) {
					gdImageSetPixel (dst, tox, toy, mapTo);
					tox++;
				}
			}
			toy++;
		}
	}
	gdFree (stx);
	gdFree (sty);
}

/**
 * Function: gdImageCopyRotated
 *
 * Copy a rotated area from an image to another image
 *
 * The area is counter-clockwise rotated using nearest-neighbor interpolation.
 *
 * Parameters:
 *   dst   - The destination image.
 *   src   - The source image.
 *   dstX  - The x-coordinate of the center of the area to copy to.
 *   dstY  - The y-coordinate of the center of the area to copy to.
 *   srcX  - The x-coordinate of the upper left corner to copy from.
 *   srcY  - The y-coordinate of the upper left corner to copy from.
 *   srcW  - The width of the area to copy from.
 *   srcH  - The height of the area to copy from.
 *   angle - The angle in degrees.
 *
 * See also:
 *   - <gdImageRotateInterpolated>
 */
BGD_DECLARE(void) gdImageCopyRotated (gdImagePtr dst,
									  gdImagePtr src,
									  double dstX, double dstY,
									  int srcX, int srcY,
									  int srcWidth, int srcHeight, int angle)
{
	double dx, dy;
	double radius = sqrt (srcWidth * srcWidth + srcHeight * srcHeight);
	double aCos = cos (angle * .0174532925);
	double aSin = sin (angle * .0174532925);
	double scX = srcX + ((double) srcWidth) / 2;
	double scY = srcY + ((double) srcHeight) / 2;
	int cmap[gdMaxColors];
	int i;

	/*
		 2.0.34: transparency preservation. The transparentness of
		 the transparent color is more important than its hue.
	*/
	if (src->transparent != -1) {
		if (dst->transparent == -1) {
			dst->transparent = src->transparent;
		}
	}

	for (i = 0; (i < gdMaxColors); i++) {
		cmap[i] = (-1);
	}
	for (dy = dstY - radius; (dy <= dstY + radius); dy++) {
		for (dx = dstX - radius; (dx <= dstX + radius); dx++) {
			double sxd = (dx - dstX) * aCos - (dy - dstY) * aSin;
			double syd = (dy - dstY) * aCos + (dx - dstX) * aSin;
			int sx = (int)(sxd + scX);
			int sy = (int)(syd + scY);
			if ((sx >= srcX) && (sx < srcX + srcWidth) &&
			        (sy >= srcY) && (sy < srcY + srcHeight)) {
				int c = gdImageGetPixel (src, sx, sy);
				/* 2.0.34: transparency wins */
				if (c == src->transparent) {
					gdImageSetPixel (dst, (int)dx, (int)dy, dst->transparent);
				} else if (!src->trueColor) {
					/* Use a table to avoid an expensive
					   lookup on every single pixel */
					if (cmap[c] == -1) {
						cmap[c] = gdImageColorResolveAlpha (dst,
						                                    gdImageRed (src, c),
						                                    gdImageGreen (src,
						                                            c),
						                                    gdImageBlue (src,
						                                            c),
						                                    gdImageAlpha (src,
						                                            c));
					}
					gdImageSetPixel (dst, (int)dx, (int)dy, cmap[c]);
				} else {
					gdImageSetPixel (dst,
					                 (int)dx, (int)dy,
					                 gdImageColorResolveAlpha (dst,
					                         gdImageRed (src,
					                                     c),
					                         gdImageGreen
					                         (src, c),
					                         gdImageBlue (src,
					                                      c),
					                         gdImageAlpha
					                         (src, c)));
				}
			}
		}
	}
}

/* When gd 1.x was first created, floating point was to be avoided.
   These days it is often faster than table lookups or integer
   arithmetic. The routine below is shamelessly, gloriously
   floating point. TBB */

/* 2.0.10: cast instead of floor() yields 35% performance improvement.
	Thanks to John Buckman. */

#define floor2(exp) ((long) exp)
/*#define floor2(exp) floor(exp)*/

/**
 * Function: gdImageCopyResampled
 *
 * Copy a resampled area from an image to another image
 *
 * If the source and destination area differ in size, the area will be resized
 * using bilinear interpolation for truecolor images, and nearest-neighbor
 * interpolation for palette images.
 * 
 * Parameters:
 *   dst  - The destination image.
 *   src  - The source image.
 *   dstX - The x-coordinate of the upper left corner to copy to.
 *   dstY - The y-coordinate of the upper left corner to copy to.
 *   srcX - The x-coordinate of the upper left corner to copy from.
 *   srcY - The y-coordinate of the upper left corner to copy from.
 *   dstW - The width of the area to copy to.
 *   dstH - The height of the area to copy to.
 *   srcW - The width of the area to copy from.
 *   srcH - The height of the area to copy from.
 *
 * See also:
 *   - <gdImageCopyResized>
 *   - <gdImageScale>
 */
BGD_DECLARE(void) gdImageCopyResampled (gdImagePtr dst,
										gdImagePtr src,
										int dstX, int dstY,
										int srcX, int srcY,
										int dstW, int dstH, int srcW, int srcH)
{
	if (!dst->trueColor) {
		gdImageCopyResized (dst, src, dstX, dstY, srcX, srcY, dstW, dstH, srcW, srcH);
		return;
	}
	for (int y = dstY; (y < dstY + dstH); y++) {
		for (int x = dstX; (x < dstX + dstW); x++) {
			float spixels = 0.0f;
			float red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
			float alpha_factor, alpha_sum = 0.0f, contrib_sum = 0.0f;
			float sy1 = ((float)(y - dstY)) * (float)srcH / (float)dstH;
			float sy2 = ((float)(y + 1 - dstY)) * (float) srcH / (float) dstH;
			float sy = sy1;
			do {
				float yportion;
				if (floorf(sy) == floorf(sy1)) {
					yportion = 1.0f - (sy - floorf(sy));
					if (yportion > sy2 - sy1) {
						yportion = sy2 - sy1;
					}
					sy = floorf(sy);
				} else if (sy == floorf(sy2)) {
					yportion = sy2 - floorf(sy2);
				} else {
					yportion = 1.0f;
				}
				float sx1 = ((float)(x - dstX)) * (float) srcW / dstW;
				float sx2 = ((float)(x + 1 - dstX)) * (float) srcW / dstW;
				float sx = sx1;
				do {
					float xportion;
					float pcontribution;
					int p;
					if (floorf(sx) == floorf(sx1)) {
						xportion = 1.0f - (sx - floorf(sx));
						if (xportion > sx2 - sx1) {
							xportion = sx2 - sx1;
						}
						sx = floorf(sx);
					} else if (sx == floorf(sx2)) {
						xportion = sx2 - floorf(sx2);
					} else {
						xportion = 1.0f;
					}
					pcontribution = xportion * yportion;
					p = gdImageGetTrueColorPixel(src, (int) sx + srcX, (int) sy + srcY);

					alpha_factor = ((gdAlphaMax - gdTrueColorGetAlpha(p))) * pcontribution;
					red += gdTrueColorGetRed (p) * alpha_factor;
					green += gdTrueColorGetGreen (p) * alpha_factor;
					blue += gdTrueColorGetBlue (p) * alpha_factor;
					alpha += gdTrueColorGetAlpha (p) * pcontribution;
					alpha_sum += alpha_factor;
					contrib_sum += pcontribution;
					spixels += xportion * yportion;
					sx += 1.0f;
				}
				while (sx < sx2);
				sy += 1.0f;
			}
			while (sy < sy2);

			if (spixels != 0.0f) {
				red /= spixels;
				green /= spixels;
				blue /= spixels;
				alpha /= spixels;
			}
			if ( alpha_sum != 0.0) {
				if( contrib_sum != 0.0) {
					alpha_sum /= contrib_sum;
				}
				red /= alpha_sum;
				green /= alpha_sum;
				blue /= alpha_sum;
			}
			/* Clamping to allow for rounding errors above */
			if (red > 255.0f) {
				red = 255.0f;
			}
			if (green > 255.0f) {
				green = 255.0f;
			}
			if (blue > 255.0f) {
				blue = 255.0f;
			}
			if (alpha > gdAlphaMax) {
				alpha = gdAlphaMax;
			}
			gdImageSetPixel(dst, x, y, gdTrueColorAlpha ((int) red, (int) green, (int) blue, (int) alpha));
		}
	}
}

/**
 * Group: Polygons
 */

/**
 * Function: gdImagePolygon
 *
 * Draws a closed polygon
 *
 * Parameters:
 *   im - The image.
 *   p  - The vertices as array of <gdPoint>s.
 *   n  - The number of vertices.
 *   c  - The color.
 *
 * See also:
 *   - <gdImageOpenPolygon>
 *   - <gdImageFilledPolygon>
 */
BGD_DECLARE(void) gdImagePolygon (gdImagePtr im, gdPointPtr p, int n, int c)
{
	if (n <= 0) {
		return;
	}


	gdImageLine (im, p->x, p->y, p[n - 1].x, p[n - 1].y, c);
	gdImageOpenPolygon (im, p, n, c);
}

/**
 * Function: gdImageOpenPolygon
 *
 * Draws an open polygon
 *
 * Parameters:
 *   im - The image.
 *   p  - The vertices as array of <gdPoint>s.
 *   n  - The number of vertices.
 *   c  - The color
 *
 * See also:
 *   - <gdImagePolygon>
 */
BGD_DECLARE(void) gdImageOpenPolygon (gdImagePtr im, gdPointPtr p, int n, int c)
{
	int i;
	int lx, ly;
	if (n <= 0) {
		return;
	}


	lx = p->x;
	ly = p->y;
	for (i = 1; (i < n); i++) {
		p++;
		gdImageLine (im, lx, ly, p->x, p->y, c);
		lx = p->x;
		ly = p->y;
	}

}

/* THANKS to Kirsten Schulz for the polygon fixes! */

/* The intersection finding technique of this code could be improved  */
/* by remembering the previous intertersection, and by using the slope. */
/* That could help to adjust intersections  to produce a nice */
/* interior_extrema. */

/**
 * Function: gdImageFilledPolygon
 *
 * Draws a filled polygon
 *
 * The polygon is filled using the even-odd fillrule what can leave unfilled
 * regions inside of self-intersecting polygons. This behavior might change in
 * a future version.
 *
 * Parameters:
 *   im - The image.
 *   p  - The vertices as array of <gdPoint>s.
 *   n  - The number of vertices.
 *   c  - The color
 *
 * See also:
 *   - <gdImagePolygon>
 */
BGD_DECLARE(void) gdImageFilledPolygon (gdImagePtr im, gdPointPtr p, int n, int c)
{
	int x1, y1;
	int x2, y2;
	int fill_color;
	if (n <= 0) {
		return;
	}

	if (c == gdAntiAliased) {
		fill_color = im->AA_color;
	} else {
		fill_color = c;
	}
	if (!im->polyAllocated) {
		if (overflow2(sizeof (int), n)) {
			return;
		}
		im->polyInts = (int *) gdMalloc (sizeof (int) * n);
		if (!im->polyInts) {
			return;
		}
		im->polyAllocated = n;
	}
	if (im->polyAllocated < n) {
		while (im->polyAllocated < n) {
			im->polyAllocated *= 2;
		}
		if (overflow2(sizeof (int), im->polyAllocated)) {
			return;
		}
		im->polyInts = (int *) gdReallocEx (im->polyInts,
						    sizeof (int) * im->polyAllocated);
		if (!im->polyInts) {
			return;
		}
	}
	int miny = p[0].y;
	int maxy = p[0].y;
	for (int i = 1; i < n; i++) {
		if (p[i].y < miny) {
			miny = p[i].y;
		}
		if (p[i].y > maxy) {
			maxy = p[i].y;
		}
	}
	/* necessary special case: horizontal line */
	if (n > 1 && miny == maxy) {
		x1 = x2 = p[0].x;
		for (int i = 1; i < n; i++) {
			if (p[i].x < x1) {
				x1 = p[i].x;
			} else if (p[i].x > x2) {
				x2 = p[i].x;
			}
		}
		gdImageLine(im, x1, miny, x2, miny, c);
		return;
	}
	int pmaxy = maxy;
	/* 2.0.16: Optimization by Ilia Chipitsine -- don't waste time offscreen */
	/* 2.0.26: clipping rectangle is even better */
	if (miny < im->cy1) {
		miny = im->cy1;
	}
	if (maxy > im->cy2) {
		maxy = im->cy2;
	}
	/* Fix in 1.3: count a vertex only once */
	for (int y = miny; y <= maxy; y++) {
		int ints = 0;
		for (int i = 0; i < n; i++) {
			int ind1, ind2;
			if (!i) {
				ind1 = n - 1;
				ind2 = 0;
			} else {
				ind1 = i - 1;
				ind2 = i;
			}
			y1 = p[ind1].y;
			y2 = p[ind2].y;
			if (y1 < y2) {
				x1 = p[ind1].x;
				x2 = p[ind2].x;
			} else if (y1 > y2) {
				y2 = p[ind1].y;
				y1 = p[ind2].y;
				x2 = p[ind1].x;
				x1 = p[ind2].x;
			} else {
				continue;
			}

			/* Do the following math as float intermediately, and round to ensure
			 * that Polygon and FilledPolygon for the same set of points have the
			 * same footprint. */

			if ((y >= y1) && (y < y2)) {
				im->polyInts[ints++] = (int) ((float) ((y - y1) * (x2 - x1)) /
				                              (float) (y2 - y1) + 0.5 + x1);
			} else if ((y == pmaxy) && (y == y2)) {
				im->polyInts[ints++] = x2;
			}
		}
		/*
		  2.0.26: polygons pretty much always have less than 100 points,
		  and most of the time they have considerably less. For such trivial
		  cases, insertion sort is a good choice. Also a good choice for
		  future implementations that may wish to indirect through a table.
		*/
		for (int i = 1; i < ints; i++) {
			int index = im->polyInts[i];
			int j = i;
			while ((j > 0) && (im->polyInts[j - 1] > index)) {
				im->polyInts[j] = im->polyInts[j - 1];
				j--;
			}
			im->polyInts[j] = index;
		}
		for (int i = 0; i < (ints-1); i += 2) {
			/* 2.0.29: back to gdImageLine to prevent segfaults when
			  performing a pattern fill */
			gdImageLine (im, im->polyInts[i], y, im->polyInts[i + 1], y,
			             fill_color);
		}
	}
	/* If we are drawing this AA, then redraw the border with AA lines. */
	/* This doesn't work as well as I'd like, but it doesn't clash either. */
	if (c == gdAntiAliased) {
		gdImagePolygon (im, p, n, c);
	}
}

/**
 * Group: other
 */

static void gdImageSetAAPixelColor(gdImagePtr im, int x, int y, int color, int t);

/**
 * Function: gdImageSetStyle
 *
 * Sets the style for following drawing operations
 *
 * Parameters:
 *   im        - The image.
 *   style     - An array of color values.
 *   noOfPixel - The number of color values.
 */
BGD_DECLARE(void) gdImageSetStyle (gdImagePtr im, int *style, int noOfPixels)
{
	if (im->style) {
		gdFree (im->style);
	}
	if (overflow2(sizeof (int), noOfPixels)) {
		return;
	}
	im->style = (int *) gdMalloc (sizeof (int) * noOfPixels);
	if (!im->style) {
		return;
	}
	memcpy (im->style, style, sizeof (int) * noOfPixels);
	im->styleLength = noOfPixels;
	im->stylePos = 0;
}

/**
 * Function: gdImageSetThickness
 *
 * Sets the thickness for following drawing operations
 *
 * Parameters:
 *   im        - The image.
 *   thickness - The thickness in pixels.
 */
BGD_DECLARE(void) gdImageSetThickness (gdImagePtr im, int thickness)
{
	im->thick = thickness;
}

/**
 * Function: gdImageSetBrush
 *
 * Sets the brush for following drawing operations
 *
 * Parameters:
 *   im    - The image.
 *   brush - The brush image.
 */
BGD_DECLARE(void) gdImageSetBrush (gdImagePtr im, gdImagePtr brush)
{
	im->brush = brush;
	if ((!im->trueColor) && (!im->brush->trueColor)) {
		for (int i = 0; (i < gdImageColorsTotal (brush)); i++) {
			int index = gdImageColorResolveAlpha (im,
			                                  gdImageRed (brush, i),
			                                  gdImageGreen (brush, i),
			                                  gdImageBlue (brush, i),
			                                  gdImageAlpha (brush, i));
			im->brushColorMap[i] = index;
		}
	}
}

/*
	Function: gdImageSetTile
*/
BGD_DECLARE(void) gdImageSetTile (gdImagePtr im, gdImagePtr tile)
{
	im->tile = tile;
	if ((!im->trueColor) && (!im->tile->trueColor)) {
		for (int i = 0; (i < gdImageColorsTotal (tile)); i++) {
			int index;
			index = gdImageColorResolveAlpha (im,
			                                  gdImageRed (tile, i),
			                                  gdImageGreen (tile, i),
			                                  gdImageBlue (tile, i),
			                                  gdImageAlpha (tile, i));
			im->tileColorMap[i] = index;
		}
	}
}

/**
 * Function: gdImageSetAntiAliased
 *
 * Set the color for subsequent anti-aliased drawing
 *
 * If <gdAntiAliased> is passed as color to drawing operations that support
 * anti-aliased drawing (such as <gdImageLine> and <gdImagePolygon>), the actual
 * color to be used can be set with this function.
 *
 * Example: draw an anti-aliased blue line:
 * | gdImageSetAntiAliased(im, gdTrueColorAlpha(0, 0, gdBlueMax, gdAlphaOpaque));
 * | gdImageLine(im, 10,10, 20,20, gdAntiAliased);
 *
 * Parameters:
 *   im - The image.
 *   c  - The color.
 *
 * See also:
 *   - <gdImageSetAntiAliasedDontBlend>
 */
BGD_DECLARE(void) gdImageSetAntiAliased (gdImagePtr im, int c)
{
	im->AA = 1;
	im->AA_color = c;
	im->AA_dont_blend = -1;
}

/**
 * Function: gdImageSetAntiAliasedDontBlend
 *
 * Set the color and "dont_blend" color for subsequent anti-aliased drawing
 *
 * This extended variant of <gdImageSetAntiAliased> allows to also specify a
 * (background) color that will not be blended in anti-aliased drawing
 * operations.
 *
 * Parameters:
 *   im         - The image.
 *   c          - The color.
 *   dont_blend - Whether to blend.
 */
BGD_DECLARE(void) gdImageSetAntiAliasedDontBlend (gdImagePtr im, int c, int dont_blend)
{
	im->AA = 1;
	im->AA_color = c;
	im->AA_dont_blend = dont_blend;
}

/**
 * Function: gdImageInterlace
 *
 * Sets whether an image is interlaced
 *
 * This is relevant only when saving the image in a format that supports
 * interlacing.
 *
 * Parameters:
 *   im           - The image.
 *   interlaceArg - Whether the image is interlaced.
 *
 * See also:
 *   - <gdImageGetInterlaced>
*/
BGD_DECLARE(void) gdImageInterlace (gdImagePtr im, int interlaceArg)
{
	im->interlace = interlaceArg;
}

/**
 * Function: gdImageCompare
 *
 * Compare two images
 *
 * Parameters:
 *   im1 - An image.
 *   im2 - Another image.
 *
 * Returns:
 *   A bitmask of <Image Comparison> flags where each set flag signals
 *   which attributes of the images are different.
 */
BGD_DECLARE(int) gdImageCompare (gdImagePtr im1, gdImagePtr im2)
{
	int x, y;
	int p1, p2;
	int cmpStatus = 0;
	int sx, sy;

	if (im1->interlace != im2->interlace) {
		cmpStatus |= GD_CMP_INTERLACE;
	}

	if (im1->transparent != im2->transparent) {
		cmpStatus |= GD_CMP_TRANSPARENT;
	}

	if (im1->trueColor != im2->trueColor) {
		cmpStatus |= GD_CMP_TRUECOLOR;
	}

	sx = im1->sx;
	if (im1->sx != im2->sx) {
		cmpStatus |= GD_CMP_SIZE_X + GD_CMP_IMAGE;
		if (im2->sx < im1->sx) {
			sx = im2->sx;
		}
	}

	sy = im1->sy;
	if (im1->sy != im2->sy) {
		cmpStatus |= GD_CMP_SIZE_Y + GD_CMP_IMAGE;
		if (im2->sy < im1->sy) {
			sy = im2->sy;
		}
	}

	if (im1->colorsTotal != im2->colorsTotal) {
		cmpStatus |= GD_CMP_NUM_COLORS;
	}

	for (y = 0; (y < sy); y++) {
		for (x = 0; (x < sx); x++) {
			p1 =
			    im1->trueColor ? gdImageTrueColorPixel (im1, x,
			            y) :
			    gdImagePalettePixel (im1, x, y);
			p2 =
			    im2->trueColor ? gdImageTrueColorPixel (im2, x,
			            y) :
			    gdImagePalettePixel (im2, x, y);
			if (gdImageRed (im1, p1) != gdImageRed (im2, p2)) {
				cmpStatus |= GD_CMP_COLOR + GD_CMP_IMAGE;
				break;
			}
			if (gdImageGreen (im1, p1) != gdImageGreen (im2, p2)) {
				cmpStatus |= GD_CMP_COLOR + GD_CMP_IMAGE;
				break;
			}
			if (gdImageBlue (im1, p1) != gdImageBlue (im2, p2)) {
				cmpStatus |= GD_CMP_COLOR + GD_CMP_IMAGE;
				break;
			}
#if 0
			/* Soon we'll add alpha channel to palettes */
			if (gdImageAlpha (im1, p1) != gdImageAlpha (im2, p2)) {
				cmpStatus |= GD_CMP_COLOR + GD_CMP_IMAGE;
				break;
			}
#endif
		}
		if (cmpStatus & GD_CMP_COLOR) {
			break;
		};
	}

	return cmpStatus;
}


/* Thanks to Frank Warmerdam for this superior implementation
	of gdAlphaBlend(), which merges alpha in the
	destination color much better. */

/**
 * Function: gdAlphaBlend
 *
 * Blend two colors
 *
 * Parameters:
 *   dst - The color to blend onto.
 *   src - The color to blend.
 *
 * See also:
 *   - <gdImageAlphaBlending>
 *   - <gdLayerOverlay>
 *   - <gdLayerMultiply>
 */
BGD_DECLARE(int) gdAlphaBlend (int dst, int src)
{
	int src_alpha = gdTrueColorGetAlpha(src);
	int dst_alpha, alpha, red, green, blue;
	int src_weight, dst_weight, tot_weight;

	/* -------------------------------------------------------------------- */
	/*      Simple cases we want to handle fast.                            */
	/* -------------------------------------------------------------------- */
	if( src_alpha == gdAlphaOpaque )
		return src;

	dst_alpha = gdTrueColorGetAlpha(dst);
	if( src_alpha == gdAlphaTransparent )
		return dst;
	if( dst_alpha == gdAlphaTransparent )
		return src;

	/* -------------------------------------------------------------------- */
	/*      What will the source and destination alphas be?  Note that      */
	/*      the destination weighting is substantially reduced as the       */
	/*      overlay becomes quite opaque.                                   */
	/* -------------------------------------------------------------------- */
	src_weight = gdAlphaTransparent - src_alpha;
	dst_weight = (gdAlphaTransparent - dst_alpha) * src_alpha / gdAlphaMax;
	tot_weight = src_weight + dst_weight;

	/* -------------------------------------------------------------------- */
	/*      What red, green and blue result values will we use?             */
	/* -------------------------------------------------------------------- */
	alpha = src_alpha * dst_alpha / gdAlphaMax;

	red = (gdTrueColorGetRed(src) * src_weight
	       + gdTrueColorGetRed(dst) * dst_weight) / tot_weight;
	green = (gdTrueColorGetGreen(src) * src_weight
	         + gdTrueColorGetGreen(dst) * dst_weight) / tot_weight;
	blue = (gdTrueColorGetBlue(src) * src_weight
	        + gdTrueColorGetBlue(dst) * dst_weight) / tot_weight;

	/* -------------------------------------------------------------------- */
	/*      Return merged result.                                           */
	/* -------------------------------------------------------------------- */
	return ((alpha << 24) + (red << 16) + (green << 8) + blue);
}

static int gdAlphaOverlayColor (int src, int dst, int max );

/**
 * Function: gdLayerOverlay
 *
 * Overlay two colors
 *
 * Parameters:
 *   dst - The color to overlay onto.
 *   src - The color to overlay.
 *
 * See also:
 *   - <gdImageAlphaBlending>
 *   - <gdAlphaBlend>
 *   - <gdLayerMultiply>
 */
BGD_DECLARE(int) gdLayerOverlay (int dst, int src)
{
	int a1, a2;
	a1 = gdAlphaMax - gdTrueColorGetAlpha(dst);
	a2 = gdAlphaMax - gdTrueColorGetAlpha(src);
	return ( ((gdAlphaMax - a1*a2/gdAlphaMax) << 24) +
		(gdAlphaOverlayColor( gdTrueColorGetRed(src), gdTrueColorGetRed(dst), gdRedMax ) << 16) +
		(gdAlphaOverlayColor( gdTrueColorGetGreen(src), gdTrueColorGetGreen(dst), gdGreenMax ) << 8) +
		(gdAlphaOverlayColor( gdTrueColorGetBlue(src), gdTrueColorGetBlue(dst), gdBlueMax ))
		);
}

/* Apply 'overlay' effect - background pixels are colourised by the foreground colour */
static int gdAlphaOverlayColor (int src, int dst, int max )
{
	dst = dst << 1;
	if( dst > max ) {
		/* in the "light" zone */
		return dst + (src << 1) - (dst * src / max) - max;
	} else {
		/* in the "dark" zone */
		return dst * src / max;
	}
}

/**
 * Function: gdLayerMultiply
 *
 * Overlay two colors with multiply effect
 *
 * Parameters:
 *   dst - The color to overlay onto.
 *   src - The color to overlay.
 *
 * See also:
 *   - <gdImageAlphaBlending>
 *   - <gdAlphaBlend>
 *   - <gdLayerOverlay>
 */
BGD_DECLARE(int) gdLayerMultiply (int dst, int src)
{
	int a1, a2, r1, r2, g1, g2, b1, b2;
	a1 = gdAlphaMax - gdTrueColorGetAlpha(src);
	a2 = gdAlphaMax - gdTrueColorGetAlpha(dst);

	r1 = gdRedMax - (a1 * (gdRedMax - gdTrueColorGetRed(src))) / gdAlphaMax;
	r2 = gdRedMax - (a2 * (gdRedMax - gdTrueColorGetRed(dst))) / gdAlphaMax;
	g1 = gdGreenMax - (a1 * (gdGreenMax - gdTrueColorGetGreen(src))) / gdAlphaMax;
	g2 = gdGreenMax - (a2 * (gdGreenMax - gdTrueColorGetGreen(dst))) / gdAlphaMax;
	b1 = gdBlueMax - (a1 * (gdBlueMax - gdTrueColorGetBlue(src))) / gdAlphaMax;
	b2 = gdBlueMax - (a2 * (gdBlueMax - gdTrueColorGetBlue(dst))) / gdAlphaMax ;

	a1 = gdAlphaMax - a1;
	a2 = gdAlphaMax - a2;
	return ( ((a1*a2/gdAlphaMax) << 24) +
			 ((r1*r2/gdRedMax) << 16) +
			 ((g1*g2/gdGreenMax) << 8) +
			 ((b1*b2/gdBlueMax))
		);
}

/**
 *	Function: gdImageAlphaBlending
 *
 *	Set the effect for subsequent drawing operations
 *
 *	Note that the effect is used for truecolor images only.
 *
 * Parameters:
 *   im               - The image.
 *   alphaBlendingArg - The effect.
 *
 * See also:
 *   - <Effects>
 */
BGD_DECLARE(void) gdImageAlphaBlending (gdImagePtr im, int alphaBlendingArg)
{
	im->alphaBlendingFlag = alphaBlendingArg;
}

/**
 * Function: gdImageSaveAlpha
 *
 * Sets the save alpha flag
 *
 * The save alpha flag specifies whether the alpha channel of the pixels should
 * be saved. This is supported only for image formats that support full alpha
 * transparency, e.g. PNG.
 */
BGD_DECLARE(void) gdImageSaveAlpha (gdImagePtr im, int saveAlphaArg)
{
	im->saveAlphaFlag = saveAlphaArg;
}

/**
 * Function: gdImageSetClip
 *
 * Sets the clipping rectangle
 *
 * The clipping rectangle restricts the drawing area for following drawing
 * operations.
 *
 * Parameters:
 *   im - The image.
 *   x1 - The x-coordinate of the upper left corner.
 *   y1 - The y-coordinate of the upper left corner.
 *   x2 - The x-coordinate of the lower right corner.
 *   y2 - The y-coordinate of the lower right corner.
 *
 * See also:
 *   - <gdImageGetClip>
 */
BGD_DECLARE(void) gdImageSetClip (gdImagePtr im, int x1, int y1, int x2, int y2)
{
	if (x1 < 0) {
		x1 = 0;
	}
	if (x1 >= im->sx) {
		x1 = im->sx - 1;
	}
	if (x2 < 0) {
		x2 = 0;
	}
	if (x2 >= im->sx) {
		x2 = im->sx - 1;
	}
	if (y1 < 0) {
		y1 = 0;
	}
	if (y1 >= im->sy) {
		y1 = im->sy - 1;
	}
	if (y2 < 0) {
		y2 = 0;
	}
	if (y2 >= im->sy) {
		y2 = im->sy - 1;
	}
	im->cx1 = x1;
	im->cy1 = y1;
	im->cx2 = x2;
	im->cy2 = y2;
}

/**
 * Function: gdImageGetClip
 *
 * Gets the current clipping rectangle
 *
 * Parameters:
 *   im - The image.
 *   x1P - (out) The x-coordinate of the upper left corner.
 *   y1P - (out) The y-coordinate of the upper left corner.
 *   x2P - (out) The x-coordinate of the lower right corner.
 *   y2P - (out) The y-coordinate of the lower right corner.
 *
 * See also:
 *   - <gdImageSetClip>
 */
BGD_DECLARE(void) gdImageGetClip (gdImagePtr im, int *x1P, int *y1P, int *x2P, int *y2P)
{
	*x1P = im->cx1;
	*y1P = im->cy1;
	*x2P = im->cx2;
	*y2P = im->cy2;
}

/**
 * Function: gdImageSetResolution
 *
 * Sets the resolution of an image.
 *
 * Parameters:
 *   im    - The image.
 *   res_x - The horizontal resolution in DPI.
 *   res_y - The vertical resolution in DPI.
 *
 * See also:
 *   - <gdImageResolutionX>
 *   - <gdImageResolutionY>
 */
BGD_DECLARE(void) gdImageSetResolution(gdImagePtr im, const unsigned int res_x, const unsigned int res_y)
{
	if (res_x > 0) im->res_x = res_x;
	if (res_y > 0) im->res_y = res_y;
}

/*
 * Added on 2003/12 by Pierre-Alain Joye (pajoye@pearfr.org)
 * */
#define BLEND_COLOR(a, nc, c, cc) \
nc = (cc) + (((((c) - (cc)) * (a)) + ((((c) - (cc)) * (a)) >> 8) + 0x80) >> 8);

static void gdImageSetAAPixelColor(gdImagePtr im, int x, int y, int color, int t)
{
	int dr,dg,db,p,r,g,b;

	/* 2.0.34: watch out for out of range calls */
	if (!gdImageBoundsSafeMacro(im, x, y)) {
		return;
	}
	p = gdImageGetPixel(im,x,y);
	/* TBB: we have to implement the dont_blend stuff to provide
	  the full feature set of the old implementation */
	if ((p == color)
	        || ((p == im->AA_dont_blend)
	            && (t != 0x00))) {
		return;
	}
	dr = gdTrueColorGetRed(color);
	dg = gdTrueColorGetGreen(color);
	db = gdTrueColorGetBlue(color);

	r = gdTrueColorGetRed(p);
	g = gdTrueColorGetGreen(p);
	b = gdTrueColorGetBlue(p);

	BLEND_COLOR(t, dr, r, dr);
	BLEND_COLOR(t, dg, g, dg);
	BLEND_COLOR(t, db, b, db);
	im->tpixels[y][x] = gdTrueColorAlpha(dr, dg, db, gdAlphaOpaque);
}

static void gdImageAALine (gdImagePtr im, int x1, int y1, int x2, int y2, int col)
{
	/* keep them as 32bits */
	long x, y, inc, frac;
	long dx, dy,tmp;
	int w, wid, wstart; 

	if (!im->trueColor) {
		/* TBB: don't crash when the image is of the wrong type */
		gdImageLine(im, x1, y1, x2, y2, col);
		return;
	}

	/* TBB: use the clipping rectangle */
	if (clip_1d (&x1, &y1, &x2, &y2, im->cx1, im->cx2) == 0)
		return;
	if (clip_1d (&y1, &x1, &y2, &x2, im->cy1, im->cy2) == 0)
		return;

	dx = x2 - x1;
	dy = y2 - y1;

	if (dx == 0 && dy == 0) {
		/* TBB: allow setting points */
		gdImageSetAAPixelColor(im, x1, y1, col, 0xFF);
		return;
	} else {
		double ag;
		/* Cast the long to an int to avoid compiler warnings about truncation.
		 * This isn't a problem as computed dy/dx values came from ints above. */
		ag = fabs(abs((int)dy) < abs((int)dx) ? cos(atan2(dy, dx)) : sin(atan2(dy, dx)));
		if (ag != 0) {
			wid = (int)(im->thick / ag);
		} else {
			wid = 1;
		}
		if (wid == 0) {
			wid = 1;
		}
	}

	/* Axis aligned lines */
	if (dx == 0) {
		gdImageVLine(im, x1, y1, y2, col);
		return;
	} else if (dy == 0) {
		gdImageHLine(im, y1, x1, x2, col);
		return;
	}

	if (abs((int)dx) > abs((int)dy)) {
		if (dx < 0) {
			tmp = x1;
			x1 = x2;
			x2 = tmp;
			tmp = y1;
			y1 = y2;
			y2 = tmp;
			dx = x2 - x1;
			dy = y2 - y1;
		}
		y = y1;
		inc = (dy * 65536) / dx;
		frac = 0;
		/* TBB: set the last pixel for consistency (<=) */
		for (x = x1 ; x <= x2 ; x++) {
			wstart = y - wid / 2;
			for (w = wstart; w < wstart + wid; w++) {
			    gdImageSetAAPixelColor(im, x , w , col , (frac >> 8) & 0xFF);
			    gdImageSetAAPixelColor(im, x , w + 1 , col, (~frac >> 8) & 0xFF);
			}
			frac += inc;
			if (frac >= 65536) {
				frac -= 65536;
				y++;
			} else if (frac < 0) {
				frac += 65536;
				y--;
			}
		}
	} else {
		if (dy < 0) {
			tmp = x1;
			x1 = x2;
			x2 = tmp;
			tmp = y1;
			y1 = y2;
			y2 = tmp;
			dx = x2 - x1;
			dy = y2 - y1;
		}
		x = x1;
		inc = (dx * 65536) / dy;
		frac = 0;
		/* TBB: set the last pixel for consistency (<=) */
		for (y = y1 ; y <= y2 ; y++) {
			wstart = x - wid / 2;
			for (w = wstart; w < wstart + wid; w++) {
			    gdImageSetAAPixelColor(im, w , y  , col, (frac >> 8) & 0xFF);
			    gdImageSetAAPixelColor(im, w + 1, y, col, (~frac >> 8) & 0xFF);
			}
			frac += inc;
			if (frac >= 65536) {
				frac -= 65536;
				x++;
			} else if (frac < 0) {
				frac += 65536;
				x--;
			}
		}
	}
}


/**
 * Function: gdImagePaletteToTrueColor
 *
 * Convert a palette image to true color
 *
 * Parameters:
 *   src - The image.
 *
 * Returns:
 *   Non-zero if the conversion succeeded, zero otherwise.
 *
 * See also:
 *   - <gdImageTrueColorToPalette>
 */
BGD_DECLARE(int) gdImagePaletteToTrueColor(gdImagePtr src)
{
	unsigned int y;
	unsigned int yy;

	if (src == NULL) {
		return 0;
	}

	if (src->trueColor == 1) {
		return 1;
	} else {
		unsigned int x;
		const unsigned int sy = gdImageSY(src);
		const unsigned int sx = gdImageSX(src);

		src->tpixels = (int **) gdMalloc(sizeof(int *) * sy);
		if (src->tpixels == NULL) {
			return 0;
		}

		for (y = 0; y < sy; y++) {
			const unsigned char *src_row = src->pixels[y];
			int * dst_row;

			/* no need to calloc it, we overwrite all pxl anyway */
			src->tpixels[y] = (int *) gdMalloc(sx * sizeof(int));
			if (src->tpixels[y] == NULL) {
				goto clean_on_error;
			}

			dst_row = src->tpixels[y];
			for (x = 0; x < sx; x++) {
				const unsigned char c = *(src_row + x);
				if (c == src->transparent) {
					*(dst_row + x) = gdTrueColorAlpha(0, 0, 0, 127);
				} else {
					*(dst_row + x) = gdTrueColorAlpha(src->red[c], src->green[c], src->blue[c], src->alpha[c]);
				}
			}
		}
	}

	/* free old palette buffer (y is sy) */
	for (yy = 0; yy < y; yy++) {
		gdFree(src->pixels[yy]);
	}
	gdFree(src->pixels);
	src->trueColor = 1;
	src->pixels = NULL;
	src->alphaBlendingFlag = 0;
	src->saveAlphaFlag = 1;

	if (src->transparent >= 0) {
		const unsigned char c = src->transparent;
		src->transparent =  gdTrueColorAlpha(src->red[c], src->green[c], src->blue[c], src->alpha[c]);
	}

	return 1;

clean_on_error:
	/* free new true color buffer (y is not allocated, have failed) */
	for (yy = 0; yy < y; yy++) {
		gdFree(src->tpixels[yy]);
	}
	gdFree(src->tpixels);
	return 0;
}
