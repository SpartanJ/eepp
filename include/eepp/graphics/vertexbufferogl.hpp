#ifndef EE_GRAPHICSCVERTEXBUFFEROGL_HPP
#define EE_GRAPHICSCVERTEXBUFFEROGL_HPP

#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics {

/** The Vertex Buffer OGL is the implementation of a Vertex Buffer without using the specific VBOs
   extensions from OpenGL.
	@see VertexBuffer
*/
class EE_API VertexBufferOGL : public VertexBuffer {
  public:
	VertexBufferOGL( const Uint32& vertexFlags = VERTEX_FLAGS_DEFAULT,
					 PrimitiveType drawType = PRIMITIVE_QUADS, const Int32& reserveVertexSize = 0,
					 const Int32& reserveIndexSize = 0,
					 VertexBufferUsageType usageType = VertexBufferUsageType::Static );

	void bind();

	void draw();

	bool compile();

	void update( const Uint32& types, bool indices );

	void reload();

	void unbind();

  protected:
	void setVertexStates();
};

}} // namespace EE::Graphics

#endif
