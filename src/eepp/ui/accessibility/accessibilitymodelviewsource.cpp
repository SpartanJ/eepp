#include "accessibilitymodelviewsource.hpp"
#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/models/persistentmodelindex.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI {

namespace {

class AccessibilityModelViewSource final : public AccessibilitySource {
  public:
	AccessibilityModelViewSource( AccessibilityManager& manager, AccessibilitySourceId sourceId,
								  Abstract::UIAbstractTableView* view ) :
		mSourceId( sourceId ),
		mView( view ),
		mHost( manager.getNodeRef( view ) ),
		mIsList( view->isType( UI_TYPE_LISTVIEW ) ),
		mIsTree( view->isType( UI_TYPE_TREEVIEW ) ) {}

	bool isValid( Uint64 id ) const override {
		auto found = mNodes.find( id );
		return found != mNodes.end() && found->second.index.isValid();
	}

	AccessibilityNodeInfo getInfo( Uint64 id ) const override {
		AccessibilityNodeInfo info;
		auto found = mNodes.find( id );
		if ( found == mNodes.end() || !found->second.index.isValid() )
			return info;
		const auto& node = found->second;
		ModelIndex index = node.index;
		info.role = node.cell ? AccessibilityRole::Cell
					: mIsList ? AccessibilityRole::ListItem
					: mIsTree ? AccessibilityRole::TreeItem
							  : AccessibilityRole::Row;
		info.value = index.data().toString();
		info.name =
			node.cell ? String( mView->getModel()->columnName( index.column() ) ) : info.value;
		info.states =
			AccessibilityState::Enabled | AccessibilityState::Visible | AccessibilityState::Showing;
		if ( mView->getSelection().contains( index ) ||
			 ( !mIsTree && !node.cell && mView->getSelection().containsRow( index.row() ) ) )
			info.states |= AccessibilityState::Selected;
		info.actions = accessibilityActionMask( AccessibilityAction::Select ) |
					   accessibilityActionMask( AccessibilityAction::ScrollTo );
		if ( mIsTree && !node.cell && mView->getModel()->rowCount( index ) > 0 ) {
			bool expanded = static_cast<UITreeView*>( mView )->isExpanded( index );
			if ( expanded )
				info.states |= AccessibilityState::Expanded;
			info.actions |= accessibilityActionMask( expanded ? AccessibilityAction::Collapse
															  : AccessibilityAction::Expand );
		}
		if ( auto cell = mView->getCellFromIndex( index ) ) {
			info.bounds = cell->getWorldBounds();
			info.boundsValid = true;
		}
		return info;
	}

	AccessibilityNodeRef getParent( Uint64 id ) const override {
		auto found = mNodes.find( id );
		if ( found == mNodes.end() || !found->second.index.isValid() )
			return {};
		ModelIndex index = found->second.index;
		if ( found->second.cell )
			return refFor( index.siblingAtColumn( mView->getMainColumn() ), false );
		return mHost;
	}

	size_t getChildCount( Uint64 id ) const override {
		auto found = mNodes.find( id );
		if ( found == mNodes.end() || !found->second.index.isValid() || found->second.cell ||
			 mIsList )
			return 0;
		if ( mIsTree )
			return 0;
		return visibleColumnCount();
	}

	AccessibilityNodeRef getChild( Uint64 id, size_t child ) override {
		auto found = mNodes.find( id );
		if ( found == mNodes.end() || !found->second.index.isValid() || found->second.cell )
			return {};
		ModelIndex index = found->second.index;
		if ( mIsTree )
			return {};
		int column = visibleColumnAt( child );
		return column >= 0 ? refFor( index.siblingAtColumn( column ), true )
						   : AccessibilityNodeRef{};
	}

	size_t getRootChildCount() const override {
		if ( mIsTree ) {
			refreshVisibleTree();
			return mVisibleTreeIndexes.size();
		}
		return mView->getModel() ? mView->getModel()->rowCount() : 0;
	}

