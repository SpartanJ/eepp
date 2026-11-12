#ifndef ETERM_CODEXINVOCATION_HPP
#define ETERM_CODEXINVOCATION_HPP
#include <cstddef>
#include <string>
#include <vector>

namespace eterm {

/** Conservative CLI classification shared by hooks and the daemon wrapper. Unknown options or
 * commands pass through unchanged, so adding a Codex subcommand cannot turn it into a TUI. */
class CodexInvocation {
  public:
	/** If requested, directoryOption receives the parsed -C/--cd option's index, or npos.
	 * Option values and positional prompts are never mistaken for directory options. */
	static bool interactive( const std::vector<std::string>& arguments,
							 size_t* directoryOption = nullptr );

	static bool relay( const std::vector<std::string>& arguments,
					   size_t* directoryOption = nullptr );
};

} // namespace eterm

#endif
