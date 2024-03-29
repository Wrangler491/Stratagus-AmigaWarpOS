//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name png.c - The png graphic file loader. */
//
//      (c) Copyright 1998-2004 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//      $Id: png.c,v 1.41 2004/06/26 14:01:06 jarod42 Exp $

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#include "stratagus.h"
#include "video.h"
#include "iolib.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  png read callback for CL-IO.
**
**  @param png_ptr  png struct pointer.
**  @param data     byte address to read to.
**  @param length   number of bytes to read.
*/
static void CL_png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	png_size_t check;

	check = (png_size_t)CLread((CLFile*)png_get_io_ptr(png_ptr), data,
		(size_t)length);
	if (check != length) {
		png_error(png_ptr, "Read Error");
	}
}

/**
**  Load a png graphic file.
**  Modified function from SDL_Image
**
**  @param name  png filename to load.
**
**  @return      graphic object with loaded graphic, or NULL if failure.
*/
Graphic* LoadGraphicPNG(const char* name)
{
	Graphic* graphic;
	CLFile* fp;
	SDL_Surface* volatile surface;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width;
	png_uint_32 height;
	int bit_depth;
	int color_type;
	int interlace_type;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	SDL_Palette* palette;
	png_bytep* volatile row_pointers;
	int row;
	int i;
	volatile int ckey;
	png_color_16* transv;

	ckey = -1;

	if (!name) {
		return NULL;
	}
	if (!(fp = CLopen(name, CL_OPEN_READ))) {
		perror("Can't open file");
		return NULL;
	}
	graphic = NULL;

	/* Initialize the data we will clean up when we're done */
	png_ptr = NULL; info_ptr = NULL; row_pointers = NULL; surface = NULL;

	/* Create the PNG loading context structure */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (png_ptr == NULL){
		fprintf(stderr, "Couldn't allocate memory for PNG file");
		goto done;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "Couldn't create image information for PNG file");
		goto done;
	}

	/* Set error handling if you are using setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in png_create_read_struct() earlier.
	 */
	if (setjmp(png_ptr->jmpbuf)) {
		fprintf(stderr, "Error reading the PNG file.");
		goto done;
	}

	/* Set up the input control */
	png_set_read_fn(png_ptr, fp, CL_png_read_data);

	/* Read PNG header info */
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
		&color_type, &interlace_type, NULL, NULL);

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr) ;

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	/* scale greyscale values to the range 0..255 */
	if (color_type == PNG_COLOR_TYPE_GRAY) {
		png_set_expand(png_ptr);
	}

	/* For images with a single "transparent colour", set colour key;
	 if more than one index has transparency, or if partially transparent
	 entries exist, use full alpha channel */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		int num_trans;
		Uint8* trans;

		png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans,
			&transv);
		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			/* Check if all tRNS entries are opaque except one */
			int i;
			int t;

			t = -1;
			for (i = 0; i < num_trans; ++i) {
				if (trans[i] == 0) {
					if (t >= 0) {
						break;
					}
					t = i;
				} else if (trans[i] != 255) {
					break;
				}
			}
			if (i == num_trans) {
				/* exactly one transparent index */
				ckey = t;
			} else {
				/* more than one transparent index, or translucency */
				png_set_expand(png_ptr);
			}
		} else {
			ckey = 0; /* actual value will be set later */
		}
	}

	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
		&color_type, &interlace_type, NULL, NULL);

	/* Allocate the SDL surface to hold the image */
	Rmask = Gmask = Bmask = Amask = 0 ;
	if (color_type != PNG_COLOR_TYPE_PALETTE) {
		if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
			Rmask = 0x000000FF;
			Gmask = 0x0000FF00;
			Bmask = 0x00FF0000;
			Amask = (info_ptr->channels == 4) ? 0xFF000000 : 0;
		} else {
			int s;

			s = (info_ptr->channels == 4) ? 0 : 8;
			Rmask = 0xFF000000 >> s;
			Gmask = 0x00FF0000 >> s;
			Bmask = 0x0000FF00 >> s;
			Amask = 0x000000FF >> s;
		}
	}
	surface = SDL_AllocSurface(SDL_SWSURFACE, width, height,
		bit_depth * info_ptr->channels, Rmask, Gmask, Bmask, Amask);
	if (surface == NULL) {
		fprintf(stderr, "Out of memory");
		goto done;
	}

	if (ckey != -1) {
		if (color_type != PNG_COLOR_TYPE_PALETTE) {
			/* FIXME: Should these be truncated or shifted down? */
			ckey = SDL_MapRGB(surface->format,
				(Uint8)transv->red,
				(Uint8)transv->green,
				(Uint8)transv->blue);
		}
		SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, ckey);
	}

	/* Create the array of pointers to image data */
	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	if (row_pointers == NULL) {
		fprintf(stderr, "Out of memory");
		SDL_FreeSurface(surface);
		surface = NULL;
		goto done;
	}
	for (row = 0; row < (int)height; ++row) {
		row_pointers[row] = (png_bytep)
			(Uint8 *)surface->pixels + row * surface->pitch;
	}

	/* Read the entire image in one go */
	png_read_image(png_ptr, row_pointers);

	/* read rest of file, get additional chunks in info_ptr - REQUIRED */
	png_read_end(png_ptr, info_ptr);

	/* Load the palette, if any */
	palette = surface->format->palette;
	if (palette) {
		if (color_type == PNG_COLOR_TYPE_GRAY) {
			palette->ncolors = 256;
			for (i = 0; i < 256; ++i) {
				palette->colors[i].r = i;
				palette->colors[i].g = i;
				palette->colors[i].b = i;
			}
		} else if (info_ptr->num_palette > 0) {
			palette->ncolors = info_ptr->num_palette;
			for (i = 0; i < info_ptr->num_palette; ++i) {
				palette->colors[i].b = info_ptr->palette[i].blue;
				palette->colors[i].g = info_ptr->palette[i].green;
				palette->colors[i].r = info_ptr->palette[i].red;
			}
		}
	}

	graphic = MakeGraphic(surface);

