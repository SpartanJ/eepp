#include "cshapegroup.hpp"

namespace EE { namespace Graphics {

cShapeGroup::cShapeGroup( const std::string& name ) :
	tResourceManager<cShape> ( true )
{
	Name( name );
}

cShapeGroup::~cShapeGroup() {
}

const std::string& cShapeGroup::Name() const {
	return mName;
}

void cShapeGroup::Name( const std::string& name ) {
	mName = name;
	mId = MakeHash( mName );
}

const Uint32& cShapeGroup::Id() const {
	return mId;
}

cShape * cShapeGroup::Add( cShape * Shape ) {
	return tResourceManager<cShape>::Add( Shape );
}

cShape * cShapeGroup::Add( const Uint32& TexId, const std::string& Name ) {
	return Add( eeNew( cShape, ( TexId, Name ) ) );
}

cShape * cShapeGroup::Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name ) {
	return Add( eeNew( cShape, ( TexId, SrcRect, Name ) ) );
}

cShape * cShapeGroup::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name ) {
	return Add( eeNew ( cShape, ( TexId, SrcRect, DestWidth, DestHeight, Name ) ) );
}

cShape * cShapeGroup::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name ) {
	return Add( eeNew ( cShape, ( TexId, SrcRect, DestWidth, DestHeight, OffsetX, OffsetY, Name ) ) );
}

Uint32 cShapeGroup::Count() {
	return tResourceManager<cShape>::Count();
}

}}
