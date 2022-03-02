#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_STROKER_H
#include FT_TRUETYPE_TABLES_H
#include <cstdlib>
#include <cstring>

namespace {
// FreeType callbacks that operate on a IOStream
unsigned long read( FT_Stream rec, unsigned long offset, unsigned char* buffer,
					unsigned long count ) {
	IOStream* stream = static_cast<IOStream*>( rec->descriptor.pointer );
	if ( static_cast<unsigned long>( stream->seek( offset ) ) == offset ) {
		if ( count > 0 )
			return static_cast<unsigned long>(
				stream->read( reinterpret_cast<char*>( buffer ), count ) );
		else
			return 0;
	} else
		return count > 0 ? 0 : 1; // error code is 0 if we're reading, or nonzero if we're seeking
}

void close( FT_Stream ) {}

// Helper to intepret memory as a specific type
template <typename T, typename U> inline T reinterpret( const U& input ) {
	T output;
	std::memcpy( &output, &input, sizeof( U ) );
	return output;
}

// Combine outline thickness, boldness and font glyph index into a single 64-bit key
EE::Uint64 combine( float outlineThickness, bool bold, EE::Uint32 index ) {
	return ( static_cast<EE::Uint64>( reinterpret<EE::Uint32>( outlineThickness ) ) << 32 ) |
		   ( static_cast<EE::Uint64>( bold ) << 31 ) | index;
}

} // namespace

namespace EE { namespace Graphics {

FontTrueType* FontTrueType::New( const std::string& FontName ) {
	return eeNew( FontTrueType, ( FontName ) );
}

FontTrueType* FontTrueType::New( const std::string& FontName, const std::string& filename ) {
	FontTrueType* fontTrueType = New( FontName );
	fontTrueType->loadFromFile( filename );
	return fontTrueType;
}

FontTrueType::FontTrueType( const std::string& FontName ) :
	Font( FontType::TTF, FontName ),
	mLibrary( NULL ),
	mFace( NULL ),
	mStreamRec( NULL ),
	mStroker( NULL ),
	mRefCount( NULL ),
	mInfo(),
	mBoldAdvanceSameAsRegular( false ) {}

FontTrueType::~FontTrueType() {
	cleanup();
}

bool checkIsColorEmojiFont( const FT_Face& face ) {
	static const uint32_t tag = FT_MAKE_TAG( 'C', 'B', 'D', 'T' );
	unsigned long length = 0;
	FT_Load_Sfnt_Table( face, tag, 0, nullptr, &length );
	return length > 0;
}

bool FontTrueType::loadFromFile( const std::string& filename ) {
	if ( !FileSystem::fileExists( filename ) &&
		 PackManager::instance()->isFallbackToPacksActive() ) {
		std::string path( filename );
		Pack* pack = PackManager::instance()->exists( path );

		if ( NULL != pack ) {
			Log::info( "Loading font from pack: %s", path.c_str() );

			return loadFromPack( pack, path );
		}

		return false;
	}

	// Cleanup the previous resources
	cleanup();
	mRefCount = new int( 1 );

	// Initialize FreeType
	FT_Library library;
	if ( FT_Init_FreeType( &library ) != 0 ) {
		Log::error( "Failed to load font \"%s\" (%s) (failed to initialize FreeType)",
					filename.c_str(), mFontName.c_str() );
		return false;
	}
	mLibrary = library;

	// Load the new font face from the specified file
	FT_Face face;
	if ( FT_New_Face( static_cast<FT_Library>( mLibrary ), filename.c_str(), 0, &face ) != 0 ) {
		Log::error( "Failed to load font \"%s\" (%s) (failed to create the font face)",
					filename.c_str(), mFontName.c_str() );
		return false;
	}

	mFace = face;
	mIsColorEmojiFont = checkIsColorEmojiFont( static_cast<FT_Face>( mFace ) );

	if ( mIsColorEmojiFont && FontManager::instance()->getColorEmojiFont() == nullptr )
		FontManager::instance()->setColorEmojiFont( this );

	FT_Stroker stroker = nullptr;
	if ( !mIsColorEmojiFont ) {
		// Load the stroker that will be used to outline the font
		if ( FT_Stroker_New( static_cast<FT_Library>( mLibrary ), &stroker ) != 0 ) {
			Log::error( "Failed to load font \"%s\" (failed to create the stroker)",
						filename.c_str() );
			FT_Done_Face( face );
			return false;
		}

		// Store the loaded font in our ugly void* :)
		mStroker = stroker;
	}

	// Select the unicode character map
	if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) != 0 ) {
		Log::error( "Failed to load font \"%s\" (failed to set the Unicode character set)",
					filename.c_str() );
		if ( stroker )
			FT_Stroker_Done( stroker );
		FT_Done_Face( face );
		return false;
	}

