#include <eepp/graphics/fontbmfont.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>

namespace EE { namespace Graphics {

FontBMFont* FontBMFont::New( const std::string fontName ) {
	return eeNew( FontBMFont, ( fontName ) );
}

FontBMFont* FontBMFont::New( const std::string fontName, const std::string& filename ) {
	FontBMFont* fontBMFont = New( fontName );
	fontBMFont->loadFromFile( filename );
	return fontBMFont;
}

FontBMFont::FontBMFont( const std::string FontName ) :
	Font( FontType::BMF, FontName ), mInfo(), mFontSize( 0 ) {}

void FontBMFont::cleanup() {
	sendEvent( Event::Unload );

	Texture* texture = mPages[mFontSize].texture;

	if ( NULL != texture && TextureFactory::existsSingleton() )
		TextureFactory::instance()->remove( texture->getTextureId() );

	mPages.clear();
	mFontSize = 0;
	mFilePath = "";
}

FontBMFont::~FontBMFont() {
	cleanup();
}

bool FontBMFont::loadFromFile( const std::string& filename ) {
	cleanup();
	if ( FileSystem::fileExists( filename ) ) {
		mFilePath = FileSystem::fileRemoveFileName( filename );
		IOStreamFile stream( filename );
		return loadFromStream( stream );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string path( filename );
		Pack* pack = PackManager::instance()->exists( path );

		if ( NULL != pack ) {
			Log::info( "Loading font from pack: %s", path.c_str() );

			return loadFromPack( pack, path );
		}
	}
	return false;
}

bool FontBMFont::loadFromMemory( const void* data, std::size_t sizeInBytes,
								 const std::string& imageFileBasePath ) {
	cleanup();
	mFilePath =
		imageFileBasePath.empty() ? FileSystem::getCurrentWorkingDirectory() : imageFileBasePath;
	IOStreamMemory stream( (const char*)data, sizeInBytes );
	return loadFromStream( stream );
}

bool FontBMFont::loadFromStream( IOStream& stream ) {
	if ( !stream.isOpen() )
		return false;

	std::string myfile( (size_t)stream.getSize(), '\0' );

	stream.read( (char*)&myfile[0], stream.getSize() );

	std::vector<std::string> lines = String::split( myfile );

	/** Implementation specification taken from raylib ( LICENSE zlib/libpng ). Copyright (c) Ramon
	 * Santamaria (@raysan5) */
	if ( lines.size() > 4 ) {
		const char* searchPoint = NULL;
		int fontSize;
		int base;
		int texWidth;
		int texHeight;
		char texFileName[129];
		int charsCount = 0;

		searchPoint = strstr( lines[1].c_str(), "lineHeight" );
		sscanf( searchPoint, "lineHeight=%i base=%i scaleW=%i scaleH=%i", &fontSize, &base,
				&texWidth, &texHeight );

		searchPoint = strstr( lines[2].c_str(), "file" );
		sscanf( searchPoint, "file=\"%128[^\"]\"", texFileName );

		searchPoint = strstr( lines[3].c_str(), "count" );
		sscanf( searchPoint, "count=%i", &charsCount );

		mFontSize = fontSize;

		FileSystem::dirAddSlashAtEnd( mFilePath );

		{
			TextureFactory* TF = TextureFactory::instance();

			Image img( mFilePath + std::string( texFileName ) );

			if ( img.getChannels() == 1 ) {
				Image rgbaImg( img.getWidth(), img.getHeight(), 4 );
				size_t w = img.getWidth(), h = img.getHeight();

				for ( size_t y = 0; y < w; y++ ) {
					for ( size_t x = 0; x < h; x++ ) {
						rgbaImg.setPixel( x, y, Color( 255, 255, 255, img.getPixel( x, y ).r ) );
					}
				}

				Texture* tex = TF->loadFromPixels( rgbaImg.getPixelsPtr(), rgbaImg.getWidth(),
												   rgbaImg.getHeight(), rgbaImg.getChannels() );

				mPages[mFontSize].texture = tex;
			} else {
				Texture* tex = TF->loadFromPixels( img.getPixelsPtr(), img.getWidth(),
												   img.getHeight(), img.getChannels() );
				mPages[mFontSize].texture = tex;
			}

			if ( NULL != mPages[mFontSize].texture ) {
				mPages[mFontSize].texture->setFilter( Texture::Filter::Nearest );
				mPages[mFontSize].texture->setCoordinateType( Texture::CoordinateType::Pixels );
			}
		}

		GlyphTable& glyphs = mPages[mFontSize].glyphs;

		size_t firstLine = 4;
		size_t lastLine = firstLine + charsCount;

		if ( lastLine > lines.size() )
			lastLine = lines.size();

		int charId, charX, charY, charWidth, charHeight, charOffsetX, charOffsetY, charAdvanceX;

		for ( size_t i = firstLine; i < lastLine; i++ ) {
			sscanf( lines[i].c_str(),
					"char id=%i x=%i y=%i width=%i height=%i xoffset=%i yoffset=%i xadvance=%i",
					&charId, &charX, &charY, &charWidth, &charHeight, &charOffsetX, &charOffsetY,
					&charAdvanceX );

			Glyph& glyph = glyphs[charId];

			glyph.advance = charAdvanceX;
			glyph.bounds = Rectf( charOffsetX, -fontSize + charOffsetY, charWidth, charHeight );
			glyph.textureRect = Rect( charX, charY, charWidth, charHeight );
		}

		const Glyph& gl1 = getGlyph( '@', mFontSize, false, false );
		const Glyph& gl2 = getGlyph( '.', mFontSize, false, false );
		mIsMonospace = gl1.advance == gl2.advance;

		sendEvent( Event::Load );
	}

	return true;
}

bool FontBMFont::loadFromPack( Pack* pack, std::string filePackPath ) {
	cleanup();
	mFilePath = FileSystem::fileRemoveFileName( filePackPath );
	return loadFromStream( *pack->getFileStream( filePackPath ) );
}

bool FontBMFont::isMonospace() const {
	return mIsMonospace;
}

bool FontBMFont::isScalable() const {
	return false;
}

const FontBMFont::Info& FontBMFont::getInfo() const {
	return mInfo;
}

const Glyph& FontBMFont::getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold,
								   bool /*italic*/, Float outlineThickness, Float ) const {
	GlyphTable& glyphs = mPages[characterSize].glyphs;

	GlyphTable::const_iterator it = glyphs.find( codePoint );

	if ( it != glyphs.end() ) {
		return it->second;
	} else {
		glyphs[characterSize] = loadGlyph( codePoint, characterSize, bold, outlineThickness );
		return glyphs[characterSize];
	}
}

GlyphDrawable* FontBMFont::getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
											 bool bold, bool italic, Float outlineThickness,
											 const Float& ) const {
	GlyphDrawableTable& drawables = mPages[characterSize].drawables;
	auto it = drawables.find( codePoint );
	if ( it != drawables.end() ) {
		return it->second;
	} else {
		const Glyph& glyph = getGlyph( codePoint, characterSize, bold, italic, outlineThickness );
		const auto& page = mPages[characterSize];
		GlyphDrawable* region = GlyphDrawable::New(
			page.texture, glyph.textureRect, glyph.bounds.getSize(),
			String::format( "%s_%d_%u", mFontName.c_str(), characterSize, codePoint ) );
		region->setAdvance( glyph.bounds.getSize().getWidth() );
		drawables[codePoint] = region;
		return region;
	}
}

