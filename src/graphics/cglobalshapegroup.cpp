#include "cglobalshapegroup.hpp"
#include "cshapegroupmanager.hpp"

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cGlobalShapeGroup)

cGlobalShapeGroup::cGlobalShapeGroup() :
	cShapeGroup( "global" )
{
}

cGlobalShapeGroup::~cGlobalShapeGroup() {
}

}}

