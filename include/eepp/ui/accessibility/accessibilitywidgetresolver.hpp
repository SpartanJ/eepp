#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYWIDGETRESOLVER_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYWIDGETRESOLVER_HPP

#include <eepp/ui/accessibility/accessibility.hpp>

namespace EE { namespace Scene {
class Node;
}} // namespace EE::Scene

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

	static AccessibilityTextInfo getText( const UIWidget* widget );

	/** Metadata-only queries avoid scanning document lines to calculate offsets. */
	static AccessibilityTextInfo getText( const UIWidget* widget, bool includeOffsets );

	static AccessibilityState getState( const UIWidget* widget );

	static AccessibilityActions getActions( const UIWidget* widget );

	/** Returns the model view whose virtual nodes represent this widget, or nullptr when the
	 * widget is exposed directly: outside every model view, or inside its active cell editor. */
	static UIWidget* getOwningModelView( const Scene::Node* node );

	/** Projects the keyboard focus node onto the accessibility element that exposes it. Hidden
	 * implementation children (for example a spin box's input) resolve to their semantic owner. */
	static UIWidget* getFocusOwner( Scene::Node* focused, const UIWidget* sceneRoot );

	/** Routes internal control state changes to their semantic owner. */
	static UIWidget* getEventTarget( UIWidget* widget, AccessibilityEvent event );

	static bool performAction( UIWidget* widget, const AccessibilityActionRequest& request );
};

}} // namespace EE::UI

#endif
