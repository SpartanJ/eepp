#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/graphics/texturefactory.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_STROKER_H
#include <cstdlib>
#include <cstring>

namespace {
	// FreeType callbacks that operate on a IOStream
	unsigned long read(FT_Stream rec, unsigned long offset, unsigned char* buffer, unsigned long count) {
		IOStream* stream = static_cast<IOStream*>(rec->descriptor.pointer);
		if (static_cast<unsigned long>(stream->seek(offset)) == offset)
		{
			if (count > 0)
				return static_cast<unsigned long>(stream->read(reinterpret_cast<char*>(buffer), count));
			else
				return 0;
		}
		else
			return count > 0 ? 0 : 1; // error code is 0 if we're reading, or nonzero if we're seeking
	}
	void close(FT_Stream) {
	}
}

namespace EE { namespace Graphics {

FontTrueType::FontTrueType() :
	mLibrary  (NULL),
	mFace     (NULL),
	mStreamRec(NULL),
	mStroker  (NULL),
	mRefCount (NULL),
	mInfo     ()
{
}

FontTrueType::FontTrueType(const FontTrueType& copy) :
	mLibrary    (copy.mLibrary),
	mFace       (copy.mFace),
	mStreamRec  (copy.mStreamRec),
	mStroker    (copy.mStroker),
	mRefCount   (copy.mRefCount),
	mInfo       (copy.mInfo),
	mPages      (copy.mPages),
	mPixelBuffer(copy.mPixelBuffer)
{
	if (mRefCount)
		(*mRefCount)++;
}

FontTrueType::~FontTrueType() {
	cleanup();
}

bool FontTrueType::loadFromFile(const std::string& filename) {
	// Cleanup the previous resources
	cleanup();
	mRefCount = new int(1);

	// Initialize FreeType
	FT_Library library;
	if (FT_Init_FreeType(&library) != 0) {
		std::cout <<  "Failed to load font \"" << filename << "\" (failed to initialize FreeType)" << std::endl;
		return false;
	}
	mLibrary = library;

	// Load the new font face from the specified file
	FT_Face face;
	if (FT_New_Face(static_cast<FT_Library>(mLibrary), filename.c_str(), 0, &face) != 0) {
		std::cout <<  "Failed to load font \"" << filename << "\" (failed to create the font face)" << std::endl;
		return false;
	}

	// Load the stroker that will be used to outline the font
	FT_Stroker stroker;
	if (FT_Stroker_New(static_cast<FT_Library>(mLibrary), &stroker) != 0) {
		std::cout <<  "Failed to load font \"" << filename << "\" (failed to create the stroker)" << std::endl;
		return false;
	}
	mStroker = stroker;

	// Select the unicode character map
	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0) {
		std::cout <<  "Failed to load font \"" << filename << "\" (failed to set the Unicode character set)" << std::endl;
		FT_Done_Face(face);
		return false;
	}

	// Store the loaded font in our ugly void* :)
	mFace = face;

	// Store the font information
	mInfo.family = face->family_name ? face->family_name : std::string();

	return true;
}

bool FontTrueType::loadFromMemory(const void* data, std::size_t sizeInBytes) {
	// Cleanup the previous resources
	cleanup();
	mRefCount = new int(1);

	// Initialize FreeType
	FT_Library library;
	if (FT_Init_FreeType(&library) != 0) {
		std::cout <<  "Failed to load font from memory (failed to initialize FreeType)" << std::endl;
		return false;
	}
	mLibrary = library;

	// Load the new font face from the specified file
	FT_Face face;
	if (FT_New_Memory_Face(static_cast<FT_Library>(mLibrary), reinterpret_cast<const FT_Byte*>(data), static_cast<FT_Long>(sizeInBytes), 0, &face) != 0) {
		std::cout <<  "Failed to load font from memory (failed to create the font face)" << std::endl;
		return false;
	}

	// Load the stroker that will be used to outline the font
	FT_Stroker stroker;
	if (FT_Stroker_New(static_cast<FT_Library>(mLibrary), &stroker) != 0) {
		std::cout <<  "Failed to load font from memory (failed to create the stroker)" << std::endl;
		return false;
	}
	mStroker = stroker;

	// Select the Unicode character map
	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0) {
		std::cout <<  "Failed to load font from memory (failed to set the Unicode character set)" << std::endl;
		FT_Done_Face(face);
		return false;
	}

