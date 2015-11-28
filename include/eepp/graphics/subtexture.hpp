#ifndef EE_GRAPHICSCSUBTEXTURE_H
#define EE_GRAPHICSCSUBTEXTURE_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/math/originpoint.hpp>

namespace EE { namespace Graphics {

/** @brief A SubTexture is a part of a texture that represent an sprite.*/
class EE_API SubTexture {
	public:
		/** Creates an empty SubTexture */
		SubTexture();

		/** Creates a SubTexture from a Texture. It will use the full Texture as a SubTexture.
		*	@param TexId The texture id
		*	@param Name The texture name ( if any )
		*/
		SubTexture( const Uint32& TexId, const std::string& Name = "" );

		/** Creates a SubTexture of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the SubTexture.
		*	@param Name The texture name ( if any )
		*/
		SubTexture( const Uint32& TexId, const Recti& SrcRect, const std::string& Name = "" );

		/** Creates a SubTexture of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the SubTexture.
		*	@param DestSize The destination size that the SubTexture will have when rendered.
		*	@param Name The texture name ( if any )
		*/
		SubTexture( const Uint32& TexId, const Recti& SrcRect, const Sizef& DestSize, const std::string& Name = "" );

		/** Creates a SubTexture of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the SubTexture.
		*	@param DestSize The destination size that the SubTexture will have when rendered.
		*	@param Offset The offset that will be added to the position passed when any Draw call is used.
		*	@param Name The texture name ( if any )
		*/
		SubTexture( const Uint32& TexId, const Recti& SrcRect, const Sizef& DestSize, const Vector2i& Offset, const std::string& Name = "" );

		virtual ~SubTexture();

		/** @return The SubTexture Id. The Id is the String::Hash of the SubTexture name. */
		const Uint32& Id() const;

		/** @return The SubTexture Name. */
		const std::string Name() const;

		/** Sets the SubTexture Name, it will also change the Id. */
		void Name( const std::string& name );

		/** @return The Texture Id that holds the SubTexture. */
		const Uint32& Texture();

		/** Set the Texture Id that holds the SubTexture. */
		void Texture( const Uint32& TexId );

		/** @return The Texture sector that represents the SubTexture */
		const Recti& SrcRect() const;

		/** Sets the Texture sector that represents the SubTexture */
		void SrcRect( const Recti& Rect );

		/** @return The Destination Size of the SubTexture. */
		const Sizef& DestSize() const;

		/** Sets the Destination Size of the SubTexture.
		*	The size can be different from the original size of the SubTexture.
		*	For example if the SubTexture width is 32 pixels, by default the destination width is 32 pixels, but it can be changed to anything wanted. */
		void DestSize( const Sizef& destSize );

		/** @return The SubTexture default offset. The offset is added to the position passed when is drawed. */
		const Vector2i& Offset() const;

		/** Set the SubTexture offset. */
		void Offset( const Vector2i& offset );

		void Draw( const Float& X, const Float& Y, const ColorA& Color = ColorA(), const Float& Angle = 0.f, const Vector2f& Scale = Vector2f::One, const EE_BLEND_MODE& Blend = ALPHA_NORMAL, const EE_RENDER_MODE& Effect = RN_NORMAL, OriginPoint Center = OriginPoint(OriginPoint::OriginCenter) );

		void Draw( const Float& X, const Float& Y, const Float& Angle, const Vector2f& Scale, const ColorA& Color0 = ColorA(), const ColorA& Color1 = ColorA(), const ColorA& Color2 = ColorA(), const ColorA& Color3 = ColorA(), const EE_BLEND_MODE& Blend = ALPHA_NORMAL, const EE_RENDER_MODE& Effect = RN_NORMAL, OriginPoint Center = OriginPoint(OriginPoint::OriginCenter) );

		void Draw( const Quad2f Q, const Vector2f& Offset = Vector2f(), const Float& Angle = 0.f, const Vector2f& Scale = Vector2f::One, const ColorA& Color0 = ColorA(), const ColorA& Color1 = ColorA(), const ColorA& Color2 = ColorA(), const ColorA& Color3 = ColorA(), const EE_BLEND_MODE& Blend = ALPHA_NORMAL );

		/** @return The texture instance used by the SubTexture. */
		Graphics::Texture * GetTexture();

		/** Replaces a color in the SubTexture ( needs Lock() ) */
		void ReplaceColor( ColorA ColorKey, ColorA NewColor );

		/** Creates a mask from a color.  */
		void CreateMaskFromColor( ColorA ColorKey, Uint8 Alpha );

		/** Creates a mask from a color. */
		void CreateMaskFromColor( RGB ColorKey, Uint8 Alpha );

		/** Creates a copy of the alpha mask to memory from the texture loaded in VRAM. */
		void CacheAlphaMask();

		/** Creates a copy in memory from the texture loaded in VRAM.  */
		void CacheColors();

		/** @return The alpha value that corresponds to the position indicated in the SubTexture.
		*	If the SubTexture wasn't locked before this call, it will be locked automatically. */
		Uint8 GetAlphaAt( const Int32& X, const Int32& Y );

		/** @return The color that corresponds to the position indicated in the SubTexture.
		*	If the SubTexture wasn't locked before this call, it will be locked automatically. */
		ColorA GetColorAt( const Int32& X, const Int32& Y );

		/** @brief Set a color to the position indicated in the SubTexture.
		*	If the SubTexture wasn't locked before this call, it will be locked automatically.
		*/
		void SetColorAt( const Int32& X, const Int32& Y, const ColorA& Color );

		/** Deletes the texture buffer from memory ( not from VRAM ) if it was cached before ( using Lock() ). */
		void ClearCache();

		/** @brief Locks the texture to be able to perform read/write operations.
		*	@see Texture::Lock */
		Uint8 * Lock();

		/** @brief Unlocks the current texture locked.
		*	@see Texture::Unlock */
		bool Unlock( const bool& KeepData = false, const bool& Modified = false );

		/** @return The SubTexture size in the texture. This is the source rect size. */
		Sizei RealSize();

		/** @return This is the same as Destination Size but with the values rounded as integers. */
		Sizei Size();

		/** @return A pixel pointer to the texture loaded in memory ( downloaded from VRAM doing Lock()/Unlock() ). */
		const Uint8* GetPixelsPtr();

		/** Saves the SubTexture to a file in the file format specified.
		*	This will get the Texture from VRAM ( it will not work with OpenGL ES ) */
		bool SaveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format );

		/** Sets the Destination Size as the Source Rect Size ( the real size of the SubTexture ). */
		void ResetDestSize();
	protected:
		Uint8 *		mPixels;
		Uint8 *		mAlpha;
		std::string mName;
		Uint32		mId;
		Uint32 		mTexId;
		Graphics::Texture * 	mTexture;
		Recti		mSrcRect;
		Sizef		mDestSize;
		Vector2i	mOffset;

		void CreateUnnamed();
};

}}

#endif
