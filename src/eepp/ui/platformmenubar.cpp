#include <eepp/ui/platformmenubar.hpp>

namespace EE { namespace UI {

#if EE_PLATFORM == EE_PLATFORM_MACOS
std::unique_ptr<PlatformMenuBar> createMacOSPlatformMenuBar();
#endif

bool PlatformMenuBar::isSupported() {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return true;
#else
	return false;
#endif
}

std::unique_ptr<PlatformMenuBar> PlatformMenuBar::create() {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return createMacOSPlatformMenuBar();
#else
	return nullptr;
#endif
}

}} // namespace EE::UI
