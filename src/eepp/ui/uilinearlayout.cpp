#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

UILinearLayout* UILinearLayout::NewWithTag( const std::string& tag,
											const UIOrientation& orientation ) {
	return eeNew( UILinearLayout, ( tag, orientation ) );
}

UILinearLayout* UILinearLayout::New() {
	return eeNew( UILinearLayout, () );
}

UILinearLayout* UILinearLayout::NewVertical() {
	return eeNew( UILinearLayout, () );
}

UILinearLayout* UILinearLayout::NewHorizontal() {
	return ( eeNew( UILinearLayout, () ) )->setOrientation( UIOrientation::Horizontal );
}

UILinearLayout::UILinearLayout() :
	UILayout( "linearlayout" ), mOrientation( UIOrientation::Vertical ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
	setClipType( ClipType::ContentBox );
}

UILinearLayout::UILinearLayout( const std::string& tag, const UIOrientation& orientation ) :
	UILayout( tag ), mOrientation( orientation ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
	setClipType( ClipType::ContentBox );
}

Uint32 UILinearLayout::getType() const {
	return UI_TYPE_LINEAR_LAYOUT;
}

bool UILinearLayout::isType( const Uint32& type ) const {
	return UILinearLayout::getType() == type ? true : UILayout::isType( type );
}

UIOrientation UILinearLayout::getOrientation() const {
	return mOrientation;
}

UILinearLayout* UILinearLayout::setOrientation( const UIOrientation& orientation ) {
	mOrientation = orientation;
	return this;
}

void UILinearLayout::updateLayout() {
	if ( !mVisible ) {
		if ( mPacking )
			return;
		mPacking = true;
		setInternalPixelsSize( Sizef::Zero );
		notifyLayoutAttrChangeParent();
		mPacking = false;
	} else {
		if ( mOrientation == UIOrientation::Vertical )
			packVertical();
		else
			packHorizontal();
	}
	mDirtyLayout = false;
}

void UILinearLayout::applyWidthPolicyOnChilds() {
	Node* child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() && child->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );

			if ( widget->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
				widget->setFlags( UI_AUTO_SIZE );
			}

			switch ( widget->getLayoutWidthPolicy() ) {
				case SizePolicy::WrapContent: {
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case SizePolicy::MatchParent: {
					int w = getPixelsSize().getWidth() - widget->getLayoutPixelsMargin().Left -
							widget->getLayoutPixelsMargin().Right - mPaddingPx.Left -
							mPaddingPx.Right;

					if ( (int)widget->getPixelsSize().getWidth() != w && w > 0 )
						widget->setPixelsSize( w, widget->getPixelsSize().getHeight() );

					break;
				}
				case SizePolicy::Fixed:
				default: {
				}
			}

			if ( widget->getLayoutHeightPolicy() == SizePolicy::MatchParent &&
				 widget->getLayoutWeight() == 0 &&
				 widget->getPixelsSize().getHeight() != getPixelsSize().getHeight() -
															widget->getLayoutPixelsMargin().Top -
															widget->getLayoutPixelsMargin().Bottom -
															mPaddingPx.Top - mPaddingPx.Bottom ) {
				widget->setPixelsSize( widget->getPixelsSize().getWidth(),
									   getPixelsSize().getHeight() -
										   widget->getLayoutPixelsMargin().Top -
										   widget->getLayoutPixelsMargin().Bottom - mPaddingPx.Top -
										   mPaddingPx.Bottom );
			}
		}

		child = child->getNextNode();
	}
}

