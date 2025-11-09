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
#include <assert.h>
#include <eterm/terminal/iterminaldisplay.hpp>
#include <eterm/terminal/terminalemulator.hpp>

#define MODBIT( x, set, bit ) ( ( set ) ? ( ( x ) |= ( bit ) ) : ( ( x ) &= ~( bit ) ) )

namespace eterm { namespace Terminal {

ITerminalDisplay::ITerminalDisplay() :
	mMode( MODE_VISIBLE ), mCursorMode( SteadyBar ), mEmulator( nullptr ) {}

void ITerminalDisplay::attach( TerminalEmulator* terminal ) {
	assert( mEmulator == nullptr );
	mEmulator = terminal;
}

void ITerminalDisplay::detach( TerminalEmulator* terminal ) {
	assert( mEmulator == terminal );
	mEmulator = nullptr;
}

void ITerminalDisplay::bell() {}

void ITerminalDisplay::resetColors() {}

int ITerminalDisplay::resetColor( const Uint32& /*index*/, const char* /*name*/ ) {
	return 0;
}

void ITerminalDisplay::setMode( TerminalWinMode mode, int set ) {
	int m = mMode;
	MODBIT( ( (int&)mMode ), set, mode );
	if ( ( mMode & MODE_REVERSE ) != ( m & MODE_REVERSE ) && mEmulator ) {
		mEmulator->redraw();
	}
}

void ITerminalDisplay::setCursorMode( TerminalCursorMode cursor ) {
	mCursorMode = cursor;
}

TerminalCursorMode ITerminalDisplay::getCursorMode() const {
	return mCursorMode;
}

void ITerminalDisplay::setTitle( const char* ) {}

void ITerminalDisplay::setIconTitle( const char* ) {}

void ITerminalDisplay::setClipboard( const char* ) {}

const char* ITerminalDisplay::getClipboard() const {
	return "";
}

void ITerminalDisplay::onProcessExit( int /*exitCode*/ ) {}

void ITerminalDisplay::onScrollPositionChange() {}

}}
