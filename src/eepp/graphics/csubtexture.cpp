#include <eepp/graphics/csubtexture.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/helper/jpeg-compressor/jpge.h>
#include <eepp/graphics/ctexturesaver.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

cSubTexture::cSubTexture() :
	mPixels(NULL),
	mAlpha(NULL),
	mId(0),
	mTexId(0),
	mTexture(NULL),
	mSrcRect( Recti(0,0,0,0) ),
	mDestSize(0,0),
	mOffset(0,0)
{
	CreateUnnamed();
}

cSubTexture::cSubTexture( const Uint32& TexId, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::Hash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect( Recti( 0, 0, NULL != mTexture ? mTexture->ImgWidth() : 0, NULL != mTexture ? mTexture->ImgHeight() : 0 ) ),
	mDestSize( (Float)mSrcRect.Size().Width(), (Float)mSrcRect.Size().Height() ),
	mOffset(0,0)
{
	CreateUnnamed();
}

cSubTexture::cSubTexture( const Uint32& TexId, const Recti& SrcRect, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::Hash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect( SrcRect ),
	mDestSize( (Float)( mSrcRect.Right - mSrcRect.Left ), (Float)( mSrcRect.Bottom - mSrcRect.Top ) ),
	mOffset(0,0)
{
	CreateUnnamed();
}

cSubTexture::cSubTexture( const Uint32& TexId, const Recti& SrcRect, const Sizef& DestSize, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::Hash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestSize(DestSize),
	mOffset(0,0)
{
	CreateUnnamed();
}

cSubTexture::cSubTexture( const Uint32& TexId, const Recti& SrcRect, const Sizef& DestSize, const Vector2i &Offset, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::Hash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestSize(DestSize),
	mOffset(Offset)
{
	CreateUnnamed();
}

cSubTexture::~cSubTexture() {
	ClearCache();
}

void cSubTexture::CreateUnnamed() {
	if ( !mName.size() )
		Name( std::string( "unnamed" ) );
}

const Uint32& cSubTexture::Id() const {
	return mId;
}

const std::string cSubTexture::Name() const {
	return mName;
}

void cSubTexture::Name( const std::string& name ) {
	mName = name;
	mId = String::Hash( mName );
}

const Uint32& cSubTexture::Texture() {
	return mTexId;
}

void cSubTexture::Texture( const Uint32& TexId ) {
	mTexId		= TexId;
	mTexture	= cTextureFactory::instance()->GetTexture( TexId );
}

const Recti& cSubTexture::SrcRect() const {
	return mSrcRect;
}

void cSubTexture::SrcRect( const Recti& Rect ) {
	mSrcRect = Rect;

	if ( NULL != mPixels )
		CacheColors();

	if ( NULL != mAlpha )
		CacheAlphaMask();
}

const Sizef& cSubTexture::DestSize() const {
	return mDestSize;
}

void cSubTexture::DestSize( const Sizef& destSize ) {
	mDestSize = destSize;
}

const Vector2i& cSubTexture::Offset() const {
	return mOffset;
}

void cSubTexture::Offset( const Vector2i& offset ) {
	mOffset = offset;
}

void cSubTexture::Draw( const Float& X, const Float& Y, const ColorA& Color, const Float& Angle, const Vector2f& Scale, const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect, OriginPoint Center ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffset.x, Y + mOffset.y, mDestSize.x, mDestSize.y, Angle, Scale, Color, Color, Color, Color, Blend, Effect, Center, mSrcRect );
}

void cSubTexture::Draw( const Float& X, const Float& Y, const Float& Angle, const Vector2f& Scale, const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect, OriginPoint Center ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffset.x, Y + mOffset.y, mDestSize.x, mDestSize.y, Angle, Scale, Color0, Color1, Color2, Color3, Blend, Effect, Center, mSrcRect );
}

void cSubTexture::Draw( const Quad2f Q, const Vector2f& Offset, const Float& Angle, const Vector2f& Scale, const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const EE_BLEND_MODE& Blend ) {
	if ( NULL != mTexture )
		mTexture->DrawQuadEx( Q, Offset, Angle, Scale, Color0, Color1, Color2, Color3, Blend, mSrcRect );
}

cTexture * cSubTexture::GetTexture() {
	return mTexture;
}

void cSubTexture::ReplaceColor( ColorA ColorKey, ColorA NewColor ) {
	mTexture->Lock();

	for ( int y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		for ( int x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
			if ( mTexture->GetPixel( x, y ) == ColorKey )
				mTexture->SetPixel( x, y, NewColor );
		}
	}

	mTexture->Unlock( false, true );
}

void cSubTexture::CreateMaskFromColor(ColorA ColorKey, Uint8 Alpha) {
	ReplaceColor( ColorKey, ColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), Alpha ) );
}

