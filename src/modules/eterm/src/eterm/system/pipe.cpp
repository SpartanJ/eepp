#ifdef _WIN32
#include <eterm/system/pipe.hpp>
#include <eterm/terminal/windowserrors.hpp>
#include <windows.h>

namespace eterm { namespace System {

Pipe::Pipe( AutoHandle&& readHandle, AutoHandle&& writeHandle ) :
	mInputHandle( std::move( readHandle ) ), mOutputHandle( std::move( writeHandle ) ) {}

bool Pipe::isTTY() const {
	return false;
}

int Pipe::write( const char* s, size_t n ) {
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

int Pipe::read( char* buf, size_t n, bool block ) {
	DWORD available;
	DWORD read;

	if ( !block ) {
		if ( !PeekNamedPipe( mInputHandle.handle(), nullptr, 0, nullptr, &available, nullptr ) ) {
			PrintLastWinApiError();
			return -1;
		}
		if ( available == 0 )
			return 0;
	}

	if ( !ReadFile( mInputHandle.handle(), buf, (DWORD)n, &read, nullptr ) ) {
		PrintLastWinApiError();
		return -1;
	}
	return (int)read;
}

}}

#endif
