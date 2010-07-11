#include "cttffont.hpp"

namespace EE { namespace Graphics {

cTTFFont::cTTFFont() : cFont(), mFont(0) {
	TF = cTextureFactory::instance();

	if ( hkFontManager::instance()->Init() )
		mTTFInit = false;
	else
		mTTFInit = true;
}

cTTFFont::~cTTFFont() {
	hkFontManager::instance()->Destroy();
}

bool cTTFFont::LoadFromPack( cPack* Pack, const std::string& FilePackPath, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor ) {
	std::vector<Uint8> TmpData;

	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, TmpData ) )
		return LoadFromMemory( reinterpret_cast<Uint8*> (&TmpData[0]), TmpData.size(), Size, Style, VerticalDraw, NumCharsToGen, FontColor, OutlineSize, OutlineColor );

	TmpData.clear();

	return false;
}

bool cTTFFont::LoadFromMemory( Uint8* TTFData, const eeUint& TTFDataSize, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor ) {
	mFilepath = "from memory";
	mLoadedFromMemory = true;

	mFont = hkFontManager::instance()->OpenFromMemory( reinterpret_cast<uint8_t*>(&TTFData[0]), TTFDataSize, Size, 0, NumCharsToGen );

	return iLoad( Size, Style, VerticalDraw, NumCharsToGen, FontColor, OutlineSize, OutlineColor );
}

bool cTTFFont::Load(const std::string& Filepath, const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor ) {
	mFilepath = Filepath;
	mLoadedFromMemory = false;

	mFont = hkFontManager::instance()->OpenFromFile( Filepath.c_str(), Size, 0, NumCharsToGen );

	return iLoad( Size, Style, VerticalDraw, NumCharsToGen, FontColor, OutlineSize, OutlineColor );
}

