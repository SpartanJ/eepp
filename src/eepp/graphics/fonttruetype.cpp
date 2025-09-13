#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/window/engine.hpp>
using namespace EE::Window;

#include <freetype/ftlcdfil.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_STROKER_H
#include FT_TRUETYPE_TABLES_H
#include <atomic>
#include <cstdlib>
#include <cstring>

#ifdef EE_TEXT_SHAPER_ENABLED
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>
#endif

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

// Helper to interpret memory as a specific type
template <typename T, typename U> inline T reinterpret( const U& input ) {
	T output;
	std::memcpy( &output, &input, sizeof( U ) );
	return output;
}

} // namespace

namespace EE { namespace Graphics {
using std::move;

static std::unordered_map<std::string, Uint32> fontsInternalIds;
static std::atomic<Uint32> fontInternalIdCounter{ 0 };

// Combine outline thickness, boldness, italics and font glyph index into a single 64-bit key
static inline Uint64 getIndexKey( Uint32 fontInternalId, Uint32 index, bool bold, bool italics,
								  Float outlineThickness ) {
	return ( static_cast<EE::Uint64>( reinterpret<EE::Uint32>( fontInternalId ) ) << 48 ) |
		   ( static_cast<EE::Uint64>(
				 reinterpret<EE::Uint32>( static_cast<Uint32>( outlineThickness ) * 100 ) )
			 << 34 ) |
		   ( static_cast<EE::Uint64>( bold ) << 33 ) |
		   ( static_cast<EE::Uint64>( italics ) << 32 ) | index;
}

static inline Uint64 getCodePointKey( Uint32 codePoint, bool bold, bool italics,
									  Float outlineThickness ) {
	return ( static_cast<EE::Uint64>(
				 reinterpret<EE::Uint32>( static_cast<Uint32>( outlineThickness ) * 100 ) )
			 << 34 ) |
		   ( static_cast<EE::Uint64>( bold ) << 33 ) |
		   ( static_cast<EE::Uint64>( italics ) << 32 ) | codePoint;
}

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
	mInfo(),
	mBoldAdvanceSameAsRegular( false ),
	mHinting( FontManager::instance()->getHinting() ),
	mAntialiasing( FontManager::instance()->getAntialiasing() ) {}

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

	mInfo.fontpath = FileSystem::fileRemoveFileName( filename );
	mInfo.filename = FileSystem::fileNameFromPath( filename );

	return setFontFace( face );
}

bool FontTrueType::loadFromMemory( const void* data, std::size_t sizeInBytes, bool copyData ) {
	const void* ptr = data;

	if ( copyData ) {
		mMemCopy.reset( reinterpret_cast<const Uint8*>( data ), sizeInBytes );

		ptr = mMemCopy.get();
	}

	// Cleanup the previous resources
	cleanup();

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

	return setFontFace( face );
}

bool FontTrueType::loadFromStream( IOStream& stream ) {
	// Cleanup the previous resources
	cleanup();

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

	bool res = setFontFace( face );
	if ( res )
		mStreamRec = rec;

	return res;
}

bool FontTrueType::loadFromPack( Pack* pack, std::string filePackPath ) {
	if ( NULL == pack )
		return false;

	bool ret = false;

	mMemCopy.clear();

	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, mMemCopy ) )
		ret = loadFromMemory( mMemCopy.get(), mMemCopy.length(), false );

	mInfo.fontpath = FileSystem::fileRemoveFileName( filePackPath );
	mInfo.filename = FileSystem::fileNameFromPath( filePackPath );

	return ret;
}

bool FontTrueType::setFontFace( void* _face ) {
	FT_Face face = (FT_Face)_face;
	mFace = face;
#ifdef EE_TEXT_SHAPER_ENABLED
	mHBFont = hb_ft_font_create( static_cast<FT_Face>( face ), NULL );
#endif
	mIsMonospaceComplete = mIsMonospace = FT_IS_FIXED_WIDTH( face );
	mIsColorEmojiFont = checkIsColorEmojiFont( face );
	mIsEmojiFont = FT_Get_Char_Index( face, 0x1F600 ) != 0;
	mIsBold = face->style_flags & FT_STYLE_FLAG_BOLD;
	mIsItalic = face->style_flags & FT_STYLE_FLAG_ITALIC;

	if ( mIsColorEmojiFont && FontManager::instance()->getColorEmojiFont() == nullptr )
		FontManager::instance()->setColorEmojiFont( this );

	if ( mIsEmojiFont && FontManager::instance()->getEmojiFont() == nullptr )
		FontManager::instance()->setEmojiFont( this );

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

	updateFontInternalId();

	sendEvent( Event::Load );

	return true;
}

const FontTrueType::Info& FontTrueType::getInfo() const {
	return mInfo;
}

void FontTrueType::updateFontInternalId() {
	auto fontInternalId = fontsInternalIds.find( mInfo.family );
	if ( fontsInternalIds.end() == fontInternalId ) {
		mFontInternalId = ++fontInternalIdCounter;
		fontsInternalIds[mInfo.family] = mFontInternalId;
	} else {
		mFontInternalId = fontInternalId->second;
	}
}

bool FontTrueType::hasGlyph( Uint32 codePoint ) const {
	return getGlyphIndex( codePoint ) != 0;
}

