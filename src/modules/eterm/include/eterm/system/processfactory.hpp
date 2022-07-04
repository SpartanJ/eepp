#ifndef ETERM_PROCESSFACTORY_HPP
#define ETERM_PROCESSFACTORY_HPP
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
#include <eterm/system/iprocessfactory.hpp>

using namespace eterm::System;
using namespace eterm::Terminal;

namespace eterm { namespace System {

class ProcessFactory : public IProcessFactory {
  public:
	ProcessFactory() = default;

	virtual ~ProcessFactory() = default;

	ProcessFactory( const ProcessFactory& ) = delete;

	ProcessFactory( ProcessFactory&& ) = delete;

	ProcessFactory& operator=( const ProcessFactory& ) = delete;

	ProcessFactory& operator=( ProcessFactory&& ) = delete;

	virtual std::unique_ptr<IProcess> createWithStdioPipe( const std::string& program,
														   const std::vector<std::string>& args,
														   const std::string& workingDirectory,
														   std::unique_ptr<IPipe>& outPipe,
														   bool withStderr = true ) override;

	virtual std::unique_ptr<IProcess>
	createWithPseudoTerminal( const std::string& program, const std::vector<std::string>& args,
							  const std::string& workingDirectory, int numColumns, int numRows,
							  std::unique_ptr<IPseudoTerminal>& outPseudoTerminal ) override;
};

}} // namespace EE::System

#endif
