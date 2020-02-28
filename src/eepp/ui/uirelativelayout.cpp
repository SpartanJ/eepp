#include <eepp/ui/uirelativelayout.hpp>

namespace EE { namespace UI {

UIRelativeLayout* UIRelativeLayout::New() {
	return eeNew( UIRelativeLayout, () );
}

UIRelativeLayout::UIRelativeLayout() : UILayout( "relativelayout" ) {}

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

void UIRelativeLayout::onSizeChange() {
	UILayout::onSizeChange();
	fixChilds();
}

void UIRelativeLayout::onChildCountChange( Node* child, const bool& removed ) {
	UILayout::onChildCountChange( child, removed );
	fixChilds();
}

void UIRelativeLayout::onParentSizeChange( const Vector2f& ) {
	fixChilds();
}

void UIRelativeLayout::fixChilds() {
	if ( getParent()->isUINode() && !getParent()->asType<UINode>()->ownsChildPosition() ) {
		setInternalPosition( Vector2f( mLayoutMargin.Left, mLayoutMargin.Top ) );
	}

	if ( getLayoutWidthRule() == LayoutSizeRule::MatchParent ) {
		Rectf padding = Rectf();

		if ( getParent()->isWidget() )
			padding = static_cast<UIWidget*>( getParent() )->getPadding();

		setInternalWidth( getParent()->getSize().getWidth() - mLayoutMargin.Left -
						  mLayoutMargin.Right - padding.Left - padding.Right );
	}

	if ( getLayoutHeightRule() == LayoutSizeRule::MatchParent ) {
		Rectf padding = Rectf();

		if ( getParent()->isWidget() )
			padding = static_cast<UIWidget*>( getParent() )->getPadding();

		setInternalHeight( getParent()->getSize().getHeight() - mLayoutMargin.Top -
						   mLayoutMargin.Bottom - padding.Top - padding.Bottom );
	}

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

void UIRelativeLayout::fixChildPos( UIWidget* widget ) {
	Vector2f pos( widget->getPosition() );

	if ( widget->getLayoutPositionRule() != LayoutPositionRule::None &&
		 widget->getParent() == widget->getLayoutPositionRuleWidget()->getParent() ) {
		UIWidget* of = widget->getLayoutPositionRuleWidget();

		switch ( widget->getLayoutPositionRule() ) {
			case LayoutPositionRule::LeftOf:
				pos.x = of->getPosition().x - widget->getSize().getWidth() -
						widget->getLayoutMargin().Right - of->getLayoutMargin().Left;
				pos.y = of->getPosition().y + widget->getLayoutMargin().Top;
				break;
			case LayoutPositionRule::RightOf:
				pos.x = of->getPosition().x + of->getSize().getWidth() +
						widget->getLayoutMargin().Left + of->getLayoutMargin().Right;
				pos.y = of->getPosition().y + widget->getLayoutMargin().Top;
				break;
			case LayoutPositionRule::TopOf:
				pos.x = of->getPosition().x + widget->getLayoutMargin().Left;
				pos.y = of->getPosition().y - widget->getSize().getHeight() -
						widget->getLayoutMargin().Bottom - of->getLayoutMargin().Top;
				break;
			case LayoutPositionRule::BottomOf:
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
	switch ( widget->getLayoutWidthRule() ) {
		case LayoutSizeRule::WrapContent: {
			widget->setFlags( UI_AUTO_SIZE );
			break;
		}
		case LayoutSizeRule::MatchParent: {
			widget->setSize( getSize().getWidth() - widget->getLayoutMargin().Left -
								 widget->getLayoutMargin().Right - mPadding.Left - mPadding.Right,
							 widget->getSize().getHeight() );
			break;
		}
		case LayoutSizeRule::Fixed:
		default: {
		}
	}

	switch ( widget->getLayoutHeightRule() ) {
		case LayoutSizeRule::WrapContent: {
			widget->setFlags( UI_AUTO_SIZE );
			break;
		}
		case LayoutSizeRule::MatchParent: {
			widget->setSize( widget->getSize().getWidth(), getSize().getHeight() -
															   widget->getLayoutMargin().Top -
															   widget->getLayoutMargin().Bottom -
															   mPadding.Top - mPadding.Bottom );
			break;
		}
		case LayoutSizeRule::Fixed:
		default: {
		}
	}
}

Uint32 UIRelativeLayout::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			fixChilds();
			break;
		}
	}

	return 0;
}

}} // namespace EE::UI
