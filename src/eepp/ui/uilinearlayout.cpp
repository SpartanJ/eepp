#include <eepp/ui/uilinearlayout.hpp>
#include <pugixml/pugixml.hpp>

namespace  EE { namespace UI {

UILinearLayout * UILinearLayout::New() {
	return eeNew( UILinearLayout, () );
}

UILinearLayout * UILinearLayout::NewVertical() {
	return eeNew( UILinearLayout, () );
}

UILinearLayout * UILinearLayout::NewHorizontal() {
	return ( eeNew( UILinearLayout, () ) )->setOrientation( UI_HORIZONTAL );
}

UILinearLayout::UILinearLayout() :
	UILayout( "linearlayout" ),
	mOrientation( UI_VERTICAL )
{
	clipEnable();
}

Uint32 UILinearLayout::getType() const {
	return UI_TYPE_LINEAR_LAYOUT;
}

bool UILinearLayout::isType( const Uint32& type ) const {
	return UIWidget::getType() == type ? true : UIWidget::isType( type );
}

UI_ORIENTATION UILinearLayout::getOrientation() const {
	return mOrientation;
}

UILinearLayout * UILinearLayout::setOrientation(const UI_ORIENTATION & orientation) {
	mOrientation = orientation;
	return this;
}

UILinearLayout * UILinearLayout::add( UIWidget * widget ) {
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

void UILinearLayout::onChildCountChange() {
	UILayout::onChildCountChange();
	pack();
}

void UILinearLayout::pack() {
	setInternalPosition( Vector2f( mLayoutMargin.Left, mLayoutMargin.Top ) );

	if ( mOrientation == UI_VERTICAL )
		packVertical();
	else
		packHorizontal();
}

void UILinearLayout::packVertical() {
	if ( getLayoutWidthRules() == MATCH_PARENT && 0 == mLayoutWeight ) {
		setInternalWidth( getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right );
	}

	if ( getLayoutHeightRules() == MATCH_PARENT ) {
		setInternalHeight( getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom );
	}

	Node * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );

			if ( widget->getLayoutHeightRules() == WRAP_CONTENT ) {
				widget->setFlags( UI_AUTO_SIZE );
			}

			switch ( widget->getLayoutWidthRules() ) {
				case WRAP_CONTENT:
				{
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case MATCH_PARENT:
				{
					int w = mDpSize.getWidth() - widget->getLayoutMargin().Left - widget->getLayoutMargin().Right - mPadding.Left - mPadding.Right;

					if ( widget->getSize().getWidth() != w )
						widget->setSize( w, widget->getSize().getHeight() );

					break;
				}
				case FIXED:
				default:
				{
				}
			}

			if ( widget->getLayoutHeightRules() == MATCH_PARENT && widget->getLayoutWeight() == 0 &&
				 widget->getSize().getHeight() != mDpSize.getHeight() - widget->getLayoutMargin().Top - widget->getLayoutMargin().Bottom - mPadding.Top - mPadding.Bottom ) {
				widget->setSize( widget->getSize().getWidth(), mDpSize.getHeight() - widget->getLayoutMargin().Top - widget->getLayoutMargin().Bottom - mPadding.Top - mPadding.Bottom );
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	Int32 curY = mPadding.Top;
	Int32 maxX = 0;
	Sizei freeSize = getTotalUsedSize();

	ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );
			Rect margin = widget->getLayoutMargin();

			curY += margin.Top;

			Vector2f pos( mPadding.Left, curY );

			if ( widget->getLayoutWeight() != 0 ) {
				Int32 totSize = ( getLayoutHeightRules() == MATCH_PARENT ) ? mDpSize.getHeight() - mPadding.Top - mPadding.Bottom :
																			 getParent()->getSize().getHeight() - mLayoutMargin.Bottom - mLayoutMargin.Top - mPadding.Top - mPadding.Bottom;
				Float size = (Float)( totSize - freeSize.getHeight() ) * widget->getLayoutWeight();

				widget->setSize( widget->getSize().getWidth(), (Int32)size );
			}

			switch ( fontHAlignGet( widget->getLayoutGravity() ) ) {
				case UI_HALIGN_CENTER:
					pos.x = ( mDpSize.getWidth() - mPadding.Left - mPadding.Right - widget->getSize().getWidth() ) / 2;
					break;
				case UI_HALIGN_RIGHT:
					pos.x = mDpSize.getWidth() - mPadding.Left - mPadding.Right - widget->getSize().getWidth() - widget->getLayoutMargin().Right;
					break;
				case UI_HALIGN_LEFT:
				default:
					pos.x = widget->getLayoutMargin().Left + mPadding.Left;
					break;
			}

			widget->setPosition( pos );

			curY += widget->getSize().getHeight() + margin.Bottom;

			maxX = eemax( maxX, (Int32)( widget->getSize().getWidth() + widget->getLayoutMargin().Left + widget->getLayoutMargin().Right ) );
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutHeightRules() == WRAP_CONTENT ) {
		curY += mPadding.Bottom;

		if ( curY != mDpSize.getHeight() ) {
			setInternalHeight( curY );
			notifyLayoutAttrChangeParent();
		}
	} else if ( getLayoutHeightRules() == MATCH_PARENT ) {
		int h = getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom;

		if ( h != mDpSize.getHeight() )
			setInternalHeight( h );
	}

	if ( getLayoutWidthRules() == WRAP_CONTENT && mDpSize.getWidth() != maxX ) {
		setInternalWidth( maxX );
		packVertical();
		notifyLayoutAttrChangeParent();
	}

	alignAgainstLayout();
}

void UILinearLayout::packHorizontal() {
	if ( getLayoutWidthRules() == MATCH_PARENT ) {
		setInternalWidth( getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right );
	}

	if ( getLayoutHeightRules() == MATCH_PARENT && 0 == mLayoutWeight ) {
		setInternalHeight( getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom );
	}

	Node * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );

