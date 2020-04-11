#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uilinearlayout.hpp>

namespace EE { namespace UI {

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
	UILayout( "linearlayout" ),
	mOrientation( UIOrientation::Vertical ),
	mHPacking( false ),
	mVPacking( false ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
	clipEnable();
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

UILinearLayout* UILinearLayout::add( UIWidget* widget ) {
	widget->setParent( this );
	return this;
}

void UILinearLayout::onSizeChange() {
	UILayout::onSizeChange();
	pack();
}

void UILinearLayout::onParentSizeChange( const Vector2f& ) {
	UILayout::onParentChange();
	pack();
}

void UILinearLayout::onChildCountChange( Node* child, const bool& removed ) {
	UILayout::onChildCountChange( child, removed );
	pack();
}

void UILinearLayout::pack() {
	if ( mOrientation == UIOrientation::Vertical )
		packVertical();
	else
		packHorizontal();
}

void UILinearLayout::packVertical() {
	if ( mVPacking )
		return;
	mVPacking = true;
	bool sizeChanged = false;
	Sizef size( getSize() );

	if ( getLayoutWidthRule() == LayoutSizeRule::MatchParent && 0 == getLayoutWeight() ) {
		Float w = getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			w = w - pLay->getPadding().Left - pLay->getPadding().Right;
		}

		if ( (int)w != (int)getSize().getWidth() ) {
			sizeChanged = true;

			size.setWidth( w );
		}
	}

	if ( getLayoutHeightRule() == LayoutSizeRule::MatchParent ) {
		Float h = getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			h = h - pLay->getPadding().Top - pLay->getPadding().Bottom;
		}

		if ( (int)h != (int)getSize().getHeight() ) {
			sizeChanged = true;

			size.setHeight( h );
		}
	}

	if ( sizeChanged ) {
		setInternalSize( size );
	}

	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( ChildLoop );

			if ( widget->getLayoutHeightRule() == LayoutSizeRule::WrapContent ) {
				widget->setFlags( UI_AUTO_SIZE );
			}

			switch ( widget->getLayoutWidthRule() ) {
				case LayoutSizeRule::WrapContent: {
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case LayoutSizeRule::MatchParent: {
					int w = getSize().getWidth() - widget->getLayoutMargin().Left -
							widget->getLayoutMargin().Right - mPadding.Left - mPadding.Right;

					if ( (int)widget->getSize().getWidth() != w && w > 0 )
						widget->setSize( w, widget->getSize().getHeight() );

					break;
				}
				case LayoutSizeRule::Fixed:
				default: {
				}
			}

			if ( widget->getLayoutHeightRule() == LayoutSizeRule::MatchParent &&
				 widget->getLayoutWeight() == 0 &&
				 widget->getSize().getHeight() !=
					 getSize().getHeight() - widget->getLayoutMargin().Top -
						 widget->getLayoutMargin().Bottom - mPadding.Top - mPadding.Bottom ) {
				widget->setSize( widget->getSize().getWidth(),
								 getSize().getHeight() - widget->getLayoutMargin().Top -
									 widget->getLayoutMargin().Bottom - mPadding.Top -
									 mPadding.Bottom );
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	Float curY = mPadding.Top;
	Float maxX = 0;
	Sizei freeSize = getTotalUsedSize();

	ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( ChildLoop );
			Rect margin = widget->getLayoutMargin();

			curY += eeceil( margin.Top );

			Vector2f pos( mPadding.Left, curY );

			if ( widget->getLayoutWeight() != 0 ) {
				Int32 totSize = ( getLayoutHeightRule() == LayoutSizeRule::MatchParent )
									? getSize().getHeight() - mPadding.Top - mPadding.Bottom
									: getParent()->getSize().getHeight() - mLayoutMargin.Bottom -
										  mLayoutMargin.Top - mPadding.Top - mPadding.Bottom;
				Float size =
					( Float )( totSize - freeSize.getHeight() ) * widget->getLayoutWeight();

				widget->setSize( widget->getSize().getWidth(), (Int32)size );
			}

			switch ( Font::getHorizontalAlign( widget->getLayoutGravity() ) ) {
				case UI_HALIGN_CENTER:
					pos.x = ( getSize().getWidth() - mPadding.Left - mPadding.Right -
							  widget->getSize().getWidth() ) /
							2;
					break;
				case UI_HALIGN_RIGHT:
					pos.x = getSize().getWidth() - mPadding.Left - mPadding.Right -
							widget->getSize().getWidth() - widget->getLayoutMargin().Right;
					break;
				case UI_HALIGN_LEFT:
				default:
					pos.x = widget->getLayoutMargin().Left + mPadding.Left;
					break;
			}

			widget->setPosition( pos );

			curY += eeceil( widget->getSize().getHeight() + margin.Bottom );

			maxX = eeceil(
				eemax( maxX, ( widget->getSize().getWidth() + widget->getLayoutMargin().Left +
							   widget->getLayoutMargin().Right ) ) );
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutHeightRule() == LayoutSizeRule::WrapContent ) {
		curY += mPadding.Bottom;

		if ( curY != (int)getSize().getHeight() ) {
			setInternalHeight( curY );
			notifyLayoutAttrChangeParent();
		}
	} else if ( getLayoutHeightRule() == LayoutSizeRule::MatchParent ) {
		int h = getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			h = h - pLay->getPadding().Top - pLay->getPadding().Bottom;
		}

		if ( h != (int)getSize().getHeight() )
			setInternalHeight( h );
	}