void UILinearLayout::applyHeightPolicyOnChilds() {
	Node* child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() && child->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );

			if ( widget->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
				widget->setFlags( UI_AUTO_SIZE );
			}

			switch ( widget->getLayoutHeightPolicy() ) {
				case SizePolicy::WrapContent: {
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case SizePolicy::MatchParent: {
					int h = getPixelsSize().getHeight() - widget->getLayoutPixelsMargin().Top -
							widget->getLayoutPixelsMargin().Bottom - mPaddingPx.Top -
							mPaddingPx.Bottom;

					if ( h != (int)widget->getPixelsSize().getHeight() && h > 0 )
						widget->setPixelsSize( widget->getPixelsSize().getWidth(), h );

					break;
				}
				case SizePolicy::Fixed:
				default: {
				}
			}

			if ( widget->getLayoutWidthPolicy() == SizePolicy::MatchParent &&
				 widget->getLayoutWeight() == 0 &&
				 widget->getPixelsSize().getWidth() != getPixelsSize().getWidth() -
														   widget->getLayoutPixelsMargin().Left -
														   widget->getLayoutPixelsMargin().Right -
														   mPaddingPx.Left - mPaddingPx.Right ) {
				widget->setPixelsSize(
					getPixelsSize().getWidth() - widget->getLayoutPixelsMargin().Left -
						widget->getLayoutPixelsMargin().Right - mPaddingPx.Left - mPaddingPx.Right,
					widget->getPixelsSize().getHeight() );
			}
		}

		child = child->getNextNode();
	}
}

void UILinearLayout::packVertical() {
	if ( mPacking )
		return;
	mPacking = true;
	bool sizeChanged = false;
	Sizef size( getPixelsSize() );

	if ( getLayoutWidthPolicy() == SizePolicy::MatchParent && 0 == getLayoutWeight() ) {
		Float w = getMatchParentWidth();

		if ( (int)w != (int)getPixelsSize().getWidth() ) {
			sizeChanged = true;
			size.setWidth( w );
		}
	}

	if ( getLayoutHeightPolicy() == SizePolicy::MatchParent ) {
		Float h = getMatchParentHeight();

		if ( (int)h != (int)getPixelsSize().getHeight() ) {
			sizeChanged = true;
			size.setHeight( h );
		}
	}

	if ( sizeChanged )
		setInternalPixelsSize( size );

	applyWidthPolicyOnChilds();

	Float curY = mPaddingPx.Top;
	Float maxX = 0;
	Sizei freeSize = getTotalUsedSize();

	Node* child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() && child->isVisible() ) {
			UIWidget* widget = child->asType<UIWidget>();
			Rectf margin = widget->getLayoutPixelsMargin();

			curY += eeceil( margin.Top );

			Vector2f pos( mPaddingPx.Left, curY );

			if ( widget->getLayoutWeight() != 0 ) {
				Int32 totSize =
					( getLayoutHeightPolicy() == SizePolicy::MatchParent )
						? getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom
						: getParent()->getPixelsSize().getHeight() - mLayoutMarginPx.Bottom -
							  mLayoutMarginPx.Top - mPaddingPx.Top - mPaddingPx.Bottom;
				Float newSize =
					eeceil( totSize - freeSize.getHeight() ) * widget->getLayoutWeight();

				widget->setPixelsSize( widget->getPixelsSize().getWidth(), newSize );
			}

			switch ( Font::getHorizontalAlign( widget->getLayoutGravity() ) ) {
				case UI_HALIGN_CENTER:
					pos.x = ( getPixelsSize().getWidth() - mPaddingPx.Left - mPaddingPx.Right -
							  widget->getPixelsSize().getWidth() ) /
							2;
					break;
				case UI_HALIGN_RIGHT:
					pos.x = getPixelsSize().getWidth() - mPaddingPx.Left - mPaddingPx.Right -
							widget->getPixelsSize().getWidth() -
							widget->getLayoutPixelsMargin().Right;
					break;
				case UI_HALIGN_LEFT:
				default:
					pos.x = widget->getLayoutPixelsMargin().Left + mPaddingPx.Left;
					break;
			}

			widget->setPixelsPosition( pos );

			curY += eeceil( widget->getPixelsSize().getHeight() + margin.Bottom );

			if ( widget->getLayoutWidthPolicy() != SizePolicy::MatchParent ) {
				maxX = eeceil( eemax( maxX, ( widget->getPixelsSize().getWidth() +
											  widget->getLayoutPixelsMargin().Left +
											  widget->getLayoutPixelsMargin().Right ) ) );
			}
		}

		child = child->getNextNode();
	}

	maxX += mPaddingPx.Left + mPaddingPx.Right;

	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		curY += mPaddingPx.Bottom;

		if ( curY != (int)getPixelsSize().getHeight() ) {
			setInternalPixelsHeight( curY );
			notifyLayoutAttrChangeParent();
		}
	} else if ( getLayoutHeightPolicy() == SizePolicy::MatchParent ) {
		int h =
			getParent()->getPixelsSize().getHeight() - mLayoutMarginPx.Top - mLayoutMarginPx.Bottom;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			h = h - pLay->getPixelsPadding().Top - pLay->getPixelsPadding().Bottom;
		}

		if ( h != (int)getPixelsSize().getHeight() )
			setInternalPixelsHeight( h );
	}

	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent && getPixelsSize().getWidth() != maxX ) {
		if ( !( 0 != getLayoutWeight() && getParent()->isType( UI_TYPE_LINEAR_LAYOUT ) &&
				getParent()->asType<UILinearLayout>()->getOrientation() ==
					UIOrientation::Horizontal ) ) {
			if ( mMinWidthEq.empty() || PixelDensity::dpToPx( mMinSize.getWidth() ) < maxX ) {
				setInternalPixelsWidth( maxX );
				mPacking = false;
				packVertical();
				notifyLayoutAttrChangeParent();
			}
		}
	}

	if ( getParent()->isUINode() &&
		 ( !getParent()->asType<UINode>()->ownsChildPosition() || isGravityOwner() ) ) {
		alignAgainstLayout();
	}
	mPacking = false;
}