bool cTTFFont::iLoad(const eeUint& Size, EE_TTF_FONTSTYLE Style, const bool& VerticalDraw, const Uint16& NumCharsToGen, const eeColor& FontColor, const Uint8& OutlineSize, const eeColor& OutlineColor ) {
	SDL_Rect CurrentPos = {0, 0, 0, 0};
	SDL_Rect GlyphRect = {0, 0, 0, 0};

	unsigned char * TempGlyphSurface;

	eeFloat Top, Bottom;
	Uint8 OutlineTotal = OutlineSize * 2;

	if ( !mTTFInit )
		return false;

	try {
		if ( mFont == NULL )
			return false;

		mFont->Style( Style );

		mVerticalDraw = VerticalDraw;
		mSize = Size;
		mNumChars = NumCharsToGen;

		mFontColor = FontColor;
		mOutlineColor = OutlineColor;

		mStyle = Style;

        mGlyphs.clear();
		mGlyphs.resize( mNumChars );

		mTexWidth = (eeFloat)NextPowOfTwo( mSize * 16 );
		mTexHeight = mTexWidth;

		if ( ( mSize >= 60 && mNumChars > 256 ) || ( OutlineSize > 2 && mNumChars >= 512 && mSize >= 24 ) )
			mTexHeight *= 2;

		mHeight = mFont->Height() + OutlineTotal;

		Uint32 TexId = TF->CreateEmptyTexture( mTexWidth, mTexHeight, eeColorA(0x00000000) );

		cTexture * Tex = TF->GetTexture( TexId );

		Tex->Lock();

		CurrentPos.x = OutlineSize;
		CurrentPos.y = OutlineSize;

		//Loop through all chars
		for ( eeUint i = 0; i < mNumChars; i++) {
			TempGlyphSurface = mFont->GlyphRender( i, 0x00000000 );

			//New temp glyph
			eeGlyph TempGlyph;

			//Get the glyph attributes
			mFont->GlyphMetrics( i, &TempGlyph.MinX, &TempGlyph.MaxX, &TempGlyph.MinY, &TempGlyph.MaxY, &TempGlyph.Advance );

			//Set size of glyph rect
			GlyphRect.w = mFont->Current()->Pixmap()->width;
			GlyphRect.h = mFont->Current()->Pixmap()->rows;

			//Set size of current position rect
			CurrentPos.w = CurrentPos.x + TempGlyph.MaxX;
			CurrentPos.h = CurrentPos.y + TempGlyph.MaxY;

			if (CurrentPos.w >= mTexWidth) {
				CurrentPos.x = 0 + OutlineSize;
				CurrentPos.y += mHeight;
			}

			//Blit the glyph onto the glyph sheet
			Uint32 * TexGlyph = reinterpret_cast<Uint32 *> ( TempGlyphSurface );
			Uint32 Alpha;
			Uint32 w = Tex->Width();
			Uint32 h = Tex->Height();
			Uint32 px, py;

			for (int y = 0; y < GlyphRect.h; ++y ) {
				for (int x = 0; x < GlyphRect.w; ++x ) {
					Alpha = ( TexGlyph[ x + y * GlyphRect.w ] >> 24 ) & 0xFF;
					
					px = CurrentPos.x + x;
					py = CurrentPos.y + y;

					if ( px < w && py < h )
						Tex->SetPixel( px, py, eeColorA( FontColor.R(), FontColor.G(), FontColor.B(), Alpha ) );
				}
			}

			// Fixes the width and height of the current pos
			CurrentPos.w = GlyphRect.w;
			CurrentPos.h = GlyphRect.h;

			// Set texture coordinates to te list
			eeRectf tR;
			tR.Left = (eeFloat)( CurrentPos.x - OutlineSize ) / mTexWidth;
			tR.Top = (eeFloat)( CurrentPos.y - OutlineSize ) / mTexHeight;

			tR.Right = (eeFloat)(CurrentPos.x + CurrentPos.w + OutlineSize ) / mTexWidth;
			tR.Bottom = (eeFloat)(CurrentPos.y + CurrentPos.h + OutlineSize ) / mTexHeight;

			GlyphRect.h += OutlineSize;
			TempGlyph.Advance += OutlineSize;

			Top = static_cast<eeFloat> ( mSize 	- GlyphRect.h - TempGlyph.MinY );
			Bottom = static_cast<eeFloat> ( mSize 	+ GlyphRect.h - TempGlyph.MaxY );

			// Translate the Glyph coordinates to the new texture coordinates
			TempGlyph.MinX -= OutlineSize;
			TempGlyph.MinY -= OutlineSize;
			TempGlyph.MaxX += OutlineSize;
			TempGlyph.MaxY += OutlineSize;
			TempGlyph.CurX = CurrentPos.x - OutlineSize;
			TempGlyph.CurW = CurrentPos.w + OutlineTotal;
			TempGlyph.CurY = CurrentPos.y - OutlineSize;
			TempGlyph.CurH = CurrentPos.h + OutlineTotal;
			TempGlyph.GlyphH = GlyphRect.h + OutlineSize;

			//Position xpos ready for next glyph
			CurrentPos.x += GlyphRect.w + OutlineTotal;

			//If the next character will run off the edge of the glyph sheet, advance to next row
			if (CurrentPos.x + CurrentPos.w > mTexWidth) {
				CurrentPos.x = 0 + OutlineSize;
				CurrentPos.y += mHeight;
			}

			//Push back to glyphs vector
			mGlyphs[i] = TempGlyph;

			//Free surface
			eeSAFE_DELETE_ARRAY( TempGlyphSurface );
		}

        if ( !mTexId ) {

		// Recover the Alpha channel
		mTexId = TexId;

		if ( OutlineSize && Tex ) {
			eeColorA P, R;
			Uint32 Pos = 0;

			std::vector<Uint8> TexO( (Uint32)Tex->Width() * (Uint32)Tex->Height(), 0 );
			std::vector<Uint8> TexN( (Uint32)Tex->Width() * (Uint32)Tex->Height(), 0 );
			std::vector<Uint8> TexI( (Uint32)Tex->Width() * (Uint32)Tex->Height(), 0 );

			// Fill the TexO ( the default font alpha channels ) and the TexN ( the new outline )
			for ( Int32 y = 0; y < Tex->Height(); y++ ) {
				for( Int32 x = 0; x < Tex->Width(); x++) {
					Pos = x + y * (Uint32)Tex->Width();
					TexO[ Pos ] = Tex->GetPixel( x, y ).A();
					TexN[ Pos ] = TexO[ Pos ];
				}
			}

			Uint8* alpha = reinterpret_cast<Uint8*>( &TexN[0] );
			Uint8* alpha2 = reinterpret_cast<Uint8*>( &TexI[0] );

			// Create the outline
			for ( Uint8 passes = 0; passes < OutlineSize; passes++ ) {
				MakeOutline( alpha, alpha2, static_cast<Int16>( Tex->Width() ), static_cast<Int16>( Tex->Height() ) );

				Uint8* temp = alpha;
				alpha = alpha2;
				alpha2 = temp;
			}

			for ( Int32 y = 0; y < Tex->Height(); y++ ) {
				for( Int32 x = 0; x < Tex->Width(); x++) {
					Pos = x + y * (Uint32)Tex->Width();

					// Fill the outline color
					Tex->SetPixel( x, y, eeColorA( OutlineColor.R(), OutlineColor.G(), OutlineColor.B(), alpha[ Pos ] ) );

					// Fill the font color
					if ( TexO[ Pos ] > 50 )
						Tex->SetPixel( x, y, eeColorA( FontColor.R(), FontColor.G(), FontColor.B(), TexO[ Pos ] ) );
				}
			}
		}

		Tex->Unlock( false, true );

        }

		hkFontManager::instance()->CloseFont( mFont );

		RebuildFromGlyphs();

		cLog::instance()->Write( "TTF Font " + mFilepath + " loaded." );
		return true;
	} catch (...) {
		cLog::instance()->Write( "Failed to load TTF Font " + mFilepath + "." );
		return false;
	}
}

