#include <eepp/graphics/csubtexture.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/helper/jpeg-compressor/jpge.h>

namespace EE { namespace Graphics {

cSubTexture::cSubTexture() :
	mPixels(NULL),
	mAlpha(NULL),
	mId(0),
	mTexId(0),
	mTexture(NULL),
	mSrcRect( eeRecti(0,0,0,0) ),
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
	mSrcRect( eeRecti( 0, 0, NULL != mTexture ? mTexture->ImgWidth() : 0, NULL != mTexture ? mTexture->ImgHeight() : 0 ) ),
	mDestSize( (eeFloat)mSrcRect.Size().Width(), (eeFloat)mSrcRect.Size().Height() ),
	mOffset(0,0)
{
	CreateUnnamed();
}

cSubTexture::cSubTexture( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( String::Hash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect( SrcRect ),
	mDestSize( (eeFloat)( mSrcRect.Right - mSrcRect.Left ), (eeFloat)( mSrcRect.Bottom - mSrcRect.Top ) ),
	mOffset(0,0)
{
	CreateUnnamed();
}

cSubTexture::cSubTexture( const Uint32& TexId, const eeRecti& SrcRect, const eeSizef& DestSize, const std::string& Name ) :
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

cSubTexture::cSubTexture( const Uint32& TexId, const eeRecti& SrcRect, const eeSizef& DestSize, const eeVector2i &Offset, const std::string& Name ) :
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

const eeRecti& cSubTexture::SrcRect() const {
	return mSrcRect;
}

void cSubTexture::SrcRect( const eeRecti& Rect ) {
	mSrcRect = Rect;

	if ( NULL != mPixels )
		CacheColors();

	if ( NULL != mAlpha )
		CacheAlphaMask();
}

const eeSizef& cSubTexture::DestSize() const {
	return mDestSize;
}

void cSubTexture::DestSize( const eeSizef& destSize ) {
	mDestSize = destSize;
}

const eeVector2i& cSubTexture::Offset() const {
	return mOffset;
}

void cSubTexture::Offset( const eeVector2i& offset ) {
	mOffset = offset;
}

void cSubTexture::Draw( const eeFloat& X, const eeFloat& Y, const eeColorA& Color, const eeFloat& Angle, const eeFloat& Scale, const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect, eeOriginPoint Center ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffset.x, Y + mOffset.y, mDestSize.x, mDestSize.y, Angle, Scale, Color, Color, Color, Color, Blend, Effect, Center, mSrcRect );
}

void cSubTexture::Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Angle, const eeFloat& Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect, eeOriginPoint Center ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffset.x, Y + mOffset.y, mDestSize.x, mDestSize.y, Angle, Scale, Color0, Color1, Color2, Color3, Blend, Effect, Center, mSrcRect );
}

void cSubTexture::Draw( const eeQuad2f Q, const eeVector2f& Offset, const eeFloat& Angle, const eeFloat& Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_BLEND_MODE& Blend ) {
	if ( NULL != mTexture )
		mTexture->DrawQuadEx( Q, Offset, Angle, Scale, Color0, Color1, Color2, Color3, Blend, mSrcRect );
}

cTexture * cSubTexture::GetTexture() {
	return mTexture;
}

void cSubTexture::ReplaceColor( eeColorA ColorKey, eeColorA NewColor ) {
	mTexture->Lock();

	for ( eeInt y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		for ( eeInt x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
			if ( mTexture->GetPixel( x, y ) == ColorKey )
				mTexture->SetPixel( x, y, NewColor );
		}
	}

	mTexture->Unlock( false, true );
}

void cSubTexture::CreateMaskFromColor(eeColorA ColorKey, Uint8 Alpha) {
	ReplaceColor( ColorKey, eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), Alpha ) );
}

void cSubTexture::CreateMaskFromColor(eeColor ColorKey, Uint8 Alpha) {
	CreateMaskFromColor( eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), 255 ), Alpha );
}

