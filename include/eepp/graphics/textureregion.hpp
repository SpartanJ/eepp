#ifndef EE_GRAPHICSTEXTUREREGION_HPP
#define EE_GRAPHICSTEXTUREREGION_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/math/originpoint.hpp>

namespace EE { namespace Graphics {

/** @brief A TextureRegion is a part of a texture that represent an sprite.*/
class EE_API TextureRegion : public DrawableResource {
  public:
	static TextureRegion* New();

	static TextureRegion* New( const Uint32& TexId, const std::string& name = "" );

	static TextureRegion* New( const Uint32& TexId, const Rect& srcRect,
							   const std::string& name = "" );

	static TextureRegion* New( const Uint32& TexId, const Rect& srcRect, const Sizef& destSize,
							   const std::string& name = "" );

	static TextureRegion* New( const Uint32& TexId, const Rect& srcRect, const Sizef& destSize,
							   const Vector2i& offset, const std::string& name = "" );

	/** Creates an empty TextureRegion */
	TextureRegion();

	/** Creates a TextureRegion from a Texture. It will use the full Texture as a TextureRegion.
	 *	@param TexId The texture id
	 *	@param name The texture name ( if any )
	 */
	TextureRegion( const Uint32& TexId, const std::string& name = "" );

	/** Creates a TextureRegion of the indicated part of the texture.
	 *	@param TexId The texture id
	 *	@param srcRect The texture part that will be used as the TextureRegion.
	 *	@param name The texture name ( if any )
	 */
	TextureRegion( const Uint32& TexId, const Rect& srcRect, const std::string& name = "" );

	/** Creates a TextureRegion of the indicated part of the texture.
	 *	@param TexId The texture id
	 *	@param srcRect The texture part that will be used as the TextureRegion.
	 *	@param destSize The destination size that the TextureRegion will have when rendered.
	 *	@param name The texture name ( if any )
	 */
	TextureRegion( const Uint32& TexId, const Rect& srcRect, const Sizef& destSize,
				   const std::string& name = "" );

	/** Creates a TextureRegion of the indicated part of the texture.
	 *	@param TexId The texture id
	 *	@param srcRect The texture part that will be used as the TextureRegion.
	 *	@param destSize The destination size that the TextureRegion will have when rendered.
	 *	@param offset The offset that will be added to the position passed when any Draw call is
	 *used.
	 *	@param name The texture name ( if any )
	 */
	TextureRegion( const Uint32& TexId, const Rect& srcRect, const Sizef& destSize,
				   const Vector2i& offset, const std::string& name = "" );

	virtual ~TextureRegion();

	/** Set the Texture Id that holds the TextureRegion. */
	void setTextureId( const Uint32& TexId );

	/** Set the Texture that holds the TextureRegion. */
	void setTexture( Texture* texture );

	/** @return The Texture sector that represents the TextureRegion */
	const Rect& getSrcRect() const;

	/** Sets the Texture sector that represents the TextureRegion */
	void setSrcRect( const Rect& rect );

	/** @return The Destination Size of the TextureRegion. */
	const Sizef& getDestSize() const;

	/** Sets the Destination Size of the TextureRegion.
	 *	The size can be different from the original size of the TextureRegion.
	 *	For example if the TextureRegion width is 32 pixels, by default the destination width is 32
	 *pixels, but it can be changed to anything you want. */
	void setDestSize( const Sizef& destSize );

	/** @return The TextureRegion default offset. The offset is added to the position passed when is
	 * drawed. */
	const Vector2i& getOffset() const;

	/** Set the TextureRegion offset. */
	void setOffset( const Vector2i& offset );

	void draw( const Float& X, const Float& Y, const Color& color = Color::White,
			   const Float& Angle = 0.f, const Vector2f& Scale = Vector2f::One,
			   const BlendMode& Blend = BlendAlpha, const RenderMode& Effect = RENDER_NORMAL,
			   OriginPoint Center = OriginPoint( OriginPoint::OriginCenter ) );

