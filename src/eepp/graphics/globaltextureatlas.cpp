#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(GlobalTextureAtlas)

GlobalTextureAtlas::GlobalTextureAtlas() :
	TextureAtlas( "global" )
{
}

GlobalTextureAtlas::~GlobalTextureAtlas() {
}

}}
