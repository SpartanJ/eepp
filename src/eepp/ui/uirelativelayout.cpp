#include <eepp/ui/uirelativelayout.hpp>

namespace EE { namespace UI {

UIRelativeLayout * UIRelativeLayout::New() {
	return eeNew( UIRelativeLayout, () );
}

UIRelativeLayout::UIRelativeLayout() :
	UIWidget()
{
}

void UIRelativeLayout::onSizeChange() {
	fixChilds();
}

void UIRelativeLayout::onChildCountChange() {
	fixChilds();
}

void UIRelativeLayout::fixChilds() {
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

}}
