#include <eepp/system/thread.hpp>
#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace UI { namespace Abstract {

UIAbstractTableView::UIAbstractTableView( const std::string& tag ) :
	UIAbstractView( tag ),
	mDragBorderDistance( PixelDensity::dpToPx( 4 ) ),
	mIconSize( PixelDensity::dpToPxI( 12 ) ),
	mSortIconSize( PixelDensity::dpToPxI( 20 ) ) {
	mHeader = UILinearLayout::NewWithTag( mTag + "::header", UIOrientation::Horizontal );
	mHeader->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mHeader->setParent( this );
	mHeader->setVisible( true );
	mHeader->setEnabled( true );
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
										   : 16 ) );
}

void UIAbstractTableView::setRowHeight( const Float& rowHeight ) {
	if ( mRowHeight != rowHeight ) {
		mRowHeight = rowHeight;
		createOrUpdateColumns();
	}
}

void UIAbstractTableView::setColumnWidth( const size_t& colIndex, const Float& width ) {
	if ( columnData( colIndex ).width != width ) {
		columnData( colIndex ).width = width;
		updateHeaderSize();
		onColumnSizeChange( colIndex );
		createOrUpdateColumns();
	}
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
		runOnMainThread( [&, flags] {
			modelUpdate( flags );
			createOrUpdateColumns();
		} );
	} else {
		UIAbstractView::onModelUpdate( flags );
		createOrUpdateColumns();
	}
}

void UIAbstractTableView::createOrUpdateColumns() {
	Model* model = getModel();
	if ( !model )
		return;

	size_t count = model->columnCount();
	Float totalWidth = 0;

	auto visibleColCount = visibleColumnCount();

	if ( mAutoColumnsWidth && visibleColCount > 1 && getModel()->rowCount() > 0 ) {
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
				usedWidth += colWidth;
				columnData( col ).width = colWidth;
			}
		}
		Float mainColMaxWidth = getMaxColumnContentWidth( mMainColumn, true );
		columnData( mMainColumn ).width = contentWidth - usedWidth >= mainColMaxWidth
											  ? contentWidth - usedWidth
											  : mainColMaxWidth;
	}

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
	mHeader->updateLayout();

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
	Sizef size;
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
	createOrUpdateColumns();
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
	createOrUpdateColumns();
}

void UIAbstractTableView::updateHeaderSize() {
	if ( !getModel() )
		return;
	size_t count = getModel()->columnCount();
	Float totalWidth = 0;
	for ( size_t i = 0; i < count; i++ ) {
		ColumnData& col = columnData( i );
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
	return eefloor( getPixelsSize().getWidth() - getPixelsPadding().Left -
					getPixelsPadding().Right -
					( mVScroll->isVisible() ? mVScroll->getPixelsSize().getWidth() : 0 ) );
}

void UIAbstractTableView::updateColumnsWidth() {
	if ( mAutoExpandOnSingleColumn || mAutoColumnsWidth ) {
		int col = 0;
		if ( visibleColumnCount() == 1 && ( col = visibleColumn() ) != -1 ) {
			Float width = eemax( getContentSpaceWidth(), getMaxColumnContentWidth( col, true ) );
			bool shouldVScrollBeVisible = shouldVerticalScrollBeVisible();
			if ( !mVScroll->isVisible() && shouldVScrollBeVisible )
				width -= getVerticalScrollBar()->getPixelsSize().getWidth();
			else if ( mVScroll->isVisible() && !shouldVScrollBeVisible )
				width += getVerticalScrollBar()->getPixelsSize().getWidth();
			columnData( col ).width = width;
			updateHeaderSize();
			onColumnSizeChange( col );
		}
	}
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
		createOrUpdateColumns();
	}
}

void UIAbstractTableView::setColumnsHidden( const std::vector<size_t> columns, bool hidden ) {
	for ( auto col : columns )
		columnData( col ).visible = !hidden;
	createOrUpdateColumns();
}

void UIAbstractTableView::setColumnsVisible( const std::vector<size_t> columns ) {
	if ( !getModel() )
		return;
	for ( size_t i = 0; i < getModel()->columnCount(); i++ )
		columnData( i ).visible = false;
	for ( auto col : columns )
		columnData( col ).visible = true;
	createOrUpdateColumns();
}