	// Store the font information
	mInfo.family = face->family_name ? face->family_name : std::string();

	sendEvent( Event::Load );

	return true;
}

bool FontTrueType::loadFromMemory( const void* data, std::size_t sizeInBytes, bool copyData ) {
	const void* ptr = data;

	if ( copyData ) {
		mMemCopy.reset( reinterpret_cast<const Uint8*>( data ), sizeInBytes );

		ptr = mMemCopy.get();
	}

	// Cleanup the previous resources
	cleanup();
	mRefCount = new int( 1 );

	// Initialize FreeType
	FT_Library library;
	if ( FT_Init_FreeType( &library ) != 0 ) {
		Log::error( "Failed to load font from memory (failed to initialize FreeType)" );
		return false;
	}
	mLibrary = library;

	// Load the new font face from the specified file
	FT_Face face;
	if ( FT_New_Memory_Face( static_cast<FT_Library>( mLibrary ),
							 reinterpret_cast<const FT_Byte*>( ptr ),
							 static_cast<FT_Long>( sizeInBytes ), 0, &face ) != 0 ) {
		Log::error( "Failed to load font from memory (failed to create the font face)" );
		return false;
	}

	mFace = face;
	mIsColorEmojiFont = checkIsColorEmojiFont( static_cast<FT_Face>( mFace ) );

	if ( mIsColorEmojiFont && FontManager::instance()->getColorEmojiFont() == nullptr )
		FontManager::instance()->setColorEmojiFont( this );

	// Load the stroker that will be used to outline the font
	FT_Stroker stroker = nullptr;
	if ( !mIsColorEmojiFont ) {
		if ( FT_Stroker_New( static_cast<FT_Library>( mLibrary ), &stroker ) != 0 ) {
			Log::error( "Failed to load font from memory (failed to create the stroker)" );
			FT_Done_Face( face );
			return false;
		}
	}

	// Select the Unicode character map
	if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) != 0 ) {
		Log::error( "Failed to load font from memory (failed to set the Unicode character set)" );
		if ( stroker )
			FT_Stroker_Done( stroker );
		FT_Done_Face( face );
		return false;
	}

	// Store the loaded font in our ugly void* :)
	mStroker = stroker;

	// Store the font information
	mInfo.family = face->family_name ? face->family_name : std::string();

	sendEvent( Event::Load );

	return true;
}

bool FontTrueType::loadFromStream( IOStream& stream ) {
	// Cleanup the previous resources
	cleanup();
	mRefCount = new int( 1 );

	// Initialize FreeType
	FT_Library library;
	if ( FT_Init_FreeType( &library ) != 0 ) {
		Log::error( "Failed to load font from stream (failed to initialize FreeType)" );
		return false;
	}
	mLibrary = library;

	// Make sure that the stream's reading position is at the beginning
	stream.seek( 0 );

	// Prepare a wrapper for our stream, that we'll pass to FreeType callbacks
	FT_StreamRec* rec = new FT_StreamRec;
	std::memset( rec, 0, sizeof( *rec ) );
	rec->base = NULL;
	rec->size = static_cast<unsigned long>( stream.getSize() );
	rec->pos = 0;
	rec->descriptor.pointer = &stream;
	rec->read = &read;
	rec->close = &close;

	// Setup the FreeType callbacks that will read our stream
	FT_Open_Args args;
	args.flags = FT_OPEN_STREAM;
	args.stream = rec;
	args.driver = 0;

	// Load the new font face from the specified stream
	FT_Face face;
	if ( FT_Open_Face( static_cast<FT_Library>( mLibrary ), &args, 0, &face ) != 0 ) {
		Log::error( "Failed to load font from stream (failed to create the font face)" );
		delete rec;
		return false;
	}

	mFace = face;
	mIsColorEmojiFont = checkIsColorEmojiFont( static_cast<FT_Face>( mFace ) );
	FT_Stroker stroker = nullptr;

	if ( mIsColorEmojiFont && FontManager::instance()->getColorEmojiFont() == nullptr )
		FontManager::instance()->setColorEmojiFont( this );

	if ( !mIsColorEmojiFont ) {
		// Load the stroker that will be used to outline the font
		if ( FT_Stroker_New( static_cast<FT_Library>( mLibrary ), &stroker ) != 0 ) {
			Log::error( "Failed to load font from stream (failed to create the stroker)" );
			FT_Done_Face( face );
			delete rec;
			return false;
		}
	}

	// Select the Unicode character map
	if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) != 0 ) {
		Log::error( "Failed to load font from stream (failed to set the Unicode character set)" );
		if ( stroker )
			FT_Stroker_Done( stroker );
		FT_Done_Face( face );
		delete rec;
		return false;
	}

	// Store the loaded font in our ugly void* :)
	mStroker = stroker;
	mStreamRec = rec;

	// Store the font information
	mInfo.family = face->family_name ? face->family_name : std::string();

	sendEvent( Event::Load );

	return true;
}

