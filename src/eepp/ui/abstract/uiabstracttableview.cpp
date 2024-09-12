#include <eepp/system/thread.hpp>
#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

namespace EE { namespace UI { namespace Abstract {

UIAbstractTableView::UIAbstractTableView( const std::string& tag ) :
	UIAbstractView( tag ),
	mDragBorderDistance( PixelDensity::dpToPx( 4 ) ),
	mIconSize( PixelDensity::dpToPxI( 12 ) ),
	mSortIconSize( PixelDensity::dpToPxI( 20 ) ) {
	mHeader = UILinearLayout::NewWithTag( mTag + "::header", UIOrientation::Horizontal );
	mHeader->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mHeader->setParent( this )->setVisible( true )->setEnabled( true );
	mVScroll->on( Event::OnAlphaChange, [this]( const Event* ) {
		if ( mVScroll->getAlpha() == 0.f || mVScroll->getAlpha() == 1.f )
			updateColumnsWidth();
	} );
}

UIAbstractTableView::~UIAbstractTableView() {}

Uint32 UIAbstractTableView::getType() const {
	return UI_TYPE_ABSTRACTTABLEVIEW;
}

bool UIAbstractTableView::isType( const Uint32& type ) const {
	return UIAbstractTableView::getType() == type ? true : UIAbstractView::isType( type );
}

Float UIAbstractTableView::getRowHeight() const {
	return mRowHeight != 0 ? mRowHeight
						   : ( eeceil( columnData( 0 ).widget
										   ? columnData( 0 ).widget->getPixelsSize().getHeight()
										   : PixelDensity::dpToPx( 16 ) ) );
}

void UIAbstractTableView::setRowHeight( const Float& rowHeight ) {
	if ( mRowHeight != rowHeight ) {
		mRowHeight = rowHeight;
		createOrUpdateColumns( false );
	}
}

void UIAbstractTableView::setColumnWidth( const size_t& colIndex, const Float& width ) {
	if ( columnData( colIndex ).width != width ) {
		columnData( colIndex ).width = width;
		updateHeaderSize();
		onColumnSizeChange( colIndex );
		createOrUpdateColumns( false );
	}
}

void UIAbstractTableView::setColumnsWidth( const Float& width ) {
	if ( !getModel() )
		return;
	for ( size_t i = 0; i < getModel()->columnCount(); i++ ) {
		columnData( i ).width = width;
		onColumnSizeChange( i );
	}
	updateHeaderSize();
	createOrUpdateColumns( false );
}

const Float& UIAbstractTableView::getColumnWidth( const size_t& colIndex ) const {
	return columnData( colIndex ).width;
}

void UIAbstractTableView::selectAll() {
	getSelection().clear();
	for ( size_t itemIndex = 0; itemIndex < getItemCount(); ++itemIndex ) {
		auto index = getModel()->index( itemIndex );
		getSelection().add( index );
	}
}

size_t UIAbstractTableView::getItemCount() const {
	if ( !getModel() )
		return 0;
	return getModel()->rowCount();
}

void UIAbstractTableView::onModelUpdate( unsigned flags ) {
	if ( !Engine::instance()->isMainThread() ) {
		static constexpr String::HashType tag = String::hash( "onModelUpdate" );
		removeActionsByTag( tag );
		runOnMainThread(
			[this, flags] {
				modelUpdate( flags );
				createOrUpdateColumns( true );
			},
			Time::Zero, tag );
	} else {
		UIAbstractView::onModelUpdate( flags );
		createOrUpdateColumns( true );
	}
}

void UIAbstractTableView::resetColumnData() {
	Model* model = getModel();
	if ( !model )
		return;
	size_t count = model->columnCount();
	for ( size_t i = 0; i < count; i++ ) {
		ColumnData& col = columnData( i );
		col.minWidth = 0;
	}
}

void UIAbstractTableView::createOrUpdateColumns( bool resetColumnData ) {
	Model* model = getModel();
	if ( !model )
		return;

	size_t count = model->columnCount();
	Float totalWidth = 0;
	auto visibleColCount = visibleColumnCount();

	if ( resetColumnData )
		this->resetColumnData();

	for ( size_t i = 0; i < count; i++ ) {
		ColumnData& col = columnData( i );
		if ( !col.widget ) {
			col.widget = eeNew( UITableHeaderColumn, ( mTag, this, i ) );
			col.widget->setParent( mHeader );
			col.widget->setEnabled( true );
			col.widget->setVisible( true );
		}
		col.visible = !isColumnHidden( i );
		col.widget->setVisible( col.visible );
		if ( !col.visible )
			continue;
		if ( col.minWidth == 0 ) {
			col.widget->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
			col.widget->setText( model->columnName( i ) );
			col.widget->reloadStyle( true, true, true );
			col.minWidth = col.widget->getPixelsSize().getWidth();
			col.minHeight = col.widget->getPixelsSize().getHeight();
		}
		col.width = eeceil( eemax( col.width, col.minWidth ) );
		col.widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		col.widget->setPixelsSize( col.width, getHeaderHeight() );
	}

	if ( mAutoColumnsWidth && visibleColCount > 1 ) {
		Float contentWidth = getContentSpaceWidth();
		bool shouldVScrollBeVisible = shouldVerticalScrollBeVisible();
		if ( !mVScroll->isVisible() && shouldVScrollBeVisible )
			contentWidth -= getVerticalScrollBar()->getPixelsSize().getWidth();
		else if ( mVScroll->isVisible() && !shouldVScrollBeVisible )
			contentWidth += getVerticalScrollBar()->getPixelsSize().getWidth();
		Float usedWidth = 0;
		for ( size_t col = 0; col < count; col++ ) {
			if ( col != mMainColumn && !isColumnHidden( col ) ) {
				Float colWidth = getMaxColumnContentWidth( col, true );
				colWidth = eemax( colWidth, columnData( col ).widget->getPixelsSize().getWidth() );
				usedWidth += colWidth;
				columnData( col ).width = colWidth;
			}
		}
		Float mainColMaxWidth = getMaxColumnContentWidth( mMainColumn, true );
		columnData( mMainColumn ).width = contentWidth - usedWidth >= mainColMaxWidth
											  ? contentWidth - usedWidth
											  : mainColMaxWidth;
		usedWidth += columnData( mMainColumn ).width;
		if ( mFitAllColumnsToWidget && usedWidth > contentWidth ) {
			size_t longestCol = 0;
			Float longestColWidth = columnData( 0 ).width;
			for ( size_t col = 1; col < count; col++ ) {
				if ( columnData( col ).width > longestColWidth ) {
					longestCol = col;
					longestColWidth = columnData( col ).width;
				}
			}
			longestColWidth = contentWidth - ( usedWidth - longestColWidth );
			if ( longestColWidth > 0 )
				columnData( longestCol ).width = longestColWidth;
		}
	}

	mHeaderHeight = 0;
	for ( size_t i = 0; i < count; i++ ) {
		ColumnData& col = columnData( i );
		if ( !col.visible )
			continue;
		mHeaderHeight = eemax( mHeaderHeight, col.minHeight );
	}

	for ( size_t i = 0; i < count; i++ ) {
		ColumnData& col = columnData( i );
		if ( !col.visible )
			continue;
		col.width = eeceil( eemax( col.width, col.minWidth ) );
		col.widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		col.widget->setPixelsSize( col.width, getHeaderHeight() );
		totalWidth += col.width;
	}

	if ( count < mColumn.size() ) {
		for ( size_t i = count; i < mColumn.size(); i++ ) {
			ColumnData& col = columnData( i );
			col.width = 0;
			col.visible = false;
			if ( col.widget ) {
				col.widget->close();
				col.widget = nullptr;
			}
		}
	}

	mHeader->setPixelsSize( totalWidth, getHeaderHeight() );
	bool visible = mHeader->isVisible();
	mHeader->setVisible( true );
	mHeader->updateLayout();
	mHeader->setVisible( visible );

	updateColumnsWidth();
}

Float UIAbstractTableView::getHeaderHeight() const {
	return areHeadersVisible()
			   ? eeceil( columnData( 0 ).widget
							 ? eemax( mHeaderHeight,
									  columnData( 0 ).widget->getPixelsSize().getHeight() )
							 : 16 )
			   : 0;
}

Sizef UIAbstractTableView::getContentSize() const {
	if ( !getModel() )
		return {};
	size_t count = getModel()->columnCount();
	Sizef size( mRowHeaderWidth, 0.f );
	for ( size_t i = 0; i < count; i++ )
		if ( !isColumnHidden( i ) )
			size.x += columnData( i ).width;
	size.y = getHeaderHeight();
	size.y += getItemCount() * getRowHeight();
	return size;
}

bool UIAbstractTableView::areHeadersVisible() const {
	return mHeader->isVisible();
}

void UIAbstractTableView::setHeadersVisible( bool visible ) {
	mHeader->setVisible( visible );
}

void UIAbstractTableView::onSizeChange() {
	UIAbstractView::onSizeChange();
	createOrUpdateColumns( false );
}

void UIAbstractTableView::onColumnSizeChange( const size_t&, bool fromUserInteraction ) {
	if ( fromUserInteraction && mAutoColumnsWidth )
		mAutoColumnsWidth = false;
}

Float UIAbstractTableView::getMaxColumnContentWidth( const size_t&, bool ) {
	return 0;
}

void UIAbstractTableView::onColumnResizeToContent( const size_t& colIndex ) {
	columnData( colIndex ).width = getMaxColumnContentWidth( colIndex, true );
	createOrUpdateColumns( false );
}

void UIAbstractTableView::updateHeaderSize() {
	if ( !getModel() )
		return;
	size_t count = getModel()->columnCount();
	Float totalWidth = 0;
	for ( size_t i = 0; i < count; i++ ) {
		const ColumnData& col = columnData( i );
		totalWidth += col.width;
	}
	mHeader->setPixelsSize( totalWidth, getHeaderHeight() );
}

int UIAbstractTableView::visibleColumn() {
	for ( size_t i = 0; i < getModel()->columnCount(); i++ ) {
		if ( columnData( i ).visible )
			return i;
	}
	return -1;
}

bool UIAbstractTableView::getAutoExpandOnSingleColumn() const {
	return mAutoExpandOnSingleColumn;
}

void UIAbstractTableView::setAutoExpandOnSingleColumn( bool autoExpandOnSingleColumn ) {
	if ( autoExpandOnSingleColumn != mAutoExpandOnSingleColumn ) {
		mAutoExpandOnSingleColumn = autoExpandOnSingleColumn;
		updateColumnsWidth();
	}
}

void UIAbstractTableView::columnResizeToContent( const size_t& colIndex ) {
	onColumnResizeToContent( colIndex );
}

Float UIAbstractTableView::getContentSpaceWidth() const {
	return eefloor(
		getPixelsSize().getWidth() - getPixelsPadding().Left - getPixelsPadding().Right -
		( mVScroll->isVisible() && ( mScrollViewType == Exclusive || mVScroll->getAlpha() != 0.f )
			  ? mVScroll->getPixelsSize().getWidth()
			  : 0 ) );
}

void UIAbstractTableView::updateColumnsWidth() {
	if ( mAutoExpandOnSingleColumn || mAutoColumnsWidth ) {
		int col = 0;
		if ( visibleColumnCount() == 1 && ( col = visibleColumn() ) != -1 ) {
			Float width = eemax( getContentSpaceWidth(), getMaxColumnContentWidth( col, true ) );
			bool shouldVScrollBeVisible = shouldVerticalScrollBeVisible();
			if ( mScrollViewType == Exclusive || mVScroll->getAlpha() != 0.f ) {
				if ( !mVScroll->isVisible() && shouldVScrollBeVisible )
					width -= getVerticalScrollBar()->getPixelsSize().getWidth();
				else if ( mVScroll->isVisible() && !shouldVScrollBeVisible )
					width += getVerticalScrollBar()->getPixelsSize().getWidth();
			}
			if ( columnData( col ).width != width ) {
				columnData( col ).width = width;
				updateHeaderSize();
				onColumnSizeChange( col );
			}
		}
	}
}

Uint32 UIAbstractTableView::onFocus( NodeFocusReason reason ) {
	if ( !Sys::isMobile() )
		getUISceneNode()->getWindow()->startTextInput();
	return UIAbstractView::onFocus( reason );
}

Uint32 UIAbstractTableView::onFocusLoss() {
	if ( !Sys::isMobile() )
		getUISceneNode()->getWindow()->stopTextInput();
	return UIAbstractView::onFocusLoss();
}

const Float& UIAbstractTableView::getDragBorderDistance() const {
	return mDragBorderDistance;
}

void UIAbstractTableView::setDragBorderDistance( const Float& dragBorderDistance ) {
	mDragBorderDistance = dragBorderDistance;
}

Vector2f UIAbstractTableView::getColumnPosition( const size_t& index ) {
	return columnData( index ).widget->getPixelsPosition();
}

int UIAbstractTableView::visibleColumnCount() const {
	if ( !getModel() )
		return 0;
	int count = 0;
	for ( size_t i = 0; i < getModel()->columnCount(); i++ ) {
		if ( columnData( i ).visible )
			count++;
	}
	return count;
}

UIAbstractTableView::ColumnData& UIAbstractTableView::columnData( const size_t& column ) const {
	if ( column >= mColumn.size() )
		mColumn.resize( column + 1 );
	return mColumn[column];
}

bool UIAbstractTableView::isColumnHidden( const size_t& column ) const {
	return !columnData( column ).visible;
}

void UIAbstractTableView::setColumnHidden( const size_t& column, bool hidden ) {
	if ( columnData( column ).visible != !hidden ) {
		columnData( column ).visible = !hidden;
		createOrUpdateColumns( false );
	}
}

void UIAbstractTableView::setColumnsHidden( const std::vector<size_t>& columns, bool hidden ) {
	for ( auto col : columns )
		columnData( col ).visible = !hidden;
	createOrUpdateColumns( false );
}

void UIAbstractTableView::setColumnsVisible( const std::vector<size_t>& columns ) {
	if ( !getModel() )
		return;

	// Check if the columns visible are the same
	if ( !mColumn.empty() && !columns.empty() ) {
		if ( mColumn.size() < 64 ) {
			Uint64 colFlags = 0;
			Uint64 newColFlags = 0;
			for ( size_t i = 0; i < mColumn.size(); ++i ) {
				if ( mColumn[i].visible )
					colFlags |= 1 << i;
			}

			for ( auto col : columns )
				newColFlags |= 1 << col;

			if ( colFlags == newColFlags )
				return;
		} else {
			// Should use a dynamic_bitset
			std::vector<bool> colFlags( mColumn.size() );
			std::vector<bool> newColFlags( mColumn.size() );

			for ( size_t i = 0; i < mColumn.size(); ++i )
				colFlags[i] = mColumn[i].visible;

			for ( size_t col : columns )
				newColFlags[col] = true;

			if ( colFlags == newColFlags )
				return;
		}
	}

	for ( size_t i = 0; i < getModel()->columnCount(); i++ )
		columnData( i ).visible = false;

	bool foundMainColumn = false;
	for ( auto col : columns ) {
		columnData( col ).visible = true;
		if ( col == mMainColumn )
			foundMainColumn = true;
	}

	if ( !foundMainColumn && !columns.empty() )
		mMainColumn = columns[0];

	createOrUpdateColumns( true );
}

UITableRow* UIAbstractTableView::createRow() {
	mUISceneNode->invalidateStyle( this );
	mUISceneNode->invalidateStyleState( this, true );
	UITableRow* rowWidget = UITableRow::New( mTag + "::row" );
	rowWidget->setParent( this );
	rowWidget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	rowWidget->reloadStyle( true, true, true );
	rowWidget->on( Event::MouseDown, [this]( const Event* event ) {
		if ( !( event->asMouseEvent()->getFlags() & ( EE_BUTTON_LMASK | EE_BUTTON_RMASK ) ) ||
			 !isRowSelection() ||
			 !getUISceneNode()->getEventDispatcher()->justPressTriggered( EE_BUTTON_LMASK |
																		  EE_BUTTON_RMASK ) )
			return;
		auto index = event->getNode()->asType<UITableRow>()->getCurIndex();
		if ( mSelectionKind == SelectionKind::Single &&
			 getUISceneNode()->getWindow()->getInput()->getSanitizedModState() &
				 KeyMod::getDefaultModifier() ) {
			getSelection().remove( index );
		} else {
			if ( mSelectionKind == SelectionKind::Multiple &&
				 getUISceneNode()->getWindow()->getInput()->getSanitizedModState() &
					 KeyMod::getDefaultModifier() ) {
				getSelection().toggle( index );
			} else {
				getSelection().set( index );
			}
		}
	} );
	onRowCreated( rowWidget );
	return rowWidget;
}

UITableRow* UIAbstractTableView::updateRow( const int& rowIndex, const ModelIndex& index,
											const Float& yOffset ) {
	if ( rowIndex >= (int)mRows.size() )
		mRows.resize( rowIndex + 1, nullptr );
	UITableRow* rowWidget = nullptr;
	if ( mRows[rowIndex] == nullptr ) {
		rowWidget = createRow();
		mRows[rowIndex] = rowWidget;
	} else {
		rowWidget = mRows[rowIndex];
	}
	rowWidget->setCurIndex( index );
	rowWidget->setPixelsSize( getContentSize().getWidth(), getRowHeight() );
	rowWidget->setPixelsPosition(
		{ mRowHeaderWidth + -mScrollOffset.x, yOffset - mScrollOffset.y } );
	if ( isRowSelection() ) {
		if ( getSelection().contains( index ) ) {
			rowWidget->pushState( UIState::StateSelected );
		} else {
			rowWidget->popState( UIState::StateSelected );
		}
	}
	return rowWidget;
}

void UIAbstractTableView::onScrollChange() {
	mHeader->setPixelsPosition( mRowHeaderWidth + -mScrollOffset.x, 0 );
}

void UIAbstractTableView::bindNavigationClick( UIWidget* widget ) {
	mWidgetsClickCbId[widget].push_back(
		widget->addEventListener( Event::MouseDoubleClick, [this]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			auto cellIdx = mouseEvent->getNode()->asType<UITableCell>()->getCurIndex();
			auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
			if ( isEditable() && ( mEditTriggers & EditTrigger::DoubleClicked ) && getModel() &&
				 getModel()->isEditable( cellIdx ) ) {
				beginEditing( cellIdx, mouseEvent->getNode()->asType<UIWidget>() );
			} else if ( ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) && !mSingleClickNavigation ) {
				onOpenModelIndex( idx, event );
			}
		} ) );

