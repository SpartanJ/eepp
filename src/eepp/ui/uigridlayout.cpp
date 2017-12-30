#include <eepp/ui/uigridlayout.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {
	
UIGridLayout * UIGridLayout::New() {
	return eeNew( UIGridLayout, () );
}

UIGridLayout::UIGridLayout() :
	UILayout(),
	mColumnMode( Weight ),
	mRowMode( Weight  ),
	mColumnWeight( 0.25f ),
	mColumnWidth( 0 ),
	mRowWeight( 0.25f ),
	mRowHeight( 0 )
{
}

Uint32 UIGridLayout::getType() const {
	return UI_TYPE_GRID_LAYOUT;
}

bool UIGridLayout::isType( const Uint32& type ) const {
	return UIGridLayout::getType() == type ? true : UIWidget::isType( type );
}

Sizei UIGridLayout::getSpan() const {
	return mSpan;
}

UIGridLayout * UIGridLayout::setSpan(const Sizei & span) {
	mSpan = span;
	invalidateDraw();
	return this;
}

UIGridLayout::ElementMode UIGridLayout::getColumnMode() const {
	return mColumnMode;
}

UIGridLayout * UIGridLayout::setColumnMode(const UIGridLayout::ElementMode & mode) {
	mColumnMode = mode;
	pack();
	invalidateDraw();
	return this;
}

UIGridLayout::ElementMode UIGridLayout::getRowMode() const {
	return mRowMode;
}

UIGridLayout *UIGridLayout::setRowMode(const UIGridLayout::ElementMode & mode) {
	mRowMode = mode;
	pack();
	invalidateDraw();
	return this;
}

Float UIGridLayout::getColumnWeight() const {
	return mColumnWeight;
}

UIGridLayout * UIGridLayout::setColumnWeight(const Float & columnWeight) {
	mColumnWeight = columnWeight;
	if ( mColumnMode == Weight )
		pack();
	invalidateDraw();
	return this;
}

int UIGridLayout::getColumnWidth() const {
	return mColumnWidth;
}

UIGridLayout * UIGridLayout::setColumnWidth(int columnWidth) {
	mColumnWidth = columnWidth;
	if ( mColumnMode == Size )
		pack();
	invalidateDraw();
	return this;
}

int UIGridLayout::getRowHeight() const {
	return mRowHeight;
}

UIGridLayout * UIGridLayout::setRowHeight(int rowHeight) {
	mRowHeight = rowHeight;
	if ( mRowMode == Size )
		pack();
	invalidateDraw();
	return this;
}

Float UIGridLayout::getRowWeight() const {
	return mRowWeight;
}

UIGridLayout * UIGridLayout::setRowWeight(const Float & rowWeight) {
	mRowWeight = rowWeight;
	if ( mRowMode == Weight )
		pack();
	invalidateDraw();
	return this;
}

Rect UIGridLayout::getPadding() const {
	return mPadding;
}

void UIGridLayout::setPadding(const Rect & padding) {
	mPadding = padding;
	pack();
}

void UIGridLayout::onSizeChange() {
	pack();
	UIWidget::onSizeChange();
}

void UIGridLayout::onChildCountChange() {
	pack();
	UIWidget::onChildCountChange();
}

void UIGridLayout::onParentSizeChange(const Vector2i & SizeChange) {
	pack();
	UIWidget::onParentSizeChange( SizeChange );
}