bool FontTrueType::loadFromPack( Pack* pack, std::string filePackPath ) {
	if ( NULL == pack )
		return false;

	bool Ret = false;

	mMemCopy.clear();

	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, mMemCopy ) ) {
		Ret = loadFromMemory( mMemCopy.get(), mMemCopy.length(), false );
	}

	return Ret;
}

const FontTrueType::Info& FontTrueType::getInfo() const {
	return mInfo;
}

Uint64 FontTrueType::getCharIndexKey( Uint32 codePoint, bool bold, Float outlineThickness ) const {
	Uint64 key = combine( outlineThickness, bold,
						  FT_Get_Char_Index( static_cast<FT_Face>( mFace ), codePoint ) );

	if ( key == 0 && !mIsColorEmojiFont && Font::isEmojiCodePoint( codePoint ) ) {
		if ( FontManager::instance()->getColorEmojiFont() != nullptr &&
			 FontManager::instance()->getColorEmojiFont()->getType() == FontType::TTF ) {
			FontTrueType* fontEmoji =
				static_cast<FontTrueType*>( FontManager::instance()->getColorEmojiFont() );
			key =
				combine( outlineThickness, bold,
						 FT_Get_Char_Index( static_cast<FT_Face>( fontEmoji->mFace ), codePoint ) );
		}
	}

	return key;
}

const Glyph& FontTrueType::getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold,
									 Float outlineThickness ) const {
	// Get the page corresponding to the character size
	GlyphTable& glyphs = mPages[characterSize].glyphs;

	// Build the key by combining the code point, bold flag, and outline thickness
	Uint64 key = getCharIndexKey( codePoint, bold, outlineThickness );

	// Search the glyph into the cache
	GlyphTable::const_iterator it = glyphs.find( key );
	if ( it != glyphs.end() ) {
		// Found: just return it
		return it->second;
	} else {
		// Not found: we have to load it
		Glyph glyph =
			loadGlyph( codePoint, characterSize, bold, outlineThickness, mPages[characterSize] );
		return glyphs.insert( std::make_pair( key, glyph ) ).first->second;
	}
}

GlyphDrawable* FontTrueType::getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
											   bool bold, Float outlineThickness ) const {
	GlyphDrawableTable& drawables = mPages[characterSize].drawables;

	Uint64 key = getCharIndexKey( codePoint, bold, outlineThickness );

	auto it = drawables.find( key );
	if ( it != drawables.end() ) {
		return it->second;
	} else {
		const Glyph& glyph = getGlyph( codePoint, characterSize, bold, outlineThickness );
		auto& page = mPages[characterSize];
		GlyphDrawable* region = GlyphDrawable::New(
			page.texture, glyph.textureRect,
			String::format( "%s_%d_%u", mFontName.c_str(), characterSize, codePoint ) );
		region->setGlyphOffset( { glyph.bounds.Left - outlineThickness,
								  characterSize + glyph.bounds.Top - outlineThickness } );
		drawables[key] = region;
		return region;
	}
}