	mWidgetsClickCbId[widget].push_back(
		widget->addEventListener( Event::MouseClick, [this]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
			if ( mouseEvent->getFlags() & EE_BUTTON_RMASK ) {
				onOpenMenuModelIndex( idx, event );
			} else if ( ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) && mSingleClickNavigation ) {
				onOpenModelIndex( idx, event );
			} else if ( isCellSelection() && ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) ) {
				auto cellIdx = mouseEvent->getNode()->asType<UITableCell>()->getCurIndex();
				if ( getUISceneNode()->getWindow()->getInput()->isControlPressed() ) {
					getSelection().remove( cellIdx );
				} else {
					getSelection().set( cellIdx );
				}
			}
		} ) );
}

UIWidget* UIAbstractTableView::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	UITableCell* widget = UITableCell::New( mTag + "::cell" );
	return setupCell( widget, rowWidget, index );
}

UIWidget* UIAbstractTableView::setupCell( UITableCell* widget, UIWidget* rowWidget,
										  const ModelIndex& index ) {
	widget->setParent( rowWidget );
	widget->unsetFlags( UI_AUTO_SIZE );
	widget->setClipType( ClipType::ContentBox );
	widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	widget->setTextAlign( UI_HALIGN_LEFT );
	widget->setCurIndex( index );
	bindNavigationClick( widget );
	return widget;
}

