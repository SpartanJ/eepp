#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/ui/uiicon.hpp>

namespace EE { namespace UI {

UIIcon* UIIcon::New( const std::string& name ) {
	return eeNew( UIIcon, ( name ) );
}

UIIcon::UIIcon( const std::string& name ) : mName( name ) {}

UIIcon::~UIIcon() {}

const std::string& UIIcon::getName() const {
	return mName;
}

Drawable* UIIcon::getSize( const int& size ) const {
	auto it = mSizes.find( size );
	if ( it != mSizes.end() )
		return it->second;
	int distance = UINT32_MAX;
	Drawable* closest = nullptr;
	for ( const auto& sit : mSizes ) {
		int diff = abs( sit.first - size );
		if ( diff < distance ) {
			distance = diff;
			closest = sit.second;
		}
	}
	return closest;
}

void UIIcon::setSize( const int& size, Drawable* drawable ) {
	mSizes[size] = drawable;
}

UIIcon* UIGlyphIcon::New( const std::string& name, FontTrueType* font, const Uint32& codePoint ) {
	return eeNew( UIGlyphIcon, ( name, font, codePoint ) );
}

Drawable* UIGlyphIcon::getSize( const int& size ) const {
	if ( !mFont )
		return nullptr;
	auto it = mSizes.find( size );
	if ( it != mSizes.end() )
		return it->second;
	GlyphDrawable* drawable = mFont->getGlyphDrawable( mCodePoint, size );
	const_cast<UIGlyphIcon*>( this )->setSize( size, drawable );
	return drawable;
}

UIGlyphIcon::UIGlyphIcon( const std::string& name, FontTrueType* font, const Uint32& codePoint ) :
	UIIcon( name ), mFont( font ), mCodePoint( codePoint ) {
	eeASSERT( mFont );
	mCloseCb = mFont->pushFontEventCallback( [this]( Uint32, Font::Event event, Font* ) {
		if ( event == Font::Event::Unload )
			mFont = nullptr;
	} );
}

UIGlyphIcon::~UIGlyphIcon() {
	if ( mFont && mCloseCb != 0 ) {
		mFont->popFontEventCallback( mCloseCb );
		mFont = nullptr;
		mCloseCb = 0;
	}
}

UIIcon* UISVGIcon::New( const std::string& name, const std::string& svgXML ) {
	return eeNew( UISVGIcon, ( name, svgXML ) );
}

UISVGIcon::~UISVGIcon() {}

Drawable* UISVGIcon::getSize( const int& size ) const {
	auto it = mSVGs.find( size );
	if ( it != mSVGs.end() )
		return it->second;

	Image::FormatConfiguration format;
	if ( mOriSize == Sizei::Zero ) {
		int w, h, c;
		if ( Image::getInfoFromMemory( (const unsigned char*)mSVGXml.data(), mSVGXml.size(), &w, &h,
									   &c, format ) ) {
			mOriSize = { w, h };
			mOriChannels = c;
		} else {
			return nullptr;
		}
	}
	format.svgScale( size / (Float)eemax( mOriSize.x, mOriSize.y ) );
	Texture* texture = TextureFactory::instance()->loadFromMemory(
		(const unsigned char*)&mSVGXml[0], mSVGXml.size(), false, Texture::ClampMode::ClampToEdge,
		false, false, format );

	mSVGs[size] = texture;
	return texture;
}

UISVGIcon::UISVGIcon( const std::string& name, const std::string& svgXML ) :
	UIIcon( name ), mSVGXml( svgXML ) {}

}} // namespace EE::UI
