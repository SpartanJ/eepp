#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitableheadercolumn.hpp>

namespace EE { namespace UI {

UITableHeaderColumn::UITableHeaderColumn( UIAbstractTableView* view, const size_t& colIndex ) :
	UIPushButton( "table::header::column" ), mView( view ), mColIndex( colIndex ) {
	setDragEnabled( true );
}

Uint32 UITableHeaderColumn::onCalculateDrag( const Vector2f& position, const Uint32& flags ) {
	if ( isDragEnabled() && isDragging() && NULL != getEventDispatcher() ) {
		EventDispatcher* eventDispatcher = getEventDispatcher();
		if ( !( flags /*press trigger*/ & mDragButton ) ) {
			setDragging( false );
			eventDispatcher->setNodeDragging( NULL );
			return 1;
		}
		Vector2f pos( eefloor( position.x ), eefloor( position.y ) );
		if ( mDragPoint != pos && std::abs( mDragPoint.x - pos.x ) > 1.f ) {
			Sizef dragDiff( ( Float )( mDragPoint.x - pos.x ), 0 );
			if ( onDrag( pos, flags, dragDiff ) ) {
				mDragPoint = pos;
				eventDispatcher->setNodeDragging( this );
			}
		}
	}
	return 1;
}

Uint32 UITableHeaderColumn::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
	if ( NULL != getEventDispatcher() && !getEventDispatcher()->isNodeDragging() &&
		 !( getEventDispatcher()->getLastPressTrigger() & mDragButton ) &&
		 ( flags & mDragButton ) && isDragEnabled() && !isDragging() &&
		 localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
		startDragging( position.asFloat() );
	}
	pushState( UIState::StatePressed );
	return Node::onMouseDown( position, flags );
}

Uint32 UITableHeaderColumn::onDrag( const Vector2f& position, const Uint32&,
									const Sizef& dragDiff ) {
	Vector2f localPos( convertToNodeSpace( position ) );
	if ( isDragging() || localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
		setPixelsSize( mSize.x - dragDiff.x, mSize.getHeight() );
		if ( mSize.getWidth() != mView->columnData( mColIndex ).width ) {
			mView->columnData( mColIndex ).width = mSize.getWidth();
			mView->updateHeaderSize();
			mView->onColumnSizeChange( mColIndex );
		}
		return 1;
	}
	return 0;
}

Uint32 UITableHeaderColumn::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
	if ( !isDragging() )
		getUISceneNode()->setCursor( Cursor::Arrow );
	return UIPushButton::onMouseLeave( position, flags );
}

Uint32 UITableHeaderColumn::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
	if ( isDragging() || localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
		getUISceneNode()->setCursor( Cursor::SizeWE );
	} else if ( !isDragging() ) {
		getUISceneNode()->setCursor( Cursor::Arrow );
	}
	return UIPushButton::onMouseMove( position, flags );
}

Uint32 UITableHeaderColumn::onMouseDoubleClick( const Vector2i& position, const Uint32& flags ) {
	Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
	if ( localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() )
		mView->onColumnResizeToContent( mColIndex );
	return UIPushButton::onMouseDoubleClick( position, flags );
}

Uint32 UITableHeaderColumn::onDragStop( const Vector2i& pos, const Uint32& flags ) {
	getUISceneNode()->setCursor( Cursor::Arrow );
	mView->columnData( mColIndex ).width = mSize.getWidth();
	mView->updateHeaderSize();
	mView->onColumnSizeChange( mColIndex );
	return UIPushButton::onDragStop( pos, flags );
}

}} // namespace EE::UI
