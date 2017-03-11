#ifndef HAIKUTTF_HKGLYPH_HPP
#define HAIKUTTF_HKGLYPH_HPP

#include "hkbase.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_STROKER_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H

namespace HaikuTTF {

class hkGlyph {
	public:
		hkGlyph();

		~hkGlyph();

		int			stored() const;
		void		stored( int stored );

		FT_UInt		index() const;
		void		index( FT_UInt index );

		FT_Bitmap *	pixmap();
		void		pixmap( const FT_Bitmap& pixmap );

		int			minX() const;
		void		minX( int minx );

		int			minY() const;
		void		minY( int miny );

		int			maxX() const;
		void		maxX( int maxx );

		int			maxY() const;
		void		maxY( int maxy );

		int			offsetY() const;
		void		offsetY( int offsety );

		int			advance() const;
		void		advance( int advance );

		u16			cached() const;
		void		cached( u16 cached );

		void 		flush();
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
