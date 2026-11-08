#pragma once

#include <string>
#include <vector>

namespace eterm {

/** Per-invocation, byte-preserving relay to the existing local Codex daemon.
 * Owns only its private listener, connections and foreground TUI; never stops the daemon. */
class CodexSessionRelay {
  public:
	static int run( const std::vector<std::string>& arguments );
};

} // namespace eterm
