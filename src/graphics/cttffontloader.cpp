#include "cttffontloader.hpp"

namespace EE { namespace Graphics {

cTTFFontLoader::cTTFFontLoader( const std::string& FontName, const std::string& Filepath, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor ) :
	cObjectLoader( FontTTFLoader ),
	mLoadType( TTF_LT_PATH ),
	mFontName( FontName ),
	mFilepath( Filepath ),
	mSize( Size ),
	mStyle( Style ),
	mVerticalDraw( VerticalDraw ),
	mNumCharsToGen( NumCharsToGen ),
	mFontColor( FontColor ),
	mOutlineSize( OutlineSize ),
	mOutlineColor( OutlineColor ),
	mFontLoaded( false )
{
}

cTTFFontLoader::cTTFFontLoader( const std::string& FontName, cPack * Pack, const std::string& FilePackPath, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor ) :
	cObjectLoader( FontTTFLoader ),
	mLoadType( TTF_LT_PACK ),
	mFontName( FontName ),
	mFilepath( FilePackPath ),
	mSize( Size ),
	mStyle( Style ),
	mVerticalDraw( VerticalDraw ),
	mNumCharsToGen( NumCharsToGen ),
	mFontColor( FontColor ),
	mOutlineSize( OutlineSize ),
	mOutlineColor( OutlineColor ),
	mPack( Pack ),
	mFontLoaded( false )
{
}

cTTFFontLoader::cTTFFontLoader( const std::string& FontName, Uint8* TTFData, const eeUint& TTFDataSize, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor ) :
	cObjectLoader( FontTTFLoader ),
	mLoadType( TTF_LT_MEM ),
	mFontName( FontName ),
	mSize( Size ),
	mStyle( Style ),
	mVerticalDraw( VerticalDraw ),
	mNumCharsToGen( NumCharsToGen ),
	mFontColor( FontColor ),
	mOutlineSize( OutlineSize ),
	mOutlineColor( OutlineColor ),
	mData( TTFData ),
	mDataSize( TTFDataSize ),
	mFontLoaded( false )
{
}

cTTFFontLoader::~cTTFFontLoader() {
}

void cTTFFontLoader::Start() {
	cObjectLoader::Start();

	mFont = eeNew( cTTFFont, ( mFontName ) );

	mFont->ThreadedLoading( mThreaded );

	if ( TTF_LT_PATH == mLoadType )
		LoadFromPath();
	else if ( TTF_LT_MEM == mLoadType )
		LoadFromMemory();
	else if ( TTF_LT_PACK == mLoadType )
		LoadFromPack();

	mFontLoaded = true;

	if ( !mThreaded )
		Update();
}

void cTTFFontLoader::Update() {
	if ( !mLoaded && mFontLoaded ) {
		mFont->UpdateLoading();

		SetLoaded();
	}
}

const std::string& cTTFFontLoader::Id() const {
	return mFontName;
}

void cTTFFontLoader::LoadFromPath() {
	mFont->Load( mFilepath, mSize, mStyle, mVerticalDraw, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor );
}

void cTTFFontLoader::LoadFromMemory() {
	mFont->LoadFromMemory( mData, mDataSize, mSize, mStyle, mVerticalDraw, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor );
}

void cTTFFontLoader::LoadFromPack() {
	mFont->LoadFromPack( mPack, mFilepath, mSize, mStyle, mVerticalDraw, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor );
}

cFont * cTTFFontLoader::Font() const {
	return mFont;
}

}}
