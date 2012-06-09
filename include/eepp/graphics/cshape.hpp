#ifndef EE_GRAPHICSCSHAPE_H
#define EE_GRAPHICSCSHAPE_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ctexture.hpp>

namespace EE { namespace Graphics {

/** @brief A Shape is a part of a texture that represent an sprite.*/
class EE_API cShape {
	public:
		cShape();

		cShape( const Uint32& TexId, const std::string& Name = "" );

		cShape( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name = "" );

		cShape( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name = "" );

		cShape( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const Int32& OffsetX, const Int32& OffsetY, const std::string& Name = "" );

		virtual ~cShape();

		const Uint32& Id() const;

		const std::string Name() const;

		void Name( const std::string& name );

		const Uint32& Texture();

		void Texture( const Uint32& TexId );

		const eeRecti& SrcRect() const;

		void SrcRect( const eeRecti& Rect );

		const eeFloat& DestWidth() const;

		void DestWidth( const eeFloat& width );

		const eeFloat& DestHeight() const;

		void DestHeight( const eeFloat& height );

		const Int32& OffsetX() const;

		void OffsetX( const Int32& offsetx );

		const Int32& OffsetY() const;

		void OffsetY( const Int32& offsety );

		void Draw( const eeFloat& X, const eeFloat& Y, const eeColorA& Color = eeColorA(), const eeFloat& Angle = 0.f, const eeFloat& Scale = 1.f, const EE_PRE_BLEND_FUNC& Blend = ALPHA_NORMAL, const EE_RENDERTYPE& Effect = RN_NORMAL, const bool& ScaleRendered = true );

		void Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Angle, const eeFloat& Scale, const eeColorA& Color0 = eeColorA(), const eeColorA& Color1 = eeColorA(), const eeColorA& Color2 = eeColorA(), const eeColorA& Color3 = eeColorA(), const EE_PRE_BLEND_FUNC& Blend = ALPHA_NORMAL, const EE_RENDERTYPE& Effect = RN_NORMAL, const bool& ScaleRendered = true );

		void Draw( const eeQuad2f Q, const eeFloat& X, const eeFloat& Y, const eeFloat& Angle = 0.f, const eeFloat& Scale = 1.f, const eeColorA& Color0 = eeColorA(), const eeColorA& Color1 = eeColorA(), const eeColorA& Color2 = eeColorA(), const eeColorA& Color3 = eeColorA(), const EE_PRE_BLEND_FUNC& Blend = ALPHA_NORMAL );

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

		Uint8 * Lock();

		bool Unlock( const bool& KeepData = false, const bool& Modified = false );

		eeSize RealSize();

		eeSize Size();

		const Uint8* GetPixelsPtr();

		bool SaveToFile(const std::string& filepath, const EE_SAVE_TYPE& Format);

		void ResetDestWidthAndHeight();
	protected:
		Uint8 *		mPixels;
		Uint8 *		mAlpha;
		std::string mName;
		Uint32		mId;
		Uint32 		mTexId;
		cTexture * 	mTexture;
		eeRecti		mSrcRect;
		eeFloat		mDestWidth;
		eeFloat		mDestHeight;
		Int32		mOffsetX;
		Int32		mOffsetY;

		void CreateUnnamed();
};

}}

#endif
