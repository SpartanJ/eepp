#include <eepp/graphics/cglobaltextureatlas.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cGlobalTextureAtlas)

cGlobalTextureAtlas::cGlobalTextureAtlas() :
	cTextureAtlas( "global" )
{
}

cGlobalTextureAtlas::~cGlobalTextureAtlas() {
}

}}
