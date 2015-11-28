#ifndef EE_GRAPHICSCSCROLLPARALLAX_H
#define EE_GRAPHICSCSCROLLPARALLAX_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace Graphics {

/** @brief The scroll parallax renders a SubTexture to the screen from a position and a size specified. If the size is bigger than the SubTexture, the SubTexture is rendered as a repeated SubTexture until it covers all the size of the parallax, adding movement to more than one Scroll Parallax will generate the ilusion of depth.
**	More info in wikipedia: http://en.wikipedia.org/wiki/Parallax_scrolling
*/
class EE_API ScrollParallax {
	public:
		ScrollParallax();

		~ScrollParallax();

		/** Constructor that create's the Scroll Parallax
		* @param SubTexture The SubTexture to Draw
		* @param Position The position of the parallax
		* @param Size The size of the parallax
		* @param Speed Speed of movement ( in Pixels Per Second )
		* @param Color The Texture Color
		* @param Blend The Blend Mode ( default ALPHA_NORMAL ) */
		ScrollParallax( Graphics::SubTexture * SubTexture, const Vector2f& Position = Vector2f(), const Sizef& Size = Sizef(), const Vector2f& Speed = Vector2f(), const ColorA& Color = ColorA(), const EE_BLEND_MODE& Blend = ALPHA_NORMAL );

		/** Create's the Scroll Parallax
		* @param SubTexture The SubTexture to Draw
		* @param Position The position of the parallax
		* @param Size The size of the parallax
		* @param Speed Speed of movement ( in Pixels Per Second )
		* @param Color The Texture Color
		* @param Blend The Blend Mode ( default ALPHA_NORMAL )
		* @return True if success
		*/
		bool Create( Graphics::SubTexture * SubTexture, const Vector2f& Position = Vector2f(), const Sizef& Size = Sizef(), const Vector2f& Speed = Vector2f(), const ColorA& Color = ColorA(), const EE_BLEND_MODE& Blend = ALPHA_NORMAL );

		/** Set the parallax texture color. */
		void Color( const ColorA& Color ) { mColor = Color; }

		/** Get the parallax texture color. */
		ColorA Color() const { return mColor; }

		/** Set the Blend Mode used. */
		void BlendMode( const EE_BLEND_MODE& Blend ) { mBlend = Blend; }

		/** @return The Blend Mode used for the parallax. */
		const EE_BLEND_MODE& BlendMode() const { return mBlend; }

		/** Draw the Scroll Parallax. */
		void Draw();
		
		/** Change the size of the current parallax
		* @param size The new size */
		void Size( const Sizef& size );
		
		/** @return Size */
		const Sizef& Size() const;
		
		/** Change the Parallax position
		* @param Pos The new parallax position */
		void Position( const Vector2f& Pos );
		
		/** @return The parallax position */
		const Vector2f& Position() const;
		
		/** @return The SubTexture used for the parallax.*/
		Graphics::SubTexture * SubTexture() const;
		
		/** Set Change the SubTexture used for the parallax. */
		void SubTexture( Graphics::SubTexture * subTexture );
		
		/** Set the parallax speed movement. */
		void Speed( const Vector2f& speed );
		
		/** @return The parallax movement speed. */
		const Vector2f& Speed() const;
	private:
		Graphics::SubTexture * 		mSubTexture;
		EE_BLEND_MODE		mBlend;
		ColorA 			mColor;
		Vector2f			mInitPos;
		Vector2f			mPos;
		Vector2f			mSpeed;
		Sizef				mSize;
		Recti				mRect;
		Clock		mElapsed;
		Vector2i			mTiles;
		Rectf				mAABB;
		Sizef				mRealSize;
		
		void SetSubTexture();
		
		void SetAABB();
};

}}

#endif