void cSubTexture::CreateMaskFromColor(RGB ColorKey, Uint8 Alpha) {
	CreateMaskFromColor( ColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), 255 ), Alpha );
}

void cSubTexture::CacheAlphaMask() {
	Uint32 size = ( mSrcRect.Right - mSrcRect.Left ) * ( mSrcRect.Bottom - mSrcRect.Top );

	eeSAFE_DELETE_ARRAY( mAlpha );
	mAlpha = eeNewArray( Uint8, size );

	mTexture->Lock();

	int rY = 0;
	int rX = 0;
	int rW = mSrcRect.Right - mSrcRect.Left;

	for ( int y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		rY = y - mSrcRect.Top;

		for ( int x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
			rX = x - mSrcRect.Left;

			mAlpha[ rX + rY * rW ] = mTexture->GetPixel( x, y ).A();
		}
	}

	mTexture->Unlock();
}

void cSubTexture::CacheColors() {
	mTexture->Lock();

	Uint32 size =  ( mSrcRect.Right - mSrcRect.Left ) * ( mSrcRect.Bottom - mSrcRect.Top ) * mTexture->Channels();

	eeSAFE_DELETE_ARRAY( mPixels );

	mPixels = eeNewArray( Uint8, size );

	int rY = 0;
	int rX = 0;
	int rW = mSrcRect.Right - mSrcRect.Left;
	ColorA tColor;
	Uint32 Channels = mTexture->Channels();
	int Pos;

	for ( int y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		rY = y - mSrcRect.Top;

		for ( int x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
			rX = x - mSrcRect.Left;

			tColor = mTexture->GetPixel( x, y );

			Pos = ( rX + rY * rW ) * Channels;

			if ( Channels >= 1 ) mPixels[ Pos ]		= tColor.R();
			if ( Channels >= 2 ) mPixels[ Pos + 1 ]	= tColor.G();
			if ( Channels >= 3 ) mPixels[ Pos + 2 ]	= tColor.B();
			if ( Channels >= 4 ) mPixels[ Pos + 3 ]	= tColor.A();
		}
	}

	mTexture->Unlock();
}

Uint8 cSubTexture::GetAlphaAt( const Int32& X, const Int32& Y ) {
	if ( mTexture->LocalCopy() )
		return mTexture->GetPixel( mSrcRect.Left + X, mSrcRect.Right + Y ).A();

	if ( NULL != mAlpha )
		return mAlpha[ X + Y * ( mSrcRect.Right - mSrcRect.Left ) ];

	if ( NULL != mPixels )
		return mPixels[ ( X + Y * ( mSrcRect.Right - mSrcRect.Left ) ) * mTexture->Channels() + 3 ];

	CacheAlphaMask();

	return GetAlphaAt( X, Y );
}