void cTTFFont::RebuildFromGlyphs() {
	eeFloat Top, Bottom;
	eeRectf tR;

	mTexCoords.resize( mNumChars );

	cTexture * Tex = TF->GetTexture( mTexId );

	TF->Bind( Tex );

	for (eeUint i = 0; i < mNumChars; i++) {
		tR.Left = (eeFloat)mGlyphs[i].CurX / Tex->Width();
		tR.Top = (eeFloat)mGlyphs[i].CurY / Tex->Height();

		tR.Right = (eeFloat)(mGlyphs[i].CurX + mGlyphs[i].CurW) / Tex->Width();
		tR.Bottom = (eeFloat)(mGlyphs[i].CurY + mGlyphs[i].CurH) / Tex->Height();

		Top = 		(eeFloat)mSize 	- mGlyphs[i].GlyphH - mGlyphs[i].MinY;
		Bottom = 	(eeFloat)mSize 	+ mGlyphs[i].GlyphH - mGlyphs[i].MaxY;

		mTexCoords[i].TexCoords[0] = tR.Left;
		mTexCoords[i].TexCoords[1] = tR.Top;
		mTexCoords[i].TexCoords[2] = tR.Left;
		mTexCoords[i].TexCoords[3] = tR.Bottom;
		mTexCoords[i].TexCoords[4] = tR.Right;
		mTexCoords[i].TexCoords[5] = tR.Bottom;
		mTexCoords[i].TexCoords[6] = tR.Right;
		mTexCoords[i].TexCoords[7] = tR.Top;
		mTexCoords[i].Vertex[0] = (eeFloat) mGlyphs[i].MinX;
		mTexCoords[i].Vertex[1] = Top;
		mTexCoords[i].Vertex[2] = (eeFloat) mGlyphs[i].MinX;
		mTexCoords[i].Vertex[3] = Bottom;
		mTexCoords[i].Vertex[4] = (eeFloat) mGlyphs[i].MaxX;
		mTexCoords[i].Vertex[5] = Bottom;
		mTexCoords[i].Vertex[6] = (eeFloat) mGlyphs[i].MaxX;
		mTexCoords[i].Vertex[7] = Top;
	}
}

bool cTTFFont::SaveTexture( const std::string& Filepath, const EE_SAVETYPE& Format ) {
	cTexture* Tex = TF->GetTexture(mTexId);

	if ( Tex != NULL )
		return Tex->SaveToFile( Filepath, Format );

	return false;
}

bool cTTFFont::SaveCoordinates( const std::string& Filepath ) {
	Uint8 chars;
	try {
		std::fstream fs ( Filepath.c_str() , std::ios::out | std::ios::binary );

		// Write the number of the fist char represented on the texture
		chars = 0;
		fs.write( reinterpret_cast<const char*> (&chars), sizeof(Uint8) );

		// Write the default size of every char
		chars = static_cast<Uint8> ( mSize );
		fs.write( reinterpret_cast<const char*> (&chars), sizeof(Uint8) );

		for (eeUint i = 0; i < mGlyphs.size(); i++)
			fs.write( reinterpret_cast<const char*> (&mGlyphs[i]), sizeof(eeGlyph) );

		fs.close();
		RebuildFromGlyphs();

		return true;
	} catch (...)  {
		cLog::instance()->Write("Unable to write " + Filepath + " on cTTFFont::SaveCoordinates");
	}
	return false;
}

bool cTTFFont::Save( const std::string& TexturePath, const std::string& CoordinatesDatPath, const EE_SAVETYPE& Format ) {
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

}}
