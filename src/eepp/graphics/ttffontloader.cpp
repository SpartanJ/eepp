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
	create();
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
	create();
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
	create();
}

TTFFontLoader::~TTFFontLoader() {
}

void TTFFontLoader::create() {
	mFont = TTFFont::New( mFontName );
}

void TTFFontLoader::start() {
	ObjectLoader::start();

	mFont->threadedLoading( mThreaded );

	if ( TTF_LT_PATH == mLoadType )
		loadFromPath();
	else if ( TTF_LT_MEM == mLoadType )
		loadFromMemory();
	else if ( TTF_LT_PACK == mLoadType )
		loadFromPack();

	mFontLoaded = true;

	if ( !mThreaded )
		update();
}

void TTFFontLoader::update() {
	if ( !mLoaded && mFontLoaded ) {
		mFont->updateLoading();

		setLoaded();
	}
}

const std::string& TTFFontLoader::getId() const {
	return mFontName;
}

void TTFFontLoader::loadFromPath() {
	mFont->load( mFilepath, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

void TTFFontLoader::loadFromMemory() {
	mFont->loadFromMemory( mData, mDataSize, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

void TTFFontLoader::loadFromPack() {
	mFont->loadFromPack( mPack, mFilepath, mSize, mStyle, mNumCharsToGen, mFontColor, mOutlineSize, mOutlineColor, mAddPixelSeparator );
}

Graphics::Font * TTFFontLoader::getFont() const {
	return mFont;
}

void TTFFontLoader::unload() {
	if ( mLoaded ) {
		TextureFactory::instance()->remove( mFont->getTexId() );

		FontManager::instance()->remove( mFont );

		reset();
	}
}

void TTFFontLoader::reset() {
	ObjectLoader::reset();

	mFontLoaded = false;
}

}}
