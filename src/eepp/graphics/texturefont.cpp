#include <eepp/graphics/texturefont.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>

namespace EE { namespace Graphics {

TextureFont * TextureFont::New( const std::string FontName ) {
	return eeNew( TextureFont, ( FontName ) );
}

TextureFont::TextureFont( const std::string FontName ) :
	Font( FONT_TYPE_TEX, FontName ),
	mStartChar(0),
	mNumChars(256),
	mLoadedCoords(false)
{
}

TextureFont::~TextureFont() {
}

bool TextureFont::Load( const Uint32& TexId, const unsigned int& StartChar, const unsigned int& Spacing, const unsigned int& TexColumns, const unsigned int& TexRows, const Uint16& NumChars ) {
	Texture * Tex = TextureFactory::instance()->GetTexture( TexId );

	mTexId = TexId;

	if ( NULL != Tex ) {
		mTexColumns		= TexColumns;
		mTexRows		= TexRows;
		mStartChar		= StartChar;
		mNumChars		= NumChars;

		mtX				= ( 1 / static_cast<Float>( mTexColumns ) );
		mtY				= ( 1 / static_cast<Float>( mTexRows ) );

		mFWidth			= (Float)( Tex->Width() / mTexColumns );
		mFHeight		= (Float)( Tex->Height() / mTexRows );
		mHeight			= mSize = mLineSkip = (unsigned int)mFHeight;

		if ( Spacing == 0 )
			mSpacing = static_cast<unsigned int>( mFWidth );
		else
			mSpacing = Spacing;

		BuildFont();

		eePRINTL( "Texture Font %s loaded.", Tex->Filepath().c_str() );

		return true;
	}

	eePRINTL( "Failed to Load Texture Font: Unknown Texture." );

	return false;
}

void TextureFont::BuildFont() {
	Float cX = 0, cY = 0;

	mTexCoords.resize( mNumChars );
	mGlyphs.resize( mNumChars );

	TextureFactory::instance()->Bind( mTexId );

	int c = 0;

	for (unsigned int i = 0; i < mNumChars; i++) {
		if ( i >= mStartChar || ( mStartChar <= 32 && i == 9 ) ) {
			c = i;

			// Little hack to always provide a tab
			if ( 9 == i ) {
				c = 32;
			}

			cX = (Float)( (c-mStartChar) % mTexColumns ) / (Float)mTexColumns;
			cY = (Float)( (c-mStartChar) / mTexColumns ) / (Float)mTexRows;

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

void TextureFont::BuildFromGlyphs() {
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

bool TextureFont::Load( const Uint32& TexId, const std::string& CoordinatesDatPath ) {
	if ( FileSystem::FileExists( CoordinatesDatPath ) ) {
		IOStreamFile IOS( CoordinatesDatPath, std::ios::in | std::ios::binary );

		return LoadFromStream( TexId, IOS );
	} else if ( PackManager::instance()->FallbackToPacks() ) {
		std::string tPath( CoordinatesDatPath );

		Pack * tPack = PackManager::instance()->Exists( tPath );

		if ( NULL != tPack ) {
			return LoadFromPack( TexId, tPack, tPath );
		}
	}

	return false;
}

bool TextureFont::LoadFromPack( const Uint32& TexId, Pack* Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( FilePackPath ) ) {
		SafeDataPointer PData;

		Pack->ExtractFileToMemory( FilePackPath, PData );

		return LoadFromMemory( TexId, reinterpret_cast<const char*> ( PData.Data ), PData.DataSize );
	}

	return false;
}

bool TextureFont::LoadFromMemory( const Uint32& TexId, const char* CoordData, const Uint32& CoordDataSize ) {
	IOStreamMemory IOS( CoordData, CoordDataSize );

	return LoadFromStream( TexId, IOS );
}

bool TextureFont::LoadFromStream( const Uint32& TexId, IOStream& IOS ) {
	mTexId = TexId;

	if ( mTexId > 0 ) {
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