Float FontTrueType::getKerning( Uint32 first, Uint32 second, unsigned int characterSize ) const {
	// Special case where first or second is 0 (null character)
	if ( first == 0 || second == 0 )
		return 0.f;

	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && FT_HAS_KERNING( face ) && setCurrentSize( characterSize ) ) {
		// Convert the characters to indices
		FT_UInt index1 = FT_Get_Char_Index( face, first );
		FT_UInt index2 = FT_Get_Char_Index( face, second );

		// Get the kerning vector
		FT_Vector kerning;
		FT_Get_Kerning( face, index1, index2, FT_KERNING_DEFAULT, &kerning );

		// X advance is already in pixels for bitmap fonts
		if ( !FT_IS_SCALABLE( face ) )
			return static_cast<Float>( kerning.x );

		// Return the X advance
		return static_cast<Float>( kerning.x ) / static_cast<Float>( 1 << 6 );
	} else {
		// Invalid font, or no kerning
		return 0.f;
	}
}

Float FontTrueType::getLineSpacing( unsigned int characterSize ) const {
	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && setCurrentSize( characterSize ) ) {
		return static_cast<Float>( face->size->metrics.height ) / static_cast<Float>( 1 << 6 );
	} else {
		return 0.f;
	}
}

#define FT_FLOOR( X ) ( ( X & -64 ) / 64 )
#define FT_CEIL( X ) ( ( ( X + 63 ) & -64 ) / 64 )

Uint32 FontTrueType::getFontHeight( const Uint32& characterSize ) {
	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && setCurrentSize( characterSize ) ) {
		if ( FT_IS_SCALABLE( face ) ) {
			FT_Fixed scale = face->size->metrics.y_scale;

			int ascent = ( FT_CEIL( FT_MulFix( face->ascender, scale ) ) );
			int descent = ( FT_CEIL( FT_MulFix( face->descender, scale ) ) );
			int height = ( ascent - descent + 1 );
			return height;
		} else {
			return static_cast<Float>( (Float)face->size->metrics.height ) /
				   static_cast<Float>( 1 << 6 );
		}
	} else {
		return 0.f;
	}
}

Float FontTrueType::getUnderlinePosition( unsigned int characterSize ) const {
	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && setCurrentSize( characterSize ) ) {
		// Return a fixed position if font is a bitmap font
		if ( !FT_IS_SCALABLE( face ) )
			return characterSize / 10.f;

		return -static_cast<Float>(
				   FT_MulFix( face->underline_position, face->size->metrics.y_scale ) ) /
			   static_cast<Float>( 1 << 6 );
	} else {
		return 0.f;
	}
}

Float FontTrueType::getUnderlineThickness( unsigned int characterSize ) const {
	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && setCurrentSize( characterSize ) ) {
		// Return a fixed thickness if font is a bitmap font
		if ( !FT_IS_SCALABLE( face ) )
			return characterSize / 14.f;

		return static_cast<Float>(
				   FT_MulFix( face->underline_thickness, face->size->metrics.y_scale ) ) /
			   static_cast<Float>( 1 << 6 );
	} else {
		return 0.f;
	}
}

Texture* FontTrueType::getTexture( unsigned int characterSize ) const {
	return mPages[characterSize].texture;
}

bool FontTrueType::loaded() const {
	return NULL != mFace;
}

FontTrueType& FontTrueType::operator=( const FontTrueType& right ) {
	FontTrueType temp( right.getName() );

	temp.mMemCopy.swap( right.mMemCopy );
	std::swap( mLibrary, temp.mLibrary );
	std::swap( mFace, temp.mFace );
	std::swap( mStreamRec, temp.mStreamRec );
	std::swap( mStroker, temp.mStroker );
	std::swap( mRefCount, temp.mRefCount );
	std::swap( mInfo, temp.mInfo );
	std::swap( mPages, temp.mPages );
	std::swap( mPixelBuffer, temp.mPixelBuffer );
	return *this;
}