	// Store the loaded font in our ugly void* :)
	mFace = face;

	// Store the font information
	mInfo.family = face->family_name ? face->family_name : std::string();

	return true;
}

bool FontTrueType::loadFromStream(IOStream& stream) {
	// Cleanup the previous resources
	cleanup();
	mRefCount = new int(1);

	// Initialize FreeType
	FT_Library library;
	if (FT_Init_FreeType(&library) != 0) {
		std::cout <<  "Failed to load font from stream (failed to initialize FreeType)" << std::endl;
		return false;
	}
	mLibrary = library;

	// Make sure that the stream's reading position is at the beginning
	stream.seek(0);

	// Prepare a wrapper for our stream, that we'll pass to FreeType callbacks
	FT_StreamRec* rec = new FT_StreamRec;
	std::memset(rec, 0, sizeof(*rec));
	rec->base               = NULL;
	rec->size               = static_cast<unsigned long>(stream.getSize());
	rec->pos                = 0;
	rec->descriptor.pointer = &stream;
	rec->read               = &read;
	rec->close              = &close;

	// Setup the FreeType callbacks that will read our stream
	FT_Open_Args args;
	args.flags  = FT_OPEN_STREAM;
	args.stream = rec;
	args.driver = 0;

	// Load the new font face from the specified stream
	FT_Face face;
	if (FT_Open_Face(static_cast<FT_Library>(mLibrary), &args, 0, &face) != 0) {
		std::cout <<  "Failed to load font from stream (failed to create the font face)" << std::endl;
		delete rec;
		return false;
	}

	// Load the stroker that will be used to outline the font
	FT_Stroker stroker;
	if (FT_Stroker_New(static_cast<FT_Library>(mLibrary), &stroker) != 0) {
		std::cout <<  "Failed to load font from stream (failed to create the stroker)" << std::endl;
		return false;
	}
	mStroker = stroker;

	// Select the Unicode character map
	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0) {
		std::cout <<  "Failed to load font from stream (failed to set the Unicode character set)" << std::endl;
		FT_Done_Face(face);
		delete rec;
		return false;
	}

	// Store the loaded font in our ugly void* :)
	mFace = face;
	mStreamRec = rec;

	// Store the font information
	mInfo.family = face->family_name ? face->family_name : std::string();

	return true;
}

const FontTrueType::Info& FontTrueType::getInfo() const {
	return mInfo;
}

const Glyph& FontTrueType::getGlyph(Uint32 codePoint, unsigned int characterSize, bool bold, Float outlineThickness) const {
	// Get the page corresponding to the character size
	GlyphTable& glyphs = mPages[characterSize].glyphs;

	// Build the key by combining the code point, bold flag, and outline thickness
	Uint64 key = (static_cast<Uint64>(*reinterpret_cast<Uint32*>(&outlineThickness)) << 32)
			   | (static_cast<Uint64>(bold ? 1 : 0) << 31)
			   |  static_cast<Uint64>(codePoint);

	// Search the glyph into the cache
	GlyphTable::const_iterator it = glyphs.find(key);
	if (it != glyphs.end()) {
		// Found: just return it
		return it->second;
	} else {
		// Not found: we have to load it
		Glyph glyph = loadGlyph(codePoint, characterSize, bold, outlineThickness);
		return glyphs.insert(std::make_pair(key, glyph)).first->second;
	}
}

Float FontTrueType::getKerning(Uint32 first, Uint32 second, unsigned int characterSize) const {
	// Special case where first or second is 0 (null character)
	if (first == 0 || second == 0)
		return 0.f;

	FT_Face face = static_cast<FT_Face>(mFace);

	if (face && FT_HAS_KERNING(face) && setCurrentSize(characterSize)) {
		// Convert the characters to indices
		FT_UInt index1 = FT_Get_Char_Index(face, first);
		FT_UInt index2 = FT_Get_Char_Index(face, second);

		// Get the kerning vector
		FT_Vector kerning;
		FT_Get_Kerning(face, index1, index2, FT_KERNING_DEFAULT, &kerning);

		// X advance is already in pixels for bitmap fonts
		if (!FT_IS_SCALABLE(face))
			return static_cast<Float>(kerning.x);

		// Return the X advance
		return static_cast<Float>(kerning.x) / static_cast<Float>(1 << 6);
	} else {
		// Invalid font, or no kerning
		return 0.f;
	}
}

