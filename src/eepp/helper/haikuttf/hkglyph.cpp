#include "hkglyph.hpp"

namespace HaikuTTF {

hkGlyph::hkGlyph() :
	mStored(0),
	mIndex(0),
	mMinX(0),
	mMaxX(0),
	mMinY(0),
	mMaxY(0),
	mOffsetY(0),
	mAdvance(0),
	mCached(0)
{
	memset( &mPixmap, 0, sizeof(mPixmap) );
}

hkGlyph::~hkGlyph() {
	flush();
}

void hkGlyph::flush() {
	mStored	= 0;
	mIndex	= 0;
	mCached	= 0;

	hkSAFE_FREE( mPixmap.buffer );
}

int hkGlyph::stored() const {
	return mStored;
}

void hkGlyph::stored( int stored ) {
	mStored = stored;
}

FT_UInt hkGlyph::index() const {
	return mIndex;
}

void hkGlyph::index( FT_UInt index ) {
	mIndex = index;
}

FT_Bitmap *	hkGlyph::pixmap() {
	return &mPixmap;
}

void hkGlyph::pixmap( const FT_Bitmap& pixmap ) {
	mPixmap = pixmap;
}

int	hkGlyph::minX() const {
	return mMinX;
}

void hkGlyph::minX( int minx ) {
	mMinX = minx;
}

int	hkGlyph::minY() const {
	return mMinY;
}

void hkGlyph::minY( int miny ) {
	mMinY = miny;
}

int	hkGlyph::maxX() const {
	return mMaxX;
}

void hkGlyph::maxX( int maxx ) {
	mMaxX = maxx;
}

int hkGlyph::maxY() const {
	return mMaxY;
}

void hkGlyph::maxY( int maxy ) {
	mMaxY = maxy;
}

int	hkGlyph::offsetY() const {
	return mOffsetY;
}

void hkGlyph::offsetY( int offsety ) {
	mOffsetY = offsety;
}

int	hkGlyph::advance() const {
	return mAdvance;
}

void hkGlyph::advance( int advance ) {
	mAdvance = advance;
}

u16	hkGlyph::cached() const {
	return mCached;
}

void hkGlyph::cached( u16 cached ) {
	mCached = cached;
}

}
