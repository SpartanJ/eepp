#ifndef ETERM_PROCESS_HPP
#define ETERM_PROCESS_HPP
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
#include <memory>
#include <string>
#include <vector>

#include <eterm/system/iprocess.hpp>
#include <eterm/terminal/pseudoterminal.hpp>

namespace eterm { namespace System {

class Process final : public IProcess {
  public:
	~Process();

	Process( const Process& ) = delete;

	Process( Process&& ) = delete;

	Process& operator=( const Process& ) = delete;

	Process& operator=( Process&& ) = delete;

	virtual void checkExitStatus() override;

	virtual bool hasExited() const override;

	virtual int getExitCode() const override;

	virtual void terminate() override;

	virtual void waitForExit() override;

	virtual int pid() override;

	static std::unique_ptr<Process> createWithPipe( const std::string& program,
													const std::vector<std::string>& args,
													const std::string& workingDirectory,
													std::unique_ptr<IPipe>& outPipe,
													bool withStderr = true );

	static std::unique_ptr<Process>
	createWithPseudoTerminal( const std::string& program, const std::vector<std::string>& args,
							  const std::string& workingDirectory,
							  Terminal::PseudoTerminal& pseudoTerminal );

  private:
	ProcessStatus mStatus{ ProcessStatus::RUNNING };
	int mExitCode{ 1 };
#ifdef _WIN32
	bool mLeaveRunning{ false };
	AutoHandle mProcessHandle;
	void* mLpAttributeList;
	int mPID;

	Process( AutoHandle&& processHandle, void* lpAttributeList, int pid );
#else
	int mPID;

	Process( int pid );
#endif
};

}} // namespace EE::System

#endif
