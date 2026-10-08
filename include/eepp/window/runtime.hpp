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

/** Process-level runtime selection, resolved once from EEPP_RUNTIME. */
class EE_API Runtime {
  public:
	/** @return The process runtime mode resolved from EEPP_RUNTIME on first use. */
	static RuntimeMode mode();

	/** @return True when the runtime requires an offscreen video driver and logical framebuffer. */
	static bool isOffscreen();

	/** @return A stable lowercase name for the resolved runtime mode. */
	static const char* modeName();
};

}} // namespace EE::Window

#endif
