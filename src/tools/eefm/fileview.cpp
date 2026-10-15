#include "fileview.hpp"
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/nodemessage.hpp>
#include <eepp/ui/models/modeleditingdelegate.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitablerow.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/window.hpp>

using namespace EE::Scene;
using namespace EE::UI::Doc;
using namespace EE::UI::Models;
using namespace EE::Window;

namespace eefm {

FileDrag& currentFileDrag() {
	static FileDrag drag;
	return drag;
}

namespace {

/** Floating label that follows the pointer while files are dragged. */
UITextView* dragLabel( UISceneNode* ui ) {
	static UITextView* label = nullptr;
	if ( !label || label->getUISceneNode() != ui ) {
		label = UITextView::New();
		label->setParent( ui->getRoot() );
		label->setClass( "file_drag_label" );
		label->setVisible( false )->setEnabled( false );
	}
	return label;
}

class FileCell : public UITableCell {
  public:
	static FileCell* New( const std::string& tag ) { return eeNew( FileCell, ( tag ) ); }

	explicit FileCell( const std::string& tag ) : UITableCell( tag ) {
		setDragEnabled( true );
		// The text takes the cell's free width, so long names end in an ellipsis (stylesheet).
		setExpandTextView( true );
	}

	Uint32 getType() const override { return FileTableView::CellType; }

	bool isType( const Uint32& type ) const override {
		return getType() == type || UITableCell::isType( type );
	}

	FileTableView* view() const {
		Node* node = getParent();
		while ( node && !node->isType( UI_TYPE_TABLEVIEW ) )
			node = node->getParent();
		return node ? node->asType<FileTableView>() : nullptr;
	}

	bool acceptsDropOfWidget( const UIWidget* widget ) override {
		if ( !widget->isType( FileTableView::CellType ) )
			return false;
		// Highlight folders only: a drop elsewhere goes to the current folder.
		auto* table = view();
		if ( !table || !table->isFolderRow || !getCurIndex().isValid() )
			return false;
		return table->isFolderRow( static_cast<size_t>( getCurIndex().row() ) );
	}

	Uint32 onDragStart( const Vector2i& position, const Uint32& flags ) override {
		// eepp starts dragging on press: the file drag only begins once the pointer moves, so
		// clicks keep selecting (and never drop the row onto itself).
		mPressPosition = position.asFloat();
		mDragActive = false;
		return UITableCell::onDragStart( position, flags );
	}

	void beginFileDrag() {
		mDragActive = true;
		if ( auto* table = view() ) {
			// Dragging an unselected row drags that row alone.
			ModelIndex rowIndex = getParent()->asType<UITableRow>()->getCurIndex();
			if ( !table->getSelection().contains( rowIndex ) )
				table->getSelection().set( rowIndex );
			auto& drag = currentFileDrag();
			drag.uris = table->dragPayload ? table->dragPayload() : std::vector<URI>{};
			drag.source = table;
			drag.folders = table->isFolderRow != nullptr;
			for ( const auto& index : table->getSelection().indexes() )
				drag.folders = drag.folders && index.isValid() &&
							   table->isFolderRow( static_cast<size_t>( index.row() ) );
			auto* label = dragLabel( getUISceneNode() );
			label->setText( drag.uris.size() == 1
								? String::fromUtf8( drag.uris[0].getLastPathSegment() )
								: String( String::format( "%zu items", drag.uris.size() ) ) );
			label->toFront();
			if ( table->onDragStarted )
				table->onDragStarted();
		}
		addClass( "dragged" );
	}

	/** Ends a press that never became a file drag, without emitting a drop. */
	void cancelPendingDrag() {
		setDragging( false, false );
		if ( getEventDispatcher() && getEventDispatcher()->getNodeDragging() == this )
			getEventDispatcher()->setNodeDragging( nullptr );
	}

	Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags ) override {
		if ( isDragging() && !mDragActive && !( flags & getDragButton() ) ) {
			cancelPendingDrag();
			return 1;
		}
		return UITableCell::onCalculateDrag( position, flags );
	}

	Uint32 onMouseUp( const Vector2i& position, const Uint32& flags ) override {
		if ( isDragging() && !mDragActive && ( flags & getDragButton() ) )
			cancelPendingDrag();
		return UITableCell::onMouseUp( position, flags );
	}

	Uint32 onMouseClick( const Vector2i& position, const Uint32& flags ) override {
		if ( flags & EE_BUTTON_MMASK ) {
			auto* table = view();
			ModelIndex rowIndex = getParent()->asType<UITableRow>()->getCurIndex();
			if ( table && table->onRowMiddleClick && rowIndex.isValid() ) {
				table->onRowMiddleClick( static_cast<size_t>( rowIndex.row() ) );
				return 1;
			}
		}
		return UITableCell::onMouseClick( position, flags );
	}

