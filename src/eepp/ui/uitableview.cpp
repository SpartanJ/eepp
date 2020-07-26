#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitableview.hpp>

namespace EE { namespace UI {

UITableView* UITableView::New() {
	return eeNew( UITableView, () );
}

UITableView::UITableView() : UIAbstractTableView( "tableview" ) {
	clipEnable();
}

Uint32 UITableView::getType() const {
	return UI_TYPE_TABLEVIEW;
}

bool UITableView::isType( const Uint32& type ) const {
	return UITableView::getType() == type ? true : UIAbstractTableView::isType( type );
}

void UITableView::drawChilds() {
	int realIndex = 0;

	size_t start = mScrollOffset.y / getRowHeight();
	size_t end =
		eemin<size_t>( (size_t)eeceil( ( mScrollOffset.y + mSize.getHeight() ) / getRowHeight() ),
					   getItemCount() );
	Float yOffset;
	for ( size_t i = start; i < end; i++ ) {
		yOffset = getHeaderHeight() + i * getRowHeight();
		ModelIndex index( getModel()->index( i ) );
		if ( yOffset - mScrollOffset.y > mSize.getHeight() )
			break;
		if ( yOffset - mScrollOffset.y + getRowHeight() < 0 )
			continue;
		for ( size_t colIndex = 0; colIndex < getModel()->columnCount(); colIndex++ ) {
			if ( columnData( colIndex ).visible ) {
				updateCell( realIndex, getModel()->index( index.row(), colIndex, index.parent() ),
							0, yOffset );
			}
		}
		updateRow( realIndex, index, yOffset )->nodeDraw();
		realIndex++;
	}

	if ( mHeader && mHeader->isVisible() )
		mHeader->nodeDraw();
	if ( mHScroll->isVisible() )
		mHScroll->nodeDraw();
	if ( mVScroll->isVisible() )
		mVScroll->nodeDraw();
}

Node* UITableView::overFind( const Vector2f& point ) {
	mUISceneNode->setIsLoading( true );

	Node* pOver = NULL;
	if ( mEnabled && mVisible ) {
		updateWorldPolygon();
		if ( mWorldBounds.contains( point ) && mPoly.pointInside( point ) ) {
			writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );
			mSceneNode->addMouseOverNode( this );
			if ( mHScroll->isVisible() && ( pOver = mHScroll->overFind( point ) ) )
				return pOver;
			if ( mVScroll->isVisible() && ( pOver = mVScroll->overFind( point ) ) )
				return pOver;
			if ( mHeader && ( pOver = mHeader->overFind( point ) ) )
				return pOver;
			int realIndex = 0;
			Float yOffset;
			size_t start = mScrollOffset.y / getRowHeight();
			size_t end = eemin<size_t>(
				(size_t)eeceil( ( mScrollOffset.y + mSize.getHeight() ) / getRowHeight() ),
				getItemCount() );
			for ( size_t i = start; i < end; i++ ) {
				yOffset = getHeaderHeight() + i * getRowHeight();
				ModelIndex index( getModel()->index( i ) );
				if ( yOffset - mScrollOffset.y > mSize.getHeight() )
					break;
				if ( yOffset - mScrollOffset.y + getRowHeight() < 0 )
					continue;
				for ( size_t colIndex = 0; colIndex < getModel()->columnCount(); colIndex++ ) {
					if ( columnData( colIndex ).visible ) {
						updateCell( realIndex,
									getModel()->index( index.row(), colIndex, index.parent() ), 0,
									yOffset );
					}
				}
				pOver = updateRow( realIndex, index, yOffset )->overFind( point );
				if ( pOver )
					break;
				realIndex++;
			}
			if ( !pOver )
				pOver = this;
		}
	}

	mUISceneNode->setIsLoading( false );
	return pOver;
}

Float UITableView::getMaxColumnContentWidth( const size_t& colIndex ) {
	Float lWidth = 0;
	getUISceneNode()->setIsLoading( true );
	Float yOffset = getHeaderHeight();
	for ( size_t i = 0; i < getItemCount(); i++ ) {
		ModelIndex index( getModel()->index( i, colIndex ) );
		UIWidget* widget = updateCell( 0, index, 0, yOffset );
		if ( widget->isType( UI_TYPE_PUSHBUTTON ) ) {
			Float w = widget->asType<UIPushButton>()->getContentSize().getWidth();
			if ( w > lWidth )
				lWidth = w;
		}
		yOffset += getRowHeight();
	}
	getUISceneNode()->setIsLoading( false );
	return lWidth;
}

void UITableView::createOrUpdateColumns() {
	if ( !getModel() )
		return;
	UIAbstractTableView::createOrUpdateColumns();
	updateContentSize();
}

void UITableView::updateContentSize() {
	Sizef oldSize( mContentSize );
	mContentSize = UIAbstractTableView::getContentSize();
	if ( oldSize != mContentSize )
		onContentSizeChange();
}

void UITableView::onColumnSizeChange( const size_t& ) {
	updateContentSize();
}

Uint32 UITableView::onKeyDown( const KeyEvent& event ) {
	if ( event.getMod() != 0 )
		return 0;

	auto curIndex = getSelection().first();
	int pageSize = eefloor( getVisibleArea().getHeight() / getRowHeight() ) - 1;

	switch ( event.getKeyCode() ) {
		case KEY_PAGEUP: {
			if ( curIndex.row() - pageSize < 0 ) {
				getSelection().set( getModel()->index( 0, 0 ) );
				scrollToTop();
			} else {
				moveSelection( -pageSize );
			}
			break;
		}
		case KEY_PAGEDOWN: {
			if ( curIndex.row() + pageSize >= (Int64)getModel()->rowCount() ) {
				getSelection().set( getModel()->index( getItemCount() - 1 ) );
				scrollToBottom();
			} else {
				moveSelection( pageSize );
			}
			break;
		}
		case KEY_UP: {
			moveSelection( -1 );
			break;
		}
		case KEY_DOWN: {
			moveSelection( 1 );
			break;
		}
		case KEY_END: {
			scrollToBottom();
			getSelection().set( getModel()->index( getItemCount() - 1 ) );
			break;
		}
		case KEY_HOME: {
			scrollToTop();
			getSelection().set( getModel()->index( 0, 0 ) );
			break;
		}
		case KEY_RETURN:
		case KEY_KP_ENTER: {
			if ( curIndex.isValid() )
				onOpenModelIndex( curIndex );
			break;
		}
		default:
			break;
	}
	return 0;
}

}} // namespace EE::UI
