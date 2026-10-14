#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/window/input.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <string_view>

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

void UITableView::drawChildren() {
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
		const auto& columnOrder = getColumnOrder();
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
			rowNode->setChildrenVisibility( false, false );
			realColIndex = 0;
			if ( mFocusSelectionDirty && !hasFocusWithin() ) {
				// Something else took the focus meanwhile (a context menu opened by the click
				// that selected): it keeps it.
				mFocusSelectionDirty = false;
			}
			bool focusRow = mFocusSelectionDirty && rowIndex.row() == getSelection().first().row();
			for ( size_t colIndex : columnOrder ) {
				auto& colData = columnData( colIndex );
				if ( !colData.visible || ( xOffset + colData.width ) - mScrollOffset.x < 0 ) {
					if ( colData.visible )
						xOffset += colData.width;
					continue;
				}
				if ( xOffset - mScrollOffset.x > mSize.getWidth() )
					break;
				xOffset += colData.width;
				UIWidget* cell = updateCell(
					{ realColIndex, realRowIndex },
					getModel()->index( rowIndex.row(), colIndex, rowIndex.parent() ), 0, yOffset );
				// The selected row's first visible cell takes the focus (see setFocusOnSelection).
				if ( focusRow && cell ) {
					cell->setFocus();
					mFocusSelectionDirty = false;
					focusRow = false;
				}
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
	if ( mRubberBand && mRubberBand->isVisible() )
		mRubberBand->nodeDraw();
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
	if ( nullptr == getModel() || !getModel()->hasChildren() )
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
	// Shift + Up / Down / Page Up / Page Down / Home / End extend the selection from its anchor.
	Uint32 mod = event.getSanitizedMod();
	if ( ( mod & KEYMOD_SHIFT ) && !( mod & ( KEYMOD_CTRL | KEYMOD_ALT | KEYMOD_META ) ) &&
		 mSelectionKind == SelectionKind::Multiple && isRowSelection() && getModel() &&
		 !isEditing() && getItemCount() > 0 ) {
		Int64 rows = static_cast<Int64>( getItemCount() );
		ModelIndex cursor = mSelectionCursor.isValid() && getModel()->isValid( mSelectionCursor )
								? mSelectionCursor
								: getSelection().first();
		Int64 row = cursor.isValid() ? cursor.row() : 0;
		Int64 page =
			std::max<Int64>( 1, eefloor( getVisibleArea().getHeight() / getRowHeight() ) - 1 );
		std::optional<Int64> target;
		switch ( event.getKeyCode() ) {
			case KEY_UP:
				target = row - 1;
				break;
			case KEY_DOWN:
				target = row + 1;
				break;
			case KEY_PAGEUP:
				target = row - page;
				break;
			case KEY_PAGEDOWN:
				target = row + page;
				break;
			case KEY_HOME:
				target = 0;
				break;
			case KEY_END:
				target = rows - 1;
				break;
			default:
				break;
		}
		if ( target ) {
			ModelIndex index = getModel()->index( std::clamp<Int64>( *target, 0, rows - 1 ),
												  cursor.isValid() ? cursor.column() : 0 );
			extendSelectionTo( index );
			scrollToIndex( index );
			return 1;
		}
	}
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

bool UITableView::getFocusOnSelection() const {
	return mFocusOnSelection;
}

void UITableView::setFocusOnSelection( bool focusOnSelection ) {
	mFocusOnSelection = focusOnSelection;
	if ( !mFocusOnSelection )
		mFocusSelectionDirty = false;
}

void UITableView::setTableFlags( Uint32 flags ) {
	UIAbstractTableView::setTableFlags( flags );
	setFocusOnSelection( flags & TableFlagFocusOnSelection );
	setRubberBandSelection( flags & TableFlagRubberBandSelection );
}

bool UITableView::getRubberBandSelection() const {
	return mRubberBandSelection;
}

void UITableView::setRubberBandSelection( bool enabled ) {
	mRubberBandSelection = enabled;
	if ( enabled )
		mTableFlags |= TableFlagRubberBandSelection;
	else
		mTableFlags &= ~TableFlagRubberBandSelection;
	if ( !enabled )
		endRubberBand();
}

Uint32 UITableView::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	// Presses reach the view itself only on its empty area (rows, header and scroll bars are
	// children).
	// Only the initial press: eepp keeps sending presses to whatever is under the pointer while
	// the button is held (a band dragged from another table must not start one here).
	EventDispatcher* dispatcher = getEventDispatcher();
	if ( ( flags & EE_BUTTON_LMASK ) && !mRubberBandPressed && dispatcher &&
		 dispatcher->justPressTriggered( EE_BUTTON_LMASK ) ) {
		mRubberBandDragged = false;
		if ( mRubberBandSelection && mSelectionKind == SelectionKind::Multiple &&
			 isRowSelection() && getModel() && !isEditing() ) {
			Vector2f local( convertToNodeSpace( position.asFloat() ) );
			if ( local.y >= getHeaderHeight() ) {
				mRubberBandPressed = true;
				mRubberBandStart = local + mScrollOffset;
				Uint32 mod = getInput()->getSanitizedModState();
				mRubberBandBase.clear();
				if ( mod & ( KeyMod::getDefaultModifier() | KEYMOD_SHIFT ) )
					mRubberBandBase = getSelection().indexes();
				subscribeScheduledUpdate();
			}
		}
	}
	return UIAbstractTableView::onMouseDown( position, flags );
}

void UITableView::scheduledUpdate( const Time& time ) {
	UIAbstractTableView::scheduledUpdate( time );
	if ( !mRubberBandPressed )
		return;
	EventDispatcher* dispatcher = getEventDispatcher();
	if ( !dispatcher || !( dispatcher->getPressTrigger() & EE_BUTTON_LMASK ) ) {
		endRubberBand();
		return;
	}
	Vector2f local( convertToNodeSpace( dispatcher->getMousePosf() ) );
	if ( !mRubberBandDragged ) {
		Vector2f moved = local + mScrollOffset - mRubberBandStart;
		if ( std::abs( moved.x ) < PixelDensity::dpToPx( 4 ) &&
			 std::abs( moved.y ) < PixelDensity::dpToPx( 4 ) )
			return;
		mRubberBandDragged = true;
	}
	// Past the top (below the header) or the bottom edge: scroll toward the pointer.
	Float scrollable = getScrollableArea().getHeight();
	if ( scrollable > 0 && mVScroll ) {
		Float top = getHeaderHeight();
		Float bottom = getPixelsSize().getHeight();
		Float delta = 0;
		if ( local.y < top )
			delta = local.y - top;
		else if ( local.y > bottom )
			delta = local.y - bottom;
		if ( delta != 0 )
			mVScroll->setValue(
				std::clamp( ( mScrollOffset.y + delta * 0.5f ) / scrollable, 0.f, 1.f ) );
	}
	updateRubberBand( local );
}

void UITableView::updateRubberBand( const Vector2f& localPosition ) {
	if ( !getModel() )
		return;
	Vector2f current = localPosition + mScrollOffset;
	Vector2f min( std::min( mRubberBandStart.x, current.x ),
				  std::min( mRubberBandStart.y, current.y ) );
	Vector2f max( std::max( mRubberBandStart.x, current.x ),
				  std::max( mRubberBandStart.y, current.y ) );
	// Rows touched by the band (row y in content coordinates starts below the header).
	Float header = getHeaderHeight();
	Float rowHeight = getRowHeight();
	Int64 count = static_cast<Int64>( getItemCount() );
	Int64 first = -1;
	Int64 last = -1;
	if ( count > 0 && rowHeight > 0 && max.y >= header ) {
		first = std::max<Int64>( 0, static_cast<Int64>( ( min.y - header ) / rowHeight ) );
		last = std::min<Int64>( count - 1, static_cast<Int64>( ( max.y - header ) / rowHeight ) );
		if ( first > last )
			first = last = -1;
	}
	// This runs every frame while the button is held: the selection (and its notifications) only
	// changes when the band covers other rows.
	bool selectionChanged = first != mRubberBandFirst || last != mRubberBandLast;
	if ( selectionChanged ) {
		mRubberBandFirst = first;
		mRubberBandLast = last;
		std::vector<ModelIndex> selection( mRubberBandBase );
		if ( first >= 0 ) {
			selection.reserve( selection.size() + ( last - first + 1 ) );
			for ( Int64 row = first; row <= last; ++row ) {
				ModelIndex index = getModel()->index( row );
				if ( mRubberBandBase.empty() ||
					 std::find( mRubberBandBase.begin(), mRubberBandBase.end(), index ) ==
						 mRubberBandBase.end() )
					selection.push_back( index );
			}
		}
		if ( selection.empty() )
			getSelection().clear();
		else
			getSelection().set( selection );
	}

	if ( !mRubberBand ) {
		mRubberBand = UIWidget::NewWithTag( mTag + "::rubberband" );
		mRubberBand->setParent( this );
		mRubberBand->setEnabled( false );
	}
	// Drawn within the rows area: the band (in view coordinates) intersected with it.
	Float left = std::max( min.x - mScrollOffset.x, 0.f );
	Float top = std::max( min.y - mScrollOffset.y, header );
	Float right = std::min( max.x - mScrollOffset.x, getPixelsSize().getWidth() );
	Float bottom = std::min( max.y - mScrollOffset.y, getPixelsSize().getHeight() );
	Vector2f position( left, top );
	Sizef size( std::max( left, right ) - left, std::max( top, bottom ) - top );
	if ( !selectionChanged && mRubberBand->isVisible() &&
		 mRubberBand->getPixelsPosition() == position && mRubberBand->getPixelsSize() == size )
		return;
	mRubberBand->setPixelsPosition( position );
	mRubberBand->setPixelsSize( size );
	mRubberBand->setVisible( true );
	invalidateDraw();
}

void UITableView::endRubberBand() {
	mRubberBandPressed = false;
	mRubberBandBase.clear();
	mRubberBandFirst = mRubberBandLast = -2;
	if ( mRubberBand && mRubberBand->isVisible() ) {
		mRubberBand->setVisible( false );
		invalidateDraw();
	}
	if ( isSubscribedForScheduledUpdate() )
		unsubscribeScheduledUpdate();
}

void UITableView::onModelSelectionChange() {
	UIAbstractTableView::onModelSelectionChange();
	// Only while the table has the keyboard: a selection changed by code must not take the focus
	// from elsewhere.
	if ( mFocusOnSelection && !getSelection().isEmpty() && hasFocusWithin() )
		mFocusSelectionDirty = true;
}

ModelIndex UITableView::findRowWithText( const std::string& text, const bool& caseSensitive,
										 FindRowWithTextMatchKind matchKind ) const {
	const Model* model = getModel();
	ConditionalLock l( getModel() != nullptr,
					   getModel() ? &const_cast<Model*>( getModel() )->resourceMutex() : nullptr );
	if ( !model || !model->hasChildren() )
		return {};
	size_t rc = model->rowCount();
	for ( size_t i = 0; i < rc; i++ ) {
		ModelIndex index = model->index( i, model->searchColumn() );
		Variant var = model->data( index );
		if ( var.isValid() ) {
			std::string convertedValue;
			const std::string_view value =
				var.isStdStringLike() ? var.asStdStringView()
									  : std::string_view{ convertedValue = var.toString() };
			bool matches = false;
			switch ( matchKind ) {
				case Abstract::UIAbstractView::FindRowWithTextMatchKind::Equals:
					matches = value == text;
					break;
				case Abstract::UIAbstractView::FindRowWithTextMatchKind::StartsWith:
					matches = caseSensitive ? String::startsWith( value, text )
											: String::istartsWith( value, text );
					break;
				case Abstract::UIAbstractView::FindRowWithTextMatchKind::Contains:
					matches = caseSensitive ? String::contains( value, text )
											: String::icontains( value, text );
					break;
			}

			if ( matches )
				return model->index( index.row(), 0 );
		}
	}
	return {};
}

}} // namespace EE::UI
