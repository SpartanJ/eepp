#ifndef ETERM_PROCESSSTATUS_HPP
#define ETERM_PROCESSSTATUS_HPP
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

namespace EE { namespace System {

enum class ProcessStatus { RUNNING = 0, EXITED };

class IProcess {
  public:
	IProcess() = default;

	virtual ~IProcess() = default;

	IProcess( const IProcess& ) = delete;

	IProcess( IProcess&& ) = delete;

	IProcess& operator=( const IProcess& ) = delete;

	IProcess& operator=( IProcess&& ) = delete;

	virtual void checkExitStatus() = 0;

	virtual bool hasExited() const = 0;

	virtual int getExitCode() const = 0;

	virtual void terminate() = 0;

	virtual void waitForExit() = 0;
};

}} // namespace EE::System

#endif
