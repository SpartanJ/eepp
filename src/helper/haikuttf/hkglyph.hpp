#ifndef HAIKUTTF_HKGLYPH_HPP
#define HAIKUTTF_HKGLYPH_HPP

#include "hkbase.hpp"

namespace HaikuTTF {

class hkGlyph {
	public:
		hkGlyph();

		~hkGlyph();

		inline int			Stored() const 					{	return mStored;			}
		inline void		Stored( int stored ) 				{	mStored = stored;			}

		inline FT_UInt		Index() const 						{	return mIndex;				}
		inline void		Index( FT_UInt index ) 				{	mIndex = index;				}

		inline FT_Bitmap *	Pixmap()							{	return &mPixmap;			}
		inline void		Pixmap( const FT_Bitmap& pixmap ) 	{	mPixmap = pixmap;			}

		inline int			MinX() const 						{	return mMinX;				}
		inline void		MinX( int minx ) 					{	mMinX = minx;				}

		inline int			MinY() const 						{	return mMinY;				}
		inline void		MinY( int miny ) 					{	mMinY = miny;				}

		inline int			MaxX() const 						{	return mMaxX;				}
		inline void		MaxX( int maxx ) 					{	mMaxX = maxx;				}

		inline int			MaxY() const 						{	return mMaxY;				}
		inline void		MaxY( int maxy ) 					{	mMaxY = maxy;				}

		inline int			OffsetY() const 					{	return mOffsetY;			}
		inline void		OffsetY( int offsety ) 				{	mOffsetY = offsety;			}

		inline int			Advance() const 					{	return mAdvance;			}
		inline void		Advance( int advance ) 				{	mAdvance = advance;			}

		inline u16			Cached() const						{	return mCached;			}
		inline void		Cached( u16 cached ) 				{	mCached = cached;			}

		void 				Flush();
	protected:
		int			mStored;
		FT_UInt		mIndex;
		FT_Bitmap	mPixmap;
		int			mMinX;
		int			mMaxX;
		int			mMinY;
		int			mMaxY;
		int			mOffsetY;
		int			mAdvance;
		u16	mCached;
};

}

#endif