void FontTrueType::cleanup() {
	sendEvent( Event::Unload );

	if ( FontManager::existsSingleton() && FontManager::instance()->getColorEmojiFont() == this )
		FontManager::instance()->setColorEmojiFont( nullptr );

	mCallbacks.clear();
	mNumCallBacks = 0;

	// Check if we must destroy the FreeType pointers
	if ( mRefCount ) {
		// Decrease the reference counter
		( *mRefCount )--;

		// Free the resources only if we are the last owner
		if ( *mRefCount == 0 ) {
			// Delete the reference counter
			delete mRefCount;

			// Destroy the stroker
			if ( mStroker )
				FT_Stroker_Done( static_cast<FT_Stroker>( mStroker ) );

			// Destroy the font face
			if ( mFace )
				FT_Done_Face( static_cast<FT_Face>( mFace ) );

			// Destroy the stream rec instance, if any (must be done after FT_Done_Face!)
			if ( mStreamRec )
				delete static_cast<FT_StreamRec*>( mStreamRec );

			// Close the library
			if ( mLibrary )
				FT_Done_FreeType( static_cast<FT_Library>( mLibrary ) );
		}
	}

	// Reset members
	mLibrary = NULL;
	mFace = NULL;
	mStroker = NULL;
	mStreamRec = NULL;
	mRefCount = NULL;
	mPages.clear();
	std::vector<Uint8>().swap( mPixelBuffer );
}

