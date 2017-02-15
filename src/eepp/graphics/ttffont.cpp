#include <eepp/graphics/ttffont.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/helper/haikuttf/haikuttf.hpp>
using namespace HaikuTTF;

namespace EE { namespace Graphics {

TTFFont::OutlineMethods TTFFont::OutlineMethod = TTFFont::OutlineEntropia;

TTFFont * TTFFont::New( const std::string FontName ) {
	return eeNew( TTFFont, ( FontName ) );
}

TTFFont::TTFFont( const std::string FontName ) :
	Font( FONT_TYPE_TTF, FontName ),
	mFont(NULL),
	mFontOutline(NULL),
	mPixels(NULL),
	mThreadedLoading(false),
	mTexReady(false)
{
}

TTFFont::~TTFFont() {
	hkFontManager::instance()->Destroy();
}

bool TTFFont::LoadFromPack( Pack* Pack, const std::string& FilePackPath, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) {
	bool Ret = false;
	SafeDataPointer PData;

	if ( Pack->isOpen() && Pack->extractFileToMemory( FilePackPath, PData ) ) {
		mFilepath = FilePackPath;

		Ret = LoadFromMemory( PData.Data, PData.DataSize, Size, Style, NumCharsToGen, FontColor, OutlineSize, OutlineColor, AddPixelSeparator );
	}

	return Ret;
}

bool TTFFont::LoadFromMemory( Uint8* TTFData, const unsigned int& TTFDataSize, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) {
	if ( !mFilepath.size() )
		mFilepath = "from memory";

	mLoadedFromMemory = true;

	mFont = hkFontManager::instance()->OpenFromMemory( reinterpret_cast<Uint8*>(&TTFData[0]), TTFDataSize, Size, 0, NumCharsToGen );

	if ( OutlineSize && OutlineFreetype == OutlineMethod ) {
		mFontOutline = hkFontManager::instance()->OpenFromMemory( reinterpret_cast<Uint8*>(&TTFData[0]), TTFDataSize, Size, 0, NumCharsToGen );
		mFontOutline->Outline( OutlineSize );
	}

	return iLoad( Size, Style, NumCharsToGen, FontColor, OutlineSize, OutlineColor, AddPixelSeparator );
}

bool TTFFont::Load( const std::string& Filepath, const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, const Uint8& OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) {
	mFilepath			= Filepath;

	if ( FileSystem::fileExists( Filepath ) ) {
		mLoadedFromMemory	= false;

		mFont = hkFontManager::instance()->OpenFromFile( Filepath.c_str(), Size, 0, NumCharsToGen );

		if ( OutlineSize && OutlineFreetype == OutlineMethod ) {
			mFontOutline = hkFontManager::instance()->OpenFromFile( Filepath.c_str(), Size, 0, NumCharsToGen );
			mFontOutline->Outline( OutlineSize );
		}

		return iLoad( Size, Style, NumCharsToGen, FontColor, OutlineSize, OutlineColor, AddPixelSeparator );
	} else if ( PackManager::instance()->fallbackToPacks() ) {
		Pack * tPack = PackManager::instance()->exists( mFilepath );

		if ( NULL != tPack ) {
			return LoadFromPack( tPack, mFilepath, Size, Style, NumCharsToGen, FontColor, OutlineSize, OutlineColor, AddPixelSeparator );
		}
	}

	return false;
}

bool TTFFont::iLoad( const unsigned int& Size, EE_TTF_FONT_STYLE Style, const Uint16& NumCharsToGen, const RGB& FontColor, Uint8 OutlineSize, const RGB& OutlineColor, const bool& AddPixelSeparator ) {
	Rect CurrentPos;
	Sizei GlyphRect;

	unsigned char * TempGlyphSurface = NULL;
	unsigned char * TempOutGlyphSurface = NULL;

	// Change the outline size to add a pixel separating the character from the around characters to prevent ugly zooming of characters
	Uint32 PixelSep = 0;

	if ( AddPixelSeparator )
		PixelSep = 1;

	Uint32 TexSize;
	Uint32 OutSize		 = ( OutlineFreetype == OutlineMethod ) ? 0 : OutlineSize;
	Uint32 OutTotal		 = ( OutlineFreetype == OutlineMethod ) ? 0 : OutlineSize * 2;

	if ( mFont == NULL ) {
		eePRINTL( "Failed to load TTF Font %s.", mFilepath.c_str() );

		return false;
	}

	mSize 			= Size;

	mFont->Style( Style );

	mHeight 		= mFont->Height()+ OutTotal;
	mLineSkip		= mFont->LineSkip();
	mAscent			= mFont->Ascent();
	mDescent		= mFont->Descent();

	if ( NULL != mFontOutline )
		mHeight		= mFontOutline->Height();

	mNumChars 		= NumCharsToGen;
	mFontColor 		= FontColor;
	mOutlineColor 	= OutlineColor;
	mStyle 			= Style;
	mTexWidth 		= 128;
	mTexHeight 		= 128;

	mGlyphs.clear();
	mGlyphs.resize( mNumChars );

	bool lastWasWidth = false;
	Uint32 ReqSize;

	// Find the best size for the texture ( aprox )
	// Totally wild guessing, but it's working
	Int32 tWildGuessW = ( mAscent + PixelSep + OutlineSize );
	Int32 tWildGuessH = tWildGuessW;

	ReqSize = mNumChars * tWildGuessW * tWildGuessH;

	do {
		TexSize = (Uint32)mTexWidth * (Uint32)mTexHeight;

		if ( TexSize < ReqSize ) {
			if ( !lastWasWidth )
				mTexWidth *= 2;
			else
				mTexHeight *= 2;

			lastWasWidth = !lastWasWidth;
		}
	} while ( TexSize < ReqSize  );

	mPixels = eeNewArray( ColorA, TexSize );
	memset( mPixels, 0x00000000, TexSize * 4 );

	CurrentPos.Left = OutSize;
	CurrentPos.Top 	= OutSize;

	Uint32 * TexGlyph;
	Uint32 w = (Uint32)mTexWidth;
	//Uint32 h = (Uint32)mTexHeight;
	ColorA fFontColor( FontColor );

	//Loop through all chars
	for ( unsigned int i = 0; i < mNumChars; i++ ) {
		TempGlyphSurface = mFont->GlyphRender( i, fFontColor.getValue() );

		//New temp glyph
		eeGlyph TempGlyph;

		//Get the glyph attributes
		mFont->GlyphMetrics( i, &TempGlyph.MinX, &TempGlyph.MaxX, &TempGlyph.MinY, &TempGlyph.MaxY, &TempGlyph.Advance );

		//Set size of glyph rect
		GlyphRect.x = mFont->Current()->Pixmap()->width;
		GlyphRect.y = mFont->Current()->Pixmap()->rows;

		// Create the outline for the glyph and copy the outline to the texture
		if ( OutlineSize && OutlineFreetype == OutlineMethod ) {
			TempOutGlyphSurface = mFontOutline->GlyphRender( i, ColorA( OutlineColor ).getValue() );

			mFontOutline->GlyphMetrics( i, &TempGlyph.MinX, &TempGlyph.MaxX, &TempGlyph.MinY, &TempGlyph.MaxY, &TempGlyph.Advance );

			// Set size of glyph rect
			GlyphRect.x = mFontOutline->Current()->Pixmap()->width;
			GlyphRect.y = mFontOutline->Current()->Pixmap()->rows;

			// Fix to ensure that the glyph is rendered with the real size
			if ( eeabs( TempGlyph.MaxX - TempGlyph.MinX ) != GlyphRect.x ) {
				TempGlyph.MaxX = TempGlyph.MinX + GlyphRect.x;
			}

			Image out( TempOutGlyphSurface, GlyphRect.x, GlyphRect.y, 4 ); out.AvoidFreeImage( true );
			Image in( TempGlyphSurface, mFont->Current()->Pixmap()->width, mFont->Current()->Pixmap()->rows, 4 ); in.AvoidFreeImage( true );

			Uint32 px = ( ( (Float)out.Width()	- (Float)in.Width() )		* 0.5f );
			Uint32 py = ( ( (Float)out.Height()	- (Float)in.Height() )	* 0.5f );

			out.Blit( &in, px, py );

			TexGlyph = reinterpret_cast<Uint32 *> ( TempOutGlyphSurface );
		} else {
			TexGlyph = reinterpret_cast<Uint32 *> ( TempGlyphSurface );
		}

		//Set size of current position rect
		CurrentPos.Right 	= CurrentPos.Left	+ TempGlyph.MaxX;
		CurrentPos.Bottom 	= CurrentPos.Top	+ TempGlyph.MaxY;

		if ( CurrentPos.Right >= mTexWidth ) {
			CurrentPos.Left = OutSize;
			CurrentPos.Top += mHeight;
		}

		// Copy the glyph to the texture
		for (int y = 0; y < GlyphRect.y; ++y ) {
			// Copy per row
			memcpy( &mPixels[ CurrentPos.Left + (CurrentPos.Top + y) * w ], &TexGlyph[ y * GlyphRect.x ], GlyphRect.x * sizeof(ColorA) );
		}

		// Fixes the width and height of the current pos
		CurrentPos.Right 	= GlyphRect.x;
		CurrentPos.Bottom 	= GlyphRect.y;

		GlyphRect.y			+= OutSize;
		TempGlyph.Advance	+= OutSize;

		// Translate the Glyph coordinates to the new texture coordinates
		TempGlyph.MinX		-= OutSize;
		TempGlyph.MinY		-= OutSize;
		TempGlyph.MaxX		+= OutSize;
		TempGlyph.MaxY		+= OutSize;
		TempGlyph.CurX		= CurrentPos.Left	- OutSize;
		TempGlyph.CurW		= CurrentPos.Right	+ OutTotal;
		TempGlyph.CurY		= CurrentPos.Top	- OutSize;
		TempGlyph.CurH		= CurrentPos.Bottom + OutTotal;
		TempGlyph.GlyphH	= GlyphRect.y		+ OutSize;

		//Position xpos ready for next glyph
		CurrentPos.Left		+= GlyphRect.x		+ OutTotal + PixelSep;

		//If the next character will run off the edge of the glyph sheet, advance to next row
		if ( CurrentPos.Left + CurrentPos.Right > mTexWidth ) {
			CurrentPos.Left = OutSize;
			CurrentPos.Top += mHeight;
		}

		// Create the outline for the glyph and copy the outline to the texture
		if ( OutlineSize && OutlineEntropia == OutlineMethod ) {
			Recti nGlyphR(
				TempGlyph.CurX,
				TempGlyph.CurY,
				TempGlyph.CurX + TempGlyph.CurW,
				TempGlyph.CurY + TempGlyph.CurH
			);
			Sizei nGlyphS( nGlyphR.size() );

			if ( nGlyphS.x > 0 && nGlyphS.y > 0 ) {
				Uint32 Pos			= 0;
				Uint32 RPos			= 0;
				Uint32 alphaSize	= nGlyphS.x * nGlyphS.y;
				Uint8 * alpha_init	= (Uint8*)malloc( alphaSize );
				Uint8 * alpha_final	= (Uint8*)malloc( alphaSize );

				// Fill the alpha_init ( the default font alpha channels ) and the alpha_final ( the new outline )
				for ( Int32 y = 0; y < nGlyphS.y; y++ ) {
					for( Int32 x = 0; x < nGlyphS.x; x++) {
						RPos	= ( nGlyphR.Left + x ) + ( nGlyphR.Top + y ) * w;
						Pos		= x + y * nGlyphS.x;

						alpha_init[ Pos ] = mPixels[ RPos ].a();
						alpha_final[ Pos ] = 0;
					}
				}

				// Create the outline
				MakeOutline( alpha_init, alpha_final, nGlyphS.x, nGlyphS.y, OutlineSize );

				for ( Int32 y = 0; y < nGlyphS.y; y++ ) {
					for( Int32 x = 0; x < nGlyphS.x; x++) {
						RPos	= ( nGlyphR.Left + x ) + ( nGlyphR.Top + y ) * w;
						Pos		= x + y * nGlyphS.x;

						// Blending the normal glyph color to the outline color
						mPixels[ RPos ] = Color::blend( ColorA( FontColor, alpha_init[ Pos ] ), ColorA( OutlineColor, alpha_final[ Pos ] ) );
					}
				}

				free( alpha_init );
				free( alpha_final );
			}
		}

		//Push back to glyphs vector
		mGlyphs[i] = TempGlyph;

		//Free surface
		hkSAFE_DELETE_ARRAY( TempGlyphSurface );

		hkSAFE_DELETE_ARRAY( TempOutGlyphSurface );
	}

	hkFontManager::instance()->CloseFont( mFont );

	if ( NULL != mFontOutline )
		hkFontManager::instance()->CloseFont( mFontOutline );

	mTexReady = true;

	if ( !mThreadedLoading )
		UpdateLoading();

	return true;
}

void TTFFont::UpdateLoading() {
	if ( mTexReady && NULL != mPixels ) {
		std::string name( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( mFilepath ) ) );

		mTexId = TextureFactory::instance()->LoadFromPixels( reinterpret_cast<unsigned char *> ( &mPixels[0] ), (Uint32)mTexWidth, (Uint32)mTexHeight, 4, false, CLAMP_TO_EDGE, false, false, name );

		eeSAFE_DELETE_ARRAY( mPixels );

		RebuildFromGlyphs();

		eePRINTL( "TTF Font %s loaded.", mFilepath.c_str() );
	}
}

void TTFFont::RebuildFromGlyphs() {
	Float Top, Bottom;
	Rectf tR;

	mTexCoords.resize( mNumChars );

	Texture * Tex = TextureFactory::instance()->GetTexture( mTexId );

	TextureFactory::instance()->Bind( Tex );

	eeGlyph tGlyph;

	for (unsigned int i = 0; i < mNumChars; i++) {
		tGlyph		= mGlyphs[i];

		tR.Left		= (Float)tGlyph.CurX / Tex->Width();
		tR.Top		= (Float)tGlyph.CurY / Tex->Height();

		tR.Right	= (Float)(tGlyph.CurX + tGlyph.CurW) / Tex->Width();
		tR.Bottom	= (Float)(tGlyph.CurY + tGlyph.CurH) / Tex->Height();

		Top			= (Float)mHeight + mDescent	- tGlyph.GlyphH - tGlyph.MinY;
		Bottom		= (Float)mHeight + mDescent	+ tGlyph.GlyphH - tGlyph.MaxY;

		mTexCoords[i].TexCoords[0] = tR.Left;
		mTexCoords[i].TexCoords[1] = tR.Top;
		mTexCoords[i].TexCoords[2] = tR.Left;
		mTexCoords[i].TexCoords[3] = tR.Bottom;
		mTexCoords[i].TexCoords[4] = tR.Right;
		mTexCoords[i].TexCoords[5] = tR.Bottom;
		mTexCoords[i].TexCoords[6] = tR.Right;
		mTexCoords[i].TexCoords[7] = tR.Top;
		mTexCoords[i].Vertex[0] = (Float) tGlyph.MinX;
		mTexCoords[i].Vertex[1] = Top;
		mTexCoords[i].Vertex[2] = (Float) tGlyph.MinX;
		mTexCoords[i].Vertex[3] = Bottom;
		mTexCoords[i].Vertex[4] = (Float) tGlyph.MaxX;
		mTexCoords[i].Vertex[5] = Bottom;
		mTexCoords[i].Vertex[6] = (Float) tGlyph.MaxX;
		mTexCoords[i].Vertex[7] = Top;
	}
}

bool TTFFont::SaveTexture( const std::string& Filepath, const EE_SAVE_TYPE& Format ) {
	Texture* Tex = TextureFactory::instance()->GetTexture(mTexId);

	if ( Tex != NULL )
		return Tex->SaveToFile( Filepath, Format );

	return false;
}

bool TTFFont::SaveCoordinates( const std::string& Filepath ) {
	IOStreamFile fs( Filepath, std::ios::out | std::ios::binary );

	if ( fs.isOpen() ) {
		sFntHdr FntHdr;

		FntHdr.Magic		= EE_TTF_FONT_MAGIC;
		FntHdr.FirstChar	= 0;
		FntHdr.NumChars		= mGlyphs.size();
		FntHdr.Size			= mSize;
		FntHdr.Height		= mHeight;
		FntHdr.LineSkip		= mLineSkip;
		FntHdr.Ascent		= mAscent;
		FntHdr.Descent		= mDescent;

		// Write the header
		fs.write( reinterpret_cast<const char*>( &FntHdr ), sizeof(sFntHdr) );

		// Write the glyphs
		fs.write( reinterpret_cast<const char*> (&mGlyphs[0]), sizeof(eeGlyph) * mGlyphs.size() );

		RebuildFromGlyphs();

		return true;
	} else {
		eePRINTL("TTFFont::SaveCoordinates(): Unable to write file: %s.", Filepath.c_str() );
	}

	return false;
}

bool TTFFont::Save( const std::string& TexturePath, const std::string& CoordinatesDatPath, const EE_SAVE_TYPE& Format ) {
	return SaveTexture(TexturePath, Format) && SaveCoordinates( CoordinatesDatPath );
}

void TTFFont::MakeOutline( Uint8 *in, Uint8 *out, Int16 w, Int16 h , Int16 OutlineSize ) {
	int y, x, s_y, s_x, get_y, get_x, index, pos;
	Uint8 c;

	for ( y = 0; y < h; y++ ) {
		for( x = 0; x < w; x++ ) {
			pos = y * w + x;

			c = in[ pos ];

			for ( s_y = -OutlineSize; s_y <= OutlineSize; s_y++ ) {
				for ( s_x = -OutlineSize; s_x <= OutlineSize; s_x++ ) {
					get_x = x + s_x;
					get_y = y + s_y;

					if ( get_x >= 0 && get_y >= 0 && get_x < w && get_y < h ) {
						index = get_y * w + get_x;

						if ( in[index] > c )
							c = in[index];
					}
				}
			}

			out[ pos ] = c;
		}
	}
}

bool TTFFont::ThreadedLoading() const {
	return mThreadedLoading;
}

void TTFFont::ThreadedLoading( const bool& isThreaded ) {
	mThreadedLoading = isThreaded;
}

}}
