#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitableview.hpp>

#include <cmath>

namespace EE { namespace UI {

UITableView* UITableView::New() {
	return eeNew( UITableView, () );
}

UITableView::UITableView( const std::string& tag ) : UIAbstractTableView( tag ) {
	setClipType( ClipType::ContentBox );
}

UITableView::UITableView() : UITableView( "tableview" ) {}

Uint32 UITableView::getType() const {
	return UI_TYPE_TABLEVIEW;
}

bool UITableView::isType( const Uint32& type ) const {
	return UITableView::getType() == type ? true : UIAbstractTableView::isType( type );
}

void UITableView::drawChilds() {
	int realRowIndex = 0;
	int realColIndex = 0;
	ConditionalLock l( getModel() != nullptr, getModel() ? &getModel()->resourceMutex() : nullptr );
	buildRowHeader();
	if ( getModel() ) {
		Float rowHeight = getRowHeight();
		size_t start = mScrollOffset.y / rowHeight;
		size_t end = eemin<size_t>(
			(size_t)eeceil( ( mScrollOffset.y + mSize.getHeight() ) / rowHeight ), getItemCount() );
		Float yOffset = 0;
		Float xOffset;
		auto colCount = getModel()->columnCount();
		auto headerHeight = getHeaderHeight();
		for ( size_t i = start; i < end; i++ ) {
			xOffset = 0;
			yOffset = headerHeight + i * rowHeight;
			ModelIndex rowIndex( getModel()->index( i ) );
			if ( yOffset - mScrollOffset.y > mSize.getHeight() )
				break;
			if ( yOffset - mScrollOffset.y + rowHeight < 0 )
				continue;
			UITableRow* rowNode = updateRow( realRowIndex, rowIndex, yOffset );
			rowNode->setChildsVisibility( false, false );
			realColIndex = 0;
			for ( size_t colIndex = 0; colIndex < colCount; colIndex++ ) {
				auto& colData = columnData( colIndex );
				if ( !colData.visible || ( xOffset + colData.width ) - mScrollOffset.x < 0 ) {
					if ( colData.visible )
						xOffset += colData.width;
					continue;
				}
				if ( xOffset - mScrollOffset.x > mSize.getWidth() )
					break;
				xOffset += colData.width;
				updateCell( { realColIndex, realRowIndex },
							getModel()->index( rowIndex.row(), colIndex, rowIndex.parent() ), 0,
							yOffset );
				realColIndex++;
			}
			rowNode->nodeDraw();
			if ( mRowHeaderWidth ) {
				updateRowHeader( realRowIndex, rowIndex,
								 realRowIndex == 0 ? fmodf( -mScrollOffset.y, rowHeight ) : 0.f );
			}
			realRowIndex++;
		}
	}
	if ( mHeader && mHeader->isVisible() )
		mHeader->nodeDraw();
	if ( mRowHeader && mRowHeader->isVisible() )
		mRowHeader->nodeDraw();
	if ( mHScroll->isVisible() )
		mHScroll->nodeDraw();
	if ( mVScroll->isVisible() )
		mVScroll->nodeDraw();
}

Node* UITableView::overFind( const Vector2f& point ) {
	ScopedOp op( [this] { mUISceneNode->setIsLoading( true ); },
				 [this] { mUISceneNode->setIsLoading( false ); } );
	Node* pOver = NULL;
	if ( !mEnabled || !mVisible )
		return pOver;
	ConditionalLock l( getModel() != nullptr, getModel() ? &getModel()->resourceMutex() : nullptr );
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
		if ( mRowHeader && ( pOver = mRowHeader->overFind( point ) ) )
			return pOver;
		Float rowHeight = getRowHeight();
		Float headerHeight = getHeaderHeight();
		Float itemCount = getItemCount();
		int realIndex = 0;
		Float yOffset;
		size_t start = mScrollOffset.y / rowHeight;
		size_t end = eemin<size_t>(
			(size_t)eeceil( ( mScrollOffset.y + mSize.getHeight() ) / rowHeight ), itemCount );
		for ( size_t i = start; i < end; i++ ) {
			yOffset = headerHeight + i * rowHeight;
			ModelIndex index( getModel()->index( i ) );
			if ( yOffset - mScrollOffset.y > mSize.getHeight() )
				break;
			if ( yOffset - mScrollOffset.y + rowHeight < 0 )
				continue;
			pOver = updateRow( realIndex, index, yOffset )->overFind( point );
			if ( pOver )
				break;
			realIndex++;
		}
		if ( !pOver )
			pOver = this;
	}
	return pOver;
}

Float UITableView::getMaxColumnContentWidth( const size_t& colIndex, bool bestGuess ) {
	Float lWidth = 0;
	ConditionalLock l( getModel() != nullptr, getModel() ? &getModel()->resourceMutex() : nullptr );
	if ( nullptr == getModel() || getModel()->rowCount() == 0 )
		return lWidth;
	ScopedOp op( [this] { mUISceneNode->setIsLoading( true ); },
				 [this] { mUISceneNode->setIsLoading( false ); } );
	Float yOffset = getHeaderHeight();
	auto worstCaseFunc = [&]( const ModelIndex& index ) {
		UIWidget* widget = updateCell( { (Int64)0, (Int64)0 }, index, 0, yOffset );
		if ( widget->isType( UI_TYPE_PUSHBUTTON ) ) {
			Float w = widget->asType<UIPushButton>()->getContentSize().getWidth();
			if ( w > lWidth )
				lWidth = w;
		}
	};
	// TODO: Improve best guess
	if ( bestGuess && getItemCount() > 10 ) {
		Variant dataTest( getModel()->data( getModel()->index( 0, colIndex ) ) );
		if ( dataTest.isString() ) {
			std::map<size_t, ModelIndex> lengths;
			for ( size_t i = 0; i < getItemCount(); i++ ) {
				ModelIndex index( getModel()->index( i, colIndex ) );
				Variant data( getModel()->data( index ) );
				if ( !data.isValid() )
					continue;
				size_t length = data.size();
				if ( lengths.empty() || length > lengths.rbegin()->first )
					lengths[length] = index;
			}
			size_t i = 0;
			auto it = lengths.rbegin();
			for ( ; it != lengths.rend() && i < 10; it++, i++ )
				worstCaseFunc( it->second );
		} else {
			for ( size_t i = 0; i < getItemCount(); i++ )
				worstCaseFunc( getModel()->index( i, colIndex ) );
		}
	} else {
		for ( size_t i = 0; i < getItemCount(); i++ )
			worstCaseFunc( getModel()->index( i, colIndex ) );
	}
	return lWidth;
}

void UITableView::createOrUpdateColumns( bool resetColumnData ) {
	if ( !getModel() ) {
		updateContentSize();
		return;
	}
	UIAbstractTableView::createOrUpdateColumns( resetColumnData );
	updateContentSize();
}

void UITableView::updateContentSize() {
	Sizef oldSize( mContentSize );
	mContentSize = UIAbstractTableView::getContentSize();
	if ( oldSize != mContentSize )
		onContentSizeChange();
}

void UITableView::onColumnSizeChange( const size_t& colIndex, bool fromUserInteraction ) {
	UIAbstractTableView::onColumnSizeChange( colIndex, fromUserInteraction );
	updateContentSize();
}

Uint32 UITableView::onKeyDown( const KeyEvent& event ) {
	bool isJump = ( event.getKeyCode() == KEY_LEFT || event.getKeyCode() == KEY_RIGHT ||
					event.getKeyCode() == KEY_UP || event.getKeyCode() == KEY_DOWN ) &&
				  ( event.getSanitizedMod() & KeyMod::getDefaultModifier() );
	if ( !isJump && ( event.getMod() & KEYMOD_CTRL_SHIFT_ALT_META ) )
		return UIAbstractTableView::onKeyDown( event );
	auto curIndex = getSelection().first();
	int pageSize = eefloor( getVisibleArea().getHeight() / getRowHeight() ) - 1;

	switch ( event.getKeyCode() ) {
		case KEY_UP: {
			// Fallback to home if using modifier key
			if ( !( event.getSanitizedMod() & KeyMod::getDefaultModifier() ) ) {
				if ( !getModel() || isEditing() )
					return 0;
				auto& model = *this->getModel();
				ModelIndex foundIndex;
				if ( !getSelection().isEmpty() ) {
					auto oldIndex = getSelection().first();
					foundIndex = model.index( oldIndex.row() - 1, oldIndex.column() );
				} else {
					foundIndex = model.index(
						0, getSelection().first().isValid() ? getSelection().first().column() : 0 );
				}
				if ( model.isValid( foundIndex ) ) {
					Float curY = getHeaderHeight() + getRowHeight() * foundIndex.row();
					getSelection().set( foundIndex );
					if ( curY < mScrollOffset.y + getHeaderHeight() + getRowHeight() ||
						 curY > mScrollOffset.y + getPixelsSize().getHeight() - mPaddingPx.Top -
									mPaddingPx.Bottom - getRowHeight() ) {
						curY -= getHeaderHeight();
						mVScroll->setValue( curY / getScrollableArea().getHeight() );
					}
				}
				return 1;
			}
		}
		case KEY_HOME: {
			getSelection().set( getModel()->index(
				0, getSelection().first().isValid() ? getSelection().first().column() : 0 ) );
			scrollToTop();
			return 1;
		}
		case KEY_PAGEUP: {
			if ( curIndex.row() - pageSize < 0 ) {
				getSelection().set( getModel()->index(
					0, getSelection().first().isValid() ? getSelection().first().column() : 0 ) );
				scrollToTop();
			} else {
				moveSelection( -pageSize );
			}
			return 1;
		}
		case KEY_DOWN: {
			if ( !getModel() || isEditing() )
				return 0;
			// Fallback to end if using modifier key
			if ( !( event.getSanitizedMod() & KeyMod::getDefaultModifier() ) ) {
				auto& model = *this->getModel();
				ModelIndex foundIndex;
				if ( !getSelection().isEmpty() ) {
					auto oldIndex = getSelection().first();
					foundIndex = model.index( oldIndex.row() + 1, oldIndex.column() );
				} else {
					foundIndex = model.index(
						0, getSelection().first().isValid() ? getSelection().first().column() : 0 );
				}
				if ( model.isValid( foundIndex ) ) {
					Float curY = getHeaderHeight() + getRowHeight() * foundIndex.row();
					getSelection().set( foundIndex );
					if ( curY < mScrollOffset.y ||
						 curY > mScrollOffset.y + getPixelsSize().getHeight() - mPaddingPx.Top -
									mPaddingPx.Bottom - getRowHeight() ) {
						curY -= eefloor( getVisibleArea().getHeight() / getRowHeight() ) *
									getRowHeight() -
								getRowHeight();
						mVScroll->setValue( curY / getScrollableArea().getHeight() );
					}
				}
				return 1;
			}
		}
		case KEY_END: {
			getSelection().set( getModel()->index(
				getItemCount() - 1,
				getSelection().first().isValid() ? getSelection().first().column() : 0 ) );
			scrollToBottom();
			return 1;
		}
		case KEY_PAGEDOWN: {
			if ( curIndex.row() + pageSize >= (Int64)getModel()->rowCount() ) {
				getSelection().set( getModel()->index( getItemCount() - 1 ) );
				scrollToBottom();
			} else {
				moveSelection( pageSize );
			}
			return 1;
		}
		case KEY_LEFT: {
			if ( !getModel() || getSelectionType() != SelectionType::Cell )
				return 0;
			auto& model = *this->getModel();
			ModelIndex foundIndex;
			if ( !getSelection().isEmpty() ) {
				auto oldIndex = getSelection().first();
				if ( oldIndex.column() == 0 )
					return 1;
				foundIndex = model.index( oldIndex.row(), oldIndex.column() - 1 );
			} else {
				foundIndex = model.index(
					0, getSelection().first().isValid() ? getSelection().first().column() : 0 );
			}

			if ( event.getSanitizedMod() & KeyMod::getDefaultModifier() )
				foundIndex = model.index( foundIndex.row(), 0 );

			if ( model.isValid( foundIndex ) ) {
				Float curX = getColumnPosition( foundIndex.column() ).x;
				getSelection().set( foundIndex );
				if ( curX < mScrollOffset.x || curX > mScrollOffset.x + getPixelsSize().getWidth() -
														  mPaddingPx.Left - mPaddingPx.Right ) {
					Float colWidth = getColumnWidth( foundIndex.column() );
					mHScroll->setValue(
						( event.getSanitizedMod() & KEYMOD_CTRL )
							? 0
							: ( mHScroll->getValue() - colWidth / getScrollableArea().getWidth() ) *
								  getScrollableArea().getWidth() / getScrollableArea().getWidth() );
				}
			}
			return 0;
		}
		case KEY_RIGHT: {
			if ( !getModel() || getSelectionType() != SelectionType::Cell )
				return 0;
			auto& model = *this->getModel();
			ModelIndex foundIndex;
			if ( !getSelection().isEmpty() ) {
				auto oldIndex = getSelection().first();
				foundIndex = model.index( oldIndex.row(), oldIndex.column() + 1 );
			} else {
				foundIndex = model.index(
					0, getSelection().first().isValid() ? getSelection().first().column() : 0 );
			}

			if ( event.getSanitizedMod() & KeyMod::getDefaultModifier() ) {
				foundIndex = model.index( foundIndex.row(),
										  model.columnCount() > 0 ? model.columnCount() - 1 : 0 );
			}

			if ( model.isValid( foundIndex ) ) {
				Float colWidth = getColumnWidth( foundIndex.column() );
				Float curX = getColumnPosition( foundIndex.column() ).x + colWidth;
				getSelection().set( foundIndex );
				if ( curX < mScrollOffset.x || curX > mScrollOffset.x + getPixelsSize().getWidth() -
														  mPaddingPx.Left - mPaddingPx.Right ) {
					mHScroll->setValue(
						( event.getSanitizedMod() & KEYMOD_CTRL )
							? getScrollableArea().getWidth()
							: ( mHScroll->getValue() + colWidth / getScrollableArea().getWidth() ) *
								  getScrollableArea().getWidth() / getScrollableArea().getWidth() );
				}
			}
			return 0;
		}
		case KEY_RETURN:
		case KEY_KP_ENTER: {
			if ( tryBeginEditing( KeyBindings::Shortcut{ event.getKeyCode(), event.getMod() } ) )
				return 1;
			if ( curIndex.isValid() )
				onOpenModelIndex( curIndex, &event );
			return 1;
		}
		case KEY_MENU:
		case KEY_APPLICATION: {
			if ( curIndex.isValid() )
				onOpenMenuModelIndex( curIndex, &event );
			return 1;
		}
		default:
			break;
	}
	return UIAbstractTableView::onKeyDown( event );
}

ModelIndex UITableView::findRowWithText( const std::string& text, const bool& caseSensitive,
										 const bool& exactMatch ) const {
	const Model* model = getModel();
	ConditionalLock l( getModel() != nullptr,
					   getModel() ? &const_cast<Model*>( getModel() )->resourceMutex() : nullptr );
	if ( !model || model->rowCount() == 0 )
		return {};
	size_t rc = model->rowCount();
	for ( size_t i = 0; i < rc; i++ ) {
		ModelIndex index = model->index(
			i, model->keyColumn() != -1 ? model->keyColumn()
										: ( model->treeColumn() >= 0 ? model->treeColumn() : 0 ) );
		Variant var = model->data( index );
		if ( var.isValid() &&
			 ( exactMatch ? var.toString() == text
						  : String::startsWith( caseSensitive ? var.toString()
															  : String::toLower( var.toString() ),
												caseSensitive ? text : String::toLower( text ) ) ) )
			return model->index( index.row(), 0 );
	}
	return {};
}

}} // namespace EE::UI
