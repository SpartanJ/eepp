#include <eepp/gaming/gameobjectobject.hpp>

#include <eepp/gaming/tilemap.hpp>
#include <eepp/gaming/maplayer.hpp>
#include <eepp/gaming/tilemaplayer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

GameObjectObject::GameObjectObject( Uint32 DataId, const Rectf& rect, MapLayer * Layer, const Uint32& Flags ) :
	GameObject( Flags, Layer ),
	mRect( rect ),
	mPoly( rect ),
	mPos( mRect.getPosition() ),
	mDataId( DataId ),
	mSelected( false )
{
}

GameObjectObject::~GameObjectObject() {
}

Uint32 GameObjectObject::getType() const {
	return GAMEOBJECT_TYPE_OBJECT;
}

bool GameObjectObject::isType( const Uint32& type ) {
	return ( GameObjectObject::getType() == type ) ? true : GameObject::isType( type );
}

Sizei GameObjectObject::getSize() {
	Sizef size( mRect.getSize() );
	return Sizei( size.x, size.y );
}

void GameObjectObject::draw() {
	Int32 selAdd	= mSelected ? 50 : 0;
	Int32 colFill	= 100 + selAdd;

	Primitives P;
	P.setFillMode( DRAW_FILL );
	P.setColor( ColorA( colFill, colFill, colFill, colFill ) );
	P.drawRectangle( mRect );

	P.setFillMode( DRAW_LINE );
	P.setColor( ColorA( 255, 255, 0, 200 ) );
	P.drawRectangle( mRect );
}

Vector2f GameObjectObject::getPosition() const {
	return mPos;
}

void GameObjectObject::setPosition( Vector2f pos ) {
	mPoly.move( pos - mPos );
	mPos	= pos;
	mRect	= Rectf( pos, Sizef( getSize().x, getSize().y ) );
}

void GameObjectObject::setPolygonPoint( Uint32 index, Vector2f p ) {
	switch ( index ) {
		case 0:
		{
			mPoly.setAt( 1, Vector2f( p.x, mPoly[1].y ) );
			mPoly.setAt( 3, Vector2f( mPoly[3].x, p.y ) );
			break;
		}
		case 1:
		{
			mPoly.setAt( 0, Vector2f( p.x, mPoly[0].y ) );
			mPoly.setAt( 2, Vector2f( mPoly[2].x, p.y ) );
			break;
		}
		case 2:
		{
			mPoly.setAt( 3, Vector2f( p.x, mPoly[3].y ) );
			mPoly.setAt( 1, Vector2f( mPoly[1].x, p.y ) );
			break;
		}
		case 3:
		default:
		{
			mPoly.setAt( 2, Vector2f( p.x, mPoly[2].y ) );
			mPoly.setAt( 0, Vector2f( mPoly[0].x, p.y ) );
			break;
		}
	}

	mPoly.setAt( index, p );
	mRect	= mPoly.toAABB();
	mPos	= Vector2f( mRect.Left, mRect.Top );
	mPoly	= mRect;
}

Uint32 GameObjectObject::getDataId() {
	return mDataId;
}

void GameObjectObject::setDataId( Uint32 Id ) {
	mDataId = Id;
}

void GameObjectObject::clearProperties() {
	mProperties.clear();
}

void GameObjectObject::addProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void GameObjectObject::editProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void GameObjectObject::removeProperty( std::string Text ) {
	mProperties.erase( Text );
}

GameObjectObject::PropertiesMap& GameObjectObject::getProperties() {
	return mProperties;
}

Uint32 GameObjectObject::getPropertyCount() {
	return mProperties.size();
}

const std::string& GameObjectObject::getName() const {
	return mName;
}

void GameObjectObject::setName( const std::string& name ) {
	mName = name;
}

const std::string& GameObjectObject::getTypeName() const {
	return mType;
}

void GameObjectObject::setTypeName( const std::string& type ) {
	mType = type;
}

Polygon2f& GameObjectObject::getPolygon() {
	return mPoly;
}

bool GameObjectObject::pointInside( const Vector2f& p ) {
	return mRect.contains( p );
}

void GameObjectObject::setProperties( const PropertiesMap& prop ) {
	mProperties = prop;
}

const bool& GameObjectObject::isSelected() const {
	return mSelected;
}

void GameObjectObject::setSelected( const bool& sel ) {
	mSelected = sel;
}

GameObjectObject * GameObjectObject::clone() {
	return eeNew( GameObjectObject, ( mDataId, mRect, mLayer, mFlags ) );
}

}}
