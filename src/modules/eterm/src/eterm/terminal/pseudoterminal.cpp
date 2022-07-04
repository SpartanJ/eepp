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

#ifndef _WIN32
// Windows has its own source file
#include <cstdio>
#include <eterm/terminal/pseudoterminal.hpp>
#include <poll.h>
#if defined( __linux )
#include <pty.h>
#elif defined( __OpenBSD__ ) || defined( __NetBSD__ ) || defined( __APPLE__ )
#include <util.h>
#elif defined( __FreeBSD__ ) || defined( __DragonFly__ )
#include <libutil.h>
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
#define _DEFAULT_SOURCE
#include <bsd/pty.h>
#endif
#include <eepp/system/log.hpp>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
using namespace EE::System;

namespace eterm { namespace Terminal {

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <fcntl.h>

int openpty( int* amaster, int* aslave, char* name, struct termios* termp, struct winsize* winp ) {
	int master, slave;
	char* name_slave;

	master = open( "/dev/ptmx", O_RDWR | O_NONBLOCK );
	if ( master == -1 ) {
		Log::error( "openpty: Fail to open master" );
		return -1;
	}

	if ( grantpt( master ) ) {
		Log::error( "openpty: grantpt failed" );
		goto fail;
	}

	if ( unlockpt( master ) ) {
		Log::error( "openpty: unlockpt failed" );
		goto fail;
	}

	name_slave = ptsname( master );
	slave = open( name_slave, O_RDWR | O_NOCTTY );
	if ( slave == -1 ) {
		Log::error( "openpty: failed to open slave name %s", name_slave );
		goto fail;
	}

	if ( termp )
		tcsetattr( slave, TCSAFLUSH, termp );
	if ( winp )
		ioctl( slave, TIOCSWINSZ, winp );

	*amaster = master;
	*aslave = slave;
	if ( name != NULL )
		strcpy( name, name_slave );

	return 0;

fail:
	close( master );
	return -1;
}
#endif

PseudoTerminal::~PseudoTerminal() {}

PseudoTerminal::PseudoTerminal( int columns, int rows, AutoHandle&& master, AutoHandle&& slave ) :
	mColumns( columns ),
	mRows( rows ),
	mMaster( std::move( master ) ),
	mSlave( std::move( slave ) ) {}

bool PseudoTerminal::isTTY() const {
	return true;
}

bool PseudoTerminal::resize( int columns, int rows ) {
	struct winsize w;

	w.ws_row = rows;
	w.ws_col = columns;
	w.ws_xpixel = 0;
	w.ws_ypixel = 0;

	if ( ioctl( (int)mMaster, TIOCSWINSZ, &w ) < 0 ) {
		perror( "PseudoTerminal::Resize" );
		return false;
	}

	return true;
}

int PseudoTerminal::getNumColumns() const {
	return mColumns;
}

int PseudoTerminal::getNumRows() const {
	return mRows;
}

int PseudoTerminal::write( const char* s, size_t n ) {
	size_t c = n;
	while ( n > 0 ) {
		ssize_t r = ::write( (int)mMaster, s, n );
		if ( r < 0 ) {
			return -1;
		}
		n -= r;
		s += r;
	}
	return c;
}

int PseudoTerminal::read( char* s, size_t n, bool block ) {
	struct pollfd pfd;
	pfd.fd = (int)mMaster;
	pfd.events = POLLIN;
	pfd.revents = 0;

	if ( !block ) {
		auto i = poll( &pfd, 1, 0 );
		if ( i == 0 || !( pfd.revents & POLLIN ) ) {
			return 0;
		}
		if ( i < 0 ) {
			perror( "PseudoTerminal::Reader(poll)" );
			return -1;
		}
	}
	ssize_t r = ::read( (int)mMaster, s, n );
	return (int)r;
}

std::unique_ptr<PseudoTerminal> PseudoTerminal::create( int columns, int rows ) {
	AutoHandle master;
	AutoHandle slave;

	struct winsize win;

	memset( &win, 0, sizeof( win ) );

	win.ws_col = columns;
	win.ws_row = rows;

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return nullptr;
#else
	if ( openpty( master.get(), slave.get(), nullptr, NULL, &win ) != 0 ) {
		perror( "PseudoTerminal::create(openpty)" );
		Log::error( "PseudoTerminal::create(openpty)" );
		return nullptr;
	}

	return std::unique_ptr<PseudoTerminal>(
		new PseudoTerminal( columns, rows, std::move( master ), std::move( slave ) ) );
#endif
}

}} // namespace eterm::Terminal