Uint32 FontTrueType::getGlyphIndex( const Uint32& codePoint ) const {
	Uint32 index;
	auto indexIter = mCodePointIndexCache.find( codePoint );
	if ( mCodePointIndexCache.end() != indexIter ) {
		index = indexIter->second;
	} else {
		index = FT_Get_Char_Index( static_cast<FT_Face>( mFace ), codePoint );
		mCodePointIndexCache[codePoint] = index;
	}
	return index;
}

Glyph FontTrueType::getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
							  Float outlineThickness ) const {
	Uint32 idx = 0;
	if ( mEnableEmojiFallback && !mIsColorEmojiFont && !mIsEmojiFont &&
		 Font::isEmojiCodePoint( codePoint ) ) {
		if ( !mIsColorEmojiFont && FontManager::instance()->getColorEmojiFont() != nullptr &&
			 FontManager::instance()->getColorEmojiFont()->getType() == FontType::TTF ) {

			FontTrueType* fontEmoji =
				static_cast<FontTrueType*>( FontManager::instance()->getColorEmojiFont() );
			if ( ( idx = fontEmoji->getGlyphIndex( codePoint ) ) ) {
				if ( mIsMonospace && mEnableDynamicMonospace ) {
					mIsMonospaceComplete = false;
					mUsingFallback = true;
				}
				return fontEmoji->getGlyphByIndex( idx, characterSize, bold, italic,
												   outlineThickness, getPage( characterSize ) );
			}
		} else if ( !mIsEmojiFont && FontManager::instance()->getEmojiFont() != nullptr &&
					FontManager::instance()->getEmojiFont()->getType() == FontType::TTF ) {

			FontTrueType* fontEmoji =
				static_cast<FontTrueType*>( FontManager::instance()->getEmojiFont() );
			if ( ( idx = fontEmoji->getGlyphIndex( codePoint ) ) ) {
				if ( mIsMonospace && mEnableDynamicMonospace ) {
					mIsMonospaceComplete = false;
					mUsingFallback = true;
				}
				return fontEmoji->getGlyphByIndex( idx, characterSize, bold, italic,
												   outlineThickness, getPage( characterSize ) );
			}
		}
	}

	if ( bold && italic && mFontBoldItalic != nullptr &&
		 ( idx = mFontBoldItalic->getGlyphIndex( codePoint ) ) ) {
		return mFontBoldItalic->getGlyphByIndex( idx, characterSize, true, true, outlineThickness,
												 getPage( characterSize ) );
	}

	if ( bold && !italic && mFontBold != nullptr &&
		 ( idx = mFontBold->getGlyphIndex( codePoint ) ) ) {
		return mFontBold->getGlyphByIndex( idx, characterSize, true, false, outlineThickness,
										   getPage( characterSize ) );
	}

	if ( italic && !bold && mFontItalic != nullptr &&
		 ( idx = mFontItalic->getGlyphIndex( codePoint ) ) ) {
		return mFontItalic->getGlyphByIndex( idx, characterSize, false, true, outlineThickness,
											 getPage( characterSize ) );
	}

	idx = getGlyphIndex( codePoint );
	if ( 0 == idx && mEnableFallbackFont && FontManager::instance()->hasFallbackFonts() ) {
		for ( Font* fallbackFontPtr : FontManager::instance()->getFallbackFonts() ) {
			if ( fallbackFontPtr->getType() != FontType::TTF )
				continue;
			FontTrueType* fallbackFont = static_cast<FontTrueType*>( fallbackFontPtr );
			if ( ( idx = fallbackFont->getGlyphIndex( codePoint ) ) ) {
				if ( mIsMonospace && mEnableDynamicMonospace ) {
					mIsMonospaceComplete = false;
					mUsingFallback = true;
				}
				return fallbackFont->getGlyphByIndex( idx, characterSize, bold, italic,
													  outlineThickness, getPage( characterSize ) );
			}
		}
	}

	return getGlyphByIndex( idx, characterSize, bold, italic, outlineThickness );
}

Glyph FontTrueType::getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
							  Float outlineThickness, Page& page ) const {
	Uint32 index = getGlyphIndex( codePoint );
	return getGlyphByIndex( index, characterSize, bold, italic, outlineThickness, page );
}

Glyph FontTrueType::getGlyphByIndex( Uint32 index, unsigned int characterSize, bool bold,
									 bool italic, Float outlineThickness, Page& page ) const {
	eeASSERT( Engine::isMainThread() );

	// Get the page corresponding to the character size
	GlyphTable& glyphs = page.glyphs;

	// Build the key by combining the code point, bold flag, and outline thickness
	Uint64 key = getIndexKey( mFontInternalId, index, bold, italic, outlineThickness );

	// Search the glyph into the cache
	GlyphTable::const_iterator it = glyphs.find( key );
	if ( it != glyphs.end() ) {
		// Found: just return it
		return it->second;
	} else {
		// Not found: we have to load it
		Glyph glyph =
			loadGlyphByIndex( index, characterSize, bold, italic, outlineThickness, page );

		return glyphs.emplace( key, glyph ).first->second;
	}
}

Glyph FontTrueType::getGlyphByIndex( Uint32 index, unsigned int characterSize, bool bold,
									 bool italic, Float outlineThickness ) const {
	return getGlyphByIndex( index, characterSize, bold, italic, outlineThickness,
							getPage( characterSize ) );
}

