#include <eepp/graphics/texturepackernode.hpp>

namespace EE { namespace Graphics { namespace Private {

TexturePackerNode::TexturePackerNode( Int32 x, Int32 y, Int32 width, Int32 height ) {
	mX 		= x;
	mY 		= y;
	mWidth 	= width;
	mHeight = height;
	mNext 	= NULL;
}

TexturePackerNode * TexturePackerNode::getNext() const {
	return mNext;
}

bool TexturePackerNode::fits( Int32 width, Int32 height, Int32 &edgeCount, const bool& AllowFlipping ) const {
	bool ret = false;

	edgeCount = 0;

	if ( width == mWidth  || height == mHeight || width == mHeight || height == mWidth ) {
		if ( width == mWidth ) {
			edgeCount++;
			if ( height == mHeight ) edgeCount++;
		}
		else if ( AllowFlipping && width == mHeight ) {
			edgeCount++;
			if ( height == mWidth ) edgeCount++;
		}
		else if ( AllowFlipping && height == mWidth ) {
			edgeCount++;
		}
		else if ( height == mHeight ) {
			edgeCount++;
		}
	}

	if ( width <= mWidth && height <= mHeight ) {
		ret = true;
	} else if ( AllowFlipping && height <= mWidth && width <= mHeight ) {
		ret = true;
	}

	return ret;
}

void TexturePackerNode::getRect( Rect &r ) const {
	r = Rect( mX, mY, mX + mWidth - 1, mY + mHeight - 1 );
}

void TexturePackerNode::validate( TexturePackerNode * n ) {
	Rect r1;
	Rect r2;
	getRect( r1 );
	n->getRect( r2 );
	eeASSERT( !r1.intersect(r2) );
}

bool TexturePackerNode::merge( const TexturePackerNode& n ) {
	bool ret = false;

	Rect r1;
	Rect r2;

	getRect( r1 );
	n.getRect( r2 );

	r1.Right++;
	r1.Bottom++;
	r2.Right++;
	r2.Bottom++;

	if ( r1.Left == r2.Left && r1.Right == r2.Right && r1.Top == r2.Bottom ) // if we share the top edge then..
	{
		mY 		= n.y();
		mHeight += n.height();
		ret 	= true;
	}
	else if ( r1.Left == r2.Left && r1.Right == r2.Right && r1.Bottom == r2.Top ) // if we share the bottom edge
	{
		mHeight += n.height();
		ret 	= true;
	}
	else if ( r1.Top == r2.Top && r1.Bottom == r2.Top && r1.Left == r2.Right ) // if we share the left edge
	{
		mX 		= n.x();
		mWidth += n.width();
		ret 	= true;
	}
	else if ( r1.Top == r2.Top && r1.Bottom == r2.Top && r1.Right == r2.Left ) // if we share the left edge
	{
		mWidth	+= n.width();
		ret 	= true;
	}

	return ret;
}

}}}
