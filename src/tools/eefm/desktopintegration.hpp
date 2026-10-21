#ifndef EEFM_DESKTOPINTEGRATION_HPP
#define EEFM_DESKTOPINTEGRATION_HPP

#include <eepp/config.hpp>
#include <string>

namespace eefm {

/** eefm's terminal preference and executable-file policy. */
class DesktopIntegration {
  public:
	enum class ExecutableKind : EE::Uint8 {
		/** Open with an application. */
		None,
		/** A native executable (ELF / Mach-O) the user may execute. */
		Program,
		/** An executable text file with an interpreter line ("#!"). */
		Script
	};

	/** Opens eterm when available, then the user's terminal emulator, in directory. */
	static bool openTerminal( const std::string& directory );

	/** Inspects permissions and the first bytes of a local file. Call off the UI thread.
	 * Always None on Windows, where opening a program runs it. */
	static ExecutableKind executableKind( const std::string& path );
};

} // namespace eefm

#endif
