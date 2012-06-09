#include <eepp/graphics/cttffont.hpp>
#include <eepp/system/ciostreamfile.hpp>
#include <eepp/helper/haikuttf/haikuttf.hpp>
using namespace HaikuTTF;

namespace EE { namespace Graphics {

cTTFFont::cTTFFont( const std::string FontName ) :
	cFont( FONT_TYPE_TTF, FontName ),
	mFont(0),
	mPixels(NULL),
	mThreadedLoading(false),
	mTexReady(false)
{
}

cTTFFont::~cTTFFont() {
	hkFontManager::instance()->Destroy();
}

bool cTTFFont::LoadFromPack( cPack* Pack, const std::string& FilePackPath, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor, const bool& AddPixelSeparator ) {
	bool Ret = false;
	SafeDataPointer PData;

	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, PData ) ) {
		mFilepath = FilePackPath;

		Ret = LoadFromMemory( PData.Data, PData.DataSize, Size, Style, VerticalDraw, NumCharsToGen, FontColor, OutlineSize, OutlineColor, AddPixelSeparator );
	}

	return Ret;
}

bool cTTFFont::LoadFromMemory( Uint8* TTFData, const eeUint& TTFDataSize, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor, const bool& AddPixelSeparator ) {
	if ( !mFilepath.size() )
		mFilepath = "from memory";

	mLoadedFromMemory = true;

	mFont = hkFontManager::instance()->OpenFromMemory( reinterpret_cast<Uint8*>(&TTFData[0]), TTFDataSize, Size, 0, NumCharsToGen );

	return iLoad( Size, Style, VerticalDraw, NumCharsToGen, FontColor, OutlineSize, OutlineColor, AddPixelSeparator );
}

bool cTTFFont::Load( const std::string& Filepath, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor, const bool& AddPixelSeparator ) {
	mFilepath			= Filepath;

	if ( FileExists( Filepath ) ) {
		mLoadedFromMemory	= false;

		mFont = hkFontManager::instance()->OpenFromFile( Filepath.c_str(), Size, 0, NumCharsToGen );

		return iLoad( Size, Style, VerticalDraw, NumCharsToGen, FontColor, OutlineSize, OutlineColor, AddPixelSeparator );
	} else if ( cPackManager::instance()->FallbackToPacks() ) {
		cPack * tPack = cPackManager::instance()->Exists( mFilepath );

		if ( NULL != tPack ) {
			return LoadFromPack( tPack, mFilepath, Size, Style, VerticalDraw, NumCharsToGen, FontColor, OutlineSize, OutlineColor, AddPixelSeparator );
		}
	}

	return false;
}

