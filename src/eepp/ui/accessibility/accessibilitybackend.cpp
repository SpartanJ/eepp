#include "accessibilitybackend.hpp"

namespace EE { namespace UI {

class NullAccessibilityBackend final : public AccessibilityBackend {
  public:
	bool isAvailable() const { return false; }

	void onEvent( const AccessibilityPendingEvent& ) {}
};

std::unique_ptr<AccessibilityBackend> createNullAccessibilityBackend() {
	return std::make_unique<NullAccessibilityBackend>();
}

#if EE_PLATFORM != EE_PLATFORM_LINUX && EE_PLATFORM != EE_PLATFORM_FREEBSD && \
	EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_MACOS
std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& ) {
	return createNullAccessibilityBackend();
}
#endif

}} // namespace EE::UI