Glyph FontBMFont::loadGlyph( Uint32 codePoint, unsigned int characterSize, bool, Float ) const {
	Glyph glyph;

	GlyphTable& glyphs = mPages[mFontSize].glyphs;
	GlyphTable::const_iterator it = glyphs.find( codePoint );

	if ( it != glyphs.end() ) {
		const Glyph& oriGlyph = it->second;

		Float scale = (Float)characterSize / (Float)mFontSize;

		glyph.textureRect = oriGlyph.textureRect;
		glyph.bounds = oriGlyph.bounds * scale;
		glyph.advance = oriGlyph.advance * scale;
		glyph.size = oriGlyph.textureRect.getSize().asFloat() * scale;
		glyph.font = (Font*)this;
	}

	return glyph;
}

Float FontBMFont::getKerning( Uint32, Uint32, unsigned int, bool, bool, Float ) const {
	return 0;
}

Float FontBMFont::getLineSpacing( unsigned int characterSize ) const {
	return ( (Float)characterSize / mFontSize ) * mFontSize;
}

Uint32 FontBMFont::getFontHeight( const Uint32& characterSize ) const {
	return (Uint32)( (Float)characterSize / mFontSize ) * mFontSize;
}

Float FontBMFont::getUnderlinePosition( unsigned int ) const {
	return 0.f;
}

Float FontBMFont::getUnderlineThickness( unsigned int ) const {
	return 0.f;
}

Texture* FontBMFont::getTexture( unsigned int ) const {
	return mPages[mFontSize].texture;
}

bool FontBMFont::loaded() const {
	return !mPages.empty();
}

FontBMFont& FontBMFont::operator=( const FontBMFont& right ) {
	FontBMFont temp( right );
	std::swap( mInfo, temp.mInfo );
	std::swap( mPages, temp.mPages );
	std::swap( mFilePath, temp.mFilePath );
	std::swap( mFontSize, temp.mFontSize );
	return *this;
}

FontBMFont::Page::~Page() {
	for ( auto drawable : drawables )
		eeDelete( drawable.second );

	if ( NULL != texture && TextureFactory::existsSingleton() )
		TextureFactory::instance()->remove( texture->getTextureId() );
}

}} // namespace EE::Graphics
