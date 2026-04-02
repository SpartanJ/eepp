#include <eepp/graphics/font.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/htmltextinput.hpp>
#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

HTMLTextInput* HTMLTextInput::New() {
	return eeNew( HTMLTextInput, () );
}

HTMLTextInput::HTMLTextInput() : UITextInput( "textinput" ) {
	mHtmlSize = 20;
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
	invalidateIntrinsicSize();
	onAutoSize();
}

Uint32 HTMLTextInput::getType() const {
	return UI_TYPE_HTML_TEXTINPUT;
}

bool HTMLTextInput::isType( const Uint32& type ) const {
	return HTMLTextInput::getType() == type || UITextInput::isType( type );
}

bool HTMLTextInput::applyProperty( const StyleSheetProperty& attribute ) {
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

std::string HTMLTextInput::getPropertyString( const PropertyDefinition* propertyDef,
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

std::vector<PropertyId> HTMLTextInput::getPropertiesImplemented() const {
	auto props = UITextInput::getPropertiesImplemented();
	props.push_back( PropertyId::Size );
	return props;
}

Float HTMLTextInput::getMinIntrinsicWidth() const {
	if ( mHtmlSize > 0 && getFont() ) {
		Float advance = getFont()->getGlyph( 'M', getFontSize(), false, false ).advance;
		return mHtmlSize * advance + mPaddingPx.Left + mPaddingPx.Right;
	}
	return UITextInput::getMinIntrinsicWidth();
}

Float HTMLTextInput::getMaxIntrinsicWidth() const {
	return getMinIntrinsicWidth();
}

void HTMLTextInput::onAutoSize() {
	if ( mPacking )
		return;
	mPacking = true;

	if ( mWidthPolicy == SizePolicy::WrapContent && getFont() ) {
		Float width = getMinIntrinsicWidth();
		if ( width > 0 ) {
			setInternalPixelsWidth( width );
		}
	}
	if ( mHeightPolicy == SizePolicy::WrapContent && getFont() ) {
		Float height = getFont()->getFontHeight( getFontSize() ) + mPaddingPx.Top +
								 mPaddingPx.Bottom;
		if ( height > 0 ) {
			setInternalPixelsHeight( height );
		}
	}
	UITextInput::onAutoSize();
	mPacking = false;
}

Uint32 HTMLTextInput::getHtmlSize() const {
	return mHtmlSize;
}

void HTMLTextInput::setHtmlSize( Uint32 size ) {
	if ( mHtmlSize != size ) {
		mHtmlSize = size;
		invalidateIntrinsicSize();
		onAutoSize();
		onSizeChange();
	}
}

}} // namespace EE::UI
