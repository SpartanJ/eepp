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
	mPos( mRect.pos() ),
	mDataId( DataId ),
	mSelected( false )
{
}

GameObjectObject::~GameObjectObject() {
}

Uint32 GameObjectObject::Type() const {
	return GAMEOBJECT_TYPE_OBJECT;
}

bool GameObjectObject::IsType( const Uint32& type ) {
	return ( GameObjectObject::Type() == type ) ? true : GameObject::IsType( type );
}

Sizei GameObjectObject::Size() {
	Sizef size( mRect.size() );
	return Sizei( size.x, size.y );
}

void GameObjectObject::Draw() {
	Int32 selAdd	= mSelected ? 50 : 0;
	Int32 colFill	= 100 + selAdd;

	Primitives P;
	P.FillMode( DRAW_FILL );
	P.SetColor( ColorA( colFill, colFill, colFill, colFill ) );
	P.DrawRectangle( mRect );

	P.FillMode( DRAW_LINE );
	P.SetColor( ColorA( 255, 255, 0, 200 ) );
	P.DrawRectangle( mRect );
}

Vector2f GameObjectObject::Pos() const {
	return mPos;
}

void GameObjectObject::Pos( Vector2f pos ) {
	mPoly.move( pos - mPos );
	mPos	= pos;
	mRect	= Rectf( pos, Sizef( Size().x, Size().y ) );
}

void GameObjectObject::SetPolygonPoint( Uint32 index, Vector2f p ) {
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

Uint32 GameObjectObject::DataId() {
	return mDataId;
}

void GameObjectObject::DataId( Uint32 Id ) {
	mDataId = Id;
}

void GameObjectObject::ClearProperties() {
	mProperties.clear();
}

void GameObjectObject::AddProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void GameObjectObject::EditProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void GameObjectObject::RemoveProperty( std::string Text ) {
	mProperties.erase( Text );
}

GameObjectObject::PropertiesMap& GameObjectObject::GetProperties() {
	return mProperties;
}

Uint32 GameObjectObject::GetPropertyCount() {
	return mProperties.size();
}

const std::string& GameObjectObject::Name() const {
	return mName;
}

void GameObjectObject::Name( const std::string& name ) {
	mName = name;
}

const std::string& GameObjectObject::TypeName() const {
	return mType;
}

void GameObjectObject::TypeName( const std::string& type ) {
	mType = type;
}

Polygon2f& GameObjectObject::GetPolygon() {
	return mPoly;
}

bool GameObjectObject::PointInside( const Vector2f& p ) {
	return mRect.contains( p );
}

void GameObjectObject::SetProperties( const PropertiesMap& prop ) {
	mProperties = prop;
}

const bool& GameObjectObject::Selected() const {
	return mSelected;
}

void GameObjectObject::Selected( const bool& sel ) {
	mSelected = sel;
}

GameObjectObject * GameObjectObject::Copy() {
	return eeNew( GameObjectObject, ( mDataId, mRect, mLayer, mFlags ) );
}

}}