void UILinearLayout::packHorizontal() {
	if ( mPacking )
		return;
	mPacking = true;
	bool sizeChanged = false;
	Sizef size( getPixelsSize() );

	if ( getLayoutWidthPolicy() == SizePolicy::MatchParent ) {
		Float w = getMatchParentWidth();

		if ( (int)w != (int)getPixelsSize().getWidth() ) {
			sizeChanged = true;
			size.setWidth( w );
		}
	}

	if ( getLayoutHeightPolicy() == SizePolicy::MatchParent && 0 == getLayoutWeight() ) {
		Float h = getMatchParentHeight();

		if ( (int)h != (int)getPixelsSize().getHeight() ) {
			sizeChanged = true;
			size.setHeight( h );
		}
	}

	if ( sizeChanged )
		setInternalPixelsSize( size );

	applyHeightPolicyOnChilds();

	Float curX = mPaddingPx.Left;
	Float maxY = 0;
	Sizei freeSize = getTotalUsedSize();

	Node* child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() && child->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );
			Rectf margin = widget->getLayoutPixelsMargin();

			curX += eeceil( margin.Left );

			Vector2f pos( curX, mPaddingPx.Top );

			if ( widget->getLayoutWeight() != 0 ) {
				Int32 totSize =
					( getLayoutWidthPolicy() == SizePolicy::MatchParent )
						? getPixelsSize().getWidth() - mPaddingPx.Left - mPaddingPx.Right
						: getParent()->getPixelsSize().getWidth() - mLayoutMarginPx.Right -
							  mLayoutMarginPx.Left - mPaddingPx.Left - mPaddingPx.Right;
				Float newSize = eeceil( totSize - freeSize.getWidth() ) * widget->getLayoutWeight();

				widget->setPixelsSize( newSize, widget->getPixelsSize().getHeight() );
			}

			switch ( Font::getVerticalAlign( widget->getLayoutGravity() ) ) {
				case UI_VALIGN_CENTER:
					pos.y = ( getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom -
							  widget->getPixelsSize().getHeight() ) /
							2;
					break;
				case UI_VALIGN_BOTTOM:
					pos.y = getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom -
							widget->getPixelsSize().getHeight() -
							widget->getLayoutPixelsMargin().Bottom;
					break;
				case UI_VALIGN_TOP:
				default:
					pos.y = widget->getLayoutPixelsMargin().Top + mPaddingPx.Top;
					break;
			}

			widget->setPixelsPosition( pos );

			curX += eeceil( widget->getPixelsSize().getWidth() + margin.Right );

			if ( widget->getLayoutHeightPolicy() != SizePolicy::MatchParent ) {
				maxY = eeceil( eemax( maxY, ( widget->getPixelsSize().getHeight() +
											  widget->getLayoutPixelsMargin().Top +
											  widget->getLayoutPixelsMargin().Bottom ) ) );
			}
		}

		child = child->getNextNode();
	}

	maxY += mPaddingPx.Top + mPaddingPx.Bottom;

	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
		curX += mPaddingPx.Right;

		if ( curX != (int)getPixelsSize().getWidth() ) {
			setInternalPixelsWidth( curX );
			notifyLayoutAttrChangeParent();
		}
	} else if ( getLayoutWidthPolicy() == SizePolicy::MatchParent ) {
		int w = getMatchParentWidth();
		if ( w != (int)getPixelsSize().getWidth() )
			setInternalPixelsWidth( w );
	}

	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent &&
		 getPixelsSize().getHeight() != maxY ) {
		if ( !( 0 != getLayoutWeight() && getParent()->isType( UI_TYPE_LINEAR_LAYOUT ) &&
				getParent()->asType<UILinearLayout>()->getOrientation() ==
					UIOrientation::Vertical ) ) {
			if ( mMinHeightEq.empty() || PixelDensity::dpToPx( mMinSize.getHeight() ) < maxY ) {
				setInternalPixelsHeight( maxY );
				mPacking = false;
				packHorizontal();
				notifyLayoutAttrChangeParent();
			}
		}
	}

	if ( getParent()->isUINode() &&
		 ( !getParent()->asType<UINode>()->ownsChildPosition() || isGravityOwner() ) ) {
		alignAgainstLayout();
	}

	mPacking = false;
}

