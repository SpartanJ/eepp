#include "ccursoral.hpp"
#include "cwindowal.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

cCursorAl::cCursorAl( cTexture * tex, const eeVector2i& hotspot, const std::string& name, cWindow * window ) :
	cCursor( tex, hotspot, name, window ),
	mCursor( NULL )
{
	Create();
}

cCursorAl::cCursorAl( cImage * img, const eeVector2i& hotspot, const std::string& name, cWindow * window ) :
	cCursor( img, hotspot, name, window ),
	mCursor( NULL )
{
	Create();
}

cCursorAl::cCursorAl( const std::string& path, const eeVector2i& hotspot, const std::string& name, cWindow * window ) :
	cCursor( path, hotspot, name, window ),
	mCursor( NULL )
{
	Create();
}

cCursorAl::~cCursorAl() {
	if ( NULL != mCursor )
		al_destroy_mouse_cursor( mCursor );
}

ALLEGRO_MOUSE_CURSOR * cCursorAl::GetCursor() const {
	return mCursor;
}

void cCursorAl::Create() {
	if ( NULL == mImage )
		return;

	int nbfl = al_get_new_bitmap_flags();

	al_set_new_bitmap_flags( ALLEGRO_MEMORY_BITMAP );

	Int32 W = mImage->Width();
	Int32 H = mImage->Height();
	int x, y, c = mImage->Channels();
	Uint8 * Ptr = mImage->GetPixels();

	ALLEGRO_BITMAP * cursor = al_create_bitmap( W , H );

	al_set_target_bitmap( cursor );

	if ( cursor && al_lock_bitmap( cursor, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY ) ) {
		eeUint Pos;

		for ( y = 0; y < H; y++ ) {
			for ( x = 0; x < W; x++ ) {
				Pos = ( x + y * W ) * c;

				if ( 4 == c )
					al_put_pixel( x, y, al_map_rgba( Ptr[Pos], Ptr[Pos+1], Ptr[Pos+2], Ptr[Pos+3] ) );
				else
					al_put_pixel( x, y, al_map_rgb( Ptr[Pos], Ptr[Pos+1], Ptr[Pos+2] ) );
			}
		}

		al_unlock_bitmap( cursor );

		mCursor = al_create_mouse_cursor( cursor, mHotSpot.x, mHotSpot.y );
	}

	if ( NULL != cursor )
		al_destroy_bitmap( cursor );

	al_set_new_bitmap_flags( nbfl );
	al_set_target_backbuffer( reinterpret_cast<cWindowAl*>( mWindow )->GetDisplay() );
}

}}}}

#endif
