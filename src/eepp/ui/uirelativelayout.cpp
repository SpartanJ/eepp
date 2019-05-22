#include <eepp/ui/uirelativelayout.hpp>

namespace EE { namespace UI {

UIRelativeLayout * UIRelativeLayout::New() {
	return eeNew( UIRelativeLayout, () );
}

UIRelativeLayout::UIRelativeLayout() :
	UILayout( "relativelayout" )
{
}

Uint32 UIRelativeLayout::getType() const {
	return UI_TYPE_RELATIVE_LAYOUT;
}

bool UIRelativeLayout::isType( const Uint32& type ) const {
	return UIRelativeLayout::getType() == type ? true : UIWidget::isType( type );
}

UIRelativeLayout * UIRelativeLayout::add(UIWidget * widget) {
	widget->setParent( this );
	return this;
}

void UIRelativeLayout::onSizeChange() {
	UILayout::onSizeChange();
	fixChilds();
}

void UIRelativeLayout::onChildCountChange() {
	UILayout::onChildCountChange();
	fixChilds();
}

void UIRelativeLayout::onParentSizeChange( const Vector2f& ) {
	fixChilds();
}

void UIRelativeLayout::fixChilds() {
	setInternalPosition( Vector2f( mLayoutMargin.Left, mLayoutMargin.Top ) );

	if ( getLayoutWidthRules() == MATCH_PARENT ) {
		Rectf padding = Rectf();

		if ( getParent()->isWidget() )
			padding = static_cast<UIWidget*>( getParent() )->getPadding();

		setInternalWidth( getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right - padding.Left - padding.Right );
	}

	if ( getLayoutHeightRules() == MATCH_PARENT ) {
		Rectf padding = Rectf();

		if ( getParent()->isWidget() )
			padding = static_cast<UIWidget*>( getParent() )->getPadding();

		setInternalHeight( getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom - padding.Top - padding.Bottom );
	}

	Node * child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() ) {
			UIWidget * widget = static_cast<UIWidget*>( child );

			fixChildSize( widget );

			fixChildPos( widget );
		}

		child = child->getNextNode();
	}
}

void UIRelativeLayout::fixChildPos( UIWidget * widget ) {
	Vector2f pos( widget->getPosition() );

	if ( widget->getLayoutPositionRule() != NONE && widget->getParent() == widget->getLayoutPositionRuleWidget()->getParent() ) {
		UIWidget * of = widget->getLayoutPositionRuleWidget();

		switch ( widget->getLayoutPositionRule() ) {
			case LEFT_OF:
				pos.x = of->getPosition().x - widget->getSize().getWidth() - widget->getLayoutMargin().Right - of->getLayoutMargin().Left;
				pos.y = of->getPosition().y + widget->getLayoutMargin().Top;
				break;
			case RIGHT_OF:
				pos.x = of->getPosition().x + of->getSize().getWidth() + widget->getLayoutMargin().Left + of->getLayoutMargin().Right;
				pos.y = of->getPosition().y + widget->getLayoutMargin().Top;
				break;
			case TOP_OF:
				pos.x = of->getPosition().x + widget->getLayoutMargin().Left;
				pos.y = of->getPosition().y - widget->getSize().getHeight() - widget->getLayoutMargin().Bottom - of->getLayoutMargin().Top;
				break;
			case BOTTOM_OF:
				pos.x = of->getPosition().x + widget->getLayoutMargin().Left;
				pos.y = of->getPosition().y + of->getSize().getHeight() + widget->getLayoutMargin().Top + of->getLayoutMargin().Bottom;
				break;
			default:
				break;
		}
	} else {
		switch ( fontHAlignGet( widget->getLayoutGravity() ) ) {
			case UI_HALIGN_CENTER:
				pos.x = ( mDpSize.getWidth() - widget->getSize().getWidth() ) / 2 + widget->getLayoutMargin().Left;
				break;
			case UI_HALIGN_RIGHT:
				pos.x = mDpSize.getWidth() - widget->getSize().getWidth() - widget->getLayoutMargin().Right - mPadding.Right;
				break;
			case UI_HALIGN_LEFT:
			default:
				pos.x = widget->getLayoutMargin().Left + mPadding.Left;
				break;
		}

		switch ( fontVAlignGet( widget->getLayoutGravity() ) ) {
			case UI_VALIGN_CENTER:
				pos.y = ( mDpSize.getHeight() - widget->getSize().getHeight() ) / 2 + widget->getLayoutMargin().Top;
				break;
			case UI_VALIGN_BOTTOM:
				pos.y = mDpSize.getHeight() - widget->getSize().getHeight() - widget->getLayoutMargin().Bottom - mPadding.Bottom;
				break;
			case UI_VALIGN_TOP:
			default:
				pos.y = widget->getLayoutMargin().Top + mPadding.Top;
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
			widget->setSize( mDpSize.getWidth() - widget->getLayoutMargin().Left - widget->getLayoutMargin().Right - mPadding.Left - mPadding.Right, widget->getSize().getHeight() );
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
			widget->setSize( widget->getSize().getWidth(), mDpSize.getHeight() - widget->getLayoutMargin().Top - widget->getLayoutMargin().Bottom - mPadding.Top - mPadding.Bottom );
			break;
		}
		case FIXED:
		default:
		{
		}
	}
}

Uint32 UIRelativeLayout::onMessage(const NodeMessage * Msg) {
	switch( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange:
		{
			fixChilds();
			break;
		}
	}

	return 0;
}

}}
