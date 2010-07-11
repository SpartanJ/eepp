#include "hkfontmanager.hpp"

namespace HaikuTTF {

hkFontManager * hkFontManager::mSingleton = 0;

hkFontManager::hkFontManager() :
	mLibrary(NULL),
	mInitialized(0)
{
	Init();
}

hkFontManager::~hkFontManager() {
	Destroy();
}

int hkFontManager::Init() {
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

void hkFontManager::Destroy() {
	if ( mInitialized ) {
		if ( --mInitialized == 0 )
			FT_Done_FreeType( mLibrary );
	}
}

int hkFontManager::WasInit() {
	return mInitialized;
}

void hkFontManager::CloseFont( hkFont * Font ) {
	if ( NULL != Font ) {
		Font->CacheFlush();

		/*if ( Font->Face() ) {
			FT_Done_Face( Font->Face() );
			Font->Face( NULL );
		}*/

		hkSAFE_DELETE( Font );
	}
}

hkFont * hkFontManager::OpenFromMemory( const uint8_t* data, unsigned long size, int ptsize, long index, unsigned int glyphCacheSize ) {
    if ( Init() != 0)
		return NULL;

    FT_Face face = NULL;

	if ( FT_New_Memory_Face( mLibrary, reinterpret_cast<const FT_Byte*>(data), static_cast<FT_Long>(size), index, &face ) != 0  )
		return NULL;

	if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) != 0 )
		return NULL;

    hkFont * Font = new hkFont( this, glyphCacheSize );

	Font->Face( face );

    return FontPrepare( Font, ptsize );
}

hkFont * hkFontManager::OpenFromFile( const char* filename, int ptsize, long index, unsigned int glyphCacheSize ) {
    if ( Init() != 0)
		return NULL;

    FT_Face face;

    if ( FT_New_Face( mLibrary, filename, index, &face ) != 0 )
		return NULL;


    if ( FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0 )
		return NULL;

    hkFont * Font = new hkFont( this, glyphCacheSize );

	Font->Face( face );

    return FontPrepare( Font, ptsize );
}

hkFont * hkFontManager::FontPrepare( hkFont * font, int ptsize ) {
	FT_Error error;
	FT_Face face;
	FT_Fixed scale;

	face = font->Face();

	if ( FT_IS_SCALABLE( face ) ) {
		error = FT_Set_Char_Size( font->Face(), 0, ptsize * 64, 0, 0 );

		if( error ) {
	    	CloseFont( font );
	    	return NULL;
	  	}

	  	scale = face->size->metrics.y_scale;
	  	font->Ascent( FT_CEIL( FT_MulFix( face->ascender, scale) ) );
		font->Descent( FT_CEIL( FT_MulFix( face->descender, scale ) ) );
		font->Height( font->Ascent() - font->Descent() + 1 );
		font->LineSkip( FT_CEIL( FT_MulFix( face->height, scale ) ) );
		font->UnderlineOffset( FT_FLOOR( FT_MulFix( face->underline_position, scale ) ) );
		font->UnderlineHeight( FT_FLOOR( FT_MulFix( face->underline_thickness, scale ) ) );
	} else {
		if ( ptsize >= face->num_fixed_sizes )
			ptsize = face->num_fixed_sizes - 1;

		font->FontSizeFamily( ptsize );
		error = FT_Set_Pixel_Sizes( face, face->available_sizes[ptsize].height, face->available_sizes[ptsize].width );

	  	font->Ascent( face->available_sizes[ptsize].height );
	  	font->Descent( 0 );
	  	font->Height( face->available_sizes[ptsize].height );
	  	font->LineSkip( FT_CEIL( font->Ascent() ) );
	  	font->UnderlineOffset( FT_FLOOR( face->underline_position ) );
	  	font->UnderlineHeight( FT_FLOOR( face->underline_thickness ) );
	}

	if ( font->UnderlineHeight() < 1 )
		font->UnderlineHeight( 1 );

	font->FaceStyle( TTF_STYLE_NORMAL );

	if ( face->style_flags & FT_STYLE_FLAG_BOLD )
		font->FaceStyle( font->FaceStyle() | TTF_STYLE_BOLD );

	if ( face->style_flags & FT_STYLE_FLAG_ITALIC )
		font->FaceStyle( font->FaceStyle() | TTF_STYLE_ITALIC );

	font->Style( font->FaceStyle() );
	font->Outline( 0 );
	font->Kerning( 1 );
	font->GlyphOverhang( face->size->metrics.y_ppem / 10 );
	font->GlyphItalics( 0.207f );
	font->GlyphItalics( font->GlyphItalics() * font->Height() );

	return font;
}

}
