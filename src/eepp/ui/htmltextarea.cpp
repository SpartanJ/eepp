#include <eepp/graphics/font.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/htmltextarea.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

HTMLTextArea* HTMLTextArea::New() {
	return eeNew( HTMLTextArea, () );
}

HTMLTextArea::HTMLTextArea() : UITextEdit( "textarea" ) {
	mFlags |= UI_HTML_ELEMENT;
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
	invalidateIntrinsicSize();
}

Uint32 HTMLTextArea::getType() const {
	return UI_TYPE_HTML_TEXTAREA;
}

bool HTMLTextArea::isType( const Uint32& type ) const {
	return HTMLTextArea::getType() == type || UITextEdit::isType( type );
}

bool HTMLTextArea::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !attribute.getPropertyDefinition() )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Rows:
			setRows( attribute.asUint( 2 ) );
			return true;
		case PropertyId::Cols:
			setCols( attribute.asUint( 20 ) );
			return true;
		default:
			break;
	}

	return UITextEdit::applyProperty( attribute );
}

std::string HTMLTextArea::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& propertyIndex ) const {
	if ( !propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Rows:
			return String::format( "%u", mRows );
		case PropertyId::Cols:
			return String::format( "%u", mCols );
		default:
			break;
	}

	return UITextEdit::getPropertyString( propertyDef, propertyIndex );
}

std::vector<PropertyId> HTMLTextArea::getPropertiesImplemented() const {
	auto props = UITextEdit::getPropertiesImplemented();
	props.push_back( PropertyId::Rows );
	props.push_back( PropertyId::Cols );
	return props;
}

Float HTMLTextArea::getMinIntrinsicWidth() const {
	if ( mCols > 0 && getFont() ) {
		Float advance = getFont()->getGlyph( 'M', getFontSize(), false, false ).advance;
		Float sbWidth = getVScrollBar() ? getVScrollBar()->getPixelsSize().getWidth() : 0;
		return mCols * advance + mPaddingPx.Left + mPaddingPx.Right + sbWidth;
	}
	return UITextEdit::getMinIntrinsicWidth();
}

Float HTMLTextArea::getMaxIntrinsicWidth() const {
	return getMinIntrinsicWidth();
}

Float HTMLTextArea::getMinIntrinsicHeight() const {
	if ( mRows > 0 && getFont() ) {
		return mRows * getFont()->getFontHeight( getFontSize() ) + mPaddingPx.Top +
			   mPaddingPx.Bottom;
	}
	return 0;
}

Float HTMLTextArea::getMaxIntrinsicHeight() const {
	return getMinIntrinsicHeight();
}

void HTMLTextArea::onAutoSize() {
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
		Float height = getMinIntrinsicHeight();
		if ( height > 0 ) {
			setInternalPixelsHeight( height );
		}
	}
	UITextEdit::onAutoSize();
	mPacking = false;
}

Uint32 HTMLTextArea::getRows() const {
	return mRows;
}

void HTMLTextArea::setRows( Uint32 rows ) {
	if ( mRows != rows ) {
		mRows = rows;
		invalidateIntrinsicSize();
		onAutoSize();
	}
}

Uint32 HTMLTextArea::getCols() const {
	return mCols;
}

void HTMLTextArea::setCols( Uint32 cols ) {
	if ( mCols != cols ) {
		mCols = cols;
		invalidateIntrinsicSize();
		onAutoSize();
	}
}

}} // namespace EE::UI
