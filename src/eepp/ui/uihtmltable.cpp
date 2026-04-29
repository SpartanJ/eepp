#include <eepp/ui/tablelayouter.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

UIHTMLTable* UIHTMLTable::New() {
	return eeNew( UIHTMLTable, () );
}

UIHTMLTable::UIHTMLTable() : UIHTMLWidget( "table" ) {
	mDisplay = CSSDisplay::Table;
	mFlags |= UI_HTML_ELEMENT | UI_OWNS_CHILDREN_POSITION;
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTable::getType() const {
	return UI_TYPE_HTML_TABLE;
}

bool UIHTMLTable::isType( const Uint32& type ) const {
	return UIHTMLTable::getType() == type || UIHTMLWidget::isType( type );
}

bool UIHTMLTable::applyProperty( const StyleSheetProperty& attribute ) {
	if ( attribute.getPropertyDefinition() == nullptr )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Cellspacing:
			if ( const_cast<UIHTMLTable*>( this )->getLayouter() ) {
				static_cast<TableLayouter*>( const_cast<UIHTMLTable*>( this )->getLayouter() )
					->setCellspacing( lengthFromValue( attribute ) );
				invalidateIntrinsicSize();
				tryUpdateLayout();
			}
			return true;
		case PropertyId::Cellpadding:
			if ( const_cast<UIHTMLTable*>( this )->getLayouter() ) {
				static_cast<TableLayouter*>( const_cast<UIHTMLTable*>( this )->getLayouter() )
					->setCellpadding( lengthFromValue( attribute ) );
				invalidateIntrinsicSize();
				tryUpdateLayout();
			}
			return true;
		case PropertyId::TableLayout: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );
			if ( const_cast<UIHTMLTable*>( this )->getLayouter() ) {
				static_cast<TableLayouter*>( const_cast<UIHTMLTable*>( this )->getLayouter() )
					->setTableLayout( val == "fixed" ? TableLayout::Fixed : TableLayout::Auto );
				invalidateIntrinsicSize();
				tryUpdateLayout();
			}
			return true;
		}
		default:
			break;
	}

	return UIHTMLWidget::applyProperty( attribute );
}

void UIHTMLTable::computeIntrinsicWidths() const {
	UILayouter* layouter = const_cast<UIHTMLTable*>( this )->getLayouter();
	if ( layouter )
		layouter->computeIntrinsicWidths();
}

Float UIHTMLTable::getMinIntrinsicWidth() const {
	computeIntrinsicWidths();
	UILayouter* layouter = const_cast<UIHTMLTable*>( this )->getLayouter();
	if ( layouter )
		return static_cast<TableLayouter*>( layouter )->getMinIntrinsicWidth();
	return 0;
}

Float UIHTMLTable::getMaxIntrinsicWidth() const {
	computeIntrinsicWidths();
	UILayouter* layouter = const_cast<UIHTMLTable*>( this )->getLayouter();
	if ( layouter )
		return static_cast<TableLayouter*>( layouter )->getMaxIntrinsicWidth();
	return 0;
}

Uint32 UIHTMLTable::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			if ( Msg->getSender() != this && !isPacking() ) {
				if ( getLayouter() )
					getLayouter()->invalidateIntrinsicWidths();
				notifyLayoutAttrChangeParent();
			}
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

UIHTMLTableRow* UIHTMLTableRow::New() {
	return eeNew( UIHTMLTableRow, () );
}

UIHTMLTableRow::UIHTMLTableRow() : UIHTMLWidget( "tr" ) {
	mDisplay = CSSDisplay::TableRow;
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableRow::getType() const {
	return UI_TYPE_HTML_TABLE_ROW;
}

bool UIHTMLTableRow::isType( const Uint32& type ) const {
	return UIHTMLTableRow::getType() == type || UIHTMLWidget::isType( type );
}

UIHTMLTableCell* UIHTMLTableCell::New( const std::string& tag ) {
	return eeNew( UIHTMLTableCell, ( tag ) );
}

UIHTMLTableCell::UIHTMLTableCell( const std::string& tag ) : UIRichText( tag ) {
	mDisplay = CSSDisplay::TableCell;
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableCell::getType() const {
	return UI_TYPE_HTML_TABLE_CELL;
}

bool UIHTMLTableCell::isType( const Uint32& type ) const {
	return UIHTMLTableCell::getType() == type || UIRichText::isType( type );
}

bool UIHTMLTableCell::applyProperty( const StyleSheetProperty& attribute ) {
	if ( attribute.getPropertyDefinition() == nullptr )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Colspan: {
			mColspan = attribute.asUint( 1 );
			if ( mColspan == 0 )
				mColspan = 1;
			notifyLayoutAttrChangeParent();
			return true;
		}
		default:
			break;
	}
	return UIRichText::applyProperty( attribute );
}

Uint32 UIHTMLTableCell::getColspan() const {
	return mColspan;
}

void UIHTMLTableCell::onSizeChange() {
	UIRichText::onSizeChange();
}

UIHTMLTableHead* UIHTMLTableHead::New() {
	return eeNew( UIHTMLTableHead, () );
}

UIHTMLTableHead::UIHTMLTableHead() : UIHTMLWidget( "thead" ) {
	mDisplay = CSSDisplay::TableHead;
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableHead::getType() const {
	return UI_TYPE_HTML_TABLE_HEAD;
}

bool UIHTMLTableHead::isType( const Uint32& type ) const {
	return UIHTMLTableHead::getType() == type || UIHTMLWidget::isType( type );
}

UIHTMLTableBody* UIHTMLTableBody::New() {
	return eeNew( UIHTMLTableBody, () );
}

UIHTMLTableBody::UIHTMLTableBody() : UIHTMLWidget( "tbody" ) {
	mDisplay = CSSDisplay::TableBody;
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableBody::getType() const {
	return UI_TYPE_HTML_TABLE_BODY;
}

bool UIHTMLTableBody::isType( const Uint32& type ) const {
	return UIHTMLTableBody::getType() == type || UIHTMLWidget::isType( type );
}

UIHTMLTableFooter* UIHTMLTableFooter::New() {
	return eeNew( UIHTMLTableFooter, () );
}

UIHTMLTableFooter::UIHTMLTableFooter() : UIHTMLWidget( "tfoot" ) {
	mDisplay = CSSDisplay::TableFooter;
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableFooter::getType() const {
	return UI_TYPE_HTML_TABLE_FOOTER;
}

bool UIHTMLTableFooter::isType( const Uint32& type ) const {
	return UIHTMLTableFooter::getType() == type || UIHTMLWidget::isType( type );
}

}} // namespace EE::UI