void cSubTexture::CacheAlphaMask() {
	Uint32 size = ( mSrcRect.Right - mSrcRect.Left ) * ( mSrcRect.Bottom - mSrcRect.Top );

	eeSAFE_DELETE_ARRAY( mAlpha );
	mAlpha = eeNewArray( Uint8, size );

	mTexture->Lock();

	eeInt rY = 0;
	eeInt rX = 0;
	eeInt rW = mSrcRect.Right - mSrcRect.Left;

	for ( eeInt y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		rY = y - mSrcRect.Top;

		for ( eeInt x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
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

	eeInt rY = 0;
	eeInt rX = 0;
	eeInt rW = mSrcRect.Right - mSrcRect.Left;
	eeColorA tColor;
	Uint32 Channels = mTexture->Channels();
	eeInt Pos;

	for ( eeInt y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		rY = y - mSrcRect.Top;

		for ( eeInt x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
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

eeColorA cSubTexture::GetColorAt( const Int32& X, const Int32& Y ) {
	if ( mTexture->LocalCopy() )
		return mTexture->GetPixel( mSrcRect.Left + X, mSrcRect.Right + Y );

	if ( NULL != mPixels ) {
		Uint32 Channels = mTexture->Channels();
		eeUint Pos = ( X + Y * ( mSrcRect.Right - mSrcRect.Left ) ) * Channels;

		if ( 4 == Channels )
			return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], mPixels[ Pos + 3 ] );
		else if ( 3 == Channels )
			return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], 255 );
		else if ( 2 == Channels )
			return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], 255, 255 );
		else
			return eeColorA( mPixels[ Pos ], 255, 255, 255 );
	}

	CacheColors();

	return GetColorAt( X, Y );
}

void cSubTexture::SetColorAt( const Int32& X, const Int32& Y, const eeColorA& Color ) {
	if ( NULL != mPixels ) {
		Uint32 Channels = mTexture->Channels();
		eeUint Pos = ( X + Y * ( mSrcRect.Right - mSrcRect.Left ) ) * Channels;

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
	if ( NULL != mPixels ) {
		if ( Modified ) {
			GLint PreviousTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

			if ( PreviousTexture != (Int32)mTexture->Handle() )
				glBindTexture(GL_TEXTURE_2D, mTexture->Handle() );

			Uint32 Channels = mTexture->Channels();
			Uint32 Channel = GL_RGBA;

			if ( 3 == Channels )
				Channel = GL_RGB;
			else if ( 2 == Channels )
				Channel = GL_LUMINANCE_ALPHA;
			else if ( 1 == Channels )
				Channel = GL_ALPHA;

			glTexSubImage2D( GL_TEXTURE_2D, 0, mSrcRect.Left, mSrcRect.Top, mSrcRect.Size().Width(), mSrcRect.Size().Height(), Channel, GL_UNSIGNED_BYTE, reinterpret_cast<const void *> ( &mPixels[0] ) );

			if ( PreviousTexture != (Int32)mTexture->Handle() )
				glBindTexture(GL_TEXTURE_2D, PreviousTexture);
		}

		if ( !KeepData ) {
			eeSAFE_DELETE_ARRAY( mPixels );
		}

		return true;
	}

	return false;
}

eeSize cSubTexture::RealSize() {
	return mSrcRect.Size();
}

eeSize cSubTexture::Size() {
	return eeSize( (Int32)mDestSize.x, (Int32)mDestSize.y );
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
			Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, RealSize().Width(), RealSize().Height(), 4, GetPixelsPtr() ) );
		} else {
			jpge::params params;
			params.m_quality = cImage::JpegQuality();
			Res = jpge::compress_image_to_jpeg_file( filepath.c_str(), RealSize().Width(), RealSize().Height(), 4, GetPixelsPtr(), params);
		}
	}

	Unlock();

	return Res;
}

void cSubTexture::ResetDestSize() {
	eeSize Size = mSrcRect.Size();
	mDestSize.x	= (eeFloat)Size.Width();
	mDestSize.y = (eeFloat)Size.Height();
}

}}
