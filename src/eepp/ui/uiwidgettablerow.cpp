#include <eepp/ui/uiwidgettable.hpp>
#include <eepp/ui/uiwidgettablerow.hpp>

namespace EE { namespace UI {

UIWidgetTableRow* UIWidgetTableRow::New() {
	return eeNew( UIWidgetTableRow, () );
}

UIWidgetTableRow::UIWidgetTableRow() : UIWidget( "widgettablerow" ) {
	applyDefaultTheme();
}

UIWidgetTableRow::~UIWidgetTableRow() {
	if ( NULL != getEventDispatcher() ) {
		if ( getEventDispatcher()->getFocusNode() == this )
			mParentNode->setFocus();

		if ( getEventDispatcher()->getMouseOverNode() == this )
			getEventDispatcher()->setMouseOverNode( mParentNode );
	}
}

Uint32 UIWidgetTableRow::getType() const {
	return UI_TYPE_WIDGETTABLEROW;
}

bool UIWidgetTableRow::isType( const Uint32& type ) const {
	return UIWidgetTableRow::getType() == type ? true : UIWidget::isType( type );
}

void UIWidgetTableRow::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "tablerow" );

	onThemeLoaded();
}

UIWidgetTable* UIWidgetTableRow::gridParent() const {
	return mParentNode->getParent()->asType<UIWidgetTable>();
}

void UIWidgetTableRow::setColumn( const Uint32& columnIndex, UINode* node ) {
	eeASSERT( columnIndex < gridParent()->getColumnsCount() );

	UIWidgetTable* P = gridParent();

	mCells[columnIndex] = node;

	if ( node->getParent() != this )
		node->setParent( this );

	if ( node->isWidget() )
		static_cast<UIWidget*>( node )->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	node->setPosition( P->getColumnPosition( columnIndex ), 0 );
	node->setSize( P->getColumnWidth( columnIndex ), P->getRowHeight() );

	node->setVisible( true );
	node->setEnabled( true );
}

UINode* UIWidgetTableRow::getColumn( const Uint32& columnIndex ) const {
	eeASSERT( columnIndex < gridParent()->getColumnsCount() );

	return mCells[columnIndex];
}

void UIWidgetTableRow::updateRow() {
	onAutoSize();

	UIWidgetTable* P = gridParent();

	for ( Uint32 i = 0; i < mCells.size(); i++ ) {
		mCells[i]->setPosition( P->getColumnPosition( i ), 0 );
		mCells[i]->setSize( P->getColumnWidth( i ), P->getRowHeight() );
	}
}

void UIWidgetTableRow::select() {
	UIWidgetTable* MyParent = getParent()->getParent()->asType<UIWidgetTable>();

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

void UIWidgetTableRow::unselect() {
	if ( mNodeFlags & NODE_FLAG_SELECTED )
		mNodeFlags &= ~NODE_FLAG_SELECTED;

	popState( UIState::StateSelected );
}

bool UIWidgetTableRow::isSelected() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SELECTED );
}

Uint32 UIWidgetTableRow::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	UIWidget::onMouseLeave( Pos, Flags );

	if ( mNodeFlags & NODE_FLAG_SELECTED )
		pushState( UIState::StateSelected );

	return 1;
}

Uint32 UIWidgetTableRow::onMessage( const NodeMessage* Msg ) {
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
			UIWidgetTable* MyParent = getParent()->getParent()->asType<UIWidgetTable>();

			if ( ( Msg->getFlags() & EE_BUTTONS_WUWD ) &&
				 MyParent->getVerticalScrollBar()->isVisible() ) {
				MyParent->getVerticalScrollBar()->getSlider()->manageClick( Msg->getFlags() );
			}

			break;
		}
	}

	return 0;
}

void UIWidgetTableRow::onAutoSize() {
	UIWidgetTable* MyParent = getParent()->getParent()->asType<UIWidgetTable>();

	setInternalSize( Sizef( MyParent->mTotalWidth, MyParent->mRowHeight ) );
}

void UIWidgetTableRow::onStateChange() {
	UIWidget::onStateChange();

	if ( isSelected() && NULL != mSkinState &&
		 !( mSkinState->getState() & UIState::StateFlagSelected ) ) {
		pushState( UIState::StateSelected, false );
	}
}

void UIWidgetTableRow::onParentChange() {
	if ( NULL != getParent() && NULL != gridParent() )
		mCells.resize( gridParent()->getColumnsCount(), NULL );
}

void UIWidgetTableRow::onAlphaChange() {
	if ( mEnabled && mVisible ) {
		UIWidgetTable* MyParent = getParent()->getParent()->asType<UIWidgetTable>();

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
