#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uilayoutermanager.hpp>

namespace EE { namespace UI {

UIHTMLWidget* UIHTMLWidget::New() {
	return eeNew( UIHTMLWidget, () );
}

UIHTMLWidget::UIHTMLWidget( const std::string& tag ) : UILayout( tag ) {}

UIHTMLWidget::~UIHTMLWidget() {
	eeSAFE_DELETE( mLayouter );
}

UILayouter* UIHTMLWidget::getLayouter() {
	if ( !mLayouter ) {
		mLayouter = UILayouterManager::create( mDisplay, this );
	}
	return mLayouter;
}

Uint32 UIHTMLWidget::getType() const {
	return UI_TYPE_HTML_WIDGET;
}

bool UIHTMLWidget::isType( const Uint32& type ) const {
	return UIHTMLWidget::getType() == type ? true : UILayout::isType( type );
}

bool UIHTMLWidget::isPacking() const {
	UILayouter* layouter = const_cast<UIHTMLWidget*>( this )->getLayouter();
	if ( layouter )
		return layouter->isPacking();
	return UILayout::isPacking();
}

void UIHTMLWidget::onDisplayChange() {
	eeSAFE_DELETE( mLayouter );
	getLayouter();
	notifyLayoutAttrChange();
}

void UIHTMLWidget::setDisplay( CSSDisplay display ) {
	if ( mDisplay != display ) {
		mDisplay = display;
		onDisplayChange();
	}
}

void UIHTMLWidget::setCSSPosition( CSSPosition position ) {
	if ( mPosition != position ) {
		mPosition = position;
		onPositionChange();
	}
}

void UIHTMLWidget::setOffsets( const Rectf& offsets ) {
	if ( mOffsets != offsets ) {
		mOffsets = offsets;
		notifyLayoutAttrChange();
	}
}

void UIHTMLWidget::setZIndex( int zIndex ) {
	mZIndex = zIndex;
}

std::string UIHTMLWidget::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& state ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Display:
			return CSSDisplayHelper::toString( mDisplay );
		case PropertyId::Position:
			return CSSPositionHelper::toString( mPosition );
		case PropertyId::Top:
			return String::fromFloat( mOffsets.Top, "dp" );
		case PropertyId::Right:
			return String::fromFloat( mOffsets.Right, "dp" );
		case PropertyId::Bottom:
			return String::fromFloat( mOffsets.Bottom, "dp" );
		case PropertyId::Left:
			return String::fromFloat( mOffsets.Left, "dp" );
		case PropertyId::ZIndex:
			return String::toString( mZIndex );
		default:
			return UILayout::getPropertyString( propertyDef );
	}
}

bool UIHTMLWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Display: {
			setDisplay( CSSDisplayHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::Position: {
			setCSSPosition( CSSPositionHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::ZIndex: {
			setZIndex( attribute.asInt() );
			return true;
		}
		case PropertyId::Top: {
			if ( attribute.asString() == "auto" )
				mOffsets.Top = 0;
			else
				mOffsets.Top = lengthFromValueAsDp( attribute );
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Right: {
			if ( attribute.asString() == "auto" )
				mOffsets.Right = 0;
			else
				mOffsets.Right = lengthFromValueAsDp( attribute );
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Bottom: {
			if ( attribute.asString() == "auto" )
				mOffsets.Bottom = 0;
			else
				mOffsets.Bottom = lengthFromValueAsDp( attribute );
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Left: {
			if ( attribute.asString() == "auto" )
				mOffsets.Left = 0;
			else
				mOffsets.Left = lengthFromValueAsDp( attribute );
			notifyLayoutAttrChange();
			return true;
		}
		default:
			break;
	}

	return UILayout::applyProperty( attribute );
}

void UIHTMLWidget::updateLayout() {
	if ( getLayouter() )
		getLayouter()->updateLayout();
	else
		UILayout::updateLayout();

	positionOutOfFlowChildren();
}

UIWidget* UIHTMLWidget::getContainingBlock() {
	if ( mPosition == CSSPosition::Fixed ) {
		Node* parent = getParent();
		UIWidget* lastWidget = parent && parent->isWidget() ? parent->asType<UIWidget>() : nullptr;
		while ( parent ) {
			if ( parent->isWidget() )
				lastWidget = parent->asType<UIWidget>();
			parent = parent->getParent();
		}
		return lastWidget;
	}

	Node* parent = getParent();
	UIWidget* lastWidget = nullptr;
	while ( parent ) {
		if ( parent->isWidget() ) {
			lastWidget = parent->asType<UIWidget>();
			if ( lastWidget->isType( UI_TYPE_HTML_WIDGET ) ) {
				if ( lastWidget->asType<UIHTMLWidget>()->getCSSPosition() != CSSPosition::Static ) {
					return lastWidget;
				}
			}
		}
		parent = parent->getParent();
	}
	return lastWidget;
}

void UIHTMLWidget::positionOutOfFlowChildren() {
	Node* child = mChild;
	while ( child ) {
		if ( child->isWidget() && child->isType( UI_TYPE_HTML_WIDGET ) ) {
			UIHTMLWidget* htmlChild = static_cast<UIHTMLWidget*>( child );
			CSSPosition pos = htmlChild->getCSSPosition();
			if ( pos == CSSPosition::Absolute || pos == CSSPosition::Fixed ) {
				UIWidget* cb = htmlChild->getContainingBlock();
				if ( cb ) {
					Rectf offsets = htmlChild->getOffsets();
					Float top = PixelDensity::dpToPx( offsets.Top );
					Float left = PixelDensity::dpToPx( offsets.Left );

					Vector2f cbPos( cb->getPixelsContentOffset().Left, cb->getPixelsContentOffset().Top );
					cbPos.x += left;
					cbPos.y += top;

					Vector2f worldPos = cb->convertToWorldSpace( cbPos );
					Vector2f localPos = convertToNodeSpace( worldPos );
					htmlChild->setPixelsPosition( localPos );
				}
			}
		}
		child = child->getNextNode();
	}
}

void UIHTMLWidget::invalidateIntrinsicSize() {
	if ( mLayouter )
		mLayouter->invalidateIntrinsicWidths();
	UIWidget::invalidateIntrinsicSize();
}

bool UIHTMLWidget::isOutOfFlow() const {
	return mPosition == CSSPosition::Absolute || mPosition == CSSPosition::Fixed;
}

}} // namespace EE::UI
