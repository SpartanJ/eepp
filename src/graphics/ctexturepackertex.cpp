#include "ctexturepackertex.hpp"
#include "../helper/SOIL/stb_image.h"

namespace EE { namespace Graphics { namespace Private {

cTexturePackerTex::cTexturePackerTex( const std::string& Name ) :
	mName(Name),
	mWidth(0),
	mHeight(0),
	mX(0),
	mY(0),
	mLongestEdge(0),
	mArea(0),
	mFlipped(false),
	mPlaced(false),
	mLoadedInfo(false),
	mDisabled(false)
{
	Int32 c;

	if ( stbi_info( Name.c_str(), &mWidth, &mHeight, &c ) ) {
		mArea 			= mWidth * mHeight;
		mLongestEdge 	= ( mWidth >= mHeight ) ? mWidth : mHeight;
		mLoadedInfo 	= true;
	}
}

void cTexturePackerTex::Place( Int32 x, Int32 y, bool flipped ) {
	if ( !mPlaced ) {
		mX 			= x;
		mY 			= y;
		mFlipped 	= flipped;
		mPlaced 	= true;
	}
}

}}}

