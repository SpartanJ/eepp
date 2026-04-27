#include <eepp/ui/uihtmlimage.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UIHTMLImage* UIHTMLImage::New() {
	return eeNew( UIHTMLImage, () );
}

UIHTMLImage::UIHTMLImage() : UIImage( "img" ) {}

UIHTMLImage::~UIHTMLImage() {}

Uint32 UIHTMLImage::getType() const {
	return UI_TYPE_HTML_IMAGE;
}

bool UIHTMLImage::isType( const Uint32& type ) const {
	return UIHTMLImage::getType() == type ? true : UIImage::isType( type );
}

void UIHTMLImage::loadFromXmlNode( const pugi::xml_node& node ) {
	for ( auto& attr : node.attributes() ) {
		if ( String::iequals( attr.name(), "alt" ) ) {
			mAlt = attr.value();
			break;
		}
	}

	beginAttributesTransaction();
	UIWidget::loadFromXmlNode( node );
	endAttributesTransaction();
}

void UIHTMLImage::draw() {
	if ( mVisible && NULL != mDrawable && 0.f != mAlpha ) {
		UIImage::draw();
	} else if ( mVisible && 0.f != mAlpha && !mAlt.empty() ) {
		UINode::draw();

		auto* themeManager = getUISceneNode()->getUIThemeManager();
		FontStyleConfig fontStyleConfig;
		fontStyleConfig.Font = themeManager->getDefaultFont();
		fontStyleConfig.CharacterSize = themeManager->getDefaultFontSize();
		fontStyleConfig.FontColor = Color( 128, 128, 128, mAlpha );

		Float textWidth = Text::getTextWidth( mAlt, fontStyleConfig );
		Float availableWidth = mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right;
		Float x = mScreenPos.x + mPaddingPx.Left;
		Float y = mScreenPos.y + mPaddingPx.Top;

		if ( textWidth < availableWidth )
			x += ( availableWidth - textWidth ) / 2;

		y += ( mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom -
			   PixelDensity::getPixelDensity() * fontStyleConfig.CharacterSize ) /
			 2;

		Text::draw( String( mAlt ), Vector2f( x, y ), fontStyleConfig );
	}
}

const std::string& UIHTMLImage::getAlt() const {
	return mAlt;
}

UIHTMLImage* UIHTMLImage::setAlt( const std::string& alt ) {
	if ( mAlt != alt ) {
		mAlt = alt;
		invalidateDraw();
	}
	return this;
}

}} // namespace EE::UI
