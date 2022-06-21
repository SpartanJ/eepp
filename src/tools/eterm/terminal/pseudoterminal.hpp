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

#include "../system/autohandle.hpp"
#include "ipseudoterminal.hpp"
#include <eepp/math/vector2.hpp>
#include <memory>

using namespace EE::Math;

namespace EE {
namespace System {
class Process;
}

namespace Terminal {
class PseudoTerminal final : public IPseudoTerminal {
  public:
	virtual ~PseudoTerminal();

	PseudoTerminal( PseudoTerminal&& ) = delete;

	PseudoTerminal( const PseudoTerminal& ) = delete;

	PseudoTerminal& operator=( PseudoTerminal&& ) = delete;

	PseudoTerminal& operator=( const PseudoTerminal& ) = delete;

	virtual bool isTTY() const override;

	virtual int getNumColumns() const override;
	virtual int getNumRows() const override;

	virtual bool resize( int columns, int rows ) override;
	virtual int write( const char* s, size_t n ) override;
	virtual int read( char* buf, size_t n, bool block = false ) override;

	static std::unique_ptr<PseudoTerminal> create( int columns, int rows );

  private:
	friend class ::EE::System::Process;
#ifdef _WIN32
	Vector2i m_size;

	AutoHandle m_hInput;
	AutoHandle m_hOutput;
	void* m_phPC;

	bool m_attached;

	PseudoTerminal( int columns, int rows, AutoHandle&& hInput, AutoHandle&& hOutput, void* hPC );
#else
	int m_columns;
	int m_rows;

	AutoHandle m_master;
	AutoHandle m_slave;

	PseudoTerminal( int columns, int rows, AutoHandle&& master, AutoHandle&& slave );
#endif
};
} // namespace Terminal
} // namespace EE
