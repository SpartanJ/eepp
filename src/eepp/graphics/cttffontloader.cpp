#include <eepp/graphics/cttffontloader.hpp>
#include <eepp/graphics/cfontmanager.hpp>
#include <eepp/graphics/ctexturefactory.hpp>

namespace EE { namespace Graphics {

cTTFFontLoader::cTTFFontLoader( const std::string& FontName, const std::string& Filepath, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) :
	ObjectLoader( FontTTFLoader ),
	mLoadType( TTF_LT_PATH ),
	mFontName( FontName ),
	mFilepath( Filepath ),
	mSize( Size ),
	mStyle( Style ),
	mNumCharsToGen( NumCharsToGen ),
	mFontColor( FontColor ),
	mOutlineSize( OutlineSize ),
	mOutlineColor( OutlineColor ),
	mAddPixelSeparator( AddPixelSeparator ),
	mFontLoaded( false )
{
	Create();
}

cTTFFontLoader::cTTFFontLoader( const std::string& FontName, Pack * Pack, const std::string& FilePackPath, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) :
	ObjectLoader( FontTTFLoader ),
	mLoadType( TTF_LT_PACK ),
	mFontName( FontName ),
	mFilepath( FilePackPath ),
	mSize( Size ),
	mStyle( Style ),
	mNumCharsToGen( NumCharsToGen ),
	mFontColor( FontColor ),
	mOutlineSize( OutlineSize ),
	mOutlineColor( OutlineColor ),
	mAddPixelSeparator( AddPixelSeparator ),
	mPack( Pack ),
	mFontLoaded( false )
{
	Create();
}

cTTFFontLoader::cTTFFontLoader( const std::string& FontName, Uint8* TTFData, const unsigned int& TTFDataSize, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) :
	ObjectLoader( FontTTFLoader ),
	mLoadType( TTF_LT_MEM ),
	mFontName( FontName ),
	mSize( Size ),
	mStyle( Style ),
	mNumCharsToGen( NumCharsToGen ),
	mFontColor( FontColor ),
	mOutlineSize( OutlineSize ),
	mOutlineColor( OutlineColor ),
	mAddPixelSeparator( AddPixelSeparator ),
	mData( TTFData ),
	mDataSize( TTFDataSize ),
	mFontLoaded( false )
{
	Create();
}

cTTFFontLoader::~cTTFFontLoader() {
}

void cTTFFontLoader::Create() {
	mFont = cTTFFont::New( mFontName );
}

void cTTFFontLoader::Start() {
	ObjectLoader::Start();

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
	mFont->Load( mFilepath, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

void cTTFFontLoader::LoadFromMemory() {
	mFont->LoadFromMemory( mData, mDataSize, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

void cTTFFontLoader::LoadFromPack() {
	mFont->LoadFromPack( mPack, mFilepath, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

cFont * cTTFFontLoader::Font() const {
	return mFont;
}

void cTTFFontLoader::Unload() {
	if ( mLoaded ) {
		cTextureFactory::instance()->Remove( mFont->GetTexId() );

		cFontManager::instance()->Remove( mFont );

		Reset();
	}
}

void cTTFFontLoader::Reset() {
	ObjectLoader::Reset();

	mFontLoaded = false;
}

}}
