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

#include "SDL_assert.h"
#include "SDL_windowsvideo.h"

#include "../../events/SDL_mouse_c.h"


HCURSOR SDL_cursor = NULL;


static SDL_Cursor *
WIN_CreateDefaultCursor()
{
    SDL_Cursor *cursor;

    cursor = SDL_calloc(1, sizeof(*cursor));
    if (cursor) {
        cursor->driverdata = LoadCursor(NULL, IDC_ARROW);
    } else {
        SDL_OutOfMemory();
    }

    return cursor;
}

static SDL_Cursor *
WIN_CreateCursor(SDL_Surface * surface, int hot_x, int hot_y)
{
    SDL_Cursor *cursor;
    HICON hicon;
    HDC hdc;
    BITMAPV4HEADER bmh;
    LPVOID pixels;
    ICONINFO ii;

    SDL_zero(bmh);
    bmh.bV4Size = sizeof(bmh);
    bmh.bV4Width = surface->w;
    bmh.bV4Height = -surface->h; /* Invert the image */
    bmh.bV4Planes = 1;
    bmh.bV4BitCount = 32;
    bmh.bV4V4Compression = BI_BITFIELDS;
    bmh.bV4AlphaMask = 0xFF000000;
    bmh.bV4RedMask   = 0x00FF0000;
    bmh.bV4GreenMask = 0x0000FF00;
    bmh.bV4BlueMask  = 0x000000FF;

    hdc = GetDC(NULL);
    SDL_zero(ii);
    ii.fIcon = FALSE;
    ii.xHotspot = (DWORD)hot_x;
    ii.yHotspot = (DWORD)hot_y;
    ii.hbmColor = CreateDIBSection(hdc, (BITMAPINFO*)&bmh, DIB_RGB_COLORS, &pixels, NULL, 0);
    ii.hbmMask = CreateBitmap(surface->w, surface->h, 1, 1, NULL);
    ReleaseDC(NULL, hdc);

    SDL_assert(surface->format->format == SDL_PIXELFORMAT_ARGB8888);
    SDL_assert(surface->pitch == surface->w * 4);
    SDL_memcpy(pixels, surface->pixels, surface->h * surface->pitch);

    hicon = CreateIconIndirect(&ii);

    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);

    if (!hicon) {
        WIN_SetError("CreateIconIndirect()");
        return NULL;
    }

    cursor = SDL_calloc(1, sizeof(*cursor));
    if (cursor) {
        cursor->driverdata = hicon;
    } else {
        DestroyIcon(hicon);
        SDL_OutOfMemory();
    }

    return cursor;
}

static void
WIN_FreeCursor(SDL_Cursor * cursor)
{
    HICON hicon = (HICON)cursor->driverdata;

    DestroyIcon(hicon);
    SDL_free(cursor);
}

static int
WIN_ShowCursor(SDL_Cursor * cursor)
{
    if (cursor) {
        SDL_cursor = (HCURSOR)cursor->driverdata;
    } else {
        SDL_cursor = NULL;
    }
    if (SDL_GetMouseFocus() != NULL) {
        SetCursor(SDL_cursor);
    }
    return 0;
}

static void
WIN_WarpMouse(SDL_Window * window, int x, int y)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    POINT pt;

    pt.x = x;
    pt.y = y;
    ClientToScreen(hwnd, &pt);
    SetCursorPos(pt.x, pt.y);
}

static int
WIN_SetRelativeMouseMode(SDL_bool enabled)
{
    SDL_Unsupported();
    return -1;
}

void
WIN_InitMouse(_THIS)
{
    SDL_Mouse *mouse = SDL_GetMouse();

    mouse->CreateCursor = WIN_CreateCursor;
    mouse->ShowCursor = WIN_ShowCursor;
    mouse->FreeCursor = WIN_FreeCursor;
    mouse->WarpMouse = WIN_WarpMouse;
    mouse->SetRelativeMouseMode = WIN_SetRelativeMouseMode;

    SDL_SetDefaultCursor(WIN_CreateDefaultCursor());
}

void
WIN_QuitMouse(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
