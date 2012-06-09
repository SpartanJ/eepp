#include <eepp/graphics/cglobalbatchrenderer.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cGlobalBatchRenderer)

cGlobalBatchRenderer::cGlobalBatchRenderer() {
	AllocVertexs( 1024 );
}

cGlobalBatchRenderer::~cGlobalBatchRenderer() {
}

}}
