#include "clight.hpp"

namespace EE { namespace Gaming {

cLight::cLight() : mActive(true), mCalculated(false), mRadio(0.0f), mColor(255,255,255), mType(LIGHT_NORMAL) {
}

cLight::~cLight() {
}

cLight::cLight( const eeFloat& Radio, const eeFloat& x, const eeFloat& y, const eeColor& Color, LIGHT_TYPE Type ) : mActive(true), mCalculated(false) {
	Create( Radio, x, y, Color, Type );
}

void cLight::Create( const eeFloat& Radio, const eeFloat& x, const eeFloat& y, const eeColor& Color, LIGHT_TYPE Type ) {
	mRadio = Radio;
	mPos.x = x;
	mPos.y = y;
	mColor = Color;
	mType = Type;
}

eeColor cLight::ProcessVertex( const eeFloat& PointX, const eeFloat& PointY, const eeColor& VertexColor, const eeColor& BaseColor ) {
	eeFloat VertexDist;

	if ( mActive ) {
		if ( mType == LIGHT_NORMAL )
			VertexDist = fabs( Distance( mPos.x, mPos.y, PointX, PointY ) );
		else {
			eeFloat XDist = fabs(mPos.x - PointX);
			eeFloat YDist = fabs(mPos.y - PointY);
			VertexDist = sqrt( (XDist * 0.5f) * (XDist * 0.5f) + YDist * YDist ) * 2.0f;
		}

		if ( VertexDist <= mRadio ) {
            eeColor TmpRGB;
            Uint8 TmpColor;
            eeFloat LightC;

			LightC = fabs( static_cast<eeFloat> ( mColor.R() - BaseColor.R() ) ) / mRadio;
			TmpColor = Uint8( (eeFloat)mColor.R() - (VertexDist * LightC) );
			TmpRGB.Red = VertexColor.R() + ( TmpColor - VertexColor.R() );

			LightC = fabs( static_cast<eeFloat> ( mColor.G() - BaseColor.G() ) ) / mRadio;
			TmpColor = Uint8( (eeFloat)mColor.G() - (VertexDist * LightC) );
			TmpRGB.Green = VertexColor.G() + ( TmpColor - VertexColor.G() );

			LightC = fabs( static_cast<eeFloat> ( mColor.B() - BaseColor.B() ) ) / mRadio;
			TmpColor = Uint8( (eeFloat)mColor.B() - (VertexDist * LightC) );
			TmpRGB.Blue = VertexColor.B() + ( TmpColor - VertexColor.B() );

			if ( TmpRGB.R() < VertexColor.R() ) TmpRGB.Red = VertexColor.R();
			if ( TmpRGB.G() < VertexColor.G() ) TmpRGB.Green = VertexColor.G();
			if ( TmpRGB.B() < VertexColor.B() ) TmpRGB.Blue = VertexColor.B();
			
			return TmpRGB;
		}
	}
	return BaseColor;
}

eeColor cLight::ProcessVertex( const eeVector2f& Pos, const eeColor& VertexColor, const eeColor& BaseColor ) {
	return ProcessVertex( Pos.x, Pos.y, VertexColor, BaseColor );
}

void cLight::UpdatePos( const eeFloat& x, const eeFloat& y ) {
	mPos.x = x;
	mPos.y = y;
}

void cLight::UpdatePos( const eeVector2f& newPos ) {
	mPos = newPos;
}

void cLight::Move( const eeFloat& addtox, const eeFloat& addtoy ) {
	mPos.x += addtox;
	mPos.y += addtoy;
}

eeAABB cLight::GetAABB() {
	if ( mType == LIGHT_NORMAL )
		return eeAABB( mPos.x - mRadio, mPos.y - mRadio, mPos.x + mRadio, mPos.y + mRadio );
	else
		return eeAABB( mPos.x - mRadio, mPos.y - mRadio * 0.5f, mPos.x + mRadio, mPos.y + mRadio * 0.5f );
}

}}
