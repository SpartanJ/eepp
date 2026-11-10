#pragma once

#include <eepp/core/containers.hpp>
#include <eepp/thirdparty/nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace eterm {

namespace UI {

class UITerminal;

} // namespace UI

using namespace EE;
using namespace UI;

/** Optional local shell recovery. Records are read only while saving the window; no
 * polling or transcript parsing is performed. Each launch gets a private, disposable hook
 * directory. */
class HarnessSessionRecovery {
  public:
	~HarnessSessionRecovery();

	/** Dispatch before constructing App. Hook mode is silent and reads bounded JSON from stdin. */
	static std::optional<int> command( int argc, char* argv[] );

	/** Shared metadata operations for hooks and daemon thread-selection responses. */
	static nlohmann::json descriptor( const std::string& harness, const nlohmann::json& payload,
									  const std::string& home );

	static void publish( const std::string& directory, const nlohmann::json& record );

	/** Remove inherited pane state, retaining the user's unrelated Claude plugins. */
	static void resetEnvironment( std::unordered_map<std::string, std::string>& environment );

	static std::string prepare( std::unordered_map<std::string, std::string>& environment,
								const nlohmann::json& resume );

	void track( const UITerminal* terminal, std::string directory );

	void remove( const UITerminal* terminal );

	nlohmann::json capture( const UITerminal* terminal ) const;

  private:
	UnorderedMap<const UITerminal*, std::string> mDirectories;
};

} // namespace eterm
