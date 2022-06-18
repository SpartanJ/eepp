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

#include <memory>
#include <string>
#include <vector>

#include "../terminal/pseudoterminal.hpp"
#include "iprocess.hpp"

#ifndef WIN32
#include <sys/types.h>
#include <unistd.h>
#else
#define NTDDI_VERSION NTDDI_WIN10_RS5
#include <windows.h>
#endif

namespace EE { namespace System {

class Process final : public IProcess {
  private:
	ProcessStatus m_status;
	bool m_leaveRunning;
	int m_exitCode;
#ifdef WIN32
	AutoHandle m_hProcess;
	LPPROC_THREAD_ATTRIBUTE_LIST m_lpAttributeList;

	Process( AutoHandle&& processHandle, LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList );
#else
	int m_pid;

	Process( int pid );
#endif

  public:
	~Process();
	Process( const Process& ) = delete;
	Process( Process&& ) = delete;
	Process& operator=( const Process& ) = delete;
	Process& operator=( Process&& ) = delete;

	virtual void CheckExitStatus() override;
	virtual bool HasExited() const override;
	virtual int GetExitCode() const override;

	virtual void Terminate() override;
	virtual void WaitForExit() override;

	static std::unique_ptr<Process> CreateWithPipe( const std::string& program,
													const std::vector<std::string>& args,
													const std::string& workingDirectory,
													std::unique_ptr<IPipe>& outPipe,
													bool withStderr = true );

	static std::unique_ptr<Process>
	CreateWithPseudoTerminal( const std::string& program, const std::vector<std::string>& args,
							  const std::string& workingDirectory,
							  Terminal::PseudoTerminal& pseudoTerminal );
};

}} // namespace EE::System
