#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYMANAGER_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYMANAGER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/ui/accessibility/accessibility.hpp>
#include <memory>

namespace EE { namespace UI {

class UISceneNode;
class UIWidget;
class AccessibilityBackend;

/** Resolves accessibility information directly from the live UI tree. */
class EE_API AccessibilityManager {
  public:
	explicit AccessibilityManager( UISceneNode* scene );

	~AccessibilityManager();

	AccessibilityNodeRef getRoot();

	AccessibilityNodeRef getNodeRef( UIWidget* widget );

	bool isValid( AccessibilityNodeRef ref ) const;

	AccessibilityNodeInfo getNodeInfo( AccessibilityNodeRef ref ) const;

	AccessibilityNodeRef getParent( AccessibilityNodeRef ref );

	size_t getChildCount( AccessibilityNodeRef ref );

	AccessibilityNodeRef getChild( AccessibilityNodeRef ref, size_t index );

	AccessibilityNodeRef hitTest( const Math::Vector2f& screenPosition );

	AccessibilityNodeRef getKeyboardFocusedNode();

	bool performAction( AccessibilityNodeRef ref, const AccessibilityActionRequest& request );

	bool isBackendAvailable() const;

	bool hasActiveNativeClients() const;

	void update();

	UISceneNode* getSceneNode() const;

	void notify( AccessibilityNodeRef ref, AccessibilityEvent event );

	const std::vector<AccessibilityPendingEvent>& getPendingEvents() const;

	void clearPendingEvents();

	void onWidgetDelete( UIWidget* widget );

  private:
	static constexpr AccessibilitySourceId WidgetSource = 1;
	UISceneNode* mScene;
	std::unique_ptr<AccessibilityBackend> mBackend;
	Uint64 mNextId{ 1 };
	UnorderedMap<UIWidget*, Uint64> mWidgetIds;
	UnorderedMap<Uint64, UIWidget*> mWidgets;
	std::vector<AccessibilityPendingEvent> mPendingEvents;

	UIWidget* resolve( AccessibilityNodeRef ref ) const;
};

}} // namespace EE::UI

#endif
