#include "fileview.hpp"
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/nodemessage.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/models/modeleditingdelegate.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitablerow.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/draganddrop.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>
#include <unordered_map>

using namespace EE::Graphics;
using namespace EE::Scene;
using namespace EE::UI::Doc;
using namespace EE::UI::Models;
using namespace EE::Window;

namespace eefm {

namespace {

/** Floating label that follows the pointer while files are dragged: the dragged row's icon and
 * name (or the item count). */
UIPushButton* dragLabel( UISceneNode* ui ) {
	static UIPushButton* label = nullptr;
	if ( !label || label->getUISceneNode() != ui ) {
		label = UIPushButton::NewWithTag( "file_drag_label" );
		label->setParent( ui->getRoot() );
		label->setVisible( false )->setEnabled( false );
	}
	return label;
}

/** Renders widget alone (nothing behind it) and reads it back: the system drag shows it. */
Image snapshot( UIWidget* widget ) {
	auto* ui = widget->getUISceneNode();
	auto* window = ui->getWindow();
	ui->updateDirtyStyles();
	ui->updateDirtyStyleStates();
	ui->updateDirtyLayouts();
	const Sizef size = widget->getPixelsSize();
	const int width = std::min<int>( std::ceil( size.getWidth() ), window->getWidth() );
	const int height = std::min<int>( std::ceil( size.getHeight() ), window->getHeight() );
	if ( width <= 0 || height <= 0 || width > 2048 || height > 2048 )
		return {};
	// Drawn at the window's top left, into the back buffer, which is never presented: the next
	// frame draws the scene again.
	const Vector2f position = widget->getPixelsPosition();
	const bool visible = widget->isVisible();
	widget->setPixelsPosition( 0, 0 );
	widget->setVisible( true );
	// The label's rounded corners keep the cleared color (the X11 preview is opaque): the theme
	// background, not the window's black.
	const RGB clearColor = window->getClearColor();
	if ( widget->getUIStyle() ) {
		auto back = widget->getUIStyle()->getVariable( "--back" );
		if ( !back.isEmpty() )
			window->setClearColor( Color::fromString( back.getValue() ).toRGB() );
	}
	auto context = Engine::instance()->makeWindowCurrent( window );
	window->clear();
	window->setClearColor( clearColor );
	widget->nodeDraw();
	GlobalBatchRenderer::instance()->draw();
	Image image( width, height, 4, Color::Transparent, false );
	FrameReadback readback;
	readback.pixels = image.getPixels();
	readback.size = { width, height };
	readback.stride = static_cast<size_t>( width ) * 4;
	readback.position = { 0, static_cast<int>( window->getHeight() ) - height };
	readback.format = FramePixelFormat::RGBA32;
	readback.origin = FrameOrigin::TopLeft;
	const bool read = window->readFrameBuffer( readback );
	widget->setPixelsPosition( position );
	widget->setVisible( visible );
	ui->invalidate( nullptr );
	if ( !read )
		return {};
	if ( readback.origin == FrameOrigin::BottomLeft ) {
		for ( int y = 0; y < height / 2; ++y ) {
			auto* row = image.getPixels() + y * readback.stride;
			auto* opposite = image.getPixels() + ( height - 1 - y ) * readback.stride;
			std::swap_ranges( row, row + readback.stride, opposite );
		}
	}
	return image;
}

class FileCell : public UITableCell {
  public:
	static FileCell* New( const std::string& tag ) { return eeNew( FileCell, ( tag ) ); }

	explicit FileCell( const std::string& tag ) : UITableCell( tag ) {
		setDragEnabled( true );
		// The text takes the cell's free width, so long names end in an ellipsis (stylesheet).
		setExpandTextView( true );
	}

