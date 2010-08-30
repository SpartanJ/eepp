#ifndef EE_GRAPHICSPRIVATECTEXTUREPACKERTEX
#define EE_GRAPHICSPRIVATECTEXTUREPACKERTEX

#include "base.hpp"

namespace EE { namespace Graphics { namespace Private {

class cTexturePackerTex {
	public:
		cTexturePackerTex( const std::string& Name );

		void 					Place( Int32 x, Int32 y, bool flipped );

		inline const std::string& Name() const	{ return mName; }

		inline const Int32& 	X() const 			{ return mX; }

		inline const Int32& 	Y() const 			{ return mY; }

		inline const Int32&	Channels()			{ return mChannels; }

		inline void			X( const Int32& x ) { mX = x; }

		inline void			Y( const Int32& y ) { mY = y; }

		inline const Int32& 	Width() const		{ return mWidth; }

		inline const Int32& 	Height() const		{ return mHeight; }

		inline void 			Width( const Int32& W ) { mWidth = W; }

		inline void 			Height( const Int32& H ) { mHeight = H; }

		inline const bool&	LoadedInfo() const	{ return mLoadedInfo; }

		inline const Int32&	Area() const		{ return mArea; }

		inline const bool&	Placed() const		{ return mPlaced; }

		inline const bool&	Flipped() const	{ return mFlipped; }

		inline const Int32&	LongestEdge() const { return mLongestEdge; }

		inline const bool& 	Disabled() const 			{ return mDisabled; }

		inline void			Disabled( const bool& d ) 	{ mDisabled = d; }
	protected:
		std::string mName;
		Int32 		mWidth;
		Int32 		mHeight;
		Int32		mChannels;
		Int32 		mX;
		Int32 		mY;
		Int32 		mLongestEdge;
		Int32 		mArea;
		bool  		mFlipped;
		bool  		mPlaced;
		bool		mLoadedInfo;
		bool		mDisabled;
};

}}}

#endif
