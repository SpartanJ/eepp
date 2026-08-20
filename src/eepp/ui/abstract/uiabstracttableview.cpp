#include <eepp/system/thread.hpp>
#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <nlohmann/json.hpp>

namespace EE { namespace UI { namespace Abstract {

static constexpr String::HashType onModelUpdateTag = String::hash( "onModelUpdate" );

UIAbstractTableView::UIAbstractTableView( const std::string& tag ) :
	UIAbstractView( tag ),
	mDragBorderDistance( PixelDensity::dpToPx( 4 ) ),
	mIconSize( PixelDensity::dpToPxI( 12 ) ),
	mSortIconSize( PixelDensity::dpToPxI( 20 ) ) {
	mHeader = UILinearLayout::NewWithTag( mTag + "::header", UIOrientation::Horizontal );
	mHeader->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mHeader->setParent( this )->setVisible( true )->setEnabled( true );
	mHeader->setUpdateLayoutEvenIfNotVisible( true );
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
		columnData( colIndex ).setWidth( width, true );
		updateHeaderSize();
		onColumnSizeChange( colIndex );
		createOrUpdateColumns( false );
	}
}

void UIAbstractTableView::setColumnMaxWidth( const size_t& colIndex, const Float& width ) {
	if ( columnData( colIndex ).maxWidth != width ) {
		columnData( colIndex ).maxWidth = width;
		if ( columnData( colIndex ).width > width ) {
			updateHeaderSize();
			onColumnSizeChange( colIndex );
			createOrUpdateColumns( false );
		}
	}
}

void UIAbstractTableView::setColumnsWidth( const Float& width ) {
	if ( !getModel() )
		return;
	for ( size_t i = 0; i < getModel()->columnCount(); i++ ) {
		columnData( i ).setWidth( width, true );
		onColumnSizeChange( i );
	}
	updateHeaderSize();
	createOrUpdateColumns( false );
}

void UIAbstractTableView::setColumnsMaxWidth( const Float& width ) {
	if ( !getModel() )
		return;
	bool needsRefresh = false;
	for ( size_t i = 0; i < getModel()->columnCount(); i++ ) {
		if ( columnData( i ).width > width )
			needsRefresh = true;
		columnData( i ).maxWidth = width;
		onColumnSizeChange( i );
	}
	if ( needsRefresh ) {
		updateHeaderSize();
		createOrUpdateColumns( false );
	}
}

const Float& UIAbstractTableView::getColumnWidth( const size_t& colIndex ) const {
	return columnData( colIndex ).width;
}

Float UIAbstractTableView::getColumnWidthPercentage( const size_t& colIndex ) const {
	return columnData( colIndex ).percentage;
}

std::vector<Float> UIAbstractTableView::getColumnsWidthPercentage() const {
	std::vector<Float> percentages;
	size_t count = getModel() ? getModel()->columnCount() : mColumn.size();
	percentages.reserve( count );
	for ( size_t i = 0; i < count; ++i )
		percentages.emplace_back( columnData( i ).percentage );
	return percentages;
}

UIAbstractTableView::ColumnWidthMode UIAbstractTableView::getColumnWidthMode() const {
	return mColumnWidthMode;
}

void UIAbstractTableView::setColumnWidthMode( ColumnWidthMode mode, bool convertCurrentWidths ) {
	if ( mode == mColumnWidthMode )
		return;
	if ( mode == ColumnWidthMode::Percentage && getModel() ) {
		setAutoColumnsWidth( false );
		if ( convertCurrentWidths ) {
			Float totalWidth = 0;
			for ( size_t i = 0; i < getModel()->columnCount(); ++i )
				if ( !isColumnHidden( i ) )
					totalWidth += columnData( i ).width;
			if ( totalWidth > 0 )
				for ( size_t i = 0; i < getModel()->columnCount(); ++i )
					if ( !isColumnHidden( i ) )
						columnData( i ).percentage = columnData( i ).width / totalWidth * 100.f;
		}
	}
	mColumnWidthMode = mode;
	createOrUpdateColumns( false );
}

bool UIAbstractTableView::isColumnWidthModeMenuEnabled() const {
	return mColumnWidthModeMenuEnabled;
}

void UIAbstractTableView::setColumnWidthModeMenuEnabled( bool enabled ) {
	mColumnWidthModeMenuEnabled = enabled;
}

void UIAbstractTableView::setColumnWidthPercentage( const size_t& colIndex, Float percentage ) {
	if ( mColumnWidthMode != ColumnWidthMode::Percentage )
		setColumnWidthMode( ColumnWidthMode::Percentage );
	if ( !getModel() || colIndex >= getModel()->columnCount() )
		return;
	auto& column = columnData( colIndex );
	int adjacent = adjacentVisibleColumn( colIndex );
	if ( adjacent >= 0 ) {
		auto& sibling = columnData( adjacent );
		Float combined = column.percentage + sibling.percentage;
		column.percentage = eeclamp( percentage, 0.f, combined );
		sibling.percentage = combined - column.percentage;
	} else {
		column.percentage = 100.f;
	}
	createOrUpdateColumns( false );
}

void UIAbstractTableView::setColumnsWidthPercentage( const std::vector<Float>& percentages ) {
	const size_t columnCount = getModel() ? getModel()->columnCount() : percentages.size();
	if ( mColumn.size() < columnCount )
		mColumn.resize( columnCount );
	const size_t suppliedColumnCount = eemin( percentages.size(), columnCount );
	Float total = 0;
	for ( size_t i = 0; i < suppliedColumnCount; ++i ) {
		columnData( i ).percentage = eemax( 0.f, percentages[i] );
		if ( !getModel() || !isColumnHidden( i ) )
			total += columnData( i ).percentage;
	}
	size_t missingVisibleColumns = 0;
	for ( size_t i = suppliedColumnCount; i < columnCount; ++i )
		if ( !getModel() || !isColumnHidden( i ) )
			++missingVisibleColumns;
	const Float missingPercentage =
		missingVisibleColumns > 0 ? eemax( 0.f, 100.f - total ) / missingVisibleColumns : 0.f;
	for ( size_t i = suppliedColumnCount; i < columnCount; ++i ) {
		columnData( i ).percentage = !getModel() || !isColumnHidden( i ) ? missingPercentage : 0.f;
		total += columnData( i ).percentage;
	}
	if ( total > 0 )
		for ( size_t i = 0; i < columnCount; ++i )
			if ( !getModel() || !isColumnHidden( i ) )
				columnData( i ).percentage = columnData( i ).percentage / total * 100.f;
	setAutoColumnsWidth( false );
	mColumnWidthMode = ColumnWidthMode::Percentage;
	if ( getModel() )
		createOrUpdateColumns( false );
}

nlohmann::json UIAbstractTableView::serializeColumnWidths() const {
	if ( !getModel() && !mPendingSerializedColumnWidths.empty() )
		return nlohmann::json::parse( mPendingSerializedColumnWidths, nullptr, false, true );
	nlohmann::json saved;
	const bool percentage = mColumnWidthMode == ColumnWidthMode::Percentage;
	saved["mode"] = percentage ? "percentage" : "pixels";
	if ( percentage ) {
		saved["widths"] = getColumnsWidthPercentage();
		return saved;
	}
	std::vector<Float> widths;
	if ( !getModel() )
		return saved;
	widths.reserve( getModel()->columnCount() );
	for ( size_t i = 0; i < getModel()->columnCount(); ++i )
		widths.emplace_back( PixelDensity::pxToDp( getColumnWidth( i ) ) );
	saved["widths"] = std::move( widths );
	return saved;
}

bool UIAbstractTableView::unserializeColumnWidths( const nlohmann::json& saved ) {
	const nlohmann::json* widths = &saved;
	ColumnWidthMode mode = ColumnWidthMode::Percentage;
	if ( saved.is_object() ) {
		if ( !saved.contains( "widths" ) || !saved["widths"].is_array() )
			return false;
		widths = &saved["widths"];
		if ( saved.value( "mode", "percentage" ) == "pixels" )
			mode = ColumnWidthMode::Pixels;
	} else if ( !saved.is_array() ) {
		return false;
	}
	std::vector<Float> values;
	values.reserve( widths->size() );
	for ( const auto& value : *widths ) {
		if ( !value.is_number() )
			return false;
		values.emplace_back( value.get<Float>() );
	}
	if ( !getModel() ) {
		mPendingSerializedColumnWidths = saved.dump();
		return true;
	}
	mPendingSerializedColumnWidths.clear();
	if ( mode == ColumnWidthMode::Percentage ) {
		setColumnsWidthPercentage( values );
	} else {
		if ( values.size() != getModel()->columnCount() )
			return false;
		setColumnWidthMode( mode, false );
		for ( size_t i = 0; i < values.size(); ++i )
			setColumnWidth( i, PixelDensity::dpToPx( values[i] ) );
	}
	return true;
}

void UIAbstractTableView::restorePendingColumnWidths() {
	if ( !getModel() || mPendingSerializedColumnWidths.empty() )
		return;
	std::string serialized( std::move( mPendingSerializedColumnWidths ) );
	mPendingSerializedColumnWidths.clear();
	auto widths = nlohmann::json::parse( serialized, nullptr, false, true );
	if ( !widths.is_discarded() )
		unserializeColumnWidths( widths );
}

void UIAbstractTableView::selectAll() {
	getSelection().clear();
	for ( size_t itemIndex = 0; itemIndex < getItemCount(); ++itemIndex ) {
		auto index = getModel()->index( itemIndex );
		getSelection().add( index );
	}
}

std::vector<ModelIndex> UIAbstractTableView::getSelectionRange( const ModelIndex& start,
																const ModelIndex& end ) const {
	std::vector<ModelIndex> range;
	if ( !getModel() )
		return range;
	int minRow = eemin( start.row(), end.row() );
	int maxRow = eemax( start.row(), end.row() );
	for ( int i = minRow; i <= maxRow; ++i ) {
		range.push_back( getModel()->index( i, start.column() ) );
	}
	return range;
}

size_t UIAbstractTableView::getItemCount() const {
	if ( !getModel() )
		return 0;
	return getModel()->rowCount();
}

void UIAbstractTableView::onModelUpdate( unsigned flags ) {
	mPendingUpdateFlags.fetch_or( flags );
	if ( !Engine::instance()->isMainThread() ) {
		removeActionsByTag( onModelUpdateTag );
		runOnMainThread(
			[this] {
				modelUpdate( mPendingUpdateFlags.exchange( 0 ) );
				createOrUpdateColumns( true );
				restorePendingColumnWidths();
			},
			Time::Zero, onModelUpdateTag );
	} else {
		UIAbstractView::onModelUpdate( flags );
		createOrUpdateColumns( true );
		restorePendingColumnWidths();
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
		col.maxWidth = 0;
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
		col.setWidth( eeceil( col.maxWidth != 0 ? eeclamp( col.width, col.minWidth, col.maxWidth )
												: eemax( col.width, col.minWidth ) ) );
		col.widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		col.widget->setPixelsSize( col.width, getHeaderHeight() );
	}

	if ( mColumnWidthMode == ColumnWidthMode::Percentage )
		updatePercentageColumnWidths();

	if ( mAutoColumnsWidth && visibleColCount > 1 ) {
		Float contentWidth = getContentSpaceWidth();
		bool shouldVScrollBeVisible = shouldVerticalScrollBeVisible();
		if ( !mVScroll->isVisible() && shouldVScrollBeVisible )
			contentWidth -= getVerticalScrollBar()->getPixelsSize().getWidth();
		else if ( mVScroll->isVisible() && !shouldVScrollBeVisible )
			contentWidth += getVerticalScrollBar()->getPixelsSize().getWidth();
		Float usedWidth = 0;
		for ( size_t colIdx = 0; colIdx < count; colIdx++ ) {
			if ( colIdx != mMainColumn && !isColumnHidden( colIdx ) ) {
				Float colWidth = getMaxColumnContentWidth( colIdx, true );
				auto& col = columnData( colIdx );
				if ( col.widget )
					colWidth = eemax( colWidth, col.widget->getPixelsSize().getWidth() );
				usedWidth += colWidth;
				col.setWidth( colWidth );
			}
		}
		Float mainColMaxWidth = getMaxColumnContentWidth( mMainColumn, true );
		auto& mainCol = columnData( mMainColumn );
		mainCol.setWidth( contentWidth - usedWidth >= mainColMaxWidth ? contentWidth - usedWidth
																	  : mainColMaxWidth );
		usedWidth += mainCol.width;
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
				columnData( longestCol ).setWidth( longestColWidth );
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
		col.setWidth( eeceil( col.maxWidth != 0 ? eeclamp( col.width, col.minWidth, col.maxWidth )
												: eemax( col.width, col.minWidth ) ) );
		if ( col.widget ) {
			col.widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
			col.widget->setPixelsSize( col.width, getHeaderHeight() );
		}
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

void UIAbstractTableView::onColumnSizeChange( const size_t& colIndex, bool fromUserInteraction ) {
	if ( fromUserInteraction && mAutoColumnsWidth )
		mAutoColumnsWidth = false;
	if ( fromUserInteraction && mColumnWidthMode == ColumnWidthMode::Percentage && getModel() ) {
		int adjacent = adjacentVisibleColumn( colIndex );
		Float contentWidth = getContentSpaceWidth();
		if ( adjacent >= 0 && contentWidth > 0 ) {
			auto& column = columnData( colIndex );
			auto& sibling = columnData( adjacent );
			Float combined = column.percentage + sibling.percentage;
			Float minPercentage = column.minWidth / contentWidth * 100.f;
			Float siblingMinPercentage = sibling.minWidth / contentWidth * 100.f;
			column.percentage = eeclamp( column.width / contentWidth * 100.f, minPercentage,
										 combined - siblingMinPercentage );
			sibling.percentage = combined - column.percentage;
			updatePercentageColumnWidths();
			updateHeaderSize();
		}
	}
}

void UIAbstractTableView::updatePercentageColumnWidths() {
	if ( !getModel() )
		return;
	Float contentWidth = getContentSpaceWidth();
	Float totalPercentage = 0;
	int visibleColumns = 0;
	for ( size_t i = 0; i < getModel()->columnCount(); ++i ) {
		if ( !isColumnHidden( i ) ) {
			totalPercentage += columnData( i ).percentage;
			visibleColumns++;
		}
	}
	if ( totalPercentage <= 0 && visibleColumns > 0 ) {
		for ( size_t i = 0; i < getModel()->columnCount(); ++i )
			if ( !isColumnHidden( i ) )
				columnData( i ).percentage = 100.f / visibleColumns;
		totalPercentage = 100.f;
	}
	contentWidth = eefloor( contentWidth );
	Float cumulativePercentage = 0;
	Float previousBoundary = 0;
	Float assignedWidth = 0;
	for ( size_t i = 0; i < getModel()->columnCount(); ++i ) {
		if ( isColumnHidden( i ) )
			continue;
		auto& column = columnData( i );
		cumulativePercentage += column.percentage;
		Float boundary = eefloor( contentWidth * cumulativePercentage / totalPercentage + 0.5f );
		Float width = boundary - previousBoundary;
		Float minWidth = eeceil( column.minWidth );
		width = column.maxWidth != 0
					? eeclamp( width, minWidth, eemax( minWidth, eefloor( column.maxWidth ) ) )
					: eemax( width, minWidth );
		column.setWidth( width, true );
		assignedWidth += width;
		previousBoundary = boundary;
	}
	Float overflow = assignedWidth - contentWidth;
	for ( size_t i = getModel()->columnCount(); overflow > 0 && i > 0; --i ) {
		if ( isColumnHidden( i - 1 ) )
			continue;
		auto& column = columnData( i - 1 );
		Float shrink = eemin( overflow, column.width - eeceil( column.minWidth ) );
		if ( shrink > 0 ) {
			column.setWidth( column.width - shrink, true );
			overflow -= shrink;
		}
	}
	for ( size_t i = 0; i < getModel()->columnCount(); ++i ) {
		auto& column = columnData( i );
		if ( column.widget && !isColumnHidden( i ) )
			column.widget->setPixelsSize( column.width, getHeaderHeight() );
	}
}

int UIAbstractTableView::adjacentVisibleColumn( size_t column ) const {
	if ( !getModel() )
		return -1;
	for ( size_t i = column + 1; i < getModel()->columnCount(); ++i )
		if ( !isColumnHidden( i ) )
			return i;
	for ( size_t i = column; i > 0; --i )
		if ( !isColumnHidden( i - 1 ) )
			return i - 1;
	return -1;
}

Float UIAbstractTableView::getMaxColumnContentWidth( const size_t&, bool ) {
	return 0;
}

void UIAbstractTableView::onColumnResizeToContent( const size_t& colIndex ) {
	columnData( colIndex ).setWidth( getMaxColumnContentWidth( colIndex, true ) );
	if ( mColumnWidthMode == ColumnWidthMode::Percentage ) {
		onColumnSizeChange( colIndex, true );
		return;
	}
	createOrUpdateColumns( false );
}

void UIAbstractTableView::updateHeaderSize() {
	if ( !getModel() )
		return;
	size_t count = getModel()->columnCount();
	Float totalWidth = 0;
	for ( size_t i = 0; i < count; i++ ) {
		const ColumnData& col = columnData( i );
		if ( col.visible )
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
					( mVScroll->isVisible() && ( mScrollViewType == ScrollViewType::Outside ||
												 mVScroll->getAlpha() != 0.f )
						  ? mVScroll->getPixelsSize().getWidth()
						  : 0 ) );
}

void UIAbstractTableView::updateColumnsWidth() {
	if ( mAutoExpandOnSingleColumn || mAutoColumnsWidth ) {
		int col = 0;
		if ( visibleColumnCount() == 1 && ( col = visibleColumn() ) != -1 ) {
			Float width = eemax( getContentSpaceWidth(), getMaxColumnContentWidth( col, true ) );
			bool shouldVScrollBeVisible = shouldVerticalScrollBeVisible();
			const bool verticalScrollConsumesWidth =
				mScrollViewType == ScrollViewType::Outside || mVScroll->getAlpha() != 0.f;
			mAutoExpandedColumnUsesVerticalScroll =
				shouldVScrollBeVisible && verticalScrollConsumesWidth;
			if ( verticalScrollConsumesWidth ) {
				if ( !mVScroll->isVisible() && shouldVScrollBeVisible )
					width -= getVerticalScrollBar()->getPixelsSize().getWidth();
				else if ( mVScroll->isVisible() && !shouldVScrollBeVisible )
					width += getVerticalScrollBar()->getPixelsSize().getWidth();
			}
			if ( columnData( col ).width != width ) {
				columnData( col ).setWidth( width );
				updateHeaderSize();

				ColumnData& colData = columnData( col );
				if ( colData.widget )
					colData.widget->setPixelsSize( colData.width, getHeaderHeight() );

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
	const auto& col = columnData( index );
	return col.widget ? col.widget->getPixelsPosition() : Vector2f::Zero;
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
					colFlags |= 1ll << i;
			}

			for ( auto col : columns )
				newColFlags |= 1ll << col;

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
			 ( getInput()->getSanitizedModState() & KeyMod::getDefaultModifier() ) ) {
			getSelection().remove( index );
		} else {
			if ( mSelectionKind == SelectionKind::Multiple &&
				 ( getInput()->getSanitizedModState() & KeyMod::getDefaultModifier() ) ) {
				getSelection().toggle( index );
			} else if ( mSelectionKind == SelectionKind::Multiple &&
						( getInput()->getSanitizedModState() & KEYMOD_SHIFT ) &&
						!getSelection().isEmpty() ) {
				getSelection().set( getSelectionRange( getSelection().first(), index ) );
			} else if ( mSelectionKind == SelectionKind::Multiple ) {
				if ( !getSelection().contains( index ) )
					getSelection().set( index );
			} else {
				getSelection().set( index );
			}
		}
	} );
	rowWidget->on( Event::MouseClick, [this]( const Event* event ) {
		if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) || !isRowSelection() ||
			 mSingleClickNavigation )
			return;

		auto index = event->getNode()->asType<UITableRow>()->getCurIndex();
		if ( 0 == getInput()->getSanitizedModState() ) {
			getSelection().set( index );
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

void UIAbstractTableView::onContentSizeChange() {
	if ( mUpdatingColumnsForScrollbars ) {
		UIScrollableWidget::onContentSizeChange();
		return;
	}

	bool verticalScrollWasVisible = mVScroll->isVisible();
	UIScrollableWidget::onContentSizeChange();
	const bool autoExpandedSingleColumn = mAutoExpandOnSingleColumn && visibleColumnCount() == 1;
	const bool columnsDependOnContentWidth = mColumnWidthMode == ColumnWidthMode::Percentage ||
											 mAutoColumnsWidth || autoExpandedSingleColumn;
	const bool verticalScrollConsumesWidth =
		mVScroll->isVisible() &&
		( mScrollViewType == ScrollViewType::Outside || mVScroll->getAlpha() != 0.f );
	// Visibility can be updated before this callback begins, so comparing only the state before and
	// after the base implementation can miss a stale auto-expanded width.
	const bool autoExpandedColumnIsStale =
		autoExpandedSingleColumn &&
		mAutoExpandedColumnUsesVerticalScroll != verticalScrollConsumesWidth;
	if ( !columnsDependOnContentWidth ||
		 ( verticalScrollWasVisible == mVScroll->isVisible() && !autoExpandedColumnIsStale ) )
		return;

	mUpdatingColumnsForScrollbars = true;
	for ( int iteration = 0; iteration < 2; ++iteration ) {
		bool visibilityUsedForColumns = mVScroll->isVisible();
		createOrUpdateColumns( false );
		UIScrollableWidget::onContentSizeChange();
		if ( visibilityUsedForColumns == mVScroll->isVisible() )
			break;
	}
	mUpdatingColumnsForScrollbars = false;
}

void UIAbstractTableView::bindNavigationClick( UIWidget* widget ) {
	mWidgetsClickCbId[widget].push_back(
		widget->on( Event::MouseDoubleClick, [this]( const Event* event ) {
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
		widget->on( Event::MouseClick, [this]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
			if ( mouseEvent->getFlags() & EE_BUTTON_RMASK ) {
				onOpenMenuModelIndex( idx, event );
			} else if ( ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) && mSingleClickNavigation ) {
				onOpenModelIndex( idx, event );
			} else if ( isCellSelection() && ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) ) {
				auto cellIdx = mouseEvent->getNode()->asType<UITableCell>()->getCurIndex();
				if ( getInput()->isControlPressed() ) {
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
	mUISceneNode->invalidateStyle( this );
	mUISceneNode->invalidateStyleState( this, true );
	widget->setParent( rowWidget );
	widget->unsetFlags( UI_AUTO_SIZE );
	widget->setClipType( ClipType::ContentBox );
	widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	widget->setTextAlign( UI_HALIGN_LEFT );
	widget->setCurIndex( index );
	bindNavigationClick( widget );

	if ( mSetupCellCb )
		mSetupCellCb( widget );

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
		updateTableCellData( cell, index );

		bool isVisible = false;
		Variant icon( getModel()->data( index, ModelRole::Icon ) );
		if ( icon.is( Variant::Type::Drawable ) && icon.asDrawable() ) {
			isVisible = true;
			cell->setIcon( icon.asDrawable() );
		} else if ( icon.is( Variant::Type::Icon ) && icon.asIcon() ) {
			isVisible = true;
			cell->setIcon( icon.asIcon()->createDrawable( mIconSize ) );
		}
		if ( cell->hasIcon() )
			cell->getIcon()->setVisible( isVisible );

		cell->updateCell( getModel() );

		if ( mOnUpdateCellCb )
			mOnUpdateCellCb( cell, getModel() );
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

void UIAbstractTableView::updateTableCellData( UITableCell* cell, const ModelIndex& index ) {
	cell->setCurIndex( index );

	if ( getModel()->classModelRoleEnabled() ) {
		bool needsReloadStyle = false;
		Variant cls( getModel()->data( index, ModelRole::Class ) );
		cell->setLoadingState( true );
		if ( cls.isValid() ) {
			bool hasClass = false;

			hasClass =
				( cls.is( Variant::Type::cstr ) &&
				  cell->hasClass( std::string_view{ cls.asCStr() } ) ) ||
				( cls.is( Variant::Type::StdString ) && cell->hasClass( cls.asStdString() ) ) ||
				cell->hasClass( cls.toString() );

			needsReloadStyle =
				cell->getClasses().empty() || cell->getClasses().size() != 1 || !hasClass;

			if ( !hasClass )
				cell->setClass( cls.toString() );
		} else {
			needsReloadStyle = !cell->getClasses().empty();
			cell->resetClass();
		}
		cell->setLoadingState( false );
		if ( needsReloadStyle )
			cell->reportStyleStateChangeRecursive();
	}

	if ( getModel()->tooltipModelRoleEnabled() ) {
		Variant tooltip( getModel()->data( index, ModelRole::Tooltip ) );
		if ( tooltip.isValid() ) {
			if ( tooltip.is( Variant::Type::String ) )
				cell->setTooltipText( tooltip.asString() );
			else if ( tooltip.is( Variant::Type::StringPtr ) )
				cell->setTooltipText( tooltip.asStringPtr() );
			else
				cell->setTooltipText( tooltip.toString() );
		}
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

bool UIAbstractTableView::isRowHeaderVisible() const {
	return mRowHeaderWidth > 0;
}

void UIAbstractTableView::setRowHeaderVisible( bool rowHeaderVisible ) {
	setRowHeaderWidth( rowHeaderVisible ? PixelDensity::dpToPx( 30 ) : 0.f );
}

bool UIAbstractTableView::hasOnUpdateCellCb() const {
	return mOnUpdateCellCb != nullptr;
}

void UIAbstractTableView::setOnUpdateCellCb(
	const std::function<void( UITableCell*, Model* )>& onUpdateCellCb ) {
	mOnUpdateCellCb = onUpdateCellCb;
}

bool UIAbstractTableView::hasSetupCellCb() const {
	return mSetupCellCb != nullptr;
}

void UIAbstractTableView::setSetupCellCb(
	const std::function<void( UITableCell* )>& onSetupCellCb ) {
	mSetupCellCb = onSetupCellCb;
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
			image->setDrawable( DrawablePtr{} );
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
			DrawablePtr icon = mUISceneNode->findIconDrawable(
				sortOrder == SortOrder::Ascending ? "arrow-down" : "arrow-up", mSortIconSize );
			if ( icon )
				image->setDrawable( std::move( icon ) );
		}
		model->sort( colIndex, sortOrder );
	}
}

Uint32 UIAbstractTableView::onTextInput( const TextInputEvent& event ) {
	if ( !mRowSearchByName )
		return 0;
	if ( mSearchTextAction )
		removeAction( mSearchTextAction );
	mSearchTextAction =
		Actions::Runnable::New( [this] { resetSearchText(); }, Milliseconds( 750 ) );
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

void UIAbstractTableView::resetSearchText() {
	if ( mSearchTextAction )
		removeAction( mSearchTextAction );
	mSearchTextAction = nullptr;
	mSearchText = "";
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
		case PropertyId::IconSize:
			setIconSize(
				(size_t)lengthFromValue( attribute.getValue(), PropertyRelativeTarget::None ) );
			break;
		case PropertyId::SortIconSize:
			setSortIconSize(
				(size_t)lengthFromValue( attribute.getValue(), PropertyRelativeTarget::None ) );
			break;
		case PropertyId::MainColumn:
			setMainColumn( attribute.asInt() );
			break;
		case PropertyId::ColumnWidthMode:
			setColumnWidthMode( String::iequals( attribute.getValue(), "percentage" )
									? ColumnWidthMode::Percentage
									: ColumnWidthMode::Pixels );
			break;
		case PropertyId::ColumnWidthModeMenu:
			setColumnWidthModeMenuEnabled( attribute.asBool() );
			break;
		case PropertyId::RowHeaderWidth:
			setRowHeaderWidth(
				lengthFromValue( attribute.getValue(), PropertyRelativeTarget::None ) );
			break;
		case PropertyId::TableFlags: {
			Uint32 flags = 0;
			String::splitCb(
				[&flags]( std::string_view token ) {
					if ( String::iequals( token, "default" ) )
						flags |= UITABLE_DEFAULT_FLAGS;
					else if ( String::iequals( token, "headers" ) )
						flags |= TableFlagHeaders;
					else if ( String::iequals( token, "auto-expand" ) )
						flags |= TableFlagAutoExpand;
					else if ( String::iequals( token, "auto-columns" ) )
						flags |= TableFlagAutoColumns;
					else if ( String::iequals( token, "fit-columns" ) )
						flags |= TableFlagFitColumns;
					else if ( String::iequals( token, "single-click" ) )
						flags |= TableFlagSingleClick;
					else if ( String::iequals( token, "row-search" ) )
						flags |= TableFlagRowSearch;
					else if ( String::iequals( token, "row-header" ) )
						flags |= TableFlagRowHeader;
					return true;
				},
				attribute.getValue(), "|" );
			setTableFlags( flags );
			break;
		}
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
		case PropertyId::IconSize:
			return String::fromFloat( (Float)getIconSize(), "px" );
		case PropertyId::SortIconSize:
			return String::fromFloat( (Float)getSortIconSize(), "px" );
		case PropertyId::MainColumn:
			return String::toString( (Int64)getMainColumn() );
		case PropertyId::ColumnWidthMode:
			return getColumnWidthMode() == ColumnWidthMode::Percentage ? "percentage" : "pixels";
		case PropertyId::ColumnWidthModeMenu:
			return isColumnWidthModeMenuEnabled() ? "true" : "false";
		case PropertyId::RowHeaderWidth:
			return String::fromFloat( getRowHeaderWidth(), "px" );
		case PropertyId::TableFlags: {
			Uint32 flags = mTableFlags;
			std::string val;
			if ( flags & TableFlagHeaders )
				val += "headers|";
			if ( flags & TableFlagAutoExpand )
				val += "auto-expand|";
			if ( flags & TableFlagAutoColumns )
				val += "auto-columns|";
			if ( flags & TableFlagFitColumns )
				val += "fit-columns|";
			if ( flags & TableFlagSingleClick )
				val += "single-click|";
			if ( flags & TableFlagRowSearch )
				val += "row-search|";
			if ( flags & TableFlagRowHeader )
				val += "row-header|";
			if ( flags & TableFlagExpandersAsIcons )
				val += "expanders-as-icons|";
			if ( flags & TableFlagFocusOnSelection )
				val += "focus-on-selection|";
			if ( flags & TableFlagDisableClipping )
				val += "disable-clipping|";
			if ( !val.empty() )
				val.pop_back();
			return val;
		}
		default:
			return UIAbstractView::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIAbstractTableView::getPropertiesImplemented() const {
	auto props = UIAbstractView::getPropertiesImplemented();
	props.insert( props.end(), { PropertyId::RowHeight, PropertyId::IconSize,
								 PropertyId::SortIconSize, PropertyId::MainColumn,
								 PropertyId::ColumnWidthMode, PropertyId::ColumnWidthModeMenu,
								 PropertyId::RowHeaderWidth, PropertyId::TableFlags } );
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
	if ( mMainColumn != mainColumn ) {
		mMainColumn = mainColumn;
		createOrUpdateColumns( false );
	}
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

Uint32 UIAbstractTableView::getTableFlags() const {
	return mTableFlags;
}

void UIAbstractTableView::setTableFlags( Uint32 flags ) {
	mTableFlags = flags;
	setHeadersVisible( flags & TableFlagHeaders );
	setAutoExpandOnSingleColumn( flags & TableFlagAutoExpand );
	setAutoColumnsWidth( flags & TableFlagAutoColumns );
	setFitAllColumnsToWidget( flags & TableFlagFitColumns );
	setSingleClickNavigation( flags & TableFlagSingleClick );
	setRowSearchByName( flags & TableFlagRowSearch );
	setRowHeaderVisible( flags & TableFlagRowHeader );
}

UITableCell* UIAbstractTableView::getCellFromIndex( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return nullptr;
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