	AccessibilityNodeRef getRootChild( size_t index ) override {
		if ( !mView->getModel() )
			return {};
		if ( mIsTree ) {
			refreshVisibleTree();
			return index < mVisibleTreeIndexes.size() ? refFor( mVisibleTreeIndexes[index], false )
													  : AccessibilityNodeRef{};
		}
		if ( index >= mView->getModel()->rowCount() )
			return {};
		return refFor( mView->getModel()->index( index, mView->getMainColumn() ), false );
	}

	AccessibilityNodeRef hitTest( const Math::Vector2f& position ) override {
		for ( const auto& entry : mNodes ) {
			auto info = getInfo( entry.first );
			if ( info.boundsValid && info.bounds.contains( position ) )
				return { mSourceId, entry.first };
		}
		return mHost;
	}

	bool performAction( Uint64 id, const AccessibilityActionRequest& request ) override {
		auto found = mNodes.find( id );
		if ( found == mNodes.end() || !found->second.index.isValid() )
			return false;
		ModelIndex index = found->second.index;
		if ( request.action == AccessibilityAction::Select ) {
			mView->setSelection( index, true, mIsTree );
			return true;
		}
		if ( request.action == AccessibilityAction::ScrollTo ) {
			mView->scrollToIndex( index );
			return true;
		}
		if ( mIsTree && !found->second.cell &&
			 ( request.action == AccessibilityAction::Expand ||
			   request.action == AccessibilityAction::Collapse ) ) {
			static_cast<UITreeView*>( mView )->setExpanded(
				index, request.action == AccessibilityAction::Expand );
			invalidate();
			return true;
		}
		return false;
	}

	void invalidate() override { mVisibleTreeIndexesValid = false; }

	void reset() override {
		mVisibleTreeIndexesValid = false;
		mVisibleTreeIndexes.clear();
		mItemIds.clear();
		mCellIds.clear();
		mNodes.clear();
	}

  private:
	struct NodeInfo {
		Models::PersistentModelIndex index;
		bool cell{ false };
	};

	AccessibilityNodeRef refFor( const ModelIndex& index, bool cell ) const {
		if ( !index.isValid() )
			return {};
		auto& ids = cell ? mCellIds : mItemIds;
		auto found = ids.find( index );
		if ( found != ids.end() )
			return { mSourceId, found->second };
		Uint64 id = mNextId++;
		ids.emplace( index, id );
		mNodes.emplace( id, NodeInfo{ Models::PersistentModelIndex( index ), cell } );
		return { mSourceId, id };
	}

	size_t visibleColumnCount() const {
		size_t count = 0;
		for ( size_t column = 0; column < mView->getModel()->columnCount(); ++column )
			count += !mView->isColumnHidden( column );
		return count;
	}

	int visibleColumnAt( size_t wanted ) const {
		for ( size_t column = 0; column < mView->getModel()->columnCount(); ++column ) {
			if ( !mView->isColumnHidden( column ) && wanted-- == 0 )
				return column;
		}
		return -1;
	}

	void refreshVisibleTree() const {
		if ( mVisibleTreeIndexesValid )
			return;
		mVisibleTreeIndexes = static_cast<UITreeView*>( mView )->getVisibleModelIndexes();
		mVisibleTreeIndexesValid = true;
	}

	AccessibilitySourceId mSourceId;
	Abstract::UIAbstractTableView* mView;
	AccessibilityNodeRef mHost;
	bool mIsList;
	bool mIsTree;
	mutable Uint64 mNextId{ 1 };
	mutable UnorderedMap<ModelIndex, Uint64> mItemIds;
	mutable UnorderedMap<ModelIndex, Uint64> mCellIds;
	mutable UnorderedMap<Uint64, NodeInfo> mNodes;
	mutable std::vector<ModelIndex> mVisibleTreeIndexes;
	mutable bool mVisibleTreeIndexesValid{ false };
};

} // namespace

std::unique_ptr<AccessibilitySource>
createAccessibilityModelViewSource( AccessibilityManager& manager, AccessibilitySourceId sourceId,
									UIWidget* widget ) {
	if ( !widget || !widget->isType( UI_TYPE_ABSTRACTTABLEVIEW ) )
		return {};
	return std::make_unique<AccessibilityModelViewSource>(
		manager, sourceId, static_cast<Abstract::UIAbstractTableView*>( widget ) );
}

}} // namespace EE::UI