Float FontTrueType::getLineSpacing(unsigned int characterSize) const {
	FT_Face face = static_cast<FT_Face>(mFace);

	if (face && setCurrentSize(characterSize)) {
		return static_cast<Float>(face->size->metrics.height) / static_cast<Float>(1 << 6);
	} else {
		return 0.f;
	}
}

Float FontTrueType::getUnderlinePosition(unsigned int characterSize) const {
	FT_Face face = static_cast<FT_Face>(mFace);

	if (face && setCurrentSize(characterSize)) {
		// Return a fixed position if font is a bitmap font
		if (!FT_IS_SCALABLE(face))
			return characterSize / 10.f;

		return -static_cast<Float>(FT_MulFix(face->underline_position, face->size->metrics.y_scale)) / static_cast<Float>(1 << 6);
	} else {
		return 0.f;
	}
}

Float FontTrueType::getUnderlineThickness(unsigned int characterSize) const {
	FT_Face face = static_cast<FT_Face>(mFace);

	if (face && setCurrentSize(characterSize)) {
		// Return a fixed thickness if font is a bitmap font
		if (!FT_IS_SCALABLE(face))
			return characterSize / 14.f;

		return static_cast<Float>(FT_MulFix(face->underline_thickness, face->size->metrics.y_scale)) / static_cast<Float>(1 << 6);
	}
	else {
		return 0.f;
	}
}

Texture* FontTrueType::getTexture(unsigned int characterSize) const {
	return mPages[characterSize].texture;
}

FontTrueType& FontTrueType::operator =(const FontTrueType& right) {
	FontTrueType temp(right);

	std::swap(mLibrary,     temp.mLibrary);
	std::swap(mFace,        temp.mFace);
	std::swap(mStreamRec,   temp.mStreamRec);
	std::swap(mStroker,     temp.mStroker);
	std::swap(mRefCount,    temp.mRefCount);
	std::swap(mInfo,        temp.mInfo);
	std::swap(mPages,       temp.mPages);
	std::swap(mPixelBuffer, temp.mPixelBuffer);
	return *this;
}

void FontTrueType::cleanup() {
	// Check if we must destroy the FreeType pointers
	if (mRefCount) {
		// Decrease the reference counter
		(*mRefCount)--;

		// Free the resources only if we are the last owner
		if (*mRefCount == 0)
		{
			// Delete the reference counter
			delete mRefCount;

			// Destroy the stroker
			if (mStroker)
				FT_Stroker_Done(static_cast<FT_Stroker>(mStroker));

			// Destroy the font face
			if (mFace)
				FT_Done_Face(static_cast<FT_Face>(mFace));

			// Destroy the stream rec instance, if any (must be done after FT_Done_Face!)
			if (mStreamRec)
				delete static_cast<FT_StreamRec*>(mStreamRec);

			// Close the library
			if (mLibrary)
				FT_Done_FreeType(static_cast<FT_Library>(mLibrary));
		}
	}

	// Reset members
	mLibrary   = NULL;
	mFace      = NULL;
	mStroker   = NULL;
	mStreamRec = NULL;
	mRefCount  = NULL;
	mPages.clear();
	mPixelBuffer.clear();
}

