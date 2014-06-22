#include <eepp/graphics/globalbatchrenderer.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(GlobalBatchRenderer)

GlobalBatchRenderer::GlobalBatchRenderer() {
	AllocVertexs( 4096 );
}

GlobalBatchRenderer::~GlobalBatchRenderer() {
}

}}
