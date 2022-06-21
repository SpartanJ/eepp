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

#include "../system/ipipe.hpp"
#include "autohandle.hpp"
#include <memory>

namespace EE { namespace System {

class Process;

class Pipe : public IPipe {
  public:
	Pipe( Pipe&& ) = delete;

	Pipe( const Pipe& ) = delete;

	Pipe& operator=( Pipe&& ) = delete;

	Pipe& operator=( const Pipe& ) = delete;

	virtual ~Pipe() = default;

	virtual bool isTTY() const override;

	virtual int write( const char* s, size_t n ) override;

	virtual int read( char* buf, size_t n, bool block = false ) override;

	static bool CreatePipePair( std::unique_ptr<Pipe>& outPipeA, std::unique_ptr<Pipe>& outPipeB );

  private:
	friend class Process;
#ifdef _WIN32
	AutoHandle m_hInput;
	AutoHandle m_hOutput;

	Pipe( AutoHandle&& readHandle, AutoHandle&& writeHandle );
#else
	AutoHandle m_handle;

	Pipe( AutoHandle&& handle );
#endif
};

}} // namespace EE::System
