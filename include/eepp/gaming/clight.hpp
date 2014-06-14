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

		cLight( const Float& Radius, const Float& x, const Float& y, const RGB& Color = RGB(255,255,255), LIGHT_TYPE Type = LIGHT_NORMAL );

		virtual ~cLight();

		void Create( const Float& Radius, const Float& x, const Float& y, const RGB& Color = RGB(255,255,255), LIGHT_TYPE Type = LIGHT_NORMAL );

		virtual RGB ProcessVertex( const Float& PointX, const Float& PointY, const RGB& VertexColor, const RGB& BaseColor );

		virtual ColorA ProcessVertex( const Float& PointX, const Float& PointY, const ColorA& VertexColor, const ColorA& BaseColor );

		RGB ProcessVertex( const Vector2f& Pos, const RGB& VertexColor, const RGB& BaseColor );

		ColorA ProcessVertex( const Vector2f& Pos, const ColorA& VertexColor, const ColorA& BaseColor );

		void Move( const Float& addtox, const Float& addtoy );

		void UpdatePos( const Float& x, const Float& y );

		void UpdatePos( const Vector2f& newPos );

		eeAABB GetAABB() const;

		const Float& Radius() const;

		void Radius( const Float& radio );

		const bool& Active() const;

		void Active( const bool& active );

		void Color( const RGB& color );

		const RGB& Color() const;

		void Type( const LIGHT_TYPE& type );

		const LIGHT_TYPE& Type() const;

		const Vector2f& Position() const;

		void Position( const Vector2f& newPos );
	protected:
		Float		mRadius;
		Vector2f	mPos;
		RGB		mColor;
		LIGHT_TYPE	mType;
		eeAABB		mAABB;
		bool		mActive;

		void UpdateAABB();
};

}}

#endif
