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

#ifdef WIN32
#include "../terminal/pseudoterminal.h"
#include "windowserrors.h"
#include <assert.h>

using namespace Hexe::Terminal;

PseudoTerminal::~PseudoTerminal() {
	ClosePseudoConsole( m_phPC );
}

PseudoTerminal::PseudoTerminal( int columns, int rows, AutoHandle&& hInput, AutoHandle&& hOutput,
								HPCON hPC ) :
	m_hInput( std::move( hInput ) ),
	m_hOutput( std::move( hOutput ) ),
	m_phPC( hPC ),
	m_attached( false ) {
	m_size.X = (SHORT)columns;
	m_size.Y = (SHORT)rows;
}

bool PseudoTerminal::IsTTY() const {
	return true;
}

bool PseudoTerminal::Resize( int columns, int rows ) {
	HRESULT hr = ResizePseudoConsole( m_phPC, { (SHORT)columns, (SHORT)rows } );
	if ( hr != S_OK ) {
		PrintErrorResult( hr );
		return false;
	}
	m_size.X = (SHORT)columns;
	m_size.Y = (SHORT)rows;
	return true;
}

int PseudoTerminal::Write( const char* s, size_t n ) {
	constexpr size_t lim = 256;

	DWORD c = (DWORD)n;
	DWORD r = 0;
	while ( n > 0 ) {
		if ( !WriteFile( (HANDLE)m_hOutput, s, (DWORD)( n > lim ? lim : n ), &r, nullptr ) ) {
			PrintLastWinApiError();
			return -1;
		}

		s += r;
		n -= r;
	}
	return (int)c;
}

int PseudoTerminal::Read( char* buf, size_t n, bool block ) {
	DWORD available;
	DWORD read;

	if ( !block ) {
		if ( !PeekNamedPipe( (HANDLE)m_hInput, nullptr, 0, nullptr, &available, nullptr ) ) {
			PrintLastWinApiError();
			return -1;
		}
		if ( available == 0 )
			return 0;
	}

	if ( !ReadFile( (HANDLE)m_hInput, buf, (DWORD)n, &read, nullptr ) ) {
		PrintLastWinApiError();
		return -1;
	}
	return (int)read;
}

int PseudoTerminal::GetNumColumns() const {
	return m_size.X;
}

int PseudoTerminal::GetNumRows() const {
	return m_size.Y;
}

std::unique_ptr<PseudoTerminal> PseudoTerminal::Create( int columns, int rows ) {
	using Pointer = std::unique_ptr<PseudoTerminal>;

	HRESULT hr{ E_UNEXPECTED };
	HPCON hPC{ INVALID_HANDLE_VALUE };
	AutoHandle hPipeIn{};
	AutoHandle hPipeOut{};

	AutoHandle hPipePTYIn{};
	AutoHandle hPipePTYOut{};

	if ( CreatePipe( hPipePTYIn.Get(), hPipeOut.Get(), NULL, 0 ) &&
		 CreatePipe( hPipeIn.Get(), hPipePTYOut.Get(), NULL, 0 ) ) {
		COORD ptySize;
		ptySize.X = (SHORT)columns;
		ptySize.Y = (SHORT)rows;

		assert( ptySize.X > 0 );
		assert( ptySize.Y > 0 );

		hr = CreatePseudoConsole( ptySize, (HANDLE)hPipePTYIn, (HANDLE)hPipePTYOut, 0, &hPC );
	}

	if ( hr != S_OK ) {
		PrintErrorResult( hr );
		return Pointer();
	}

	return Pointer(
		new PseudoTerminal( columns, rows, std::move( hPipeIn ), std::move( hPipeOut ), hPC ) );
}
#endif
