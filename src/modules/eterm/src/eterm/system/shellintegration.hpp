#ifndef ETERM_SHELLINTEGRATION_HPP
#define ETERM_SHELLINTEGRATION_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace eterm { namespace System {

/** Internal shell startup preparation, called before spawning a PTY child. */
class ShellIntegration {
  public:
	/** Updates the child environment and returns owned replacement arguments, or an empty vector
	 * to retain the original arguments. All shell detection and startup policy lives here. */
	static std::vector<std::string>
	prepare( const std::string& executable, const std::vector<std::string>& arguments,
			 std::unordered_map<std::string, std::string>& environment );
};

}} // namespace eterm::System

#endif
