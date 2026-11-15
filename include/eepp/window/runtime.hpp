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

/** Process-level runtime selection and window creation overrides. */
class EE_API Runtime {
  public:
	/** @return The process runtime mode resolved from EEPP_RUNTIME on first use. */
	static RuntimeMode mode();

	/** @return True when the runtime requires an offscreen video driver and logical framebuffer. */
	static bool isOffscreen();

	/** @return True when EEPP_WINDOW_HIDDEN is exactly "1". This does not change the runtime
	 * or renderer. Set it before creating windows to suppress native window presentation. */
	static bool forceHiddenWindows();

	/** @return True when EEPP_FORCE_SOFTWARE_RENDERING is nonempty and not exactly "0".
	 * Requests a software GL visual before window creation in the SDL backends. */
	static bool forceSoftwareRendering();

	/** @return The requested multisample count overridden by EEPP_MSAA when it contains a
	 * nonnegative decimal integer representable by the backend's signed 32-bit sample count.
	 * Zero disables MSAA; unset, empty, or invalid values preserve the requested count. */
	static Uint32 resolveMultisamples( Uint32 requested );

	/** @return A stable lowercase name for the resolved runtime mode. */
	static const char* modeName();
};

}} // namespace EE::Window

#endif
