#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYWIDGETRESOLVER_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYWIDGETRESOLVER_HPP

#include <eepp/ui/accessibility/accessibility.hpp>

namespace EE { namespace UI {

class UIWidget;

/** Resolves standard widget semantics directly from the live widget instance. */
class EE_API AccessibilityWidgetResolver {
  public:
	static AccessibilityRole getRole( const UIWidget* widget );

	static String getName( const UIWidget* widget );

	static String getDescription( const UIWidget* widget );

	static String getValue( const UIWidget* widget );

	static AccessibilityRangeInfo getRange( const UIWidget* widget );

	static AccessibilityState getState( const UIWidget* widget );

	static AccessibilityActions getActions( const UIWidget* widget );

	static bool performAction( UIWidget* widget, const AccessibilityActionRequest& request );
};

}} // namespace EE::UI

#endif
