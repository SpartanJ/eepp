#ifndef EE_GRAPHICSPRIVATECTEXTUREPACKERNODE
#define EE_GRAPHICSPRIVATECTEXTUREPACKERNODE

#include <eepp/graphics/base.hpp>

namespace EE { namespace Graphics { namespace Private {

class TexturePackerNode {
	public:
		TexturePackerNode( Int32 x, Int32 y, Int32 width, Int32 height );

		bool 					fits( Int32 wid, Int32 hit, Int32 &edgeCount, const bool& AllowFlipping = false ) const;

		void 					getRect( Recti &r ) const;

		void 					validate( TexturePackerNode * n );

		bool 					merge( const TexturePackerNode &n );

		TexturePackerNode * 	getNext() const;

		inline void			setNext( TexturePackerNode * Next ) { mNext = Next; }

		inline const Int32& 	x() const 		{ return mX; }

		inline const Int32& 	y() const 		{ return mY; }

		inline void			x( const Int32& x ) { mX = x; }

		inline void			y( const Int32& y ) { mY = y; }

		inline const Int32& 	width() const 	{ return mWidth; }

		inline const Int32& 	height() const	{ return mHeight; }

		inline void 			width( const Int32& W ) { mWidth = W; }

		inline void 			height( const Int32& H ) { mHeight = H; }
	protected:
		TexturePackerNode * 	mNext;
		Int32 					mX;
		Int32 					mY;
		Int32 					mWidth;
		Int32 					mHeight;
}; 

}}}

#endif
