#include <eepp/graphics/fontsprite.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>

namespace EE { namespace Graphics {

FontSprite* FontSprite::New( const std::string fontName ) {
	return eeNew( FontSprite, ( fontName ) );
}

FontSprite* FontSprite::New( const std::string fontName, const std::string& filename ) {
	FontSprite* fontSprite = New( fontName );
	fontSprite->loadFromFile( filename );
	return fontSprite;
}

FontSprite::FontSprite( const std::string FontName ) :
	Font( FontType::Sprite, FontName ), mInfo(), mFontSize( 0 ) {}

void FontSprite::cleanup() {
	sendEvent( Event::Unload );

	Texture* texture = mPages[mFontSize].texture;

	if ( NULL != texture && TextureFactory::existsSingleton() )
		TextureFactory::instance()->remove( texture->getTextureId() );

	mPages.clear();
	mFontSize = 0;
	mFilePath = "";
}

FontSprite::~FontSprite() {
	cleanup();
}

bool FontSprite::loadFromFile( const std::string& filename, Color key, Uint32 firstChar,
							   int spacing ) {
	cleanup();
	if ( FileSystem::fileExists( filename ) ) {
		mFilePath = FileSystem::fileRemoveFileName( filename );
		IOStreamFile stream( filename );
		return loadFromStream( stream, key, firstChar, spacing );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string path( filename );
		Pack* pack = PackManager::instance()->exists( path );

		if ( NULL != pack ) {
			Log::info( "Loading font from pack: %s", path.c_str() );

			return loadFromPack( pack, path, key, firstChar, spacing );
		}
	}
	return false;
}

bool FontSprite::loadFromMemory( const void* data, std::size_t sizeInBytes, Color key,
								 Uint32 firstChar, int spacing ) {
	cleanup();
	mFilePath = FileSystem::getCurrentWorkingDirectory();
	IOStreamMemory stream( (const char*)data, sizeInBytes );
	return loadFromStream( stream, key, firstChar, spacing );
}

bool FontSprite::loadFromStream( IOStream& stream, Color key, Uint32 firstChar, int spacing ) {
	if ( !stream.isOpen() )
		return false;

	Image img( stream );

	if ( img.getPixelsPtr() == NULL )
		return false;

	/** Implementation specification taken from raylib ( LICENSE zlib/libpng ). Copyright (c) Ramon
	 * Santamaria (@raysan5) */
	int x = 0;
	int y = 0;
	int w = img.getWidth();
	int h = img.getHeight();

	for ( y = 0; y < h; y++ ) {
		for ( x = 0; x < w; x++ ) {
			if ( img.getPixel( x, y ) != key )
				break;
		}

		if ( img.getPixel( x, y ) != key )
			break;
	}

	int charSpacing = x;
	int lineSpacing = y;
	int charRowHeight = 0;

	while ( img.getPixel( charSpacing, lineSpacing + charRowHeight ) != key )
		charRowHeight++;

	int charHeight = charRowHeight;
	int index = 0;

	mFontSize = charHeight;

	int lineToRead = 0;
	int xPosToRead = charSpacing;

	GlyphTable& glyphs = mPages[mFontSize].glyphs;

	while ( ( lineSpacing + lineToRead * ( charHeight + lineSpacing ) ) < h ) {
		while ( xPosToRead < w &&
				img.getPixel( xPosToRead,
							  lineSpacing + lineToRead * ( charHeight + lineSpacing ) ) != key ) {
			int charWidth = 0;

			while ( img.getPixel( xPosToRead + charWidth,
								  lineSpacing + lineToRead * ( charHeight + lineSpacing ) ) != key )
				charWidth++;

			Glyph& glyph = glyphs[firstChar + index];

			glyph.textureRect =
				Rect( xPosToRead, lineSpacing + lineToRead * ( charHeight + lineSpacing ),
					  charWidth, charHeight );
			glyph.advance = charWidth + spacing;
			glyph.bounds = Rectf( 0.f, -charHeight, charWidth, charHeight );
			glyph.font = this;

			xPosToRead += ( charWidth + charSpacing );

			index++;
		}

		lineToRead++;
		xPosToRead = charSpacing;
	}

	img.createMaskFromColor( Color::Fuchsia, 0 );

	Texture* texture = TextureFactory::instance()->loadFromPixels(
		img.getPixelsPtr(), img.getWidth(), img.getHeight(), img.getChannels() );
	mPages[mFontSize].texture = texture;
	if ( NULL != texture ) {
		texture->setFilter( Texture::Filter::Nearest );
		texture->setCoordinateType( Texture::CoordinateType::Pixels );
	}
	sendEvent( Event::Load );

	return true;
}

bool FontSprite::loadFromPack( Pack* pack, std::string filePackPath, Color key, Uint32 firstChar,
							   int spacing ) {
	cleanup();
	mFilePath = FileSystem::fileRemoveFileName( filePackPath );
	return loadFromStream( *pack->getFileStream( filePackPath ), key, firstChar, spacing );
}

const FontSprite::Info& FontSprite::getInfo() const {
	return mInfo;
}

Glyph FontSprite::getGlyph( Uint32 codePoint, unsigned int characterSize, bool, bool,
							Float ) const {
	GlyphTable& glyphs = mPages[characterSize].glyphs;

	GlyphTable::const_iterator it = glyphs.find( codePoint );

	if ( it != glyphs.end() ) {
		return it->second;
	} else {
		glyphs[characterSize] = loadGlyph( codePoint, characterSize );
		return glyphs[characterSize];
	}
}

GlyphDrawable* FontSprite::getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
											 bool bold, bool italic,
											 Float outlineThickness ) const {
	GlyphDrawableTable& drawables = mPages[characterSize].drawables;
	auto it = drawables.find( codePoint );
	if ( it != drawables.end() ) {
		return it->second;
	} else {
		auto glyph = getGlyph( codePoint, characterSize, bold, italic, outlineThickness );
		const auto& page = mPages[characterSize];
		GlyphDrawable* region = GlyphDrawable::New(
			page.texture, glyph.textureRect, glyph.bounds.getSize(),
			String::format( "%s_%d_%u", mFontName.c_str(), characterSize, codePoint ) );
		region->setAdvance( glyph.bounds.getSize().getWidth() );
		drawables[codePoint] = region;
		return region;
	}
}

Glyph FontSprite::loadGlyph( Uint32 codePoint, unsigned int characterSize ) const {
	Glyph glyph;

	GlyphTable& glyphs = mPages[mFontSize].glyphs;
	GlyphTable::const_iterator it = glyphs.find( codePoint );

	if ( it != glyphs.end() ) {
		auto oriGlyph = it->second;

		Float scale = (Float)characterSize / (Float)mFontSize;

		glyph.textureRect = oriGlyph.textureRect;
		glyph.bounds = oriGlyph.bounds * scale;
		glyph.advance = oriGlyph.advance * scale;
		glyph.size = oriGlyph.textureRect.getSize().asFloat() * scale;
	}

	return glyph;
}

Float FontSprite::getKerning( Uint32, Uint32, unsigned int, bool, bool, Float ) const {
	return 0;
}

Float FontSprite::getLineSpacing( unsigned int characterSize ) const {
	return ( (Float)characterSize / mFontSize ) * mFontSize;
}

Uint32 FontSprite::getFontHeight( const Uint32& characterSize ) const {
	return (Uint32)( (Float)characterSize / mFontSize ) * mFontSize;
}

Float FontSprite::getUnderlinePosition( unsigned int ) const {
	return 0.f;
}

Float FontSprite::getUnderlineThickness( unsigned int ) const {
	return 0.f;
}

Texture* FontSprite::getTexture( unsigned int ) const {
	return mPages[mFontSize].texture;
}

bool FontSprite::loaded() const {
	return !mPages.empty();
}

FontSprite& FontSprite::operator=( const FontSprite& right ) {
	FontSprite temp( right );
	std::swap( mInfo, temp.mInfo );
	std::swap( mPages, temp.mPages );
	std::swap( mFilePath, temp.mFilePath );
	std::swap( mFontSize, temp.mFontSize );
	return *this;
}

FontSprite::Page::~Page() {
	for ( auto drawable : drawables )
		eeDelete( drawable.second );

	if ( NULL != texture && TextureFactory::existsSingleton() )
		TextureFactory::instance()->remove( texture->getTextureId() );
}

}} // namespace EE::Graphics
