#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uigridlayout.hpp>

namespace EE { namespace UI {

UIGridLayout* UIGridLayout::New() {
	return eeNew( UIGridLayout, () );
}

UIGridLayout::UIGridLayout() :
	UILayout( "gridlayout" ),
	mColumnMode( Weight ),
	mRowMode( Weight ),
	mColumnWeight( 0.25f ),
	mColumnWidth( 0 ),
	mRowWeight( 0.25f ),
	mRowHeight( 0 ) {}

Uint32 UIGridLayout::getType() const {
	return UI_TYPE_GRID_LAYOUT;
}

bool UIGridLayout::isType( const Uint32& type ) const {
	return UIGridLayout::getType() == type ? true : UIWidget::isType( type );
}

Sizei UIGridLayout::getSpan() const {
	return mSpan;
}

UIGridLayout* UIGridLayout::setSpan( const Sizei& span ) {
	mSpan = span;
	invalidateDraw();
	return this;
}

UIGridLayout::ElementMode UIGridLayout::getColumnMode() const {
	return mColumnMode;
}

UIGridLayout* UIGridLayout::setColumnMode( const UIGridLayout::ElementMode& mode ) {
	mColumnMode = mode;
	pack();
	invalidateDraw();
	return this;
}

UIGridLayout::ElementMode UIGridLayout::getRowMode() const {
	return mRowMode;
}

UIGridLayout* UIGridLayout::setRowMode( const UIGridLayout::ElementMode& mode ) {
	mRowMode = mode;
	pack();
	invalidateDraw();
	return this;
}

Float UIGridLayout::getColumnWeight() const {
	return mColumnWeight;
}

UIGridLayout* UIGridLayout::setColumnWeight( const Float& columnWeight ) {
	mColumnWeight = columnWeight;
	if ( mColumnMode == Weight )
		pack();
	invalidateDraw();
	return this;
}

int UIGridLayout::getColumnWidth() const {
	return mColumnWidth;
}

UIGridLayout* UIGridLayout::setColumnWidth( int columnWidth ) {
	mColumnWidth = columnWidth;
	if ( mColumnMode == Size )
		pack();
	invalidateDraw();
	return this;
}

int UIGridLayout::getRowHeight() const {
	return mRowHeight;
}

UIGridLayout* UIGridLayout::setRowHeight( int rowHeight ) {
	mRowHeight = rowHeight;
	if ( mRowMode == Size )
		pack();
	invalidateDraw();
	return this;
}

Float UIGridLayout::getRowWeight() const {
	return mRowWeight;
}

UIGridLayout* UIGridLayout::setRowWeight( const Float& rowWeight ) {
	mRowWeight = rowWeight;
	if ( mRowMode == Weight )
		pack();
	invalidateDraw();
	return this;
}

void UIGridLayout::onSizeChange() {
	pack();
	UIWidget::onSizeChange();
}

void UIGridLayout::onChildCountChange() {
	pack();
	UIWidget::onChildCountChange();
}

void UIGridLayout::onPaddingChange() {
	pack();
	UILayout::onPaddingChange();
}

void UIGridLayout::onParentSizeChange( const Vector2f& SizeChange ) {
	pack();
	UIWidget::onParentSizeChange( SizeChange );
}

void UIGridLayout::pack() {
	Sizef oldSize( getSize() );

	setInternalPosition( Vector2f( mLayoutMargin.Left, mLayoutMargin.Top ) );

	if ( getLayoutWidthRule() == LayoutSizeRule::MatchParent ) {
		setInternalWidth( getParent()->getSize().getWidth() - mLayoutMargin.Left -
						  mLayoutMargin.Right );
	}

	if ( getLayoutHeightRule() == LayoutSizeRule::MatchParent ) {
		setInternalHeight( getParent()->getSize().getHeight() - mLayoutMargin.Top -
						   mLayoutMargin.Bottom );
	}

	Node* ChildLoop = mChild;

	Vector2f pos( mPadding.Left, mPadding.Top );
	Sizef targetSize( getTargetElementSize() );

	if ( getHorizontalAlign() == UI_HALIGN_RIGHT )
		pos.x = getSize().getWidth() - mPadding.Right;

	bool usedLastRow = true;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( ChildLoop );
			usedLastRow = true;

			if ( widget->getLayoutWeight() != 0.f )
				targetSize.x = widget->getLayoutWeight() *
							   ( getSize().getWidth() - mPadding.Left - mPadding.Right );

			widget->setLayoutSizeRules( LayoutSizeRule::Fixed, LayoutSizeRule::Fixed );
			if ( targetSize >= Sizef::Zero )
				widget->setSize( targetSize );
			widget->setPosition( pos );

			pos.x += getHorizontalAlign() == UI_HALIGN_RIGHT ? -targetSize.getWidth()
															 : targetSize.getWidth();

			if ( pos.x < mPadding.Left ||
				 pos.x + targetSize.x > getSize().getWidth() - mPadding.Right ||
				 pos.x + targetSize.x + mSpan.x > getSize().getWidth() - mPadding.Right ) {
				pos.x = getHorizontalAlign() == UI_HALIGN_RIGHT
							? getSize().getWidth() - mPadding.Right
							: mPadding.Left;

				pos.y += targetSize.getHeight() + mSpan.y;
				usedLastRow = false;
			} else {
				pos.x += getHorizontalAlign() == UI_HALIGN_RIGHT ? -mSpan.x : mSpan.x;
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutHeightRule() == LayoutSizeRule::WrapContent ) {
		setInternalHeight( pos.y + ( usedLastRow ? targetSize.getHeight() : 0 ) + mPadding.Bottom );
	}

	if ( oldSize != getSize() ) {
		notifyLayoutAttrChangeParent();
	}

	invalidateDraw();
}

Uint32 UIGridLayout::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			pack();
			break;
		}
	}

	return 0;
}