	Uint32 onDrag( const Vector2f& position, const Uint32&, const Sizef& ) override {
		if ( !mDragActive ) {
			if ( std::abs( position.x - mPressPosition.x ) < PixelDensity::dpToPx( 4 ) &&
				 std::abs( position.y - mPressPosition.y ) < PixelDensity::dpToPx( 4 ) )
				return 0;
			beginFileDrag();
		}
		auto* label = dragLabel( getUISceneNode() );
		label->setVisible( true );
		label->setPixelsPosition( position + PixelDensity::dpToPx( 12 ) );
		getUISceneNode()->getWindow()->getCursorManager()->set( Cursor::Hand );
		setEnabled( false );
		Node* over = getUISceneNode()->overFind( position );
		setEnabled( true );
		UIWidget* target = nullptr;
		if ( over && over->isWidget() )
			target = over->asType<UIWidget>()->acceptsDropOfWidgetInTree( this );
		if ( target != mDropTarget ) {
			if ( mDropTarget )
				mDropTarget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 0 );
			mDropTarget = target;
			if ( mDropTarget )
				mDropTarget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 1 );
		}
		// The cell itself never moves.
		return 1;
	}

	Uint32 onDragStop( const Vector2i& position, const Uint32& flags ) override {
		mDragActive = false;
		dragLabel( getUISceneNode() )->setVisible( false );
		removeClass( "dragged" );
		getUISceneNode()->getWindow()->getCursorManager()->set( Cursor::Arrow );
		if ( mDropTarget ) {
			mDropTarget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 0 );
			mDropTarget = nullptr;
		}
		return UITableCell::onDragStop( position, flags );
	}

	Uint32 onKeyDown( const KeyEvent& event ) override {
		// Escape cancels the drag without dropping.
		if ( event.getKeyCode() == KEY_ESCAPE && isDragging() ) {
			setDragging( false, false );
			getEventDispatcher()->setNodeDragging( nullptr );
			return 1;
		}
		return UITableCell::onKeyDown( event );
	}

  private:
	UIWidget* mDropTarget{ nullptr };
	Vector2f mPressPosition;
	bool mDragActive{ false };
};

} // namespace

FileTableView* FileTableView::New() {
	return eeNew( FileTableView, () );
}

FileTableView::FileTableView() : UITableView() {
	// Renaming is in place, started by beginRename() (F2, the context menu); double clicks open.
	setEditable( true );
	setEditTriggers( EditTrigger::None );
	onCreateEditingDelegate = [this]( const ModelIndex& index ) -> ModelEditingDelegate* {
		auto* delegate = StringModelEditingDelegate::New();
		bool folder = isFolderRow && index.row() >= 0 && isFolderRow( index.row() );
		delegate->onWillBeginEditing = [delegate, folder] {
			// As Dolphin: the name without its extension is selected, ready to type over.
			auto* input = delegate->getWidget()->asType<UITextInput>();
			const String& text = input->getText();
			size_t dot = folder ? String::InvalidPos : text.rfind( '.' );
			if ( dot != String::InvalidPos && dot > 0 )
				input->getDocument().setSelection(
					TextRange( { 0, 0 }, { 0, static_cast<Int64>( dot ) } ) );
		};
		return delegate;
	};
}

void FileTableView::beginRename( size_t row ) {
	if ( !getModel() || row >= getModel()->rowCount() )
		return;
	ModelIndex index = getModel()->index( row, getModel()->searchColumn() );
	getSelection().set( getModel()->index( row ) );
	scrollToIndex( index );
	// The row's cells exist once it is laid out (after scrolling).
	runOnMainThread( [this, index] {
		UITableCell* cell = getCellFromIndex( index );
		if ( !cell )
			return;
		beginEditing( index, cell );
		// The editor covers the name only (the icon stays), its text where the name was.
		UIWidget* editor = getEditWidget();
		if ( editor && cell->getTextView() ) {
			Float x = std::max( 0.f, cell->getTextView()->getPixelsPosition().x -
										 editor->getPixelsPadding().Left );
			editor->setPixelsPosition( x, 0 );
			editor->setPixelsSize( cell->getPixelsSize().getWidth() - x,
								   cell->getPixelsSize().getHeight() );
		}
	} );
}

UIWidget* FileTableView::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	// Same tag as the stock cells, so theme rules for tableview::cell apply.
	return setupCell( FileCell::New( mTag + "::cell" ), rowWidget, index );
}

std::optional<size_t> FileTableView::rowOf( Node* node ) const {
	while ( node && node != this ) {
		ModelIndex index;
		if ( node->isType( UI_TYPE_TABLECELL ) )
			index = node->asType<UITableCell>()->getCurIndex();
		else if ( auto* row = dynamic_cast<UITableRow*>( node ) )
			index = row->getCurIndex();
		else {
			node = node->getParent();
			continue;
		}
		if ( index.isValid() )
			return static_cast<size_t>( index.row() );
		return std::nullopt;
	}
	return std::nullopt;
}

Uint32 FileTableView::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::Drop ) {
		const auto* drop = static_cast<const NodeDropMessage*>( msg );
		if ( drop->getDroppedNode() && drop->getDroppedNode()->isType( CellType ) ) {
			if ( onFilesDropped )
				onFilesDropped( rowOf( drop->getSender() ) );
			return 1;
		}
	}
	return UITableView::onMessage( msg );
}

Uint32 FileTableView::onKeyDown( const KeyEvent& event ) {
	// Escape clears the selection (as Dolphin); with nothing selected it is left to others.
	if ( event.getKeyCode() == KEY_ESCAPE && !isEditing() &&
		 !( event.getMod() & ( KEYMOD_CTRL | KEYMOD_SHIFT | KEYMOD_ALT | KEYMOD_META ) ) &&
		 !getSelection().isEmpty() ) {
		getSelection().clear();
		return 1;
	}
	return UITableView::onKeyDown( event );
}

Uint32 FileTableView::onMouseClick( const Vector2i& position, const Uint32& flags ) {
	// The release of a rubber band selection is not a click on the empty area.
	if ( rubberBandDragged() )
		return UITableView::onMouseClick( position, flags );
	// Clicks that reach the view itself landed on the empty area below the rows.
	if ( flags & EE_BUTTON_RMASK ) {
		getSelection().clear();
		if ( onBackgroundMenu )
			onBackgroundMenu();
		return 1;
	}
	if ( flags & EE_BUTTON_LMASK )
		getSelection().clear();
	return UITableView::onMouseClick( position, flags );
}

} // namespace eefm