			if ( widget->getLayoutWidthRules() == WRAP_CONTENT ) {
				widget->setFlags( UI_AUTO_SIZE );
			}

			switch ( widget->getLayoutHeightRules() ) {
				case WRAP_CONTENT:
				{
					widget->setFlags( UI_AUTO_SIZE );
					break;
				}
				case MATCH_PARENT:
				{
					int h = mDpSize.getHeight() - widget->getLayoutMargin().Top - widget->getLayoutMargin().Bottom - mPadding.Top - mPadding.Bottom;

					if ( h != widget->getSize().getHeight() )
						widget->setSize( widget->getSize().getWidth(), h );

					break;
				}
				case FIXED:
				default:
				{
				}
			}

			if ( widget->getLayoutWidthRules() == MATCH_PARENT && widget->getLayoutWeight() == 0 &&
				 widget->getSize().getWidth() != mDpSize.getWidth() - widget->getLayoutMargin().Left  - widget->getLayoutMargin().Top - mPadding.Left - mPadding.Right ) {
				widget->setSize( mDpSize.getWidth(), widget->getSize().getWidth() - widget->getLayoutMargin().Left  - widget->getLayoutMargin().Top - mPadding.Left - mPadding.Right );
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	Int32 curX = mPadding.Left;
	Int32 maxY = 0;
	Sizei freeSize = getTotalUsedSize();

	ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );
			Rect margin = widget->getLayoutMargin();

			curX += margin.Left;

			Vector2f pos( curX, mPadding.Top );

			if ( widget->getLayoutWeight() != 0 ) {
				Int32 totSize = ( getLayoutWidthRules() == MATCH_PARENT ) ? mDpSize.getWidth() - mPadding.Left - mPadding.Right :
																			getParent()->getSize().getWidth() - mLayoutMargin.Right - mLayoutMargin.Left - mPadding.Left - mPadding.Right;
				Float size = (Float)( totSize - freeSize.getWidth() ) * widget->getLayoutWeight();

				widget->setSize( (Int32)size, widget->getSize().getHeight() );
			}

			switch ( fontVAlignGet( widget->getLayoutGravity() ) ) {
				case UI_VALIGN_CENTER:
					pos.y = ( mDpSize.getHeight() - mPadding.Top - mPadding.Bottom - widget->getSize().getHeight() ) / 2;
					break;
				case UI_VALIGN_BOTTOM:
					pos.y = mDpSize.getHeight() - mPadding.Top - mPadding.Bottom - widget->getSize().getHeight() - widget->getLayoutMargin().Bottom;
					break;
				case UI_VALIGN_TOP:
				default:
					pos.y = widget->getLayoutMargin().Top + mPadding.Top;
					break;
			}

			widget->setPosition( pos );

			curX += widget->getSize().getWidth() + margin.Right;

			maxY = eemax( maxY, (Int32)( widget->getSize().getHeight() + widget->getLayoutMargin().Top + widget->getLayoutMargin().Bottom ) );
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutWidthRules() == WRAP_CONTENT ) {
		curX += mPadding.Right;

		if ( curX != mDpSize.getWidth() ) {
			setInternalWidth( curX );
			notifyLayoutAttrChangeParent();
		}
	} else if ( getLayoutWidthRules() == MATCH_PARENT ) {
		int w = getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right;

		if ( w != mDpSize.getWidth() )
			setInternalWidth( w );
	}

	if ( getLayoutHeightRules() == WRAP_CONTENT && mDpSize.getHeight() != maxY ) {
		setInternalHeight( maxY );
		packHorizontal();
		notifyLayoutAttrChangeParent();
	}

	alignAgainstLayout();
}

Sizei UILinearLayout::getTotalUsedSize() {
	Node * ChildLoop = mChild;
	Sizei size( 0, 0 );

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );
			Rect margin = widget->getLayoutMargin();

			size.x += margin.Left + margin.Right;

			size.y += margin.Top + margin.Bottom;

			if ( widget->getLayoutWidthRules() == FIXED || widget->getLayoutWidthRules() == WRAP_CONTENT ) {
				if ( 0 == widget->getLayoutWeight() && mOrientation == UI_HORIZONTAL ) {
					size.x += widget->getSize().getWidth();
				}
			}

			if ( widget->getLayoutHeightRules() == FIXED || widget->getLayoutHeightRules() == WRAP_CONTENT ) {
				if ( 0 == widget->getLayoutWeight() && mOrientation == UI_VERTICAL ) {
					size.y += widget->getSize().getHeight();
				}
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	return size;
}

bool UILinearLayout::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "orientation" == name ) {
		std::string val = attribute.asString();
		String::toLowerInPlace( val );

		if ( "horizontal" == val )
			setOrientation( UI_HORIZONTAL );
		else if ( "vertical" == val )
			setOrientation( UI_VERTICAL );
	} else {
		return UILayout::setAttribute( attribute, state );
	}

	return true;
}

Uint32 UILinearLayout::onMessage(const NodeMessage * Msg) {
	switch( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange:
		{
			pack();
			break;
		}
	}

	return 0;
}

}}
