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
#include <cstdint>
#include <eterm/system/ipipe.hpp>
#include <stdint.h>

namespace eterm { namespace Terminal {

class IPseudoTerminal : public eterm::System::IPipe {
  public:
	IPseudoTerminal() = default;

	IPseudoTerminal( IPseudoTerminal&& ) = delete;

	IPseudoTerminal( const IPseudoTerminal& ) = delete;

	IPseudoTerminal& operator=( IPseudoTerminal&& ) = delete;

	IPseudoTerminal& operator=( const IPseudoTerminal& ) = delete;

	virtual ~IPseudoTerminal() = default;

	virtual int getNumColumns() const = 0;

	virtual int getNumRows() const = 0;

	virtual bool resize( int columns, int rows, int pixelWidth, int pixelHeight ) = 0;

	bool resize( int columns, int rows ) { return resize( columns, rows, 0, 0 ); }

	/** What the session waits on for output, or -1 to poll on a timer: on POSIX a descriptor for
	 * poll() (readable for output, writable for pending input); on Windows an event HANDLE that is
	 * signaled while output is buffered. */
	virtual std::intptr_t getWaitHandle() const { return -1; }

	/** POSIX: input is buffered until the descriptor becomes writable (read() flushes it). */
	virtual bool hasPendingWrites() const { return false; }

	/** Starts a terminal-generated reply of the given size, which is written whole until
	 * endReply(), or dropped whole if this returns false. Replies are bounded because a child can
	 * request them forever without reading; user input is not. */
	virtual bool beginReply( size_t /*size*/ ) { return true; }

	virtual void endReply() {}

	/** Windows: no more output can arrive and none is left to read. POSIX reports a hangup
	 * through poll(). */
	virtual bool hasHungUp() const { return false; }
};

}} // namespace eterm::Terminal

#endif
