#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYMANAGER_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYMANAGER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/ui/accessibility/accessibility.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <memory>

namespace EE { namespace Scene {
class Node;
}} // namespace EE::Scene

namespace EE { namespace UI { namespace Doc {
struct DocumentContentChange;
class TextDocument;
}}} // namespace EE::UI::Doc

namespace EE { namespace UI {

class UISceneNode;
class UIWidget;
class AccessibilityBackend;
class AccessibilitySource;
class AccessibilityTextIndex;
class AccessibilityTextIndexes;

/** Resolves accessibility information directly from the live UI tree. */
class EE_API AccessibilityManager {
  public:
	explicit AccessibilityManager( UISceneNode* scene );

	~AccessibilityManager();

	AccessibilityNodeRef getRoot();

	AccessibilityNodeRef getNodeRef( UIWidget* widget );

	/** The widget's identity if one was handed out, without registering one. Clients only hold
	 * handed-out identities, so property changes of other widgets need no notification. */
	AccessibilityNodeRef findNodeRef( UIWidget* widget ) const;

	bool isValid( AccessibilityNodeRef ref ) const;

	/** Scalar queries avoid constructing names, values, text offsets or relation caches. */
	AccessibilityRole getNodeRole( AccessibilityNodeRef ref ) const;

	AccessibilityState getNodeState( AccessibilityNodeRef ref ) const;

	AccessibilityNodeInfo getNodeInfo( AccessibilityNodeRef ref, bool includeValue = true ) const;

	/** Native metadata queries may also omit document-offset calculation. */
	AccessibilityNodeInfo getNodeInfo( AccessibilityNodeRef ref, bool includeValue,
									   bool includeTextOffsets ) const;

	AccessibilityNodeRef getParent( AccessibilityNodeRef ref );

	Int32 getIndexInParent( AccessibilityNodeRef ref );

	AccessibilityTextInfo getTextInfo( AccessibilityNodeRef ref ) const;

	/** Ranged text access in code points. Prefer these over getNodeInfo()'s value for text
	 * elements: they never copy more than the requested range. */
	Int32 getTextLength( AccessibilityNodeRef ref ) const;

	String getTextRange( AccessibilityNodeRef ref, Int32 start, Int32 end ) const;

	/** The line containing `offset`: [start, end), end past its newline. */
	bool getTextLineBounds( AccessibilityNodeRef ref, Int32 offset, Int32& start,
							Int32& end ) const;

	AccessibilityTextRevision getTextRevision( AccessibilityNodeRef ref ) const;

	/** Reports an edit of a text widget's document, or nullptr when its whole text was replaced.
	 * Only backends requesting edit records receive them. The first few edits per widget and
	 * frame are forwarded exactly; later ones collapse into a single whole-text change, so a
	 * replace-all costs one notification. */
	void onTextChanged( UIWidget* widget, const Doc::DocumentContentChange* change );

	/** While set, text changes of this element are not forwarded: the caller reports them itself
	 * (AT-SPI SetTextContents sends one minimal diff). Node refs are local to this manager, so
	 * another window's element sharing the document still reports its changes. Pass an invalid
	 * ref to clear. */
	void setSuppressedTextChanges( AccessibilityNodeRef ref );

	size_t getChildCount( AccessibilityNodeRef ref );

	AccessibilityNodeRef getChild( AccessibilityNodeRef ref, size_t index );

	const std::vector<AccessibilityNodeRef>& getChildren( AccessibilityNodeRef ref );

	std::vector<AccessibilityNodeRef> getSelectedChildren( AccessibilityNodeRef ref );

	AccessibilityNodeRef hitTest( const Math::Vector2f& screenPosition );

	AccessibilityNodeRef getKeyboardFocusedNode();

	bool performAction( AccessibilityNodeRef ref, const AccessibilityActionRequest& request );

	bool isBackendAvailable() const;

	bool isBackendInitializationComplete() const;

	bool hasActiveNativeClients() const;

	/** Updates the scene's cached fast-path flag after an actual native accessibility query. */
	void onNativeClientObserved();

	/** The scene's last client went away: releases what only clients needed, such as queried
	 * model rows and their persistent model registrations. Identities are never reused, so a
	 * reconnecting client cannot reach a stale row. */
	void onClientsDisconnected();