GlyphDrawable* FontTrueType::getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
											   bool bold, bool italic,
											   Float outlineThickness ) const {
	// mKeyCache
	Page& page = getPage( characterSize );
	GlyphDrawableTable& drawables = page.drawables;

	Uint32 glyphIndex = 0;
	Uint32 fontInternalId = mFontInternalId;

	Uint64 codePointKey = getCodePointKey( codePoint, bold, italic, outlineThickness );
	auto cache = mKeyCache.find( codePointKey );
	bool isItalic = false;

	if ( cache != mKeyCache.end() ) {
		fontInternalId = std::get<0>( cache->second );
		glyphIndex = std::get<1>( cache->second );
		isItalic = std::get<2>( cache->second );
	} else {
		Uint32 tGlyphIndex = 0;

		if ( mEnableEmojiFallback && Font::isEmojiCodePoint( codePoint ) && !mIsColorEmojiFont &&
			 !mIsEmojiFont ) {
			if ( !mIsColorEmojiFont && FontManager::instance()->getColorEmojiFont() != nullptr &&
				 FontManager::instance()->getColorEmojiFont()->getType() == FontType::TTF ) {
				FontTrueType* fontEmoji =
					static_cast<FontTrueType*>( FontManager::instance()->getColorEmojiFont() );
				tGlyphIndex = fontEmoji->getGlyphIndex( codePoint );
				if ( 0 != tGlyphIndex ) {
					glyphIndex = tGlyphIndex;
					fontInternalId = fontEmoji->getFontInternalId();
				} else {
					glyphIndex = getGlyphIndex( codePoint );
				}
			} else if ( !mIsEmojiFont && FontManager::instance()->getEmojiFont() != nullptr &&
						FontManager::instance()->getEmojiFont()->getType() == FontType::TTF ) {
				FontTrueType* fontEmoji =
					static_cast<FontTrueType*>( FontManager::instance()->getEmojiFont() );
				tGlyphIndex = fontEmoji->getGlyphIndex( codePoint );
				if ( 0 != tGlyphIndex ) {
					glyphIndex = tGlyphIndex;
					fontInternalId = fontEmoji->getFontInternalId();
				} else {
					glyphIndex = getGlyphIndex( codePoint );
				}
			} else {
				glyphIndex = getGlyphIndex( codePoint );
			}
		} else {
			glyphIndex = getGlyphIndex( codePoint );
		}

		if ( bold && italic && mFontBoldItalic != nullptr &&
			 ( tGlyphIndex = mFontBoldItalic->getGlyphIndex( codePoint ) ) ) {
			glyphIndex = tGlyphIndex;
			fontInternalId = mFontBoldItalic->getFontInternalId();
			isItalic = true;
		}

		if ( bold && !italic && mFontBold != nullptr &&
			 ( tGlyphIndex = mFontBold->getGlyphIndex( codePoint ) ) ) {
			glyphIndex = tGlyphIndex;
			fontInternalId = mFontBold->getFontInternalId();
		}

		if ( italic && !bold && mFontItalic != nullptr &&
			 ( tGlyphIndex = mFontItalic->getGlyphIndex( codePoint ) ) ) {
			glyphIndex = tGlyphIndex;
			fontInternalId = mFontItalic->getFontInternalId();
			isItalic = true;
		}

		if ( 0 == glyphIndex && mEnableFallbackFont &&
			 FontManager::instance()->hasFallbackFonts() ) {
			for ( Font* fontFallbackPtr : FontManager::instance()->getFallbackFonts() ) {
				if ( fontFallbackPtr->getType() != FontType::TTF )
					continue;
				FontTrueType* fontFallback = static_cast<FontTrueType*>( fontFallbackPtr );
				tGlyphIndex = fontFallback->getGlyphIndex( codePoint );
				if ( 0 != tGlyphIndex ) {
					glyphIndex = tGlyphIndex;
					fontInternalId = fontFallback->getFontInternalId();
					if ( mIsMonospace && mEnableDynamicMonospace ) {
						mIsMonospaceComplete = false;
						mUsingFallback = true;
					}
					break;
				}
			}
			if ( 0 == glyphIndex )
				glyphIndex = getGlyphIndex( codePoint );
		}

		mKeyCache[codePointKey] = { fontInternalId, glyphIndex, isItalic };
	}

	Uint64 key = getIndexKey( fontInternalId, glyphIndex, bold, italic, outlineThickness );

	auto it = drawables.find( key );
	if ( it != drawables.end() ) {
		return it->second;
	} else {
		auto glyph = getGlyph( codePoint, characterSize, bold, italic, outlineThickness );
		GlyphDrawable* region = GlyphDrawable::New(
			page.texture, glyph.textureRect, glyph.size,
			String::format( "%s_%d_%u", mFontName.c_str(), characterSize, glyphIndex ) );

		region->setGlyphOffset( { glyph.bounds.Left - outlineThickness,
								  characterSize + glyph.bounds.Top - outlineThickness } );
		region->setAdvance( glyph.advance );
		region->setIsItalic( isItalic );

		drawables[key] = region;
		return region;
	}
	return nullptr;
}

