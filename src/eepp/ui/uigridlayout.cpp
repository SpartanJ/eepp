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
	mRowHeight( 0 ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
}

Uint32 UIGridLayout::getType() const {
	return UI_TYPE_GRID_LAYOUT;
}

bool UIGridLayout::isType( const Uint32& type ) const {
	return UIGridLayout::getType() == type ? true : UIWidget::isType( type );
}

Sizei UIGridLayout::getBoxMargin() const {
	return mBoxMargin;
}

UIGridLayout* UIGridLayout::setBoxMargin( const Sizei& span ) {
	mBoxMargin = span;
	invalidateDraw();
	return this;
}

UIGridLayout::ElementMode UIGridLayout::getColumnMode() const {
	return mColumnMode;
}

UIGridLayout* UIGridLayout::setColumnMode( const UIGridLayout::ElementMode& mode ) {
	mColumnMode = mode;
	updateLayout();
	invalidateDraw();
	return this;
}

UIGridLayout::ElementMode UIGridLayout::getRowMode() const {
	return mRowMode;
}

UIGridLayout* UIGridLayout::setRowMode( const UIGridLayout::ElementMode& mode ) {
	mRowMode = mode;
	tryUpdateLayout();
	invalidateDraw();
	return this;
}

Float UIGridLayout::getColumnWeight() const {
	return mColumnWeight;
}

UIGridLayout* UIGridLayout::setColumnWeight( const Float& columnWeight ) {
	mColumnWeight = columnWeight;
	if ( mColumnMode == Weight )
		tryUpdateLayout();
	invalidateDraw();
	return this;
}

int UIGridLayout::getColumnWidth() const {
	return mColumnWidth;
}

UIGridLayout* UIGridLayout::setColumnWidth( int columnWidth ) {
	mColumnWidth = columnWidth;
	if ( mColumnMode == Size )
		tryUpdateLayout();
	invalidateDraw();
	return this;
}

int UIGridLayout::getRowHeight() const {
	return mRowHeight;
}

UIGridLayout* UIGridLayout::setRowHeight( int rowHeight ) {
	mRowHeight = rowHeight;
	if ( mRowMode == Size )
		tryUpdateLayout();
	invalidateDraw();
	return this;
}

Float UIGridLayout::getRowWeight() const {
	return mRowWeight;
}

UIGridLayout* UIGridLayout::setRowWeight( const Float& rowWeight ) {
	mRowWeight = rowWeight;
	if ( mRowMode == Weight )
		tryUpdateLayout();
	invalidateDraw();
	return this;
}

