#include "ctexturefontloader.hpp"
#include "cfontmanager.hpp"

namespace EE { namespace Graphics {

cTextureFontLoader::cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const eeUint& StartChar, const eeUint& Spacing, const bool& VerticalDraw, const eeUint& TexColumns, const eeUint& TexRows, const Uint16& NumChars ) :
	cObjectLoader( FontTexLoader ),
	mLoadType( TEF_LT_TEX ),
	mFontName( FontName ),
	mStartChar( StartChar ),
	mSpacing( Spacing ),
	mVerticalDraw( VerticalDraw ),
	mTexColumns( TexColumns ),
	mTexRows( TexRows ),
	mNumChars( NumChars ),
	mTexLoaded( false ),
	mFontLoaded( false )
{
	mTexLoader = TexLoader;
}

cTextureFontLoader::cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const std::string& CoordinatesDatPath, const bool& VerticalDraw ) :
	cObjectLoader( FontTexLoader ),
	mLoadType( TEF_LT_PATH ),
	mFontName( FontName ),
	mFilepath( CoordinatesDatPath ),
	mVerticalDraw( VerticalDraw ),
	mTexLoaded( false ),
	mFontLoaded( false )
{
	mTexLoader = TexLoader;
}

cTextureFontLoader::cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, cPack * Pack, const std::string& FilePackPath, const bool& VerticalDraw ) :
	cObjectLoader( FontTexLoader ),
	mLoadType( TEF_LT_PACK ),
	mFontName( FontName ),
	mFilepath( FilePackPath ),
	mVerticalDraw( VerticalDraw ),
	mPack( Pack ),
	mTexLoaded( false ),
	mFontLoaded( false )
{
	mTexLoader = TexLoader;
}

cTextureFontLoader::cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const Uint8* CoordData, const Uint32& CoordDataSize, const bool& VerticalDraw ) :
	cObjectLoader( FontTexLoader ),
	mLoadType( TEF_LT_MEM ),
	mFontName( FontName ),
	mVerticalDraw( VerticalDraw ),
	mData( CoordData ),
	mDataSize( CoordDataSize ),
	mTexLoaded( false ),
	mFontLoaded( false )
{
	mTexLoader = TexLoader;
}

cTextureFontLoader::~cTextureFontLoader() {
	eeSAFE_DELETE( mTexLoader );
}

void cTextureFontLoader::Start() {
	cObjectLoader::Start();

	mTexLoader->Threaded( false );

	if ( !mThreaded ) {
		Update();
	}
}

void cTextureFontLoader::Update() {
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

const std::string& cTextureFontLoader::Id() const {
	return mFontName;
}

void cTextureFontLoader::LoadFromPath() {
	mFont->Load( mTexLoader->Id(), mFilepath, mVerticalDraw );
}

void cTextureFontLoader::LoadFromMemory() {
	mFont->LoadFromMemory( mTexLoader->Id(), mData, mDataSize, mVerticalDraw );
}

void cTextureFontLoader::LoadFromPack() {
	mFont->LoadFromPack( mTexLoader->Id(), mPack, mFilepath, mVerticalDraw );
}

void cTextureFontLoader::LoadFromTex() {
	mFont->Load( mTexLoader->Id(), mStartChar, mSpacing, mVerticalDraw, mTexColumns, mTexRows, mNumChars );
}

void cTextureFontLoader::LoadFont() {
	mFont = eeNew( cTextureFont, ( mFontName ) );

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

cFont * cTextureFontLoader::Font() const {
	return mFont;
}

void cTextureFontLoader::Unload() {
	if ( mLoaded ) {
		mTexLoader->Unload();

		cFontManager::instance()->Remove( mFont );

		Reset();
	}
}

void cTextureFontLoader::Reset() {
	cObjectLoader::Reset();

	mFont			= NULL;
	mTexLoaded		= false;
	mFontLoaded		= false;
}

}}
