#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/helper/jpeg-compressor/jpge.h>
#include <eepp/graphics/texturesaver.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

SubTexture::SubTexture() :
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

SubTexture::SubTexture( const Uint32& TexId, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::hash( mName ) ),
	mTexId( TexId ),
	mTexture( TextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect( Recti( 0, 0, NULL != mTexture ? mTexture->ImgWidth() : 0, NULL != mTexture ? mTexture->ImgHeight() : 0 ) ),
	mDestSize( (Float)mSrcRect.size().width(), (Float)mSrcRect.size().height() ),
	mOffset(0,0)
{
	CreateUnnamed();
}

SubTexture::SubTexture( const Uint32& TexId, const Recti& SrcRect, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::hash( mName ) ),
	mTexId( TexId ),
	mTexture( TextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect( SrcRect ),
	mDestSize( (Float)( mSrcRect.Right - mSrcRect.Left ), (Float)( mSrcRect.Bottom - mSrcRect.Top ) ),
	mOffset(0,0)
{
	CreateUnnamed();
}

SubTexture::SubTexture( const Uint32& TexId, const Recti& SrcRect, const Sizef& DestSize, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::hash( mName ) ),
	mTexId( TexId ),
	mTexture( TextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestSize(DestSize),
	mOffset(0,0)
{
	CreateUnnamed();
}

SubTexture::SubTexture( const Uint32& TexId, const Recti& SrcRect, const Sizef& DestSize, const Vector2i &Offset, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::hash( mName ) ),
	mTexId( TexId ),
	mTexture( TextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestSize(DestSize),
	mOffset(Offset)
{
	CreateUnnamed();
}

SubTexture::~SubTexture() {
	ClearCache();
}

void SubTexture::CreateUnnamed() {
	if ( !mName.size() )
		Name( std::string( "unnamed" ) );
}

const Uint32& SubTexture::Id() const {
	return mId;
}

const std::string SubTexture::Name() const {
	return mName;
}

void SubTexture::Name( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );
}

const Uint32& SubTexture::Texture() {
	return mTexId;
}

void SubTexture::Texture( const Uint32& TexId ) {
	mTexId		= TexId;
	mTexture	= TextureFactory::instance()->GetTexture( TexId );
}

const Recti& SubTexture::SrcRect() const {
	return mSrcRect;
}

void SubTexture::SrcRect( const Recti& Rect ) {
	mSrcRect = Rect;

	if ( NULL != mPixels )
		CacheColors();

	if ( NULL != mAlpha )
		CacheAlphaMask();
}

const Sizef& SubTexture::DestSize() const {
	return mDestSize;
}

void SubTexture::DestSize( const Sizef& destSize ) {
	mDestSize = destSize;
}

const Vector2i& SubTexture::Offset() const {
	return mOffset;
}

void SubTexture::Offset( const Vector2i& offset ) {
	mOffset = offset;
}

void SubTexture::Draw( const Float& X, const Float& Y, const ColorA& Color, const Float& Angle, const Vector2f& Scale, const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect, OriginPoint Center ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffset.x, Y + mOffset.y, mDestSize.x, mDestSize.y, Angle, Scale, Color, Color, Color, Color, Blend, Effect, Center, mSrcRect );
}

void SubTexture::Draw( const Float& X, const Float& Y, const Float& Angle, const Vector2f& Scale, const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect, OriginPoint Center ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffset.x, Y + mOffset.y, mDestSize.x, mDestSize.y, Angle, Scale, Color0, Color1, Color2, Color3, Blend, Effect, Center, mSrcRect );
}

void SubTexture::Draw( const Quad2f Q, const Vector2f& Offset, const Float& Angle, const Vector2f& Scale, const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const EE_BLEND_MODE& Blend ) {
	if ( NULL != mTexture )
		mTexture->DrawQuadEx( Q, Offset, Angle, Scale, Color0, Color1, Color2, Color3, Blend, mSrcRect );
}

Graphics::Texture * SubTexture::GetTexture() {
	return mTexture;
}

void SubTexture::ReplaceColor( ColorA ColorKey, ColorA NewColor ) {
	mTexture->Lock();

	for ( int y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		for ( int x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
			if ( mTexture->GetPixel( x, y ) == ColorKey )
				mTexture->SetPixel( x, y, NewColor );
		}
	}

	mTexture->Unlock( false, true );
}

void SubTexture::CreateMaskFromColor(ColorA ColorKey, Uint8 Alpha) {
	ReplaceColor( ColorKey, ColorA( ColorKey.r(), ColorKey.g(), ColorKey.b(), Alpha ) );
}

void SubTexture::CreateMaskFromColor(RGB ColorKey, Uint8 Alpha) {
	CreateMaskFromColor( ColorA( ColorKey.r(), ColorKey.g(), ColorKey.b(), 255 ), Alpha );
}

void SubTexture::CacheAlphaMask() {
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

			mAlpha[ rX + rY * rW ] = mTexture->GetPixel( x, y ).a();
		}
	}

	mTexture->Unlock();
}