Sizei UILinearLayout::getTotalUsedSize() {
	Node* child = mChild;
	Sizei size( 0, 0 );

	while ( NULL != child ) {
		if ( child->isWidget() && child->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );
			Rectf margin = widget->getLayoutPixelsMargin();

			size.x += margin.Left + margin.Right;

			size.y += margin.Top + margin.Bottom;

			if ( widget->getLayoutWidthPolicy() == SizePolicy::Fixed ||
				 widget->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
				if ( 0 == widget->getLayoutWeight() && mOrientation == UIOrientation::Horizontal ) {
					size.x += widget->getPixelsSize().getWidth();
				}
			}

			if ( widget->getLayoutHeightPolicy() == SizePolicy::Fixed ||
				 widget->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
				if ( 0 == widget->getLayoutWeight() && mOrientation == UIOrientation::Vertical ) {
					size.y += widget->getPixelsSize().getHeight();
				}
			}
		}

		child = child->getNextNode();
	}

	return size;
}

std::string UILinearLayout::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Orientation:
			return getOrientation() == UIOrientation::Horizontal ? "horizontal" : "vertical";
		case PropertyId::GravityOwner:
			return isGravityOwner() ? "true" : "false";
		default:
			return UILayout::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UILinearLayout::getPropertiesImplemented() const {
	auto props = UILayout::getPropertiesImplemented();
	auto local = { PropertyId::Orientation, PropertyId::GravityOwner };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UILinearLayout::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Orientation: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "horizontal" == val )
				setOrientation( UIOrientation::Horizontal );
			else if ( "vertical" == val )
				setOrientation( UIOrientation::Vertical );
			break;
		}
		case PropertyId::GravityOwner: {
			setGravityOwner( attribute.asBool() );
			break;
		}
		default:
			return UILayout::applyProperty( attribute );
	}

	return true;
}

Uint32 UILinearLayout::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

}} // namespace EE::UI
