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

void TextureFontLoader::Start() {
	ObjectLoader::Start();

	mTexLoader->Threaded( false );

	if ( !mThreaded ) {
		Update();
	}
}

void TextureFontLoader::Update() {
	if ( !mLoaded ) {
		if ( !mTexLoaded ) {
			mTexLoader->Load();

			mTexLoader->Update();

			mTexLoaded = mTexLoader->IsLoaded();
		}

		if ( mTexLoaded && !mFontLoaded ) {
			LoadFont();
		}

		if ( mFontLoaded ) {
			SetLoaded();
		}
	}
}

const std::string& TextureFontLoader::Id() const {
	return mFontName;
}

void TextureFontLoader::LoadFromPath() {
	mFont->Load( mTexLoader->Id(), mFilepath );
}

void TextureFontLoader::LoadFromMemory() {
	mFont->LoadFromMemory( mTexLoader->Id(), mData, mDataSize );
}

void TextureFontLoader::LoadFromPack() {
	mFont->LoadFromPack( mTexLoader->Id(), mPack, mFilepath );
}

void TextureFontLoader::LoadFromTex() {
	mFont->Load( mTexLoader->Id(), mStartChar, mSpacing, mTexColumns, mTexRows, mNumChars );
}

void TextureFontLoader::LoadFont() {
	mFont = TextureFont::New( mFontName );

	if ( TEF_LT_PATH == mLoadType )
		LoadFromPath();
	else if ( TEF_LT_MEM == mLoadType )
		LoadFromMemory();
	else if ( TEF_LT_PACK == mLoadType )
		LoadFromPack();
	else if ( TEF_LT_TEX == mLoadType )
		LoadFromTex();

	mFontLoaded = true;
}

Graphics::Font * TextureFontLoader::Font() const {
	return mFont;
}

void TextureFontLoader::Unload() {
	if ( mLoaded ) {
		mTexLoader->Unload();

		FontManager::instance()->Remove( mFont );

		Reset();
	}
}

void TextureFontLoader::Reset() {
	ObjectLoader::Reset();

	mFont			= NULL;
	mTexLoaded		= false;
	mFontLoaded		= false;
}

}}