GlyphDrawable* FontTrueType::getGlyphDrawableFromGlyphIndex( Uint32 glyphIndex,
															 unsigned int characterSize, bool bold,
															 bool italic, Float outlineThickness,
															 Page& page ) const {
	GlyphDrawableTable& drawables = page.drawables;
	Uint64 key = getIndexKey( mFontInternalId, glyphIndex, bold, italic, outlineThickness );

	auto it = drawables.find( key );
	if ( it != drawables.end() ) {
		return it->second;
	} else {
		auto glyph =
			getGlyphByIndex( glyphIndex, characterSize, bold, italic, outlineThickness, page );
		GlyphDrawable* region = GlyphDrawable::New(
			page.texture, glyph.textureRect, glyph.size,
			String::format( "%s_%d_%u", mFontName.c_str(), characterSize, glyphIndex ) );

		region->setGlyphOffset( { glyph.bounds.Left - outlineThickness,
								  characterSize + glyph.bounds.Top - outlineThickness } );
		region->setAdvance( glyph.advance );
		region->setIsItalic( italic );

		drawables[key] = region;
		return region;
	}
	return nullptr;
}

GlyphDrawable* FontTrueType::getGlyphDrawableFromGlyphIndex( Uint32 glyphIndex,
															 unsigned int characterSize, bool bold,
															 bool italic,
															 Float outlineThickness ) const {
	return getGlyphDrawableFromGlyphIndex( glyphIndex, characterSize, bold, italic,
										   outlineThickness, getPage( characterSize ) );
}

Float FontTrueType::getKerning( Uint32 first, Uint32 second, unsigned int characterSize, bool bold,
								bool italic, Float outlineThickness ) const {
	// Special case where first or second is 0 (null character)
	if ( first == 0 || second == 0 || isMonospace() )
		return 0.f;

	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && setCurrentSize( characterSize ) ) {
		auto glyph1 = getGlyph( first, characterSize, bold, italic, outlineThickness );
		auto glyph2 = getGlyph( second, characterSize, bold, italic, outlineThickness );

		if ( glyph1.font != glyph2.font )
			return 0.f;

		// Convert the characters to indices
		FT_UInt index1 = getGlyphIndex( first );
		FT_UInt index2 = getGlyphIndex( second );

		// Retrieve position compensation deltas generated by FT_LOAD_FORCE_AUTOHINT flag
		auto firstRsbDelta = static_cast<Float>( glyph1.rsbDelta );
		auto secondLsbDelta = static_cast<Float>( glyph2.lsbDelta );

		// Get the kerning vector
		FT_Vector kerning;
		kerning.x = kerning.y = 0;

		if ( glyph1.font == glyph2.font ) {
			if ( FT_HAS_KERNING( face ) )
				FT_Get_Kerning( face, index1, index2, FT_KERNING_UNFITTED, &kerning );

			// X advance is already in pixels for bitmap fonts
			if ( !FT_IS_SCALABLE( face ) )
				return static_cast<Float>( kerning.x );
		}

		// Return the X advance
		return std::floor(
			( secondLsbDelta - firstRsbDelta + static_cast<float>( kerning.x ) + 32 ) /
			static_cast<float>( 1 << 6 ) );
	} else {
		// Invalid font, or no kerning
		return 0.f;
	}
}

Float FontTrueType::getKerningFromGlyphIndex( Uint32 index1, Uint32 index2,
											  unsigned int characterSize, bool bold, bool italic,
											  Float outlineThickness ) const {
	// Special case where first or second is 0 (null character)
	if ( index1 == 0 || index2 == 0 || isMonospace() )
		return 0.f;

	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && setCurrentSize( characterSize ) ) {
		// Retrieve position compensation deltas generated by FT_LOAD_FORCE_AUTOHINT flag
		auto firstRsbDelta = static_cast<float>(
			getGlyphByIndex( index1, characterSize, bold, italic, outlineThickness ).rsbDelta );
		auto secondLsbDelta = static_cast<float>(
			getGlyphByIndex( index2, characterSize, bold, italic, outlineThickness ).lsbDelta );

		// Get the kerning vector
		FT_Vector kerning;
		kerning.x = kerning.y = 0;
		if ( FT_HAS_KERNING( face ) )
			FT_Get_Kerning( face, index1, index2, FT_KERNING_UNFITTED, &kerning );

		// X advance is already in pixels for bitmap fonts
		if ( !FT_IS_SCALABLE( face ) ) {
			return static_cast<Float>( kerning.x );
		}

		// Return the X advance
		Float val =
			std::floor( ( secondLsbDelta - firstRsbDelta + static_cast<float>( kerning.x ) + 32 ) /
						static_cast<float>( 1 << 6 ) );
		return val;
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

Float FontTrueType::getAscent( unsigned int characterSize ) const {
	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && setCurrentSize( characterSize ) ) {
		if ( !FT_IS_SCALABLE( face ) )
			return static_cast<Float>( face->size->metrics.ascender ) /
				   static_cast<Float>( 1 << 6 );

		return static_cast<Float>( FT_MulFix( face->ascender, face->size->metrics.y_scale ) ) /
			   static_cast<Float>( 1 << 6 );
	} else {
		return 0.f;
	}
}

Float FontTrueType::getDescent( unsigned int characterSize ) const {
	FT_Face face = static_cast<FT_Face>( mFace );

	if ( face && setCurrentSize( characterSize ) ) {
		if ( !FT_IS_SCALABLE( face ) )
			return static_cast<Float>( face->size->metrics.descender ) /
				   static_cast<Float>( 1 << 6 );

		return static_cast<Float>( FT_MulFix( -face->descender, face->size->metrics.y_scale ) ) /
			   static_cast<Float>( 1 << 6 );
	} else {
		return 0.f;
	}
}

