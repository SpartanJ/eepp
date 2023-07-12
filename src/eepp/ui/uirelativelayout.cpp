#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

UIRelativeLayout* UIRelativeLayout::New() {
	return eeNew( UIRelativeLayout, () );
}

UIRelativeLayout::UIRelativeLayout() : UILayout( "relativelayout" ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
}

UIRelativeLayout::UIRelativeLayout( const std::string& tagName ) : UILayout( tagName ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
}

Uint32 UIRelativeLayout::getType() const {
	return UI_TYPE_RELATIVE_LAYOUT;
}

bool UIRelativeLayout::isType( const Uint32& type ) const {
	return UIRelativeLayout::getType() == type ? true : UILayout::isType( type );
}

UIRelativeLayout* UIRelativeLayout::add( UIWidget* widget ) {
	widget->setParent( this );
	return this;
}

void UIRelativeLayout::updateLayout() {
	if ( mPacking )
		return;
	mPacking = true;

	if ( !mVisible ) {
		setInternalPixelsSize( Sizef::Zero );
		notifyLayoutAttrChangeParent();
	} else {

		if ( getParent()->isUINode() &&
			 ( !getParent()->asType<UINode>()->ownsChildPosition() || isGravityOwner() ) ) {
			setInternalPosition( Vector2f( mLayoutMargin.Left, mLayoutMargin.Top ) );
		}

		Sizef s( getSizeFromLayoutPolicy() );

		if ( s != getPixelsSize() )
			setInternalPixelsSize( s );

		Node* child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				UIWidget* widget = static_cast<UIWidget*>( child );

				fixChildSize( widget );

				fixChildPos( widget );
			}

			child = child->getNextNode();
		}
	}

	mDirtyLayout = false;
	mPacking = false;
}

void UIRelativeLayout::fixChildPos( UIWidget* widget ) {
	Vector2f pos( widget->getPosition() );

	if ( widget->getLayoutPositionPolicy() != PositionPolicy::None &&
		 widget->getParent() == widget->getLayoutPositionPolicyWidget()->getParent() ) {
		UIWidget* of = widget->getLayoutPositionPolicyWidget();

		switch ( widget->getLayoutPositionPolicy() ) {
			case PositionPolicy::LeftOf:
				pos.x = of->getPosition().x - widget->getSize().getWidth() -
						widget->getLayoutMargin().Right - of->getLayoutMargin().Left;
				pos.y = of->getPosition().y + widget->getLayoutMargin().Top;
				break;
			case PositionPolicy::RightOf:
				pos.x = of->getPosition().x + of->getSize().getWidth() +
						widget->getLayoutMargin().Left + of->getLayoutMargin().Right;
				pos.y = of->getPosition().y + widget->getLayoutMargin().Top;
				break;
			case PositionPolicy::TopOf:
				pos.x = of->getPosition().x + widget->getLayoutMargin().Left;
				pos.y = of->getPosition().y - widget->getSize().getHeight() -
						widget->getLayoutMargin().Bottom - of->getLayoutMargin().Top;
				break;
			case PositionPolicy::BottomOf:
				pos.x = of->getPosition().x + widget->getLayoutMargin().Left;
				pos.y = of->getPosition().y + of->getSize().getHeight() +
						widget->getLayoutMargin().Top + of->getLayoutMargin().Bottom;
				break;
			default:
				break;
		}
	} else {
		switch ( Font::getHorizontalAlign( widget->getLayoutGravity() ) ) {
			case UI_HALIGN_CENTER:
				pos.x = ( getSize().getWidth() - widget->getSize().getWidth() ) / 2 +
						widget->getLayoutMargin().Left;
				break;
			case UI_HALIGN_RIGHT:
				pos.x = getSize().getWidth() - widget->getSize().getWidth() -
						widget->getLayoutMargin().Right - mPadding.Right;
				break;
			case UI_HALIGN_LEFT:
			default:
				pos.x = widget->getLayoutMargin().Left + mPadding.Left;
				break;
		}

		switch ( Font::getVerticalAlign( widget->getLayoutGravity() ) ) {
			case UI_VALIGN_CENTER:
				pos.y = ( getSize().getHeight() - widget->getSize().getHeight() ) / 2 +
						widget->getLayoutMargin().Top;
				break;
			case UI_VALIGN_BOTTOM:
				pos.y = getSize().getHeight() - widget->getSize().getHeight() -
						widget->getLayoutMargin().Bottom - mPadding.Bottom;
				break;
			case UI_VALIGN_TOP:
			default:
				pos.y = widget->getLayoutMargin().Top + mPadding.Top;
				break;
		}
	}

	widget->setPosition( pos );
}

void UIRelativeLayout::fixChildSize( UIWidget* widget ) {
	switch ( widget->getLayoutWidthPolicy() ) {
		case SizePolicy::WrapContent: {
			widget->setFlags( UI_AUTO_SIZE );
			break;
		}
		case SizePolicy::MatchParent: {
			widget->setSize( getSize().getWidth() - widget->getLayoutMargin().Left -
								 widget->getLayoutMargin().Right - mPadding.Left - mPadding.Right,
							 widget->getSize().getHeight() );
			break;
		}
		case SizePolicy::Fixed:
		default: {
		}
	}

	switch ( widget->getLayoutHeightPolicy() ) {
		case SizePolicy::WrapContent: {
			widget->setFlags( UI_AUTO_SIZE );
			break;
		}
		case SizePolicy::MatchParent: {
			widget->setSize( widget->getSize().getWidth(), getSize().getHeight() -
															   widget->getLayoutMargin().Top -
															   widget->getLayoutMargin().Bottom -
															   mPadding.Top - mPadding.Bottom );
			break;
		}
		case SizePolicy::Fixed:
		default: {
		}
	}
}

Uint32 UIRelativeLayout::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

}} // namespace EE::UI
