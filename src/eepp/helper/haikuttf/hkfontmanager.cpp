#include "hkfontmanager.hpp"

namespace HaikuTTF {

hkFontManager * hkFontManager::mSingleton = NULL;

hkFontManager * hkFontManager::exists() {
	return mSingleton;
}

hkFontManager * hkFontManager::instance() {
	if ( mSingleton == NULL ) {
		mSingleton = hkNew( hkFontManager, () );
	}

	return mSingleton;
}

void hkFontManager::destroySingleton() {
	hkSAFE_DELETE( mSingleton );
}

hkFontManager::hkFontManager() :
	mLibrary(NULL),
	mInitialized(0)
{
	init();
}

hkFontManager::~hkFontManager() {
	destroy();
}

int hkFontManager::init() {
	int status = 0;

	if ( !mInitialized ) {
		FT_Error error = FT_Init_FreeType( &mLibrary );

		if ( error )
			status = -1;
	}

	if ( status == 0 )
		++mInitialized;

	return status;
}

void hkFontManager::destroy() {
	if ( mInitialized ) {
		if ( --mInitialized == 0 ) {
			FT_Done_FreeType( mLibrary );
		}
	}
}

int hkFontManager::wasInit() {
	return mInitialized;
}

void hkFontManager::closeFont( hkFont * Font ) {
	hkSAFE_DELETE( Font );
}

hkFont * hkFontManager::openFromMemory( const u8* data, unsigned long size, int ptsize, long index, unsigned int glyphCacheSize ) {
	if ( init() != 0 )
		return NULL;

	FT_Face face = NULL;

	mutexLock();

	if ( FT_New_Memory_Face( mLibrary, reinterpret_cast<const FT_Byte*>(data), static_cast<FT_Long>(size), index, &face ) != 0  )
		return NULL;

	if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) != 0 )
		return NULL;

	mutexUnlock();

	hkFont * Font = hkNew( hkFont, ( this, glyphCacheSize ) );

	Font->face( face );

	return fontPrepare( Font, ptsize );
}

hkFont * hkFontManager::openFromFile( const char* filename, int ptsize, long index, unsigned int glyphCacheSize ) {
	if ( init() != 0 )
		return NULL;

	FT_Face face;

	mutexLock();

	if ( FT_New_Face( mLibrary, filename, index, &face ) != 0 )
		return NULL;

	if ( FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0 )
		return NULL;

	mutexUnlock();

	hkFont * Font = hkNew( hkFont, ( this, glyphCacheSize ) );

	Font->face( face );

	return fontPrepare( Font, ptsize );
}

hkFont * hkFontManager::fontPrepare( hkFont * font, int ptsize ) {
	FT_Error error;
	FT_Face face;
	FT_Fixed scale;

	face = font->face();

	if ( FT_IS_SCALABLE( face ) ) {
		mutexLock();

		error = FT_Set_Char_Size( font->face(), 0, ptsize * 64, 0, 0 );

		if( error ) {
			closeFont( font );

			mutexUnlock();

			return NULL;
		}

		mutexUnlock();

		scale = face->size->metrics.y_scale;
		font->ascent( FT_CEIL( FT_MulFix( face->ascender, scale) ) );
		font->descent( FT_CEIL( FT_MulFix( face->descender, scale ) ) );
		font->height( font->ascent() - font->descent() + 1 );
		font->lineSkip( FT_CEIL( FT_MulFix( face->height, scale ) ) );
		font->underlineOffset( FT_FLOOR( FT_MulFix( face->underline_position, scale ) ) );
		font->underlineHeight( FT_FLOOR( FT_MulFix( face->underline_thickness, scale ) ) );
	} else {
		if ( ptsize >= face->num_fixed_sizes )
			ptsize = face->num_fixed_sizes - 1;

		font->fontSizeFamily( ptsize );
		error = FT_Set_Pixel_Sizes( face, face->available_sizes[ptsize].height, face->available_sizes[ptsize].width );

		font->ascent( face->available_sizes[ptsize].height );
		font->descent( 0 );
		font->height( face->available_sizes[ptsize].height );
		font->lineSkip( FT_CEIL( font->ascent() ) );
		font->underlineOffset( FT_FLOOR( face->underline_position ) );
		font->underlineHeight( FT_FLOOR( face->underline_thickness ) );
	}

	if ( font->underlineHeight() < 1 )
		font->underlineHeight( 1 );

	font->faceStyle( HK_TTF_STYLE_NORMAL );

	if ( face->style_flags & FT_STYLE_FLAG_BOLD )
		font->faceStyle( font->faceStyle() | HK_TTF_STYLE_BOLD );

	if ( face->style_flags & FT_STYLE_FLAG_ITALIC )
		font->faceStyle( font->faceStyle() | HK_TTF_STYLE_ITALIC );

	font->style( font->faceStyle() );
	font->outline( 0 );
	font->kerning( 1 );
	font->glyphOverhang( face->size->metrics.y_ppem / 10 );
	font->glyphItalics( 0.207f );
	font->glyphItalics( font->glyphItalics() * font->height() );

	return font;
}

void hkFontManager::mutexLock() {
	lock();
}

void hkFontManager::mutexUnlock() {
	unlock();
}

FT_Library hkFontManager::getLibrary() const {
	return mLibrary;
}

}