UIWidget* UIAbstractTableView::updateCell( const Vector2<Int64>& posIndex, const ModelIndex& index,
										   const size_t&, const Float& yOffset ) {
	if ( posIndex.y >= (int)mWidgets.size() )
		mWidgets.resize( posIndex.y + 1 );
	auto* widget = mWidgets[posIndex.y][posIndex.x];
	if ( !widget ) {
		UIWidget* rowWidget = updateRow( posIndex.y, index, yOffset );
		widget = createCell( rowWidget, index );
		mWidgets[posIndex.y][posIndex.x] = widget;
		widget->reloadStyle( true, true, true );
	}
	const auto& colData = columnData( index.column() );
	if ( !colData.visible ) {
		widget->setVisible( false, false );
		return widget;
	} else {
		widget->setVisible( true, false );
	}
	widget->setPixelsSize( colData.width, getRowHeight() );
	widget->setPixelsPosition( { getColumnPosition( index.column() ).x, 0 } );
	if ( widget->isType( UI_TYPE_TABLECELL ) ) {
		UITableCell* cell = widget->asType<UITableCell>();
		cell->setCurIndex( index );

		if ( getModel()->classModelRoleEnabled() ) {
			bool needsReloadStyle = false;
			Variant cls( getModel()->data( index, ModelRole::Class ) );
			cell->setLoadingState( true );
			if ( cls.isValid() ) {
				std::string clsStr( cls.toString() );
				needsReloadStyle = cell->getClasses().empty() || cell->getClasses().size() != 1 ||
								   clsStr != cell->getClasses()[0];
				cell->setClass( clsStr );
			} else {
				needsReloadStyle = !cell->getClasses().empty();
				cell->resetClass();
			}
			cell->setLoadingState( false );
			if ( needsReloadStyle )
				cell->reportStyleStateChangeRecursive();
		}

		Variant txt( getModel()->data( index, ModelRole::Display ) );
		if ( txt.isValid() ) {
			if ( txt.is( Variant::Type::String ) )
				cell->setText( txt.asString() );
			else if ( txt.is( Variant::Type::StringPtr ) )
				cell->setText( txt.asStringPtr() );
			else
				cell->setText( txt.toString() );
		}

		bool isVisible = false;
		Variant icon( getModel()->data( index, ModelRole::Icon ) );
		if ( icon.is( Variant::Type::Drawable ) && icon.asDrawable() ) {
			isVisible = true;
			cell->setIcon( icon.asDrawable() );
		} else if ( icon.is( Variant::Type::Icon ) && icon.asIcon() ) {
			isVisible = true;
			cell->setIcon( icon.asIcon()->getSize( mIconSize ) );
		}
		if ( cell->hasIcon() )
			cell->getIcon()->setVisible( isVisible );

		cell->updateCell( getModel() );
	}

	if ( isCellSelection() ) {
		if ( getSelection().contains( index ) ) {
			widget->pushState( UIState::StateSelected );
		} else {
			widget->popState( UIState::StateSelected );
		}
	}

	return widget;
}