	if ( getLayoutWidthRule() == LayoutSizeRule::WrapContent && getSize().getWidth() != maxX ) {
		if ( !( 0 != getLayoutWeight() && getParent()->isType( UI_TYPE_LINEAR_LAYOUT ) &&
				getParent()->asType<UILinearLayout>()->getOrientation() ==
					UIOrientation::Horizontal ) ) {
			setInternalWidth( maxX );
			mVPacking = false;
			packVertical();
			notifyLayoutAttrChangeParent();
		}
	}

	if ( getParent()->isUINode() && !getParent()->asType<UINode>()->ownsChildPosition() ) {
		alignAgainstLayout();
	}
	mVPacking = false;
}

void UILinearLayout::packHorizontal() {
	if ( mHPacking )
		return;
	mHPacking = true;
	bool sizeChanged = false;
	Sizef size( getSize() );

	if ( getLayoutWidthRule() == LayoutSizeRule::MatchParent ) {
		Float w = getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			w = w - pLay->getPadding().Left - pLay->getPadding().Right;
		}

		if ( (int)w != (int)getSize().getWidth() ) {
			sizeChanged = true;

			size.setWidth( w );
		}
	}

	if ( getLayoutHeightRule() == LayoutSizeRule::MatchParent && 0 == getLayoutWeight() ) {
		Float h = getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			h = h - pLay->getPadding().Top - pLay->getPadding().Bottom;
		}