bool cTTFFont::iLoad( const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, Uint8 OutlineSize, const eeColor& OutlineColor, const bool& AddPixelSeparator ) {
	eeRect CurrentPos;
	eeSize GlyphRect;

	unsigned char * TempGlyphSurface;

	// Change the outline size to add a pixel separating the character from the around characters to prevent ugly zooming of characters
	Uint32 PixelSep = 0;

	if ( AddPixelSeparator )
		PixelSep = 1;

	Uint8 OutlineTotal = OutlineSize * 2;
	Uint32 TexSize;

	if ( mFont == NULL ) {
		cLog::instance()->Write( "Failed to load TTF Font " + mFilepath + "." );

		return false;
	}

	mFont->Style( Style );

	mVerticalDraw 	= VerticalDraw;
	mSize 			= Size;
	mHeight 		= mFont->Height() + OutlineTotal;
	mLineSkip		= mFont->LineSkip();
	mAscent			= mFont->Ascent();
	mDescent		= mFont->Descent();

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

	// Find the larger glyph
	/*
	int maxWidth = 0, maxHeight = 0;

	for ( eeUint i = 0; i < mNumChars; i++) {
		TempGlyphSurface = mFont->GlyphRender( i, 0x00000000 );

		maxWidth = eemax( mFont->Current()->Pixmap()->width, maxWidth );
		maxHeight = eemax( mFont->Current()->Pixmap()->rows, maxHeight );

		hkSAFE_DELETE_ARRAY( TempGlyphSurface );
	}

	//ReqSize = mNumChars * mHeight * mHeight;
	*/

	// Find the best size for the texture ( aprox )
	// Totally wild guessing, but it's working
	Int32 tWildGuessW = ( mAscent + PixelSep + OutlineTotal );
	Int32 tWildGuessH = tWildGuessW; //( mHeight );

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

	mPixels = eeNewArray( eeColorA, TexSize );
	memset( mPixels, 0x00000000, TexSize * 4 );

	CurrentPos.Left = OutlineSize;
	CurrentPos.Top 	= OutlineSize;

	//Loop through all chars
	for ( eeUint i = 0; i < mNumChars; i++) {
		TempGlyphSurface = mFont->GlyphRender( i, 0x00000000 );

		//New temp glyph
		eeGlyph TempGlyph;

		//Get the glyph attributes
		mFont->GlyphMetrics( i, &TempGlyph.MinX, &TempGlyph.MaxX, &TempGlyph.MinY, &TempGlyph.MaxY, &TempGlyph.Advance );

		//Set size of glyph rect
		GlyphRect.x = mFont->Current()->Pixmap()->width;
		GlyphRect.y = mFont->Current()->Pixmap()->rows;

		//Set size of current position rect
		CurrentPos.Right 	= CurrentPos.Left + TempGlyph.MaxX;
		CurrentPos.Bottom 	= CurrentPos.Top + TempGlyph.MaxY;

		if ( CurrentPos.Right >= mTexWidth ) {
			CurrentPos.Left = 0 + OutlineSize;
			CurrentPos.Top += mHeight;
		}

		//Blit the glyph onto the glyph sheet
		Uint32 * TexGlyph = reinterpret_cast<Uint32 *> ( TempGlyphSurface );
		Uint32 Alpha;
		Uint32 w = (Uint32)mTexWidth;
		Uint32 h = (Uint32)mTexHeight;
		Uint32 px, py;

		for (int y = 0; y < GlyphRect.y; ++y ) {
			for (int x = 0; x < GlyphRect.x; ++x ) {
				Alpha = ( TexGlyph[ x + y * GlyphRect.x ] >> 24 ) & 0xFF;

				px = CurrentPos.Left + x;
				py = CurrentPos.Top + y;

				if ( px < w && py < h ) {
					mPixels[ px + py * w ] = eeColorA( FontColor.R(), FontColor.G(), FontColor.B(), Alpha );
				}
			}
		}

		// Fixes the width and height of the current pos
		CurrentPos.Right 	= GlyphRect.x;
		CurrentPos.Bottom 	= GlyphRect.y;

		// Set texture coordinates to te list
		eeRectf tR;
		tR.Left = (eeFloat)( CurrentPos.Left - OutlineSize ) / mTexWidth;
		tR.Top = (eeFloat)( CurrentPos.Top - OutlineSize ) / mTexHeight;

		tR.Right = (eeFloat)(CurrentPos.Left + CurrentPos.Right + OutlineSize ) / mTexWidth;
		tR.Bottom = (eeFloat)(CurrentPos.Top + CurrentPos.Bottom + OutlineSize ) / mTexHeight;

		GlyphRect.y += OutlineSize;
		TempGlyph.Advance += OutlineSize;

		// Translate the Glyph coordinates to the new texture coordinates
		TempGlyph.MinX -= OutlineSize;
		TempGlyph.MinY -= OutlineSize;
		TempGlyph.MaxX += OutlineSize;
		TempGlyph.MaxY += OutlineSize;
		TempGlyph.CurX = CurrentPos.Left - OutlineSize;
		TempGlyph.CurW = CurrentPos.Right + OutlineTotal;
		TempGlyph.CurY = CurrentPos.Top - OutlineSize;
		TempGlyph.CurH = CurrentPos.Bottom + OutlineTotal;
		TempGlyph.GlyphH = GlyphRect.y + OutlineSize;

		//Position xpos ready for next glyph
		CurrentPos.Left += GlyphRect.x + OutlineTotal + PixelSep;

		//If the next character will run off the edge of the glyph sheet, advance to next row
		if ( CurrentPos.Left + CurrentPos.Right > mTexWidth ) {
			CurrentPos.Left = 0 + OutlineSize;
			CurrentPos.Top += mHeight;
		}

		//Push back to glyphs vector
		mGlyphs[i] = TempGlyph;

		//Free surface
		hkSAFE_DELETE_ARRAY( TempGlyphSurface );
	}

	if ( OutlineSize ) {
		Uint32 Pos = 0;

		std::vector<Uint8> TexO( TexSize, 0 );
		std::vector<Uint8> TexN( TexSize, 0 );
		std::vector<Uint8> TexI( TexSize, 0 );

		// Fill the TexO ( the default font alpha channels ) and the TexN ( the new outline )
		for ( Int32 y = 0; y < mTexHeight; y++ ) {
			for( Int32 x = 0; x < mTexWidth; x++) {
				Pos = x + y * (Uint32)mTexWidth;
				TexO[ Pos ] = mPixels[ Pos ].A();
				TexN[ Pos ] = TexO[ Pos ];
			}
		}

		Uint8* alpha = reinterpret_cast<Uint8*>( &TexN[0] );
		Uint8* alpha2 = reinterpret_cast<Uint8*>( &TexI[0] );

		// Create the outline
		for ( Uint8 passes = 0; passes < OutlineSize; passes++ ) {
			MakeOutline( alpha, alpha2, static_cast<Int16>( mTexWidth ), static_cast<Int16>( mTexHeight ) );

			Uint8* temp = alpha;
			alpha = alpha2;
			alpha2 = temp;
		}

		for ( Int32 y = 0; y < mTexHeight; y++ ) {
			for( Int32 x = 0; x < mTexWidth; x++) {
				Pos = x + y * (Uint32)mTexWidth;

				// Fill the outline color
				mPixels[ Pos ] = eeColorA( OutlineColor.R(), OutlineColor.G(), OutlineColor.B(), alpha[ Pos ] );

				// Fill the font color
				if ( TexO[ Pos ] > 50 )
					mPixels[ Pos ] = eeColorA( FontColor.R(), FontColor.G(), FontColor.B(), TexO[ Pos ] );
			}
		}
	}

	hkFontManager::instance()->CloseFont( mFont );

	mTexReady = true;

	if ( !mThreadedLoading )
		UpdateLoading();

	return true;
}

