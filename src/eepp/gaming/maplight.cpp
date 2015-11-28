#include <eepp/gaming/maplight.hpp>

namespace EE { namespace Gaming {

MapLight::MapLight() :
	mRadius( 0 ),
	mColor( 255, 255, 255 ),
	mType( LIGHT_NORMAL ),
	mActive( true )
{
}

MapLight::~MapLight() {
}

MapLight::MapLight( const Float& Radius, const Float& x, const Float& y, const RGB& Color, LIGHT_TYPE Type ) :
	mActive( true )
{
	Create( Radius, x, y, Color, Type );
}

void MapLight::Create( const Float& Radius, const Float& x, const Float& y, const RGB& Color, LIGHT_TYPE Type ) {
	mRadius	= Radius;
	mColor	= Color;
	mType	= Type;

	UpdatePos( x, y );
}

RGB MapLight::ProcessVertex( const Float& PointX, const Float& PointY, const RGB& VertexColor, const RGB& BaseColor ) {
	Float VertexDist;

	if ( mActive ) {
		if ( mType == LIGHT_NORMAL )
			VertexDist = eeabs( mPos.Distance( Vector2f( PointX, PointY ) ) );
		else {
			Float XDist = eeabs(mPos.x - PointX) * 0.5f;
			Float YDist = eeabs(mPos.y - PointY);
			VertexDist = eesqrt( XDist * XDist + YDist * YDist ) * 2.0f;
		}

		if ( VertexDist <= mRadius ) {
			RGB	TmpRGB;
			Uint8	TmpColor;
			Float	LightC;

			LightC			= eeabs( static_cast<Float> ( mColor.R() - BaseColor.R() ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.R() - (VertexDist * LightC) );
			TmpRGB.Red		= VertexColor.R() + ( TmpColor - VertexColor.R() );

			LightC			= eeabs( static_cast<Float> ( mColor.G() - BaseColor.G() ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.G() - (VertexDist * LightC) );
			TmpRGB.Green	= VertexColor.G() + ( TmpColor - VertexColor.G() );

			LightC			= eeabs( static_cast<Float> ( mColor.B() - BaseColor.B() ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.B() - (VertexDist * LightC) );
			TmpRGB.Blue		= VertexColor.B() + ( TmpColor - VertexColor.B() );

			if ( TmpRGB.R() < VertexColor.R() ) TmpRGB.Red		= VertexColor.R();
			if ( TmpRGB.G() < VertexColor.G() ) TmpRGB.Green	= VertexColor.G();
			if ( TmpRGB.B() < VertexColor.B() ) TmpRGB.Blue		= VertexColor.B();
			
			return TmpRGB;
		}
	}

	return BaseColor;
}

ColorA MapLight::ProcessVertex( const Float& PointX, const Float& PointY, const ColorA& VertexColor, const ColorA& BaseColor ) {
	Float VertexDist;

	if ( mActive ) {
		if ( mType == LIGHT_NORMAL )
			VertexDist = eeabs( mPos.Distance( Vector2f( PointX, PointY ) ) );
		else {
			Float XDist = eeabs(mPos.x - PointX) * 0.5f;
			Float YDist = eeabs(mPos.y - PointY);
			VertexDist = eesqrt( XDist * XDist + YDist * YDist ) * 2.0f;
		}

		if ( VertexDist <= mRadius ) {
			ColorA	TmpRGB;
			Uint8		TmpColor;
			Float		LightC;

			LightC			= eeabs( static_cast<Float> ( mColor.R() - BaseColor.R() ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.R() - (VertexDist * LightC) );
			TmpRGB.Red		= VertexColor.R() + ( TmpColor - VertexColor.R() );

			LightC			= eeabs( static_cast<Float> ( mColor.G() - BaseColor.G() ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.G() - (VertexDist * LightC) );
			TmpRGB.Green	= VertexColor.G() + ( TmpColor - VertexColor.G() );

			LightC			= eeabs( static_cast<Float> ( mColor.B() - BaseColor.B() ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.B() - (VertexDist * LightC) );
			TmpRGB.Blue		= VertexColor.B() + ( TmpColor - VertexColor.B() );

			if ( TmpRGB.R() < VertexColor.R() ) TmpRGB.Red		= VertexColor.R();
			if ( TmpRGB.G() < VertexColor.G() ) TmpRGB.Green	= VertexColor.G();
			if ( TmpRGB.B() < VertexColor.B() ) TmpRGB.Blue		= VertexColor.B();

			return TmpRGB;
		}
	}

	return BaseColor;
}

RGB MapLight::ProcessVertex( const Vector2f& Pos, const RGB& VertexColor, const RGB& BaseColor ) {
	return ProcessVertex( Pos.x, Pos.y, VertexColor, BaseColor );
}

ColorA MapLight::ProcessVertex( const Vector2f& Pos, const ColorA& VertexColor, const ColorA& BaseColor ) {
	return ProcessVertex( Pos.x, Pos.y, VertexColor, BaseColor );
}

void MapLight::UpdatePos( const Float& x, const Float& y ) {
	mPos.x = x;
	mPos.y = y;
	UpdateAABB();
}

void MapLight::Position( const Vector2f& newPos ) {
	UpdatePos( newPos.x, newPos.y );
}

void MapLight::UpdatePos( const Vector2f& newPos ) {
	UpdatePos( newPos.x, newPos.y );
}

void MapLight::Move( const Float& addtox, const Float& addtoy ) {
	UpdatePos( mPos.x + addtox, mPos.y + addtoy );
}

eeAABB MapLight::GetAABB() const {
	return mAABB;
}

void MapLight::UpdateAABB() {
	if ( mType == LIGHT_NORMAL )
		mAABB = eeAABB( mPos.x - mRadius, mPos.y - mRadius, mPos.x + mRadius, mPos.y + mRadius );
	else
		mAABB = eeAABB( mPos.x - mRadius, mPos.y - mRadius * 0.5f, mPos.x + mRadius, mPos.y + mRadius * 0.5f );
}

const Float& MapLight::Radius() const {
	return mRadius;
}

void MapLight::Radius( const Float& radius ) {
	if ( radius > 0 ) {
		mRadius = radius;
		UpdateAABB();
	}
}

const bool& MapLight::Active() const {
	return mActive;
}

void MapLight::Active( const bool& active ) {
	mActive = active;
}

void MapLight::Color( const RGB& color ) {
	mColor = color;
}

const RGB& MapLight::Color() const {
	return mColor;
}

void MapLight::Type( const LIGHT_TYPE& type ) {
	mType = type;
	UpdateAABB();
}

const LIGHT_TYPE& MapLight::Type() const {
	return mType;
}

const Vector2f& MapLight::Position() const {
	return mPos;
}

}}