		if ( (int)h != (int)getSize().getHeight() ) {
			sizeChanged = true;

			size.setHeight( h );
		}
	}

	if ( sizeChanged ) {
		setInternalSize( size );
	}

	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( ChildLoop );

			if ( widget->getLayoutWidthRule() == LayoutSizeRule::WrapContent ) {
				widget->setFlags( UI_AUTO_SIZE );
			}

			switch ( widget->getLayoutHeightRule() ) {
				case LayoutSizeRule::WrapContent: {
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case LayoutSizeRule::MatchParent: {
					int h = getSize().getHeight() - widget->getLayoutMargin().Top -
							widget->getLayoutMargin().Bottom - mPadding.Top - mPadding.Bottom;

					if ( h != (int)widget->getSize().getHeight() && h > 0 )
						widget->setSize( widget->getSize().getWidth(), h );

					break;
				}
				case LayoutSizeRule::Fixed:
				default: {
				}
			}

			if ( widget->getLayoutWidthRule() == LayoutSizeRule::MatchParent &&
				 widget->getLayoutWeight() == 0 &&
				 widget->getSize().getWidth() !=
					 getSize().getWidth() - widget->getLayoutMargin().Left -
						 widget->getLayoutMargin().Right - mPadding.Left - mPadding.Right ) {
				widget->setSize( getSize().getWidth() - widget->getLayoutMargin().Left -
									 widget->getLayoutMargin().Right - mPadding.Left -
									 mPadding.Right,
								 widget->getSize().getHeight() );
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	Float curX = mPadding.Left;
	Float maxY = 0;
	Sizei freeSize = getTotalUsedSize();

	ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget* widget = static_cast<UIWidget*>( ChildLoop );
			Rect margin = widget->getLayoutMargin();

			curX += eeceil( margin.Left );

			Vector2f pos( curX, mPadding.Top );

			if ( widget->getLayoutWeight() != 0 ) {
				Int32 totSize = ( getLayoutWidthRule() == LayoutSizeRule::MatchParent )
									? getSize().getWidth() - mPadding.Left - mPadding.Right
									: getParent()->getSize().getWidth() - mLayoutMargin.Right -
										  mLayoutMargin.Left - mPadding.Left - mPadding.Right;
				Float size = ( Float )( totSize - freeSize.getWidth() ) * widget->getLayoutWeight();

				widget->setSize( (Int32)size, widget->getSize().getHeight() );
			}

			switch ( Font::getVerticalAlign( widget->getLayoutGravity() ) ) {
				case UI_VALIGN_CENTER:
					pos.y = ( getSize().getHeight() - mPadding.Top - mPadding.Bottom -
							  widget->getSize().getHeight() ) /
							2;
					break;
				case UI_VALIGN_BOTTOM:
					pos.y = getSize().getHeight() - mPadding.Top - mPadding.Bottom -
							widget->getSize().getHeight() - widget->getLayoutMargin().Bottom;
					break;
				case UI_VALIGN_TOP:
				default:
					pos.y = widget->getLayoutMargin().Top + mPadding.Top;
					break;
			}

			widget->setPosition( pos );

			curX += eeceil( widget->getSize().getWidth() + margin.Right );

			maxY = eeceil(
				eemax( maxY, ( widget->getSize().getHeight() + widget->getLayoutMargin().Top +
							   widget->getLayoutMargin().Bottom ) ) );
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutWidthRule() == LayoutSizeRule::WrapContent ) {
		curX += mPadding.Right;

		if ( curX != (int)getSize().getWidth() ) {
			setInternalWidth( curX );
			notifyLayoutAttrChangeParent();
		}
	} else if ( getLayoutWidthRule() == LayoutSizeRule::MatchParent ) {
		int w = getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			w = w - pLay->getPadding().Left - pLay->getPadding().Right;
		}

		if ( w != (int)getSize().getWidth() )
			setInternalWidth( w );
	}

	if ( getLayoutHeightRule() == LayoutSizeRule::WrapContent && getSize().getHeight() != maxY ) {
		if ( !( 0 != getLayoutWeight() && getParent()->isType( UI_TYPE_LINEAR_LAYOUT ) &&
				getParent()->asType<UILinearLayout>()->getOrientation() ==
					UIOrientation::Vertical ) ) {
			setInternalHeight( maxY );
			mHPacking = false;
			packHorizontal();
			notifyLayoutAttrChangeParent();
		}
	}

	if ( getParent()->isUINode() && !getParent()->asType<UINode>()->ownsChildPosition() ) {
		alignAgainstLayout();
	}
	mHPacking = false;
}

Sizei UILinearLayout::getTotalUsedSize() {
	Node* ChildLoop = mChild;
	Sizei size( 0, 0 );

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() ) {
			UIWidget* widget = static_cast<UIWidget*>( ChildLoop );
			Rect margin = widget->getLayoutMargin();

			size.x += margin.Left + margin.Right;

			size.y += margin.Top + margin.Bottom;

			if ( widget->getLayoutWidthRule() == LayoutSizeRule::Fixed ||
				 widget->getLayoutWidthRule() == LayoutSizeRule::WrapContent ) {
				if ( 0 == widget->getLayoutWeight() && mOrientation == UIOrientation::Horizontal ) {
					size.x += widget->getSize().getWidth();
				}
			}

			if ( widget->getLayoutHeightRule() == LayoutSizeRule::Fixed ||
				 widget->getLayoutHeightRule() == LayoutSizeRule::WrapContent ) {
				if ( 0 == widget->getLayoutWeight() && mOrientation == UIOrientation::Vertical ) {
					size.y += widget->getSize().getHeight();
				}
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	return size;
}

std::string UILinearLayout::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Orientation:
			return getOrientation() == UIOrientation::Horizontal ? "horizontal" : "vertical";
		default:
			return UILayout::getPropertyString( propertyDef, propertyIndex );
	}
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
		default:
			return UILayout::applyProperty( attribute );
	}

	return true;
}

Uint32 UILinearLayout::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			pack();
			break;
		}
	}

	return 0;
}

}} // namespace EE::UI
