#include <eepp/gaming/cgameobjectobject.hpp>

#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/clayer.hpp>
#include <eepp/gaming/ctilelayer.hpp>
#include <eepp/graphics/cprimitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {


cGameObjectObject::cGameObjectObject( Uint32 DataId, cLayer * Layer, const Uint32& Flags ) :
	cGameObject( Flags, Layer ),
	//mRect( Layer->Map()->GetObjPoly( DataId ) ),
	mPos( mRect.Pos() ),
	mDataId( DataId )
{
}

cGameObjectObject::cGameObjectObject( Uint32 DataId, const eeRecti& rect, cLayer * Layer, const Uint32& Flags ) :
	cGameObject( Flags, Layer ),
	mRect( rect.Left, rect.Top, rect.Right, rect.Bottom ),
	mPos( mRect.Pos() ),
	mDataId( DataId )
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

eeSize cGameObjectObject::Size() {
	return eeSize( mRect.Size().x, mRect.Size().y );
}

void cGameObjectObject::Draw() {
	cPrimitives P;
	P.FillMode( EE_DRAW_FILL );
	P.SetColor( eeColorA( 100, 100, 100, 30 ) );
	P.DrawRectangle( mRect );

	P.FillMode( EE_DRAW_LINE );
	P.SetColor( eeColorA( 50, 50, 50, 200 ) );
	P.DrawRectangle( mRect );
}

eeVector2f cGameObjectObject::Pos() const {
	return mPos;
}

void cGameObjectObject::Pos( eeVector2f pos ) {
	mPos	= pos;
	mRect	= eeRectf( pos, eeSizef( Size().x, Size().y ) );
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

}}