void UIAbstractTableView::moveSelection( int steps ) {
	if ( !getModel() )
		return;
	auto& model = *this->getModel();
	ModelIndex newIndex;
	if ( !getSelection().isEmpty() ) {
		auto oldIndex = getSelection().first();
		newIndex = model.index( oldIndex.row() + steps, oldIndex.column() );
	} else {
		newIndex = model.index( 0, 0 );
	}
	setSelection( newIndex );
}

void UIAbstractTableView::setSelection( const ModelIndex& index, bool scrollToSelection,
										bool openModelIndexTree ) {
	if ( !getModel() )
		return;
	auto& model = *this->getModel();
	if ( model.isValid( index ) ) {
		if ( openModelIndexTree )
			onOpenMenuModelIndex( index );
		getSelection().set( index );
		if ( scrollToSelection ) {
			auto rowHeight = getRowHeight();
			scrollToPosition( { { mScrollOffset.x, getHeaderHeight() + index.row() * rowHeight },
								{ columnData( index.column() ).width, rowHeight } } );
		}
	}
}

const size_t& UIAbstractTableView::getIconSize() const {
	return mIconSize;
}

void UIAbstractTableView::setIconSize( const size_t& iconSize ) {
	mIconSize = iconSize;
}

const size_t& UIAbstractTableView::getSortIconSize() const {
	return mSortIconSize;
}

