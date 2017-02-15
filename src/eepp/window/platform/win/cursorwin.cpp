#include <eepp/window/platform/win/cursorwin.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>

#include <eepp/window/platform/win/winimpl.hpp>
#include <eepp/window/window.hpp>

#define WINDOWS_RGB(r,g,b)  ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

namespace EE { namespace Window { namespace Platform {

CursorWin::CursorWin( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( tex, hotspot, name, window )
{
	create();
}

CursorWin::CursorWin( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( img, hotspot, name, window )
{
	create();
}

CursorWin::CursorWin( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( path, hotspot, name, window )
{
	create();
}

CursorWin::~CursorWin() {
	if ( NULL != mCursor )
		DestroyIcon( (HCURSOR)mCursor );
}

static BITMAPINFO *get_bitmap_info( Image * bitmap ) {
	BITMAPINFO *bi;
	int i;

	bi = (BITMAPINFO *) eeMalloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);

	ZeroMemory(&bi->bmiHeader, sizeof(BITMAPINFOHEADER));

	bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi->bmiHeader.biBitCount = 32;
	bi->bmiHeader.biPlanes = 1;
	bi->bmiHeader.biWidth = (int)bitmap->Width();
	bi->bmiHeader.biHeight = -((int)bitmap->Height());
	bi->bmiHeader.biClrUsed = 256;
	bi->bmiHeader.biCompression = BI_RGB;

	for (i = 0; i < 256; i++) {
		bi->bmiColors[i].rgbRed = 0;
		bi->bmiColors[i].rgbGreen = 0;
		bi->bmiColors[i].rgbBlue = 0;
		bi->bmiColors[i].rgbReserved = 0;
	}

	return bi;
}

static BYTE *get_dib_from_bitmap_32(Image *bitmap) {
	int w, h;
	int x, y;
	int pitch;
	BYTE *pixels;
	BYTE *dst;

	w = (int)bitmap->Width();
	h = (int)bitmap->Height();
	pitch = w * 4;

	pixels = (BYTE *) eeMalloc(h * pitch);
	if (!pixels)
		return NULL;

	for (y = 0; y < h; y++) {
		dst = pixels + y * pitch;

		for (x = 0; x < w; x++) {
			ColorA C = bitmap->GetPixel( x, y );

			/* BGR */
			dst[0] = C.b();
			dst[1] = C.g();
			dst[2] = C.r();
			dst[3] = C.a();

			dst += 4;
		}
	}

	return pixels;
}

static void local_stretch_blit_to_hdc( Image *bitmap, HDC dc, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h) {
	const int bitmap_h = (const int)bitmap->Height();
	const int bottom_up_src_y = bitmap_h - src_y - src_h;
	BYTE *pixels;
	BITMAPINFO *bi;

	bi = get_bitmap_info(bitmap);
	pixels = get_dib_from_bitmap_32(bitmap);

	/* Windows treats all source bitmaps as bottom-up when using StretchDIBits
	* unless the source (x,y) is (0,0).  To work around this buggy behavior, we
	* can use negative heights to reverse the direction of the blits.
	*
	* See <http://wiki.allegro.cc/StretchDIBits> for a detailed explanation.
	*/
	if (bottom_up_src_y == 0 && src_x == 0 && src_h != bitmap_h) {
		StretchDIBits( dc, dest_x, dest_h+dest_y-1, dest_w, -dest_h, src_x, bitmap_h - src_y + 1, src_w, -src_h, pixels, bi, DIB_RGB_COLORS, SRCCOPY );
	} else {
		 StretchDIBits( dc, dest_x, dest_y, dest_w, dest_h, src_x, bottom_up_src_y, src_w, src_h, pixels, bi, DIB_RGB_COLORS, SRCCOPY );
	}

	eeFree(pixels);
	eeFree(bi);
}

static void local_draw_to_hdc( HDC dc, Image * bitmap, int x, int y ) {
	int w = bitmap->Width();
	int h = bitmap->Height();
	local_stretch_blit_to_hdc(bitmap, dc, 0, 0, w, h, x, y, w, h);
}

void CursorWin::create() {
	if ( NULL == mImage && mImage->MemSize() )
		return;

	int x, y;
	int sys_sm_cx, sys_sm_cy;
	HDC h_dc;
	HDC h_and_dc;
	HDC h_xor_dc;
	ICONINFO iconinfo;
	HBITMAP and_mask;
	HBITMAP xor_mask;
	HBITMAP hOldAndMaskBitmap;
	HBITMAP hOldXorMaskBitmap;
	HICON icon;

	/* Get allowed cursor size - Windows can't make cursors of arbitrary size */
	sys_sm_cx = GetSystemMetrics(SM_CXCURSOR);
	sys_sm_cy = GetSystemMetrics(SM_CYCURSOR);

	if ( ( (int)mImage->Width() > sys_sm_cx ) || ( (int)mImage->Height() > sys_sm_cy ) ) {
		return;
	}

	/* Create bitmap */
	h_dc = GetDC( getPlatform()->getHandler() );
	h_xor_dc = CreateCompatibleDC(h_dc);
	h_and_dc = CreateCompatibleDC(h_dc);

	/* Prepare AND (monochrome) and XOR (colour) mask */
	and_mask = CreateBitmap(sys_sm_cx, sys_sm_cy, 1, 1, NULL);
	xor_mask = CreateCompatibleBitmap(h_dc, sys_sm_cx, sys_sm_cy);
	hOldAndMaskBitmap = (HBITMAP) SelectObject(h_and_dc, and_mask);
	hOldXorMaskBitmap = (HBITMAP) SelectObject(h_xor_dc, xor_mask);

	/* Create transparent cursor */
	for (y = 0; y < sys_sm_cy; y++) {
		for (x = 0; x < sys_sm_cx; x++) {
			SetPixel(h_and_dc, x, y, WINDOWS_RGB(255, 255, 255));
			SetPixel(h_xor_dc, x, y, WINDOWS_RGB(0, 0, 0));
		}
	}

	local_draw_to_hdc( h_xor_dc, mImage, 0, 0 );

	/* Make cursor background transparent */
	for (y = 0; y < (int)mImage->Height(); y++) {
		for (x = 0; x < (int)mImage->Width(); x++) {
			ColorA C = mImage->GetPixel( x, y );

			if ( C.a() != 0 ) {
				/* Don't touch XOR value */
				SetPixel( h_and_dc, x, y, 0 );
			} else {
				/* No need to touch AND value */
				SetPixel( h_xor_dc, x, y, WINDOWS_RGB( 0, 0, 0 ) );
			}
		}
	}

	SelectObject(h_and_dc, hOldAndMaskBitmap);
	SelectObject(h_xor_dc, hOldXorMaskBitmap);
	DeleteDC(h_and_dc);
	DeleteDC(h_xor_dc);
	ReleaseDC( getPlatform()->getHandler() , h_dc );

	iconinfo.fIcon = false;
	iconinfo.xHotspot = mHotSpot.x;
	iconinfo.yHotspot = mHotSpot.y;
	iconinfo.hbmMask = and_mask;
	iconinfo.hbmColor = xor_mask;

	icon = CreateIconIndirect(&iconinfo);

	DeleteObject(and_mask);
	DeleteObject(xor_mask);

	mCursor = (void*)icon;
}

WinImpl * CursorWin::getPlatform() {
	return reinterpret_cast<WinImpl*>( mWindow->getPlatform() );
}

void * CursorWin::getCursor() const {
	return mCursor;
}

}}}

#endif