Glyph FontTrueType::loadGlyph(Uint32 codePoint, unsigned int characterSize, bool bold, Float outlineThickness) const {
	// The glyph to return
	Glyph glyph;

	// First, transform our ugly void* to a FT_Face
	FT_Face face = static_cast<FT_Face>(mFace);
	if (!face)
		return glyph;

	// Set the character size
	if (!setCurrentSize(characterSize))
		return glyph;

	// Load the glyph corresponding to the code point
	FT_Int32 flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT;
	if (outlineThickness != 0)
		flags |= FT_LOAD_NO_BITMAP;
	if (FT_Load_Char(face, codePoint, flags) != 0)
		return glyph;

	// Retrieve the glyph
	FT_Glyph glyphDesc;
	if (FT_Get_Glyph(face->glyph, &glyphDesc) != 0)
		return glyph;

	// Apply bold and outline (there is no fallback for outline) if necessary -- first technique using outline (highest quality)
	FT_Pos weight = 1 << 6;
	bool outline = (glyphDesc->format == FT_GLYPH_FORMAT_OUTLINE);
	if (outline) {
		if (bold)
		{
			FT_OutlineGlyph outlineGlyph = (FT_OutlineGlyph)glyphDesc;
			FT_Outline_Embolden(&outlineGlyph->outline, weight);
		}

		if (outlineThickness != 0)
		{
			FT_Stroker stroker = static_cast<FT_Stroker>(mStroker);

			FT_Stroker_Set(stroker, static_cast<FT_Fixed>(outlineThickness * static_cast<Float>(1 << 6)), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
			FT_Glyph_Stroke(&glyphDesc, stroker, false);
		}
	}

	// Convert the glyph to a bitmap (i.e. rasterize it)
	FT_Glyph_To_Bitmap(&glyphDesc, FT_RENDER_MODE_NORMAL, 0, 1);
	FT_Bitmap& bitmap = reinterpret_cast<FT_BitmapGlyph>(glyphDesc)->bitmap;

	// Apply bold if necessary -- fallback technique using bitmap (lower quality)
	if (!outline) {
		if (bold)
			FT_Bitmap_Embolden(static_cast<FT_Library>(mLibrary), &bitmap, weight, weight);

		if (outlineThickness != 0)
			std::cout <<  "Failed to outline glyph (no fallback available)" << std::endl;
	}

	// Compute the glyph's advance offset
	glyph.advance = static_cast<Float>(face->glyph->metrics.horiAdvance) / static_cast<Float>(1 << 6);
	if (bold)
		glyph.advance += static_cast<Float>(weight) / static_cast<Float>(1 << 6);

	int width  = bitmap.width;
	int height = bitmap.rows;

	if ((width > 0) && (height > 0)) {
		// Leave a small padding around characters, so that filtering doesn't
		// pollute them with pixels from neighbors
		const unsigned int padding = 1;

		// Get the glyphs page corresponding to the character size
		Page& page = mPages[characterSize];

		// Find a good position for the new glyph into the texture
		glyph.textureRect = findGlyphRect(page, width + 2 * padding, height + 2 * padding);

		// Make sure the texture data is positioned in the center
		// of the allocated texture rectangle
		glyph.textureRect.Left += padding;
		glyph.textureRect.Top += padding;
		glyph.textureRect.Right -= 2 * padding;
		glyph.textureRect.Bottom -= 2 * padding;

		// Compute the glyph's bounding box
		glyph.bounds.Left   =  static_cast<Float>(face->glyph->metrics.horiBearingX) / static_cast<Float>(1 << 6);
		glyph.bounds.Top    = -static_cast<Float>(face->glyph->metrics.horiBearingY) / static_cast<Float>(1 << 6);
		glyph.bounds.Right  =  static_cast<Float>(face->glyph->metrics.width)        / static_cast<Float>(1 << 6) + outlineThickness * 2;
		glyph.bounds.Bottom =  static_cast<Float>(face->glyph->metrics.height)       / static_cast<Float>(1 << 6) + outlineThickness * 2;

		// Extract the glyph's pixels from the bitmap
		mPixelBuffer.resize(width * height * 4, 255);
		const Uint8* pixels = bitmap.buffer;
		if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
		{
			// Pixels are 1 bit monochrome values
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					// The color channels remain white, just fill the alpha channel
					std::size_t index = (x + y * width) * 4 + 3;
					mPixelBuffer[index] = ((pixels[x / 8]) & (1 << (7 - (x % 8)))) ? 255 : 0;
				}
				pixels += bitmap.pitch;
			}
		}
		else
		{
			// Pixels are 8 bits gray levels
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					// The color channels remain white, just fill the alpha channel
					std::size_t index = (x + y * width) * 4 + 3;
					mPixelBuffer[index] = pixels[x];
				}
				pixels += bitmap.pitch;
			}
		}

		// Write the pixels to the texture
		unsigned int x = glyph.textureRect.Left;
		unsigned int y = glyph.textureRect.Top;
		unsigned int w = glyph.textureRect.Right;
		unsigned int h = glyph.textureRect.Bottom;
		page.texture->update(&mPixelBuffer[0], w, h, x, y);
	}

	// Delete the FT glyph
	FT_Done_Glyph(glyphDesc);

	// Done :)
	return glyph;
}

