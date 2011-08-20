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
	Flush();
}

void hkGlyph::Flush() {
	mStored	= 0;
	mIndex	= 0;
	mCached	= 0;

	hkSAFE_FREE( mPixmap.buffer );
}

int hkGlyph::Stored() const {
	return mStored;
}

void hkGlyph::Stored( int stored ) {
	mStored = stored;
}

FT_UInt hkGlyph::Index() const {
	return mIndex;
}

void hkGlyph::Index( FT_UInt index ) {
	mIndex = index;
}

FT_Bitmap *	hkGlyph::Pixmap() {
	return &mPixmap;
}

void hkGlyph::Pixmap( const FT_Bitmap& pixmap ) {
	mPixmap = pixmap;
}

int	hkGlyph::MinX() const {
	return mMinX;
}

void hkGlyph::MinX( int minx ) {
	mMinX = minx;
}

int	hkGlyph::MinY() const {
	return mMinY;
}

void hkGlyph::MinY( int miny ) {
	mMinY = miny;
}

int	hkGlyph::MaxX() const {
	return mMaxX;
}

void hkGlyph::MaxX( int maxx ) {
	mMaxX = maxx;
}

int hkGlyph::MaxY() const {
	return mMaxY;
}

void hkGlyph::MaxY( int maxy ) {
	mMaxY = maxy;
}

int	hkGlyph::OffsetY() const {
	return mOffsetY;
}

void hkGlyph::OffsetY( int offsety ) {
	mOffsetY = offsety;
}

int	hkGlyph::Advance() const {
	return mAdvance;
}

void hkGlyph::Advance( int advance ) {
	mAdvance = advance;
}

u16	hkGlyph::Cached() const {
	return mCached;
}

void hkGlyph::Cached( u16 cached ) {
	mCached = cached;
}

}
