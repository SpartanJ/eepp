#ifndef EPROC_PROCESS_ICON_RESOLVER_MACOS_HPP
#define EPROC_PROCESS_ICON_RESOLVER_MACOS_HPP

#include <string>
#include <unordered_map>

namespace eproc {

/** Finds an application bundle's declared icon from its executable path. */
class ProcessIconResolverMacOS {
  public:
	const std::string& resolve( const std::string& executablePath );

  private:
	std::unordered_map<std::string, std::string> mIcons;
};

} // namespace eproc

#endif // EPROC_PROCESS_ICON_RESOLVER_MACOS_HPP
