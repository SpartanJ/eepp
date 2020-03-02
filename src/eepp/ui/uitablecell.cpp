#include <eepp/ui/uitable.hpp>
#include <eepp/ui/uitablecell.hpp>

namespace EE { namespace UI {

UITableCell* UITableCell::New() {
	return eeNew( UITableCell, () );
}

UITableCell::UITableCell() : UIWidget( "tablecell" ) {
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

void UITableCell::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "gridcell" );

	onThemeLoaded();
}

UITable* UITableCell::gridParent() const {
	return mParentCtrl->getParent()->asType<UITable>();
}

void UITableCell::setCell( const Uint32& CollumnIndex, UINode* Ctrl ) {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	UITable* P = gridParent();

	mCells[CollumnIndex] = Ctrl;

	if ( Ctrl->getParent() != this )
		Ctrl->setParent( this );

	if ( Ctrl->isWidget() )
		static_cast<UIWidget*>( Ctrl )->setLayoutSizeRules( LayoutSizeRule::Fixed,
															LayoutSizeRule::Fixed );

	Ctrl->setPosition( P->getCellPosition( CollumnIndex ), 0 );
	Ctrl->setSize( P->getCollumnWidth( CollumnIndex ), P->getRowHeight() );

	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
}

UINode* UITableCell::getCell( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < gridParent()->getCollumnsCount() );

	return mCells[CollumnIndex];
}

void UITableCell::fixCell() {
	onAutoSize();

	UITable* P = gridParent();

	for ( Uint32 i = 0; i < mCells.size(); i++ ) {
		mCells[i]->setPosition( P->getCellPosition( i ), 0 );
		mCells[i]->setSize( P->getCollumnWidth( i ), P->getRowHeight() );
	}
}

void UITableCell::select() {
	UITable* MyParent = getParent()->getParent()->asType<UITable>();

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

	popState( UIState::StateSelected );
}

bool UITableCell::isSelected() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SELECTED );
}

Uint32 UITableCell::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	UIWidget::onMouseLeave( Pos, Flags );

	if ( mNodeFlags & NODE_FLAG_SELECTED )
		pushState( UIState::StateSelected );

	return 1;
}

Uint32 UITableCell::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Click: {
			if ( Msg->getFlags() & EE_BUTTONS_LRM ) {
				select();

				NodeMessage tMsg( this, NodeMessage::CellClicked, Msg->getFlags() );

				messagePost( &tMsg );

				return 1;
			}

			break;
		}
		case NodeMessage::MouseUp: {
			UITable* MyParent = getParent()->getParent()->asType<UITable>();

			if ( ( Msg->getFlags() & EE_BUTTONS_WUWD ) &&
				 MyParent->getVerticalScrollBar()->isVisible() ) {
				MyParent->getVerticalScrollBar()->getSlider()->manageClick( Msg->getFlags() );
			}

			break;
		}
	}

	return 0;
}

void UITableCell::onAutoSize() {
	UITable* MyParent = getParent()->getParent()->asType<UITable>();

	setInternalSize( Sizef( MyParent->mTotalWidth, MyParent->mRowHeight ) );
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
		UITable* MyParent = getParent()->getParent()->asType<UITable>();

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

}} // namespace EE::UI
