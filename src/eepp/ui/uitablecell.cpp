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
		if ( getEventDispatcher()->getFocusNode() == this )
			mParentNode->setFocus();

		if ( getEventDispatcher()->getMouseOverNode() == this )
			getEventDispatcher()->setMouseOverNode( mParentNode );
	}
}

void UITableCell::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "gridcell" );

	onThemeLoaded();
}

UITable* UITableCell::gridParent() const {
	return mParentNode->getParent()->asType<UITable>();
}

void UITableCell::setCell( const Uint32& ColumnIndex, UINode* node ) {
	eeASSERT( ColumnIndex < gridParent()->getColumnsCount() );

	UITable* P = gridParent();

	mCells[ColumnIndex] = node;

	if ( node->getParent() != this )
		node->setParent( this );

	if ( node->isWidget() )
		static_cast<UIWidget*>( node )->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	node->setPosition( P->getCellPosition( ColumnIndex ), 0 );
	node->setSize( P->getColumnWidth( ColumnIndex ), P->getRowHeight() );

	node->setVisible( true );
	node->setEnabled( true );
}

UINode* UITableCell::getCell( const Uint32& ColumnIndex ) const {
	eeASSERT( ColumnIndex < gridParent()->getColumnsCount() );

	return mCells[ColumnIndex];
}

void UITableCell::fixCell() {
	onAutoSize();

	UITable* P = gridParent();

	for ( Uint32 i = 0; i < mCells.size(); i++ ) {
		mCells[i]->setPosition( P->getCellPosition( i ), 0 );
		mCells[i]->setSize( P->getColumnWidth( i ), P->getRowHeight() );
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
		case NodeMessage::MouseClick: {
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

	if ( isSelected() && NULL != mSkinState &&
		 !( mSkinState->getState() & UIState::StateFlagSelected ) ) {
		pushState( UIState::StateSelected, false );
	}
}

void UITableCell::onParentChange() {
	if ( NULL != getParent() && NULL != gridParent() )
		mCells.resize( gridParent()->getColumnsCount(), NULL );
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