#else
#include <assert.h>
#include <eterm/terminal/pseudoterminal.hpp>
#include <eterm/terminal/windowserrors.hpp>
#define NTDDI_VERSION NTDDI_WIN10_RS5
#include <windows.h>

namespace eterm { namespace Terminal {

PseudoTerminal::~PseudoTerminal() {
	ClosePseudoConsole( mPHPC );
}

PseudoTerminal::PseudoTerminal( int columns, int rows, AutoHandle&& hInput, AutoHandle&& hOutput,
								void* hPC ) :
	mInputHandle( std::move( hInput ) ),
	mOutputHandle( std::move( hOutput ) ),
	mPHPC( hPC ),
	mAttached( false ) {
	mSize.x = (SHORT)columns;
	mSize.y = (SHORT)rows;
}

bool PseudoTerminal::isTTY() const {
	return true;
}

bool PseudoTerminal::resize( int columns, int rows ) {
	HRESULT hr = ResizePseudoConsole( mPHPC, { (SHORT)columns, (SHORT)rows } );
	if ( hr != S_OK ) {
		PrintErrorResult( hr );
		return false;
	}
	mSize.x = (SHORT)columns;
	mSize.y = (SHORT)rows;
	return true;
}

int PseudoTerminal::write( const char* s, size_t n ) {
	constexpr size_t lim = 256;

	DWORD c = (DWORD)n;
	DWORD r = 0;
	while ( n > 0 ) {
		if ( !WriteFile( (HANDLE)mOutputHandle, s, (DWORD)( n > lim ? lim : n ), &r, nullptr ) ) {
			PrintLastWinApiError();
			return -1;
		}

		s += r;
		n -= r;
	}
	return (int)c;
}

int PseudoTerminal::read( char* buf, size_t n, bool block ) {
	DWORD available = 0;
	DWORD read;

	if ( !block ) {
		if ( !PeekNamedPipe( mInputHandle.handle(), nullptr, 0, nullptr, &available, nullptr ) ) {
			PrintLastWinApiError();
			return -1;
		}
		if ( available == 0 ) {
			return 0;
		}
	}

	if ( !ReadFile( mInputHandle.handle(), buf, available, &read, nullptr ) ) {
		PrintLastWinApiError();
		return -1;
	}
	return (int)read;
}

int PseudoTerminal::getNumColumns() const {
	return mSize.x;
}

int PseudoTerminal::getNumRows() const {
	return mSize.y;
}

std::unique_ptr<PseudoTerminal> PseudoTerminal::create( int columns, int rows ) {
	using Pointer = std::unique_ptr<PseudoTerminal>;

	HRESULT hr{ E_UNEXPECTED };
	void* hPC{ INVALID_HANDLE_VALUE };
	AutoHandle hPipeIn{};
	AutoHandle hPipeOut{};

	AutoHandle hPipePTYIn{};
	AutoHandle hPipePTYOut{};

	if ( CreatePipe( hPipePTYIn.get(), hPipeOut.get(), NULL, 0 ) &&
		 CreatePipe( hPipeIn.get(), hPipePTYOut.get(), NULL, 0 ) ) {
		COORD ptySize;
		ptySize.X = (SHORT)columns;
		ptySize.Y = (SHORT)rows;

		assert( ptySize.X > 0 );
		assert( ptySize.Y > 0 );

		hr = CreatePseudoConsole( ptySize, hPipePTYIn.handle(), hPipePTYOut.handle(), 0, &hPC );
	}

	if ( hr != S_OK ) {
		PrintErrorResult( hr );
		return Pointer();
	}

	return Pointer(
		new PseudoTerminal( columns, rows, std::move( hPipeIn ), std::move( hPipeOut ), hPC ) );
}

}} // namespace eterm::Terminal

#endif