Sizef UIGridLayout::getTargetElementSize() const {
	return Sizef( mColumnMode == Size ? mColumnWidth
									  : ( ( getLayoutHeightRule() == LayoutSizeRule::WrapContent
												? getParent()->getSize().getWidth()
												: getSize().getWidth() ) -
										  mPadding.Left - mPadding.Right ) *
											mColumnWeight,
				  mRowMode == Size ? mRowHeight
								   : ( ( getLayoutHeightRule() == LayoutSizeRule::WrapContent
											 ? getParent()->getSize().getHeight()
											 : getSize().getHeight() ) -
									   mPadding.Top - mPadding.Bottom ) *
										 mRowWeight );
}

std::string UIGridLayout::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::ColumnSpan:
			return String::format( "%ddp", mSpan.x );
		case PropertyId::RowSpan:
			return String::format( "%ddp", mSpan.y );
		case PropertyId::ColumnMode:
			return getColumnMode() == Size ? "size" : "weight";
		case PropertyId::RowMode:
			return getRowMode() == Size ? "size" : "weight";
		case PropertyId::ColumnWeight:
			return String::fromFloat( getColumnWeight() );
		case PropertyId::RowWeight:
			return String::fromFloat( getRowWeight() );
		case PropertyId::ColumnWidth:
			return String::format( "%ddp", getColumnWidth() );
		case PropertyId::RowHeight:
			return String::format( "%ddp", getRowHeight() );
		case PropertyId::ReverseDraw:
			return isReverseDraw() ? "true" : "false";
		default:
			return UILayout::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UIGridLayout::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ColumnSpan:
			setSpan( Sizei( attribute.asDpDimensionI(), mSpan.y ) );
			break;
		case PropertyId::RowSpan:
			setSpan( Sizei( mSpan.x, attribute.asDpDimensionI() ) );
			break;
		case PropertyId::ColumnMode: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );
			setColumnMode( "size" == val ? Size : Weight );
			break;
		}
		case PropertyId::RowMode: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );
			setRowMode( "size" == val ? Size : Weight );
			break;
		}
		case PropertyId::ColumnWeight:
			setColumnWeight( attribute.asFloat() );
			break;
		case PropertyId::ColumnWidth:
			setColumnWidth( attribute.asDpDimensionI() );
			break;
		case PropertyId::RowWeight:
			setRowWeight( attribute.asDpDimension() );
			break;
		case PropertyId::RowHeight:
			setRowHeight( attribute.asDpDimensionI() );
			break;
		case PropertyId::ReverseDraw:
			setReverseDraw( attribute.asBool() );
			break;
		default:
			return UILayout::applyProperty( attribute );
	}

	return true;
}

}} // namespace EE::UI
