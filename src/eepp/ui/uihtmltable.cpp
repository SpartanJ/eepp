#include <eepp/ui/tablelayouter.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uiscenenode.hpp>
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

std::vector<PropertyId> UIHTMLTable::getPropertiesImplemented() const {
	auto props = UIHTMLWidget::getPropertiesImplemented();
	auto local = { PropertyId::CellSpacing, PropertyId::CellPadding, PropertyId::TableLayout };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

std::string UIHTMLTable::getPropertyString( const PropertyDefinition* propertyDef,
											const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::CellSpacing:
			if ( const_cast<UIHTMLTable*>( this )->getLayouter() &&
				 mDisplay == CSSDisplay::Table ) {
				return String::fromFloat(
					static_cast<TableLayouter*>( const_cast<UIHTMLTable*>( this )->getLayouter() )
						->getCellSpacing() );
			}
			return "";
		case PropertyId::CellPadding:
			if ( const_cast<UIHTMLTable*>( this )->getLayouter() &&
				 mDisplay == CSSDisplay::Table ) {
				return String::fromFloat(
					static_cast<TableLayouter*>( const_cast<UIHTMLTable*>( this )->getLayouter() )
						->getCellPadding() );
			}
			return "";
		case PropertyId::TableLayout:
			return mTopEq;
		default:
			return UIHTMLWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UIHTMLTable::applyProperty( const StyleSheetProperty& attribute ) {
	if ( attribute.getPropertyDefinition() == nullptr )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::CellSpacing:
			if ( getLayouter() && mDisplay == CSSDisplay::Table ) {
				static_cast<TableLayouter*>( getLayouter() )
					->setCellSpacing( lengthFromValue( attribute ) );
				invalidateIntrinsicSize();
				tryUpdateLayout();
			}
			return true;
		case PropertyId::CellPadding:
			if ( getLayouter() && mDisplay == CSSDisplay::Table ) {
				static_cast<TableLayouter*>( getLayouter() )
					->setCellPadding( lengthFromValue( attribute ) );
				invalidateIntrinsicSize();
				tryUpdateLayout();
			}
			return true;
		case PropertyId::TableLayout:
			if ( getLayouter() && mDisplay == CSSDisplay::Table ) {
				static_cast<TableLayouter*>( getLayouter() )
					->setTableLayout( String::iequals( attribute.getValue(), "fixed" )
										  ? TableLayout::Fixed
										  : TableLayout::Auto );
				invalidateIntrinsicSize();
				tryUpdateLayout();
			}
			return true;
		default:
			break;
	}

	return UIHTMLWidget::applyProperty( attribute );
}

Float UIHTMLTable::cssWidthPropertyToBorderBoxWidth( const StyleSheetProperty& property ) const {
	return lengthFromValueForCSS( property );
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
			auto reasons = layoutInvalidationFromMessage( Msg );

			if ( reasons && ( reasons & ~toLayoutInvalidationFlags(
											LayoutInvalidationReason::PaintOnly ) ) == 0 )
				return 1;

			bool isChild = Msg->getSender() != this;
			// The table owns its cells until the complete layout-tree traversal finishes, not
			// just while TableLayouter is measuring rows. Descendants visited afterward can
			// change their content contribution; replay those reasons once in onLayoutUpdate()
			// instead of synchronously restarting every ancestor table for each child message.
			if ( isChild &&
				 ( isPacking() || ( mUISceneNode->isUpdatingLayouts() && mUpdatingLayoutTree ) ) ) {
				mTableDirtyReasons |= reasons;
				return 1;
			}

			if ( isChild && ( reasons & toLayoutInvalidationFlags(
											LayoutInvalidationReason::IntrinsicSize ) ) ) {
				if ( getLayouter() )
					getLayouter()->invalidateIntrinsicWidths();
			}

			if ( isChild ) {
				notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
			}

			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

void UIHTMLTable::onLayoutUpdate() {
	UIHTMLWidget::onLayoutUpdate();

	if ( !mTableDirtyReasons )
		return;

	LayoutInvalidationFlags reasons = mTableDirtyReasons;
	mTableDirtyReasons = 0;

	LayoutInvalidationFlags nonPaint =
		reasons & ~toLayoutInvalidationFlags( LayoutInvalidationReason::PaintOnly );
	if ( !nonPaint )
		return;

	if ( nonPaint & toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) ) {
		if ( getLayouter() )
			getLayouter()->invalidateIntrinsicWidths();
	}

	const Sizef oldSize = getPixelsSize();
	tryUpdateLayout();
	// Deferred descendant changes may have changed the table's intrinsic contribution even
	// when this replay leaves its used box unchanged. Ancestors may already have measured the
	// old contribution before the descendant tree traversal; let them measure the completed one.
	if ( oldSize != getPixelsSize() ||
		 ( nonPaint & toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) ) )
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
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

std::vector<PropertyId> UIHTMLTableCell::getPropertiesImplemented() const {
	auto props = UIHTMLWidget::getPropertiesImplemented();
	auto local = { PropertyId::ColSpan };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

std::string UIHTMLTableCell::getPropertyString( const PropertyDefinition* propertyDef,
												const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::ColSpan:
			return String::format( "%lld", mColSpan );
		default:
			return UIHTMLWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UIHTMLTableCell::applyProperty( const StyleSheetProperty& attribute ) {
	if ( attribute.getPropertyDefinition() == nullptr )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ColSpan: {
			mColSpan = attribute.asUint( 1 );
			if ( mColSpan == 0 )
				mColSpan = 1;
			notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
			return true;
		}
		default:
			break;
	}
	return UIRichText::applyProperty( attribute );
}

Uint32 UIHTMLTableCell::getColSpan() const {
	return mColSpan;
}

static UIHTMLTable* owningTableForCell( const UIHTMLTableCell* cell ) {
	Node* parent = cell->getParent();
	if ( !parent || !parent->isType( UI_TYPE_HTML_TABLE_ROW ) )
		return nullptr;

	for ( parent = parent->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isType( UI_TYPE_HTML_TABLE ) ) {
			auto* table = parent->asType<UIHTMLTable>();
			return table->getDisplay() == CSSDisplay::Table ? table : nullptr;
		}
	}
	return nullptr;
}

void UIHTMLTableCell::onSizeChange() {
	// CSS table layout measures cell content at the assigned column width, then commits the
	// row's used height. Both sizes are outputs of that same pass: re-entering cell layout or
	// invalidating intrinsic content widths here duplicates measurement and feeds geometry
	// notifications back into the table. Keep drawing/size events current; TableLayouter explicitly
	// measures the cell and the normal tree traversal still updates its positioned descendants.
	auto* table = owningTableForCell( this );
	if ( table && table->isPacking() )
		UIWidget::onSizeChange( false );
	else
		UIRichText::onSizeChange();
}

Uint32 UIHTMLTableCell::onMessage( const NodeMessage* Msg ) {
	if ( Msg->getMsg() == NodeMessage::LayoutAttributeChange ) {
		auto reasons = layoutInvalidationFromMessage( Msg );
		const auto paintOnly = toLayoutInvalidationFlags( LayoutInvalidationReason::PaintOnly );
		bool senderIsFixed =
			Msg->getSender()->isType( UI_TYPE_HTML_WIDGET ) &&
			Msg->getSender()->asConstType<UIHTMLWidget>()->getCSSPosition() == CSSPosition::Fixed;
		if ( ( reasons && ( reasons & ~paintOnly ) == 0 ) || senderIsFixed )
			return UIRichText::onMessage( Msg );

		auto* table = owningTableForCell( this );
		const auto contentReasons =
			toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) |
			toLayoutInvalidationFlags( LayoutInvalidationReason::FormattingContext );
		if ( table && ( table->isPacking() || isPacking() ) ) {
			// Child changes can arrive while the table assigns widths, or even after a cell
			// was measured (for example a size event creates content). Preserve them for the
			// completed tree pass instead of re-entering cell measurement during assignment.
			if ( reasons & contentReasons )
				invalidateIntrinsicSize();
			mDeferredLayoutReasons |= reasons;
			return 1;
		}
		if ( table && !mUpdatingLayoutTree && ( reasons & contentReasons ) ) {
			Uint32 result = UIRichText::onMessage( Msg );
			// A table fixes the used cell height to the row height. New content can therefore
			// leave the cell box unchanged even when its required content height grows. The
			// table must measure it again at wrap-content height, not wait for a box resize.
			notifyLayoutAttrChangeParent(
				reasons | toLayoutInvalidationFlags( LayoutInvalidationReason::NormalFlowChild ) );
			return result;
		}
	}
	return UIRichText::onMessage( Msg );
}

void UIHTMLTableCell::onLayoutUpdate() {
	LayoutInvalidationFlags deferred = mDeferredLayoutReasons;
	UIRichText::onLayoutUpdate();

	LayoutInvalidationFlags nonPaint =
		deferred & ~toLayoutInvalidationFlags( LayoutInvalidationReason::PaintOnly );

	if ( nonPaint & ( toLayoutInvalidationFlags( LayoutInvalidationReason::NormalFlowChild ) |
					  toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) |
					  toLayoutInvalidationFlags( LayoutInvalidationReason::FormattingContext ) ) ) {
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
	}
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
