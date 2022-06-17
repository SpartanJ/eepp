// The MIT License (MIT)

// Copyright (c) 2020 Fredrik A. Kristiansen

//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
#pragma once

#include <stdint.h>
#include <string.h>

namespace Hexe { namespace Terminal {

enum cursor_mode {
	BlinkingBlock = 0,
	BlinkingBlockDefault = 1,
	SteadyBlock = 2,
	BlinkUnderline = 3,
	SteadyUnderline = 4,
	BlinkBar = 5,
	SteadyBar = 6,
	StExtension = 7,
	MAX_CURSOR
};

enum win_mode {
	MODE_VISIBLE = 1 << 0,
	MODE_FOCUSED = 1 << 1,
	MODE_APPKEYPAD = 1 << 2,
	MODE_MOUSEBTN = 1 << 3,
	MODE_MOUSEMOTION = 1 << 4,
	MODE_REVERSE = 1 << 5,
	MODE_KBDLOCK = 1 << 6,
	MODE_HIDE = 1 << 7,
	MODE_APPCURSOR = 1 << 8,
	MODE_MOUSESGR = 1 << 9,
	MODE_8BIT = 1 << 10,
	MODE_BLINK = 1 << 11,
	MODE_FBLINK = 1 << 12,
	MODE_FOCUS = 1 << 13,
	MODE_MOUSEX10 = 1 << 14,
	MODE_MOUSEMANY = 1 << 15,
	MODE_BRCKTPASTE = 1 << 16,
	MODE_NUMLOCK = 1 << 17,
	MODE_MOUSE = MODE_MOUSEBTN | MODE_MOUSEMOTION | MODE_MOUSEX10 | MODE_MOUSEMANY,
};

enum glyph_attribute {
	ATTR_NULL = 0,
	ATTR_BOLD = 1 << 0,
	ATTR_FAINT = 1 << 1,
	ATTR_ITALIC = 1 << 2,
	ATTR_UNDERLINE = 1 << 3,
	ATTR_BLINK = 1 << 4,
	ATTR_REVERSE = 1 << 5,
	ATTR_INVISIBLE = 1 << 6,
	ATTR_STRUCK = 1 << 7,
	ATTR_WRAP = 1 << 8,
	ATTR_WIDE = 1 << 9,
	ATTR_WDUMMY = 1 << 10,
	ATTR_BOXDRAW = 1 << 11,
	ATTR_EMOJI = 1 << 12,
	ATTR_BOLD_FAINT = ATTR_BOLD | ATTR_FAINT,
};

enum selection_mode { SEL_IDLE = 0, SEL_EMPTY = 1, SEL_READY = 2 };

enum selection_type { SEL_REGULAR = 1, SEL_RECTANGULAR = 2 };

enum selection_snap { SNAP_WORD = 1, SNAP_LINE = 2 };

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;

typedef uint_least32_t Rune;

typedef struct {
	Rune u;		 /* character code */
	ushort mode; /* attribute flags */
	uint32_t fg; /* foreground  */
	uint32_t bg; /* background  */
} TerminalGlyph;

typedef TerminalGlyph* Line;

union Arg {
	int i;
	uint ui;
	float f;
	const void* v;
	const char* s;

	Arg() { memset( this, 0, sizeof( *this ) ); }
	Arg( int x ) { i = x; }
	Arg( uint x ) { ui = x; }
	Arg( float x ) { f = x; }
	Arg( const void* x ) { v = x; }
	Arg( const char* x ) { s = x; }
};

enum term_mode {
	MODE_WRAP = 1 << 0,
	MODE_INSERT = 1 << 1,
	MODE_ALTSCREEN = 1 << 2,
	MODE_CRLF = 1 << 3,
	MODE_ECHO = 1 << 4,
	MODE_PRINT = 1 << 5,
	MODE_UTF8 = 1 << 6,
};

enum cursor_movement { CURSOR_SAVE, CURSOR_LOAD };

enum cursor_state { CURSOR_DEFAULT = 0, CURSOR_WRAPNEXT = 1, CURSOR_ORIGIN = 2 };

enum charset { CS_GRAPHIC0, CS_GRAPHIC1, CS_UK, CS_USA, CS_MULTI, CS_GER, CS_FIN };

enum escape_state {
	ESC_START = 1,
	ESC_CSI = 2,
	ESC_STR = 4, /* DCS, OSC, PM, APC */
	ESC_ALTCHARSET = 8,
	ESC_STR_END = 16, /* a final string was encountered */
	ESC_TEST = 32,	  /* Enter in test mode */
	ESC_UTF8 = 64,
};

typedef struct {
	TerminalGlyph attr; /* current char attributes */
	int x;
	int y;
	char state;
} TCursor;

typedef struct {
	int mode;
	int type;
	int snap;
	/*
	 * Selection variables:
	 * nb – normalized coordinates of the beginning of the selection
	 * ne – normalized coordinates of the end of the selection
	 * ob – original coordinates of the beginning of the selection
	 * oe – original coordinates of the end of the selection
	 */
	struct {
		int x, y;
	} nb, ne, ob, oe;

	int alt;
} Selection;

}} // namespace Hexe::Terminal