Recti FontTrueType::findGlyphRect(Page& page, unsigned int width, unsigned int height) const {
	// Find the line that fits well the glyph
	Row* row = NULL;
	Float bestRatio = 0;
	for (std::vector<Row>::iterator it = page.rows.begin(); it != page.rows.end() && !row; ++it) {
		Float ratio = static_cast<Float>(height) / it->height;

		// Ignore rows that are either too small or too high
		if ((ratio < 0.7f) || (ratio > 1.f))
			continue;

		// Check if there's enough horizontal space left in the row
		if (width > page.texture->getSize().x - it->width)
			continue;

		// Make sure that this new row is the best found so far
		if (ratio < bestRatio)
			continue;

		// The current row passed all the tests: we can select it
		row = &*it;
		bestRatio = ratio;
	}

	// If we didn't find a matching row, create a new one (10% taller than the glyph)
	if (!row) {
		int rowHeight = height + height / 10;
		while ((page.nextRow + rowHeight >= (Uint32)page.texture->getSize().y) || (width >= (Uint32)page.texture->getSize().x))
		{
			// Not enough space: resize the texture if possible
			unsigned int textureWidth  = page.texture->getSize().x;
			unsigned int textureHeight = page.texture->getSize().y;
			if ( ( textureWidth * 2 <= Texture::getMaximumSize()) && (textureHeight * 2 <= Texture::getMaximumSize() ) ) {
				// Make the texture 2 times bigger

				//page.texture->lock();
				Image newImage;
				newImage.create(textureWidth * 2, textureHeight * 2, 4);
				newImage.copyImage(page.texture, 0, 0);
				//page.texture->unlock();

				page.texture->replace(&newImage);			} else {
				// Oops, we've reached the maximum texture size...
				std::cout <<  "Failed to add a new character to the font: the maximum texture size has been reached" << std::endl;
				return Recti(0, 0, 2, 2);
			}
		}

		// We can now create the new row
		page.rows.push_back(Row(page.nextRow, rowHeight));
		page.nextRow += rowHeight;
		row = &page.rows.back();
	}

	// Find the glyph's rectangle on the selected row
	Recti rect(row->width, row->top, width, height);

	// Update the row informations
	row->width += width;

	return rect;
}

bool FontTrueType::setCurrentSize(unsigned int characterSize) const {
	// FT_Set_Pixel_Sizes is an expensive function, so we must call it
	// only when necessary to avoid killing performances

	FT_Face face = static_cast<FT_Face>(mFace);
	FT_UShort currentSize = face->size->metrics.x_ppem;

	if (currentSize != characterSize) {
		FT_Error result = FT_Set_Pixel_Sizes(face, 0, characterSize);

		if (result == FT_Err_Invalid_Pixel_Size)
		{
			// In the case of bitmap fonts, resizing can
			// fail if the requested size is not available
			if (!FT_IS_SCALABLE(face))
			{
				std::cout <<  "Failed to set bitmap font size to " << characterSize << std::endl;
				std::cout <<  "Available sizes are: ";
				for (int i = 0; i < face->num_fixed_sizes; ++i)
					std::cout <<  face->available_sizes[i].height << " ";
				std::cout <<  std::endl;
			}
		}

		return result == FT_Err_Ok;
	} else {
		return true;
	}
}

FontTrueType::Page::Page() :
	texture(NULL),
	nextRow(3)
{	
	// Make sure that the texture is initialized by default
	Image image;
	image.create(128, 128, 4);

	// Reserve a 2x2 white square for texturing underlines
	for (int x = 0; x < 2; ++x)
		for (int y = 0; y < 2; ++y)
			image.setPixel(x, y, ColorA(255, 255, 255, 255));

	// Create the texture
	Uint32 texId = TextureFactory::instance()->loadFromPixels( image.getPixelsPtr(), image.getWidth(), image.getHeight(), image.getChannels(), false, CLAMP_TO_EDGE, false, true );
	texture = TextureFactory::instance()->getTexture( texId );
}