Glyph FontTrueType::loadGlyph( Uint32 codePoint, unsigned int characterSize, bool bold,
							   Float outlineThickness, Page& page, const Float& forceSize ) const {
	// The glyph to return
	Glyph glyph;

	if ( !mIsColorEmojiFont && Font::isEmojiCodePoint( codePoint ) ) {
		if ( FontManager::instance()->getColorEmojiFont() != nullptr &&
			 FontManager::instance()->getColorEmojiFont()->getType() == FontType::TTF ) {

			Float forcedSize = 0.f;

			if ( isMonospace() ) {
				Glyph monospaceGlyph = getGlyph( ' ', characterSize, bold, outlineThickness );
				forcedSize = monospaceGlyph.advance;
			}

			FontTrueType* fontEmoji =
				static_cast<FontTrueType*>( FontManager::instance()->getColorEmojiFont() );
			return fontEmoji->loadGlyph( codePoint, characterSize, bold, outlineThickness, page,
										 forcedSize );
		}
	}

	// First, transform our ugly void* to a FT_Face
	FT_Face face = static_cast<FT_Face>( mFace );
	if ( !face ) {
		Log::error( "FT_Face failed for: codePoint %d characterSize: %d font %s", codePoint,
					characterSize, mFontName.c_str() );
		return glyph;
	}

	// Set the character size
	if ( !setCurrentSize( characterSize ) ) {
		Log::error(
			"FontTrueType::setCurrentSize failed for: codePoint %d characterSize: %d font %s",
			codePoint, characterSize, mFontName.c_str() );
		return glyph;
	}

	FT_Error err = 0;

	// Load the glyph corresponding to the code point
	FT_Int32 flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_COLOR;
	if ( outlineThickness != 0 )
		flags |= FT_LOAD_NO_BITMAP;
	if ( ( err = FT_Load_Char( face, codePoint, flags ) ) != 0 ) {
		Log::error( "FT_Load_Char failed for: codePoint %d characterSize: %d font: %s error: %d",
					codePoint, characterSize, mFontName.c_str(), err );
		return glyph;
	}

	// Retrieve the glyph
	FT_Glyph glyphDesc;
	if ( FT_Get_Glyph( face->glyph, &glyphDesc ) != 0 ) {
		Log::error( "FT_Get_Glyph failed for: codePoint %d characterSize: %d font: %s", codePoint,
					characterSize, mFontName.c_str() );
		return glyph;
	}

	// Apply bold and outline (there is no fallback for outline) if necessary -- first technique
	// using outline (highest quality)
	FT_Pos weight = 1 << 6;
	bool outline = ( glyphDesc->format == FT_GLYPH_FORMAT_OUTLINE );
	if ( outline ) {
		if ( bold ) {
			FT_OutlineGlyph outlineGlyph = (FT_OutlineGlyph)glyphDesc;
			FT_Outline_Embolden( &outlineGlyph->outline, weight );
		}

		if ( outlineThickness != 0 ) {
			FT_Stroker stroker = static_cast<FT_Stroker>( mStroker );

			FT_Stroker_Set(
				stroker, static_cast<FT_Fixed>( outlineThickness * static_cast<Float>( 1 << 6 ) ),
				FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0 );
			FT_Glyph_Stroke( &glyphDesc, stroker, true );
		}
	}

	// Convert the glyph to a bitmap (i.e. rasterize it)
	FT_Glyph_To_Bitmap( &glyphDesc, FT_RENDER_MODE_NORMAL, 0, 1 );
	FT_Bitmap& bitmap = reinterpret_cast<FT_BitmapGlyph>( glyphDesc )->bitmap;

	// Apply bold if necessary -- fallback technique using bitmap (lower quality)
	if ( !outline ) {
		if ( bold )
			FT_Bitmap_Embolden( static_cast<FT_Library>( mLibrary ), &bitmap, weight, weight );

		if ( outlineThickness != 0 )
			Log::error( "Failed to outline glyph (no fallback available)" );
	}

	// Compute the glyph's advance offset
	glyph.advance =
		static_cast<Float>( face->glyph->metrics.horiAdvance ) / static_cast<Float>( 1 << 6 );

	if ( forceSize > 0.f )
		glyph.advance = forceSize;

	if ( bold && !mBoldAdvanceSameAsRegular )
		glyph.advance += static_cast<Float>( weight ) / static_cast<Float>( 1 << 6 );

	int width = bitmap.width;
	int height = bitmap.rows;

	if ( ( width > 0 ) && ( height > 0 ) ) {
		// Leave a small padding around characters, so that filtering doesn't
		// pollute them with pixels from neighbors
		const int padding = 2;

		Float scale = mIsColorEmojiFont
						  ? (Float)( forceSize > 0.f ? forceSize : characterSize ) / (Float)height
						  : 1.f;

		int destWidth = width;
		int destHeight = height;

		if ( mIsColorEmojiFont ) {
			destWidth *= scale;
			destHeight *= scale;
			if ( forceSize <= 0.f )
				glyph.advance *= scale;
		}

		width += 2 * padding;
		height += 2 * padding;
		destWidth += 2 * padding;
		destHeight += 2 * padding;

		// Compute the glyph's bounding box
		glyph.bounds.Left =
			static_cast<Float>( face->glyph->metrics.horiBearingX ) / static_cast<Float>( 1 << 6 );
		glyph.bounds.Top =
			-static_cast<Float>( face->glyph->metrics.horiBearingY ) / static_cast<Float>( 1 << 6 );
		glyph.bounds.Right =
			static_cast<Float>( face->glyph->metrics.width ) / static_cast<Float>( 1 << 6 ) +
			outlineThickness * 2;
		glyph.bounds.Bottom =
			static_cast<Float>( face->glyph->metrics.height ) / static_cast<Float>( 1 << 6 ) +
			outlineThickness * 2;

		// Resize the pixel buffer to the new size and fill it with transparent white pixels
		const Uint32 bufferSize = width * height * 4;
		mPixelBuffer.resize( bufferSize );

		Uint8* pixelPtr = &mPixelBuffer[0];
		Uint8* current = pixelPtr;
		Uint8* end = current + bufferSize;

		while ( current != end ) {
			( *current++ ) = 255;
			( *current++ ) = 255;
			( *current++ ) = 255;
			( *current++ ) = 0;
		}

		// Extract the glyph's pixels from the bitmap
		const Uint8* pixels = bitmap.buffer;
		if ( bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
			// Pixels are 1 bit monochrome values
			for ( int y = padding; y < height - padding; ++y ) {
				for ( int x = padding; x < width - padding;
					  ++x ) // Extract the glyph's pixels from the bitmap
				{
					// The color channels remain white, just fill the alpha channel
					std::size_t index = x + y * width;
					mPixelBuffer[index * 4 + 3] = ( ( pixels[( x - padding ) / 8] ) &
													( 1 << ( 7 - ( ( x - padding ) % 8 ) ) ) )
													  ? 255
													  : 0;
				}
				pixels += bitmap.pitch;
			}
		} else if ( bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ) {
			Image source( (Uint8*)pixels, bitmap.width, bitmap.rows, 4 );
			Image dest( &mPixelBuffer[0], width, height, 4 );
			source.avoidFreeImage( true );
			dest.avoidFreeImage( true );
			for ( size_t y = 0; y < bitmap.rows; ++y ) {
				for ( size_t x = 0; x < bitmap.width; ++x ) {
					Color col = source.getPixel( x, y );
					dest.setPixel( x + padding, y + padding, Color( col.b, col.g, col.r, col.a ) );
				}
			}

			if ( scale < 1.f ) {
				dest.scale( scale );
				dest.avoidFreeImage( true );
				pixelPtr = dest.getPixels();
				glyph.bounds.Left *= scale;
				glyph.bounds.Right *= scale;
				glyph.bounds.Top *= scale;
				glyph.bounds.Bottom *= scale;
				destWidth = dest.getWidth() + 2 * padding;
				destHeight = dest.getHeight() + 2 * padding;
			}
		} else {
			// Pixels are 8 bits gray levels
			for ( int y = padding; y < height - padding; ++y ) {
				for ( int x = padding; x < width - padding; ++x ) {
					// The color channels remain white, just fill the alpha channel
					std::size_t index = x + y * width;
					mPixelBuffer[index * 4 + 3] = pixels[x - padding];
				}
				pixels += bitmap.pitch;
			}
		}

		// Find a good position for the new glyph into the texture
		glyph.textureRect = findGlyphRect( page, destWidth, destHeight );

		// Write the pixels to the texture
		unsigned int x = glyph.textureRect.Left;
		unsigned int y = glyph.textureRect.Top;
		unsigned int w = glyph.textureRect.Right;
		unsigned int h = glyph.textureRect.Bottom;

		if ( bitmap.pixel_mode == FT_PIXEL_MODE_BGRA && scale < 1.f ) {
			w = destWidth - 2 * padding;
			h = destHeight - 2 * padding;
			x += padding;
			y += padding;
		}

		// Make sure the texture data is positioned in the center
		// of the allocated texture rectangle
		glyph.textureRect.Left += padding;
		glyph.textureRect.Top += padding;
		glyph.textureRect.Right -= 2 * padding;
		glyph.textureRect.Bottom -= 2 * padding;

		page.texture->update( pixelPtr, w, h, x, y );

		if ( scale < 1.f )
			eeFree( pixelPtr );
	}

	// Delete the FT glyph
	FT_Done_Glyph( glyphDesc );

	// Done :)
	return glyph;
}

