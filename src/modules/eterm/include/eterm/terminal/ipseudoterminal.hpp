#ifndef ETERM_IPSEUDOTERMINAL_HPP
#define ETERM_IPSEUDOTERMINAL_HPP
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
#include <eterm/system/ipipe.hpp>
#include <stdint.h>

namespace EE { namespace Terminal {

class IPseudoTerminal : public EE::System::IPipe {
  public:
	IPseudoTerminal() = default;

	IPseudoTerminal( IPseudoTerminal&& ) = delete;

	IPseudoTerminal( const IPseudoTerminal& ) = delete;

	IPseudoTerminal& operator=( IPseudoTerminal&& ) = delete;

	IPseudoTerminal& operator=( const IPseudoTerminal& ) = delete;

	virtual ~IPseudoTerminal() = default;

	virtual int getNumColumns() const = 0;

	virtual int getNumRows() const = 0;

	virtual bool resize( int columns, int rows ) = 0;
};

}} // namespace EE::Terminal

#endif
