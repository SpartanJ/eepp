#include "cshape.hpp"
#include "ctexturefactory.hpp"

namespace EE { namespace Graphics {

cShape::cShape() :
	mPixels(NULL),
	mAlpha(NULL),
	mId(0),
	mTexId(0),
	mTexture(NULL),
	mSrcRect( eeRecti(0,0,0,0) ),
	mDestWidth(0),
	mDestHeight(0),
	mOffSetX(0),
	mOffSetY(0)
{
	CreateUnnamed();
}

cShape::cShape( const Uint32& TexId, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( MakeHash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect( eeRecti( 0, 0, mTexture->Width(), mTexture->Height() ) ),
	mDestWidth( (eeFloat)mTexture->Width() ),
	mDestHeight( (eeFloat)mTexture->Height() ),
	mOffSetX(0),
	mOffSetY(0)
{
	CreateUnnamed();
}

cShape::cShape( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( MakeHash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestWidth( (eeFloat)( mSrcRect.Right - mSrcRect.Left ) ),
	mDestHeight( (eeFloat)( mSrcRect.Bottom - mSrcRect.Top ) ),
	mOffSetX(0),
	mOffSetY(0)
{
	CreateUnnamed();
}

cShape::cShape( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( MakeHash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestWidth(DestWidth),
	mDestHeight(DestHeight),
	mOffSetX(0),
	mOffSetY(0)
{
	CreateUnnamed();
}

cShape::cShape( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name ) :
	mPixels(NULL),
	mAlpha(NULL),
	mName( Name ),
	mId( MakeHash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestWidth(DestWidth),
	mDestHeight(DestHeight),
	mOffSetX(OffsetX),
	mOffSetY(OffsetY)
{
	CreateUnnamed();
}

cShape::~cShape() {
	ClearCache();
}

void cShape::CreateUnnamed() {
	if ( !mName.size() )
		Name( std::string( "unnamed" ) );
}

const Uint32& cShape::Id() const {
	return mId;
}

const std::string cShape::Name() const {
	return mName;
}

void cShape::Name( const std::string& name ) {
	mName = name;
	mId = MakeHash( mName );
}

const Uint32& cShape::Texture() {
	return mTexId;
}

void cShape::Texture( const Uint32& TexId ) {
	mTexId = TexId;
	mTexture = cTextureFactory::instance()->GetTexture( TexId );
}

const eeRecti& cShape::SrcRect() const {
	return mSrcRect;
}

void cShape::SrcRect( const eeRecti& Rect ) {
	mSrcRect = Rect;

	if ( NULL != mPixels )
		CacheColors();

	if ( NULL != mAlpha )
		CacheAlphaMask();
}

const eeFloat& cShape::DestWidth() const {
	return mDestWidth;
}

void cShape::DestWidth( const eeFloat& width ) {
	mDestWidth = width;
}

const eeFloat& cShape::DestHeight() const {
	return mDestHeight;
}

void cShape::DestHeight( const eeFloat& height ) {
	mDestHeight = height;
}

const eeFloat& cShape::OffsetX() const {
	return mOffSetX;
}

void cShape::OffsetX( const eeFloat& offsetx ) {
	mOffSetX = offsetx;
}

const eeFloat& cShape::OffsetY() const {
	return mOffSetY;
}

void cShape::OffsetY( const eeFloat& offsety ) {
	mOffSetY = offsety;
}

void cShape::Draw( const eeFloat& X, const eeFloat& Y, const eeColorA& Color, const eeFloat& Angle, const eeFloat& Scale, const EE_PRE_BLEND_FUNC& Blend, const EE_RENDERTYPE& Effect, const bool& ScaleRendered ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffSetX, Y + mOffSetY, mDestWidth, mDestHeight, Angle, Scale, Color, Color, Color, Color, Blend, Effect, ScaleRendered, mSrcRect );
}

void cShape::Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Angle, const eeFloat& Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_PRE_BLEND_FUNC& Blend, const EE_RENDERTYPE& Effect, const bool& ScaleRendered ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffSetX, Y + mOffSetY, mDestWidth, mDestHeight, Angle, Scale, Color0, Color1, Color2, Color3, Blend, Effect, ScaleRendered, mSrcRect );
}

void cShape::Draw( const eeQuad2f Q, const eeFloat& X, const eeFloat& Y, const eeFloat& Angle, const eeFloat& Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_PRE_BLEND_FUNC& Blend ) {
	if ( NULL != mTexture )
		mTexture->DrawQuadEx( Q, X, Y, Angle, Scale, Color0, Color1, Color2, Color3, Blend, mSrcRect );
}

cTexture * cShape::GetTexture() {
	return mTexture;
}

void cShape::ReplaceColor( eeColorA ColorKey, eeColorA NewColor ) {
	mTexture->Lock();

	for ( eeInt y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		for ( eeInt x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
			if ( mTexture->GetPixel( x, y ) == ColorKey )
				mTexture->SetPixel( x, y, NewColor );
		}
	}

	mTexture->Unlock( false, true );
}

void cShape::CreateMaskFromColor(eeColorA ColorKey, Uint8 Alpha) {
	ReplaceColor( ColorKey, eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), Alpha ) );
}

void cShape::CreateMaskFromColor(eeColor ColorKey, Uint8 Alpha) {
	CreateMaskFromColor( eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), 255 ), Alpha );
}

void cShape::CacheAlphaMask() {
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

void cShape::CacheColors() {
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

Uint8 cShape::GetAlphaAt( const Int32& X, const Int32& Y ) {
	if ( mTexture->LocalCopy() )
		return mTexture->GetPixel( mSrcRect.Left + X, mSrcRect.Right + Y ).A();

	if ( NULL != mAlpha )
		return mAlpha[ X + Y * ( mSrcRect.Right - mSrcRect.Left ) ];

	if ( NULL != mPixels )
		return mPixels[ ( X + Y * ( mSrcRect.Right - mSrcRect.Left ) ) * mTexture->Channels() + 3 ];

	CacheAlphaMask();

	return GetAlphaAt( X, Y );
}

eeColorA cShape::GetColorAt( const Int32& X, const Int32& Y ) {
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

void cShape::SetColorAt( const Int32& X, const Int32& Y, const eeColorA& Color ) {
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

void cShape::ClearCache() {
	eeSAFE_DELETE_ARRAY( mPixels );
	eeSAFE_DELETE_ARRAY( mAlpha );
}

Uint8 * cShape::Lock() {
	CacheColors();

	return &mPixels[0];
}

bool cShape::Unlock( const bool& KeepData, const bool& Modified ) {
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

eeSize cShape::RealSize() {
	return mSrcRect.Size();
}

eeSize cShape::Size() {
	return eeSize( (Int32)mDestWidth, (Int32)mDestHeight );
}

const Uint8* cShape::GetPixelsPtr() {
	if ( mPixels == NULL ) {
		Lock();
		Unlock(true);
	}

	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

bool cShape::SaveToFile(const std::string& filepath, const EE_SAVE_TYPE& Format) {
	bool Res = false;

	Lock();

	if ( mTexture )
		Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, RealSize().Width(), RealSize().Height(), 4, GetPixelsPtr() ) );

	Unlock();

	return Res;
}

void cShape::ResetDestWidthAndHeight() {
	eeSize Size = mSrcRect.Size();
	mDestWidth 	= (eeFloat)Size.Width();
	mDestHeight = (eeFloat)Size.Height();
}

}}
