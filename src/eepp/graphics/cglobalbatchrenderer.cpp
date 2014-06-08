#include <eepp/graphics/cglobalbatchrenderer.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cGlobalBatchRenderer)

cGlobalBatchRenderer::cGlobalBatchRenderer() {
	AllocVertexs( 4096 );
}

cGlobalBatchRenderer::~cGlobalBatchRenderer() {
}

}}