void cTTFFont::UpdateLoading() {
	if ( mTexReady && NULL != mPixels ) {
		std::string name( FileRemoveExtension( FileNameFromPath( mFilepath ) ) );

		mTexId = cTextureFactory::instance()->LoadFromPixels( reinterpret_cast<unsigned char *> ( &mPixels[0] ), (Uint32)mTexWidth, (Uint32)mTexHeight, 4, false, EE_CLAMP_TO_EDGE, false, false, name );

		eeSAFE_DELETE_ARRAY( mPixels );

		RebuildFromGlyphs();

		cLog::instance()->Write( "TTF Font " + mFilepath + " loaded." );
	}
}

void cTTFFont::RebuildFromGlyphs() {
	eeFloat Top, Bottom;
	eeRectf tR;

	mTexCoords.resize( mNumChars );

	cTexture * Tex = cTextureFactory::instance()->GetTexture( mTexId );

	cTextureFactory::instance()->Bind( Tex );

	eeGlyph tGlyph;

	for (eeUint i = 0; i < mNumChars; i++) {
		tGlyph		= mGlyphs[i];

		tR.Left		= (eeFloat)tGlyph.CurX / Tex->Width();
		tR.Top		= (eeFloat)tGlyph.CurY / Tex->Height();

		tR.Right	= (eeFloat)(tGlyph.CurX + tGlyph.CurW) / Tex->Width();
		tR.Bottom	= (eeFloat)(tGlyph.CurY + tGlyph.CurH) / Tex->Height();

		Top			= (eeFloat)mHeight + mDescent	- tGlyph.GlyphH - tGlyph.MinY;
		Bottom		= (eeFloat)mHeight + mDescent	+ tGlyph.GlyphH - tGlyph.MaxY;

		mTexCoords[i].TexCoords[0] = tR.Left;
		mTexCoords[i].TexCoords[1] = tR.Top;
		mTexCoords[i].TexCoords[2] = tR.Left;
		mTexCoords[i].TexCoords[3] = tR.Bottom;
		mTexCoords[i].TexCoords[4] = tR.Right;
		mTexCoords[i].TexCoords[5] = tR.Bottom;
		mTexCoords[i].TexCoords[6] = tR.Right;
		mTexCoords[i].TexCoords[7] = tR.Top;
		mTexCoords[i].Vertex[0] = (eeFloat) tGlyph.MinX;
		mTexCoords[i].Vertex[1] = Top;
		mTexCoords[i].Vertex[2] = (eeFloat) tGlyph.MinX;
		mTexCoords[i].Vertex[3] = Bottom;
		mTexCoords[i].Vertex[4] = (eeFloat) tGlyph.MaxX;
		mTexCoords[i].Vertex[5] = Bottom;
		mTexCoords[i].Vertex[6] = (eeFloat) tGlyph.MaxX;
		mTexCoords[i].Vertex[7] = Top;
	}
}

