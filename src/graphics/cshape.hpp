#ifndef EE_GRAPHICSCSHAPE_H
#define EE_GRAPHICSCSHAPE_H

#include "base.hpp"
#include "ctexture.hpp"

namespace EE { namespace Graphics {

/** @brief A Shape is a part of a texture that represent an sprite.*/
class EE_API cShape {
	public:
		cShape();

		cShape( const Uint32& TexId, const std::string& Name = "" );

		cShape( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name = "" );

		cShape( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name = "" );

		cShape( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name = "" );

		~cShape();

		const Uint32& Id() const;

		const std::string Name() const;

		void Name( const std::string& name );

		const Uint32 Texture();

		void Texture( const Uint32& TexId );

		eeRecti SrcRect() const;

		void SrcRect( const eeRecti& Rect );

		const eeFloat DestWidth() const;

		void DestWidth( const eeFloat& width );

		const eeFloat DestHeight() const;

		void DestHeight( const eeFloat& height );

		const eeFloat OffsetX() const;

		void OffsetX( const eeFloat& offsetx );

		const eeFloat OffsetY() const;

		void OffsetY( const eeFloat& offsety );

		void Draw( const eeFloat& X, const eeFloat& Y, const eeRGBA& Color = eeRGBA(), const eeFloat& Angle = 0.f, const eeFloat& Scale = 1.f, const EE_RENDERALPHAS& Blend = ALPHA_NORMAL, const EE_RENDERTYPE& Effect = RN_NORMAL, const bool& ScaleRendered = true );

		void Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Angle = 0.f, const eeFloat& Scale = 1.f, const eeRGBA& Color0 = eeRGBA(), const eeRGBA& Color1 = eeRGBA(), const eeRGBA& Color2 = eeRGBA(), const eeRGBA& Color3 = eeRGBA(), const EE_RENDERALPHAS& Blend = ALPHA_NORMAL, const EE_RENDERTYPE& Effect = RN_NORMAL, const bool& ScaleRendered = true );

		cTexture * GetTexture();

		void ReplaceColor( eeColorA ColorKey, eeColorA NewColor );

		void CreateMaskFromColor( eeColorA ColorKey, Uint8 Alpha );

		void CreateMaskFromColor( eeColor ColorKey, Uint8 Alpha );

		void CacheAlphaMask();

		void CacheColors();

		Uint8 GetAlphaAt( const Int32& X, const Int32& Y );

		eeColorA GetColorAt( const Int32& X, const Int32& Y );

		void SetColorAt( const Int32& X, const Int32& Y, const eeColorA& Color );

		void ClearCache();

		eeColorA * Lock();

		bool Unlock( const bool& KeepData = false, const bool& Modified = false );

		eeSize RealSize();

		eeSize Size();

		const Uint8* GetPixelsPtr();

		bool SaveToFile(const std::string& filepath, const EE_SAVETYPE& Format);
	protected:
		#ifndef ALLOC_VECTORS
		eeColorA *	mPixels;
		Uint8 *		mAlpha;
		#else
		std::vector<eeColorA> mPixels;
		std::vector<Uint8> mAlpha;
		#endif
		
		std::string mName;
		Uint32		mId;
		Uint32 		mTexId;
		cTexture * 	mTexture;
		eeRecti		mSrcRect;
		eeFloat		mDestWidth;
		eeFloat		mDestHeight;
		eeFloat		mOffSetX;
		eeFloat		mOffSetY;

		void CreateUnnamed();
};

}}

#endif