UITableRow* UIAbstractTableView::createRow() {
	mUISceneNode->invalidateStyle( this );
	mUISceneNode->invalidateStyleState( this, true );
	UITableRow* rowWidget = UITableRow::New( mTag + "::row" );
	rowWidget->setParent( this );
	rowWidget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	rowWidget->reloadStyle( true, true, true );
	rowWidget->addEventListener( Event::MouseDown, [&]( const Event* event ) {
		if ( ( !getEventDispatcher()->getMouseDownNode() ||
			   getEventDispatcher()->getMouseDownNode() == this ||
			   isParentOf( getEventDispatcher()->getMouseDownNode() ) ) &&
			 getEventDispatcher()->getNodeDragging() == nullptr ) {
			getSelection().set( event->getNode()->asType<UITableRow>()->getCurIndex() );
		}
	} );
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
	rowWidget->setPixelsPosition( { -mScrollOffset.x, yOffset - mScrollOffset.y } );
	if ( getSelection().contains( index ) ) {
		rowWidget->pushState( UIState::StateSelected );
	} else {
		rowWidget->popState( UIState::StateSelected );
	}
	return rowWidget;
}

void UIAbstractTableView::onScrollChange() {
	mHeader->setPixelsPosition( -mScrollOffset.x, 0 );
}

void UIAbstractTableView::bindNavigationClick( UIWidget* widget ) {
	mWidgetsClickCbId[widget] = widget->addEventListener(
		mSingleClickNavigation ? Event::MouseClick : Event::MouseDoubleClick,
		[&]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				onOpenModelIndex( idx, event );
			}
		} );

	widget->addEventListener( Event::MouseClick, [&]( const Event* event ) {
		auto mouseEvent = static_cast<const MouseEvent*>( event );
		auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
		if ( mouseEvent->getFlags() & EE_BUTTON_RMASK ) {
			onOpenMenuModelIndex( idx, event );
		}
	} );
}

UIWidget* UIAbstractTableView::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	UITableCell* widget = UITableCell::New( mTag + "::cell" );
	widget->setParent( rowWidget );
	widget->unsetFlags( UI_AUTO_SIZE );
	widget->clipEnable();
	widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	widget->setTextAlign( UI_HALIGN_LEFT );
	widget->setCurIndex( index );
	bindNavigationClick( widget );
	return widget;
}

UIWidget* UIAbstractTableView::updateCell( const int& rowIndex, const ModelIndex& index,
										   const size_t&, const Float& yOffset ) {
	if ( rowIndex >= (int)mWidgets.size() )
		mWidgets.resize( rowIndex + 1 );
	auto* widget = mWidgets[rowIndex][index.column()];
	if ( !widget ) {
		UIWidget* rowWidget = updateRow( rowIndex, index, yOffset );
		widget = createCell( rowWidget, index );
		mWidgets[rowIndex][index.column()] = widget;
		widget->reloadStyle( true, true, true );
	}
	widget->setPixelsSize( columnData( index.column() ).width, getRowHeight() );
	widget->setPixelsPosition( { getColumnPosition( index.column() ).x, 0 } );
	if ( widget->isType( UI_TYPE_TABLECELL ) ) {
		UITableCell* cell = widget->asType<UITableCell>();
		cell->setCurIndex( index );

		Variant txt( getModel()->data( index, ModelRole::Display ) );
		if ( txt.isValid() ) {
			if ( txt.is( Variant::Type::String ) )
				cell->setText( txt.asString() );
			else if ( txt.is( Variant::Type::cstr ) )
				cell->setText( txt.asCStr() );
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
		cell->getIcon()->setVisible( isVisible );

		cell->updateCell( getModel() );
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

void UIAbstractTableView::setSelection( const ModelIndex& index, bool scrollToSelection ) {
	if ( !getModel() )
		return;
	auto& model = *this->getModel();
	if ( model.isValid( index ) && scrollToSelection ) {
		getSelection().set( index );
		scrollToPosition( { { mScrollOffset.x, getHeaderHeight() + index.row() * getRowHeight() },
							{ columnData( index.column() ).width, getRowHeight() } } );
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
		[&] {
			mSearchTextAction = nullptr;
			mSearchText = "";
		},
		Milliseconds( 350 ) );
	runAction( mSearchTextAction );
	mSearchText += String::trim( String::toLower( event.getText() ) );
	if ( !mSearchText.empty() ) {
		ModelIndex index = findRowWithText( mSearchText );
		if ( index.isValid() )
			setSelection( index );
	}
	return 1;
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
													const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::RowHeight:
			return String::format( "%.2fdp", getRowHeight() );
		default:
			return UIAbstractView::getPropertyString( propertyDef, propertyIndex );
	}
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

}}} // namespace EE::UI::Abstract