done:   /* Clean up and return */
	png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : (png_infopp)0,
		(png_infopp)0);
	if (row_pointers) {
		free(row_pointers);
	}
	CLclose(fp);
	return graphic;
}

/**
**  Save a screenshot to a PNG file.
**
**  @param name  PNG filename to save.
*/
void SaveScreenshotPNG(const char* name)
{
	FILE* fp;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char* row;
	int i;
	int bpp;
#ifdef USE_OPENGL
	GLvoid* pixels;
#else
	int j;
#endif

	bpp = TheScreen->format->BytesPerPixel;

	fp = fopen(name, "wb");
	if (fp == NULL) {
		return;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fclose(fp);
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return;
	}

	if (setjmp(png_ptr->jmpbuf)) {
		/* If we get here, we had a problem reading the file */
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return;
	}

	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, VideoWidth, VideoHeight, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_set_bgr(png_ptr);

	VideoLockScreen();

	row = (char*)malloc(VideoWidth * 3);

	png_write_info(png_ptr, info_ptr);

#ifdef USE_OPENGL
	pixels = malloc(VideoWidth * VideoHeight * 3);
	if (!pixels) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
#ifndef WARPOS
	glReadBuffer(GL_FRONT);
#endif
	glReadPixels(0, 0, VideoWidth, VideoHeight, GL_RGB, GL_UNSIGNED_BYTE,
		pixels);
	for (i = 0; i < VideoHeight; ++i) {
		int j;
		unsigned char* src;
		unsigned char* dst;

		src = (unsigned char*)pixels + (VideoHeight - 1 - i) * VideoWidth * 3;
		dst = row;

		// Convert bgr to rgb
		for (j = 0; j < VideoWidth; ++j) {
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
			dst += 3;
			src += 3;
		}
		png_write_row(png_ptr, row);
	}
	free(pixels);
#else
	for (i = 0; i < VideoHeight; ++i) {
		switch (VideoDepth) {
			case 15: {
				Uint16 c;
				for (j = 0; j < VideoWidth; ++j) {
					c = ((Uint16*)TheScreen->pixels)[j + i * VideoWidth];
					row[j * 3 + 0] = (((c >> 0) & 0x1f) * 0xff) / 0x1f;
					row[j * 3 + 1] = (((c >> 5) & 0x1f) * 0xff) / 0x1f;
					row[j * 3 + 2] = (((c >> 10) & 0x1f) * 0xff) / 0x1f;
				}
				break;
			}
			case 16: {
				Uint16 c;
				for (j = 0; j < VideoWidth; ++j) {
					c = ((Uint16*)TheScreen->pixels)[j + i * VideoWidth];
					row[j * 3 + 0] = (((c >> 0) & 0x1f) * 0xff) / 0x1f;
					row[j * 3 + 1] = (((c >> 5) & 0x3f) * 0xff) / 0x3f;
					row[j * 3 + 2] = (((c >> 11) & 0x1f) * 0xff) / 0x1f;
				}
				break;
			}
			case 24: {
				Uint8 c;
				for (j = 0; j < VideoWidth; ++j) {
					c = ((Uint8*)TheScreen->pixels)[j * bpp + i * VideoWidth * 3];
					memcpy(row, (char*)TheScreen->pixels + i * VideoWidth, VideoWidth * 3);
				}
				break;
			}
			case 32: {
				Uint32 c;
				for (j = 0; j < VideoWidth; ++j) {
					c = ((Uint32*)TheScreen->pixels)[j + i * VideoWidth];
					row[j * 3 + 0] = ((c >> 0) & 0xff);
					row[j * 3 + 1] = ((c >> 8) & 0xff);
					row[j * 3 + 2] = ((c >> 16) & 0xff);
				}
				break;
			}
		}
		png_write_row(png_ptr, row);
	}
#endif

	png_write_end(png_ptr, info_ptr);

	VideoUnlockScreen();

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(row);

	fclose(fp);
}

//@}
