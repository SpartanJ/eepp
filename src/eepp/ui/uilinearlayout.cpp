#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscenenode.hpp>

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
	UILayout( "linearlayout" ), mOrientation( UIOrientation::Vertical ), mPacking( false ) {
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

void UILinearLayout::updateLayout() {
	if ( mOrientation == UIOrientation::Vertical )
		packVertical();
	else
		packHorizontal();
	mDirtyLayout = false;
}

bool UILinearLayout::isPacking() const {
	return mPacking;
}

void UILinearLayout::packVertical() {
	if ( mPacking )
		return;
	mPacking = true;
	bool sizeChanged = false;
	Sizef size( getSize() );

	if ( getLayoutWidthPolicy() == SizePolicy::MatchParent && 0 == getLayoutWeight() ) {
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

	if ( getLayoutHeightPolicy() == SizePolicy::MatchParent ) {
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

			if ( widget->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
				widget->setFlags( UI_AUTO_SIZE );
			}

			switch ( widget->getLayoutWidthPolicy() ) {
				case SizePolicy::WrapContent: {
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case SizePolicy::MatchParent: {
					int w = getSize().getWidth() - widget->getLayoutMargin().Left -
							widget->getLayoutMargin().Right - mPadding.Left - mPadding.Right;

					if ( (int)widget->getSize().getWidth() != w && w > 0 )
						widget->setSize( w, widget->getSize().getHeight() );

					break;
				}
				case SizePolicy::Fixed:
				default: {
				}
			}

			if ( widget->getLayoutHeightPolicy() == SizePolicy::MatchParent &&
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
				Int32 totSize = ( getLayoutHeightPolicy() == SizePolicy::MatchParent )
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

	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		curY += mPadding.Bottom;

		if ( curY != (int)getSize().getHeight() ) {
			setInternalHeight( curY );
			notifyLayoutAttrChangeParent();
		}
	} else if ( getLayoutHeightPolicy() == SizePolicy::MatchParent ) {
		int h = getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			h = h - pLay->getPadding().Top - pLay->getPadding().Bottom;
		}

		if ( h != (int)getSize().getHeight() )
			setInternalHeight( h );
	}

	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent && getSize().getWidth() != maxX ) {
		if ( !( 0 != getLayoutWeight() && getParent()->isType( UI_TYPE_LINEAR_LAYOUT ) &&
				getParent()->asType<UILinearLayout>()->getOrientation() ==
					UIOrientation::Horizontal ) ) {
			setInternalWidth( maxX );
			mPacking = false;
			packVertical();
			notifyLayoutAttrChangeParent();
		}
	}

	if ( getParent()->isUINode() && !getParent()->asType<UINode>()->ownsChildPosition() ) {
		alignAgainstLayout();
	}
	mPacking = false;
}

void UILinearLayout::packHorizontal() {
	if ( mPacking )
		return;
	mPacking = true;
	bool sizeChanged = false;
	Sizef size( getSize() );

	if ( getLayoutWidthPolicy() == SizePolicy::MatchParent ) {
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

	if ( getLayoutHeightPolicy() == SizePolicy::MatchParent && 0 == getLayoutWeight() ) {
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

			if ( widget->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
				widget->setFlags( UI_AUTO_SIZE );
			}

			switch ( widget->getLayoutHeightPolicy() ) {
				case SizePolicy::WrapContent: {
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case SizePolicy::MatchParent: {
					int h = getSize().getHeight() - widget->getLayoutMargin().Top -
							widget->getLayoutMargin().Bottom - mPadding.Top - mPadding.Bottom;

					if ( h != (int)widget->getSize().getHeight() && h > 0 )
						widget->setSize( widget->getSize().getWidth(), h );

					break;
				}
				case SizePolicy::Fixed:
				default: {
				}
			}

			if ( widget->getLayoutWidthPolicy() == SizePolicy::MatchParent &&
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
				Int32 totSize = ( getLayoutWidthPolicy() == SizePolicy::MatchParent )
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

	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
		curX += mPadding.Right;

		if ( curX != (int)getSize().getWidth() ) {
			setInternalWidth( curX );
			notifyLayoutAttrChangeParent();
		}
	} else if ( getLayoutWidthPolicy() == SizePolicy::MatchParent ) {
		int w = getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right;

		if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
			UILayout* pLay = static_cast<UILayout*>( getParent() );
			w = w - pLay->getPadding().Left - pLay->getPadding().Right;
		}

		if ( w != (int)getSize().getWidth() )
			setInternalWidth( w );
	}

	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent && getSize().getHeight() != maxY ) {
		if ( !( 0 != getLayoutWeight() && getParent()->isType( UI_TYPE_LINEAR_LAYOUT ) &&
				getParent()->asType<UILinearLayout>()->getOrientation() ==
					UIOrientation::Vertical ) ) {
			setInternalHeight( maxY );
			mPacking = false;
			packHorizontal();
			notifyLayoutAttrChangeParent();
		}
	}

	if ( getParent()->isUINode() && !getParent()->asType<UINode>()->ownsChildPosition() ) {
		alignAgainstLayout();
	}
	mPacking = false;
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

			if ( widget->getLayoutWidthPolicy() == SizePolicy::Fixed ||
				 widget->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
				if ( 0 == widget->getLayoutWeight() && mOrientation == UIOrientation::Horizontal ) {
					size.x += widget->getSize().getWidth();
				}
			}

			if ( widget->getLayoutHeightPolicy() == SizePolicy::Fixed ||
				 widget->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
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
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

}} // namespace EE::UI
