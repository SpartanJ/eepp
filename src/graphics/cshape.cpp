#include "cshape.hpp"
#include "ctexturefactory.hpp"
#include "cshapemanager.hpp"

namespace EE { namespace Graphics {

cShape::cShape() :
	mId(0),
	mTexId(0),
	mTexture(NULL),
	mSrcRect( eeRecti(0,0,0,0) ),
	mDestWidth(0),
	mDestHeight(0),
	mOffSetX(0),
	mOffSetY(0)
{
	#ifndef ALLOC_VECTORS
	mPixels = NULL;
	mAlpha = NULL;
	#endif
	CreateUnnamed();
}

cShape::cShape( const Uint32& TexId, const std::string& Name ) :
	mName( Name ),
	mId( MakeHash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::Instance()->GetTexture( TexId ) ),
	mSrcRect( eeRecti( 0, 0, mTexture->Width(), mTexture->Height() ) ),
	mDestWidth( (eeFloat)mTexture->Width() ),
	mDestHeight( (eeFloat)mTexture->Height() ),
	mOffSetX(0),
	mOffSetY(0)
{
	#ifndef ALLOC_VECTORS
	mPixels = NULL;
	mAlpha = NULL;
	#endif
}

cShape::cShape( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name ) :
	mName( Name ),
	mId( MakeHash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::Instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestWidth( (eeFloat)( mSrcRect.Right - mSrcRect.Left ) ),
	mDestHeight( (eeFloat)( mSrcRect.Bottom - mSrcRect.Top ) ),
	mOffSetX(0),
	mOffSetY(0)
{
	#ifndef ALLOC_VECTORS
	mPixels = NULL;
	mAlpha = NULL;
	#endif
}

cShape::cShape( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name ) :
	mName( Name ),
	mId( MakeHash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::Instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestWidth(DestWidth),
	mDestHeight(DestHeight),
	mOffSetX(0),
	mOffSetY(0)
{
	#ifndef ALLOC_VECTORS
	mPixels = NULL;
	mAlpha = NULL;
	#endif
}

cShape::cShape( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name ) :
	mName( Name ),
	mId( MakeHash( mName ) ),
	mTexId( TexId ),
	mTexture( cTextureFactory::Instance()->GetTexture( TexId ) ),
	mSrcRect(SrcRect),
	mDestWidth(DestWidth),
	mDestHeight(DestHeight),
	mOffSetX(OffsetX),
	mOffSetY(OffsetY)
{
	#ifndef ALLOC_VECTORS
	mPixels = NULL;
	mAlpha = NULL;
	#endif
}

cShape::~cShape() {
	ClearCache();
}

void cShape::CreateUnnamed() {
	Name( StrFormated( "unnamed%d", cShapeManager::instance()->Count() - 1 ) );
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

const Uint32 cShape::Texture() {
	return mTexId;
}

void cShape::Texture( const Uint32& TexId ) {
	mTexId = TexId;
	mTexture = cTextureFactory::Instance()->GetTexture( TexId );
}

eeRecti cShape::SrcRect() const {
	return mSrcRect;
}

void cShape::SrcRect( const eeRecti& Rect ) {
	mSrcRect = Rect;

	#ifndef ALLOC_VECTORS
	if ( NULL != mPixels )
	#else
	if ( mPixels.size() )
	#endif
		CacheColors();

	#ifndef ALLOC_VECTORS
	if ( NULL != mAlpha )
	#else
	if ( mAlpha.size() )
	#endif
		CacheAlphaMask();
}

const eeFloat cShape::DestWidth() const {
	return mDestWidth;
}

void cShape::DestWidth( const eeFloat& width ) {
	mDestWidth = width;
}

const eeFloat cShape::DestHeight() const {
	return mDestHeight;
}

void cShape::DestHeight( const eeFloat& height ) {
	mDestHeight = height;
}

const eeFloat cShape::OffsetX() const {
	return mOffSetX;
}

void cShape::OffsetX( const eeFloat& offsetx ) {
	mOffSetX = offsetx;
}

const eeFloat cShape::OffsetY() const {
	return mOffSetY;
}

void cShape::OffsetY( const eeFloat& offsety ) {
	mOffSetY = offsety;
}

void cShape::Draw( const eeFloat& X, const eeFloat& Y, const eeRGBA& Color, const eeFloat& Angle, const eeFloat& Scale, const EE_RENDERALPHAS& Blend, const EE_RENDERTYPE& Effect, const bool& ScaleRendered ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffSetX, Y + mOffSetY, mDestWidth, mDestHeight, Angle, Scale, Color, Color, Color, Color, Blend, Effect, ScaleRendered, mSrcRect );
}

void cShape::Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Angle, const eeFloat& Scale, const eeRGBA& Color0, const eeRGBA& Color1, const eeRGBA& Color2, const eeRGBA& Color3, const EE_RENDERALPHAS& Blend, const EE_RENDERTYPE& Effect, const bool& ScaleRendered ) {
	if ( NULL != mTexture )
		mTexture->DrawEx( X + mOffSetX, Y + mOffSetY, mDestWidth, mDestHeight, Angle, Scale, Color0, Color1, Color2, Color3, Blend, Effect, ScaleRendered, mSrcRect );
}

cTexture * cShape::GetTexture() {
	return mTexture;
}

void cShape::ReplaceColor(eeColorA ColorKey, eeColorA NewColor) {
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

	#ifndef ALLOC_VECTORS
	eeSAFE_DELETE_ARRAY( mAlpha );
	mAlpha = new Uint8[ size ];
	#else
	mAlpha.clear();
	mAlpha.resize( size );
	#endif

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
	Uint32 size =  ( mSrcRect.Right - mSrcRect.Left ) * ( mSrcRect.Bottom - mSrcRect.Top ) ;

	#ifndef ALLOC_VECTORS
	eeSAFE_DELETE_ARRAY( mPixels );
	mPixels = new eeColorA[ size ];
	#else
	mPixels.clear();
	mPixels.resize( size );
	#endif

	mTexture->Lock();

	eeInt rY = 0;
	eeInt rX = 0;
	eeInt rW = mSrcRect.Right - mSrcRect.Left;

	for ( eeInt y = mSrcRect.Top; y < mSrcRect.Bottom; y++ ) {
		rY = y - mSrcRect.Top;

		for ( eeInt x = mSrcRect.Left; x < mSrcRect.Right; x++ ) {
			rX = x - mSrcRect.Left;

			mPixels[ rX + rY * rW ] = mTexture->GetPixel( x, y );
		}
	}

	mTexture->Unlock();
}

Uint8 cShape::GetAlphaAt( const Int32& X, const Int32& Y ) {
	if ( mTexture->LocalCopy() )
		return mTexture->GetPixel( mSrcRect.Left + X, mSrcRect.Right + Y ).A();

	#ifndef ALLOC_VECTORS
	if ( NULL != mAlpha )
	#else
	if ( mAlpha.size() )
	#endif
		return mAlpha[ X + Y * ( mSrcRect.Right - mSrcRect.Left ) ];

	#ifndef ALLOC_VECTORS
	if ( NULL != mPixels )
	#else
	if ( mPixels.size() )
	#endif
		return mPixels[ X + Y * ( mSrcRect.Right - mSrcRect.Left ) ].A();

	CacheAlphaMask();

	return GetAlphaAt( X, Y );
}

eeColorA cShape::GetColorAt( const Int32& X, const Int32& Y ) {
	if ( mTexture->LocalCopy() )
		return mTexture->GetPixel( mSrcRect.Left + X, mSrcRect.Right + Y );

	#ifndef ALLOC_VECTORS
	if ( NULL != mPixels )
	#else
	if ( mPixels.size() )
	#endif
		return mPixels[ X + Y * ( mSrcRect.Right - mSrcRect.Left ) ];

	CacheColors();

	return GetColorAt( X, Y );
}

void cShape::SetColorAt( const Int32& X, const Int32& Y, const eeColorA& Color ) {
	#ifndef ALLOC_VECTORS
	if ( NULL != mPixels )
	#else
	if ( mPixels.size() )
	#endif
		mPixels[ X + Y * ( mSrcRect.Right - mSrcRect.Left ) ] = Color;
	else {
		CacheColors();
		mPixels[ X + Y * ( mSrcRect.Right - mSrcRect.Left ) ] = Color;
	}
}

void cShape::ClearCache() {
	#ifndef ALLOC_VECTORS
	eeSAFE_DELETE_ARRAY( mPixels );
	eeSAFE_DELETE_ARRAY( mAlpha );
	#else
	mPixels.clear();
	mAlpha.clear();
	#endif
}

eeColorA * cShape::Lock() {
	CacheColors();

	return &mPixels[0];
}

bool cShape::Unlock( const bool& KeepData, const bool& Modified ) {
	#ifndef ALLOC_VECTORS
	if ( NULL != mPixels ) {
	#else
	if ( mPixels.size() ) {
	#endif
		if ( Modified ) {
			GLint PreviousTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

			if ( PreviousTexture != (Int32)mTexture->Texture() )
				glBindTexture(GL_TEXTURE_2D, mTexture->Texture() );

			glTexSubImage2D( GL_TEXTURE_2D, 0, mSrcRect.Left, mSrcRect.Top, mSrcRect.Size().Width(), mSrcRect.Size().Height(), GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<const void *> ( &mPixels[0] ) );

			if ( PreviousTexture != (Int32)mTexture->Texture() )
				glBindTexture(GL_TEXTURE_2D, PreviousTexture);
		}

		if ( !KeepData ) {
			#ifndef ALLOC_VECTORS
			eeSAFE_DELETE_ARRAY( mPixels );
			#else
			mPixels.clear();
			#endif
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
	#ifndef ALLOC_VECTORS
	if ( mPixels == NULL ) {
	#else
	if ( !mPixels.size() ) {
	#endif
		Lock();
		Unlock(true);
	}

	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

bool cShape::SaveToFile(const std::string& filepath, const EE_SAVETYPE& Format) {
	bool Res = false;

	Lock();

	if ( mTexture )
		Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, RealSize().Width(), RealSize().Height(), 4, GetPixelsPtr() ) );

	Unlock();

	return Res;
}

}}
