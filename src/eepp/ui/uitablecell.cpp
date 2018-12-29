#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitable.hpp>

namespace EE { namespace UI {

UITableCell * UITableCell::New() {
	return eeNew( UITableCell, () );
}

UITableCell::UITableCell() :
	UIWidget( "tablecell" )
{
	applyDefaultTheme();
}

UITableCell::~UITableCell() {
	if ( NULL != getEventDispatcher() ) {
		if ( getEventDispatcher()->getFocusControl() == this )
			mParentCtrl->setFocus();

		if ( getEventDispatcher()->getOverControl() == this )
			getEventDispatcher()->setOverControl( mParentCtrl );
	}
}

void UITableCell::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "gridcell" );
}

UITable * UITableCell::gridParent() const {
	return reinterpret_cast<UITable*> ( mParentCtrl->getParent() );
}

void UITableCell::setCell( const Uint32& CollumnIndex, UINode * Ctrl ) {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	UITable * P = gridParent();

	mCells[ CollumnIndex ] = Ctrl;

	if ( Ctrl->getParent() != this )
		Ctrl->setParent( this );

	if ( Ctrl->isWidget() )
		static_cast<UIWidget*>( Ctrl )->setLayoutSizeRules( FIXED, FIXED );

	Ctrl->setPosition( P->getCellPosition( CollumnIndex ), 0 );
	Ctrl->setSize( P->getCollumnWidth( CollumnIndex ), P->getRowHeight() );

	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
}

UINode * UITableCell::getCell( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	return mCells[ CollumnIndex ];
}

void UITableCell::fixCell() {
	onAutoSize();

	UITable * P = gridParent();

	for ( Uint32 i = 0; i < mCells.size(); i++ ) {
		mCells[i]->setPosition	( P->getCellPosition( i )	, 0					);
		mCells[i]->setSize		( P->getCollumnWidth( i )	, P->getRowHeight()	);
	}
}

void UITableCell::select() {
	UITable * MyParent 	= reinterpret_cast<UITable*> ( getParent()->getParent() );

	if ( MyParent->getItemSelected() != this ) {
		if ( NULL != MyParent->getItemSelected() )
			MyParent->getItemSelected()->unselect();

		bool wasSelected = 0 != ( mNodeFlags & NODE_FLAG_SELECTED );

		pushState( UIState::StateSelected );

		mNodeFlags |= NODE_FLAG_SELECTED;

		MyParent->mSelected = MyParent->getItemIndex( this );

		if ( !wasSelected ) {
			MyParent->onSelected();
		}
	}
}

void UITableCell::unselect() {
	if ( mNodeFlags & NODE_FLAG_SELECTED )
		mNodeFlags &= ~NODE_FLAG_SELECTED;

	popState( UIState::StateSelected);
}

bool UITableCell::isSelected() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SELECTED );
}

Uint32 UITableCell::onMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UINode::onMouseExit( Pos, Flags );

	if ( mNodeFlags & NODE_FLAG_SELECTED )
		pushState( UIState::StateSelected );

	return 1;
}

Uint32 UITableCell::onMessage( const NodeMessage * Msg ) {
	switch( Msg->getMsg() ) {
		case NodeMessage::MouseEnter:
		{
			onMouseEnter( Vector2i(), Msg->getFlags() );
			break;
		}
		case NodeMessage::MouseExit:
		{
			onMouseExit( Vector2i(), Msg->getFlags() );
			break;
		}
		case NodeMessage::Click:
		{
			if ( Msg->getFlags() & EE_BUTTONS_LRM ) {
				select();

				NodeMessage tMsg( this, NodeMessage::CellClicked, Msg->getFlags() );

				messagePost( &tMsg );

				return 1;
			}

			break;
		}
		case NodeMessage::MouseUp:
		{
			UITable * MyParent 	= reinterpret_cast<UITable*> ( getParent()->getParent() );

			if ( ( Msg->getFlags() & EE_BUTTONS_WUWD ) && MyParent->getVerticalScrollBar()->isVisible() ) {
				MyParent->getVerticalScrollBar()->getSlider()->manageClick( Msg->getFlags() );
			}

			break;
		}
	}

	return 0;
}

void UITableCell::onAutoSize() {
	UITable * MyParent 	= reinterpret_cast<UITable*> ( getParent()->getParent() );

	setSize( MyParent->mTotalWidth, MyParent->mRowHeight );
}

void UITableCell::onStateChange() {
	UIWidget::onStateChange();

	if ( isSelected() && !( mSkinState->getState() & UIState::StateFlagSelected ) ) {
		pushState( UIState::StateSelected, false );
	}
}

void UITableCell::onParentChange() {
	if ( NULL != getParent() && NULL != gridParent() )
		mCells.resize( gridParent()->getCollumnsCount(), NULL );
}

void UITableCell::onAlphaChange() {
	if ( mEnabled && mVisible ) {
		UITable * MyParent 	= reinterpret_cast<UITable*> ( getParent()->getParent() );

		if ( NULL != MyParent && MyParent->getAlpha() != mAlpha ) {
			setAlpha( MyParent->getAlpha() );

			for ( Uint32 i = 0; i < mCells.size(); i++ ) {
				if ( NULL != mCells[i] ) {
					mCells[i]->setAlpha( MyParent->getAlpha() );
				}
			}
		}
	}
}

}}
