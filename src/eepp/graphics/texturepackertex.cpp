#include <eepp/graphics/image.hpp>
#include <eepp/graphics/texturepackertex.hpp>

namespace EE { namespace Graphics { namespace Private {

TexturePackerTex::TexturePackerTex( const std::string& Name,
									const Image::FormatConfiguration& imageFormatConfiguration ) :
	mName( Name ),
	mWidth( 0 ),
	mHeight( 0 ),
	mChannels( 0 ),
	mX( 0 ),
	mY( 0 ),
	mLongestEdge( 0 ),
	mArea( 0 ),
	mFlipped( false ),
	mPlaced( false ),
	mLoadedInfo( false ),
	mDisabled( false ),
	mImg( NULL ) {
	if ( Image::getInfo( Name.c_str(), &mWidth, &mHeight, &mChannels, imageFormatConfiguration ) ) {
		mArea = mWidth * mHeight;
		mLongestEdge = ( mWidth >= mHeight ) ? mWidth : mHeight;
		mLoadedInfo = true;
	}
}

TexturePackerTex::TexturePackerTex( EE::Graphics::Image* Img, const std::string& Name ) :
	mName( Name ),
	mWidth( Img->getWidth() ),
	mHeight( Img->getHeight() ),
	mChannels( Img->getChannels() ),
	mX( 0 ),
	mY( 0 ),
	mLongestEdge( 0 ),
	mArea( 0 ),
	mFlipped( false ),
	mPlaced( false ),
	mLoadedInfo( false ),
	mDisabled( false ),
	mImg( Img ) {
	mArea = mWidth * mHeight;
	mLongestEdge = ( mWidth >= mHeight ) ? mWidth : mHeight;
	mLoadedInfo = true;
}

void TexturePackerTex::place( Int32 x, Int32 y, bool flipped ) {
	if ( !mPlaced ) {
		mX = x;
		mY = y;
		mFlipped = flipped;
		mPlaced = true;
	}
}

EE::Graphics::Image* TexturePackerTex::getImage() const {
	return mImg;
}

}}} // namespace EE::Graphics::Private
