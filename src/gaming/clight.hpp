#ifndef EE_GAMINGCLIGHT_H
#define EE_GAMINGCLIGHT_H

#include "base.hpp"

namespace EE { namespace Gaming {

/** @enum LIGHT_TYPE Define the light spot type */
enum LIGHT_TYPE {
	LIGHT_NORMAL = 0,
	LIGHT_ISOMETRIC = 1
};

class EE_API cLight {
	public:
		cLight();
		virtual ~cLight();

		cLight( const eeFloat& Radio, const eeFloat& x, const eeFloat& y, const eeColor& Color = eeColor(255,255,255), LIGHT_TYPE Type = LIGHT_NORMAL );
		void Create( const eeFloat& Radio, const eeFloat& x, const eeFloat& y, const eeColor& Color = eeColor(255,255,255), LIGHT_TYPE Type = LIGHT_NORMAL );

		virtual eeColor ProcessVertex( const eeFloat& PointX, const eeFloat& PointY, const eeColor& VertexColor, const eeColor& BaseColor );
		eeColor ProcessVertex( const eeVector2f& Pos, const eeColor& VertexColor, const eeColor& BaseColor );

		void Move( const eeFloat& addtox, const eeFloat& addtoy );
		void UpdatePos( const eeFloat& x, const eeFloat& y );
		void UpdatePos( const eeVector2f& newPos );

		eeFloat Radio() const { return mRadio; }
		void Radio( const eeFloat& radio ) { mRadio = radio; }

		bool Active() const { return mActive; }
		void Active( const bool& active ) { mActive = active; }

		void Color( const eeColor& color ) { mColor = color; }
		eeColor Color() const { return mColor; }

		void Type( const LIGHT_TYPE& type ) { mType = type; }
		LIGHT_TYPE Type() const { return mType; }

		eeAABB GetAABB();

		eeVector2f Position() const { return mPos; }
	protected:
		bool mActive, mCalculated;
		eeFloat mRadio;
		eeVector2f mPos;
		eeColor mColor;
		LIGHT_TYPE mType;
};

}}

#endif
