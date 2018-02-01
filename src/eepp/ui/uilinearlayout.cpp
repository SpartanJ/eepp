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
	UILayout(),
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

void UILinearLayout::onParentSizeChange( const Vector2f& SizeChange ) {
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
					int w = mDpSize.getWidth() - widget->getLayoutMargin().Left - widget->getLayoutMargin().Right;

					if ( widget->getSize().getWidth() != w )
						widget->setSize( w, widget->getSize().getHeight() );

					break;
				}
				case FIXED:
				default:
				{
				}
			}

			if ( widget->getLayoutHeightRules() == MATCH_PARENT && widget->getLayoutWeight() == 0 && widget->getSize().getHeight() != mDpSize.getHeight() ) {
				widget->setSize( widget->getSize().getWidth(), mDpSize.getHeight() );
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	Int32 curY = 0;
	Int32 maxX = 0;
	Sizei freeSize = getTotalUsedSize();

	ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );
			Rect margin = widget->getLayoutMargin();

			curY += margin.Top;

			Vector2f pos( 0, curY );

			if ( widget->getLayoutWeight() != 0 ) {
				Int32 totSize = ( getLayoutHeightRules() == MATCH_PARENT ) ? mDpSize.getHeight() : getParent()->getSize().getHeight() - mLayoutMargin.Bottom - mLayoutMargin.Top;
				Float size = (Float)( totSize - freeSize.getHeight() ) * widget->getLayoutWeight();

				widget->setSize( widget->getSize().getWidth(), (Int32)size );
			}

			switch ( fontHAlignGet( widget->getLayoutGravity() ) ) {
				case UI_HALIGN_CENTER:
					pos.x = ( mDpSize.getWidth() - widget->getSize().getWidth() ) / 2;
					break;
				case UI_HALIGN_RIGHT:
					pos.x = mDpSize.getWidth() - widget->getSize().getWidth() - widget->getLayoutMargin().Right;
					break;
				case UI_HALIGN_LEFT:
				default:
					pos.x = widget->getLayoutMargin().Left;
					break;
			}

			widget->setPosition( pos );

			curY += widget->getSize().getHeight() + margin.Bottom;

			maxX = eemax( maxX, (Int32)( widget->getSize().getWidth() + widget->getLayoutMargin().Left + widget->getLayoutMargin().Right ) );
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutHeightRules() == WRAP_CONTENT ) {
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
					int h = mDpSize.getHeight() - widget->getLayoutMargin().Top - widget->getLayoutMargin().Bottom;

					if ( h != widget->getSize().getHeight() )
						widget->setSize( widget->getSize().getWidth(), h );

					break;
				}
				case FIXED:
				default:
				{
				}
			}

			if ( widget->getLayoutWidthRules() == MATCH_PARENT && widget->getLayoutWeight() == 0 && widget->getSize().getWidth() != mDpSize.getWidth() ) {
				widget->setSize( mDpSize.getWidth(), widget->getSize().getWidth() );
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	Int32 curX = 0;
	Int32 maxY = 0;
	Sizei freeSize = getTotalUsedSize();

	ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isWidget() && ChildLoop->isVisible() ) {
			UIWidget * widget = static_cast<UIWidget*>( ChildLoop );
			Rect margin = widget->getLayoutMargin();

			curX += margin.Left;

			Vector2f pos( curX, 0 );

			if ( widget->getLayoutWeight() != 0 ) {
				Int32 totSize = ( getLayoutWidthRules() == MATCH_PARENT ) ? mDpSize.getWidth() : getParent()->getSize().getWidth() - mLayoutMargin.Right - mLayoutMargin.Left;
				Float size = (Float)( totSize - freeSize.getWidth() ) * widget->getLayoutWeight();

				widget->setSize( (Int32)size, widget->getSize().getHeight() );
			}

			switch ( fontVAlignGet( widget->getLayoutGravity() ) ) {
				case UI_VALIGN_CENTER:
					pos.y = ( mDpSize.getHeight() - widget->getSize().getHeight() ) / 2;
					break;
				case UI_VALIGN_BOTTOM:
					pos.y = mDpSize.getHeight() - widget->getSize().getHeight() - widget->getLayoutMargin().Bottom;
					break;
				case UI_VALIGN_TOP:
				default:
					pos.y = widget->getLayoutMargin().Top;
					break;
			}

			widget->setPosition( pos );

			curX += widget->getSize().getWidth() + margin.Right;

			maxY = eemax( maxY, (Int32)( widget->getSize().getHeight() + widget->getLayoutMargin().Top + widget->getLayoutMargin().Bottom ) );
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( getLayoutWidthRules() == WRAP_CONTENT ) {
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

void UILinearLayout::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "orientation" == name ) {
			std::string val = ait->as_string();
			String::toLowerInPlace( val );

			if ( "horizontal" == val )
				setOrientation( UI_HORIZONTAL );
			else if ( "vertical" == val )
				setOrientation( UI_VERTICAL );
		}
	}

	endPropertiesTransaction();
}

Uint32 UILinearLayout::onMessage(const UIMessage * Msg) {
	switch( Msg->getMsg() ) {
		case UIMessage::LayoutAttributeChange:
		{
			pack();
			break;
		}
	}

	return 0;
}

}}