void SubTexture::CacheColors() {
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

			if ( Channels >= 1 ) mPixels[ Pos ]		= tColor.r();
			if ( Channels >= 2 ) mPixels[ Pos + 1 ]	= tColor.g();
			if ( Channels >= 3 ) mPixels[ Pos + 2 ]	= tColor.b();
			if ( Channels >= 4 ) mPixels[ Pos + 3 ]	= tColor.a();
		}
	}

	mTexture->Unlock();
}

Uint8 SubTexture::GetAlphaAt( const Int32& X, const Int32& Y ) {
	if ( mTexture->LocalCopy() )
		return mTexture->GetPixel( mSrcRect.Left + X, mSrcRect.Right + Y ).a();

	if ( NULL != mAlpha )
		return mAlpha[ X + Y * ( mSrcRect.Right - mSrcRect.Left ) ];

	if ( NULL != mPixels )
		return mPixels[ ( X + Y * ( mSrcRect.Right - mSrcRect.Left ) ) * mTexture->Channels() + 3 ];

	CacheAlphaMask();

	return GetAlphaAt( X, Y );
}

ColorA SubTexture::GetColorAt( const Int32& X, const Int32& Y ) {
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

void SubTexture::SetColorAt( const Int32& X, const Int32& Y, const ColorA& Color ) {
	if ( NULL != mPixels ) {
		Uint32 Channels = mTexture->Channels();
		unsigned int Pos = ( X + Y * ( mSrcRect.Right - mSrcRect.Left ) ) * Channels;

		if ( Channels >= 1 ) mPixels[ Pos ]		= Color.r();
		if ( Channels >= 2 ) mPixels[ Pos + 1 ]	= Color.g();
		if ( Channels >= 3 ) mPixels[ Pos + 2 ]	= Color.b();
		if ( Channels >= 4 ) mPixels[ Pos + 3 ]	= Color.a();
	} else {
		CacheColors();
		SetColorAt( X, Y, Color );
	}
}

void SubTexture::ClearCache() {
	eeSAFE_DELETE_ARRAY( mPixels );
	eeSAFE_DELETE_ARRAY( mAlpha );
}

Uint8 * SubTexture::Lock() {
	CacheColors();

	return &mPixels[0];
}

bool SubTexture::Unlock( const bool& KeepData, const bool& Modified ) {
	if ( NULL != mPixels  && NULL != mTexture ) {
		if ( Modified ) {
			TextureSaver saver( mTexture->Handle() );

			Uint32 Channels = mTexture->Channels();
			Uint32 Channel = GL_RGBA;

			if ( 3 == Channels )
				Channel = GL_RGB;
			else if ( 2 == Channels )
				Channel = GL_LUMINANCE_ALPHA;
			else if ( 1 == Channels )
				Channel = GL_ALPHA;

			glTexSubImage2D( GL_TEXTURE_2D, 0, mSrcRect.Left, mSrcRect.Top, mSrcRect.size().width(), mSrcRect.size().height(), Channel, GL_UNSIGNED_BYTE, reinterpret_cast<const void *> ( &mPixels[0] ) );
		}

		if ( !KeepData ) {
			eeSAFE_DELETE_ARRAY( mPixels );
		}

		return true;
	}

	return false;
}

Sizei SubTexture::RealSize() {
	return mSrcRect.size();
}

Sizei SubTexture::Size() {
	return Sizei( (Int32)mDestSize.x, (Int32)mDestSize.y );
}

const Uint8* SubTexture::GetPixelsPtr() {
	if ( mPixels == NULL ) {
		Lock();
		Unlock(true);
	}

	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

bool SubTexture::SaveToFile(const std::string& filepath, const EE_SAVE_TYPE& Format) {
	bool Res = false;

	Lock();

	if ( NULL != mTexture ) {
		if ( SAVE_TYPE_JPG != Format ) {
			Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, RealSize().width(), RealSize().height(), mTexture->Channels(), GetPixelsPtr() ) );
		} else {
			jpge::params params;
			params.m_quality = Image::JpegQuality();
			Res = jpge::compress_image_to_jpeg_file( filepath.c_str(), RealSize().width(), RealSize().height(), mTexture->Channels(), GetPixelsPtr(), params);
		}
	}

	Unlock();

	return Res;
}

void SubTexture::ResetDestSize() {
	Sizei Size = mSrcRect.size();
	mDestSize.x	= (Float)Size.width();
	mDestSize.y = (Float)Size.height();
}

}}