	~FileCell() override {
		if ( sNativeDragCell == this )
			sNativeDragCell = nullptr;
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

	static UIIcon* rowIcon( FileTableView* table, const ModelIndex& row ) {
		auto* model = table->getModel();
		if ( !model || !row.isValid() )
			return nullptr;
		Variant data(
			model->data( model->index( row.row(), table->getMainColumn() ), ModelRole::Icon ) );
		return data.is( Variant::Type::Icon ) ? data.asIcon() : nullptr;
	}

	/** The row whose icon stands for the dragged items: one of the most common type (icons are
	 * cached per type), the pressed row on a tie. */
	static ModelIndex dragIconRow( FileTableView* table, const ModelIndex& pressed ) {
		const auto indexes = table->getSelection().indexes();
		if ( indexes.size() < 2 )
			return pressed;
		std::unordered_map<UIIcon*, std::pair<size_t, ModelIndex>> counts;
		for ( const auto& index : indexes ) {
			auto& count =
				counts.emplace( rowIcon( table, index ), std::make_pair( 0, index ) ).first->second;
			++count.first;
		}
		const size_t pressedCount = counts[rowIcon( table, pressed )].first;
		ModelIndex best = pressed;
		size_t bestCount = pressedCount;
		for ( const auto& index : indexes ) {
			const auto& count = counts[rowIcon( table, index )];
			if ( count.first > bestCount ) {
				bestCount = count.first;
				best = count.second;
			}
		}
		return best;
	}

	void beginFileDrag() {
		mDragActive = true;
		if ( auto* table = view() ) {
			// Dragging an unselected row drags that row alone.
			ModelIndex rowIndex = getParent()->asType<UITableRow>()->getCurIndex();
			if ( !table->getSelection().contains( rowIndex ) )
				table->getSelection().set( rowIndex );
			auto& drag = FileTableView::currentDrag();
			drag.uris = table->dragPayload ? table->dragPayload() : std::vector<URI>{};
			drag.source = table;
			drag.folders = table->isFolderRow != nullptr;
			drag.native = false;
			for ( const auto& index : table->getSelection().indexes() ) {
				drag.folders = drag.folders && index.isValid() &&
							   table->isFolderRow( static_cast<size_t>( index.row() ) );
			}
			auto* label = dragLabel( getUISceneNode() );
			const ModelIndex iconRow = dragIconRow( table, rowIndex );
			DrawablePtr icon;
			if ( UIIcon* found = rowIcon( table, iconRow ) )
				icon = found->createDrawable( table->getIconSize() );
			label->setIcon( icon );
			// Folders are tinted as in the rows (stylesheet).
			bool folder = table->isFolderRow && iconRow.isValid() &&
						  table->isFolderRow( static_cast<size_t>( iconRow.row() ) );
			if ( folder )
				label->addClass( "vfs_folder" );
			else
				label->removeClass( "vfs_folder" );
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

	/** Starts the system drag along with eefm's own as soon as files are dragged (local files
	 * only: other applications get paths), so they can be dropped anywhere: other windows, even
	 * after switching to them with Alt+Tab mid-drag. eefm's drag keeps following the pointer
	 * while it sees it; a drop on eefm is its own (Move/Copy) drop. */
	void startNativeDrag() {
		auto* window = getUISceneNode()->getWindow();
		auto drag = DragAndDrop::forWindow( *window );
		if ( !drag.getCapabilities().fileSource )
			return;
		DragOffer offer;
		for ( const auto& uri : FileTableView::currentDrag().uris ) {
			if ( uri.getScheme() != "file" )
				return;
			offer.files.push_back( { uri.getFSPath() } );
		}
		Image preview = snapshot( dragLabel( getUISceneNode() ) );
		if ( preview.getPixels() )
			offer.preview.image = &preview;
		// Drops on eefm's own window complete as eefm's drag (see endNativeDrag()).
		offer.sourceHandlesDrops = true;
		auto result = drag.startDrag( offer, []( DragResult result ) {
			FileCell* cell = sNativeDragCell;
			sNativeDragCell = nullptr;
			if ( cell )
				cell->endNativeDrag( result.releasedOverSource );
		} );
		if ( result.status != DragStartStatus::Started ) {
			Log::warning( "eefm: native drag not started: %s", result.error.c_str() );
			return;
		}
		FileTableView::currentDrag().native = true;
		sNativeDragCell = this;
		mNativeDrag = true;
		// The system draws the dragged files.
		dragLabel( getUISceneNode() )->setVisible( false );
	}

	/** The system drag ended. Released over eefm's own window (X11 offers it to no one), it is
	 * dropped as eefm's drag. Elsewhere the drop was the target's (eefm included, as a file drop
	 * on Wayland, macOS and Windows), or cancelled. */
	void endNativeDrag( bool drop ) {
		mNativeDrag = false;
		if ( !isDragging() )
			return;
		setDragging( false, drop );
		if ( getEventDispatcher() && getEventDispatcher()->getNodeDragging() == this )
			getEventDispatcher()->setNodeDragging( nullptr );
	}

	Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags ) override {
		if ( isDragging() && !mDragActive && !( flags & getDragButton() ) ) {
			cancelPendingDrag();
			return 1;
		}
		// The system drag owns the release: eefm's drag waits for it to end.
		if ( mNativeDrag && !( flags & getDragButton() ) )
			return 1;
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
			startNativeDrag();
		}
		if ( !mNativeDrag ) {
			auto* label = dragLabel( getUISceneNode() );
			label->setVisible( true );
			label->setPixelsPosition( position + PixelDensity::dpToPx( 12 ) );
		}
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
			if ( mNativeDrag )
				DragAndDrop::forWindow( *getUISceneNode()->getWindow() ).cancelDrag();
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
	/** A system drag started with this one is in progress. */
	bool mNativeDrag{ false };
	/** The cell whose drag the system drag in progress started with, while it exists. */
	static FileCell* sNativeDragCell;
};

FileCell* FileCell::sNativeDragCell = nullptr;

} // namespace

FileDrag& FileTableView::currentDrag() {
	static FileDrag drag;
	return drag;
}

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
			if ( dot != String::InvalidPos && dot > 0 ) {
				input->getDocument().setSelection(
					TextRange( { 0, 0 }, { 0, static_cast<Int64>( dot ) } ) );
			}
		};
		return delegate;
	};
}

