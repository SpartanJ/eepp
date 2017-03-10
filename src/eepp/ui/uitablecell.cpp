#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitable.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UITableCell * UITableCell::New() {
	return eeNew( UITableCell, () );
}

UITableCell::UITableCell() :
	UIWidget()
{
	applyDefaultTheme();
}

UITableCell::~UITableCell() {
	if ( UIManager::instance()->getFocusControl() == this )
		mParentCtrl->setFocus();

	if ( UIManager::instance()->getOverControl() == this )
		UIManager::instance()->setOverControl( mParentCtrl );
}

void UITableCell::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	setThemeControl( Theme, "gridcell" );
}

UITable * UITableCell::gridParent() const {
	return reinterpret_cast<UITable*> ( mParentCtrl->getParent() );
}

void UITableCell::setCell( const Uint32& CollumnIndex, UIControl * Ctrl ) {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	UITable * P = gridParent();

	mCells[ CollumnIndex ] = Ctrl;

	if ( Ctrl->getParent() != this )
		Ctrl->setParent( this );

	Ctrl->setPosition( P->getCellPosition( CollumnIndex ), 0 );
	Ctrl->setSize( P->getCollumnWidth( CollumnIndex ), P->getRowHeight() );

	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
}

UIControl * UITableCell::getCell( const Uint32& CollumnIndex ) const {
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

void UITableCell::update() {
	if ( mEnabled && mVisible ) {
		UITable * MyParent 	= reinterpret_cast<UITable*> ( getParent()->getParent() );
		Uint32 Flags				= UIManager::instance()->getInput()->getClickTrigger();

		if ( NULL != MyParent && MyParent->getAlpha() != mAlpha ) {
			setAlpha( MyParent->getAlpha() );

			for ( Uint32 i = 0; i < mCells.size(); i++ ) {
				if ( NULL != mCells[i] && mCells[i]->isAnimated() ) {
					reinterpret_cast<UIControlAnim*>( mCells[i] )->setAlpha( MyParent->getAlpha() );
				}
			}
		}

		if ( isMouseOverMeOrChilds() ) {
			if ( ( Flags & EE_BUTTONS_WUWD ) && MyParent->getVerticalScrollBar()->isVisible() ) {
				MyParent->getVerticalScrollBar()->getSlider()->manageClick( Flags );
			}
		}
	}

	UIWidget::update();
}

void UITableCell::select() {
	UITable * MyParent 	= reinterpret_cast<UITable*> ( getParent()->getParent() );

	if ( MyParent->getItemSelected() != this ) {
		if ( NULL != MyParent->getItemSelected() )
			MyParent->getItemSelected()->unselect();

		bool wasSelected = 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );

		setSkinState( UISkinState::StateSelected );

		mControlFlags |= UI_CTRL_FLAG_SELECTED;

		MyParent->mSelected = MyParent->getItemIndex( this );

		if ( !wasSelected ) {
			MyParent->onSelected();
		}
	}
}

void UITableCell::unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	setSkinState( UISkinState::StateNormal );
}

bool UITableCell::isSelected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

Uint32 UITableCell::onMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UIControl::onMouseExit( Pos, Flags );

	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		setSkinState( UISkinState::StateSelected );

	return 1;
}

Uint32 UITableCell::onMessage( const UIMessage * Msg ) {
	switch( Msg->getMsg() ) {
		case UIMessage::MsgMouseEnter:
		{
			onMouseEnter( Vector2i(), Msg->getFlags() );
			break;
		}
		case UIMessage::MsgMouseExit:
		{
			onMouseExit( Vector2i(), Msg->getFlags() );
			break;
		}
		case UIMessage::MsgClick:
		{
			if ( Msg->getFlags() & EE_BUTTONS_LRM ) {
				select();

				UIMessage tMsg( this, UIMessage::MsgCellClicked, Msg->getFlags() );

				messagePost( &tMsg );

				return 1;
			}
		}
	}

	return 0;
}

void UITableCell::onAutoSize() {
	UITable * MyParent 	= reinterpret_cast<UITable*> ( getParent()->getParent() );

	setSize( MyParent->mTotalWidth, MyParent->mRowHeight );
}

void UITableCell::onStateChange() {
	if ( isSelected() && mSkinState->getState() != UISkinState::StateSelected ) {
		setSkinState( UISkinState::StateSelected );
	}
}

void UITableCell::onParentChange() {
	if ( NULL != getParent() && NULL != gridParent() )
		mCells.resize( gridParent()->getCollumnsCount(), NULL );
}

}}
