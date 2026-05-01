#include <eepp/graphics/font.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uihtmltextarea.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

UIHTMLTextArea* UIHTMLTextArea::New() {
	return eeNew( UIHTMLTextArea, () );
}

UIHTMLTextArea::UIHTMLTextArea() : UITextEdit( "textarea" ) {
	mFlags |= UI_HTML_ELEMENT;
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
	invalidateIntrinsicSize();
}

Uint32 UIHTMLTextArea::getType() const {
	return UI_TYPE_HTML_TEXTAREA;
}

bool UIHTMLTextArea::isType( const Uint32& type ) const {
	return UIHTMLTextArea::getType() == type || UITextEdit::isType( type );
}

bool UIHTMLTextArea::applyProperty( const StyleSheetProperty& attribute ) {
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

std::string UIHTMLTextArea::getPropertyString( const PropertyDefinition* propertyDef,
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

std::vector<PropertyId> UIHTMLTextArea::getPropertiesImplemented() const {
	auto props = UITextEdit::getPropertiesImplemented();
	props.push_back( PropertyId::Rows );
	props.push_back( PropertyId::Cols );
	return props;
}

Float UIHTMLTextArea::getMinIntrinsicWidth() const {
	if ( mCols > 0 && getFont() ) {
		Float advance = getFont()->getGlyph( 'M', getFontSize(), false, false ).advance;
		Float sbWidth = getVScrollBar() ? getVScrollBar()->getPixelsSize().getWidth() : 0;
		return mCols * advance + mPaddingPx.Left + mPaddingPx.Right + sbWidth;
	}
	return UITextEdit::getMinIntrinsicWidth();
}

Float UIHTMLTextArea::getMaxIntrinsicWidth() const {
	return getMinIntrinsicWidth();
}

Float UIHTMLTextArea::getMinIntrinsicHeight() const {
	if ( mRows > 0 && getFont() ) {
		return mRows * getFont()->getFontHeight( getFontSize() ) + mPaddingPx.Top +
			   mPaddingPx.Bottom;
	}
	return 0;
}

Float UIHTMLTextArea::getMaxIntrinsicHeight() const {
	return getMinIntrinsicHeight();
}

void UIHTMLTextArea::onAutoSize() {
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

Uint32 UIHTMLTextArea::getRows() const {
	return mRows;
}

void UIHTMLTextArea::setRows( Uint32 rows ) {
	if ( mRows != rows ) {
		mRows = rows;
		invalidateIntrinsicSize();
		onAutoSize();
	}
}

Uint32 UIHTMLTextArea::getCols() const {
	return mCols;
}

void UIHTMLTextArea::setCols( Uint32 cols ) {
	if ( mCols != cols ) {
		mCols = cols;
		invalidateIntrinsicSize();
		onAutoSize();
	}
}

}} // namespace EE::UI
