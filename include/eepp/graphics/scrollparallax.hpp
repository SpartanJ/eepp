#ifndef EE_GRAPHICSCSCROLLPARALLAX_H
#define EE_GRAPHICSCSCROLLPARALLAX_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/textureregion.hpp>

#include <eepp/system/clock.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The scroll parallax renders a TextureRegion to the screen from a position and a size
*specified. If the size is bigger than the TextureRegion, the TextureRegion is rendered as a
*repeated TextureRegion until it covers all the size of the parallax, adding movement to more than
*one Scroll Parallax will generate the ilusion of depth. *	More info in wikipedia:
*http://en.wikipedia.org/wiki/Parallax_scrolling
*/
class EE_API ScrollParallax {
  public:
	ScrollParallax();

	~ScrollParallax();

	/** Constructor that create's the Scroll Parallax
	 * @param textureRegion The TextureRegion to Draw
	 * @param position The position of the parallax
	 * @param size The size of the parallax
	 * @param speed Speed of movement ( in Pixels Per Second )
	 * @param color The Texture Color
	 * @param Blend The Blend Mode ( default BlendAlpha ) */
	ScrollParallax( TextureRegion* textureRegion, const Vector2f& position = Vector2f(),
					const Sizef& size = Sizef(), const Vector2f& speed = Vector2f(),
					const Color& color = Color::White, const BlendMode& Blend = BlendAlpha );

	/** Create's the Scroll Parallax
	 * @param textureRegion The TextureRegion to Draw
	 * @param position The position of the parallax
	 * @param size The size of the parallax
	 * @param speed Speed of movement ( in Pixels Per Second )
	 * @param color The Texture Color
	 * @param Blend The Blend Mode ( default BlendAlpha )
	 * @return True if success
	 */
	bool create( TextureRegion* textureRegion, const Vector2f& position = Vector2f(),
				 const Sizef& size = Sizef(), const Vector2f& speed = Vector2f(),
				 const Color& color = Color::White, const BlendMode& Blend = BlendAlpha );

	/** Set the parallax texture color. */
	void setColor( const Color& Color ) { mColor = Color; }

	/** Get the parallax texture color. */
	Color getColor() const { return mColor; }

	/** Set the Blend Mode used. */
	void setBlendMode( const BlendMode& Blend ) { mBlend = Blend; }

	/** @return The Blend Mode used for the parallax. */
	const BlendMode& getBlendMode() const { return mBlend; }

	/** Draw the Scroll Parallax. */
	void draw();

	/** Change the size of the current parallax
	 * @param size The new size */
	void setSize( const Sizef& size );

	/** @return Size */
	const Sizef& getSize() const;

	/** Change the Parallax position
	 * @param Pos The new parallax position */
	void setPosition( const Vector2f& Pos );

	/** @return The parallax position */
	const Vector2f& getPosition() const;

	/** @return The TextureRegion used for the parallax.*/
	TextureRegion* getTextureRegion() const;

	/** Set Change the TextureRegion used for the parallax. */
	void setTextureRegion( TextureRegion* textureRegion );

	/** Set the parallax speed movement. */
	void setSpeed( const Vector2f& speed );

	/** @return The parallax movement speed. */
	const Vector2f& getSpeed() const;

  private:
	TextureRegion* mTextureRegion;
	BlendMode mBlend;
	Color mColor;
	Vector2f mInitPos;
	Vector2f mPos;
	Vector2f mSpeed;
	Sizef mSize;
	Rect mRect;
	Clock mElapsed;
	Vector2i mTiles;
	Rectf mAABB;
	Sizef mRealSize;

	void setTextureRegion();

	void setAABB();
};

}} // namespace EE::Graphics

#endif
