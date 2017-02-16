#include <eepp/ui/uigridcell.hpp>
#include <eepp/ui/uigenericgrid.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIGridCell::UIGridCell( UIGridCell::CreateParams& Params ) :
	UIComplexControl( Params )
{
	mCells.resize( gridParent()->getCollumnsCount(), NULL );

	applyDefaultTheme();
}

UIGridCell::~UIGridCell() {
	if ( UIManager::instance()->focusControl() == this )
		mParentCtrl->setFocus();

	if ( UIManager::instance()->overControl() == this )
		UIManager::instance()->overControl( mParentCtrl );
}

void UIGridCell::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "gridcell" );
}

UIGenericGrid * UIGridCell::gridParent() const {
	return reinterpret_cast<UIGenericGrid*> ( mParentCtrl->parent() );
}

void UIGridCell::cell( const Uint32& CollumnIndex, UIControl * Ctrl ) {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	UIGenericGrid * P = gridParent();

	mCells[ CollumnIndex ] = Ctrl;

	if ( Ctrl->parent() != this )
		Ctrl->parent( this );

	Ctrl->position		( P->getCellPosition( CollumnIndex )		, 0					);
	Ctrl->size		( P->collumnWidth( CollumnIndex )	, P->rowHeight()	);

	Ctrl->visible( true );
	Ctrl->enabled( true );
}

UIControl * UIGridCell::cell( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	return mCells[ CollumnIndex ];
}

void UIGridCell::fixCell() {
	autoSize();

	UIGenericGrid * P = gridParent();

	for ( Uint32 i = 0; i < mCells.size(); i++ ) {
		mCells[i]->position	( P->getCellPosition( i )	, 0					);
		mCells[i]->size		( P->collumnWidth( i )	, P->rowHeight()	);
	}
}

void UIGridCell::update() {
	if ( mEnabled && mVisible ) {
		UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( parent()->parent() );
		Uint32 Flags				= UIManager::instance()->getInput()->clickTrigger();

		if ( NULL != MyParent && MyParent->alpha() != mAlpha ) {
			alpha( MyParent->alpha() );

			for ( Uint32 i = 0; i < mCells.size(); i++ ) {
				if ( NULL != mCells[i] && mCells[i]->isAnimated() ) {
					reinterpret_cast<UIControlAnim*>( mCells[i] )->alpha( MyParent->alpha() );
				}
			}
		}

		if ( isMouseOverMeOrChilds() ) {
			if ( ( Flags & EE_BUTTONS_WUWD ) && MyParent->verticalScrollBar()->visible() ) {
				MyParent->verticalScrollBar()->getSlider()->manageClick( Flags );
			}
		}
	}

	UIComplexControl::update();
}

void UIGridCell::select() {
	UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( parent()->parent() );

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

void UIGridCell::autoSize() {
	UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( parent()->parent() );

	size( MyParent->mTotalWidth, MyParent->mRowHeight );
}

void UIGridCell::onStateChange() {
	if ( isSelected() && mSkinState->getState() != UISkinState::StateSelected ) {
		setSkinState( UISkinState::StateSelected );
	}
}

}}
