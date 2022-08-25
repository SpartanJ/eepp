#ifndef ETERM_IETERMINALDISPLAY_HPP
#define ETERM_IETERMINALDISPLAY_HPP
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
#include <eepp/config.hpp>
#include <eterm/terminal/terminaltypes.hpp>

using namespace EE;

namespace eterm { namespace Terminal {

class TerminalEmulator;

class ITerminalDisplay {
  public:
	ITerminalDisplay();

	virtual ~ITerminalDisplay() = default;

	ITerminalDisplay( const ITerminalDisplay& ) = delete;

	ITerminalDisplay( ITerminalDisplay&& ) = delete;

	ITerminalDisplay& operator=( const ITerminalDisplay& ) = delete;

	ITerminalDisplay& operator=( ITerminalDisplay&& ) = delete;

	virtual void attach( TerminalEmulator* terminal );

	virtual void detach( TerminalEmulator* terminal );

	virtual void bell();

	virtual void resetColors();

	virtual int resetColor( const Uint32& index, const char* name );

	virtual void setMode( TerminalWinMode mode, int set );

	inline bool getMode( TerminalWinMode mode ) const { return mMode & mode; }

	virtual void setCursorMode( TerminalCursorMode cursor );

	virtual TerminalCursorMode getCursorMode() const;

	virtual void setTitle( const char* title );

	virtual void setIconTitle( const char* title );

	virtual void setClipboard( const char* text );

	virtual const char* getClipboard() const;

	virtual bool drawBegin( Uint32 columns, Uint32 rows ) = 0;

	virtual void drawLine( Line line, int x1, int y, int x2 ) = 0;

	virtual void drawCursor( int cx, int cy, TerminalGlyph g, int ox, int oy,
							 TerminalGlyph og ) = 0;

	virtual void drawEnd() = 0;

  protected:
	friend class TerminalEmulator;

	int mMode;
	TerminalCursorMode mCursorMode;
	TerminalEmulator* mEmulator;

	virtual void onProcessExit( int exitCode );
};

}} // namespace eterm::Terminal

#endif
