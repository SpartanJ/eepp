#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKEND_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKEND_HPP

#include <eepp/ui/accessibility/accessibility.hpp>
#include <memory>

namespace EE { namespace UI {

class AccessibilityManager;

inline bool hasState( AccessibilityState states, AccessibilityState state ) {
	return ( static_cast<Uint64>( states ) & static_cast<Uint64>( state ) ) != 0;
}

inline bool hasAction( AccessibilityActions actions, AccessibilityAction action ) {
	return ( actions & accessibilityActionMask( action ) ) != 0;
}

class AccessibilityBackend {
  public:
	virtual ~AccessibilityBackend() = default;

	virtual bool isAvailable() const = 0;

	virtual bool isInitializationComplete() const { return true; }

	virtual bool hasActiveClients() const { return false; }

	virtual void update() {}

	virtual void onEvent( const AccessibilityPendingEvent& event ) = 0;

	/** Evicts native wrappers for a model source whose identity is no longer valid. */
	virtual void onSourceInvalidated( AccessibilitySourceId ) {}

	/** Prunes removed nodes while preserving wrappers for surviving persistent indexes. */
	virtual void onSourceChanged( AccessibilitySourceId ) {}
};

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager );

std::unique_ptr<AccessibilityBackend> createNullAccessibilityBackend();

}} // namespace EE::UI

#endif