void UIAbstractTableView::setSortIconSize( const size_t& sortIconSize ) {
	mSortIconSize = sortIconSize;
}

void UIAbstractTableView::onOpenModelIndex( const ModelIndex& index, const Event* triggerEvent ) {
	ModelEvent event( getModel(), index, this, ModelEventType::Open, triggerEvent );
	sendEvent( &event );
}

void UIAbstractTableView::onOpenMenuModelIndex( const ModelIndex& index,
												const Event* triggerEvent ) {
	ModelEvent event( getModel(), index, this, ModelEventType::OpenMenu, triggerEvent );
	sendEvent( &event );
}

Float UIAbstractTableView::getRowHeaderWidth() const {
	return mRowHeaderWidth;
}

void UIAbstractTableView::setRowHeaderWidth( Float rowHeaderWidth ) {
	if ( mRowHeaderWidth == rowHeaderWidth )
		return;
	mRowHeaderWidth = rowHeaderWidth;
	onScrollChange();
	buildRowHeader();
}

void UIAbstractTableView::buildRowHeader() {
	if ( mRowHeaderWidth == 0 ) {
		if ( mRowHeader )
			mRowHeader->setVisible( false )->setEnabled( false );
		return;
	}

	if ( mRowHeader == nullptr ) {
		mRowHeader = UILinearLayout::NewWithTag( mTag + "::rowheader", UIOrientation::Vertical );
		mRowHeader->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		mRowHeader->setParent( this )->setVisible( true )->setEnabled( true );
	}

	mRowHeader->setPaddingPixelsTop( mHeader->getPixelsSize().getHeight() );
	mRowHeader->setPixelsSize( { mRowHeaderWidth, getPixelsSize().getHeight() } );
	mRowHeader->setClipType( ClipType::PaddingBox );

	Uint32 rowsCount = Math::roundUp( mSize.getHeight() / getRowHeight() ) + 1;

	if ( mRowHeader->getChildCount() < rowsCount ) {
		int createCount = rowsCount - mRowHeader->getChildCount();
		for ( int i = 0; i < createCount; i++ ) {
			UIWidget* row = UIPushButton::NewWithTag( mTag + "::rowheader::row" );
			row->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
			row->setParent( mRowHeader );
			row->setPixelsSize( mRowHeaderWidth, getRowHeight() );
		}
	}
}

