#include <eepp/maps/maplight.hpp>

namespace EE { namespace Maps {

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
	create( Radius, x, y, Color, Type );
}

void MapLight::create( const Float& Radius, const Float& x, const Float& y, const RGB& Color, LIGHT_TYPE Type ) {
	mRadius	= Radius;
	mColor	= Color;
	mType	= Type;

	updatePos( x, y );
}

RGB MapLight::processVertex( const Float& PointX, const Float& PointY, const RGB& VertexColor, const RGB& BaseColor ) {
	Float VertexDist;

	if ( mActive ) {
		if ( mType == LIGHT_NORMAL )
			VertexDist = eeabs( mPos.distance( Vector2f( PointX, PointY ) ) );
		else {
			Float XDist = eeabs(mPos.x - PointX) * 0.5f;
			Float YDist = eeabs(mPos.y - PointY);
			VertexDist = eesqrt( XDist * XDist + YDist * YDist ) * 2.0f;
		}

		if ( VertexDist <= mRadius ) {
			RGB	TmpRGB;
			Uint8	TmpColor;
			Float	LightC;

			LightC			= eeabs( static_cast<Float> ( mColor.r - BaseColor.r ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.r - (VertexDist * LightC) );
			TmpRGB.r		= VertexColor.r + ( TmpColor - VertexColor.r );

			LightC			= eeabs( static_cast<Float> ( mColor.g - BaseColor.g ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.g - (VertexDist * LightC) );
			TmpRGB.g	= VertexColor.g + ( TmpColor - VertexColor.g );

			LightC			= eeabs( static_cast<Float> ( mColor.b - BaseColor.b ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.b - (VertexDist * LightC) );
			TmpRGB.b		= VertexColor.b + ( TmpColor - VertexColor.b );

			if ( TmpRGB.r < VertexColor.r ) TmpRGB.r		= VertexColor.r;
			if ( TmpRGB.g < VertexColor.g ) TmpRGB.g	= VertexColor.g;
			if ( TmpRGB.b < VertexColor.b ) TmpRGB.b		= VertexColor.b;
			
			return TmpRGB;
		}
	}

	return BaseColor;
}

Color MapLight::processVertex( const Float& PointX, const Float& PointY, const Color& VertexColor, const Color& BaseColor ) {
	Float VertexDist;

	if ( mActive ) {
		if ( mType == LIGHT_NORMAL )
			VertexDist = eeabs( mPos.distance( Vector2f( PointX, PointY ) ) );
		else {
			Float XDist = eeabs(mPos.x - PointX) * 0.5f;
			Float YDist = eeabs(mPos.y - PointY);
			VertexDist = eesqrt( XDist * XDist + YDist * YDist ) * 2.0f;
		}

		if ( VertexDist <= mRadius ) {
			Color	TmpRGB;
			Uint8		TmpColor;
			Float		LightC;

			LightC			= eeabs( static_cast<Float> ( mColor.r - BaseColor.r ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.r - (VertexDist * LightC) );
			TmpRGB.r		= VertexColor.r + ( TmpColor - VertexColor.r );

			LightC			= eeabs( static_cast<Float> ( mColor.g - BaseColor.g ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.g - (VertexDist * LightC) );
			TmpRGB.g	= VertexColor.g + ( TmpColor - VertexColor.g );

			LightC			= eeabs( static_cast<Float> ( mColor.b - BaseColor.b ) ) / mRadius;
			TmpColor		= Uint8( (Float)mColor.b - (VertexDist * LightC) );
			TmpRGB.b		= VertexColor.b + ( TmpColor - VertexColor.b );

			if ( TmpRGB.r < VertexColor.r ) TmpRGB.r		= VertexColor.r;
			if ( TmpRGB.g < VertexColor.g ) TmpRGB.g	= VertexColor.g;
			if ( TmpRGB.b < VertexColor.b ) TmpRGB.b		= VertexColor.b;

			return TmpRGB;
		}
	}

	return BaseColor;
}

RGB MapLight::processVertex( const Vector2f& Pos, const RGB& VertexColor, const RGB& BaseColor ) {
	return processVertex( Pos.x, Pos.y, VertexColor, BaseColor );
}

Color MapLight::processVertex( const Vector2f& Pos, const Color& VertexColor, const Color& BaseColor ) {
	return processVertex( Pos.x, Pos.y, VertexColor, BaseColor );
}

void MapLight::updatePos( const Float& x, const Float& y ) {
	mPos.x = x;
	mPos.y = y;
	updateAABB();
}

void MapLight::setPosition( const Vector2f& newPos ) {
	updatePos( newPos.x, newPos.y );
}

void MapLight::updatePos( const Vector2f& newPos ) {
	updatePos( newPos.x, newPos.y );
}

void MapLight::move( const Float& addtox, const Float& addtoy ) {
	updatePos( mPos.x + addtox, mPos.y + addtoy );
}

eeAABB MapLight::getAABB() const {
	return mAABB;
}

void MapLight::updateAABB() {
	if ( mType == LIGHT_NORMAL )
		mAABB = eeAABB( mPos.x - mRadius, mPos.y - mRadius, mPos.x + mRadius, mPos.y + mRadius );
	else
		mAABB = eeAABB( mPos.x - mRadius, mPos.y - mRadius * 0.5f, mPos.x + mRadius, mPos.y + mRadius * 0.5f );
}

const Float& MapLight::getRadius() const {
	return mRadius;
}

void MapLight::setRadius( const Float& radius ) {
	if ( radius > 0 ) {
		mRadius = radius;
		updateAABB();
	}
}

const bool& MapLight::isActive() const {
	return mActive;
}

void MapLight::setActive( const bool& active ) {
	mActive = active;
}

void MapLight::setColor( const RGB& color ) {
	mColor = color;
}

const RGB& MapLight::getColor() const {
	return mColor;
}

void MapLight::setType( const LIGHT_TYPE& type ) {
	mType = type;
	updateAABB();
}

const LIGHT_TYPE& MapLight::getType() const {
	return mType;
}

const Vector2f& MapLight::getPosition() const {
	return mPos;
}

}}
