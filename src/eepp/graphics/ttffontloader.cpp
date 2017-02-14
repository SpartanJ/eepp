#include <eepp/graphics/ttffontloader.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>

namespace EE { namespace Graphics {

TTFFontLoader::TTFFontLoader( const std::string& FontName, const std::string& Filepath, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) :
	ObjectLoader( FontTTFLoaderType ),
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

TTFFontLoader::TTFFontLoader( const std::string& FontName, Pack * Pack, const std::string& FilePackPath, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) :
	ObjectLoader( FontTTFLoaderType ),
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

TTFFontLoader::TTFFontLoader( const std::string& FontName, Uint8* TTFData, const unsigned int& TTFDataSize, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) :
	ObjectLoader( FontTTFLoaderType ),
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

TTFFontLoader::~TTFFontLoader() {
}

void TTFFontLoader::Create() {
	mFont = TTFFont::New( mFontName );
}

void TTFFontLoader::start() {
	ObjectLoader::start();

	mFont->ThreadedLoading( mThreaded );

	if ( TTF_LT_PATH == mLoadType )
		LoadFromPath();
	else if ( TTF_LT_MEM == mLoadType )
		LoadFromMemory();
	else if ( TTF_LT_PACK == mLoadType )
		LoadFromPack();

	mFontLoaded = true;

	if ( !mThreaded )
		update();
}

void TTFFontLoader::update() {
	if ( !mLoaded && mFontLoaded ) {
		mFont->UpdateLoading();

		setLoaded();
	}
}

const std::string& TTFFontLoader::Id() const {
	return mFontName;
}

void TTFFontLoader::LoadFromPath() {
	mFont->Load( mFilepath, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

void TTFFontLoader::LoadFromMemory() {
	mFont->LoadFromMemory( mData, mDataSize, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

void TTFFontLoader::LoadFromPack() {
	mFont->LoadFromPack( mPack, mFilepath, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

Graphics::Font * TTFFontLoader::Font() const {
	return mFont;
}

void TTFFontLoader::unload() {
	if ( mLoaded ) {
		TextureFactory::instance()->Remove( mFont->GetTexId() );

		FontManager::instance()->remove( mFont );

		reset();
	}
}

void TTFFontLoader::reset() {
	ObjectLoader::reset();

	mFontLoaded = false;
}

}}
