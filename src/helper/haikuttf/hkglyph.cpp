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
	memset( &mBitmap, 0, sizeof(mBitmap) );
	memset( &mPixmap, 0, sizeof(mPixmap) );
}

hkGlyph::~hkGlyph() {
	Flush();
}

void hkGlyph::Flush() {
	mStored = 0;
	mIndex = 0;

	hkSAFE_FREE( mBitmap.buffer );
	hkSAFE_FREE( mPixmap.buffer );

	mCached = 0;
}

}
