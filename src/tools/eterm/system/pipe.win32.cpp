#ifdef _WIN32
#include "pipe.hpp"
#include "../terminal/windowserrors.hpp"
#include <windows.h>

using namespace Hexe::System;

Pipe::Pipe( AutoHandle&& readHandle, AutoHandle&& writeHandle ) :
	m_hInput( std::move( readHandle ) ), m_hOutput( std::move( writeHandle ) ) {}

bool Pipe::IsTTY() const {
	return false;
}

int Pipe::Write( const char* s, size_t n ) {
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

int Pipe::Read( char* buf, size_t n, bool block ) {
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

#endif
