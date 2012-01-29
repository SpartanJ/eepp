/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_ndsvideo_h
#define _SDL_ndsvideo_h

#include "../SDL_sysvideo.h"

#include "SDL_ndswindow.h"

#define SCREEN_GAP 92			/* line-equivalent gap between the 2 screens  */

/* Per Window information. */
struct NDS_WindowData {
	struct {
		int bg_id;
		void *vram_pixels;           /* where the pixel data is stored (a pointer into VRAM) */
		void *pixels;				 /* area in user frame buffer */
		int length;
	} main, sub;

    int pitch, bpp;             /* useful information about the texture */
    struct {
        int x, y;
    } scale;                    /* x/y stretch (24.8 fixed point) */

    struct {
        int x, y;
    } scroll;                   /* x/y offset */
    int rotate;                 /* -32768 to 32767, texture rotation */

	/* user frame buffer - todo: better way to do double buffering */
	void *pixels;
	int pixels_length;
};


#endif /* _SDL_ndsvideo_h */

/* vi: set ts=4 sw=4 expandtab: */
