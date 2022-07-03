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

#include <eterm/system/autohandle.hpp>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

namespace EE { namespace System {

AutoHandle::AutoHandle() : mHandle( invalid_value() ) {}
AutoHandle::AutoHandle( AutoHandle&& other ) : mHandle( other.mHandle ) {
	other.mHandle = invalid_value();
}
AutoHandle::~AutoHandle() {
	release();
}

AutoHandle& AutoHandle::operator=( AutoHandle&& other ) {
	if ( mHandle == other.mHandle )
		return *this;
	release();
	mHandle = other.mHandle;
	other.mHandle = invalid_value();
	return *this;
}

#ifdef _WIN32
AutoHandle::AutoHandle( HANDLE handle ) : mHandle( handle ) {}

void AutoHandle::release() const {
	if ( mHandle != INVALID_HANDLE_VALUE ) {
		CloseHandle( mHandle );
		mHandle = INVALID_HANDLE_VALUE;
	}
}
#else
AutoHandle::AutoHandle( int fd ) : mHandle( fd ) {}

void AutoHandle::release() const {
	if ( mHandle != -1 ) {
		close( mHandle );
	}
}
#endif

AutoHandle::operator bool() const noexcept {
	return mHandle != invalid_value();
}

AutoHandle::type* AutoHandle::get() {
	return &mHandle;
}

const AutoHandle::type* AutoHandle::get() const {
	return &mHandle;
}

AutoHandle::operator AutoHandle::type() const noexcept {
	return mHandle;
}

#ifdef _WIN32
// On Windows we deal with the Windows API, which means HANDLE is used as the handle to files
// and pipes
constexpr AutoHandle::type AutoHandle::invalid_value() {
	return nullptr;
}
#else
// On non-Windows OS'es we deal with file descriptors
constexpr AutoHandle::type AutoHandle::invalid_value() {
	return -1;
}
#endif

}} // namespace EE::System