#define FT_FLOOR( X ) ( ( X & -64 ) / 64 )
#define FT_CEIL( X ) ( ( ( X + 63 ) & -64 ) / 64 )

Uint32 FontTrueType::getFontHeight( const Uint32& characterSize ) const {
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
	return getPage( characterSize ).texture;
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
	std::swap( mInfo, temp.mInfo );
	std::swap( mPages, temp.mPages );
	std::swap( mPixelBuffer, temp.mPixelBuffer );
	return *this;
}

void FontTrueType::cleanup() {
	sendEvent( Event::Unload );

	if ( FontManager::existsSingleton() && FontManager::instance()->getColorEmojiFont() == this )
		FontManager::instance()->setColorEmojiFont( nullptr );

	if ( mFontBoldItalicCb != 0 && mFontBoldItalic != nullptr ) {
		mFontBoldItalic->popFontEventCallback( mFontBoldItalicCb );
		mFontBoldItalicCb = 0;
	}

	if ( mFontBoldCb != 0 && mFontBold != nullptr ) {
		mFontBold->popFontEventCallback( mFontBoldCb );
		mFontBoldCb = 0;
	}

	if ( mFontItalicCb != 0 && mFontItalic != nullptr ) {
		mFontItalic->popFontEventCallback( mFontItalicCb );
		mFontItalicCb = 0;
	}

	mCallbacks.clear();
	mNumCallBacks = 0;

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( mHBFont )
		hb_font_destroy( (hb_font_t*)mHBFont );
#endif

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

	// Reset members
	mLibrary = NULL;
	mFace = NULL;
	mStroker = NULL;
	mStreamRec = NULL;
	mPages.clear();
	std::vector<Uint8>().swap( mPixelBuffer );
}

static int fontSetLoadOptions( FontAntialiasing antialiasing, FontHinting hinting ) {
	int load_target =
		antialiasing == FontAntialiasing::None
			? FT_LOAD_TARGET_MONO
			: ( hinting == FontHinting::Slight ? FT_LOAD_TARGET_LIGHT : FT_LOAD_TARGET_NORMAL );
	int hint = hinting == FontHinting::None ? FT_LOAD_NO_HINTING : FT_LOAD_FORCE_AUTOHINT;
	return load_target | hint;
}

static constexpr FT_Render_Mode
fontSetRenderOptions( FT_Library library, FontAntialiasing antialiasing, FontHinting hinting ) {
	if ( antialiasing == FontAntialiasing::None )
		return FT_RENDER_MODE_MONO;
	if ( antialiasing == FontAntialiasing::Subpixel ) {
		unsigned char weights[] = { 0x10, 0x40, 0x70, 0x40, 0x10 };
		switch ( hinting ) {
			case FontHinting::None:
				FT_Library_SetLcdFilter( library, FT_LCD_FILTER_NONE );
				break;
			case FontHinting::Slight:
			case FontHinting::Full:
				FT_Library_SetLcdFilterWeights( library, weights );
				break;
		}
		return FT_RENDER_MODE_LCD;
	} else {
		switch ( hinting ) {
			case FontHinting::None:
				return FT_RENDER_MODE_NORMAL;
				break;
			case FontHinting::Slight:
				return FT_RENDER_MODE_LIGHT;
				break;
			case FontHinting::Full:
				return FT_RENDER_MODE_LIGHT;
				break;
		}
	}
	return FT_RENDER_MODE_NORMAL;
}

