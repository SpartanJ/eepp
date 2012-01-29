/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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

    SDL1.3 DirectFB driver by couriersud@arcor.de
	
*/

#include "SDL_assert.h"
#include "SDL_DirectFB_video.h"
#include "SDL_DirectFB_shape.h"
#include "SDL_DirectFB_window.h"

#include "../SDL_shape_internals.h"

SDL_Window*
DirectFB_CreateShapedWindow(const char *title,unsigned int x,unsigned int y,unsigned int w,unsigned int h,Uint32 flags) {
    return SDL_CreateWindow(title,x,y,w,h,flags /*| SDL_DFB_WINDOW_SHAPED */);
}

SDL_WindowShaper*
DirectFB_CreateShaper(SDL_Window* window) {
    SDL_WindowShaper* result = NULL;

    result = malloc(sizeof(SDL_WindowShaper));
    result->window = window;
    result->mode.mode = ShapeModeDefault;
    result->mode.parameters.binarizationCutoff = 1;
    result->userx = result->usery = 0;
    SDL_ShapeData* data = SDL_malloc(sizeof(SDL_ShapeData));
    result->driverdata = data;
    data->surface = NULL;
    window->shaper = result;
    int resized_properly = DirectFB_ResizeWindowShape(window);
    SDL_assert(resized_properly == 0);

    return result;
}

int
DirectFB_ResizeWindowShape(SDL_Window* window) {
    SDL_ShapeData* data = window->shaper->driverdata;
    SDL_assert(data != NULL);
        
    if (window->x != -1000) 
    {
		window->shaper->userx = window->x;
		window->shaper->usery = window->y;
    }
    SDL_SetWindowPosition(window,-1000,-1000);
    
    return 0;
}
    
int
DirectFB_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shape_mode) {
 
    if(shaper == NULL || shape == NULL || shaper->driverdata == NULL)
        return -1;
    if(shape->format->Amask == 0 && SDL_SHAPEMODEALPHA(shape_mode->mode))
        return -2;
    if(shape->w != shaper->window->w || shape->h != shaper->window->h)
        return -3;

    {
        SDL_VideoDisplay *display = SDL_GetDisplayForWindow(shaper->window);
        SDL_DFB_DEVICEDATA(display->device);
     	Uint32 *pixels;
     	Sint32 pitch;
     	Uint32 h,w;
     	Uint8  *src, *bitmap;
        DFBSurfaceDescription dsc;

        SDL_ShapeData *data = shaper->driverdata;

        SDL_DFB_RELEASE(data->surface);

        dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
        dsc.width = shape->w;
        dsc.height = shape->h;
        dsc.caps = DSCAPS_PREMULTIPLIED;
        dsc.pixelformat = DSPF_ARGB;

        SDL_DFB_CHECKERR(devdata->dfb->CreateSurface(devdata->dfb, &dsc, &data->surface));

        /* Assume that shaper->alphacutoff already has a value, because SDL_SetWindowShape() should have given it one. */
        SDL_DFB_ALLOC_CLEAR(bitmap, shape->w * shape->h);
        SDL_CalculateShapeBitmap(shaper->mode,shape,bitmap,1);

    	src = bitmap;

        SDL_DFB_CHECK(data->surface->Lock(data->surface, DSLF_WRITE | DSLF_READ, (void **) &pixels, &pitch));

    	h = shaper->window->h;
    	while (h--) {
    		for (w = 0; w < shaper->window->w; w++) {
    			if (*src)
    				pixels[w] = 0xFFFFFFFF;
    			else
    				pixels[w] = 0;
    			src++;

    		}
    		pixels += (pitch >> 2);
    	}
        SDL_DFB_CHECK(data->surface->Unlock(data->surface));
    	SDL_DFB_FREE(bitmap);

    	/* FIXME: Need to call this here - Big ?? */
    	DirectFB_WM_RedrawLayout(SDL_GetDisplayForWindow(shaper->window)->device, shaper->window);
    }
    
    return 0;
error:
	return -1;
}