FontTrueType::Page::~Page() {
	if ( NULL != texture )
		TextureFactory::instance()->remove( texture->getId() );
}

void FontTrueType::cacheWidth( const String& Text, const Uint32& characterSize, bool bold, Float outlineThickness, std::vector<Float>& LinesWidth, Float& CachedWidth, int& NumLines , int& LargestLineCharCount ) {
	LinesWidth.clear();

	Float Width = 0, MaxWidth = 0;
	Int32 CharID;
	Int32 Lines = 1;
	Int32 CharCount = 0;
	LargestLineCharCount = 0;

	for (std::size_t i = 0; i < Text.size(); ++i) {
		CharID = static_cast<Int32>( Text.at(i) );
		Glyph glyph = getGlyph( CharID, characterSize, bold, outlineThickness );

		Width += glyph.advance;

		CharCount++;

		if ( CharID == '\t' )
			Width += glyph.advance * 3;

		if ( CharID == '\n' ) {
			Lines++;

			Float lWidth = ( CharID == '\t' ) ? glyph.advance * 4.f : glyph.advance;

			LinesWidth.push_back( Width - lWidth );

			Width = 0;

			CharCount = 0;
		} else {
			if ( CharCount > LargestLineCharCount )
				LargestLineCharCount = CharCount;
		}

		if ( Width > MaxWidth )
			MaxWidth = Width;
	}

	if ( Text.size() && Text.at( Text.size() - 1 ) != '\n' ) {
		LinesWidth.push_back( Width );
	}

	CachedWidth = MaxWidth;
	NumLines = Lines;
}

Int32 FontTrueType::findClosestCursorPosFromPoint( const String& Text, const Uint32& characterSize, bool bold, Float outlineThickness, const Vector2i& pos ) {
	Float Width = 0, lWidth = 0, Height = getLineSpacing(characterSize), lHeight = 0;
	Int32 CharID;
	std::size_t tSize = Text.size();

	for (std::size_t i = 0; i < tSize; ++i) {
		CharID = static_cast<Int32>( Text.at(i) );
		Glyph glyph = getGlyph( CharID, characterSize, bold, outlineThickness );

		lWidth = Width;

		Width += glyph.advance;

		if ( CharID == '\t' ) {
			Width += glyph.advance * 3;
		}

		if ( CharID == '\n' ) {
			lWidth = 0;
			Width = 0;
		}

		if ( pos.x <= Width && pos.x >= lWidth && pos.y <= Height && pos.y >= lHeight ) {
			if ( i + 1 < tSize ) {
				Int32 curDist	= eeabs( pos.x - lWidth );
				Int32 nextDist	= eeabs( pos.x - ( lWidth + glyph.advance ) );

				if ( nextDist < curDist ) {
					return  i + 1;
				}
			}

			return i;
		}

		if ( CharID == '\n' ) {
			lHeight = Height;
			Height += getLineSpacing(characterSize);

			if ( pos.x > Width && pos.y <= lHeight ) {
				return i;
			}
		}
	}

	if ( pos.x >= Width ) {
		return tSize;
	}

	return -1;
}

Vector2i FontTrueType::getCursorPos( const String& Text, const Uint32& characterSize, bool bold, Float outlineThickness, const Uint32& Pos ) {
	Float Width = 0, Height = getLineSpacing(characterSize);
	Int32 CharID;
	std::size_t tSize = ( Pos < Text.size() ) ? Pos : Text.size();

	for (std::size_t i = 0; i < tSize; ++i) {
		CharID = static_cast<Int32>( Text.at(i) );
		Glyph glyph = getGlyph( CharID, characterSize, bold, outlineThickness );

		Width += glyph.advance;

		if ( CharID == '\t' ) {
			Width += glyph.advance * 3;
		}

		if ( CharID == '\n' ) {
			Width = 0;
			Height += getLineSpacing(characterSize);
		}
	}

	return Vector2i( Width, Height );
}

static bool isStopSelChar( Uint32 c ) {
	return ( !String::isCharacter( c ) && !String::isNumber( c ) ) ||
			' ' == c ||
			'.' == c ||
			',' == c ||
			';' == c ||
			':' == c ||
			'\n' == c ||
			'"' == c ||
			'\'' == c;
}

