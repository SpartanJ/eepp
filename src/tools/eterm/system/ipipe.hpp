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

#include <stdint.h>
#include <stdlib.h>

namespace Hexe { namespace System {

class IPipe {
  public:
	IPipe() = default;

	IPipe( IPipe&& ) = delete;

	IPipe( const IPipe& ) = delete;

	IPipe& operator=( IPipe&& ) = delete;

	IPipe& operator=( const IPipe& ) = delete;

	virtual ~IPipe() = default;

	virtual bool IsTTY() const = 0;

	virtual int Write( const char* s, size_t n ) = 0;

	virtual int Read( char* buf, size_t n, bool block = false ) = 0;
};

}} // namespace Hexe::System
