#include <eepp/graphics/fonttruetype.hpp>
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
	for ( auto& it : mSizes ) {
		int diff = abs( it.first - size );
		if ( diff < distance ) {
			distance = diff;
			closest = it.second;
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
	mCloseCb = mFont->pushFontEventCallback( [&]( Uint32, Font::Event event, Font* ) {
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

}} // namespace EE::UI
