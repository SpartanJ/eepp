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

#include "autohandle.hpp"
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

EE::AutoHandle::AutoHandle() : m_hHandle( invalid_value() ) {}
EE::AutoHandle::AutoHandle( AutoHandle&& other ) : m_hHandle( other.m_hHandle ) {
	other.m_hHandle = invalid_value();
}
EE::AutoHandle::~AutoHandle() {
	release();
}

EE::AutoHandle& EE::AutoHandle::operator=( AutoHandle&& other ) {
	if ( m_hHandle == other.m_hHandle )
		return *this;
	release();
	m_hHandle = other.m_hHandle;
	other.m_hHandle = invalid_value();
	return *this;
}

#ifdef _WIN32
EE::AutoHandle::AutoHandle( HANDLE handle ) : m_hHandle( handle ) {}

void EE::AutoHandle::Release() const {
	if ( m_hHandle != INVALID_HANDLE_VALUE ) {
		CloseHandle( m_hHandle );
		m_hHandle = INVALID_HANDLE_VALUE;
	}
}
#else
EE::AutoHandle::AutoHandle( int fd ) : m_hHandle( fd ) {}

void EE::AutoHandle::release() const {
	if ( m_hHandle != -1 ) {
		close( m_hHandle );
	}
}
#endif

EE::AutoHandle::operator bool() const noexcept {
	return m_hHandle != invalid_value();
}

EE::AutoHandle::type* EE::AutoHandle::get() {
	return &m_hHandle;
}

const EE::AutoHandle::type* EE::AutoHandle::get() const {
	return &m_hHandle;
}

EE::AutoHandle::operator EE::AutoHandle::type() const noexcept {
	return m_hHandle;
}

#ifdef _WIN32
// On Windows we deal with the Windows API, which means HANDLE is used as the handle to files
// and pipes
constexpr EE::AutoHandle::type EE::AutoHandle::invalid_value() {
	return static_cast<AutoHandle::type>( 0 );
}
#else
// On non-Windows OS'es we deal with file descriptors
constexpr EE::AutoHandle::type EE::AutoHandle::invalid_value() {
	return -1;
}
#endif