ColorA cSubTexture::GetColorAt( const Int32& X, const Int32& Y ) {
	if ( mTexture->LocalCopy() )
		return mTexture->GetPixel( mSrcRect.Left + X, mSrcRect.Right + Y );

	if ( NULL != mPixels ) {
		Uint32 Channels = mTexture->Channels();
		unsigned int Pos = ( X + Y * ( mSrcRect.Right - mSrcRect.Left ) ) * Channels;

		if ( 4 == Channels )
			return ColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], mPixels[ Pos + 3 ] );
		else if ( 3 == Channels )
			return ColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], 255 );
		else if ( 2 == Channels )
			return ColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], 255, 255 );
		else
			return ColorA( mPixels[ Pos ], 255, 255, 255 );
	}

	CacheColors();

	return GetColorAt( X, Y );
}

void cSubTexture::SetColorAt( const Int32& X, const Int32& Y, const ColorA& Color ) {
	if ( NULL != mPixels ) {
		Uint32 Channels = mTexture->Channels();
		unsigned int Pos = ( X + Y * ( mSrcRect.Right - mSrcRect.Left ) ) * Channels;

		if ( Channels >= 1 ) mPixels[ Pos ]		= Color.R();
		if ( Channels >= 2 ) mPixels[ Pos + 1 ]	= Color.G();
		if ( Channels >= 3 ) mPixels[ Pos + 2 ]	= Color.B();
		if ( Channels >= 4 ) mPixels[ Pos + 3 ]	= Color.A();
	} else {
		CacheColors();
		SetColorAt( X, Y, Color );
	}
}

void cSubTexture::ClearCache() {
	eeSAFE_DELETE_ARRAY( mPixels );
	eeSAFE_DELETE_ARRAY( mAlpha );
}

Uint8 * cSubTexture::Lock() {
	CacheColors();

	return &mPixels[0];
}

bool cSubTexture::Unlock( const bool& KeepData, const bool& Modified ) {
	if ( NULL != mPixels  && NULL != mTexture ) {
		if ( Modified ) {
			cTextureSaver saver( mTexture->Handle() );

			Uint32 Channels = mTexture->Channels();
			Uint32 Channel = GL_RGBA;

			if ( 3 == Channels )
				Channel = GL_RGB;
			else if ( 2 == Channels )
				Channel = GL_LUMINANCE_ALPHA;
			else if ( 1 == Channels )
				Channel = GL_ALPHA;

			glTexSubImage2D( GL_TEXTURE_2D, 0, mSrcRect.Left, mSrcRect.Top, mSrcRect.Size().Width(), mSrcRect.Size().Height(), Channel, GL_UNSIGNED_BYTE, reinterpret_cast<const void *> ( &mPixels[0] ) );
		}

		if ( !KeepData ) {
			eeSAFE_DELETE_ARRAY( mPixels );
		}

		return true;
	}

	return false;
}

Sizei cSubTexture::RealSize() {
	return mSrcRect.Size();
}

Sizei cSubTexture::Size() {
	return Sizei( (Int32)mDestSize.x, (Int32)mDestSize.y );
}

const Uint8* cSubTexture::GetPixelsPtr() {
	if ( mPixels == NULL ) {
		Lock();
		Unlock(true);
	}

	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

bool cSubTexture::SaveToFile(const std::string& filepath, const EE_SAVE_TYPE& Format) {
	bool Res = false;

	Lock();

	if ( NULL != mTexture ) {
		if ( SAVE_TYPE_JPG != Format ) {
			Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, RealSize().Width(), RealSize().Height(), mTexture->Channels(), GetPixelsPtr() ) );
		} else {
			jpge::params params;
			params.m_quality = cImage::JpegQuality();
			Res = jpge::compress_image_to_jpeg_file( filepath.c_str(), RealSize().Width(), RealSize().Height(), mTexture->Channels(), GetPixelsPtr(), params);
		}
	}

	Unlock();

	return Res;
}

void cSubTexture::ResetDestSize() {
	Sizei Size = mSrcRect.Size();
	mDestSize.x	= (Float)Size.Width();
	mDestSize.y = (Float)Size.Height();
}

}}
