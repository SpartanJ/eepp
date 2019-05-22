#include <eepp/ui/uigridlayout.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {
	
UIGridLayout * UIGridLayout::New() {
	return eeNew( UIGridLayout, () );
}

UIGridLayout::UIGridLayout() :
	UILayout( "gridlayout" ),
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

void UIGridLayout::onParentSizeChange(const Vector2f& SizeChange) {
	pack();
	UIWidget::onParentSizeChange( SizeChange );
}

void UIGridLayout::pack() {
	Sizef oldSize( mDpSize );

	//setInternalPosition( Vector2i( mLayoutMargin.Left, mLayoutMargin.Top ) );

	if ( getLayoutWidthRules() == MATCH_PARENT ) {
		setInternalWidth( getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right );
	}

	if ( getLayoutHeightRules() == MATCH_PARENT ) {
		setInternalHeight( getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom );
	}

	Node * ChildLoop = mChild;

	Vector2f pos(mPadding.Left,mPadding.Top);
	Sizef targetSize( getTargetElementSize() );

	if ( getHorizontalAlign() == UI_HALIGN_RIGHT )
		pos.x = mDpSize.getWidth() - mPadding.Right;

	bool usedLastRow = true;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );
			usedLastRow = true;

			if ( widget->getLayoutWeight() != 0.f )
				targetSize.x = widget->getLayoutWeight() * ( mDpSize.getWidth() - mPadding.Left - mPadding.Right );

			widget->setLayoutSizeRules( FIXED, FIXED );
			widget->setSize( targetSize );
			widget->setPosition( pos );

			pos.x += getHorizontalAlign() == UI_HALIGN_RIGHT ? -targetSize.getWidth() : targetSize.getWidth();

			if ( pos.x < mPadding.Left || pos.x + targetSize.x > mDpSize.getWidth() - mPadding.Right || pos.x + targetSize.x + mSpan.x > mDpSize.getWidth() - mPadding.Right ) {
				pos.x = getHorizontalAlign() == UI_HALIGN_RIGHT ? mDpSize.getWidth() - mPadding.Right : mPadding.Left;

				pos.y += targetSize.getHeight() + mSpan.y;
				usedLastRow = false;
			} else {
				pos.x += getHorizontalAlign() == UI_HALIGN_RIGHT ? -mSpan.x : mSpan.x;
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutHeightRules() == WRAP_CONTENT ) {
		setInternalHeight( pos.y + ( usedLastRow ? targetSize.getHeight() : 0 ) + mPadding.Bottom );
	}

	if ( oldSize != mDpSize ) {
		notifyLayoutAttrChangeParent();
	}

	invalidateDraw();
}

Uint32 UIGridLayout::onMessage(const NodeMessage * Msg) {
	switch( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange:
		{
			pack();
			break;
		}
	}

	return 0;
}

Sizef UIGridLayout::getTargetElementSize() const {
	return Sizef( mColumnMode == Size ? mColumnWidth : ( ( getLayoutHeightRules() == WRAP_CONTENT ? getParent()->getSize().getWidth() : mDpSize.getWidth() ) - mPadding.Left - mPadding.Right ) * mColumnWeight,
				  mRowMode == Size ? mRowHeight : ( ( getLayoutHeightRules() == WRAP_CONTENT ? getParent()->getSize().getHeight() : mDpSize.getHeight() ) - mPadding.Top - mPadding.Bottom ) * mRowWeight );
}

bool UIGridLayout::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "columnspan" == name ) {
		setSpan( Sizei( attribute.asInt(), mSpan.y ) );
	} else if ( "rowspan" == name ) {
		setSpan( Sizei( mSpan.x, attribute.asInt() ) );
	} else if ( "span" == name ) {
		setSpan( Sizei( attribute.asInt(), attribute.asInt() ) );
	} else if ( "columnmode" == name ) {
		std::string val( attribute.asString() );
		String::toLowerInPlace( val );
		setColumnMode( "size" == val ? Size : Weight );
	} else if ( "rowmode" == name ) {
		std::string val( attribute.asString() );
		String::toLowerInPlace( val );
		setRowMode( "size" == val ? Size : Weight );
	} else if ( "columnweight" == name ) {
		setColumnWeight( attribute.asFloat() );
	} else if ( "columnwidth" == name ) {
		setColumnWidth( attribute.asInt() );
	} else if ( "rowweight" == name ) {
		setRowWeight( attribute.asFloat() );
	} else if ( "rowheight" == name ) {
		setRowHeight( attribute.asInt() );
	} else if ( "reversedraw" == name ) {
		setReverseDraw( attribute.asBool() );
	} else {
		return UILayout::setAttribute( attribute, state );
	}

	return true;
}

}} 