	void update();

	/** Replaces the platform backend, which update() otherwise creates on first use. Intended
	 * for tests that observe what the manager delivers. */
	void setBackend( std::unique_ptr<AccessibilityBackend> backend );

	UISceneNode* getSceneNode() const;

	void notify( AccessibilityNodeRef ref, AccessibilityEvent event,
				 AccessibilityNodeRef related = {}, Int32 index = -1 );

	const std::vector<AccessibilityPendingEvent>& getPendingEvents() const;

	void clearPendingEvents();

	/** Asks assistive clients to speak a message without moving focus. Delivered on the next
	 * update(); a no-op while no client is active. */
	void announce( const String& message, AccessibilityLive priority = AccessibilityLive::Polite );

	/** Queues the current text of a widget inside a live region, replacing any announcement the
	 * same widget queued earlier in this frame. */
	void onLiveRegionChanged( UIWidget* widget, AccessibilityLive priority );

	const std::vector<AccessibilityAnnouncement>& getPendingAnnouncements() const;

	/** The widget with this id changed its name, appeared or went away. Widgets that a client
	 * read and that are named or described by it (aria-labelledby, aria-describedby) report
	 * NameChanged or DescriptionChanged. Dependents inside `excluded` are skipped. */
	void onRelationTargetChanged( const std::string& id, const Scene::Node* excluded = nullptr );

	/** Whether an active client may hold a name or description resolved through the id with this
	 * hash, so its dependents must be told when that id goes away. Allocation-free; it matches
	 * exactly what onRelationTargetChanged() would act on. */
	bool isRelationTarget( String::HashType idHash ) const;

	/** A subtree appeared or is going away: notifies the dependents of each relation target in
	 * it, except those inside the subtree when it is going away. */
	void onRelationSubtreeChanged( const Scene::Node* subtree, bool removed );

	/** Reports a reparent/reorder edge for the exposed roots of a widget or plain-node subtree.
	 * Removal is reported while the old parent links are intact; arrival after final relinking. */
	void onNodeParentChange( Scene::Node* node, bool removed );

	/** Returns whether the widget owned a model source that has now been released. */
	bool onWidgetAccessibilitySourceDelete( UIWidget* widget );

	/** Empty and native-root-only managers skip invalidation inline, without a tree walk.
	 * Explicit semantic queries can retain descendants/text indexes without a native client:
	 * those still require invalidation. Active root relations also require the full path. */
	void onWidgetDelete( UIWidget* widget ) {
		if ( needsWidgetInvalidation( widget ) )
			invalidateWidget( widget );
	}

	/** Invalidates registered identities before a subtree changes accessibility owners. */
	void onSubtreeRemoved( Scene::Node* node );

	/** A callback may request a policy change, but cannot destroy its own native call stack.
	 * Native backends use this allocation-free scope around incoming calls. The scene applies
	 * the last request at the beginning of a later, non-reentrant update. */
	class ScopedDispatch {
	  public:
		explicit ScopedDispatch( AccessibilityManager& manager ) :
			mDepth( manager.mDispatchDepth ) {
			++mDepth;
		}

		~ScopedDispatch() { --mDepth; }

		ScopedDispatch( const ScopedDispatch& ) = delete;

		ScopedDispatch& operator=( const ScopedDispatch& ) = delete;

	  private:
		Uint32& mDepth;
	};

  private:
	friend class UISceneNode;
	friend class AccessibilityWidgetResolver;
	friend class UIWidget;

	/** Shared by normal invalidation and destruction, which checks this before querying shutdown
	 * state. Do not use client activity alone: explicit semantic queries can retain identities. */
	bool needsWidgetInvalidation( UIWidget* widget ) const {
		if ( !mTextIndexes && mWidgetSources.empty() ) {
			if ( mWidgetIds.empty() )
				return false;
			if ( mWidgetIds.size() == 1 &&
				 ( mRelationTargets.empty() || !mScene->hasActiveAccessibilityClients() ) ) {
				const auto* root = mScene->getRoot();
				if ( widget != root && mWidgetIds.begin()->first == root )
					return false;
			}
		}
		return true;
	}

