#ifndef ETERM_CODEXINVOCATION_HPP
#define ETERM_CODEXINVOCATION_HPP
#include <string>
#include <vector>

namespace eterm {

/** Conservative CLI classification shared by hooks and the daemon wrapper. Unknown options or
 * commands pass through unchanged, so adding a Codex subcommand cannot turn it into a TUI. */
class CodexInvocation {
  public:
	static bool interactive( const std::vector<std::string>& arguments );

	static bool relay( const std::vector<std::string>& arguments );
};

} // namespace eterm

#endif
