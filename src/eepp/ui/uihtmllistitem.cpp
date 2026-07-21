#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uihtmllistitem.hpp>
#include <eepp/ui/uihtmlliststyle.hpp>
#include <eepp/ui/uilayouter.hpp>

namespace EE { namespace UI {

UIHTMLListItem* UIHTMLListItem::New() {
	return eeNew( UIHTMLListItem, () );
}

UIHTMLListItem::UIHTMLListItem() : UIRichText( "li" ) {
	mDisplay = CSSDisplay::ListItem;
}

void UIHTMLListItem::setListStyleType( CSSListStyleType type ) {
	if ( mListStyleType != type ) {
		mListStyleType = type;
		invalidateList();
	}
}

void UIHTMLListItem::setListStylePosition( CSSListStylePosition pos ) {
	if ( mListStylePosition != pos ) {
		mListStylePosition = pos;
		invalidateList();
	}
}

Uint32 UIHTMLListItem::getType() const {
	return UI_TYPE_HTML_LIST_ITEM;
}

bool UIHTMLListItem::isType( const Uint32& type ) const {
	return UIHTMLListItem::getType() == type ? true : UIRichText::isType( type );
}

void UIHTMLListItem::draw() {
	UIRichText::draw();
	if ( mVisible && 0.f != mAlpha && mDisplay == CSSDisplay::ListItem ) {
		const FontStyleConfig& style = mRichText.getFontStyleConfig();
		Float fontSize = style.CharacterSize;
		Float offset = 0.25f * fontSize;
		Float lineTop = mScreenPos.y + mPaddingPx.Top;

		if ( UIHTMLListStyle::isPrimitiveMarker( mListStyleType ) ) {
			UIHTMLListStyle::drawPrimitiveMarker( mListStyleType, mScreenPos, mPaddingPx, style );
		} else if ( mListMarkerText && !mListMarkerText->getString().empty() ) {
			Float markerX =
				mScreenPos.x + mPaddingPx.Left - mListMarkerText->getTextWidth() - offset;
			mListMarkerText->draw( markerX, lineTop, Vector2f::One, 0.f, getBlendMode() );
		}
	}
}

bool UIHTMLListItem::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ListStyleType:
			setListStyleType( CSSListStyleTypeHelper::fromString( attribute.value() ) );
			return true;
		case PropertyId::ListStylePosition:
			setListStylePosition( CSSListStylePositionHelper::fromString( attribute.value() ) );
			return true;
		default:
			return UIRichText::applyProperty( attribute );
	}
}

std::string UIHTMLListItem::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::ListStyleType:
			return CSSListStyleTypeHelper::toString( mListStyleType );
		case PropertyId::ListStylePosition:
			return CSSListStylePositionHelper::toString( mListStylePosition );
		default:
			return UIRichText::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIHTMLListItem::getPropertiesImplemented() const {
	auto props = UIRichText::getPropertiesImplemented();
	props.push_back( PropertyId::ListStyleType );
	props.push_back( PropertyId::ListStylePosition );
	return props;
}

void UIHTMLListItem::invalidateList() {
	if ( !UIHTMLListStyle::isTextMarker( mListStyleType ) ) {
		mListMarkerText.reset();
	} else {
		String::View markerStr = getListMarkerString();
		if ( !markerStr.empty() ) {
			if ( !mListMarkerText )
				mListMarkerText = std::make_unique<Text>();
			mListMarkerText->setString( markerStr );
			mListMarkerText->setStyleConfig( mRichText.getFontStyleConfig() );
		} else {
			mListMarkerText.reset();
		}
	}
	invalidateDraw();
}

int UIHTMLListItem::countPrecedingLiSiblings() const {
	int count = 0;
	Node* prev = getPrevNode();
	while ( prev ) {
		if ( prev->isWidget() && prev->asType<UIWidget>()->getElementTag() == "li" )
			count++;
		prev = prev->getPrevNode();
	}
	return count;
}

String::View UIHTMLListItem::getListMarkerString() const {
	static String sBuf;

	if ( !UIHTMLListStyle::isTextMarker( mListStyleType ) )
		return {};

	sBuf = UIHTMLListStyle::getTextMarkerString( mListStyleType, countPrecedingLiSiblings() );
	return sBuf.view();
}

}} // namespace EE::UI
