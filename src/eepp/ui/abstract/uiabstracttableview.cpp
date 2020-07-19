#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>

namespace EE { namespace UI { namespace Abstract {

UIAbstractTableView::UIAbstractTableView( const std::string& tag ) :
	UIAbstractView( tag ), mDragBorderDistance( PixelDensity::dpToPx( 4 ) ) {
	mHeader = UILinearLayout::NewWithTag( "table::header", UIOrientation::Horizontal );
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
	return eeceil( columnData( 0 ).widget ? columnData( 0 ).widget->getPixelsSize().getHeight()
										  : 16 );
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
	UIAbstractView::onModelUpdate( flags );
	createOrUpdateColumns();
}

void UIAbstractTableView::createOrUpdateColumns() {
	Model* model = getModel();
	if ( !model )
		return;

	size_t count = model->columnCount();
	Float totalWidth = 0;

	for ( size_t i = 0; i < count; i++ ) {
		ColumnData& col = columnData( i );
		if ( !col.widget ) {
			col.widget = eeNew( UITableHeaderColumn, ( this, i ) );
			col.widget->setParent( mHeader );
			col.widget->setEnabled( true );
			col.widget->setVisible( true );
		}
		col.visible = !isColumnHidden( i );
		col.widget->setVisible( col.visible );
		if ( !col.visible )
			continue;
		col.widget->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
		col.widget->setText( model->columnName( i ) );
		col.widget->reloadStyle( true, true, true );
		col.width = eeceil( eemax( col.width, col.widget->getPixelsSize().getWidth() ) );
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
	return areHeadersVisible() ? eeceil( columnData( 0 ).widget
											 ? columnData( 0 ).widget->getPixelsSize().getHeight()
											 : 16 )
							   : 0;
}

Sizef UIAbstractTableView::getContentSize() const {
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

void UIAbstractTableView::onColumnSizeChange( const size_t& ) {}

void UIAbstractTableView::onColumnResizeToContent( const size_t& ) {}

void UIAbstractTableView::updateHeaderSize() {
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

void UIAbstractTableView::updateColumnsWidth() {
	int col = 0;
	Float width =
		eefloor( getPixelsSize().getWidth() - getPixelsPadding().Left - getPixelsPadding().Right -
				 ( mVScroll->isVisible() ? mVScroll->getPixelsSize().getWidth() : 0 ) );

	if ( visibleColumnCount() == 1 && ( col = visibleColumn() ) != -1 ) {
		columnData( col ).width = width;
		updateHeaderSize();
		onColumnSizeChange( col );
	}
}

void UIAbstractTableView::updateScroll() {
	UIAbstractView::updateScroll();
	updateColumnsWidth();
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

}}} // namespace EE::UI::Abstract