void UIAbstractTableView::updateRowHeader( int realRowIndex, const ModelIndex& index,
										   Float yOffset ) {
	if ( !mRowHeader || mRowHeaderWidth == 0 )
		return;
	Node* child = mRowHeader->getChildAt( realRowIndex );
	if ( !child )
		return;
	UIPushButton* row = child->asType<UIPushButton>();

	row->setPixelsSize( mRowHeaderWidth, getRowHeight() );
	row->setLayoutPixelsMarginTop( eefloor( yOffset ) );

	if ( getModel() )
		row->setText( getModel()->rowName( index.row() ) );
}

void UIAbstractTableView::onRowCreated( UITableRow* row ) {
	RowCreatedEvent rowEvent( this, Event::OnRowCreated, row );
	sendEvent( &rowEvent );
}

void UIAbstractTableView::onSortColumn( const size_t& colIndex ) {
	Model* model = getModel();
	if ( !model )
		return;
	if ( model->isSortable() && model->isColumnSortable( colIndex ) ) {
		if ( -1 != model->keyColumn() && (Int64)colIndex != model->keyColumn() &&
			 columnData( model->keyColumn() ).widget ) {
			UIImage* image =
				columnData( model->keyColumn() ).widget->getExtraInnerWidget()->asType<UIImage>();
			image->setForegroundFillEnabled( false );
			image->setDrawable( nullptr );
		}
		SortOrder sortOrder = model->sortOrder() == SortOrder::Ascending ? SortOrder::Descending
																		 : SortOrder::Ascending;
		UIPushButton* button = columnData( colIndex ).widget;
		UIImage* image = button->getExtraInnerWidget()->asType<UIImage>();
		std::string tag = button->getElementTag() + "::arrow";
		image->setElementTag( sortOrder == SortOrder::Ascending ? tag + "-up" : tag + "-down" );
		image->setForegroundFillEnabled( true );
		image->reloadStyle();
		if ( image->getForeground() )
			image->getForeground()->setAlpha( 255 );
		if ( image && image->getForeground() == nullptr ) {
			Drawable* icon = mUISceneNode->findIconDrawable(
				sortOrder == SortOrder::Ascending ? "arrow-down" : "arrow-up", mSortIconSize );
			if ( icon )
				image->setDrawable( icon );
		}
		model->sort( colIndex, sortOrder );
	}
}

