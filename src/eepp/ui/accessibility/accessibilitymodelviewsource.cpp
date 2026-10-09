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
		mManager( manager ),
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
			if ( !cell->hasVisibility() || !info.bounds.intersect( mView->getWorldBounds() ) ) {
				info.states = static_cast<AccessibilityState>(
					static_cast<Uint64>( info.states ) &
					~static_cast<Uint64>( AccessibilityState::Showing ) );
			}
		} else {
			info.states = static_cast<AccessibilityState>(
				static_cast<Uint64>( info.states ) &
				~static_cast<Uint64>( AccessibilityState::Showing ) );
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
		if ( found == mNodes.end() || !found->second.index.isValid() )
			return 0;
		if ( hostsEditor( found->second ) )
			return 1;
		if ( found->second.cell || mIsList || mIsTree )
			return 0;
		return visibleColumnCount();
	}

	AccessibilityNodeRef getChild( Uint64 id, size_t child ) override {
		auto found = mNodes.find( id );
		if ( found == mNodes.end() || !found->second.index.isValid() )
			return {};
		if ( hostsEditor( found->second ) )
			return child == 0 ? mManager.getNodeRef( exposedEditor() ) : AccessibilityNodeRef{};
		if ( found->second.cell || mIsList || mIsTree )
			return {};
		ModelIndex index = found->second.index;
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
		// The view's hit test visits only realized widgets, never every historically queried row.
		auto* editor = exposedEditor();
		for ( auto* node = mView->overFind( position ); node && node != mView;
			  node = node->getParent() ) {
			if ( node == editor )
				return mManager.getNodeRef( editor );
			if ( node->isType( UI_TYPE_TABLECELL ) ) {
				auto index = static_cast<UITableCell*>( node )->getCurIndex();
				if ( mIsList || mIsTree )
					index = index.siblingAtColumn( mView->getMainColumn() );
				return refFor( index, !mIsList && !mIsTree );
			}
		}
		return mHost;
	}

	AccessibilityNodeRef getEmbeddedWidgetParent( const UIWidget* widget ) override {
		const auto* editor = exposedEditor();
		if ( !editor || editor != widget )
			return {};
		const ModelIndex& index = mView->getEditIndex();
		return mIsList || mIsTree ? refFor( index.siblingAtColumn( mView->getMainColumn() ), false )
								  : refFor( index, true );
	}

	Int32 getIndexInParent( Uint64 id ) const override {
		auto found = mNodes.find( id );
		if ( found == mNodes.end() || !found->second.index.isValid() )
			return -1;
		const ModelIndex index = found->second.index;
		if ( found->second.cell ) {
			for ( size_t child = 0, count = visibleColumnCount(); child < count; ++child ) {
				if ( visibleColumnAt( child ) == index.column() )
					return static_cast<Int32>( child );
			}
			return -1;
		}
		if ( !mIsTree )
			return index.row();
		refreshVisibleTree();
		if ( mVisibleTreePositions.empty() ) {
			mVisibleTreePositions.reserve( mVisibleTreeIndexes.size() );
			for ( size_t child = 0; child < mVisibleTreeIndexes.size(); ++child )
				mVisibleTreePositions.emplace( mVisibleTreeIndexes[child],
											   static_cast<Int32>( child ) );
		}
		auto position = mVisibleTreePositions.find( index );
		return position == mVisibleTreePositions.end() ? -1 : position->second;
	}

	bool getSelectedChildren( std::vector<AccessibilityNodeRef>& selected ) const override {
		if ( mView->getSelection().size() <= 1 ) {
			auto row = mView->getSelection().first().siblingAtColumn( mView->getMainColumn() );
			if ( row.isValid() )
				selected.emplace_back( refFor( row, false ) );
			return true;
		}
		auto indexes = mView->getSelection().indexes();
		selected.reserve( indexes.size() );
		UnorderedSet<ModelIndex> rows;
		rows.reserve( indexes.size() );
		for ( const auto& index : indexes ) {
			auto row = index.siblingAtColumn( mView->getMainColumn() );
			if ( row.isValid() && rows.emplace( row ).second )
				selected.emplace_back( refFor( row, false ) );
		}
		return true;
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

	void invalidate() override {
		mVisibleTreeIndexesValid = false;
		// Persistent indexes follow inserted/removed rows, but ordinary ModelIndex keys do not.
		// Re-key only queried nodes, preserving their identities and pruning removed rows.
		mItemIds.clear();
		mCellIds.clear();
		for ( auto it = mNodes.begin(); it != mNodes.end(); ) {
			if ( !it->second.index.isValid() ) {
				it = mNodes.erase( it );
				continue;
			}
			auto& ids = it->second.cell ? mCellIds : mItemIds;
			ids.emplace( ModelIndex( it->second.index ), it->first );
			++it;
		}
	}

	void reset() override {
		mVisibleTreeIndexesValid = false;
		mVisibleTreeIndexes.clear();
		mVisibleTreePositions.clear();
		mItemIds.clear();
		mCellIds.clear();
		mNodes.clear();
	}

  private:
	struct NodeInfo {
		Models::PersistentModelIndex index;
		bool cell{ false };
	};

	/** The cell editor, when it is a live, exposed accessibility element. */
	UIWidget* exposedEditor() const {
		auto* editor = mView->getEditWidget();
		return editor && mView->getEditIndex().isValid() && !editor->isDestroying() &&
					   !editor->isClosing() && editor->isAccessibilityElement() &&
					   !editor->isAccessibilityHidden()
				   ? editor
				   : nullptr;
	}

	bool hostsEditor( const NodeInfo& node ) const {
		if ( !exposedEditor() )
			return false;
		const ModelIndex& edit = mView->getEditIndex();
		if ( mIsList || mIsTree )
			return !node.cell &&
				   ModelIndex( node.index ) == edit.siblingAtColumn( mView->getMainColumn() );
		return node.cell && ModelIndex( node.index ) == edit;
	}

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
		for ( size_t column : mView->getColumnOrder() ) {
			if ( !mView->isColumnHidden( column ) && wanted-- == 0 )
				return column;
		}
		return -1;
	}

	void refreshVisibleTree() const {
		if ( mVisibleTreeIndexesValid )
			return;
		mVisibleTreeIndexes = static_cast<UITreeView*>( mView )->getVisibleModelIndexes();
		mVisibleTreePositions.clear();
		mVisibleTreeIndexesValid = true;
	}

	AccessibilityManager& mManager;
	AccessibilitySourceId mSourceId;
	Abstract::UIAbstractTableView* mView;
	AccessibilityNodeRef mHost;
	bool mIsList;
	bool mIsTree;
	mutable bool mVisibleTreeIndexesValid{ false };
	mutable Uint64 mNextId{ 1 };
	mutable UnorderedMap<ModelIndex, Uint64> mItemIds;
	mutable UnorderedMap<ModelIndex, Uint64> mCellIds;
	mutable UnorderedMap<Uint64, NodeInfo> mNodes;
	mutable std::vector<ModelIndex> mVisibleTreeIndexes;
	mutable UnorderedMap<ModelIndex, Int32> mVisibleTreePositions;
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