void UIGridLayout::updateLayout() {
	if ( mPacking )
		return;
	mPacking = true;

	if ( !mVisible ) {
		setInternalPixelsSize( Sizef::Zero );
		notifyLayoutAttrChangeParent();
		mPacking = false;
		mDirtyLayout = false;
		return;
	}

	Sizef oldSize( getSize() );

	if ( getParent()->isUINode() &&
		 ( !getParent()->asType<UINode>()->ownsChildPosition() || isGravityOwner() ) ) {
		setInternalPosition( Vector2f( mLayoutMargin.Left, mLayoutMargin.Top ) );
	}

	if ( getLayoutWidthPolicy() == SizePolicy::MatchParent )
		setInternalPixelsWidth( getMatchParentWidth() );

	if ( getLayoutHeightPolicy() == SizePolicy::MatchParent )
		setInternalPixelsHeight( getMatchParentHeight() );

	Node* ChildLoop = mChild;

	Vector2f pos( mPadding.Left, mPadding.Top );
	Sizef targetSize( getTargetElementSize() );
	Float initX = 0.f;

	if ( getHorizontalAlign() == UI_HALIGN_RIGHT )
		pos.x = getSize().getWidth() - targetSize.getWidth() - mPadding.Right;
	else if ( getHorizontalAlign() == UI_HALIGN_CENTER && getSize().getWidth() > 0 ) {
		initX =
			mPadding.Left + eeceil( ( (Int32)targetSize.getWidth() %
									  ( static_cast<Int32>( getSize().getWidth() - mPadding.Left -
															mPadding.Right ) ) ) *
									0.5f );
		pos.x = initX;
	}

	bool usedLastRow = true;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( ChildLoop );
			usedLastRow = true;

			if ( widget->getLayoutWeight() != 0.f )
				targetSize.x = widget->getLayoutWeight() *
							   ( getSize().getWidth() - mPadding.Left - mPadding.Right );

			widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
			if ( targetSize >= Sizef::Zero )
				widget->setSize( targetSize );
			widget->setPosition( pos );

			pos.x += getHorizontalAlign() == UI_HALIGN_RIGHT ? -targetSize.getWidth()
															 : targetSize.getWidth();

			if ( pos.x < mPadding.Left ||
				 pos.x + targetSize.x > getSize().getWidth() - mPadding.Right ||
				 pos.x + targetSize.x + mBoxMargin.x > getSize().getWidth() - mPadding.Right ) {

				if ( getHorizontalAlign() == UI_HALIGN_CENTER ) {
					pos.x = initX;
				} else if ( getHorizontalAlign() == UI_HALIGN_RIGHT ) {
					pos.x = getSize().getWidth() - mPadding.Right;
				} else {
					pos.x = mPadding.Left;
				}

				pos.y += targetSize.getHeight() + mBoxMargin.y;
				usedLastRow = false;
			} else {
				pos.x += getHorizontalAlign() == UI_HALIGN_RIGHT ? -mBoxMargin.x : mBoxMargin.x;
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		setInternalHeight( pos.y + ( usedLastRow ? targetSize.getHeight() : 0 ) + mPadding.Bottom );
	}

	if ( oldSize != getSize() ) {
		notifyLayoutAttrChangeParent();
	}

	invalidateDraw();
	mDirtyLayout = false;
	mPacking = false;
}

Uint32 UIGridLayout::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

void UIGridLayout::onParentSizeChange( const Vector2f& size ) {
	tryUpdateLayout();
	UILayout::onParentSizeChange( size );
}

Sizef UIGridLayout::getTargetElementSize() const {
	return Sizef( mColumnMode == Size ? mColumnWidth
									  : ( ( getLayoutHeightPolicy() == SizePolicy::WrapContent
												? getParent()->getSize().getWidth()
												: getSize().getWidth() ) -
										  mPadding.Left - mPadding.Right ) *
											mColumnWeight,
				  mRowMode == Size ? mRowHeight
								   : ( ( getLayoutHeightPolicy() == SizePolicy::WrapContent
											 ? getParent()->getSize().getHeight()
											 : getSize().getHeight() ) -
									   mPadding.Top - mPadding.Bottom ) *
										 mRowWeight );
}

std::string UIGridLayout::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::ColumnMargin:
			return String::format( "%ddp", mBoxMargin.x );
		case PropertyId::RowMargin:
			return String::format( "%ddp", mBoxMargin.y );
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

std::vector<PropertyId> UIGridLayout::getPropertiesImplemented() const {
	auto props = UILayout::getPropertiesImplemented();
	auto local = { PropertyId::ColumnMargin, PropertyId::RowMargin,	   PropertyId::ColumnMode,
				   PropertyId::RowMode,		 PropertyId::ColumnWeight, PropertyId::RowWeight,
				   PropertyId::ColumnWidth,	 PropertyId::RowHeight,	   PropertyId::ReverseDraw };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIGridLayout::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ColumnMargin:
			setBoxMargin( Sizei( attribute.asDpDimensionI( this ), mBoxMargin.y ) );
			break;
		case PropertyId::RowMargin:
			setBoxMargin( Sizei( mBoxMargin.x, attribute.asDpDimensionI( this ) ) );
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
			setColumnWidth( attribute.asDpDimensionI( this ) );
			break;
		case PropertyId::RowWeight:
			setRowWeight( attribute.asFloat() );
			break;
		case PropertyId::RowHeight:
			setRowHeight( attribute.asDpDimensionI( this ) );
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
