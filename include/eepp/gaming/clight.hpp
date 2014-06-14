#ifndef EE_GAMINGCLIGHT_H
#define EE_GAMINGCLIGHT_H

#include <eepp/gaming/base.hpp>

namespace EE { namespace Gaming {

/** @enum LIGHT_TYPE Define the light spot type */
enum LIGHT_TYPE {
	LIGHT_NORMAL	= 0,
	LIGHT_ISOMETRIC = 1
};

class EE_API cLight {
	public:
		cLight();

		cLight( const Float& Radius, const Float& x, const Float& y, const eeColor& Color = eeColor(255,255,255), LIGHT_TYPE Type = LIGHT_NORMAL );

		virtual ~cLight();

		void Create( const Float& Radius, const Float& x, const Float& y, const eeColor& Color = eeColor(255,255,255), LIGHT_TYPE Type = LIGHT_NORMAL );

		virtual eeColor ProcessVertex( const Float& PointX, const Float& PointY, const eeColor& VertexColor, const eeColor& BaseColor );

		virtual eeColorA ProcessVertex( const Float& PointX, const Float& PointY, const eeColorA& VertexColor, const eeColorA& BaseColor );

		eeColor ProcessVertex( const eeVector2f& Pos, const eeColor& VertexColor, const eeColor& BaseColor );

		eeColorA ProcessVertex( const eeVector2f& Pos, const eeColorA& VertexColor, const eeColorA& BaseColor );

		void Move( const Float& addtox, const Float& addtoy );

		void UpdatePos( const Float& x, const Float& y );

		void UpdatePos( const eeVector2f& newPos );

		eeAABB GetAABB() const;

		const Float& Radius() const;

		void Radius( const Float& radio );

		const bool& Active() const;

		void Active( const bool& active );

		void Color( const eeColor& color );

		const eeColor& Color() const;

		void Type( const LIGHT_TYPE& type );

		const LIGHT_TYPE& Type() const;

		const eeVector2f& Position() const;

		void Position( const eeVector2f& newPos );
	protected:
		Float		mRadius;
		eeVector2f	mPos;
		eeColor		mColor;
		LIGHT_TYPE	mType;
		eeAABB		mAABB;
		bool		mActive;

		void UpdateAABB();
};

}}

#endif
