#include <eepp/graphics/primitives.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uihtmllistitem.hpp>
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
		Float lineHeight =
			style.Font ? style.Font->getLineSpacing( (unsigned int)fontSize ) : fontSize;
		Float lineTop = mScreenPos.y + mPaddingPx.Top;

		if ( mListStyleType == CSSListStyleType::Disc ) {
			Float radius = fontSize * 0.22f;
			Float markerX = std::floor( mScreenPos.x + mPaddingPx.Left - radius * 2.f - offset );
			Float markerY = std::floor( lineTop + ( lineHeight - radius * 2.f ) * 0.5f + radius );
			Primitives p;
			p.setColor( style.FontColor );
			p.setFillMode( PrimitiveFillMode::DRAW_FILL );
			p.drawCircle( { markerX, markerY }, radius );
		} else if ( mListStyleType == CSSListStyleType::Circle ) {
			Float radius = fontSize * 0.2f;
			Float lineWidth = fontSize * 0.04f;
			Float markerX = std::floor( mScreenPos.x + mPaddingPx.Left - radius * 2.f - offset );
			Float markerY = std::floor( lineTop + ( lineHeight - radius * 2.f ) * 0.5f + radius );
			Primitives p;
			p.setColor( style.FontColor );
			p.setFillMode( PrimitiveFillMode::DRAW_LINE );
			p.setLineWidth( lineWidth );
			p.drawCircle( { markerX, markerY }, radius );
		} else if ( mListStyleType == CSSListStyleType::Square ) {
			Float size = fontSize * 0.38f;
			Float markerX = std::floor( mScreenPos.x + mPaddingPx.Left - size - fontSize * 0.5 );
			Float markerY = std::floor( lineTop + ( lineHeight - size ) * 0.5f );
			Primitives p;
			p.setColor( style.FontColor );
			p.setFillMode( PrimitiveFillMode::DRAW_FILL );
			p.drawRectangle( Rectf( markerX, markerY, markerX + size, markerY + size ) );
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
	if ( mListStyleType == CSSListStyleType::None || mListStyleType == CSSListStyleType::Disc ||
		 mListStyleType == CSSListStyleType::Circle ||
		 mListStyleType == CSSListStyleType::Square ) {
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

	switch ( mListStyleType ) {
		case CSSListStyleType::None:
		case CSSListStyleType::Disc:
		case CSSListStyleType::Circle:
		case CSSListStyleType::Square:
			return {};
		case CSSListStyleType::Decimal: {
			int idx = countPrecedingLiSiblings() + 1;
			sBuf = String( String::toString( idx ) + ". " );
			return sBuf.view();
		}
		case CSSListStyleType::LowerAlpha: {
			int idx = countPrecedingLiSiblings();
			char c = 'a' + ( idx % 26 );
			sBuf = String( 1, (String::StringBaseType)c ) + ". ";
			return sBuf.view();
		}
		case CSSListStyleType::UpperAlpha: {
			int idx = countPrecedingLiSiblings();
			char c = 'A' + ( idx % 26 );
			sBuf = String( 1, (String::StringBaseType)c ) + ". ";
			return sBuf.view();
		}
		case CSSListStyleType::LowerRoman: {
			static const char* numerals[] = { "i",	 "ii",	 "iii", "iv", "v",	"vi",
											  "vii", "viii", "ix",	"x",  "xi", "xii" };
			int idx = countPrecedingLiSiblings();
			if ( idx < 12 )
				sBuf = String( numerals[idx] ) + ". ";
			else
				sBuf = String( String::toString( idx + 1 ) + ". " );
			return sBuf.view();
		}
		case CSSListStyleType::UpperRoman: {
			static const char* numerals[] = { "I",	 "II",	 "III", "IV", "V",	"VI",
											  "VII", "VIII", "IX",	"X",  "XI", "XII" };
			int idx = countPrecedingLiSiblings();
			if ( idx < 12 )
				sBuf = String( numerals[idx] ) + ". ";
			else
				sBuf = String( String::toString( idx + 1 ) + ". " );
			return sBuf.view();
		}
	}
	return {};
}

}} // namespace EE::UI
