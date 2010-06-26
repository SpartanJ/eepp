#include "ctexture.hpp"

namespace EE { namespace Graphics {

cTexture::cTexture() : 
	mFilepath(""), 
	mId(0),
	mTexture(0), 
	mWidth(0), 
	mHeight(0), 
	mImgWidth(0), 
	mImgHeight(0), 
	mChannels(4), 
	mMipmap(true), 
	mModified(false), 
	mColorKey(true), 
	mClampMode( EE_CLAMP_TO_EDGE ), 
	mFilter( TEX_LINEAR ), 
	mCompressedTexture(false), 
	mLocked(false), 
	mGrabed(false), 
	mPixels(NULL)
{
}

cTexture::cTexture( const cTexture& Copy ) :
	mFilepath( Copy.mFilepath ),
	mId( Copy.mId ),
	mTexture( Copy.mTexture ),
	mWidth( Copy.mWidth ),
	mHeight( Copy.mHeight ),
	mImgWidth( Copy.mImgWidth ),
	mImgHeight( Copy.mImgHeight ),
	mChannels( Copy.mChannels ),
	mMipmap( Copy.mMipmap ),
	mModified( Copy.mModified ),
	mColorKey( Copy.mColorKey ),
	mClampMode( Copy.mClampMode ),
	mFilter( Copy.mFilter ),
	mCompressedTexture( Copy.mCompressedTexture ),
	mLocked( Copy.mLocked ),
	mGrabed ( Copy.mGrabed )
{
	Pixels( reinterpret_cast<const Uint8*>( Copy.mPixels ) );
}

cTexture::~cTexture() {
	DeleteTexture();
}

cTexture& cTexture::operator =(const cTexture& Other) {
    cTexture Temp(Other);

	std::swap(mFilepath, Temp.mFilepath);
	std::swap(mId, Temp.mId);
    std::swap(mTexture, Temp.mTexture);
	std::swap(mWidth, Temp.mWidth);
	std::swap(mHeight, Temp.mHeight);
	std::swap(mImgWidth, Temp.mImgWidth);
	std::swap(mImgHeight, Temp.mImgHeight);
	std::swap(mMipmap, Temp.mMipmap);
	std::swap(mModified, Temp.mModified);
	std::swap(mColorKey, Temp.mColorKey);
	std::swap(mLocked, Temp.mLocked);
	std::swap(mFilter, Temp.mFilter);
	std::swap(mClampMode, Temp.mClampMode);
	std::swap(mCompressedTexture, Temp.mCompressedTexture);
	std::swap(mGrabed, Temp.mGrabed);
	std::swap(mChannels, Temp.mChannels);
	Pixels( reinterpret_cast<const Uint8*>( Temp.mPixels ) );
	
    return *this;
}

void cTexture::DeleteTexture() {
	if ( mTexture ) {
		GLuint Texture = static_cast<GLuint>(mTexture);
		glDeleteTextures(1, &Texture);
		
		mTexture = 0;
		mModified = false;
		mLocked = false;
		mGrabed = false;
		
		eeSAFE_DELETE_ARRAY( mPixels );
	}
}

cTexture::cTexture( const Uint32& texture, const eeInt& width, const eeInt& height, const eeInt& imgwidth, const eeInt& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint8* data ) {
	Create( texture, width, height, imgwidth, imgheight, UseMipmap, Channels, filepath, ColorKey, ClampMode, CompressedTexture, data );
}

void cTexture::Create( const Uint32& texture, const eeInt& width, const eeInt& height, const eeInt& imgwidth, const eeInt& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint8* data ) {
	mFilepath = filepath;
	mId = MakeHash( mFilepath );
	
	mTexture = texture;
	mWidth = width;
	mHeight = height;
	mImgWidth = imgwidth;
	mImgHeight = imgheight;
	mMipmap = UseMipmap;
	
	mColorKey = ColorKey;
	mClampMode = ClampMode;
	mCompressedTexture = CompressedTexture;
	mChannels = Channels;

	mLocked = false;
	mGrabed = false;

	mFilter = TEX_LINEAR;
	
	Pixels(data);
}

void cTexture::Pixels( const Uint8* data ) {
	if ( data != NULL ) {
		eeSAFE_DELETE_ARRAY( mPixels );
		
		eeUint size = (eeUint)mWidth * (eeUint)mHeight;
		mPixels = new eeColorA[ size ];
		
		memcpy( reinterpret_cast<void*>( mPixels ), reinterpret_cast<const void*> ( data ), size );
	}
}

eeColorA* cTexture::Lock() {
	if ( !mLocked ) {
		GLint PreviousTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, mTexture);

		Int32 width = 0, height = 0;
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		mWidth = (eeInt)width;
		mHeight = (eeInt)height;
		
		if ( !( eeARRAY_SIZE( mPixels ) >= (eeUint)width  * (eeUint)height ) ) {
			eeSAFE_DELETE_ARRAY( mPixels );
			eeUint size = (eeUint)mWidth * (eeUint)mHeight;
			mPixels = new eeColorA[ size ];
		}

		glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<Uint8*> (&mPixels[0]) );

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, PreviousTexture);