Uint32 UIAbstractTableView::onTextInput( const TextInputEvent& event ) {
	if ( !mRowSearchByName )
		return 0;
	if ( mSearchTextAction )
		removeAction( mSearchTextAction );
	mSearchTextAction = Actions::Runnable::New(
		[this] {
			mSearchTextAction = nullptr;
			mSearchText = "";
		},
		Milliseconds( 750 ) );
	runAction( mSearchTextAction );
	mSearchText += String::trim( String::toLower( event.getText() ) );
	if ( mSearchText.empty() )
		return 1;
	ModelIndex index = findRowWithText( mSearchText );
	if ( index.isValid() ) {
		setSelection( index );
	} else {
		if ( mSearchText.size() >= 2 &&
			 mSearchText[mSearchText.size() - 2] == mSearchText[mSearchText.size() - 1] ) {
			mSearchText.pop_back();
			const Model* model = getModel();
			ModelIndex sel = getSelection().first();
			auto col = model->keyColumn() != -1
						   ? model->keyColumn()
						   : ( model->treeColumn() >= 0 ? model->treeColumn() : 0 );
			Int64 rowCount = model->rowCount( sel.parent() );
			for ( auto rowNext = sel.row() + 1; rowNext < rowCount; rowNext++ ) {
				ModelIndex next = model->index( rowNext, col, sel.parent() );
				Variant var = model->data( next );
				if ( var.isValid() &&
					 String::startsWith( String::toLower( var.toString() ), mSearchText ) ) {
					setSelection( model->index( next.row(), 0, next.parent() ) );
					return 1;
				}
			}

			ModelIndex fIndex = findRowWithText( mSearchText );
			if ( fIndex.isValid() )
				setSelection( fIndex );
		}
	}
	return 1;
}