Glyph FontTrueType::loadGlyphByIndex( Uint32 index, unsigned int characterSize, bool bold,
									  bool /*italic*/, Float outlineThickness, Page& page ) const {
	// The glyph to return
	Glyph glyph;

	// First, transform our ugly void* to a FT_Face
	FT_Face face = static_cast<FT_Face>( mFace );
	if ( !face ) {
		Log::error( "FT_Face failed for: codePoint %d characterSize: %d font %s", index,
					characterSize, mFontName.c_str() );
		return glyph;
	}

	// Set the character size
	if ( !setCurrentSize( characterSize ) ) {
		Log::error(
			"FontTrueType::setCurrentSize failed for: codePoint %d characterSize: %d font %s",
			index, characterSize, mFontName.c_str() );
		return glyph;
	}

	FT_Error err = 0;

	auto loadOptions = fontSetLoadOptions( mAntialiasing, mHinting );
	auto renderOptions =
		fontSetRenderOptions( static_cast<FT_Library>( mLibrary ), mAntialiasing, mHinting );

	// Load the glyph corresponding to the code point
	FT_Int32 flags = loadOptions | FT_LOAD_COLOR;
	if ( outlineThickness != 0 && !mIsColorEmojiFont )
		flags |= FT_LOAD_NO_BITMAP;
	if ( ( err = FT_Load_Glyph( face, index, flags ) ) != 0 ) {
		Log::error( "FT_Load_Char failed for: codePoint %d characterSize: %d font: %s error: %d",
					index, characterSize, mFontName.c_str(), err );
		return glyph;
	}

	// Retrieve the glyph
	FT_Glyph glyphDesc;
	FT_GlyphSlot slot = face->glyph;
	if ( FT_Get_Glyph( slot, &glyphDesc ) != 0 ) {
		Log::error( "FT_Get_Glyph failed for: codePoint %d characterSize: %d font: %s", index,
					characterSize, mFontName.c_str() );
		return glyph;
	}

	// Apply bold and outline (there is no fallback for outline) if necessary -- first technique
	// using outline (highest quality)
	FT_Pos weight = 1 << 6;
	bool outline = ( glyphDesc->format == FT_GLYPH_FORMAT_OUTLINE );
	if ( outline ) {
		if ( bold && !mIsBold ) {
			FT_OutlineGlyph outlineGlyph = (FT_OutlineGlyph)glyphDesc;
			FT_Outline_EmboldenXY( &outlineGlyph->outline, 1 << 5, weight );
		}

		if ( outlineThickness != 0 && !mIsColorEmojiFont ) {
			FT_Stroker stroker = static_cast<FT_Stroker>( mStroker );

			FT_Stroker_Set(
				stroker, static_cast<FT_Fixed>( outlineThickness * static_cast<Float>( 1 << 6 ) ),
				FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0 );
			FT_Glyph_Stroke( &glyphDesc, stroker, true );
		}
	}

	// Convert the glyph to a bitmap (i.e. rasterize it)
	FT_Glyph_To_Bitmap( &glyphDesc, renderOptions, 0, 1 );
	FT_Bitmap& bitmap = reinterpret_cast<FT_BitmapGlyph>( glyphDesc )->bitmap;

	// Apply bold if necessary -- fallback technique using bitmap (lower quality)
	if ( !outline ) {
		if ( bold && !mIsBold )
			FT_Bitmap_Embolden( static_cast<FT_Library>( mLibrary ), &bitmap, weight, weight );

		if ( outlineThickness != 0 && !mIsColorEmojiFont )
			Log::error( "Failed to outline glyph (no fallback available)" );
	}

	// Compute the glyph's advance offset
	glyph.advance = static_cast<Float>( slot->metrics.horiAdvance ) / static_cast<Float>( 1 << 6 );

	if ( bold && !mBoldAdvanceSameAsRegular )
		glyph.advance += static_cast<Float>( weight ) / static_cast<Float>( 1 << 6 );

	glyph.lsbDelta = static_cast<int>( slot->lsb_delta );
	glyph.rsbDelta = static_cast<int>( slot->rsb_delta );
	glyph.font = (Font*)this;

	int width = bitmap.width;
	int height = bitmap.rows;

	if ( mAntialiasing == FontAntialiasing::Subpixel && bitmap.pixel_mode == FT_PIXEL_MODE_LCD )
		width /= 3;

	if ( ( width > 0 ) && ( height > 0 ) ) {
		// Leave a small padding around characters, so that filtering doesn't
		// pollute them with pixels from neighbors
		const int padding = 2;

		Float scale = 1.f;

		if ( mIsColorEmojiFont || mIsEmojiFont ) {
			scale = eemin( 1.f, (Float)characterSize / height );
		}

		int destWidth = width;
		int destHeight = height;

		glyph.advance = eeceil( glyph.advance * scale );

		if ( scale >= 1.f ) {
			destWidth *= scale;
			destHeight *= scale;
			width += 2 * padding;
			height += 2 * padding;
		}

		destWidth += 2 * padding;
		destHeight += 2 * padding;

		// Compute the glyph's bounding box
		glyph.bounds.Left =
			static_cast<Float>( slot->metrics.horiBearingX ) / static_cast<Float>( 1 << 6 );
		glyph.bounds.Top =
			-static_cast<Float>( slot->metrics.horiBearingY ) / static_cast<Float>( 1 << 6 );
		glyph.bounds.Right =
			static_cast<Float>( slot->metrics.width ) / static_cast<Float>( 1 << 6 ) +
			outlineThickness * 2;
		glyph.bounds.Bottom =
			static_cast<Float>( slot->metrics.height ) / static_cast<Float>( 1 << 6 ) +
			outlineThickness * 2;

		// Resize the pixel buffer to the new size and fill it with transparent white pixels
		const Uint32 bufferSize = width * height * 4;
		mPixelBuffer.resize( bufferSize );

		Uint8* pixelPtr = &mPixelBuffer[0];
		Uint8* current = pixelPtr;
		Uint8* end = current + bufferSize;

		if ( bitmap.pixel_mode == FT_PIXEL_MODE_LCD ) {
			while ( current != end ) {
				( *current++ ) = 0;
				( *current++ ) = 0;
				( *current++ ) = 0;
				( *current++ ) = 0;
			}
		} else {
			while ( current != end ) {
				( *current++ ) = 255;
				( *current++ ) = 255;
				( *current++ ) = 255;
				( *current++ ) = 0;
			}
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
			Image source( const_cast<Uint8*>( pixels ), bitmap.width, bitmap.rows, 4 );
			Image dest( &mPixelBuffer[0], width, height, 4 );
			source.avoidFreeImage( true );
			dest.avoidFreeImage( true );
			for ( size_t y = 0; y < bitmap.rows; ++y ) {
				for ( size_t x = 0; x < bitmap.width; ++x ) {
					Color col = source.getPixel( x, y );
					dest.setPixel( x, y, Color( col.b, col.g, col.r, col.a ) );
				}
			}

			if ( scale < 1.f ) {
				dest.scale( scale );
				dest.avoidFreeImage( true );
				pixelPtr = dest.getPixels();
				glyph.bounds.Left = glyph.bounds.Left * scale + outlineThickness;
				glyph.bounds.Right *= scale;
				glyph.bounds.Top = glyph.bounds.Top * scale + outlineThickness;
				glyph.bounds.Bottom *= scale;
				destWidth = dest.getWidth() + 2 * padding;
				destHeight = dest.getHeight() + 2 * padding;
			}
		} else if ( bitmap.pixel_mode == FT_PIXEL_MODE_LCD ) {
			for ( int y = padding; y < height - padding; ++y ) {
				for ( int x = padding; x < width - padding; ++x ) {
					const std::size_t index = ( x + y * width ) * 4;
					const Uint8* px = &pixels[( x - padding ) * 3];
					mPixelBuffer[index + 0] = px[0];
					mPixelBuffer[index + 1] = px[1];
					mPixelBuffer[index + 2] = px[2];
					mPixelBuffer[index + 3] =
						(Uint8)( ( (int)px[0] + (int)px[1] + (int)px[2] ) / 3.f );
				}
				pixels += bitmap.pitch;
			}
		} else {
			if ( scale < 1.f ) {
				// Pixels are 8 bits gray levels
				for ( int y = 0; y < height; ++y ) {
					for ( int x = 0; x < width; ++x ) {
						// The color channels remain white, just fill the alpha channel
						std::size_t index = x + y * width;
						mPixelBuffer[index * 4 + 3] = pixels[x];
					}
					pixels += bitmap.pitch;
				}

				Image dest( &mPixelBuffer[0], bitmap.width, bitmap.rows, 4 );
				dest.avoidFreeImage( true );
				dest.scale( scale );
				dest.avoidFreeImage( true );
				pixelPtr = dest.getPixels();
				glyph.bounds.Left = glyph.bounds.Left * scale;
				glyph.bounds.Right *= scale;
				glyph.bounds.Top = glyph.bounds.Top * scale;
				glyph.bounds.Bottom *= scale;
				destWidth = dest.getWidth() + 2 * padding;
				destHeight = dest.getHeight() + 2 * padding;
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
		}

		// Find a good position for the new glyph into the texture
		glyph.textureRect = findGlyphRect( page, destWidth, destHeight );

		// Write the pixels to the texture
		unsigned int x = glyph.textureRect.Left;
		unsigned int y = glyph.textureRect.Top;
		unsigned int w = glyph.textureRect.Right;
		unsigned int h = glyph.textureRect.Bottom;

		if ( scale < 1.f ) {
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

		glyph.size = { (Float)glyph.textureRect.Right, (Float)glyph.textureRect.Bottom };

		page.texture->update( pixelPtr, w, h, x, y );

		if ( scale < 1.f )
			eeSAFE_DELETE_ARRAY( pixelPtr );
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

	// Update the row information
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
					if ( face->num_fixed_sizes > 0 ) {
						std::string str;
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
				} else if ( characterSize != currentSize &&
							( result = FT_Set_Pixel_Sizes( face, 0, it->second ) ) == FT_Err_Ok ) {
					return true;
				}
			}
		}

		return result == FT_Err_Ok;
	} else {
		return true;
	}
}