Rect FontTrueType::findGlyphRect( Page& page, unsigned int width, unsigned int height ) const {
	// Find the line that fits well the glyph
	Row* row = NULL;
	Float bestRatio = 0;
	for ( std::vector<Row>::iterator it = page.rows.begin(); it != page.rows.end() && !row; ++it ) {
		Float ratio = static_cast<Float>( height ) / it->height;

		// Ignore rows that are either too small or too high
		if ( ( ratio < 0.7f ) || ( ratio > 1.f ) )
			continue;

		// Check if there's enough horizontal space left in the row
		if ( width > page.texture->getPixelsSize().x - it->width )
			continue;

		// Make sure that this new row is the best found so far
		if ( ratio < bestRatio )
			continue;

		// The current row passed all the tests: we can select it
		row = &*it;
		bestRatio = ratio;
	}

	// If we didn't find a matching row, create a new one (10% taller than the glyph)
	if ( !row ) {
		int rowHeight = height + height / 10;
		while ( ( page.nextRow + rowHeight >= (Uint32)page.texture->getPixelsSize().y ) ||
				( width >= (Uint32)page.texture->getPixelsSize().x ) ) {
			// Not enough space: resize the texture if possible
			unsigned int textureWidth = page.texture->getPixelsSize().x;
			unsigned int textureHeight = page.texture->getPixelsSize().y;
			if ( ( textureWidth * 2 <= Texture::getMaximumSize() ) &&
				 ( textureHeight * 2 <= Texture::getMaximumSize() ) ) {
				// Make the texture 2 times bigger
				Image newImage;
				newImage.create( textureWidth * 2, textureHeight * 2, 4 );
				newImage.copyImage( page.texture );

				page.texture->replace( &newImage );
			} else {
				// Oops, we've reached the maximum texture size...
				Log::error(
					"Failed to add a new character to the font: the maximum texture size has "
					"been reached" );
				return Rect( 0, 0, 2, 2 );
			}
		}

		// We can now create the new row
		page.rows.push_back( Row( page.nextRow, rowHeight ) );
		page.nextRow += rowHeight;
		row = &page.rows.back();
	}

	// Find the glyph's rectangle on the selected row
	Rect rect( row->width, row->top, width, height );

	// Update the row informations
	row->width += width;

	return rect;
}

