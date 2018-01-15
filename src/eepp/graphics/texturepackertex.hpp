#ifndef EE_GRAPHICSPRIVATECTEXTUREPACKERTEX
#define EE_GRAPHICSPRIVATECTEXTUREPACKERTEX

#include <eepp/graphics/base.hpp>

namespace EE { namespace Graphics {

class Image;

namespace Private {

class TexturePackerTex {
	public:
		TexturePackerTex( const std::string& name );

		TexturePackerTex( EE::Graphics::Image * Img, const std::string& name );

		void 					place( Int32 x, Int32 y, bool flipped );

		inline const std::string& name() const				{ return mName; }

		inline const Int32& 	x() const					{ return mX; }

		inline const Int32& 	y() const					{ return mY; }

		inline const Int32&		channels()					{ return mChannels; }

		inline void				x( const Int32& x )			{ mX = x; }

		inline void				y( const Int32& y )			{ mY = y; }

		inline const Int32& 	width() const				{ return mWidth; }

		inline const Int32& 	height() const				{ return mHeight; }

		inline void 			width( const Int32& W )		{ mWidth = W; }

		inline void 			height( const Int32& H )	{ mHeight = H; }

		inline const bool&		loadedInfo() const			{ return mLoadedInfo; }

		inline const Int32&		area() const				{ return mArea; }

		inline const bool&		placed() const				{ return mPlaced; }

		inline void placed( bool placed )					{ mPlaced = placed; }

		inline const bool&		flipped() const				{ return mFlipped; }

		inline const Int32&		longestEdge() const			{ return mLongestEdge; }

		inline const bool&		disabled() const 			{ return mDisabled; }

		inline void				disabled( const bool& d ) 	{ mDisabled = d; }

		inline const Int32& 	destWidth() const			{ return mDestWidth; }

		inline const Int32& 	destHeight() const			{ return mDestHeight; }

		inline void 			destWidth( const Int32& W )	{ mDestWidth = W; }

		inline void 			destHeight( const Int32& H ){ mDestHeight = H; }

		inline const Int32& 	offsetX() const				{ return mOffsetX; }

		inline const Int32& 	offsetY() const				{ return mOffsetY; }

		inline void 			offsetX( const Int32& offx ){ mOffsetX = offx; }

		inline void 			offsetY( const Int32& offy ){ mDestHeight = offy; }

		EE::Graphics::Image *	getImage() const;
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
		EE::Graphics::Image *	mImg;
};

}}}

#endif
