#include <eepp/graphics/ctexturepackertex.hpp>
#include <eepp/graphics/cimage.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>

namespace EE { namespace Graphics { namespace Private {

cTexturePackerTex::cTexturePackerTex( const std::string& Name ) :
	mName(Name),
	mWidth(0),
	mHeight(0),
	mChannels(0),
	mX(0),
	mY(0),
	mLongestEdge(0),
	mArea(0),
	mFlipped(false),
	mPlaced(false),
	mLoadedInfo(false),
	mDisabled(false),
	mImg( NULL )
{
	if ( stbi_info( Name.c_str(), &mWidth, &mHeight, &mChannels ) ) {
		mArea 			= mWidth * mHeight;
		mLongestEdge 	= ( mWidth >= mHeight ) ? mWidth : mHeight;
		mLoadedInfo 	= true;
	}
}

cTexturePackerTex::cTexturePackerTex(cImage * Img , const std::string& Name ) :
	mName( Name ),
	mWidth( Img->Width() ),
	mHeight( Img->Height() ),
	mChannels( Img->Channels() ),
	mX(0),
	mY(0),
	mLongestEdge(0),
	mArea(0),
	mFlipped(false),
	mPlaced(false),
	mLoadedInfo(false),
	mDisabled(false),
	mImg( Img )
{
	mArea 			= mWidth * mHeight;
	mLongestEdge 	= ( mWidth >= mHeight ) ? mWidth : mHeight;
	mLoadedInfo 	= true;
}

void cTexturePackerTex::Place( Int32 x, Int32 y, bool flipped ) {
	if ( !mPlaced ) {
		mX 			= x;
		mY 			= y;
		mFlipped 	= flipped;
		mPlaced 	= true;
	}
}

cImage * cTexturePackerTex::Image() const {
	return mImg;
}

}}}

