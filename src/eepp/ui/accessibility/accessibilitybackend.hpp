#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKEND_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKEND_HPP

#include <eepp/ui/accessibility/accessibility.hpp>
#include <memory>

namespace EE { namespace UI {

class AccessibilityManager;

class AccessibilityBackend {
  public:
	virtual ~AccessibilityBackend() = default;

	virtual bool isAvailable() const = 0;

	virtual bool hasActiveClients() const { return false; }

	virtual void update() {}

	virtual void onEvent( const AccessibilityPendingEvent& event ) = 0;

	/** Evicts native wrappers for a model source whose identity is no longer valid. */
	virtual void onSourceInvalidated( AccessibilitySourceId ) {}
};

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager );

std::unique_ptr<AccessibilityBackend> createNullAccessibilityBackend();

}} // namespace EE::UI

#endif