		mLocked = true;
	}
	return &mPixels[0];
}

bool cTexture::Unlock(const bool& KeepData, const bool& Modified) {
	if ( mLocked ) {
		Int32 width = 0, height = 0;
		GLuint NTexId = 0;

		if ( Modified || mModified )	{
			GLint PreviousTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, mTexture);

			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

			Uint32 flags = mMipmap ? SOIL_FLAG_MIPMAPS : 0;
			flags = (mClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;

			NTexId = SOIL_create_OGL_texture( reinterpret_cast<const Uint8*>(&mPixels[0]), &width, &height, SOIL_LOAD_RGBA, mTexture, flags);
			mChannels = 4;

			SetTextureFilter(mFilter);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, PreviousTexture);

			mModified = false;
		}

		if (!KeepData)
			eeSAFE_DELETE_ARRAY( mPixels );

		mLocked = false;

		if ( (eeInt)NTexId == mTexture || !Modified )
			return true;
	}
	return false;
}

const Uint8* cTexture::GetPixelsPtr() {
	if ( mPixels == NULL ) {
		Lock();
		Unlock(true);
	}

	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

const eeColorA& cTexture::GetPixel(const eeUint& x, const eeUint& y) {
	#ifdef EE_DEBUG
	if ( mPixels == NULL || (eeInt)x > mWidth || (eeInt)y > mHeight )
		return eeColorA::Black;
	#endif

	return mPixels[ x + y * (Uint32)mWidth ];
}

void cTexture::SetPixel(const eeUint& x, const eeUint& y, const eeColorA& Color) {
	#ifdef EE_DEBUG
	if ( mPixels == NULL || (eeInt)x > mWidth || (eeInt)y > mHeight )
		return;
	#endif

	mPixels[ x + y * (eeUint)mWidth ] = Color;

	mModified = true;
}

bool cTexture::SaveToFile(const std::string& filepath, const EE_SAVETYPE& Format) {
	bool Res = false;

	Lock();

	if (mTexture)
		Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, (Int32)mWidth, (Int32)mHeight, 4, GetPixelsPtr() ) );

	Unlock();

	return Res;
}

void cTexture::SetTextureFilter(const EE_TEX_FILTER& filter) {
	if (mTexture) {
		if ( mFilter != filter ) {
			mFilter = filter;

			GLint PreviousTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, mTexture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (mFilter == TEX_LINEAR) ? GL_LINEAR : GL_NEAREST);

			if ( mMipmap )
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilter == TEX_LINEAR) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
			else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilter == TEX_LINEAR) ? GL_LINEAR : GL_NEAREST);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, PreviousTexture);
		}
	}
}

void cTexture::ReplaceColor(eeColorA ColorKey, eeColorA NewColor) {
	Lock();
	
	for ( eeInt i = 0; i < mWidth * mHeight; i++ )
		if ( mPixels[i] == ColorKey )
			mPixels[i] = NewColor;
	
	Unlock(false, true);
}

void cTexture::CreateMaskFromColor(eeColorA ColorKey, Uint8 Alpha) {
	ReplaceColor( ColorKey, eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), Alpha ) );
}

void cTexture::CreateMaskFromColor(eeColor ColorKey, Uint8 Alpha) {
	CreateMaskFromColor( eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), 255 ), Alpha );
}

bool cTexture::LocalCopy() {
	return ( mPixels != NULL );
}

void cTexture::ClampMode( const EE_CLAMP_MODE& clampmode ) {
	if ( mClampMode != clampmode ) {
		ApplyClampMode();
		mClampMode = clampmode;
	}
}

void cTexture::ApplyClampMode() {
	if (mTexture) {
		GLint PreviousTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, mTexture);

		if( mClampMode == EE_CLAMP_REPEAT ) {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		} else {
			unsigned int clamp_mode = 0x812F; // GL_CLAMP_TO_EDGE
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_mode );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_mode );
		}

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, PreviousTexture);
	}
}

void cTexture::ClearCache() {
	eeSAFE_DELETE_ARRAY( mPixels );
}

}}
