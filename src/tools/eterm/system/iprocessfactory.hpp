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

#include "../terminal/ipseudoterminal.hpp"
#include "iprocess.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Hexe { namespace System {
class IProcessFactory {
  public:
	IProcessFactory() = default;
	virtual ~IProcessFactory() = default;
	IProcessFactory( const IProcessFactory& ) = delete;
	IProcessFactory( IProcessFactory&& ) = delete;
	IProcessFactory& operator=( const IProcessFactory& ) = delete;
	IProcessFactory& operator=( IProcessFactory&& ) = delete;

	virtual std::unique_ptr<IProcess> CreateWithStdioPipe( const std::string& program,
														   const std::vector<std::string>& args,
														   const std::string& workingDirectory,
														   std::unique_ptr<IPipe>& outPipe,
														   bool withStderr = true ) = 0;

	virtual std::unique_ptr<IProcess> CreateWithPseudoTerminal(
		const std::string& program, const std::vector<std::string>& args,
		const std::string& workingDirectory, int numColumns, int numRows,
		std::unique_ptr<Hexe::Terminal::IPseudoTerminal>& outPseudoTerminal ) = 0;
};

}} // namespace Hexe::System
