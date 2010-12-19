#ifndef EE_GRAPHICSCSCROLLPARALLAX_H
#define EE_GRAPHICSCSCROLLPARALLAX_H

#include "base.hpp"
#include "ctexture.hpp"
#include "cshape.hpp"

namespace EE { namespace Graphics {

class EE_API cScrollParallax {
	public:
		cScrollParallax();
		~cScrollParallax();

		/** Constructor that create's the Scroll Parallax */
		cScrollParallax( cShape * Shape, const eeFloat& DestX, const eeFloat& DestY, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeVector2f& Speed, const eeColorA& Color = eeColorA(), const EE_PRE_BLEND_FUNC& Blend = ALPHA_NORMAL );

		/** Create's the Scroll Parallax
		* @param Shape The Shape to Draw
		* @param DestX The X position
		* @param DestY The Y position
		* @param DestWidth The Width of the Parallax
		* @param DestHeight The Height of the Parallax
		* @param Speed Speed of movement ( in Pixels Per Second )
		* @param Color The Texture Color
		* @param Effect The Blend Mode ( default ALPHA_NORMAL )
		* @return True if success
		*/
		bool Create( cShape * Shape, const eeFloat& DestX, const eeFloat& DestY, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeVector2f& Speed, const eeColorA& Color = eeColorA(), const EE_PRE_BLEND_FUNC& Blend = ALPHA_NORMAL );

		/** Set the Color */
		void Color( const eeColorA& Color ) { mColor = Color; }

		/** Get the color */
		eeColorA Color() const { return mColor; }

		/** Set the Blend Mode */
		void BlendMode( const EE_PRE_BLEND_FUNC& Blend ) { mBlend = Blend; }

		/** @return The Blend Mode */
		const EE_PRE_BLEND_FUNC& BlendMode() const { return mBlend; }

		/** Draw the Scroll Parallax
		* @param XDirVel X Direction Speed to move the parallax.
		* @param YDirVel Y Direction Speed to move the parallax.
		*/
		void Draw();
		
		/** Change the size of the current parallax
		* @param DestWidth The Width of the Parallax
		* @param DestHeight The Height of the Parallax
		*/
		void Size( const eeFloat& DestWidth, const eeFloat& DestHeight );
		
		/** @return Size */
		const eeSizef& Size() const;
		
		/** Change the Parallax position
		* @param Pos New Position
		*/
		void Position( const eeVector2f& Pos );
		
		/** @return Position */
		const eeVector2f& Position() const;
		
		/** @return Shape */
		cShape * Shape() const;
		
		/** Set Shape */
		void Shape( cShape * shape );
		
		/** Set the parallax speed */
		void Speed( const eeVector2f& speed );
		
		/** @return The parallax speed */
		const eeVector2f& Speed() const;
	private:
		cShape * 			mShape;
		EE_PRE_BLEND_FUNC 	mBlend;
		eeColorA 			mColor;
		eeVector2f			mInitPos;
		eeVector2f			mPos;
		eeVector2f			mSpeed;
		eeSizef				mSize;
		eeRecti				mRect;
		cTimeElapsed		mElapsed;
		eeVector2i			mTiles;
		eeRectf				mAABB;
		eeSizef				mRealSize;
		
		void SetShape();
		
		void SetAABB();
};

}}

#endif
