#ifndef EE_GRAPHICSPRIVATECTEXTUREPACKERTEX
#define EE_GRAPHICSPRIVATECTEXTUREPACKERTEX

#include <eepp/graphics/base.hpp>

namespace EE { namespace Graphics { namespace Private {

class cTexturePackerTex {
	public:
		cTexturePackerTex( const std::string& Name );

		void 					Place( Int32 x, Int32 y, bool flipped );

		inline const std::string& Name() const				{ return mName; }

		inline const Int32& 	X() const					{ return mX; }

		inline const Int32& 	Y() const					{ return mY; }

		inline const Int32&		Channels()					{ return mChannels; }

		inline void				X( const Int32& x )			{ mX = x; }

		inline void				Y( const Int32& y )			{ mY = y; }

		inline const Int32& 	Width() const				{ return mWidth; }

		inline const Int32& 	Height() const				{ return mHeight; }

		inline void 			Width( const Int32& W )		{ mWidth = W; }

		inline void 			Height( const Int32& H )	{ mHeight = H; }

		inline const bool&		LoadedInfo() const			{ return mLoadedInfo; }

		inline const Int32&		Area() const				{ return mArea; }

		inline const bool&		Placed() const				{ return mPlaced; }

		inline const bool&		Flipped() const				{ return mFlipped; }

		inline const Int32&		LongestEdge() const			{ return mLongestEdge; }

		inline const bool&		Disabled() const 			{ return mDisabled; }

		inline void				Disabled( const bool& d ) 	{ mDisabled = d; }

		inline const Int32& 	DestWidth() const			{ return mDestWidth; }

		inline const Int32& 	DestHeight() const			{ return mDestHeight; }

		inline void 			DestWidth( const Int32& W )	{ mDestWidth = W; }

		inline void 			DestHeight( const Int32& H ){ mDestHeight = H; }

		inline const Int32& 	OffsetX() const				{ return mOffsetX; }

		inline const Int32& 	OffsetY() const				{ return mOffsetY; }

		inline void 			OffsetX( const Int32& offx ){ mOffsetX = offx; }

		inline void 			OffsetY( const Int32& offy ){ mDestHeight = offy; }
	protected:
		std::string mName;
		Int32 		mWidth;
		Int32 		mHeight;
		Int32		mChannels;
		Int32 		mX;
		Int32 		mY;
		Int32 		mLongestEdge;
		Int32 		mArea;
		Int32		mDestWidth;
		Int32		mDestHeight;
		Int32		mOffsetX;
		Int32		mOffsetY;
		bool  		mFlipped;
		bool  		mPlaced;
		bool		mLoadedInfo;
		bool		mDisabled;
};

}}}

#endif
