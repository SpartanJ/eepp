#include "ctexturefont.hpp"

namespace EE { namespace Graphics {

cTextureFont::cTextureFont( const std::string FontName ) :
	cFont( FONT_TYPE_TEX, FontName ),
	mStartChar(0),
	mNumChars(256),
	mLoadedCoords(false)
{
}

cTextureFont::~cTextureFont() {
}

bool cTextureFont::Load( const Uint32& TexId, const eeUint& StartChar, const eeUint& Spacing, const bool& VerticalDraw, const eeUint& TexColumns, const eeUint& TexRows, const Uint16& NumChars ) {
	cTexture * Tex = cTextureFactory::instance()->GetTexture( TexId );

	mTexId = TexId;

	if ( NULL != Tex ) {
		mTexColumns = TexColumns;
		mTexRows = TexRows;
		mStartChar = StartChar;
		mNumChars = NumChars;

		mtX = ( 1 / static_cast<eeFloat>( mTexColumns ) );
		mtY = ( 1 / static_cast<eeFloat>( mTexRows ) );

		mFWidth = (eeFloat)( Tex->Width() / mTexColumns );
		mFHeight = (eeFloat)( Tex->Height() / mTexRows );
		mHeight = mSize = (eeUint)mFHeight;

		mVerticalDraw = VerticalDraw;

		if ( Spacing == 0 )
			mSpacing = static_cast<eeUint>( mFWidth );
		else
			mSpacing = Spacing;

		BuildFont();

		cLog::instance()->Write( "Texture Font " + Tex->Filepath() + " loaded." );

		return true;
	}

	cLog::instance()->Write( "Failed to load Texture Font " );

	return false;
}

void cTextureFont::BuildFont() {
	eeFloat cX = 0, cY = 0;

	mTexCoords.resize( mNumChars );
	mGlyphs.resize( mNumChars );

	cTextureFactory::instance()->Bind( mTexId );

	for (eeUint i = 0; i < mNumChars; i++) {
		if ( i >= mStartChar ) {
			cX = (eeFloat)( (i-mStartChar) % mTexColumns ) / (eeFloat)mTexColumns;
			cY = (eeFloat)( (i-mStartChar) / mTexColumns ) / (eeFloat)mTexRows;

			mGlyphs[i].Advance = mSpacing;

			mTexCoords[i].TexCoords[0] = cX;
			mTexCoords[i].TexCoords[1] = cY;
			mTexCoords[i].TexCoords[2] = cX;
			mTexCoords[i].TexCoords[3] = cY + mtY;
			mTexCoords[i].TexCoords[4] = cX + mtX;
			mTexCoords[i].TexCoords[5] = cY + mtY;
			mTexCoords[i].TexCoords[6] = cX + mtX;
			mTexCoords[i].TexCoords[7] = cY;
			mTexCoords[i].Vertex[0] = 0;
			mTexCoords[i].Vertex[1] = 0;
			mTexCoords[i].Vertex[2] = 0;
			mTexCoords[i].Vertex[3] = mFHeight;
			mTexCoords[i].Vertex[4] = mFWidth;
			mTexCoords[i].Vertex[5] = mFHeight;
			mTexCoords[i].Vertex[6] = mFWidth;
			mTexCoords[i].Vertex[7] = 0;
		}
	}
}

void cTextureFont::BuildFontFromDat() {
	eeFloat Top, Bottom;
	eeRectf tR;

	mTexCoords.resize( mNumChars );

	cTexture * Tex = cTextureFactory::instance()->GetTexture( mTexId );

	cTextureFactory::instance()->Bind( Tex );

	for (eeUint i = 0; i < mNumChars; i++) {
		tR.Left = (eeFloat)mGlyphs[i].CurX / Tex->Width();
		tR.Top = (eeFloat)mGlyphs[i].CurY / Tex->Height();

		tR.Right = (eeFloat)(mGlyphs[i].CurX + mGlyphs[i].CurW) / Tex->Width();
		tR.Bottom = (eeFloat)(mGlyphs[i].CurY + mGlyphs[i].CurH) / Tex->Height();

		Top = 		mFHeight 	- mGlyphs[i].GlyphH - mGlyphs[i].MinY;
		Bottom = 	mFHeight 	+ mGlyphs[i].GlyphH - mGlyphs[i].MaxY;

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

bool cTextureFont::Load( const Uint32& TexId, const std::string& CoordinatesDatPath, const bool& VerticalDraw ) {
	if ( FileExists( CoordinatesDatPath ) ) {
		std::vector<Uint8> TmpData;

		FileGet( CoordinatesDatPath, TmpData );

		return LoadFromMemory( TexId, reinterpret_cast<Uint8*> ( &TmpData[0] ), (Uint32)TmpData.size(), VerticalDraw );
	}

	return false;
}

bool cTextureFont::LoadFromPack( const Uint32& TexId, cPack* Pack, const std::string& FilePackPath, const bool& VerticalDraw ) {
	bool Ret = false;
	cPack::SafePointerData PData;

	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, PData ) )
		Ret = LoadFromMemory( TexId, reinterpret_cast<const Uint8*> ( PData.Data ), PData.DataSize, VerticalDraw );

	return Ret;
}

bool cTextureFont::LoadFromMemory( const Uint32& TexId, const Uint8* CoordData, const Uint32& CoordDataSize, const bool& VerticalDraw ) {
	mTexId = TexId;

	if ( mTexId > 0 ) {
		mVerticalDraw = VerticalDraw;

		if ( CoordData != NULL ) {
			mNumChars = static_cast<Uint16> ( ( CoordDataSize - 2 ) / 32 );

			mGlyphs.resize( mNumChars );

			// Read the number of the first char represented on the texture
			mStartChar = CoordData[0];

			// Read the default size of every char
			mFWidth = CoordData[1];
			mFHeight = CoordData[1];
			mHeight = mSize = (Uint32)mFHeight;

			// Read every char coordinates
			memcpy( reinterpret_cast<void*> (&mGlyphs[0]), reinterpret_cast<const void*> (&CoordData[2]), sizeof(eeGlyph) * mNumChars );

			BuildFontFromDat();
			mLoadedCoords = true;
			return true;
		}
	}

	return false;
}

}}
