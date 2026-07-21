#include <eepp/graphics/font.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uihtmltextinput.hpp>
#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

UIHTMLTextInput* UIHTMLTextInput::New() {
	return eeNew( UIHTMLTextInput, () );
}

UIHTMLTextInput::UIHTMLTextInput() : UIHTMLTextInput( "textinput" ) {}

UIHTMLTextInput::UIHTMLTextInput( const std::string& tag ) : UITextInput( tag ) {
	mHtmlSize = 20;
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
	invalidateIntrinsicSize();
	onAutoSize();
}

Uint32 UIHTMLTextInput::getType() const {
	return UI_TYPE_HTML_TEXTINPUT;
}

bool UIHTMLTextInput::isType( const Uint32& type ) const {
	return UIHTMLTextInput::getType() == type || UITextInput::isType( type );
}

bool UIHTMLTextInput::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !attribute.getPropertyDefinition() )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Size:
			setHtmlSize( attribute.asUint( 20 ) );
			return true;
		default:
			break;
	}

	return UITextInput::applyProperty( attribute );
}

std::string UIHTMLTextInput::getPropertyString( const PropertyDefinition* propertyDef,
												const Uint32& propertyIndex ) const {
	if ( !propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Size:
			return String::format( "%u", mHtmlSize );
		default:
			break;
	}

	return UITextInput::getPropertyString( propertyDef, propertyIndex );
}

std::vector<PropertyId> UIHTMLTextInput::getPropertiesImplemented() const {
	auto props = UITextInput::getPropertiesImplemented();
	props.push_back( PropertyId::Size );
	return props;
}

Float UIHTMLTextInput::getMinIntrinsicWidth() const {
	if ( mHtmlSize > 0 && getFont() ) {
		Float advance = getFont()->getGlyph( 'M', getFontSize(), false, false ).advance;
		return mHtmlSize * advance + mPaddingPx.Left + mPaddingPx.Right;
	}
	return UITextInput::getMinIntrinsicWidth();
}

Float UIHTMLTextInput::getMaxIntrinsicWidth() const {
	return getMinIntrinsicWidth();
}

void UIHTMLTextInput::onAutoSize() {
	if ( mPacking )
		return;
	mPacking = true;

	if ( mWidthPolicy == SizePolicy::WrapContent && getFont() ) {
		Float width = getMinIntrinsicWidth();
		if ( width > 0 ) {
			setInternalPixelsWidth( width );
		}
	}

	UITextInput::onAutoSize();

	mPacking = false;
}

Uint32 UIHTMLTextInput::getHtmlSize() const {
	return mHtmlSize;
}

void UIHTMLTextInput::setHtmlSize( Uint32 size ) {
	if ( mHtmlSize != size ) {
		mHtmlSize = size;
		invalidateIntrinsicSize();
		onAutoSize();
		onSizeChange();
	}
}

}} // namespace EE::UI
