#include <eepp/graphics/texturefontloader.hpp>
#include <eepp/graphics/fontmanager.hpp>

namespace EE { namespace Graphics {

TextureFontLoader::TextureFontLoader( const std::string FontName, TextureLoader * TexLoader, const unsigned int& StartChar, const unsigned int& Spacing, const unsigned int& TexColumns, const unsigned int& TexRows, const Uint16& NumChars ) :
	ObjectLoader( FontTexLoaderType ),
	mLoadType( TEF_LT_TEX ),
	mFontName( FontName ),
	mStartChar( StartChar ),
	mSpacing( Spacing ),
	mTexColumns( TexColumns ),
	mTexRows( TexRows ),
	mNumChars( NumChars ),
	mTexLoaded( false ),
	mFontLoaded( false )
{
	mTexLoader = TexLoader;
}

TextureFontLoader::TextureFontLoader( const std::string FontName, TextureLoader * TexLoader, const std::string& CoordinatesDatPath ) :
	ObjectLoader( FontTexLoaderType ),
	mLoadType( TEF_LT_PATH ),
	mFontName( FontName ),
	mFilepath( CoordinatesDatPath ),
	mTexLoaded( false ),
	mFontLoaded( false )
{
	mTexLoader = TexLoader;
}

TextureFontLoader::TextureFontLoader( const std::string FontName, TextureLoader * TexLoader, Pack * Pack, const std::string& FilePackPath ) :
	ObjectLoader( FontTexLoaderType ),
	mLoadType( TEF_LT_PACK ),
	mFontName( FontName ),
	mFilepath( FilePackPath ),
	mPack( Pack ),
	mTexLoaded( false ),
	mFontLoaded( false )
{
	mTexLoader = TexLoader;
}

TextureFontLoader::TextureFontLoader( const std::string FontName, TextureLoader * TexLoader, const char* CoordData, const Uint32& CoordDataSize ) :
	ObjectLoader( FontTexLoaderType ),
	mLoadType( TEF_LT_MEM ),
	mFontName( FontName ),
	mData( CoordData ),
	mDataSize( CoordDataSize ),
	mTexLoaded( false ),
	mFontLoaded( false )
{
	mTexLoader = TexLoader;
}

TextureFontLoader::~TextureFontLoader() {
	eeSAFE_DELETE( mTexLoader );
}

void TextureFontLoader::start() {
	ObjectLoader::start();

	mTexLoader->threaded( false );

	if ( !mThreaded ) {
		update();
	}
}

void TextureFontLoader::update() {
	if ( !mLoaded ) {
		if ( !mTexLoaded ) {
			mTexLoader->load();

			mTexLoader->update();

			mTexLoaded = mTexLoader->isLoaded();
		}

		if ( mTexLoaded && !mFontLoaded ) {
			loadFont();
		}

		if ( mFontLoaded ) {
			setLoaded();
		}
	}
}

const std::string& TextureFontLoader::getId() const {
	return mFontName;
}

void TextureFontLoader::loadFromPath() {
	mFont->load( mTexLoader->getId(), mFilepath );
}

void TextureFontLoader::loadFromMemory() {
	mFont->loadFromMemory( mTexLoader->getId(), mData, mDataSize );
}

void TextureFontLoader::loadFromPack() {
	mFont->loadFromPack( mTexLoader->getId(), mPack, mFilepath );
}

void TextureFontLoader::loadFromTex() {
	mFont->load( mTexLoader->getId(), mStartChar, mSpacing, mTexColumns, mTexRows, mNumChars );
}

void TextureFontLoader::loadFont() {
	mFont = TextureFont::New( mFontName );

	if ( TEF_LT_PATH == mLoadType )
		loadFromPath();
	else if ( TEF_LT_MEM == mLoadType )
		loadFromMemory();
	else if ( TEF_LT_PACK == mLoadType )
		loadFromPack();
	else if ( TEF_LT_TEX == mLoadType )
		loadFromTex();

	mFontLoaded = true;
}

Graphics::Font * TextureFontLoader::getFont() const {
	return mFont;
}

void TextureFontLoader::unload() {
	if ( mLoaded ) {
		mTexLoader->unload();

		FontManager::instance()->remove( mFont );

		reset();
	}
}

void TextureFontLoader::reset() {
	ObjectLoader::reset();

	mFont			= NULL;
	mTexLoaded		= false;
	mFontLoaded		= false;
}

}}
