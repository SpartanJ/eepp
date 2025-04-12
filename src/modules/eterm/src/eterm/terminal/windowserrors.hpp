#ifndef ETERM_WINDOWSERRORS_HPP
#define ETERM_WINDOWSERRORS_HPP
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
#ifdef _WIN32
#include <eepp/system/log.hpp>
using namespace EE::System;

#define NTDDI_VERSION NTDDI_WIN10_RS5
#ifdef _WIN32_WINDOWS
#undef _WIN32_WINDOWS
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#ifdef WINVER
#undef WINVER
#endif
#define _WIN32_WINDOWS 0x0602
#define _WIN32_WINNT 0x0602
#define WINVER 0x0602
#include <comdef.h>
#include <iostream>
#include <string>
#include <windows.h>

inline void PrintErrorResult( HRESULT hr ) {
	_com_error err( hr );
	LPCTSTR errMsg = err.ErrorMessage();
	std::cerr << "ERROR: " << errMsg << std::endl;
	Log::error( "ERROR: %s", errMsg );
}

static void PrintWinApiError( DWORD error ) {
	if ( error == 0 )
		return;
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&messageBuffer, 0, NULL );

	std::string message( messageBuffer, size );

	// Free the buffer.
	LocalFree( messageBuffer );

	Log::error( "ERROR WinAPI: %s", message.c_str() );
}

inline void PrintLastWinApiError( void ) {
	PrintWinApiError( GetLastError() );
}

#endif

#endif
