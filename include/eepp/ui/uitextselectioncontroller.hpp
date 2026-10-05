#ifndef EE_UI_UITEXTSELECTIONCONTROLLER_HPP
#define EE_UI_UITEXTSELECTIONCONTROLLER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/scene/event.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/scene/nodemessage.hpp>
#include <functional>

namespace EE { namespace UI {

using namespace EE::Math;
using namespace EE::Scene;

class UIWidget;
class UIRichText;
class UIHTMLWidget;
class UIScrollView;
class UIPopUpMenu;

/** Coordinates document ordering, input, and CSS selection policy across RichText paint owners.
 * Each owner still handles local character geometry and selection painting. */
class EE_API UITextSelectionController {
  public:
	struct Point {
		UIRichText* owner{ nullptr };
		Int64 offset{ 0 };

		bool isValid() const { return owner != nullptr; }
	};

	struct Selection {
		Point anchor;
		Point focus;

		bool isValid() const { return anchor.isValid() && focus.isValid(); }
	};

	explicit UITextSelectionController( UIWidget* host = nullptr );

	~UITextSelectionController();

	void setHost( UIWidget* host );

	void setSelectionRoot( UIWidget* root );

	UIWidget* getSelectionRoot() const { return mRoot; }

	const Selection& getSelection() const { return mRawSelection; }

	bool isSelecting() const { return mDragging; }

	bool hasSelection() const;

	void clear();

	void selectAll();

	void setSelection( Point anchor, Point focus );

	String getSelectionString() const;

	bool copySelection();

	/** Overrides URL resolution for Copy Link. By default, the link's scene resolves its href. */
	void setLinkResolverCb( std::function<std::string( const std::string& )> cb );

	bool onKeyDown( const KeyEvent& event );

	bool onMouseDown( UIRichText* source, const Vector2i& position, Uint32 flags );

	bool onMouseUp( const Vector2i& position, Uint32 flags );

	bool onMouseDoubleClick( UIRichText* source, const Vector2i& position, Uint32 flags );

	bool onMouseUpMessage( const NodeMessage* message );

	bool showContextMenu( const Vector2i& position, Uint32 flags, const Node* target = nullptr );

	/** Consumes the pending click from a text drag so a link does not navigate on release. */
	bool consumeSuppressedClick();

	void updateSelectionDrag();

	void onDocumentWillChange();

	void onDocumentChanged();

	/** Called before an owner's RichText member is destroyed. */
	void onOwnerWillBeDestroyed( UIRichText* owner );

	void refresh();

  private:
	UIWidget* mHost{ nullptr };
	UIWidget* mRoot{ nullptr };
	UIHTMLWidget* mContainRoot{ nullptr };
	UIScrollView* mScrollTarget{ nullptr };
	UIPopUpMenu* mCurrentMenu{ nullptr };
	std::function<std::string( const std::string& )> mLinkResolverCb;
	EventConnection mMenuItemConnection;
	EventConnection mMenuCloseConnection;
	SmallVector<UIRichText*, 8> mOwners;
	// Ancestors of paint owners identify atomic placeholders for independently painted descendants.
	UnorderedSet<const Node*> mOwnerSubtreeNodes;
	// One paint owner can have several document-order slices around nested block content.
	struct SelectionSlice {
		UIRichText* owner;
		Int64 start;
		Int64 end;
		const Node* source; // Used only while sorting; cleared before the slices are cached.
	};
	struct SelectedSlice {
		UIRichText* owner;
		Int64 start;
		Int64 end;
	};
	SmallVector<SelectionSlice, 8> mSlices;
	SmallVector<SelectedSlice, 8> mSelectedSlices;
	SmallVector<UIRichText*, 8> mSelectedOwners;
	// Keep pointer direction before CSS all/contain resolution; project only the resolved range.
	Selection mRawSelection;
	Selection mResolvedSelection;
	bool mDragging{ false };
	bool mExplicitSelectAll{ false };
	bool mSuppressClick{ false };
	Vector2i mDragStart;

	void collectOwners();

	Point hitTest( const Vector2f& position ) const;

	void resolveAndProject();

	int indexOf( const UIRichText* owner ) const;

	int sliceIndexOf( Point point ) const;

	int compare( Point a, Point b ) const;

	Point firstPointIn( const UIHTMLWidget* subtree ) const;

	Point lastPointIn( const UIHTMLWidget* subtree ) const;

	static bool isDescendantOf( const UIWidget* widget, const UIWidget* ancestor );
};

}} // namespace EE::UI

#endif