bool UIAbstractTableView::tryBeginEditing( KeyBindings::Shortcut fromShortcut ) {
	if ( isEditable() && getSelection().first().isValid() && getModel() &&
		 getModel()->isEditable( getSelection().first() ) &&
		 ( mEditTriggers & EditTrigger::EditKeyPressed ) && !mEditShortcuts.empty() ) {
		fromShortcut = KeyBindings::sanitizeShortcut( fromShortcut );
		for ( const auto& shortcut : mEditShortcuts ) {
			if ( shortcut == fromShortcut ) {
				beginEditing( getSelection().first(), getCellFromIndex( getSelection().first() ) );
				return true;
			}
		}
	}
	return false;
}

Uint32 UIAbstractTableView::onKeyDown( const KeyEvent& event ) {
	if ( tryBeginEditing( KeyBindings::Shortcut{ event.getKeyCode(), event.getMod() } ) )
		return 1;
	return UIAbstractView::onKeyDown( event );
}

bool UIAbstractTableView::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::RowHeight:
			setRowHeight( lengthFromValue( attribute.getValue(), PropertyRelativeTarget::None ) );
			break;
		default:
			return UIAbstractView::applyProperty( attribute );
	}

	return true;
}

std::string UIAbstractTableView::getPropertyString( const PropertyDefinition* propertyDef,
													const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::RowHeight:
			return String::fromFloat( getRowHeight(), "px" );
		default:
			return UIAbstractView::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIAbstractTableView::getPropertiesImplemented() const {
	auto props = UIAbstractView::getPropertiesImplemented();
	props.push_back( PropertyId::RowHeight );
	return props;
}

bool UIAbstractTableView::getRowSearchByName() const {
	return mRowSearchByName;
}

void UIAbstractTableView::setRowSearchByName( bool rowSearchByName ) {
	mRowSearchByName = rowSearchByName;
}

bool UIAbstractTableView::getAutoColumnsWidth() const {
	return mAutoColumnsWidth;
}

void UIAbstractTableView::setAutoColumnsWidth( bool autoColumnsWidth ) {
	if ( mAutoColumnsWidth != autoColumnsWidth ) {
		mAutoColumnsWidth = autoColumnsWidth;
		if ( mAutoColumnsWidth )
			updateColumnsWidth();
	}
}

const size_t& UIAbstractTableView::getMainColumn() const {
	return mMainColumn;
}

void UIAbstractTableView::setMainColumn( const size_t& mainColumn ) {
	mMainColumn = mainColumn;
}

bool UIAbstractTableView::getSingleClickNavigation() const {
	return mSingleClickNavigation;
}

void UIAbstractTableView::setSingleClickNavigation( bool singleClickNavigation ) {
	if ( singleClickNavigation != mSingleClickNavigation ) {
		mSingleClickNavigation = singleClickNavigation;
		// Rebind the clicks
		for ( const auto& widgetIt : mWidgetsClickCbId ) {
			widgetIt.first->removeEventListener( widgetIt.second );
			bindNavigationClick( widgetIt.first );
		}
	}
}

bool UIAbstractTableView::getFitAllColumnsToWidget() const {
	return mFitAllColumnsToWidget;
}

void UIAbstractTableView::setFitAllColumnsToWidget( bool fitAllColumnsToWidget ) {
	mFitAllColumnsToWidget = fitAllColumnsToWidget;
}

void UIAbstractTableView::recalculateColumnsWidth() {
	createOrUpdateColumns( false );
}

UITableCell* UIAbstractTableView::getCellFromIndex( const ModelIndex& index ) const {
	for ( const auto& row : mWidgets ) {
		for ( const auto& widget : row ) {
			if ( widget.second->isType( UI_TYPE_TABLECELL ) &&
				 widget.second->asType<UITableCell>()->getCurIndex() == index ) {
				return widget.second->asType<UITableCell>();
			}
		}
	}
	return nullptr;
}

}}} // namespace EE::UI::Abstract
