#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYWIDGETRESOLVER_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYWIDGETRESOLVER_HPP

#include <eepp/ui/accessibility/accessibility.hpp>

namespace EE { namespace Scene {
class Node;
}} // namespace EE::Scene

namespace EE { namespace UI { namespace Doc {
class TextDocument;
class TextPosition;
}}} // namespace EE::UI::Doc

namespace EE { namespace UI {

class UIWidget;

struct AccessibilityIssue {
	const UIWidget* widget{ nullptr };
	String message;
};

/** Resolves standard widget semantics directly from the live widget instance. */
class EE_API AccessibilityWidgetResolver {
  public:
	static AccessibilityRole getRole( const UIWidget* widget );

	/** A readable role name for tools and logs, for example "CheckBox". */
	static const char* getRoleName( AccessibilityRole role );

	/** Controls whose content is fully described by their own name, value and state. Their
	 * implementation children (a button's text view, a menu item's shortcut label) are not exposed.
	 */
	static bool isLeafRole( AccessibilityRole role );

	/** The outermost leaf control containing the node, or nullptr when no ancestor is a leaf. */
	static UIWidget* getLeafOwner( const Scene::Node* node );

	/** Whether the widget or any ancestor is hidden from accessibility (aria-hidden). */
	static bool isHiddenFromAccessibility( const UIWidget* widget );

	static String getName( const UIWidget* widget );

	static String getDescription( const UIWidget* widget );

	static String getValue( const UIWidget* widget );

	/** The keyboard shortcut of a menu item; empty for every other widget. */
	static AccessibilityShortcut getShortcut( const UIWidget* widget );

	static AccessibilityRangeInfo getRange( const UIWidget* widget );

	static AccessibilityTextInfo getText( const UIWidget* widget );

	/** Metadata-only queries avoid scanning document lines to calculate offsets. */
	static AccessibilityTextInfo getText( const UIWidget* widget, bool includeOffsets );

	/** The document behind a text element; nullptr for other widgets and for password inputs,
	 * whose text is never exposed. */
	static const Doc::TextDocument* getTextDocument( const UIWidget* widget );

	/** Ranged text access, in code points. None of these copy more than the requested range. */
	static Int32 getTextLength( const UIWidget* widget );

	static String getTextRange( const UIWidget* widget, Int32 start, Int32 end );

	/** The line containing `offset`: [start, end), end past its newline. */
	static bool getTextLineBounds( const UIWidget* widget, Int32 offset, Int32& start, Int32& end );

	static AccessibilityTextRevision getTextRevision( const UIWidget* widget );

	/** The code-point offset of a document position. */
	static Int32 getTextOffset( const Doc::TextDocument& document,
								const Doc::TextPosition& position );

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

	/** Lists common authoring problems in a widget tree: controls without an accessible name and
	 * aria-labelledby / aria-describedby ids that resolve to nothing. Hidden and invisible
	 * subtrees are skipped, as are the implementation children of model views. */
	static std::vector<AccessibilityIssue> audit( const UIWidget* root );

  private:
	/** The widget's own name: its label, else its native text, else its tooltip. Relations are
	 * not followed, so a labelled-by target that is itself labelled by another cannot loop. */
	static String getOwnName( const UIWidget* widget );
};

}} // namespace EE::UI

#endif
