#include <eepp/graphics/fonttruetypeloader.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/pack.hpp>

namespace EE { namespace Graphics {

FontTrueTypeLoader * FontTrueTypeLoader::New( const std::string& FontName, const std::string& Filepath ) {
	return eeNew( FontTrueTypeLoader, ( FontName, Filepath ) );
}

FontTrueTypeLoader * FontTrueTypeLoader::New( const std::string& FontName, Pack * Pack, const std::string& FilePackPath ) {
	return eeNew( FontTrueTypeLoader, ( FontName, Pack, FilePackPath ) );
}

FontTrueTypeLoader * FontTrueTypeLoader::New( const std::string& FontName, Uint8* TTFData, const unsigned int& TTFDataSize ) {
	return eeNew( FontTrueTypeLoader, ( FontName, TTFData, TTFDataSize ) );
}

FontTrueTypeLoader * FontTrueTypeLoader::New( const std::string& FontName, IOStream& stream ) {
	return eeNew( FontTrueTypeLoader, ( FontName, stream ) );
}

FontTrueTypeLoader::FontTrueTypeLoader( const std::string& FontName, const std::string& Filepath ) :
	ObjectLoader( FontLoader ),
	mLoadType( TTF_LT_PATH ),
	mFontName( FontName ),
	mFilepath( Filepath ),
	mFontLoaded( false )
{
	create();
}

FontTrueTypeLoader::FontTrueTypeLoader( const std::string& FontName, System::Pack * Pack, const std::string& FilePackPath ) :
	ObjectLoader( FontLoader ),
	mLoadType( TTF_LT_PACK ),
	mFontName( FontName ),
	mFilepath( FilePackPath ),
	mPack( Pack ),
	mFontLoaded( false )
{
	create();
}

FontTrueTypeLoader::FontTrueTypeLoader( const std::string& FontName, Uint8* TTFData, const unsigned int& TTFDataSize ) :
	ObjectLoader( FontLoader ),
	mLoadType( TTF_LT_MEM ),
	mFontName( FontName ),
	mData( TTFData ),
	mDataSize( TTFDataSize ),
	mFontLoaded( false )
{
	create();
}

FontTrueTypeLoader::FontTrueTypeLoader( const std::string& FontName, IOStream& stream ) :
	ObjectLoader( FontLoader ),
	mLoadType( TTF_LT_STREAM ),
	mFontName( FontName ),
	mIOStream( &stream ),
	mFontLoaded( false )
{
	create();
}

FontTrueTypeLoader::~FontTrueTypeLoader() {
}

void FontTrueTypeLoader::create() {
	mFont = FontTrueType::New( mFontName );
}

void FontTrueTypeLoader::start() {
	ObjectLoader::start();

	if ( TTF_LT_PATH == mLoadType )
		loadFromFile();
	else if ( TTF_LT_MEM == mLoadType )
		loadFromMemory();
	else if ( TTF_LT_PACK == mLoadType )
		loadFromPack();
	else if ( TTF_LT_STREAM == mLoadType )
		loadFromStream();

	mFontLoaded = true;

	if ( !mThreaded )
		update();
}

void FontTrueTypeLoader::update() {
	if ( !mLoaded && mFontLoaded ) {
		setLoaded();
	}
}

const std::string& FontTrueTypeLoader::getId() const {
	return mFontName;
}

void FontTrueTypeLoader::loadFromFile() {
	mFont->loadFromFile( mFilepath );
}

void FontTrueTypeLoader::loadFromMemory() {
	mFont->loadFromMemory( mData, mDataSize );
}

void FontTrueTypeLoader::loadFromPack() {
	mFont->loadFromPack( mPack, mFilepath );
}

void FontTrueTypeLoader::loadFromStream() {
	mFont->loadFromStream( *mIOStream );
}

Graphics::Font * FontTrueTypeLoader::getFont() const {
	return mFont;
}

void FontTrueTypeLoader::unload() {
	if ( mLoaded ) {
		FontManager::instance()->remove( mFont );

		reset();
	}
}

void FontTrueTypeLoader::reset() {
	ObjectLoader::reset();

	mFontLoaded = false;
}

}}