void FileTableView::beginNavigation() {
	resetSearchText();
	if ( !mReplayingNavigation )
		mNavigationInput.clear();
	mLoading = true;
	// The selected cell can disappear as soon as the model clears the previous directory.
	if ( hasFocusWithin() )
		setFocus();
}

void FileTableView::endNavigation() {
	mLoading = false;
	if ( mReplayingNavigation || mNavigationInput.empty() )
		return;
	if ( !hasFocusWithin() ) {
		mNavigationInput.clear();
		return;
	}
	mReplayingNavigation = true;
	size_t consumed = 0;
	while ( !mLoading && consumed < mNavigationInput.size() ) {
		const auto character = mNavigationInput[consumed++];
		if ( character == '\n' ) {
			const auto index = getSelection().first();
			if ( !index.isValid() ) {
				consumed = mNavigationInput.size();
				break;
			}
			onOpenModelIndex( index );
		} else {
			UITableView::onTextInput( TextInputEvent( this, Event::TextInput, character, 0 ) );
		}
	}
	mNavigationInput.erase( 0, consumed );
	mReplayingNavigation = false;
}

Uint32 FileTableView::onTextInput( const TextInputEvent& event ) {
	if ( mLoading && !isEditing() ) {
		mNavigationInput += event.getText();
		return 1;
	}
	return UITableView::onTextInput( event );
}

void FileTableView::onWidgetFocusLoss() {
	// Abandon pending commands even if focus returns before the listing finishes.
	mNavigationInput.clear();
	UITableView::onWidgetFocusLoss();
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
		if ( node->isType( UI_TYPE_TABLECELL ) ) {
			index = node->asType<UITableCell>()->getCurIndex();
		} else if ( node->isType( UI_TYPE_TABLEROW ) ) {
			index = node->asType<UITableRow>()->getCurIndex();
		} else {
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
	if ( mLoading && ( event.getKeyCode() == KEY_RETURN || event.getKeyCode() == KEY_KP_ENTER ) &&
		 !isEditing() && event.getSanitizedMod() == 0 ) {
		mNavigationInput += '\n';
		return 1;
	}
	if ( event.getKeyCode() == KEY_ESCAPE )
		mNavigationInput.clear();
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
