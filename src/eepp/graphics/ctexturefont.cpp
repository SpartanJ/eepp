#include <eepp/graphics/ctexturefont.hpp>
#include <eepp/system/ciostreamfile.hpp>
#include <eepp/system/ciostreammemory.hpp>

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
		mTexColumns		= TexColumns;
		mTexRows		= TexRows;
		mStartChar		= StartChar;
		mNumChars		= NumChars;

		mtX				= ( 1 / static_cast<eeFloat>( mTexColumns ) );
		mtY				= ( 1 / static_cast<eeFloat>( mTexRows ) );

		mFWidth			= (eeFloat)( Tex->Width() / mTexColumns );
		mFHeight		= (eeFloat)( Tex->Height() / mTexRows );
		mHeight			= mSize = mLineSkip = (eeUint)mFHeight;

		mVerticalDraw	= VerticalDraw;

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

void cTextureFont::BuildFromGlyphs() {
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

		Top = 		mHeight + mDescent 	- tGlyph.GlyphH - tGlyph.MinY;
		Bottom = 	mHeight + mDescent 	+ tGlyph.GlyphH - tGlyph.MaxY;

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

bool cTextureFont::Load( const Uint32& TexId, const std::string& CoordinatesDatPath, const bool& VerticalDraw ) {
	if ( FileSystem::FileExists( CoordinatesDatPath ) ) {
		cIOStreamFile IOS( CoordinatesDatPath, std::ios::in | std::ios::binary );

		return LoadFromStream( TexId, IOS, VerticalDraw );
	} else if ( cPackManager::instance()->FallbackToPacks() ) {
		std::string tPath( CoordinatesDatPath );

		cPack * tPack = cPackManager::instance()->Exists( tPath );

		if ( NULL != tPack ) {
			return LoadFromPack( TexId, tPack, tPath, VerticalDraw );
		}
	}

	return false;
}

bool cTextureFont::LoadFromPack( const Uint32& TexId, cPack* Pack, const std::string& FilePackPath, const bool& VerticalDraw ) {
	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( FilePackPath ) ) {
		SafeDataPointer PData;

		Pack->ExtractFileToMemory( FilePackPath, PData );

		return LoadFromMemory( TexId, reinterpret_cast<const char*> ( PData.Data ), PData.DataSize, VerticalDraw );
	}

	return false;
}

bool cTextureFont::LoadFromMemory( const Uint32& TexId, const char* CoordData, const Uint32& CoordDataSize, const bool& VerticalDraw ) {
	cIOStreamMemory IOS( CoordData, CoordDataSize );

	return LoadFromStream( TexId, IOS, VerticalDraw );
}

bool cTextureFont::LoadFromStream( const Uint32& TexId, cIOStream& IOS, const bool& VerticalDraw ) {
	mTexId = TexId;

	if ( mTexId > 0 ) {
		mVerticalDraw = VerticalDraw;

		if ( IOS.IsOpen() ) {
			sFntHdr FntHdr;

			IOS.Read( (char*)&FntHdr, sizeof(sFntHdr) );

			if ( EE_TTF_FONT_MAGIC != FntHdr.Magic )
				return false;

			mStartChar	= FntHdr.FirstChar;
			mNumChars	= FntHdr.NumChars;
			mSize		= FntHdr.Size;
			mHeight		= FntHdr.Height;
			mLineSkip	= FntHdr.LineSkip;
			mAscent		= FntHdr.Ascent;
			mDescent	= FntHdr.Descent;

			mGlyphs.resize( mNumChars );

			// Read the glyphs
			IOS.Read( (char*)&mGlyphs[0], sizeof(eeGlyph) * mNumChars );

			BuildFromGlyphs();

			mLoadedCoords = true;

			return true;
		}
	}

	return false;
}

}}
