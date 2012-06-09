#include <eepp/graphics/cglobalshapegroup.hpp>
#include <eepp/graphics/cshapegroupmanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cGlobalShapeGroup)

cGlobalShapeGroup::cGlobalShapeGroup() :
	cShapeGroup( "global" )
{
}

cGlobalShapeGroup::~cGlobalShapeGroup() {
}

}}

