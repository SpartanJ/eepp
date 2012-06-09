#ifndef HAIKUTTF_HKGLYPH_HPP
#define HAIKUTTF_HKGLYPH_HPP

#include "hkbase.hpp"

namespace HaikuTTF {

class hkGlyph {
	public:
		hkGlyph();

		~hkGlyph();

		int			Stored() const;
		void		Stored( int stored );

		FT_UInt		Index() const;
		void		Index( FT_UInt index );

		FT_Bitmap *	Pixmap();
		void		Pixmap( const FT_Bitmap& pixmap );

		int			MinX() const;
		void		MinX( int minx );

		int			MinY() const;
		void		MinY( int miny );

		int			MaxX() const;
		void		MaxX( int maxx );

		int			MaxY() const;
		void		MaxY( int maxy );

		int			OffsetY() const;
		void		OffsetY( int offsety );

		int			Advance() const;
		void		Advance( int advance );

		u16			Cached() const;
		void		Cached( u16 cached );

		void 		Flush();
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
		u16			mCached;
};

}

#endif
