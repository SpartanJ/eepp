#include "codexinvocation.hpp"
#include <algorithm>
#include <string_view>

namespace eterm {

bool CodexInvocation::interactive( const std::vector<std::string>& arguments ) {
	bool selection = false;
	bool prompt = false;
	for ( size_t i = 0; i < arguments.size(); ++i ) {
		const std::string_view arg = arguments[i];
		if ( arg == "--" )
			return true; // Explicit prompt/positional boundary.
		if ( arg.empty() || arg.front() != '-' ) {
			if ( !selection && !prompt && ( arg == "resume" || arg == "fork" ) ) {
				selection = true;
				continue;
			}
			// A selection's positional argument is an ID/name, never another subcommand.
			// Subcommand names never contain whitespace, so such a word is an initial prompt; the
			// options after it still decide. Other bare words are ambiguous and deliberately
			// passed through; prompts can use --.
			if ( !selection ) {
				if ( prompt || arg.find_first_of( " \t\r\n" ) == std::string_view::npos )
					return false;
				prompt = true;
			}
			continue;
		}
		const auto equals = arg.find( '=' );
		const auto flag = arg.substr( 0, equals );
		constexpr std::string_view Values[]{ "-c",		  "--config",  "-m",
											 "--model",	  "-p",		   "--profile",
											 "-C",		  "--cd",	   "--add-dir",
											 "-i",		  "--image",   "-s",
											 "--sandbox", "-a",		   "--ask-for-approval",
											 "--enable",  "--disable", "--local-provider" };
		constexpr std::string_view Flags[]{
			"--no-daemon",	   "--oss",
			"--full-auto",	   "--dangerously-bypass-approvals-and-sandbox",
			"--no-alt-screen", "--search",
			"--last",		   "--all" };
		if ( std::find( std::begin( Values ), std::end( Values ), flag ) != std::end( Values ) ) {
			if ( equals == std::string_view::npos && ++i >= arguments.size() )
				return false;
		} else if ( std::find( std::begin( Flags ), std::end( Flags ), flag ) ==
						std::end( Flags ) ||
					equals != std::string_view::npos ) {
			return false;
		}
	}
	return true;
}

bool CodexInvocation::relay( const std::vector<std::string>& arguments ) {
	if ( !interactive( arguments ) )
		return false;
	return std::find( arguments.begin(), arguments.end(), "--no-daemon" ) == arguments.end();
}

} // namespace eterm