FontTrueType::Page& FontTrueType::getPage( unsigned int characterSize ) const {
	auto pageIt = mPages.find( characterSize );
	if ( pageIt == mPages.end() ) {
		std::string name =
			String::format( "@font:TrueType:%s:%d", mInfo.family.c_str(), characterSize );
		if ( mIsBold )
			name += ":bold";
		if ( mIsItalic )
			name += ":italic";
		mPages[characterSize] = std::make_unique<Page>( mFontInternalId, name );
		pageIt = mPages.find( characterSize );
	}
	return *pageIt->second;
}

FontAntialiasing FontTrueType::getAntialiasing() const {
	return mAntialiasing;
}

void FontTrueType::setAntialiasing( FontAntialiasing antialiasing ) {
	if ( antialiasing != mAntialiasing ) {
		mAntialiasing = antialiasing;
		clearCache();
	}
}

FontHinting FontTrueType::getHinting() const {
	return mHinting;
}

void FontTrueType::setHinting( FontHinting hinting ) {
	if ( hinting != mHinting ) {
		mHinting = hinting;
		clearCache();
	}
}

bool FontTrueType::getEnableDynamicMonospace() const {
	return mEnableDynamicMonospace;
}

void FontTrueType::setEnableDynamicMonospace( bool enableDynamicMonospace ) {
	mEnableDynamicMonospace = enableDynamicMonospace;
}

bool FontTrueType::isFallbackFontEnabled() const {
	return mEnableFallbackFont;
}

void FontTrueType::setEnableFallbackFont( bool enableFallbackFont ) {
	mEnableFallbackFont = enableFallbackFont;
}

bool FontTrueType::isEmojiFallbackEnabled() const {
	return mEnableEmojiFallback;
}

void FontTrueType::setEnableEmojiFallback( bool enableEmojiFallback ) {
	mEnableEmojiFallback = enableEmojiFallback;
}

const Uint32& FontTrueType::getFontInternalId() const {
	return mFontInternalId;
}

void FontTrueType::setIsEmojiFont( bool isEmojiFont ) {
	mIsEmojiFont = isEmojiFont;
}

void FontTrueType::setForceIsMonospace( bool isMonospace ) {
	mIsMonospaceComplete = isMonospace;
}

void FontTrueType::setIsColorEmojiFont( bool isColorEmojiFont ) {
	mIsColorEmojiFont = isColorEmojiFont;
}

