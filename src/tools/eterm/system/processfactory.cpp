#include "../system/processfactory.hpp"
#include "../system/process.hpp"
#include "../terminal/pseudoterminal.hpp"

using namespace EE::System;

std::unique_ptr<IProcess> ProcessFactory::createWithStdioPipe( const std::string& program,
															   const std::vector<std::string>& args,
															   const std::string& workingDirectory,
															   std::unique_ptr<IPipe>& outPipe,
															   bool withStderr ) {
	return Process::createWithPipe( program, args, workingDirectory, outPipe, withStderr );
}

std::unique_ptr<IProcess> ProcessFactory::createWithPseudoTerminal(
	const std::string& program, const std::vector<std::string>& args,
	const std::string& workingDirectory, int numColumns, int numRows,
	std::unique_ptr<IPseudoTerminal>& outPseudoTerminal ) {
	outPseudoTerminal = nullptr;

	auto pseudoTerminal = Terminal::PseudoTerminal::create( numColumns, numRows );
	if ( !pseudoTerminal ) {
		fprintf( stderr, "Failed to create pseudo terminal\n" );
		return nullptr;
	}

	auto process = System::Process::createWithPseudoTerminal( program, args, workingDirectory,
															  *pseudoTerminal );
	if ( !process ) {
		fprintf( stderr, "Failed to spawn process\n" );
		return nullptr;
	}

	outPseudoTerminal = std::move( pseudoTerminal );
	return process;
}
