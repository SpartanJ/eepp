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

/** Opt-in diagnostics (EEPP_ACCESSIBILITY_TRACE set): client activity transitions and native
 * client connections, printed to stderr. Only rare transitions consult it. */
bool isAccessibilityTraceEnabled();

class AccessibilityBackend {
  public:
	virtual ~AccessibilityBackend() = default;

	virtual bool isAvailable() const = 0;

	virtual bool isInitializationComplete() const { return true; }

	virtual bool hasActiveClients() const { return false; }

	virtual void update() {}

	virtual void onEvent( const AccessibilityPendingEvent& event ) = 0;

	/** An edit of a text element, delivered synchronously before its ValueChanged event.
	 * Returns false when the change was ignored, so it does not count toward the frame's budget
	 * of exact changes. Changes the manager suppresses never arrive here. */
	virtual bool onTextChanged( AccessibilityNodeRef /*ref*/,
								const AccessibilityTextChange& /*change*/ ) {
		return false;
	}

	/** Speaks a message through the platform announcement API. Called from update() on the UI
	 * thread, after the frame's events were delivered. */
	virtual void announce( const String& /*message*/, AccessibilityLive /*priority*/ ) {}

	/** Evicts native wrappers for a model source whose identity is no longer valid. */
	virtual void onSourceInvalidated( AccessibilitySourceId ) {}

	/** Prunes removed nodes while preserving wrappers for surviving persistent indexes. */
	virtual void onSourceChanged( AccessibilitySourceId ) {}
};

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager );

std::unique_ptr<AccessibilityBackend> createNullAccessibilityBackend();

}} // namespace EE::UI

#endif
