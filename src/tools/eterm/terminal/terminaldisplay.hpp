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

#include "types.hpp"

namespace Hexe { namespace Terminal {

class TerminalEmulator;
class TerminalDisplay {
  protected:
	int mMode;
	cursor_mode mCursorMode;
	TerminalEmulator* mEmulator;

  public:
	TerminalDisplay();
	virtual ~TerminalDisplay() = default;
	TerminalDisplay( const TerminalDisplay& ) = delete;
	TerminalDisplay( TerminalDisplay&& ) = delete;
	TerminalDisplay& operator=( const TerminalDisplay& ) = delete;
	TerminalDisplay& operator=( TerminalDisplay&& ) = delete;

	virtual void Attach( TerminalEmulator* terminal );
	virtual void Detach( TerminalEmulator* terminal );

	virtual void Bell();

	virtual void ResetColors();
	virtual int ResetColor( int index, const char* name );

	virtual void SetMode( win_mode mode, int set );
	inline bool GetMode( win_mode mode ) const { return mMode & mode; }

	virtual void SetCursorMode( cursor_mode cursor );
	virtual cursor_mode GetCursorMode() const;

	virtual void SetTitle( const char* title );
	virtual void SetIconTitle( const char* title );

	virtual void SetClipboard( const char* text );
	virtual const char* GetClipboard() const;

	virtual bool DrawBegin( int columns, int rows ) = 0;
	virtual void DrawLine( Line line, int x1, int y, int x2 ) = 0;
	virtual void DrawCursor( int cx, int cy, TerminalGlyph g, int ox, int oy, TerminalGlyph og ) = 0;
	virtual void DrawEnd() = 0;
};

}} // namespace Hexe::Terminal