	void draw( const Float& X, const Float& Y, const Float& Angle, const Vector2f& Scale,
			   const Color& Color0 = Color::White, const Color& Color1 = Color::White,
			   const Color& Color2 = Color::White, const Color& Color3 = Color::White,
			   const BlendMode& Blend = BlendAlpha, const RenderMode& Effect = RENDER_NORMAL,
			   OriginPoint Center = OriginPoint( OriginPoint::OriginCenter ) );

	void draw( const Quad2f Q, const Vector2f& offset = Vector2f(), const Float& Angle = 0.f,
			   const Vector2f& Scale = Vector2f::One, const Color& Color0 = Color::White,
			   const Color& Color1 = Color::White, const Color& Color2 = Color::White,
			   const Color& Color3 = Color::White, const BlendMode& Blend = BlendAlpha );

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	/** @return The texture instance used by the TextureRegion. */
	Graphics::Texture* getTexture();

	/** Replaces a color in the TextureRegion ( needs Lock() ) */
	void replaceColor( Color ColorKey, Color NewColor );

	/** Creates a mask from a color.  */
	void createMaskFromColor( Color ColorKey, Uint8 Alpha );

	/** Creates a mask from a color. */
	void createMaskFromColor( RGB ColorKey, Uint8 Alpha );

	/** Creates a copy of the alpha mask to memory from the texture loaded in VRAM. */
	void cacheAlphaMask();

	/** Creates a copy in memory from the texture loaded in VRAM.  */
	void cacheColors();

	/** @return The alpha value that corresponds to the position indicated in the TextureRegion.
	 *	If the TextureRegion wasn't locked before this call, it will be locked automatically. */
	Uint8 getAlphaAt( const Int32& X, const Int32& Y );

	/** @return The color that corresponds to the position indicated in the TextureRegion.
	 *	If the TextureRegion wasn't locked before this call, it will be locked automatically. */
	Color getColorAt( const Int32& X, const Int32& Y );

	/** @brief Set a color to the position indicated in the TextureRegion.
	 *	If the TextureRegion wasn't locked before this call, it will be locked automatically.
	 */
	void setColorAt( const Int32& X, const Int32& Y, const Color& Color );

	/** Deletes the texture buffer from memory ( not from VRAM ) if it was cached before ( using
	 * Lock() ). */
	void clearCache();

	/** @brief Locks the texture to be able to perform read/write operations.
	 *	@see Texture::Lock */
	Uint8* lock();

	/** @brief Unlocks the current texture locked.
	 *	@see Texture::Unlock */
	bool unlock( const bool& KeepData = false, const bool& Modified = false );

	/** @return The TextureRegion size in the texture. This is the source rect size. */
	Sizei getRealSize();

	/** @return This is the same as Destination Size but with the values rounded as integers. */
	Sizef getSize();

	/** @return A pixel pointer to the texture loaded in memory ( downloaded from VRAM doing
	 * Lock()/Unlock() ). */
	const Uint8* getPixelsPtr();

	/** Saves the TextureRegion to a file in the file format specified.
	 *	This will get the Texture from VRAM ( it will not work with OpenGL ES ) */
	bool saveToFile(
		const std::string& filepath, const Image::SaveType& Format,
		const Image::FormatConfiguration& imageFormatConfiguration = Image::FormatConfiguration() );

	/** Sets the Destination Size as the Source Rect Size ( the real size of the TextureRegion )
	 * multiplied by the pixel density. */
	void resetDestSize();

	Float getPixelDensity() const;

	void setPixelDensity( const Float& pixelDensity );

	Sizei getDpSize();

	Sizef getPxSize();

	Sizef getOriDestSize() const;

	void setOriDestSize( const Sizef& oriDestSize );

  protected:
	Uint8* mPixels;
	Uint8* mAlphaMask;
	Graphics::Texture* mTexture;
	Rect mSrcRect;
	Sizef mOriDestSize;
	Sizef mDestSize;
	Vector2i mOffset;
	Float mPixelDensity;
};

}} // namespace EE::Graphics

#endif
