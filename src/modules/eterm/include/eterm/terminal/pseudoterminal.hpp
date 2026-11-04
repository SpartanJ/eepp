#ifndef ETERM_PSEUDOTERMINAL_HPP
#define ETERM_PSEUDOTERMINAL_HPP
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
#include <eepp/math/vector2.hpp>
#include <eterm/system/autohandle.hpp>
#include <eterm/terminal/ipseudoterminal.hpp>
#include <eterm/terminal/ptyoutputqueue.hpp>
#include <memory>
#ifndef _WIN32
#include <deque>
#endif

using namespace EE;
using namespace EE::Math;
using namespace eterm::System;

namespace eterm {

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

	virtual bool resize( int columns, int rows, int pixelWidth, int pixelHeight ) override;
	virtual int write( const char* s, size_t n ) override;
	virtual int read( char* buf, size_t n, bool block = false ) override;

#ifdef _WIN32
	virtual std::intptr_t getWaitHandle() const override;

	virtual bool hasHungUp() const override;
#else
	virtual std::intptr_t getWaitHandle() const override { return mMaster.handle(); }

	virtual bool hasPendingWrites() const override { return mWriteBuffer.pending() != 0; }

	virtual bool beginReply( size_t size ) override;

	virtual void endReply() override;

	/** Most bytes of replies that can wait for the child to read. Input does not count. */
	static constexpr size_t ReplyBudget = 4 * 1024 * 1024;
#endif

	static std::unique_ptr<PseudoTerminal> create( int columns, int rows );

  private:
	friend class ::eterm::System::Process;
#ifdef _WIN32
	Vector2i mSize;

	AutoHandle mInputHandle;
	AutoHandle mOutputHandle;
	void* mPHPC;

	bool mAttached;

	/* Drains the ConPTY output pipe on its own thread, which anonymous pipes need to be waited
	 * on: they cannot be read asynchronously. */
	std::unique_ptr<struct PseudoTerminalReader> mReader;

	PseudoTerminal( int columns, int rows, AutoHandle&& hInput, AutoHandle&& hOutput,
					void* hPC ) noexcept;
#else
	int mColumns;
	int mRows;

	AutoHandle mMaster;
	AutoHandle mSlave;

	/** A run of replies in mWriteBuffer, as offsets into everything ever queued. */
	struct ReplySpan {
		Uint64 start;
		Uint64 end;
	};

	PtyOutputQueue mWriteBuffer;
	Uint64 mBytesQueued{ 0 };
	Uint64 mBytesDelivered{ 0 };
	std::deque<ReplySpan> mReplySpans;
	size_t mPendingReplyBytes{ 0 };
	Uint64 mReplyStart{ 0 };

	void queueWrite( const char* data, size_t size );

	void deliverQueuedWrite( size_t size );

	PseudoTerminal( int columns, int rows, AutoHandle&& master, AutoHandle&& slave ) noexcept;
#endif
};
} // namespace Terminal
} // namespace eterm

#endif