void FontTrueType::selectSubStringFromCursor( const String& Text, const Uint32& characterSize, bool bold, Float outlineThickness, const Int32& CurPos, Int32& InitCur, Int32& EndCur ) {
	InitCur	= 0;
	EndCur	= Text.size();

	for ( std::size_t i = CurPos; i < Text.size(); i++ ) {
		if ( isStopSelChar( Text[i] ) ) {
			EndCur = i;
			break;
		}
	}

	if ( 0 == CurPos ) {
		InitCur = 0;
	}

	for ( Int32 i = CurPos; i >= 0; i-- ) {
		if ( isStopSelChar( Text[i] ) ) {
			InitCur = i + 1;
			break;
		}
	}

	if ( InitCur == EndCur ) {
		InitCur = EndCur = -1;
	}
}

void FontTrueType::shrinkText( std::string& Str, const Uint32& characterSize, bool bold, Float outlineThickness, const Uint32& MaxWidth ) {
	if ( !Str.size() )
		return;

	Float tCurWidth = 0.f;
	Float tWordWidth = 0.f;
	Float tMaxWidth = (Float) MaxWidth;
	char * tChar = &Str[0];
	char * tLastSpace = NULL;

	while ( *tChar ) {
		Glyph pChar = getGlyph( *tChar, characterSize, bold, outlineThickness );
		Float fCharWidth	= (Float)pChar.advance;

		if ( ( *tChar ) == '\t' )
			fCharWidth += pChar.advance * 3;

		tWordWidth		+= fCharWidth;

		if ( ' ' == *tChar || '\0' == *( tChar + 1 ) ) {
			if ( tCurWidth + tWordWidth < tMaxWidth ) {
				tCurWidth		+= tWordWidth;
				tLastSpace		= tChar;

				tChar++;
			} else {
				if ( NULL != tLastSpace ) {
					*tLastSpace		= '\n';
					tChar	= tLastSpace + 1;
				} else {
					*tChar	= '\n';
				}

				if ( '\0' == *( tChar + 1 ) )
					tChar++;

				tLastSpace		= NULL;
				tCurWidth		= 0.f;
			}

			tWordWidth = 0.f;
		} else if ( '\n' == *tChar ) {
			tWordWidth 		= 0.f;
			tCurWidth 		= 0.f;
			tLastSpace		= NULL;
			tChar++;
		} else {
			tChar++;
		}
	}
}

void FontTrueType::shrinkText( String& Str, const Uint32& characterSize, bool bold, Float outlineThickness, const Uint32& MaxWidth ) {
	if ( !Str.size() )
		return;

	Float tCurWidth = 0.f;
	Float tWordWidth = 0.f;
	Float tMaxWidth = (Float) MaxWidth;
	String::StringBaseType * tChar = &Str[0];
	String::StringBaseType * tLastSpace = NULL;

	while ( *tChar ) {
		Glyph pChar = getGlyph( *tChar, characterSize, bold, outlineThickness );

		Float fCharWidth	= (Float)pChar.advance;

		if ( ( *tChar ) == '\t' )
			fCharWidth += pChar.advance * 3;

		// Add the new char width to the current word width
		tWordWidth		+= fCharWidth;

		if ( ' ' == *tChar || '\0' == *( tChar + 1 ) ) {

			// If current width plus word width is minor to the max width, continue adding
			if ( tCurWidth + tWordWidth < tMaxWidth ) {
				tCurWidth		+= tWordWidth;
				tLastSpace		= tChar;

				tChar++;
			} else {
				// If it was an space before, replace that space for an new line
				// Start counting from the new line first character
				if ( NULL != tLastSpace ) {
					*tLastSpace		= '\n';
					tChar	= tLastSpace + 1;
				} else {	// The word is larger than the current possible width
					*tChar	= '\n';
				}

				if ( '\0' == *( tChar + 1 ) )
					tChar++;

				// Set the last spaces as null, because is a new line
				tLastSpace		= NULL;

				// New line, new current width
				tCurWidth		= 0.f;
			}

			// New word, so we reset the current word width
			tWordWidth = 0.f;
		} else if ( '\n' == *tChar ) {
			tWordWidth 		= 0.f;
			tCurWidth 		= 0.f;
			tLastSpace		= NULL;
			tChar++;
		} else {
			tChar++;
		}
	}
}

}}
