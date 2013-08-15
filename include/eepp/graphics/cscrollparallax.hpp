#ifndef EE_GRAPHICSCSCROLLPARALLAX_H
#define EE_GRAPHICSCSCROLLPARALLAX_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ctexture.hpp>
#include <eepp/graphics/csubtexture.hpp>

namespace EE { namespace Graphics {

/** @brief The scroll parallax renders a SubTexture to the screen from a position and a size specified. If the size is bigger than the SubTexture, the SubTexture is rendered as a repeated SubTexture until it covers all the size of the parallax, adding movement to more than one Scroll Parallax will generate the ilusion of depth.
**	More info in wikipedia: http://en.wikipedia.org/wiki/Parallax_scrolling
*/
class EE_API cScrollParallax {
	public:
		cScrollParallax();

		~cScrollParallax();

		/** Constructor that create's the Scroll Parallax
		* @param SubTexture The SubTexture to Draw
		* @param Position The position of the parallax
		* @param Size The size of the parallax
		* @param Speed Speed of movement ( in Pixels Per Second )
		* @param Color The Texture Color
		* @param Blend The Blend Mode ( default ALPHA_NORMAL ) */
		cScrollParallax( cSubTexture * SubTexture, const eeVector2f& Position = eeVector2f(), const eeSizef& Size = eeSizef(), const eeVector2f& Speed = eeVector2f(), const eeColorA& Color = eeColorA(), const EE_BLEND_MODE& Blend = ALPHA_NORMAL );

		/** Create's the Scroll Parallax
		* @param SubTexture The SubTexture to Draw
		* @param Position The position of the parallax
		* @param Size The size of the parallax
		* @param Speed Speed of movement ( in Pixels Per Second )
		* @param Color The Texture Color
		* @param Blend The Blend Mode ( default ALPHA_NORMAL )
		* @return True if success
		*/
		bool Create( cSubTexture * SubTexture, const eeVector2f& Position = eeVector2f(), const eeSizef& Size = eeSizef(), const eeVector2f& Speed = eeVector2f(), const eeColorA& Color = eeColorA(), const EE_BLEND_MODE& Blend = ALPHA_NORMAL );

		/** Set the parallax texture color. */
		void Color( const eeColorA& Color ) { mColor = Color; }

		/** Get the parallax texture color. */
		eeColorA Color() const { return mColor; }

		/** Set the Blend Mode used. */
		void BlendMode( const EE_BLEND_MODE& Blend ) { mBlend = Blend; }

		/** @return The Blend Mode used for the parallax. */
		const EE_BLEND_MODE& BlendMode() const { return mBlend; }

		/** Draw the Scroll Parallax. */
		void Draw();
		
		/** Change the size of the current parallax
		* @param size The new size */
		void Size( const eeSizef& size );
		
		/** @return Size */
		const eeSizef& Size() const;
		
		/** Change the Parallax position
		* @param Pos The new parallax position */
		void Position( const eeVector2f& Pos );
		
		/** @return The parallax position */
		const eeVector2f& Position() const;
		
		/** @return The SubTexture used for the parallax.*/
		cSubTexture * SubTexture() const;
		
		/** Set Change the SubTexture used for the parallax. */
		void SubTexture( cSubTexture * subTexture );
		
		/** Set the parallax speed movement. */
		void Speed( const eeVector2f& speed );
		
		/** @return The parallax movement speed. */
		const eeVector2f& Speed() const;
	private:
		cSubTexture * 		mSubTexture;
		EE_BLEND_MODE		mBlend;
		eeColorA 			mColor;
		eeVector2f			mInitPos;
		eeVector2f			mPos;
		eeVector2f			mSpeed;
		eeSizef				mSize;
		eeRecti				mRect;
		cClock		mElapsed;
		eeVector2i			mTiles;
		eeRectf				mAABB;
		eeSizef				mRealSize;
		
		void SetSubTexture();
		
		void SetAABB();
};

}}

#endif
