#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYMANAGER_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYMANAGER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/ui/accessibility/accessibility.hpp>
#include <memory>

namespace EE { namespace UI {

class UISceneNode;
class UIWidget;
class AccessibilityBackend;
class AccessibilitySource;

/** Resolves accessibility information directly from the live UI tree. */
class EE_API AccessibilityManager {
  public:
	explicit AccessibilityManager( UISceneNode* scene );

	~AccessibilityManager();

	AccessibilityNodeRef getRoot();

	AccessibilityNodeRef getNodeRef( UIWidget* widget );

	bool isValid( AccessibilityNodeRef ref ) const;

	AccessibilityNodeInfo getNodeInfo( AccessibilityNodeRef ref, bool includeValue = true ) const;

	AccessibilityNodeRef getParent( AccessibilityNodeRef ref );

	size_t getChildCount( AccessibilityNodeRef ref );

	AccessibilityNodeRef getChild( AccessibilityNodeRef ref, size_t index );

	const std::vector<AccessibilityNodeRef>& getChildren( AccessibilityNodeRef ref );

	AccessibilityNodeRef hitTest( const Math::Vector2f& screenPosition );

	AccessibilityNodeRef getKeyboardFocusedNode();

	bool performAction( AccessibilityNodeRef ref, const AccessibilityActionRequest& request );

	bool isBackendAvailable() const;

	bool hasActiveNativeClients() const;

	/** Updates the scene's cached fast-path flag after an actual native accessibility query. */
	void onNativeClientObserved();

	void update();

	UISceneNode* getSceneNode() const;

	void notify( AccessibilityNodeRef ref, AccessibilityEvent event,
				 AccessibilityNodeRef related = {}, Int32 index = -1 );

	const std::vector<AccessibilityPendingEvent>& getPendingEvents() const;

	void clearPendingEvents();

	void onWidgetParentChange( UIWidget* widget );

	void onWidgetRemovedFromParent( UIWidget* widget );

	void onWidgetDelete( UIWidget* widget );

  private:
	static constexpr AccessibilitySourceId WidgetSource = 1;
	UISceneNode* mScene;
	std::unique_ptr<AccessibilityBackend> mBackend;
	Uint64 mNextId{ 1 };
	UnorderedMap<UIWidget*, Uint64> mWidgetIds;
	UnorderedMap<Uint64, UIWidget*> mWidgets;
	AccessibilitySourceId mNextSourceId{ WidgetSource + 1 };
	UnorderedMap<AccessibilitySourceId, std::unique_ptr<AccessibilitySource>> mSources;
	UnorderedMap<UIWidget*, AccessibilitySourceId> mWidgetSources;
	std::vector<AccessibilityPendingEvent> mPendingEvents;
	struct ChildrenCacheEntry {
		std::vector<AccessibilityNodeRef> children;
		AccessibilityNodeRef parent;
	};
	ChildrenCacheEntry mChildrenCache[2];
	Uint8 mMostRecentlyUsedChildrenCache{};

	UIWidget* resolve( AccessibilityNodeRef ref ) const;

	AccessibilitySource* resolveSource( AccessibilityNodeRef ref ) const;

	AccessibilitySource* sourceFor( UIWidget* widget );

	void invalidateChildren();
};

}} // namespace EE::UI

#endif