bool FontTrueType::isColorEmojiFont() const {
	return mIsColorEmojiFont;
}

bool FontTrueType::isMonospace() const {
	if ( mIsMonospaceCompletePending )
		updateMonospaceState();
	return mIsMonospaceComplete;
}

bool FontTrueType::isIdentifiedAsMonospace() const {
	return mIsMonospace;
}

bool FontTrueType::isScalable() const {
	return FT_IS_SCALABLE( static_cast<FT_Face>( mFace ) );
}

bool FontTrueType::isEmojiFont() const {
	return mIsEmojiFont;
}

bool FontTrueType::getBoldAdvanceSameAsRegular() const {
	return mBoldAdvanceSameAsRegular;
}

void FontTrueType::setBoldAdvanceSameAsRegular( bool boldAdvanceSameAsRegular ) {
	mBoldAdvanceSameAsRegular = boldAdvanceSameAsRegular;
}

void FontTrueType::updateMonospaceState() const {
	mIsMonospaceComplete = mIsMonospace && !mUsingFallback;
	if ( !Engine::isEngineRunning() || !Engine::instance()->isMainThread() ) {
		mIsMonospaceCompletePending = true;
		return;
	}
	mIsMonospaceCompletePending = false;
	if ( mIsMonospaceComplete && mFontBold != nullptr ) {
		mIsMonospaceComplete = mIsMonospaceComplete && mFontBold->isMonospace() &&
							   getGlyph( ' ', 10, false, false ).advance ==
								   mFontBold->getGlyph( ' ', 10, false, false ).advance;
	}
	if ( mIsMonospaceComplete && mFontItalic != nullptr ) {
		mIsMonospaceComplete = mIsMonospaceComplete && mFontItalic->isMonospace() &&
							   getGlyph( ' ', 10, false, false ).advance ==
								   mFontItalic->getGlyph( ' ', 10, false, false ).advance;
	}
	if ( mIsMonospaceComplete && mFontBoldItalic != nullptr ) {
		mIsMonospaceComplete = mIsMonospaceComplete && mFontBoldItalic->isMonospace() &&
							   getGlyph( ' ', 10, false, false ).advance ==
								   mFontBoldItalic->getGlyph( ' ', 10, false, false ).advance;
	}
}

void FontTrueType::setBoldFont( FontTrueType* fontBold ) {
	if ( fontBold == mFontBold )
		return;
	mFontBold = fontBold;
	if ( mFontBold != nullptr ) {
		mFontBoldCb = mFontBold->pushFontEventCallback( [this]( Uint32, Event event, Font* ) {
			if ( event == Font::Event::Unload ) {
				// Maybe we should recreate the page table
				mFontBold = nullptr;
				mFontBoldCb = 0;
			}
		} );
	}
	updateMonospaceState();
}

void FontTrueType::setItalicFont( FontTrueType* fontItalic ) {
	if ( fontItalic == mFontItalic )
		return;
	mFontItalic = fontItalic;
	if ( mFontItalic != nullptr ) {
		mFontItalicCb = mFontItalic->pushFontEventCallback( [this]( Uint32, Event event, Font* ) {
			if ( event == Font::Event::Unload ) {
				// Maybe we should recreate the page table
				mFontItalic = nullptr;
				mFontItalicCb = 0;
			}
		} );
	}
	updateMonospaceState();
}

void FontTrueType::setBoldItalicFont( FontTrueType* fontBoldItalic ) {
	if ( fontBoldItalic == mFontBoldItalic )
		return;
	mFontBoldItalic = fontBoldItalic;
	if ( mFontBoldItalic != nullptr ) {
		mFontBoldItalicCb =
			mFontBoldItalic->pushFontEventCallback( [this]( Uint32, Event event, Font* ) {
				if ( event == Font::Event::Unload ) {
					// Maybe we should recreate the page table
					mFontBoldItalic = nullptr;
					mFontBoldItalicCb = 0;
				}
			} );
	}
	updateMonospaceState();
}

FontTrueType::Page::Page( const Uint32 fontInternalId, const std::string& pageName ) :
	texture( NULL ), nextRow( 3 ), fontInternalId( fontInternalId ) {
	// Make sure that the texture is initialized by default
	Image image;
	image.create( 128, 128, 4 );

	// Reserve a 2x2 white square for texturing underlines
	for ( int x = 0; x < 2; ++x )
		for ( int y = 0; y < 2; ++y )
			image.setPixel( x, y, Color( 255, 255, 255, 255 ) );

	// Create the texture
	texture = TextureFactory::instance()->loadFromPixels(
		image.getPixelsPtr(), image.getWidth(), image.getHeight(), image.getChannels(), false,
		Texture::ClampMode::ClampToEdge, false, true );
	texture->setCoordinateType( Texture::CoordinateType::Pixels );
	texture->setName( pageName );
}

FontTrueType::Page::~Page() {
	for ( auto drawable : drawables )
		eeDelete( drawable.second );

	if ( NULL != texture && TextureFactory::existsSingleton() )
		TextureFactory::instance()->remove( texture->getTextureId() );
}

void FontTrueType::clearCache() {
	mPages.clear();
	mClosestCharacterSize.clear();
	mCodePointIndexCache.clear();
	mKeyCache.clear();
	Text::GlobalInvalidationId++;
}

}} // namespace EE::Graphics
