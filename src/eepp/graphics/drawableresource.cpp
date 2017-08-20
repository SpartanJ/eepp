#include <eepp/graphics/drawableresource.hpp>

namespace EE { namespace Graphics {

DrawableResource::DrawableResource( EE_DRAWABLE_TYPE drawableType ) :
	Drawable( drawableType ),
	mId(0)
{
	createUnnamed();
}

DrawableResource::DrawableResource( EE_DRAWABLE_TYPE drawableType, const std::string& name ) :
	Drawable( drawableType ),
	mId(0)
{
	setName( name );
}

const Uint32& DrawableResource::getId() const {
	return mId;
}

const std::string DrawableResource::getName() const {
	return mName;
}

void DrawableResource::setName( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );
}

void DrawableResource::createUnnamed() {
	if ( !mName.size() )
		setName( std::string( "unnamed" ) );
}

}} 