bool FontTrueType::setCurrentSize( unsigned int characterSize ) const {
	// FT_Set_Pixel_Sizes is an expensive function, so we must call it
	// only when necessary to avoid killing performances

	FT_Face face = static_cast<FT_Face>( mFace );
	FT_UShort currentSize = face->size->metrics.x_ppem;

	if ( currentSize != characterSize ) {
		if ( mIsColorEmojiFont ) {
			int bestMatch = 0;
			int diff = eeabs( characterSize - face->available_sizes[0].width );
			for ( int i = 1; i < face->num_fixed_sizes; ++i ) {
				int ndiff = eeabs( characterSize - face->available_sizes[i].width );
				if ( ndiff < diff ) {
					bestMatch = i;
					diff = ndiff;
				}
			}
			characterSize = bestMatch;
		}

		FT_Error result = mIsColorEmojiFont ? FT_Select_Size( face, characterSize )
											: FT_Set_Pixel_Sizes( face, 0, characterSize );

		if ( result == FT_Err_Invalid_Pixel_Size ) {
			// In the case of bitmap fonts, resizing can
			// fail if the requested size is not available
			if ( !FT_IS_SCALABLE( face ) ) {
				auto it = mClosestCharacterSize.find( characterSize );

				if ( it == mClosestCharacterSize.end() ) {
					Log::warning( "Failed to set bitmap font size to %d", characterSize );
					Log::warning( "Available sizes are: " );
					std::string str;
					if ( face->num_fixed_sizes > 0 ) {
						unsigned int selectedHeight = face->available_sizes[0].height;
						int curDistance = eeabs( characterSize - selectedHeight );
						for ( int i = 0; i < face->num_fixed_sizes; ++i ) {
							str += String::format( "%d ", face->available_sizes[i].height );
							int tDistance =
								eeabs( characterSize - face->available_sizes[i].height );
							if ( tDistance < curDistance ) {
								curDistance = tDistance;
								selectedHeight = face->available_sizes[i].height;
							}
						}
						Log::warning( str );
						Log::warning( "Setting closest bitmap font size available: ",
									  selectedHeight );
						mClosestCharacterSize[characterSize] = selectedHeight;
						return setCurrentSize( selectedHeight );
					} else {
						return false;
					}
				} else if ( characterSize != currentSize && face->size->metrics.x_ppem > 0 ) {
					return setCurrentSize( it->second );
				}
			}
		}

		return result == FT_Err_Ok;
	} else {
		return true;
	}
}

bool FontTrueType::isColorEmojiFont() const {
	return mIsColorEmojiFont;
}

bool FontTrueType::isMonospace() const {
	return FT_IS_FIXED_WIDTH( static_cast<FT_Face>( mFace ) );
}

bool FontTrueType::getBoldAdvanceSameAsRegular() const {
	return mBoldAdvanceSameAsRegular;
}

void FontTrueType::setBoldAdvanceSameAsRegular( bool boldAdvanceSameAsRegular ) {
	mBoldAdvanceSameAsRegular = boldAdvanceSameAsRegular;
}

FontTrueType::Page::Page() : texture( NULL ), nextRow( 3 ) {
	// Make sure that the texture is initialized by default
	Image image;
	image.create( 128, 128, 4 );

	// Reserve a 2x2 white square for texturing underlines
	for ( int x = 0; x < 2; ++x )
		for ( int y = 0; y < 2; ++y )
			image.setPixel( x, y, Color( 255, 255, 255, 255 ) );

	// Create the texture
	Uint32 texId = TextureFactory::instance()->loadFromPixels(
		image.getPixelsPtr(), image.getWidth(), image.getHeight(), image.getChannels(), false,
		Texture::ClampMode::ClampToEdge, false, true );
	texture = TextureFactory::instance()->getTexture( texId );
	texture->setCoordinateType( Texture::CoordinateType::Pixels );
}

FontTrueType::Page::~Page() {
	for ( auto drawable : drawables )
		eeDelete( drawable.second );

	if ( NULL != texture && TextureFactory::existsSingleton() )
		TextureFactory::instance()->remove( texture->getTextureId() );
}

}} // namespace EE::Graphics