void UIGridLayout::pack() {
	Sizei oldSize( mSize );

	//setInternalPosition( Vector2i( mLayoutMargin.Left, mLayoutMargin.Top ) );

	if ( getLayoutWidthRules() == MATCH_PARENT ) {
		setInternalWidth( getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right );
	}

	if ( getLayoutHeightRules() == MATCH_PARENT ) {
		setInternalHeight( getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom );
	}

	UIControl * ChildLoop = mChild;

	Vector2i pos(mPadding.Left,mPadding.Top);
	Sizei targetSize( getTargetElementSize() );

	if ( getHorizontalAlign() == UI_HALIGN_RIGHT )
		pos.x = mSize.getWidth() - mPadding.Right;

	bool usedLastRow = true;
	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );
			usedLastRow = true;

			if ( widget->getLayoutWeight() != 0.f )
				targetSize.x = widget->getLayoutWeight() * ( mSize.getWidth() - mPadding.Left - mPadding.Right );

			widget->setSize( targetSize );
			widget->setPosition( pos );

			pos.x += getHorizontalAlign() == UI_HALIGN_RIGHT ? -targetSize.getWidth() : targetSize.getWidth();

			if ( pos.x < mPadding.Left || pos.x + targetSize.x > mSize.getWidth() - mPadding.Right || pos.x + targetSize.x + mSpan.x > mSize.getWidth() - mPadding.Right ) {
				pos.x = getHorizontalAlign() == UI_HALIGN_RIGHT ? mSize.getWidth() - mPadding.Right : mPadding.Left;

				pos.y += targetSize.getHeight() + mSpan.y;
				usedLastRow = false;
			} else {
				pos.x += getHorizontalAlign() == UI_HALIGN_RIGHT ? -mSpan.x : mSpan.x;
			}
		}

		ChildLoop = ChildLoop->getNextControl();
	}

	if ( getLayoutHeightRules() == WRAP_CONTENT ) {
		setInternalHeight( pos.y + ( usedLastRow ? targetSize.getHeight() : 0 ) );
	}

	if ( oldSize != mSize ) {
		notifyLayoutAttrChangeParent();
	}

	invalidateDraw();
}

Uint32 UIGridLayout::onMessage(const UIMessage * Msg) {
	switch( Msg->getMsg() ) {
		case UIMessage::LayoutAttributeChange:
		{
			pack();
			break;
		}
	}

	return 0;
}

Sizei UIGridLayout::getTargetElementSize() {
	return Sizei( mColumnMode == Size ? mColumnWidth : ( ( getLayoutHeightRules() == WRAP_CONTENT ? getParent()->getSize().getWidth() : mSize.getWidth() ) - mPadding.Left - mPadding.Right ) * mColumnWeight,
				  mRowMode == Size ? mRowHeight : ( ( getLayoutHeightRules() == WRAP_CONTENT ? getParent()->getSize().getHeight() : mSize.getHeight() ) - mPadding.Top - mPadding.Bottom ) * mRowWeight );
}

void UIGridLayout::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "columnspan" == name ) {
			setSpan( Sizei( ait->as_int(), mSpan.y ) );
		} else if ( "rowspan" == name ) {
			setSpan( Sizei( mSpan.x, ait->as_int() ) );
		} else if ( "span" == name ) {
			setSpan( Sizei( ait->as_int(), ait->as_int() ) );
		} else if ( "columnmode" == name ) {
			std::string val( ait->as_string() );
			String::toLowerInPlace( val );
			setColumnMode( "size" == val ? Size : Weight );
		} else if ( "rowmode" == name ) {
			std::string val( ait->as_string() );
			String::toLowerInPlace( val );
			setRowMode( "size" == val ? Size : Weight );
		} else if ( "columnweight" == name ) {
			setColumnWeight( ait->as_float() );
		} else if ( "columnwidth" == name ) {
			setColumnWidth( ait->as_int() );
		} else if ( "rowweight" == name ) {
			setRowWeight( ait->as_float() );
		} else if ( "rowheight" == name ) {
			setRowHeight( ait->as_int() );
		} else if ( "padding" == name ) {
			int val = PixelDensity::toDpFromStringI( ait->as_string() );
			setPadding( Rect( val, val, val, val ) );
		} else if ( "paddingleft" == name ) {
			setPadding( Rect( PixelDensity::toDpFromStringI( ait->as_string() ), mPadding.Top, mPadding.Right, mPadding.Bottom ) );
		} else if ( "paddingright" == name ) {
			setPadding( Rect( mPadding.Left, mPadding.Top, PixelDensity::toDpFromStringI( ait->as_string() ), mPadding.Bottom ) );
		} else if ( "paddingtop" == name ) {
			setPadding( Rect( mPadding.Left, PixelDensity::toDpFromStringI( ait->as_string() ), mPadding.Right, mPadding.Bottom ) );
		} else if ( "paddingbottom" == name ) {
			setPadding( Rect( mPadding.Left, mPadding.Top, mPadding.Right, PixelDensity::toDpFromStringI( ait->as_string() ) ) );
		} else if ( "reversedraw" == name ) {
			setReverseDraw( ait->as_bool() );
		}
	}

	endPropertiesTransaction();
}

}} 