bool cTTFFont::SaveTexture( const std::string& Filepath, const EE_SAVE_TYPE& Format ) {
	cTexture* Tex = cTextureFactory::instance()->GetTexture(mTexId);

	if ( Tex != NULL )
		return Tex->SaveToFile( Filepath, Format );

	return false;
}

bool cTTFFont::SaveCoordinates( const std::string& Filepath ) {
	cIOStreamFile fs( Filepath, std::ios::out | std::ios::binary );

	if ( fs.IsOpen() ) {
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
		fs.Write( reinterpret_cast<const char*>( &FntHdr ), sizeof(sFntHdr) );

		// Write the glyphs
		fs.Write( reinterpret_cast<const char*> (&mGlyphs[0]), sizeof(eeGlyph) * mGlyphs.size() );

		RebuildFromGlyphs();

		return true;
	} else {
		cLog::instance()->Write("Unable to write " + Filepath + " on cTTFFont::SaveCoordinates");
	}

	return false;
}

bool cTTFFont::Save( const std::string& TexturePath, const std::string& CoordinatesDatPath, const EE_SAVE_TYPE& Format ) {
	return SaveTexture(TexturePath, Format) && SaveCoordinates( CoordinatesDatPath );
}

void cTTFFont::MakeOutline( Uint8 *in, Uint8 *out, Int16 w, Int16 h) {
	for ( eeInt y = 0; y < h; y++ ) {
		for( eeInt x = 0; x < w; x++ ) {
			eeInt c = in[ y * w + x ];

			for ( eeInt s_y = -1; s_y <= 1; s_y++ ) {
				for ( eeInt s_x = -1; s_x <= 1; s_x++ ) {
					eeInt get_x = x + s_x;
					eeInt get_y = y + s_y;

					if ( get_x >= 0 && get_y >= 0 && get_x < w && get_y < h ) {
						eeInt index = get_y * w + get_x;

						if ( in[index] > c )
							c = in[index];
					}
				}

				out[ y * w + x ] = c;
			}
		}
	}
}

bool cTTFFont::ThreadedLoading() const {
	return mThreadedLoading;
}

void cTTFFont::ThreadedLoading( const bool& isThreaded ) {
	mThreadedLoading = isThreaded;
}

}}