	static constexpr AccessibilitySourceId WidgetSource = 1;
	UISceneNode* mScene;
	std::unique_ptr<AccessibilityBackend> mBackend;
	/** Allocated only by a text query/event that needs offsets. Shared documents use one index
	 * in this manager; ordinary edits update existing indexes even when native edits are muted. */
	mutable std::unique_ptr<AccessibilityTextIndexes> mTextIndexes;
	Uint64 mNextId{ 1 };
	UnorderedMap<UIWidget*, Uint64> mWidgetIds;
	UnorderedMap<Uint64, UIWidget*> mWidgets;
	AccessibilitySourceId mNextSourceId{ WidgetSource + 1 };
	UnorderedMap<AccessibilitySourceId, std::unique_ptr<AccessibilitySource>> mSources;
	UnorderedMap<UIWidget*, AccessibilitySourceId> mWidgetSources;
	std::vector<AccessibilityPendingEvent> mPendingEvents;
	std::vector<AccessibilityAnnouncement> mPendingAnnouncements;
	/** Edits forwarded this frame per text element; reset by update(). */
	std::vector<std::pair<AccessibilityNodeRef, Uint32>> mTextChangeCounts;
	static constexpr Uint32 MaxExactTextChangesPerFrame = 4;
	AccessibilityNodeRef mSuppressedTextRef;
	/** Ids referenced by aria-labelledby / aria-describedby of widgets a client has read. Only
	 * changes to these widgets can make a client's cached name or description stale. Ids, not
	 * widget pointers: entries never dangle, and a stale one costs one scene scan. */
	mutable UnorderedSet<String::HashType> mRelationTargets;
	struct ChildrenCacheEntry {
		std::vector<AccessibilityNodeRef> children;
		AccessibilityNodeRef parent;
	};
	ChildrenCacheEntry mChildrenCache[2];
	/** The last exposed widget reported as added, its semantic parent and index. A widget
	 * added right after it takes the next index without rescanning its siblings, so populating
	 * a container stays linear. Shares the children cache's validity: invalidateChildren()
	 * clears it, and only a change under another parent restores it. */
	AccessibilityNodeRef mLastAddedParent;
	UIWidget* mLastAddedWidget{ nullptr };
	Int32 mLastAddedIndex{ -1 };
	Uint8 mMostRecentlyUsedChildrenCache{};
	/** Last policy requested during native dispatch; replacing the backend must wait until
	 * its current call stack has unwound. These fields fit the manager's existing tail padding. */
	AccessibilityPolicy mDeferredPolicy{ AccessibilityPolicy::Auto };
	/** Whether a callback queued a policy request, including one that cancels an earlier request
	 * by restoring the current policy. Cleared when the scene applies or supersedes it. */
	bool mHasDeferredPolicy{ false };
	/** An indexed document received ambiguous edits this frame. Only a frame boundary clears
	 * this, not a query/rebuild, so every editor sharing it uses the same safe fallback. */
	bool mHasAmbiguousTextChanges{ false };
	/** Nesting count of guarded queries, actions, notifications and backend updates. Nonzero
	 * prevents policy-driven destruction and recursive backend dispatch from user callbacks. */
	Uint32 mDispatchDepth{};

	UIWidget* resolve( AccessibilityNodeRef ref ) const;

	AccessibilityTextIndex& textIndexFor( const UIWidget* widget,
										  const Doc::TextDocument& document ) const;

	AccessibilitySource* resolveSource( AccessibilityNodeRef ref ) const;

	AccessibilitySource* sourceFor( UIWidget* widget );

	void invalidateChildren();

	AccessibilityNodeRef getWidgetParent( Scene::Node* node );

	/** Reports the exposed roots of an arriving/departing transparent subtree. */
	void onSemanticSubtreeChange( Scene::Node* node, bool removed );

	/** Returns whether any identity or model source in the subtree was released. */
	bool removeSubtreeIdentities( Scene::Node* node );

	void invalidateWidget( UIWidget* widget );

	/** Whether a client can receive events: without one, only invalidation runs. */
	bool hasActiveClients() const;
};

}} // namespace EE::UI

#endif
