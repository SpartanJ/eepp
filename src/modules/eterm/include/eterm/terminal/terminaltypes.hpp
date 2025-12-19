#ifndef ETERM_TERMINALTYPES_HPP
#define ETERM_TERMINALTYPES_HPP
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
#include <string>
#include <string_view>

using namespace std::literals;

namespace eterm { namespace Terminal {

enum TerminalCursorMode {
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

struct TerminalCursorHelper {
	static TerminalCursorMode modeFromString( std::string_view str ) {
		if ( str == "blinking_block" )
			return TerminalCursorMode::BlinkingBlock;
		if ( str == "steady_block" )
			return TerminalCursorMode::SteadyBlock;
		if ( str == "blink_underline" )
			return TerminalCursorMode::BlinkUnderline;
		if ( str == "steady_underline" )
			return TerminalCursorMode::SteadyUnderline;
		if ( str == "blink_bar" )
			return TerminalCursorMode::BlinkBar;
		if ( str == "steady_bar" )
			return TerminalCursorMode::SteadyBar;
		return TerminalCursorMode::SteadyUnderline;
	}

	static std::string modeToString( TerminalCursorMode mode ) {
		switch ( mode ) {
			case BlinkingBlock:
			case BlinkingBlockDefault:
				return "blinking_block";
			case SteadyBlock:
				return "steady_block";
			case BlinkUnderline:
				return "blink_underline";
			case SteadyUnderline:
				return "steady_underline";
			case BlinkBar:
				return "blink_bar";
			case SteadyBar:
				return "steady_bar";
			case StExtension:
			case MAX_CURSOR:
				break;
		}
		return "steady_underline";
	}
};

enum TerminalWinMode {
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

enum TerminalGlyphAttribute {
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

enum TerminalSelectionMode { SEL_IDLE = 0, SEL_EMPTY = 1, SEL_READY = 2 };

enum TerminalSelectionType { SEL_REGULAR = 1, SEL_RECTANGULAR = 2 };

enum TerminalSelectionSnap { SNAP_WORD = 1, SNAP_LINE = 2 };

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;

typedef uint_least32_t Rune;

struct TerminalGlyph {
	Rune u{ 0 };	  /* character code */
	ushort mode{ 0 }; /* attribute flags */
	uint32_t fg{ 0 }; /* foreground  */
	uint32_t bg{ 0 }; /* background  */

	bool operator==( const TerminalGlyph& r ) {
		return u == r.u && mode == r.mode && fg == r.fg && bg == r.bg;
	}

	bool operator!=( const TerminalGlyph& r ) { return !( *this == r ); }
};

typedef TerminalGlyph* Line;

union TerminalArg {
	int i;
	uint ui;
	float f;
	const void* v;
	const char* s;

	TerminalArg() { memset( this, 0, sizeof( *this ) ); }
	TerminalArg( int x ) { i = x; }
	TerminalArg( uint x ) { ui = x; }
	TerminalArg( float x ) { f = x; }
	TerminalArg( const void* x ) { v = x; }
	TerminalArg( const char* x ) { s = x; }
};

enum TermMode {
	MODE_WRAP = 1 << 0,
	MODE_INSERT = 1 << 1,
	MODE_ALTSCREEN = 1 << 2,
	MODE_CRLF = 1 << 3,
	MODE_ECHO = 1 << 4,
	MODE_PRINT = 1 << 5,
	MODE_UTF8 = 1 << 6,
};

enum TerminalCursorMovement { CURSOR_SAVE, CURSOR_LOAD };

enum TerminalCursorState { CURSOR_DEFAULT = 0, CURSOR_WRAPNEXT = 1, CURSOR_ORIGIN = 2 };

enum TerminalCharset { CS_GRAPHIC0, CS_GRAPHIC1, CS_UK, CS_USA, CS_MULTI, CS_GER, CS_FIN };

enum TerminalEscapeState {
	ESC_START = 1,
	ESC_CSI = 2,
	ESC_STR = 4, /* DCS, OSC, PM, APC */
	ESC_ALTCHARSET = 8,
	ESC_STR_END = 16, /* a final string was encountered */
	ESC_TEST = 32,	  /* Enter in test mode */
	ESC_UTF8 = 64,
};

struct TerminalCursor {
	TerminalGlyph attr; /* current char attributes */
	int x{ 0 };
	int y{ 0 };
	char state{ 0 };
};

struct TerminalSelection {
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
};

}} // namespace eterm::Terminal

#endif
