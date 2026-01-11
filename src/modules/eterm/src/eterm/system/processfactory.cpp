#include <eterm/system/process.hpp>
#include <eterm/system/processfactory.hpp>
#include <eterm/terminal/pseudoterminal.hpp>

using namespace eterm::System;

namespace eterm { namespace System {

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
	std::unique_ptr<IPseudoTerminal>& outPseudoTerminal,
	const std::unordered_map<std::string, std::string>& env ) {
	outPseudoTerminal = nullptr;

	auto pseudoTerminal = Terminal::PseudoTerminal::create( numColumns, numRows );
	if ( !pseudoTerminal ) {
		fprintf( stderr,
				 "ProcessFactory::createWithPseudoTerminal: Failed to create pseudo terminal\n" );
		return nullptr;
	}

	auto process = System::Process::createWithPseudoTerminal( program, args, workingDirectory,
															  *pseudoTerminal, env );
	if ( !process ) {
		fprintf( stderr, "ProcessFactory::createWithPseudoTerminal: Failed to spawn process\n" );
		return nullptr;
	}

	outPseudoTerminal = std::move( pseudoTerminal );
	return process;
}

}} // namespace eterm::System
