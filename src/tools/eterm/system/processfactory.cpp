#include "../system/processfactory.hpp"
#include "../system/process.hpp"
#include "../terminal/pseudoterminal.hpp"

using namespace Hexe::System;

std::unique_ptr<IProcess> ProcessFactory::CreateWithStdioPipe( const std::string& program,
															   const std::vector<std::string>& args,
															   const std::string& workingDirectory,
															   std::unique_ptr<IPipe>& outPipe,
															   bool withStderr ) {
	return Process::CreateWithPipe( program, args, workingDirectory, outPipe, withStderr );
}

std::unique_ptr<IProcess> ProcessFactory::CreateWithPseudoTerminal(
	const std::string& program, const std::vector<std::string>& args,
	const std::string& workingDirectory, int numColumns, int numRows,
	std::unique_ptr<Hexe::Terminal::IPseudoTerminal>& outPseudoTerminal ) {
	outPseudoTerminal = nullptr;

	auto pseudoTerminal = Terminal::PseudoTerminal::Create( numColumns, numRows );
	if ( !pseudoTerminal ) {
		fprintf( stderr, "Failed to create pseudo terminal\n" );
		return nullptr;
	}

	auto process = System::Process::CreateWithPseudoTerminal( program, args, workingDirectory,
															  *pseudoTerminal );
	if ( !process ) {
		fprintf( stderr, "Failed to spawn process\n" );
		return nullptr;
	}

	outPseudoTerminal = std::move( pseudoTerminal );
	return process;
}
