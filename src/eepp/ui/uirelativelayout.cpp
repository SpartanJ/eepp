#include <eepp/ui/uirelativelayout.hpp>

namespace EE { namespace UI {

UIRelativeLayout * UIRelativeLayout::New() {
	return eeNew( UIRelativeLayout, () );
}

UIRelativeLayout::UIRelativeLayout() :
	UIWidget()
{
}

UIRelativeLayout * UIRelativeLayout::add(UIWidget * widget) {
	widget->setParent( this );
	return this;
}

void UIRelativeLayout::onSizeChange() {
	fixChilds();
}

void UIRelativeLayout::onChildCountChange() {
	fixChilds();
}

void UIRelativeLayout::onParentSizeChange( const Vector2i& SizeChange ) {
	fixChilds();
}

void UIRelativeLayout::fixChilds() {
	setInternalPosition( Vector2i( mLayoutMargin.Left, mPos.y ) );
	setInternalPosition( Vector2i( mPos.x, mLayoutMargin.Top ) );

	if ( getLayoutWidthRules() == MATCH_PARENT ) {
		setInternalWidth( getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right );
	}

	if ( getLayoutHeightRules() == MATCH_PARENT ) {
		setInternalHeight( getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom );
	}

	UIControl * child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() ) {
			UIWidget * widget = static_cast<UIWidget*>( child );

			fixChildSize( widget );

			fixChildPos( widget );
		}

		child = child->getNextControl();
	}
}

void UIRelativeLayout::fixChildPos( UIWidget * widget ) {
	Vector2i pos( widget->getPosition() );

	if ( widget->getLayoutPositionRule() != LayoutPositionRules::NONE && widget->getParent() == widget->getLayoutPositionRuleWidget()->getParent() ) {
		UIWidget * of = widget->getLayoutPositionRuleWidget();

		switch ( widget->getLayoutPositionRule() ) {
			case LEFT_OF:
				pos.x = of->getPosition().x - widget->getSize().getWidth() - widget->getLayoutMargin().Right - of->getLayoutMargin().Left;
				pos.y = of->getPosition().y;
				break;
			case RIGHT_OF:
				pos.x = of->getPosition().x + of->getSize().getWidth() + widget->getLayoutMargin().Left + of->getLayoutMargin().Right;
				pos.y = of->getPosition().y;
				break;
			case TOP_OF:
				pos.x = of->getPosition().x;
				pos.y = of->getPosition().y - widget->getSize().getHeight() - widget->getLayoutMargin().Bottom - of->getLayoutMargin().Top;
				break;
			case BOTTOM_OF:
				pos.x = of->getPosition().x;
				pos.y = of->getPosition().y + of->getSize().getHeight() + widget->getLayoutMargin().Top + of->getLayoutMargin().Bottom;
				break;
			default:
				break;
		}
	} else {
		switch ( fontHAlignGet( widget->getLayoutGravity() ) ) {
			case UI_HALIGN_CENTER:
				pos.x = ( mSize.getWidth() - widget->getSize().getWidth() ) / 2;
				break;
			case UI_HALIGN_RIGHT:
				pos.x = mSize.getWidth() - widget->getSize().getWidth() - widget->getLayoutMargin().Right;
				break;
			case UI_HALIGN_LEFT:
			default:
				pos.x = widget->getLayoutMargin().Left;
				break;
		}

		switch ( fontVAlignGet( widget->getLayoutGravity() ) ) {
			case UI_VALIGN_CENTER:
				pos.y = ( mSize.getHeight() - widget->getSize().getHeight() ) / 2;
				break;
			case UI_VALIGN_BOTTOM:
				pos.y = mSize.getHeight() - widget->getSize().getHeight() - widget->getLayoutMargin().Bottom;
				break;
			case UI_VALIGN_TOP:
			default:
				pos.y = widget->getLayoutMargin().Left;
				break;
		}
	}

	widget->setPosition( pos );
}

void UIRelativeLayout::fixChildSize( UIWidget * widget ) {
	switch ( widget->getLayoutWidthRules() ) {
		case WRAP_CONTENT:
		{
			widget->setFlags( UI_AUTO_SIZE );
			break;
		}
		case MATCH_PARENT:
		{
			widget->setSize( mSize.getWidth() - widget->getLayoutMargin().Left - widget->getLayoutMargin().Right, widget->getSize().getHeight() );
			break;
		}
		case FIXED:
		default:
		{
		}
	}

	switch ( widget->getLayoutHeightRules() ) {
		case WRAP_CONTENT:
		{
			widget->setFlags( UI_AUTO_SIZE );
			break;
		}
		case MATCH_PARENT:
		{
			widget->setSize( widget->getSize().getWidth(), mSize.getHeight() - widget->getLayoutMargin().Top - widget->getLayoutMargin().Bottom );
			break;
		}
		case FIXED:
		default:
		{
		}
	}
}

Uint32 UIRelativeLayout::onMessage(const UIMessage * Msg) {
	switch( Msg->getMsg() ) {
		case UIMessage::MsgLayoutAttributeChange:
		{
			fixChilds();
			return 1;
		}
	}

	return 0;
}

}}
