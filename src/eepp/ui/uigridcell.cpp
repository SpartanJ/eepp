#include <eepp/ui/uigridcell.hpp>
#include <eepp/ui/uigenericgrid.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIGridCell * UIGridCell::New() {
	return eeNew( UIGridCell, () );
}

UIGridCell::UIGridCell() :
	UIComplexControl()
{
	applyDefaultTheme();
}

UIGridCell::~UIGridCell() {
	if ( UIManager::instance()->getFocusControl() == this )
		mParentCtrl->setFocus();

	if ( UIManager::instance()->getOverControl() == this )
		UIManager::instance()->setOverControl( mParentCtrl );
}

void UIGridCell::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "gridcell" );
}

UIGenericGrid * UIGridCell::gridParent() const {
	return reinterpret_cast<UIGenericGrid*> ( mParentCtrl->getParent() );
}

void UIGridCell::setCell( const Uint32& CollumnIndex, UIControl * Ctrl ) {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	UIGenericGrid * P = gridParent();

	mCells[ CollumnIndex ] = Ctrl;

	if ( Ctrl->getParent() != this )
		Ctrl->setParent( this );

	Ctrl->setPosition( P->getCellPosition( CollumnIndex ), 0 );
	Ctrl->setSize( P->getCollumnWidth( CollumnIndex ), P->getRowHeight() );

	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
}

UIControl * UIGridCell::getCell( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	return mCells[ CollumnIndex ];
}

void UIGridCell::fixCell() {
	onAutoSize();

	UIGenericGrid * P = gridParent();

	for ( Uint32 i = 0; i < mCells.size(); i++ ) {
		mCells[i]->setPosition	( P->getCellPosition( i )	, 0					);
		mCells[i]->setSize		( P->getCollumnWidth( i )	, P->getRowHeight()	);
	}
}

void UIGridCell::update() {
	if ( mEnabled && mVisible ) {
		UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( getParent()->getParent() );
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

	UIComplexControl::update();
}

void UIGridCell::select() {
	UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( getParent()->getParent() );

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

void UIGridCell::unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	setSkinState( UISkinState::StateNormal );
}

bool UIGridCell::isSelected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

Uint32 UIGridCell::onMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UIControl::onMouseExit( Pos, Flags );

	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		setSkinState( UISkinState::StateSelected );

	return 1;
}

Uint32 UIGridCell::onMessage( const UIMessage * Msg ) {
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

void UIGridCell::onAutoSize() {
	UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( getParent()->getParent() );

	setSize( MyParent->mTotalWidth, MyParent->mRowHeight );
}

void UIGridCell::onStateChange() {
	if ( isSelected() && mSkinState->getState() != UISkinState::StateSelected ) {
		setSkinState( UISkinState::StateSelected );
	}
}

void UIGridCell::onParentChange() {
	if ( NULL != getParent() && NULL != gridParent() )
		mCells.resize( gridParent()->getCollumnsCount(), NULL );
}

}}
