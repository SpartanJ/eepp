#include <eepp/gaming/cgameobjectobject.hpp>

#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/clayer.hpp>
#include <eepp/gaming/ctilelayer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cGameObjectObject::cGameObjectObject( Uint32 DataId, const Rectf& rect, cLayer * Layer, const Uint32& Flags ) :
	cGameObject( Flags, Layer ),
	mRect( rect ),
	mPoly( rect ),
	mPos( mRect.Pos() ),
	mDataId( DataId ),
	mSelected( false )
{
}

cGameObjectObject::~cGameObjectObject() {
}

Uint32 cGameObjectObject::Type() const {
	return GAMEOBJECT_TYPE_OBJECT;
}

bool cGameObjectObject::IsType( const Uint32& type ) {
	return ( cGameObjectObject::Type() == type ) ? true : cGameObject::IsType( type );
}

Sizei cGameObjectObject::Size() {
	Sizef size( mRect.Size() );
	return Sizei( size.x, size.y );
}

void cGameObjectObject::Draw() {
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

Vector2f cGameObjectObject::Pos() const {
	return mPos;
}

void cGameObjectObject::Pos( Vector2f pos ) {
	mPoly.Move( pos - mPos );
	mPos	= pos;
	mRect	= Rectf( pos, Sizef( Size().x, Size().y ) );
}

void cGameObjectObject::SetPolygonPoint( Uint32 index, Vector2f p ) {
	switch ( index ) {
		case 0:
		{
			mPoly.SetAt( 1, Vector2f( p.x, mPoly[1].y ) );
			mPoly.SetAt( 3, Vector2f( mPoly[3].x, p.y ) );
			break;
		}
		case 1:
		{
			mPoly.SetAt( 0, Vector2f( p.x, mPoly[0].y ) );
			mPoly.SetAt( 2, Vector2f( mPoly[2].x, p.y ) );
			break;
		}
		case 2:
		{
			mPoly.SetAt( 3, Vector2f( p.x, mPoly[3].y ) );
			mPoly.SetAt( 1, Vector2f( mPoly[1].x, p.y ) );
			break;
		}
		case 3:
		default:
		{
			mPoly.SetAt( 2, Vector2f( p.x, mPoly[2].y ) );
			mPoly.SetAt( 0, Vector2f( mPoly[0].x, p.y ) );
			break;
		}
	}

	mPoly.SetAt( index, p );
	mRect	= mPoly.ToAABB();
	mPos	= Vector2f( mRect.Left, mRect.Top );
	mPoly	= mRect;
}

Uint32 cGameObjectObject::DataId() {
	return mDataId;
}

void cGameObjectObject::DataId( Uint32 Id ) {
	mDataId = Id;
}

void cGameObjectObject::ClearProperties() {
	mProperties.clear();
}

void cGameObjectObject::AddProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void cGameObjectObject::EditProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void cGameObjectObject::RemoveProperty( std::string Text ) {
	mProperties.erase( Text );
}

cGameObjectObject::PropertiesMap& cGameObjectObject::GetProperties() {
	return mProperties;
}

Uint32 cGameObjectObject::GetPropertyCount() {
	return mProperties.size();
}

const std::string& cGameObjectObject::Name() const {
	return mName;
}

void cGameObjectObject::Name( const std::string& name ) {
	mName = name;
}

const std::string& cGameObjectObject::TypeName() const {
	return mType;
}

void cGameObjectObject::TypeName( const std::string& type ) {
	mType = type;
}

Polygon2f& cGameObjectObject::GetPolygon() {
	return mPoly;
}

bool cGameObjectObject::PointInside( const Vector2f& p ) {
	return mRect.Contains( p );
}

void cGameObjectObject::SetProperties( const PropertiesMap& prop ) {
	mProperties = prop;
}

const bool& cGameObjectObject::Selected() const {
	return mSelected;
}

void cGameObjectObject::Selected( const bool& sel ) {
	mSelected = sel;
}

cGameObjectObject * cGameObjectObject::Copy() {
	return eeNew( cGameObjectObject, ( mDataId, mRect, mLayer, mFlags ) );
}

}}
