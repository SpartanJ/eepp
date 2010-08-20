#ifndef EE_GRAPHICSPRIVATECTEXTUREPACKERNODE
#define EE_GRAPHICSPRIVATECTEXTUREPACKERNODE

#include "base.hpp"

namespace EE { namespace Graphics { namespace Private {

class cTexturePackerNode {
	public:
		cTexturePackerNode( Int32 x, Int32 y, Int32 width, Int32 height );

		bool 					Fits( Int32 wid, Int32 hit, Int32 &edgeCount, const bool& AllowFlipping = false ) const;

		void 					GetRect( eeRecti &r ) const;

		void 					Validate( cTexturePackerNode * n );

		bool 					Merge( const cTexturePackerNode &n );

		cTexturePackerNode * 	GetNext() const;

		inline void			SetNext( cTexturePackerNode * Next ) { mNext = Next; }

		inline const Int32& 	X() const 		{ return mX; }

		inline const Int32& 	Y() const 		{ return mY; }

		inline void			X( const Int32& x ) { mX = x; }

		inline void			Y( const Int32& y ) { mY = y; }

		inline const Int32& 	Width() const 	{ return mWidth; }

		inline const Int32& 	Height() const	{ return mHeight; }

		inline void 			Width( const Int32& W ) { mWidth = W; }

		inline void 			Height( const Int32& H ) { mHeight = H; }
	protected:
		cTexturePackerNode * 	mNext;
		Int32 					mX;
		Int32 					mY;
		Int32 					mWidth;
		Int32 					mHeight;
}; 

}}}

#endif
