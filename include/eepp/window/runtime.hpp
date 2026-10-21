#ifndef EE_WINDOW_RUNTIME_HPP
#define EE_WINDOW_RUNTIME_HPP

#include <eepp/window/base.hpp>

namespace EE { namespace Window {

/** Process-level execution mode, independent of the selected WindowBackend. */
enum class RuntimeMode : Uint8 {
	Native,	  //!< Normal platform window and display presentation.
	Headless, //!< Offscreen rendering without presentation.
	Terminal  //!< Offscreen rendering presented through a terminal transport.
};

/** Process-level runtime selection and window visibility overrides. */
class EE_API Runtime {
  public:
	/** @return The process runtime mode resolved from EEPP_RUNTIME on first use. */
	static RuntimeMode mode();

	/** @return True when the runtime requires an offscreen video driver and logical framebuffer. */
	static bool isOffscreen();

	/** @return True when EEPP_WINDOW_HIDDEN is exactly "1". This does not change the runtime
	 * or renderer. Set it before creating windows to suppress native window presentation. */
	static bool forceHiddenWindows();

	/** @return A stable lowercase name for the resolved runtime mode. */
	static const char* modeName();
};

}} // namespace EE::Window

#endif
