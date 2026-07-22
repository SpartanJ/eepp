#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/ui/uiicon.hpp>
#include <limits>

namespace EE { namespace UI {

UIIcon* UIIcon::New( const std::string& name ) {
	return eeNew( UIIcon, ( name ) );
}

UIIcon::UIIcon( const std::string& name ) : mName( name ) {}

UIIcon::~UIIcon() {}

const std::string& UIIcon::getName() const {
	return mName;
}

const DrawablePtr& UIIcon::getSource( const int& size ) const {
	static const DrawablePtr empty;
	auto it = mSizes.find( size );
	if ( it != mSizes.end() )
		return it->second;
	int distance = std::numeric_limits<int>::max();
	const DrawablePtr* closest = nullptr;
	for ( const auto& sit : mSizes ) {
		int diff = abs( sit.first - size );
		if ( diff < distance ) {
			distance = diff;
			closest = &sit.second;
		}
	}
	return closest ? *closest : empty;
}

DrawablePtr UIIcon::createDrawable( const int& size ) const {
	const DrawablePtr& source = getSource( size );
	return source ? source->clone() : DrawablePtr{};
}

void UIIcon::setSource( const int& size, DrawablePtr drawable ) {
	mSizes[size] = std::move( drawable );
}

UIIcon* UIGlyphIcon::New( const std::string& name, FontTrueType* font, const Uint32& codePoint ) {
	return eeNew( UIGlyphIcon, ( name, font, codePoint ) );
}

const DrawablePtr& UIGlyphIcon::getSource( const int& size ) const {
	static const DrawablePtr empty;
	if ( !mFont )
		return empty;
	auto it = mSizes.find( size );
	if ( it != mSizes.end() )
		return it->second;
	GlyphDrawable* drawable = mFont->getGlyphDrawable( mCodePoint, size );
	if ( !drawable )
		return empty;
	const_cast<UIGlyphIcon*>( this )->setSource( size, drawable->clone() );
	return UIIcon::getSource( size );
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

const DrawablePtr& UISVGIcon::getSource( const int& size ) const {
	static const DrawablePtr empty;
	auto it = mSizes.find( size );
	if ( it != mSizes.end() )
		return it->second;

	Image::FormatConfiguration format;
	if ( mOriSize == Sizei::Zero ) {
		int w, h, c;
		if ( Image::getInfoFromMemory( (const unsigned char*)mSVGXml.data(), mSVGXml.size(), &w, &h,
									   &c, format ) ) {
			mOriSize = { w, h };
			mOriChannels = c;
		} else {
			return empty;
		}
	}
	format.svgScale( size / (Float)eemax( mOriSize.x, mOriSize.y ) );
	TexturePtr texture = TextureFactory::instance()->loadFromMemory(
		(const unsigned char*)&mSVGXml[0], mSVGXml.size(), false, Texture::ClampMode::ClampToEdge,
		false, false, format );

	if ( !texture )
		return empty;
	const_cast<UISVGIcon*>( this )->setSource( size, std::move( texture ) );
	return UIIcon::getSource( size );
}

UISVGIcon::UISVGIcon( const std::string& name, const std::string& svgXML ) :
	UIIcon( name ), mSVGXml( svgXML ) {}

}} // namespace EE::UI
